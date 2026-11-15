"""The firmware reports its version, and starts with the motors off."""

from pathlib import Path

REQUIREMENTS = ["D4", "S5", "T1", "T2"]
MOVES = False
SETUP = "Robot connected over USB, Upload-Cam switch on Upload, main power on."
BRIEF = """Nothing moves. The robot restarts when the port opens, then the test
listens for 2 seconds. Nothing to do or watch."""

MAX_LOOP_US = 5000  # T2


def run(t):
    robot = t.robot
    firmware = robot.version()
    t.check("version reported", bool(firmware and firmware["version"] and firmware["commit"]),
            str(firmware), ["D4"])
    t.check("protocol version reported", bool(firmware) and firmware.get("protocol") == "8",
            f"protocol {firmware.get('protocol') if firmware else '-'}", ["D4"])
    version_file = Path(__file__).resolve().parents[2] / "VERSION"
    if firmware and version_file.is_file():
        expected = version_file.read_text(encoding="utf-8").strip()
        t.check("version matches the VERSION file", firmware["version"] == expected,
                f"robot {firmware['version']}, VERSION {expected}", ["D4"])

    since = robot.now()
    t.wait(2.0)
    samples = [s for _, s in robot.telemetry(since)]
    t.check("telemetry arrives 10 times a second", 18 <= len(samples) <= 22, f"{len(samples)} in 2 s", ["T1"])
    t.check("standby with no motor output", bool(samples) and all(
        s.state == 0 and s.left_pwm == 0 and s.right_pwm == 0 for s in samples), "", ["S5"])
    if samples:
        mv = samples[-1].battery_mv
        t.check("battery reading plausible with main power on", 6000 <= mv <= 8700, f"{mv} mV")
    health = [h for _, h in robot.health(since)]
    if health:
        longest = max(h.longest_frame_us for h in health)
        t.check("longest frame's work under 5 ms", longest < MAX_LOOP_US, f"{longest} us", ["T2"])
    else:
        t.check("health message received", False, "none in 2 s", ["T2"])
