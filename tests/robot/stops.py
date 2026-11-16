"""Stops work and hold: the stop command, the link timeout, and arming."""

REQUIREMENTS = ["F4", "S1", "S3", "S4", "S5"]
MOVES = True
SETUP = """Robot on a table or the floor with room to move about 10 cm each way,
or with the tracks lifted. USB connected, switch on Upload, main power on."""
BRIEF = """The robot drives forward for half a second and is stopped by command, then
backward for half a second and stopped by going quiet on the link, then it
returns. Between moves the computer keeps sending a 1% drive command, which
is too weak to turn the tracks; it checks that a stop holds. About 10
seconds. Nothing to do; watch that the robot stops each time."""

CREEP = b"{D,1,1}"  # about 2 of 255 PWM: the tracks don't turn
STOP_WITHIN_S = 0.25  # S1 asks for 150 ms; telemetry comes every 100 ms


def stopped(sample):
    return sample.state == 2 and sample.left_pwm == 0 and sample.right_pwm == 0


def first_stop_after(robot, since):
    for at, sample in robot.telemetry(since):
        if stopped(sample):
            return at, sample
    return None, None


def protocol_state(state):
    from conq.protocol import STATES

    return STATES.get(state, str(state))


def run(t):
    from conq.protocol import NEUTRAL, STOP, drive_message

    robot = t.robot

    t.step("drive commands before any neutral command")
    start = robot.now()
    robot.stream(CREEP, 0.6)
    samples = [s for _, s in robot.telemetry(start + 0.15)]
    # Standby after a restart over USB; a held stop over the Wi-Fi dongle, where
    # the robot doesn't restart and the camera board sends stops until it connects.
    states = sorted({protocol_state(s.state) for s in samples})
    t.check("ignored until a neutral command",
            bool(samples) and all(s.state in (0, 2) and s.left_pwm == 0 and s.right_pwm == 0 for s in samples),
            "robot " + ", ".join(states), ["S4", "S5"])

    t.step("forward, then the stop command")
    robot.send(NEUTRAL)
    robot.stream(drive_message(30, 30), 0.5)
    sent = robot.now()
    robot.send(STOP)
    robot.stream(CREEP, 1.0)
    at, _ = first_stop_after(robot, sent)
    t.check("stop command brakes", at is not None and at - sent <= STOP_WITHIN_S,
            f"stopped telemetry {round((at - sent) * 1000) if at else '-'} ms after the command", ["F4", "S1"])
    held = [s for _, s in robot.telemetry(sent + STOP_WITHIN_S + 0.1)]
    t.check("stop holds while drive commands keep coming", bool(held) and all(stopped(s) for s in held),
            "", ["S1"])
    robot.drive_for(-30, -30, 0.5)  # return
    t.wait(0.8)

    t.step("backward, then silence on the link")
    robot.send(NEUTRAL)
    robot.stream(drive_message(-30, -30), 0.5)
    last = robot.now()
    t.wait(0.8)
    at, sample = first_stop_after(robot, last)
    t.check("link timeout stops after about 300 ms",
            at is not None and 0.2 <= at - last <= 0.45,
            f"stopped telemetry {round((at - last) * 1000) if at else '-'} ms after the last message", ["S3"])
    t.check("link timeout fault reported", sample is not None and bool(sample.faults & 0x01),
            f"faults {sample.faults if sample else '-'}", ["S3"])
    resumed = robot.now()
    robot.stream(CREEP, 1.0)
    held = [s for _, s in robot.telemetry(resumed + 0.15)]
    t.check("timeout stop holds when messages return", bool(held) and all(stopped(s) for s in held), "", ["S3"])
    robot.drive_for(30, 30, 0.5)  # return
    t.wait(0.8)

    t.step("a neutral command re-arms")
    armed = robot.now()
    robot.send(NEUTRAL)
    robot.stream(CREEP, 0.5)
    robot.send(STOP)
    samples = [s for _, s in robot.telemetry(armed + 0.15, armed + 0.5)]
    t.check("drives again after neutral", bool(samples) and all(s.state == 1 for s in samples), "", ["S1"])
