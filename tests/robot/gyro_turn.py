"""Turning the robot by hand changes the heading by the same angle."""

REQUIREMENTS = ["F1", "T4"]
MOVES = False
SETUP = """Robot on a table with room to turn it, USB connected, Upload-Cam switch
on Upload, main power on. The USB cable must allow a quarter turn."""
BRIEF = """The tracks don't move: you turn the robot by hand. After the gyro has
calibrated (about 2 seconds, keep the robot still), you'll see "Turn now". Then,
within 10 seconds, lift or slide the robot a quarter turn to the LEFT
(counterclockwise seen from above, so its front ends up where its left side
was), put it down, and keep it still until the test ends."""

QUARTER = 9000  # hundredths of a degree
TOLERANCE = 1500  # 15 degrees: a turn by hand isn't exact


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
    t.step("turn by hand")
    t.wait(12.0)
    end_heading = [s for _, s in robot.sensors()][-1].heading_cdeg
    change = heading_change(start_heading, end_heading)
    t.check("heading changed by about 90 degrees", abs(abs(change) - QUARTER) <= TOLERANCE,
            f"changed {change / 100:.1f} deg", ["F1", "T4"])
    t.check("a left turn counts as positive", change > 0,
            "positive means counterclockwise seen from above; if negative, the chip is mounted upside down "
            "and the sign must be flipped", ["F1"])
