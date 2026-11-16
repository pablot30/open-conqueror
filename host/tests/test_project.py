"""Checks that keep the host package, the robot tests and the docs in step."""

import os
import re
import sys
import tempfile
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
