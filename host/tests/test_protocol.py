"""Unit tests for the link protocol as the host computer sees it."""

import unittest

from conq import protocol


class MessageTests(unittest.TestCase):
    def test_drive_message(self) -> None:
        self.assertEqual(protocol.drive_message(50, -100), b"{D,50,-100}")

    def test_drive_message_rejects_out_of_range(self) -> None:
        with self.assertRaises(ValueError):
            protocol.drive_message(101, 0)

    def test_camera_message(self) -> None:
        self.assertEqual(protocol.camera_message(60, 120), b"{P,60,120}")
        with self.assertRaises(ValueError):
            protocol.camera_message(181, 90)

    def test_camera_message_with_speed(self) -> None:
        self.assertEqual(protocol.camera_message(60, 120, 50), b"{P,60,120,50}")
        self.assertEqual(protocol.camera_message(60, 120, 0), b"{P,60,120,0}")
        for speed in (-1, 1000):
            with self.assertRaises(ValueError):
                protocol.camera_message(60, 120, speed)

    def test_direction_speeds(self) -> None:
        self.assertEqual(protocol.direction_speeds("f", 30), (30, 30))
        self.assertEqual(protocol.direction_speeds("bl", 30), (-15, -30))
        with self.assertRaises(ValueError):
            protocol.direction_speeds("up", 30)
        with self.assertRaises(ValueError):
            protocol.direction_speeds("f", 0)

    def test_splitter_handles_split_and_noise(self) -> None:
        splitter = protocol.MessageSplitter()
        self.assertEqual(splitter.feed(b"noise{T,1,"), [])
        self.assertEqual(splitter.feed(b"2}\n{V,0.1.0,abc}"), ["T,1,2", "V,0.1.0,abc"])

    def test_parse_telemetry(self) -> None:
        kind, values = protocol.parse_message("T,1200,1,-91,182,7600,1,412")
        self.assertEqual(kind, "T")
        telemetry = protocol.parse_telemetry(values)
        assert telemetry is not None
        self.assertEqual(telemetry.left_pwm, -91)
        self.assertEqual(telemetry.battery_mv, 7600)
        self.assertEqual(telemetry.fault_names, ["link timeout"])
        self.assertIn("driving", telemetry.describe())
        self.assertEqual(telemetry.to_dict()["state_name"], "driving")

    def test_parse_telemetry_protocol_2(self) -> None:
        telemetry = protocol.parse_telemetry("1200,0,0,0,7400,0,900,0".split(","))
        assert telemetry is not None
        self.assertEqual(telemetry.battery_status_name, "valid")
        self.assertIn("(valid)", telemetry.describe())

    def test_parse_health(self) -> None:
        health = protocol.parse_health(["0", "4", "4", "1"])
        assert health is not None
        self.assertEqual(health.events, {"battery sensor": 4, "frame overrun": 4, "link timeout": 1})
        sensors = protocol.parse_sensors(["9000", "-150", "0"])
        assert sensors is not None
        self.assertEqual(sensors.gyro_status_name, "valid")
        self.assertIn("90.0 deg", sensors.describe())
        self.assertEqual(health.to_dict()["events"]["link timeout"], "prefailed")
        self.assertIsNone(protocol.parse_health([]))
        self.assertIsNone(protocol.parse_health(["x"]))

    def test_parse_telemetry_rejects_bad_fields(self) -> None:
        self.assertIsNone(protocol.parse_telemetry(["1", "2"]))
        self.assertIsNone(protocol.parse_telemetry(["x"] * 7))


if __name__ == "__main__":
    unittest.main()
