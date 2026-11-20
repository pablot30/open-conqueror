"""The connection over the robot's own Wi-Fi network (F11, S3), against a fake
camera board on this computer: no robot needed."""

import json
import socket
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest import mock

from conq import cli, link, network, protocol, robottest

ROOT = Path(__file__).resolve().parents[2]
TELEMETRY = b"{T,1000,0,0,0,7400,0}"


class FakeCameraBoard:
    """Listens on localhost like the camera board's port 100: sends what a
    test gives it, answers {V}, and keeps every byte the client sends."""

    def __init__(self, heartbeat_s: float | None = 0.1, telemetry: bool = True) -> None:
        self.server = socket.create_server(("127.0.0.1", 0))
        self.server.settimeout(0.05)
        self.port = f"tcp:127.0.0.1:{self.server.getsockname()[1]}"
        self.heartbeat_s = heartbeat_s
        self.telemetry = telemetry
        self.received = b""
        self.connections = 0
        self.silent = False
        self.client: socket.socket | None = None
        self._running = True
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()

    def _serve(self) -> None:
        last = 0.0
        while self._running:
            if self.client is None:
                try:
                    self.client, _ = self.server.accept()
                except OSError:
                    continue
                self.client.settimeout(0.02)
                self.connections += 1
            try:
                data = self.client.recv(256)
                if data == b"":
                    self.drop()
                    continue
                self.received += data
                if b"{V}" in data and not self.silent:
                    self.client.sendall(b"{V,0.1.0,abc1234,8}")
            except socket.timeout:
                pass
            except OSError:
                self.drop()
                continue
            if not self.silent and time.monotonic() - last >= (self.heartbeat_s or 0.1):
                last = time.monotonic()
                out = (b"{Heartbeat}" if self.heartbeat_s else b"") + (TELEMETRY if self.telemetry else b"")
                if out:
                    self.client.sendall(out)

    def send(self, data: bytes) -> None:
        assert self.client is not None
        self.client.sendall(data)

    def drop(self) -> None:
        if self.client is not None:
            self.client.close()
            self.client = None

    def stop(self) -> None:
        self._running = False
        self._thread.join(timeout=1)
        self.drop()
        self.server.close()


def wait_until(predicate, timeout: float = 2.0) -> bool:
    end = time.monotonic() + timeout
    while time.monotonic() < end:
        if predicate():
            return True
        time.sleep(0.01)
    return False


class AddressTests(unittest.TestCase):
    def test_addresses(self) -> None:
        self.assertEqual(network.parse_address("tcp:192.168.4.1:100"), ("192.168.4.1", 100))
        self.assertEqual(network.parse_address("tcp:192.168.4.1"), ("192.168.4.1", 100))
        self.assertTrue(network.is_network_port("TCP:192.168.4.1"))
        self.assertFalse(network.is_network_port("COM3"))
        for bad in ("tcp:", "tcp:192.168.4.1:x", "tcp:192.168.4.1:70000"):
            with self.assertRaises(ValueError):
                network.parse_address(bad)

    def test_network_port_is_not_the_dongle(self) -> None:
        with mock.patch.object(link, "usb_ports", side_effect=AssertionError("no USB lookup")):
            self.assertFalse(link.is_dongle("tcp:192.168.4.1:100"))


