"""The camera points where it's told, and centres at power-on."""

import time

REQUIREMENTS = ["F14"]
MOVES = False
SETUP = "Robot connected over USB, Upload-Cam switch on Upload, main power on."
BRIEF = """The tracks don't move; only the camera on its pan-tilt mount does. Watch
it. It turns to its left, then its right, then back to the centre, then tilts
up, down and back to the centre, about 2 seconds per position. Afterwards the
test asks whether it moved each time and ended up facing straight ahead."""

POSITIONS = [(135, 90, "left"), (45, 90, "right"), (90, 90, "centre"),
             (90, 60, "up or down"), (90, 120, "the other way"), (90, 90, "centre")]


def run(t):
    from conq.protocol import camera_message

    robot = t.robot
    for pan, tilt, name in POSITIONS:
        t.step(f"camera to pan {pan}, tilt {tilt} ({name})")
        robot.send(camera_message(pan, tilt))
        time.sleep(2.0)
    errors = [m for m in robot.received(kind="E")]
    t.check("commands understood", not errors, f"{len(errors)} errors")
    t.confirm("camera_moved", "Did the camera pan left, right and back, then tilt both ways and back?", ["F14"])
    t.confirm("camera_centred", "Did it end up facing straight ahead?", ["F14"])
