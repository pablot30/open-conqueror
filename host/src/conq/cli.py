"""conq: drive, check and test the open-conqueror robot from the host computer.

Every command runs without a prompt (except `shell`), and --json prints one
JSON object for agents and scripts. The port is found automatically when only
one USB serial port is connected; otherwise give --port or set CONQ_PORT.

The port is either the robot's own USB cable or the Wi-Fi dongle, an Arduino
UNO R4 WiFi (firmware/dongle/), which conq recognises by its USB IDs. Opening
the robot's USB port restarts the UNO, so each command waits about 2 s first.
Over the dongle the robot doesn't restart, and commands report if the Wi-Fi
link drops.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from datetime import datetime
from pathlib import Path
from typing import Any

from . import __version__, protocol, robottest
from .link import PortError, Robot, choose_port, is_dongle, usb_ports


def repo_root() -> Path:
    """The repository this command runs in, found from the current folder,
    or from where the package lives when installed in editable mode."""
    for start in (Path.cwd(), Path(__file__).resolve()):
        for folder in (start, *start.parents):
            if (folder / "VERSION").is_file() and (folder / "tests" / "robot").is_dir():
                return folder
    return Path.cwd()


def output(args: argparse.Namespace, data: dict[str, Any], text: str) -> int:
    if args.json:
        print(json.dumps(data, indent=2))
    else:
        print(text)
    return 0 if data.get("ok", True) else 1


def connect(args: argparse.Namespace) -> Robot:
    robot = Robot(choose_port(args.port))
    try:
        robot.wait_until_ready()
    except PortError:
        robot.close()
        raise
    return robot


def link_note(robot: Robot) -> str:
    """A line for people when the Wi-Fi link dropped during a command."""
    if not robot.link_drops:
        return ""
    times = ", ".join(f"{t:.1f} s" for t in robot.link_drops)
    return f"\nWarning: the Wi-Fi dongle lost the robot at {times}; some messages may be missing."


# --- Commands ---------------------------------------------------------------

def cmd_ports(args: argparse.Namespace) -> int:
    ports = usb_ports()
    text = "\n".join(f"{p['port']}  {p['description']}{'  (Wi-Fi dongle)' if p['dongle'] else ''}"
                     for p in ports) or "No USB serial ports."
    return output(args, {"ok": True, "ports": ports}, text)


def cmd_version(args: argparse.Namespace) -> int:
    robot = connect(args)
    try:
        firmware = robot.version()
        dongle = robot.dongle_version() if robot.via_dongle else None
    finally:
        robot.close()
    data = {"ok": firmware is not None, "port": robot.port, "firmware": firmware, "conq": __version__,
            "link": robot.link_summary(), "dongle": dongle}
    text = (f"Firmware {firmware['version']} ({firmware['commit']}) on {robot.port}" if firmware
            else f"No answer from {robot.port}.")
    if robot.via_dongle:
        text += (f"\nThrough the Wi-Fi dongle {dongle['version']} ({dongle['commit']}), "
                 f"{robot.link.describe() if robot.link else 'link state unknown'}" if dongle
                 else "\nThrough the Wi-Fi dongle, which didn't report its version")
    text += link_note(robot)
    return output(args, data, text)


def cmd_status(args: argparse.Namespace) -> int:
    robot = connect(args)
    try:
        since = robot.now()
        time.sleep(args.seconds)
        samples = robot.telemetry(since)
        health = robot.health()
        sensors = robot.sensors(since)
    finally:
        robot.close()
    latest = samples[-1][1] if samples else None
    last_health = health[-1][1] if health else None
    last_sensors = sensors[-1][1] if sensors else None
    data = {"ok": latest is not None, "port": robot.port,
            "telemetry": latest.to_dict() if latest else None, "samples": len(samples),
            "health": last_health.to_dict() if last_health else None,
            "sensors": last_sensors.to_dict() if last_sensors else None,
            "link": robot.link_summary()}
    text = latest.describe() if latest else "No telemetry received."
    if last_sensors:
        text += "\nSensors: " + last_sensors.describe()
    if last_health:
        text += "\nHealth: " + last_health.describe()
    if robot.via_dongle and robot.link:
        text += "\nWi-Fi: " + robot.link.describe()
    text += link_note(robot)
    return output(args, data, text)


def cmd_drive(args: argparse.Namespace) -> int:
    left, right = protocol.direction_speeds(args.direction, args.speed)
    robot = connect(args)
    try:
        start = robot.now()
        robot.drive_for(left, right, args.seconds)
        time.sleep(0.3)
        samples = robot.telemetry(start)
    finally:
        robot.close()
    driving = [t for _, t in samples if t.state == 1]
    final = samples[-1][1] if samples else None
    data = {
        "ok": bool(driving) and final is not None and final.state != 1,
        "command": {"direction": args.direction, "speed": args.speed, "seconds": args.seconds,
                    "left": left, "right": right},
        "pwm_seen": sorted({(t.left_pwm, t.right_pwm) for t in driving}),
        "final": final.to_dict() if final else None,
        "link": robot.link_summary(),
    }
    text = (f"Drove {args.direction} at {args.speed}% for {args.seconds} s. PWM seen: {data['pwm_seen']}. "
            f"Now: {final.describe() if final else 'no telemetry'}") + link_note(robot)
    return output(args, data, text)


def camera_wait(pan: int, tilt: int, speed: int | None) -> float:
    """Seconds to stay connected: a move from the centre at this speed, plus the settle time."""
    if speed == 0:
        return 0.7
    dps = 100 if speed is None else speed
    return max(abs(pan - 90), abs(tilt - 90)) / dps + 0.7


def cmd_camera(args: argparse.Namespace) -> int:
    message = protocol.camera_message(args.pan, args.tilt, args.speed)
    robot = connect(args)
    try:
        robot.send(message)
        time.sleep(camera_wait(args.pan, args.tilt, args.speed))  # the servos move, then settle
    finally:
        robot.close()
    return output(args, {"ok": True, "pan": args.pan, "tilt": args.tilt, "speed": args.speed},
                  f"Camera sent to pan {args.pan}, tilt {args.tilt}.")


def cmd_ir(args: argparse.Namespace) -> int:
    from . import irkeys

    robot = connect(args)
    try:
        if args.action == "listen":
            since = robot.now()
            time.sleep(args.seconds)
            codes = [c for _, c in robot.ir_codes(since)]
            data = {"ok": True, "port": robot.port, "codes": [c.to_dict() for c in codes]}
            return output(args, data, "\n".join(c.describe() for c in codes) or "No IR codes received.")
        names = args.keys or (irkeys.REQUIRED + irkeys.OPTIONAL)
        say = lambda text: print(text, file=sys.stderr, flush=True)  # noqa: E731
        codes_by_key = irkeys.record_keys(robot, names, say)
    finally:
        robot.close()
    missing = [n for n in irkeys.REQUIRED if n in names and codes_by_key.get(n) is None]
    lines = irkeys.table_lines(codes_by_key)
    data = {"ok": not missing, "port": robot.port, "keys": codes_by_key, "missing": missing, "table": lines}
    text = ("Paste these into firmware/uno/devices/ir/src/ir_keys.cpp:\n" + "\n".join(lines)
            + (f"\nMissing: {', '.join(missing)}" if missing else ""))
    return output(args, data, text)


def cmd_stop(args: argparse.Namespace) -> int:
    robot = Robot(choose_port(args.port))
    robot.close()  # close() sends a stop
    return output(args, {"ok": True, "port": robot.port}, f"Stop sent on {robot.port}.")


def cmd_shell(args: argparse.Namespace) -> int:
    from .shell import Shell

    robot = connect(args)
    try:
        Shell(robot).cmdloop()
    except KeyboardInterrupt:
        pass
    finally:
        robot.close()
    return 0


def parse_answers(pairs: list[str]) -> dict[str, str]:
    answers = {}
    for pair in pairs:
        key, sep, value = pair.partition("=")
        if not sep:
            raise ValueError(f"--answer needs KEY=VALUE, got {pair}")
        answers[key.strip()] = value.strip()
    return answers


def cmd_test(args: argparse.Namespace) -> int:
    root = repo_root()
    paths = [Path(p) for p in args.tests] or robottest.find_tests(root / "tests" / "robot")
    tests = [robottest.load(p) for p in paths]

    if args.list:
        data = {"ok": True, "tests": [t.describe() for t in tests]}
        text = "\n".join(f"{t.name:<22} {'moves ' if t.moves else 'still '} {', '.join(t.requirements)}"
                         + ("  (USB only: checks power-on)" if t.needs_fresh_start else "")
                         for t in tests)
        return output(args, data, text)

    interactive = sys.stdin.isatty() and not args.json
    moving = [t.name for t in tests if t.moves]
    if moving and not args.confirm_moves and not interactive:
        return output(args, {"ok": False, "error": "tests that move the robot need --confirm-moves",
                             "moving_tests": moving},
                      "These tests move the robot: " + ", ".join(moving) + ". Add --confirm-moves.")

    port = choose_port(args.port)
    if is_dongle(port):
        print("Over the Wi-Fi dongle: Upload-Cam switch on Cam, not Upload, and no cable to the robot. "
              "Tests that check power-on are skipped.", file=sys.stderr, flush=True)
    answers = parse_answers(args.answer)
    run_dir = Path(args.log_dir) if args.log_dir else (
        root / "build" / "robot-tests" / datetime.now().strftime("%Y%m%d-%H%M%S"))
    run_dir.mkdir(parents=True, exist_ok=True)

    reports = []
    for test in tests:
        print(f"\n== {test.name} ({', '.join(test.requirements)})\nSetup: {test.setup}\n{test.brief}",
              file=sys.stderr, flush=True)
        if interactive and (test.moves and not args.confirm_moves):
            input("Press Enter when ready, or Ctrl+C to stop. ")
        report = robottest.run_test(test, port, run_dir / f"{test.name}.jsonl", answers, interactive)
        reports.append(report)
        print(f"   -> {report['status']}", file=sys.stderr, flush=True)

    summary = {
        "ok": all(r["status"] == "passed" for r in reports),
        "status": ("passed" if all(r["status"] == "passed" for r in reports)
                   else "failed" if any(r["status"] == "failed" for r in reports) else "incomplete"),
        "conq": __version__,
        "reports": reports,
        "folder": run_dir.as_posix(),
    }
    (run_dir / "report.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    text = "\n".join(f"{r['test']:<22} {r['status']}" + (f"  ({r['results'][0]['detail']})"
                     if r["results"] and r["results"][0]["check"] == "fresh start" else "")
                     for r in reports) + f"\nReport: {run_dir / 'report.json'}"
    return output(args, summary, text)


# --- Arguments --------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="conq", description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", action="version", version=f"conq {__version__}")
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--port", help="serial port, such as COM3 or /dev/ttyUSB0")
    common.add_argument("--json", action="store_true", help="print one JSON object")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("ports", parents=[common], help="list USB serial ports").set_defaults(func=cmd_ports)
    sub.add_parser("version", parents=[common], help="the firmware's version and commit").set_defaults(
        func=cmd_version)
    p = sub.add_parser("status", parents=[common], help="the robot's latest telemetry")
    p.add_argument("--seconds", type=float, default=1.2,
                   help="how long to listen (default 1.2, long enough for a health message)")
    p.set_defaults(func=cmd_status)
    p = sub.add_parser("drive", parents=[common], help="drive in one direction for a while, then stop")
    p.add_argument("direction", choices=list(protocol.DIRECTIONS), help="f b l r fl fr bl br")
    p.add_argument("speed", type=int, help="1 to 100 percent")
    p.add_argument("seconds", type=float, help="how long to drive")
    p.set_defaults(func=cmd_drive)
    p = sub.add_parser("ir", parents=[common], help="the IR remote: record its key codes, or listen")
    p.add_argument("action", choices=["record", "listen"])
    p.add_argument("--keys", nargs="*", help="keys to record (default: up down left right ok 1 to 6)")
    p.add_argument("--seconds", type=float, default=10.0, help="how long to listen (default 10)")
    p.set_defaults(func=cmd_ir)
    sub.add_parser("stop", parents=[common], help="brake both tracks").set_defaults(func=cmd_stop)
    p = sub.add_parser("camera", parents=[common], help="point the camera; 90 90 is the centre")
    p.add_argument("pan", type=int, help="0 to 180 degrees")
    p.add_argument("tilt", type=int, help="0 to 180 degrees")
    p.add_argument("--speed", type=int, metavar="DPS",
                   help="degrees a second, 0 to 999; 0 is as fast as possible (default 100)")
    p.set_defaults(func=cmd_camera)
    sub.add_parser("shell", parents=[common], help="interactive prompt, for people").set_defaults(func=cmd_shell)
    p = sub.add_parser("test", parents=[common], help="run robot tests from tests/robot/")
    p.add_argument("tests", nargs="*", help="test files; all of tests/robot/ if none")
    p.add_argument("--list", action="store_true", help="list the tests without running them")
    p.add_argument("--answer", action="append", default=[], metavar="KEY=VALUE",
                   help="answer a test's question in advance, such as tracks_correct=yes")
    p.add_argument("--confirm-moves", action="store_true",
                   help="confirm the tracks are clear for tests that move the robot")
    p.add_argument("--log-dir", help="where to write logs and the report (default build/robot-tests/<time>)")
    p.set_defaults(func=cmd_test)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        return int(args.func(args))
    except (PortError, ValueError, robottest.TestDefinitionError) as error:
        if getattr(args, "json", False):
            print(json.dumps({"ok": False, "error": str(error)}, indent=2))
        else:
            print(f"conq: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
