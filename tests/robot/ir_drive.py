"""The IR remote's arrows drive while held and stop on release; OK stops and the stop holds."""

REQUIREMENTS = ["F10", "S2", "S3"]
MOVES = True
SETUP = """Robot on a table at least 40 cm from every edge, tracks clear, or better,
on the floor. USB connected, Upload-Cam switch on Upload, main power on. The
key table in firmware/uno/devices/ir/src/ir_keys.cpp must be filled in from
the ir_keys test and the firmware flashed with it, or the arrows do nothing.
The remote in your hand, aimed at the robot's front."""
BRIEF = """The tracks move, so keep your hand near the robot's rear and be ready to
catch it. For each of UP, DOWN, LEFT and RIGHT in turn, the test names the
key; press and hold that key for about 1 second, then let go. UP and DOWN
move the robot 10 to 20 cm forward and back; LEFT and RIGHT spin it on the
spot. It should stop about a third of a second after you let go. Then the
computer drives the robot forward slowly for up to 10 seconds, and you
press OK on the remote once: the robot must stop at once and stay stopped
while the computer keeps sending drive commands for 2 seconds. Watch for any
move after you let go or after OK. About 2 minutes."""

STEPS = [("UP", 1, 1), ("DOWN", -1, -1), ("LEFT", -1, 1), ("RIGHT", 1, -1)]
WAIT_S = 20.0


def moved(samples, sign_left, sign_right):
    return [s for _, s in samples if s.state == 1 and s.left_pwm * sign_left > 0 and s.right_pwm * sign_right > 0]


def run(t):
    from conq.protocol import NEUTRAL, drive_message

    robot = t.robot
    for name, left, right in STEPS:
        t.say(f"Press and hold {name} for about 1 second, then let go.")
        t.step(f"waiting for {name}")
        asked = robot.now()
        began = robot.wait_for(lambda: robot.telemetry(asked) and moved(robot.telemetry(asked), left, right), WAIT_S)
        if not began:
            seen = [c.describe() for _, c in robot.ir_codes(asked)]
            t.check(f"{name} drives", False,
                    "the robot didn't drive the right way; codes seen: " + ("; ".join(seen[:3]) or "none")
                    + ". Is the key table filled in and flashed?", ["F10"])
            continue
        released = robot.wait_for(lambda: [c for _, c in robot.ir_codes(asked) if c.kind == 3], WAIT_S)
        robot.wait_for(lambda: False, 0.5)
        samples = robot.telemetry(asked)
        t.check(f"{name} drives while held", True, f"{len(moved(samples, left, right))} samples driving", ["F10"])
        t.check(f"{name} stops when the key is released", bool(released) and samples[-1][1].state == 2,
                "no release message" if not released else "", ["F10"])
        t.check(f"{name} release is not a link fault", all("link timeout" not in s.fault_names for _, s in samples),
                "", ["S3"])
        t.confirm(f"{name.lower()}_stopped_quickly", f"Did the robot stop within about a third of a second of letting go of {name}?",
                  ["F10"])
        t.wait(0.5)

    t.step("OK stops a drive from the computer")
    t.say("The robot is about to drive forward slowly. Press OK on the remote once, within 10 seconds.")
    robot.send(NEUTRAL)
    started = robot.now()
    stopped_at = None
    end = started + 10.0
    while robot.now() < end:
        robot.send(drive_message(25, 25))
        t.wait(0.1)
        stop = [at for at, s in robot.telemetry(started + 0.3) if s.state == 2]
        if stop:
            stopped_at = stop[0]
            break
    t.check("OK stops the robot", stopped_at is not None, "no stop within 10 s", ["F10", "S2"])
    if stopped_at is not None:
        t.step("the computer keeps driving; the stop must hold")
        robot.stream(drive_message(25, 25), 2.0)
        after = [s for _, s in robot.telemetry(stopped_at + 0.15)]
        t.check("the stop holds", bool(after) and all(s.state == 2 and s.left_pwm == 0 and s.right_pwm == 0 for s in after),
                "", ["S2"])
    robot.send(b"{S}")