class NetworkLinkTests(unittest.TestCase):
    def setUp(self) -> None:
        self.board = FakeCameraBoard()
        self.addCleanup(self.board.stop)

    def open(self) -> link.Robot:
        robot = link.Robot(self.board.port)
        self.addCleanup(robot.close)
        return robot

    def test_heartbeats_are_answered_and_kept_from_the_robots_messages(self) -> None:
        robot = self.open()
        self.assertTrue(wait_until(lambda: self.board.received.count(b"{Heartbeat}") >= 3))
        self.assertTrue(robot.telemetry())
        bodies = [m.body for m in robot.messages if m.direction == "rx"]
        self.assertNotIn("Heartbeat", bodies)
        self.assertEqual(robot.link_summary()["via"], "network")
        self.assertTrue(robot.over_wifi)
        self.assertFalse(robot.via_dongle)

    def test_messages_reach_the_robot_and_stray_bytes_are_dropped(self) -> None:
        robot = self.open()
        robot.send(protocol.drive_message(30, 30))
        self.assertTrue(wait_until(lambda: b"{D,30,30}" in self.board.received))
        self.board.send(b"junk{K,0,12}more{" + b"x" * 200 + b"}")
        self.assertTrue(wait_until(lambda: robot.received(kind="K")))
        self.assertFalse(any("junk" in m.body or "xxx" in m.body for m in robot.received()))
        self.assertEqual(robot.version(), {"version": "0.1.0", "commit": "abc1234", "protocol": "8"})

    def test_silence_is_a_lost_connection_and_it_opens_again(self) -> None:
        with mock.patch.object(network, "LINK_TIMEOUT_S", 0.3), mock.patch.object(network, "REOPEN_PERIOD_S", 0.1):
            robot = self.open()
            robot.wait_until_ready()
            self.board.silent = True
            self.assertTrue(wait_until(lambda: robot.link_drops))
            self.assertFalse(robot.link.connected)
            notes = [m.body for m in robot.messages if m.direction == "note"]
            self.assertTrue(any("no message from the camera board for 0.3 s" in n for n in notes), notes)
            robot.send(protocol.STOP)  # dropped while down, without an error
            self.board.drop()
            self.board.silent = False
            self.assertTrue(wait_until(lambda: robot.link.connected and self.board.connections == 2))
            self.assertTrue(any("open again" in m.body for m in robot.messages if m.direction == "note"))

    def test_closed_connection_is_reported(self) -> None:
        robot = self.open()
        self.assertTrue(wait_until(lambda: self.board.client is not None))
        self.board.drop()
        self.assertTrue(wait_until(lambda: robot.link_drops))
        notes = " ".join(m.body for m in robot.messages if m.direction == "note")
        self.assertIn("the camera board closed the connection", notes)
        self.assertEqual(robot.link_summary()["status"], "connecting to the camera board")

    def test_close_sends_a_stop(self) -> None:
        robot = link.Robot(self.board.port)
        robot.close()
        self.assertTrue(wait_until(lambda: protocol.STOP in self.board.received))


    def test_a_reconnect_that_ends_after_close_is_discarded(self) -> None:
        changes = []
        net = network.NetworkLink(self.board.port, lambda up, reason: changes.append(up))
        net._lose("test")
        real = socket.create_connection

        def slow_connect(*args, **kwargs):
            connection = real(*args, **kwargs)
            net.close()  # Robot.close() gives up waiting while this reconnect runs
            return connection

        net._last_attempt = 0.0
        with mock.patch.object(network.socket, "create_connection", slow_connect):
            net.read()
        self.assertIsNone(net._socket)
        self.assertEqual(changes, [False])


class NoRobotTests(unittest.TestCase):
    def test_unreachable_board_says_what_to_check(self) -> None:
        unused = socket.create_server(("127.0.0.1", 0))
        port = f"tcp:127.0.0.1:{unused.getsockname()[1]}"
        unused.close()
        with self.assertRaises(link.PortError) as error:
            link.Robot(port)
        self.assertIn("ELEGOO-", str(error.exception))

    def test_no_telemetry_points_at_the_upload_cam_switch(self) -> None:
        board = FakeCameraBoard(telemetry=False)
        self.addCleanup(board.stop)
        robot = link.Robot(board.port)
        self.addCleanup(robot.close)
        with mock.patch.object(link, "NETWORK_READY_S", 0.3), self.assertRaises(link.PortError) as error:
            robot.wait_until_ready()
        self.assertIn("Upload-Cam switch on Cam", str(error.exception))


class CommandTests(unittest.TestCase):
    def setUp(self) -> None:
        self.board = FakeCameraBoard()
        self.addCleanup(self.board.stop)

    def test_status_over_the_network(self) -> None:
        with mock.patch("builtins.print") as printed:
            code = cli.main(["status", "--port", self.board.port, "--seconds", "0.3", "--json"])
        self.assertEqual(code, 0)
        data = json.loads(printed.call_args[0][0])
        self.assertEqual(data["telemetry"]["battery_mv"], 7400)
        self.assertEqual(data["link"]["via"], "network")

    def test_power_on_tests_are_skipped_over_the_network(self) -> None:
        test = robottest.load(ROOT / "tests" / "robot" / "version_and_standby.py")
        with tempfile.TemporaryDirectory() as folder:
            report = robottest.run_test(test, self.board.port, Path(folder) / "log.jsonl", {}, interactive=False)
        self.assertEqual(report["status"], "incomplete")
        self.assertEqual(report["results"][0]["check"], "fresh start")
        self.assertEqual(report["firmware"]["version"], "0.1.0")

    def test_report_holds_no_absolute_paths(self) -> None:
        """#89: report.json can be copied into docs/test-reports/ as it is."""
        test_file = ROOT / "tests" / "robot" / "version_and_standby.py"
        with tempfile.TemporaryDirectory() as folder, mock.patch("builtins.print"):
            code = cli.main(["test", str(test_file), "--port", self.board.port, "--log-dir", folder, "--json"])
            text = (Path(folder) / "report.json").read_text(encoding="utf-8")
            report = json.loads(text)["reports"][0]
            self.assertTrue((Path(folder) / report["log"]).is_file())
        self.assertEqual(code, 1)  # incomplete over the network, as above
        self.assertEqual(report["log"], "logs/version_and_standby.jsonl")
        self.assertEqual(report["file"], "tests/robot/version_and_standby.py")
        for absolute in (str(Path.home()), Path.home().as_posix(), folder, Path(folder).as_posix(), str(ROOT)):
            self.assertNotIn(absolute.replace("\\", "\\\\"), text)
        self.assertNotIn('"folder"', text)


if __name__ == "__main__":
    unittest.main()
