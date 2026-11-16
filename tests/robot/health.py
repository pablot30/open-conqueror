"""Diagnostics, first iteration: the health message and signal status (A5)."""

REQUIREMENTS = ["T1", "T2", "S9"]
MOVES = False
SETUP = "Robot connected over USB, Upload-Cam switch on Upload, main power on."
BRIEF = """Nothing moves and there's nothing to do. The test listens for 5 seconds
to the health message, which comes once a second, and to the battery's signal
status in telemetry."""


def run(t):
    from conq.protocol import EVENTS

    robot = t.robot
    since = robot.now()
    t.wait(5.0)
    health = [h for _, h in robot.health(since)]
    t.check("health message once a second", 4 <= len(health) <= 6, f"{len(health)} in 5 s")
    if not health:
        return
    last = health[-1]
    t.check("every event listed", list(last.events) == EVENTS, str(last.to_dict()["events"]))
    t.check("battery sensor passed", last.events.get("battery sensor") == 4, "", ["S9"])
    t.check("no frame overruns", last.events.get("frame overrun") == 4, "", ["T2"])
    t.check("no frames skipped", last.skipped_frames == 0, f"{last.skipped_frames} skipped", ["T1"])
    t.check("link timeout not failed while idle", last.events.get("link timeout") == 4)

    t.check("battery reading valid", last.battery_status == 0, f"status {last.battery_status_name}", ["S9"])
    t.check("longest frame's work under 5 ms", last.longest_frame_us < 5000, f"{last.longest_frame_us} us", ["T2"])
