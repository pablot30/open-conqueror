"""Driving straight stays straight: heading hold keeps the heading within a couple of degrees over about 2 m."""

REQUIREMENTS = ["F1", "T4"]
MOVES = True
SETUP = """Robot on a flat floor with a clear lane at least 3 m long and 0.5 m wide,
battery charged. Best over Wi-Fi: the dongle in the computer, Upload-Cam
switch on Cam, and the robot switched on where it stands, then left still for
3 seconds while the gyro calibrates. Or with a LONG USB cable (3 m or more)
laid along the lane with slack, switch on Upload. A tape measure, and a strip
of tape across the floor under the robot's middle as a start mark."""
BRIEF = """The tracks move: the robot drives forward for 5 seconds at 40% and stops.
Forward is roughly 2 m depending on the battery. The test then asks you
to measure, and waits for your answer: how far to the side of a straight
line from its start mark the robot ended up. Leave it where it is until
you've answered. Then it drives backward for 5 seconds at 40% and stops. Stand beside the lane, not in front of it, and
keep a hand free to stop the robot with the MODE button (and, over USB, to
catch the cable). Over USB, let the robot sit still for the first 3 seconds
after you start the test: opening the port restarts it and the gyro
calibrates then, and the robot must not be touched. Lay the tape
measure along the lane before the test. Watch for the robot veering, and over USB for the cable pulling it. About 1 minute."""

DRIVE_S = 5.0
SPEED = 40
MAX_HEADING_CHANGE_CDEG = 300  # 3 degrees over the run
HOLDING_SHARE = 0.8


def heading_change(start, end):
    return (end - start + 18000) % 36000 - 18000


def one_run(t, name, left, right):
    from conq.protocol import NEUTRAL, drive_message

    robot = t.robot
    t.step(f"{name} for {DRIVE_S:.0f} s")
    robot.send(NEUTRAL)
    since = robot.now()
    robot.stream(drive_message(left, right), DRIVE_S)
    until = robot.now()
    robot.send(b"{S}")
    t.wait(1.0)
    samples = [s for at, s in robot.sensors(since + 0.3) if at <= until]
    return samples


def run(t):
    robot = t.robot
    valid = robot.wait_for(lambda: [s for _, s in robot.sensors() if s.gyro_status == 0], 6.0)
    if not valid:
        t.check("gyro valid", False, "no valid gyro reading", ["F1"])
        return
    t.wait(1.0)
    for name, speed in (("forward", SPEED), ("backward", -SPEED)):
        samples = one_run(t, name, speed, speed)
        if len(samples) < 8:
            t.check(f"{name}: sensor messages arrived", False, f"{len(samples)} messages", ["F1"])
            continue
        change = heading_change(samples[0].heading_cdeg, samples[-1].heading_cdeg)
        holding = [s for s in samples if s.hold == 1]
        corrections = [s.correction_permille for s in holding if s.correction_permille is not None]
        t.check(f"{name}: heading hold was working", len(holding) >= HOLDING_SHARE * len(samples),
                f"holding in {len(holding)} of {len(samples)} messages; states seen: "
                f"{sorted({s.hold_name for s in samples})}", ["F1"])
        t.check(f"{name}: heading changed little", abs(change) <= MAX_HEADING_CHANGE_CDEG,
                f"changed {change / 100:+.1f} deg (limit {MAX_HEADING_CHANGE_CDEG / 100:.0f}); "
                f"corrections {min(corrections, default=0) / 10:+.1f}% to {max(corrections, default=0) / 10:+.1f}%",
                ["F1", "T4"])
        t.check(f"{name}: correction stayed small", all(abs(c) <= 50 for c in corrections),
                "", ["F1"])
        if name == "forward":
            t.confirm("sideways_within_10cm",
                      "Measured from the start mark, did the robot stop within 10 cm of a straight line "
                      "(about 2 m forward)? Leave it where it is until you answer.", ["F1"])
