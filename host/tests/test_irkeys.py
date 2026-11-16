"""Recording the IR remote's keys, with a fake robot."""

import unittest
from unittest import mock

from conq import irkeys, protocol


class FakeRobot:
    """Answers each request for a key with the next scripted code."""

    def __init__(self, codes: list[list[int] | None]) -> None:
        self.codes = codes
        self.asked = 0
        self.sent: list[protocol.IrCode] = []
        self.clock = 0.0

    def now(self) -> float:
        return self.clock

    def ir_codes(self, since: float = 0.0):
        return [(self.clock, c) for c in self.sent]

    def wait_for(self, predicate, timeout, poll=0.02):
        if timeout < 1.0:  # the quiet time between keys
            return None
        self.clock += 1.0
        code = self.codes[self.asked]
        self.asked += 1
        self.sent = [] if code is None else [protocol.IrCode(*code)]
        return predicate()


class RecordTests(unittest.TestCase):
    def test_records_each_key_and_formats_the_table(self) -> None:
        robot = FakeRobot([[0xFF00, 0x18, 0], [0xFF00, 0x52, 0], None])
        said: list[str] = []
        codes = irkeys.record_keys(robot, ["up", "down", "ok"], said.append)  # type: ignore[arg-type]
        self.assertEqual(codes["up"], {"address": 0xFF00, "command": 0x18})
        self.assertIsNone(codes["ok"])
        self.assertIn("Press and release the UP key", said[0])
        self.assertEqual(irkeys.table_lines(codes), [
            "    {Key::kUp, 0xFF00U, 0x18U, true},",
            "    {Key::kDown, 0xFF00U, 0x52U, true},",
            "// ok: not recorded"])

    def test_unreadable_pulses_are_called_out(self) -> None:
        robot = FakeRobot([None])
        said: list[str] = []
        robot.sent = []
        with mock.patch.object(robot, "ir_codes", return_value=[(0.0, protocol.IrCode(40, 0, 2))]):
            self.assertIsNone(irkeys.record_key(robot, "up", said.append))  # type: ignore[arg-type]
        self.assertTrue(any("couldn't read" in line for line in said))

    def test_number_keys_are_comments_for_now(self) -> None:
        lines = irkeys.table_lines({"1": {"address": 0xFF00, "command": 0x0C}})
        self.assertTrue(lines[0].startswith("// key 1:"))


if __name__ == "__main__":
    unittest.main()
