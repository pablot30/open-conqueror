"""Unit tests for the computer tool's message handling. Run with:
python -m unittest discover -s host
"""

import unittest

import oc


class MessageTests(unittest.TestCase):
    def test_drive_message(self) -> None:
        self.assertEqual(oc.drive_message(50, -100), b"{D,50,-100}")

    def test_drive_message_rejects_out_of_range(self) -> None:
        with self.assertRaises(ValueError):
            oc.drive_message(101, 0)

    def test_splitter_handles_split_and_noise(self) -> None:
        splitter = oc.MessageSplitter()
        self.assertEqual(splitter.feed(b"noise{T,1,"), [])
        self.assertEqual(splitter.feed(b"2}\n{V,0.1.0,abc}"), ["T,1,2", "V,0.1.0,abc"])

    def test_parse_telemetry(self) -> None:
        kind, values = oc.parse_message("T,1200,1,-91,182,7600,0,412")
        self.assertEqual(kind, "T")
        telemetry = oc.parse_telemetry(values)
        assert telemetry is not None
        self.assertEqual(telemetry.left_pwm, -91)
        self.assertEqual(telemetry.battery_mv, 7600)
        self.assertIn("driving", telemetry.describe())

    def test_parse_telemetry_rejects_bad_fields(self) -> None:
        self.assertIsNone(oc.parse_telemetry(["1", "2"]))
        self.assertIsNone(oc.parse_telemetry(["x"] * 7))


if __name__ == "__main__":
    unittest.main()
