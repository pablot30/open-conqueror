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
test names a rough place for the book: about 5, 10, 30, 60, 100 and 150 cm
from the front of the sensor. Stand the book upright, face square to the
robot, roughly there; it doesn't need to be exact. Press Enter when it's in
place and your hands are away; the test then reads for about 2 seconds and
says "got it" with its own reading. Then read the tape measure and type the
distance from the sensor's front to the book's face, to the nearest half
centimetre. It shows the error and lets you keep the result or redo that
distance, as often as you like. Type s to skip a distance that doesn't fit.
Each distance waits for you, with no time limit. About 5 minutes in all."""

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


def read_still(robot, since, seconds):
    """The mean distance over a window, or None if the target moved or no
    valid readings came."""
    samples = valid_distances(robot, since)
    samples = [d for at, d in samples if at <= since + seconds]
    if len(samples) < 6 or max(samples) - min(samples) > STILL_MM:
        return None
    return sum(samples) / len(samples)


def ask_number(prompt):
    """A number typed by the person, or a one-letter choice."""
    while True:
        typed = input(prompt).strip().lower().replace(",", ".")
        if typed in ("r", "s"):
            return typed
        try:
            return float(typed)
        except ValueError:
            print("Please type a number such as 29.5, or r or s.")


def measure_interactively(t, robot, cm):
    """One distance, at the person's pace. Returns (robot mm, tape cm), or None
    if skipped. The person can redo the distance as often as they like."""
    import time

    while True:
        typed = input(f"Place the book at about {cm} cm, keep your hands away, then press Enter "
                      "(s and Enter to skip this distance): ").strip().lower()
        if typed == "s":
            return None
        t.step(f"reading at about {cm} cm")
        mean = None
        for _ in range(3):
            start = robot.now()
            time.sleep(STILL_FOR_S + 0.3)
            mean = read_still(robot, start + 0.2, STILL_FOR_S)
            if mean is not None:
                break
            t.say("The readings moved or were missing; keep the book and your hands still. Reading again.")
        if mean is None:
            t.say("No steady reading. Let's try this distance again.")
            continue
        t.say(f"got it: the robot reads {mean / 10:.1f} cm")
        answer = ask_number("Tape measure, sensor's front to the book's face, in cm "
                            "(r to redo this distance, s to skip it): ")
        if answer == "s":
            return None
        if answer == "r":
            continue
        error = mean - answer * 10
        keep = input(f"Robot {mean / 10:.1f} cm, tape {answer:.1f} cm, error {error / 10:+.1f} cm. "
                     "Enter to keep, r to redo this distance: ").strip().lower()
        if keep == "r":
            continue
        return mean, answer


def run(t):
    robot = t.robot
    previous = None
    for cm in DISTANCES_CM:
        if t.interactive:
            point = measure_interactively(t, robot, cm)
            if point is None:
                t.say(f"skipped {cm} cm")
                t.results.append({"check": f"reading near {cm} cm", "status": "skipped",
                                  "detail": "skipped by the person", "requirements": ["T3"]})
                continue
            mean, actual_cm = point
        else:
            t.say(f"Put the book at about {cm} cm from the sensor's front, then keep still.")
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
                t.check(f"reading near {cm} cm", False, "no still, valid reading", ["T3"])
                continue
            previous = mean
            actual_cm = t.measure(f"tape_{cm}", "Read the tape: how far is the book's face from the sensor's front?")
            if actual_cm is None:
                continue
        error = mean - actual_cm * 10
        t.check(f"reading near {cm} cm", abs(error) <= TOLERANCE_MM,
                f"read {mean / 10:.1f} cm, tape {actual_cm:.1f} cm, error {error / 10:+.1f} cm "
                f"({100 * error / (actual_cm * 10):+.1f}%)", ["T3"])
