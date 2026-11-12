"""conq: drive, check and test the open-conqueror robot from the host computer.

Every command runs without a prompt (except `shell`), and --json prints one
JSON object for agents and scripts. The port is found automatically when only
one USB serial port is connected; otherwise give --port or set CONQ_PORT.

Opening the port restarts the UNO, so each command waits about 2 s first.
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
from .link import PortError, Robot, choose_port, usb_ports


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
    robot.wait_for_boot()
    return robot


# --- Commands ---------------------------------------------------------------

def cmd_ports(args: argparse.Namespace) -> int:
    ports = usb_ports()
    text = "\n".join(f"{p['port']}  {p['description']}" for p in ports) or "No USB serial ports."
    return output(args, {"ok": True, "ports": ports}, text)


def cmd_version(args: argparse.Namespace) -> int:
    robot = connect(args)
    try:
        firmware = robot.version()
    finally:
        robot.close()
    data = {"ok": firmware is not None, "port": robot.port, "firmware": firmware, "conq": __version__}
    text = (f"Firmware {firmware['version']} ({firmware['commit']}) on {robot.port}" if firmware
            else f"No answer from {robot.port}.")
    return output(args, data, text)


def cmd_status(args: argparse.Namespace) -> int:
    robot = connect(args)
    try:
        since = robot.now()
        time.sleep(args.seconds)
        samples = robot.telemetry(since)
    finally:
        robot.close()
    latest = samples[-1][1] if samples else None
    data = {"ok": latest is not None, "port": robot.port,
            "telemetry": latest.to_dict() if latest else None, "samples": len(samples)}
    return output(args, data, latest.describe() if latest else "No telemetry received.")


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
    }
    text = (f"Drove {args.direction} at {args.speed}% for {args.seconds} s. PWM seen: {data['pwm_seen']}. "
            f"Now: {final.describe() if final else 'no telemetry'}")
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
                         for t in tests)
        return output(args, data, text)

    interactive = sys.stdin.isatty() and not args.json
    moving = [t.name for t in tests if t.moves]
    if moving and not args.confirm_moves and not interactive:
        return output(args, {"ok": False, "error": "tests that move the robot need --confirm-moves",
                             "moving_tests": moving},
                      "These tests move the robot: " + ", ".join(moving) + ". Add --confirm-moves.")

    port = choose_port(args.port)
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
    text = "\n".join(f"{r['test']:<22} {r['status']}" for r in reports) + f"\nReport: {run_dir / 'report.json'}"
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
    p.add_argument("--seconds", type=float, default=0.5, help="how long to listen (default 0.5)")
    p.set_defaults(func=cmd_status)
    p = sub.add_parser("drive", parents=[common], help="drive in one direction for a while, then stop")
    p.add_argument("direction", choices=list(protocol.DIRECTIONS), help="f b l r fl fr bl br")
    p.add_argument("speed", type=int, help="1 to 100 percent")
    p.add_argument("seconds", type=float, help="how long to drive")
    p.set_defaults(func=cmd_drive)
    sub.add_parser("stop", parents=[common], help="brake both tracks").set_defaults(func=cmd_stop)
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
