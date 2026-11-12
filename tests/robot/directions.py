"""Every direction drives the right tracks, at the expected motor output."""

REQUIREMENTS = ["F2", "F3", "F4", "S10"]
MOVES = True
SETUP = """Robot on a table or the floor with room to move about 10 cm each way,
or with the tracks lifted. USB connected, switch on Upload, main power on."""
BRIEF = """The tracks turn at 30% for half a second at a time, each move followed by
its opposite, so the robot ends near where it started: forward, back, spin
left, spin right, forward-left, back-left, forward-right, back-right. About
20 seconds in all. Watch from behind the robot, so its left is your left, and
check that each move turns the tracks the way its name says."""

SPEED = 30
DURATION = 0.5
ORDER = ["f", "b", "l", "r", "fl", "bl", "fr", "br"]
TOLERANCE = 3  # PWM counts: the battery reading moves while driving
MAX_MOTOR_MV = 6000  # S10


def expected_pwm(percent, battery_mv):
    limit = 255 if battery_mv <= MAX_MOTOR_MV else (255 * MAX_MOTOR_MV) // battery_mv
    return int(percent * limit / 100)


def run(t):
    from conq.protocol import direction_speeds

    robot = t.robot
    for name in ORDER:
        left, right = direction_speeds(name, SPEED)
        t.step(f"drive {name}: {left}, {right}")
        start = robot.now()
        robot.drive_for(left, right, DURATION)
        end = robot.now()
        t.wait(1.0)
        driving = [s for _, s in robot.telemetry(start + 0.15, end) if s.state == 1]
        if not driving:
            t.check(f"{name}: drove", False, "no driving telemetry", ["F3"])
            continue
        sample = driving[-1]
        want_left = expected_pwm(left, sample.battery_mv)
        want_right = expected_pwm(right, sample.battery_mv)
        ok = abs(sample.left_pwm - want_left) <= TOLERANCE and abs(sample.right_pwm - want_right) <= TOLERANCE
        t.check(f"{name}: motor output", ok,
                f"PWM {sample.left_pwm}, {sample.right_pwm}; expected about {want_left}, {want_right} "
                f"at {sample.battery_mv} mV", ["F2", "F3", "S10"])
        stopped = [s for _, s in robot.telemetry(end + 0.25) if s.state == 2 and s.left_pwm == 0]
        t.check(f"{name}: stopped after", bool(stopped), "", ["F4"])

    t.confirm("tracks_correct", "Did every move turn the tracks the way its name says?", ["F2", "F3"])
