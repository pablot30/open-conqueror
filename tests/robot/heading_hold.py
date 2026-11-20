"""Driving straight stays straight: heading hold keeps the heading within a couple of degrees and 10 cm over 2 m."""

REQUIREMENTS = ["F1", "T4"]
MOVES = True
SETUP = """Robot on a flat floor with a clear lane at least 3.5 m long and 0.5 m
wide, battery charged. Connected over the robot's own Wi-Fi (--port
tcp:192.168.4.1), Upload-Cam switch on Cam; a long USB cable (4 m or more)
laid along the lane with slack also works. A tape measure laid along the
lane, and a strip of tape across the floor under the robot's middle as a
start mark."""
BRIEF = """The tracks move: the robot drives forward for 14 seconds at 40% and
stops. That's about 2 m: on carpet it covered about 14 cm/s (measured on
2026-11-20); a hard floor may be faster, up to about 3 m. The test then
asks for two measurements and waits for each: how far the robot's middle
travelled along the lane, and how far it is to the side of the lane's
centre line. Leave it where it is until you've answered. Then it drives
backward for 14 seconds at 40% and stops. Stand beside the lane, not in
front of it, and keep a hand free to stop the robot with the MODE button
(and, over USB, to catch the cable). Over USB, let the robot sit still for
the first 3 seconds after you start the test: opening the port restarts it
and the gyro calibrates then, and the robot must not be touched. Watch for
the robot veering, and over USB for the cable pulling it. About 2 minutes."""

DRIVE_S = 14.0
SPEED = 40
MIN_FORWARD_CM = 200   # F1: drive 2 m
MAX_SIDEWAYS_CM = 10
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
            forward = t.measure("forward_cm", "Leave the robot where it is. Along the lane, how far did its "
                                "middle travel from the start mark?")
            sideways = t.measure("sideways_cm", "And how far is its middle to the side of the lane's "
                                 "centre line? (0 if you can't see any)")
            if forward is not None:
                t.check("forward: drove at least 2 m", forward >= MIN_FORWARD_CM,
                        f"{forward:.0f} cm in {DRIVE_S:.0f} s, about {forward / DRIVE_S:.0f} cm/s",
                        ["F1"])
            if sideways is not None:
                t.check("forward: stayed within 10 cm of a straight line",
                        abs(sideways) <= MAX_SIDEWAYS_CM, f"{sideways:.1f} cm to the side", ["F1"])
