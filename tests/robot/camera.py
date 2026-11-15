"""The camera points where it's told, moves smoothly, and centres at power-on."""

import time

REQUIREMENTS = ["F14"]
MOVES = False
SETUP = "Robot connected over USB, Upload-Cam switch on Upload, main power on."
BRIEF = """The tracks don't move; only the camera on its pan-tilt mount does. Watch
the camera, which turns about 2 to 3 seconds per position. First it turns to its
left, then its right, then back to the centre, then tilts up, down and back to the
centre, each as a steady sweep at 100 degrees a second. Then it pans all the way
to the right and back to the centre twice: first as a 1-second sweep each way,
then as an instant jump each way, so you can compare them. Afterwards the test
asks whether the sweeps were smooth and whether the camera ended up facing
straight ahead."""

POSITIONS = [(135, 90, "left"), (45, 90, "right"), (90, 90, "centre"),
             (90, 60, "up or down"), (90, 120, "the other way"), (90, 90, "centre")]
SETTLE = 0.7  # the firmware releases the servos this long after they arrive


def wait_for(from_pan, from_tilt, pan, tilt, speed=100):
    """Seconds for a move to finish at `speed` degrees a second, plus the settle time."""
    if speed == 0:
        return SETTLE + 0.3
    return max(abs(pan - from_pan), abs(tilt - from_tilt)) / speed + SETTLE


def run(t):
    from conq.protocol import camera_message

    robot = t.robot
    pan, tilt = 90, 90
    for new_pan, new_tilt, name in POSITIONS:
        t.step(f"camera to pan {new_pan}, tilt {new_tilt} ({name}) at the default speed")
        robot.send(camera_message(new_pan, new_tilt))
        time.sleep(wait_for(pan, tilt, new_pan, new_tilt))
        pan, tilt = new_pan, new_tilt
    for speed, label in ((None, "the default speed"), (0, "speed 0, an instant jump")):
        t.step(f"camera pan all the way to the right, then back to the centre, at {label}")
        robot.send(camera_message(0, 90, speed))
        time.sleep(wait_for(90, 90, 0, 90, 0 if speed == 0 else 100) + 0.5)
        robot.send(camera_message(90, 90, speed))
        time.sleep(wait_for(0, 90, 90, 90, 0 if speed == 0 else 100) + 0.5)
    errors = [m for m in robot.received(kind="E")]
    t.check("commands understood", not errors, f"{len(errors)} errors")
    t.confirm("camera_moved", "Did the camera pan left, right and back, then tilt both ways and back?", ["F14"])
    t.confirm("camera_smooth", "Were the default-speed moves smooth?", ["F14"])
    t.confirm("camera_centred", "Did it end up facing straight ahead?", ["F14"])
