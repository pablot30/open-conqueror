"""The MODE button stops the robot, and the stop holds."""

REQUIREMENTS = ["S1"]
MOVES = False
SETUP = "Robot connected over USB, Upload-Cam switch on Upload, main power on."
BRIEF = """The tracks won't move: the computer sends a 1% drive command, too weak to
turn them, so a stop is easy to see in the telemetry. Within the next 2
minutes, press the MODE button 3 times, a few seconds apart. It's the small
push button on the board on top of the UNO, not a key on the IR remote."""

CREEP = b"{D,1,1}"
PRESSES = 3
WAIT_S = 120
HOLD_S = 1.5


def run(t):
    from conq.protocol import NEUTRAL

    robot = t.robot
    t.say("Press MODE now, 3 times, a few seconds apart.")
    robot.send(NEUTRAL)
    end = robot.now() + WAIT_S
    presses = 0
    armed_at = robot.now()
    while presses < PRESSES and robot.now() < end:
        robot.send(CREEP)
        t.wait(0.1)
        # Only telemetry newer than the last re-arm counts, and only after the
        # robot has been seen driving again.
        recent = robot.telemetry(armed_at)
        if not any(s.state == 1 for _, s in recent):
            continue
        last_driving = max(at for at, s in recent if s.state == 1)
        pressed = [at for at, s in recent if s.state == 2 and at > last_driving]
        if not pressed:
            continue
        presses += 1
        at = pressed[0]
        robot.stream(CREEP, HOLD_S)
        held = [s for _, s in robot.telemetry(at)]
        t.check(f"press {presses} stops and holds", all(s.state == 2 and s.left_pwm == 0 for s in held),
                f"held for {HOLD_S} s of drive commands", ["S1"])
        robot.send(NEUTRAL)
        armed_at = robot.now()
    t.check("presses seen", presses > 0, f"{presses} of {PRESSES} within {WAIT_S} s", ["S1"])
