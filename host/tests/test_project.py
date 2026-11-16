"""Checks that keep the host package, the robot tests and the docs in step."""

import os
import re
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

from conq import __version__, cli, link, robottest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import traceability  # noqa: E402  # one reader for requirement IDs, shared with dev.py trace


def requirement_ids() -> set[str]:
    return set(traceability.requirement_ids((ROOT / "docs" / "requirements.md").read_text(encoding="utf-8")))


class VersionTests(unittest.TestCase):
    def test_package_version_matches_pyproject(self) -> None:
        pyproject = (ROOT / "host" / "pyproject.toml").read_text(encoding="utf-8")
        self.assertIn(f'version = "{__version__}"', pyproject)


class RobotTestDefinitionTests(unittest.TestCase):
    """Every robot test loads, and names only requirements that exist (traceability)."""

    def test_robot_tests_are_valid(self) -> None:
        known = requirement_ids()
        self.assertTrue(known, "no requirement IDs found in docs/requirements.md")
        paths = robottest.find_tests(ROOT / "tests" / "robot")
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(test=path.name):
                test = robottest.load(path)
                unknown = set(test.requirements) - known
                self.assertFalse(unknown, f"unknown requirement IDs {unknown}")
                self.assertTrue(test.setup and test.brief)
                # Checks inside the test may only name IDs the test declares.
                used = set(re.findall(r'"([A-Z]{1,2}\d+)"', path.read_text(encoding="utf-8")))
                self.assertFalse(used - set(test.requirements), f"undeclared IDs {used - set(test.requirements)}")


class ContextTests(unittest.TestCase):
    def test_unanswered_question_leaves_test_incomplete(self) -> None:
        context = robottest.Context(robot=mock.Mock(), answers={}, interactive=False)
        self.assertIsNone(context.confirm("tracks_correct", "Right way?"))
        self.assertEqual(context.results[-1]["status"], "unanswered")

    def test_answer_given_in_advance(self) -> None:
        context = robottest.Context(robot=mock.Mock(), answers={"tracks_correct": "yes"}, interactive=False)
        self.assertTrue(context.confirm("tracks_correct", "Right way?"))
        self.assertEqual(context.results[-1]["status"], "passed")


class PortTests(unittest.TestCase):
    def test_explicit_port_wins(self) -> None:
        self.assertEqual(link.choose_port("COM7"), "COM7")

    def test_environment_variable(self) -> None:
        with mock.patch.dict(os.environ, {"CONQ_PORT": "/dev/ttyACM9"}):
            self.assertEqual(link.choose_port(None), "/dev/ttyACM9")

    def test_single_usb_port_is_chosen(self) -> None:
        with mock.patch.dict(os.environ, {}, clear=True), \
                mock.patch.object(link, "usb_ports", return_value=[{"port": "/dev/cu.usb1"}]):
            self.assertEqual(link.choose_port(None), "/dev/cu.usb1")

    def test_several_ports_need_a_choice(self) -> None:
        ports = [{"port": "a"}, {"port": "b"}]
        with mock.patch.dict(os.environ, {}, clear=True), mock.patch.object(link, "usb_ports", return_value=ports):
            with self.assertRaises(link.PortError):
                link.choose_port(None)


class FakeSerial:
    def __init__(self, *_: object, **__: object) -> None:
        self.baudrate = _[1] if len(_) > 1 else None
        self.written: list[bytes] = []

    def read(self, _: int) -> bytes:
        time.sleep(0.01)
        return b""

    def write(self, data: bytes) -> None:
        self.written.append(data)

    def close(self) -> None:
        pass


