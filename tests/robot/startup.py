"""At power-on: the gyro calibrates, the camera centres, the LED breathes violet."""

REQUIREMENTS = ["F5", "F13", "F14", "T6", "T4"]
MOVES = False
NEEDS_FRESH_START = True  # over the Wi-Fi dongle the robot doesn't restart
SETUP = """Robot standing still on a table, USB connected, Upload-Cam switch on
Upload, main power on. Don't touch the robot or the table while it starts."""
BRIEF = """The tracks don't move. Opening the port restarts the robot. Watch two
things during the first seconds: the camera on its pan-tilt mount turns to its
centre position and stops, and the LED on top of the robot slowly breathes
violet, brighter and dimmer. The test then listens for 10 seconds while the
robot stands still. Afterwards it asks whether the camera ended up facing
straight ahead, and whether the LED breathed violet."""

READY_WITHIN_S = 3.0  # T6
MAX_STILL_RATE_CDPS = 50  # 0.5 degrees/s, T4
MAX_DRIFT_CDEG = 300  # 3 degrees in 10 s while still


def heading_change(start, end):
    """The shortest signed change between two headings, in hundredths of a degree."""
    return (end - start + 18000) % 36000 - 18000


def run(t):
    robot = t.robot
    telemetry = robot.telemetry()
    sensors = robot.sensors()
    if not telemetry:
        t.check("telemetry after startup", False, "none received")
        return
    first_at, first = telemetry[0]
    boot_at = first_at - first.ms / 1000.0  # host time of the robot's power-on
    valid = [(at, s) for at, s in robot.wait_for(
        lambda: [(a, s) for a, s in robot.sensors() if s.gyro_status == 0], 5.0) or []]
    if not valid:
        t.check("gyro calibrated", False, f"no valid gyro reading; last: {sensors[-1][1].describe() if sensors else '-'}",
                ["F13"])
        return
    ready = valid[0][0] - boot_at
    t.check("gyro calibrated and valid within 3 s of power-on", ready <= READY_WITHIN_S,
            f"valid {ready:.2f} s after power-on", ["F13", "T6"])

    t.step("standing still for 10 s")
    start = robot.now()
    t.wait(10.0)
    still = [s for _, s in robot.sensors(start) if s.gyro_status == 0]
    if len(still) < 2:
        t.check("sensor messages while still", False, f"{len(still)} received")
        return
    worst = max(abs(s.rate_cdps) for s in still)
    t.check("rate near zero while still", worst <= MAX_STILL_RATE_CDPS, f"largest {worst / 100:.2f} deg/s", ["T4"])
    drift = heading_change(still[0].heading_cdeg, still[-1].heading_cdeg)
    t.check("heading steady while still", abs(drift) <= MAX_DRIFT_CDEG,
            f"moved {drift / 100:.2f} deg in about 10 s", ["T4"])

    t.confirm("camera_centred", "Did the camera turn to its centre and end up facing straight ahead?", ["F14"])
    t.confirm("led_breathing", "Did the LED on top breathe violet, slowly brighter and dimmer?", ["F5"])
