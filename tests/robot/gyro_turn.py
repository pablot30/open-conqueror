"""Turning the robot by hand changes the heading by the same angle."""

REQUIREMENTS = ["F1", "T4"]
MOVES = False
SETUP = """Robot on a table with room to turn it, USB connected, Upload-Cam switch
on Upload, main power on. The USB cable must allow a quarter turn."""
BRIEF = """The tracks don't move: you turn the robot by hand. Keep it still for
the first 3 seconds while the gyro calibrates. Then, whenever you're ready
within 30 seconds, turn it a quarter turn to the LEFT (counterclockwise seen
from above, so its front ends up where its left side was), put it down, and
keep it still. The test waits for you to start and to stop."""

QUARTER = 9000  # hundredths of a degree
TOLERANCE = 1500  # 15 degrees: a turn by hand isn't exact
START_CDPS = 1000  # 10 degrees/s counts as turning
STILL_CDPS = 100   # under 1 degree/s counts as still
STILL_FOR_S = 1.5
WAIT_S = 30.0


def heading_change(start, end):
    return (end - start + 18000) % 36000 - 18000


def run(t):
    robot = t.robot
    valid = robot.wait_for(lambda: [s for _, s in robot.sensors() if s.gyro_status == 0], 5.0)
    if not valid:
        t.check("gyro calibrated", False, "no valid gyro reading", ["F1"])
        return
    t.wait(0.5)
    start_heading = [s for _, s in robot.sensors()][-1].heading_cdeg
    t.say("Turn now: a quarter turn to the LEFT, then keep the robot still.")
    t.step("waiting for the turn to start")
    began = robot.wait_for(lambda: [a for a, s in robot.sensors() if abs(s.rate_cdps) > START_CDPS], WAIT_S)
    if not began:
        t.check("turn seen", False, f"no turn within {WAIT_S} s", ["F1"])
        return
    t.step("waiting for the robot to be still again")
    def still_since():
        recent = [(a, s) for a, s in robot.sensors(began[0])]
        moving = [a for a, s in recent if abs(s.rate_cdps) > STILL_CDPS]
        last_move = moving[-1] if moving else began[0]
        return robot.now() - last_move >= STILL_FOR_S
    robot.wait_for(still_since, WAIT_S)
    end_heading = [s for _, s in robot.sensors()][-1].heading_cdeg
    change = heading_change(start_heading, end_heading)
    t.check("heading changed by about 90 degrees", abs(abs(change) - QUARTER) <= TOLERANCE,
            f"changed {change / 100:.1f} deg", ["F1", "T4"])
    t.check("a left turn counts as positive", change > 0,
            "positive means counterclockwise seen from above; if negative, the chip is mounted upside down "
            "and the sign must be flipped", ["F1"])