class DongleTests(unittest.TestCase):
    def ports(self, vid: int, pid: int) -> list[mock.Mock]:
        port = mock.Mock(device="/dev/cu.usbmodem1", description="board", vid=vid, pid=pid)
        return [port]

    def test_dongle_is_recognised_by_its_usb_ids(self) -> None:
        with mock.patch("serial.tools.list_ports.comports", return_value=self.ports(0x2341, 0x1002)):
            self.assertTrue(link.is_dongle("/dev/cu.usbmodem1"))
            self.assertTrue(link.usb_ports()[0]["dongle"])
        with mock.patch("serial.tools.list_ports.comports", return_value=self.ports(0x1A86, 0x7523)):
            self.assertFalse(link.is_dongle("/dev/cu.usbmodem1"))

    def test_dongle_opens_at_its_own_rate_and_reports_drops(self) -> None:
        with mock.patch("serial.Serial", FakeSerial) as _:
            robot = link.Robot("/dev/cu.usbmodem1", dongle=True)
            try:
                self.assertEqual(robot._serial.baudrate, 115200)
                robot._record("rx", "W,3,-40")
                robot._record("rx", "W,3,-41")
                self.assertEqual(robot.link_drops, [])
                robot._record("rx", "W,0,0")
                self.assertEqual(len(robot.link_drops), 1)
                self.assertTrue(any(m.direction == "note" and "Wi-Fi link lost" in m.body for m in robot.messages))
                summary = robot.link_summary()
                self.assertEqual(summary["via"], "dongle")
                self.assertEqual(summary["status"], "looking for the robot's network")
            finally:
                robot.close()

    def test_robot_usb_cable_keeps_9600(self) -> None:
        with mock.patch("serial.Serial", FakeSerial):
            robot = link.Robot("/dev/ttyUSB0", dongle=False)
            try:
                self.assertEqual(robot._serial.baudrate, 9600)
                self.assertEqual(robot.link_summary()["via"], "usb")
            finally:
                robot.close()

    def test_power_on_tests_are_skipped_over_the_dongle(self) -> None:
        test = robottest.load(ROOT / "tests" / "robot" / "version_and_standby.py")
        self.assertTrue(test.needs_fresh_start)
        fake = mock.Mock(via_dongle=True, link_drops=[])
        fake.link_summary.return_value = {"via": "dongle"}
        with tempfile.TemporaryDirectory() as folder, mock.patch.object(robottest, "Robot", return_value=fake):
            report = robottest.run_test(test, "x", Path(folder) / "log.jsonl", {}, interactive=False)
        self.assertEqual(report["status"], "incomplete")
        self.assertEqual(report["results"][0]["check"], "fresh start")
        fake.close.assert_called_once()

    def test_a_link_drop_leaves_a_test_incomplete(self) -> None:
        test = robottest.load(ROOT / "tests" / "robot" / "health.py")
        fake = mock.Mock(via_dongle=True, link_drops=[1.5])
        fake.link_summary.return_value = {"via": "dongle"}
        with tempfile.TemporaryDirectory() as folder, mock.patch.object(robottest, "Robot", return_value=fake), \
                mock.patch.object(test, "run", lambda t: t.check("x", True, "", [])):
            report = robottest.run_test(test, "x", Path(folder) / "log.jsonl", {}, interactive=False)
        self.assertEqual(report["status"], "incomplete")
        self.assertEqual(report["results"][-1]["check"], "Wi-Fi link stayed up")


class CliTests(unittest.TestCase):
    def test_list_robot_tests_as_json(self) -> None:
        with mock.patch("sys.stdout") as stdout:
            code = cli.main(["test", "--list", "--json"])
        self.assertEqual(code, 0)
        printed = "".join(call.args[0] for call in stdout.write.call_args_list)
        self.assertIn('"requirements"', printed)

    def test_moving_tests_need_confirmation_without_a_person(self) -> None:
        with tempfile.TemporaryDirectory() as folder, mock.patch("sys.stdout"), \
                mock.patch("sys.stdin.isatty", return_value=False):
            code = cli.main(["test", str(ROOT / "tests" / "robot" / "stops.py"), "--port", "x",
                             "--log-dir", folder])
        self.assertEqual(code, 1)


if __name__ == "__main__":
    unittest.main()
