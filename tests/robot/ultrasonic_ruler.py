"""The ultrasonic distance matches a ruler from 5 to 150 cm, within 1 cm."""

REQUIREMENTS = ["T3"]
MOVES = False
SETUP = """Robot on a table, facing a flat wall or a large book standing upright,
with a ruler or tape measure laid along the table in front of the ultrasonic
sensor. USB connected, Upload-Cam switch on Upload, main power on. The tracks
don't move. You need the book (or another hard, flat target at least 15 cm
wide) and about 1.6 m of clear table in front of the robot."""
BRIEF = """The tracks don't move, and the test only listens to the ultrasonic sensor,
the two round "eyes" on the front of the robot. For each distance in turn the
test tells you where to put the book: the book's flat face, upright and square
to the robot, at that distance from the front of the sensor, measured with the
ruler. The distances are 5, 10, 30, 60, 100 and 150 cm. Place the book when
told, then keep it and your hands still until the test says "got it". Each
distance waits up to 40 seconds for you, then reads for about 2 seconds. Keep
your hands out of the sensor's view while it reads. About 4 minutes in all."""

DISTANCES_CM = [5, 10, 30, 60, 100, 150]
TOLERANCE_MM = 10  # T3: within 1 cm
STILL_MM = 5       # readings this close together count as a still target
STILL_FOR_S = 2.0
WAIT_S = 40.0
MIN_PLACE_S = 5.0  # time to put the book down before a reading counts
MOVED_MM = 20      # a new position differs from the last by at least this much


def valid_distances(robot, since):
    return [(at, s.distance_mm) for at, s in robot.sensors(since)
            if s.distance_status == 0 and s.distance_mm is not None]


def run(t):
    robot = t.robot
    previous = None
    for cm in DISTANCES_CM:
        t.say(f"Put the book at {cm} cm from the sensor's front, then keep still.")
        t.step(f"waiting for a still target at {cm} cm")
        asked = robot.now()

        def still_target():
            samples = valid_distances(robot, max(asked, robot.now() - STILL_FOR_S))
            if len(samples) < 6 or robot.now() - asked < MIN_PLACE_S:
                return None
            values = [d for _, d in samples]
            mean = sum(values) / len(values)
            if max(values) - min(values) > STILL_MM:
                return None
            if previous is not None and abs(mean - previous) < MOVED_MM:
                return None  # still the last position
            return mean

        mean = robot.wait_for(still_target, WAIT_S)
        if mean is None:
            statuses = sorted({s.distance_status_name for _, s in robot.sensors(asked)})
            t.check(f"reading at {cm} cm", False,
                    f"no still valid reading within {WAIT_S:.0f} s (statuses seen: {', '.join(statuses) or 'none'})",
                    ["T3"])
            continue
        t.say("got it")
        previous = mean
        error = mean - cm * 10
        t.check(f"reading at {cm} cm", abs(error) <= TOLERANCE_MM,
                f"read {mean / 10:.1f} cm, error {error / 10:+.1f} cm", ["T3"])
