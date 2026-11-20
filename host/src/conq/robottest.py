"""Robot tests: scripted checks that run on the real robot.

Each test is a Python file in tests/robot/ with these names:

    REQUIREMENTS  list of requirement IDs it checks, such as ["S1", "S3"]
    MOVES         True if the tracks will turn
    SETUP         how the robot must be set up before it starts
    BRIEF         what will happen, what the person does, and what to watch for
    run(t)        the test itself; t is a Context

and optionally:

    NEEDS_FRESH_START  True if it checks what happens at power-on. Only the USB
                       cable restarts the robot, so over Wi-Fi (the dongle or
                       the robot's network) the test is skipped and the run is
                       incomplete.

The test checks the robot's telemetry itself and asks the person only for what
a script can't see, such as which way a track turned. Answers can be given in
advance with --answer, so an agent can run a test after asking the person.
"""

from __future__ import annotations

import importlib.util
import sys
import time
import traceback
from dataclasses import dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from types import ModuleType
from typing import Any, Callable

from .link import Robot


class TestDefinitionError(Exception):
    pass


@dataclass
class RobotTest:
    name: str
    path: Path
    requirements: list[str]
    moves: bool
    setup: str
    brief: str
    run: Callable[["Context"], None]
    needs_fresh_start: bool = False

    def describe(self) -> dict[str, Any]:
        return {"name": self.name, "file": self.path.as_posix(), "requirements": self.requirements,
                "moves": self.moves, "needs_fresh_start": self.needs_fresh_start,
                "setup": self.setup, "brief": self.brief}


def load(path: Path) -> RobotTest:
    spec = importlib.util.spec_from_file_location(f"conq_robot_test_{path.stem}", path)
    if spec is None or spec.loader is None:
        raise TestDefinitionError(f"{path}: not a Python file")
    module: ModuleType = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    missing = [name for name in ("REQUIREMENTS", "MOVES", "SETUP", "BRIEF", "run") if not hasattr(module, name)]
    if missing:
        raise TestDefinitionError(f"{path}: missing {', '.join(missing)}")
    requirements = list(module.REQUIREMENTS)
    if not requirements or not all(isinstance(r, str) for r in requirements):
        raise TestDefinitionError(f"{path}: REQUIREMENTS must list requirement IDs")
    return RobotTest(path.stem, path, requirements, bool(module.MOVES),
                     str(module.SETUP).strip(), str(module.BRIEF).strip(), module.run,
                     bool(getattr(module, "NEEDS_FRESH_START", False)))


def relative_path(path: Path, base: Path) -> str:
    """The path relative to base, with forward slashes on every system, or
    only the file name if it lies outside base, so no home folder is kept."""
    try:
        return path.resolve().relative_to(base.resolve()).as_posix()
    except ValueError:
        return path.name


def find_tests(directory: Path) -> list[Path]:
    return sorted(p for p in directory.glob("*.py") if not p.name.startswith("_"))


@dataclass
class Context:
    robot: Robot
    answers: dict[str, str]
    interactive: bool
    results: list[dict[str, Any]] = field(default_factory=list)

    def say(self, text: str) -> None:
        print(text, file=sys.stderr, flush=True)

    def step(self, text: str) -> None:
        """Marks the start of a step, in the log and on screen."""
        self.robot.note(text)
        self.say(f"- {text}")

    def check(self, name: str, passed: bool, detail: str = "", requirements: list[str] | None = None) -> bool:
        self.results.append({"check": name, "status": "passed" if passed else "failed",
                             "detail": detail, "requirements": requirements or []})
        self.say(f"  {'PASS' if passed else 'FAIL'} {name}{': ' + detail if detail else ''}")
        return passed

    def confirm(self, key: str, question: str, requirements: list[str] | None = None) -> bool | None:
        """Asks the person a yes/no question and records it as a check.
        Unanswered questions leave the test incomplete rather than failed."""
        answer = self.answers.get(key)
        if answer is None and self.interactive:
            answer = input(f"{question} [y/n] ").strip()
        if answer is None:
            self.results.append({"check": key, "status": "unanswered", "detail": question,
                                 "requirements": requirements or []})
            self.say(f"  ASK  {key}: {question} (give it with --answer {key}=yes)")
            return None
        yes = answer.lower() in ("y", "yes", "true", "1")
        self.results.append({"check": key, "status": "passed" if yes else "failed",
                             "detail": f"{question} Answer: {answer}", "requirements": requirements or []})
        return yes

    def measure(self, key: str, question: str, unit: str = "cm") -> float | None:
        """Asks the person for a measured number, such as a distance read off a
        tape measure. Returns None, and leaves the test incomplete, if there's
        no answer."""
        answer = self.answers.get(key)
        while answer is None and self.interactive:
            typed = input(f"{question} [{unit}] ").strip().replace(",", ".")
            try:
                float(typed)
                answer = typed
            except ValueError:
                print("Please type a number, such as 29.5.")
        if answer is None:
            self.results.append({"check": key, "status": "unanswered", "detail": question, "requirements": []})
            self.say(f"  ASK  {key}: {question} (give it with --answer {key}=<number>)")
            return None
        return float(answer.replace(",", "."))

    def wait(self, seconds: float) -> None:
        time.sleep(seconds)


def run_test(test: RobotTest, port: str, log_path: Path, answers: dict[str, str],
             interactive: bool) -> dict[str, Any]:
    started = datetime.now(timezone.utc)
    robot = Robot(port, log_path)
    context = Context(robot, answers, interactive)
    firmware = None
    try:
        robot.wait_until_ready()
        firmware = robot.version()
        if test.needs_fresh_start and robot.over_wifi:
            context.results.append({"check": "fresh start", "status": "skipped", "requirements": [],
                                    "detail": "checks power-on, and only the USB cable restarts the robot"})
        else:
            test.run(context)
    except KeyboardInterrupt:
        context.results.append({"check": "run", "status": "failed", "detail": "interrupted", "requirements": []})
    except Exception:  # a broken test is a failed test, with the reason kept
        context.results.append({"check": "run", "status": "failed",
                                "detail": traceback.format_exc(limit=3), "requirements": []})
    finally:
        robot.close()
    if robot.link_drops:
        context.results.append({"check": "Wi-Fi link stayed up", "status": "skipped", "requirements": [],
                                "detail": f"the Wi-Fi link lost the robot at {robot.link_drops} s; "
                                          "checks after that may be missing"})

    statuses = {r["status"] for r in context.results}
    if "failed" in statuses or not context.results:
        status = "failed"
    elif "unanswered" in statuses or "skipped" in statuses:
        status = "incomplete"  # nothing failed, but not everything was checked
    else:
        status = "passed"
    return {
        "test": test.name,
        "file": test.path.as_posix(),
        "requirements": test.requirements,
        "status": status,
        "firmware": firmware,
        "port": port,
        "link": robot.link_summary(),
        "started": started.isoformat(timespec="seconds"),
        "duration_s": round((datetime.now(timezone.utc) - started).total_seconds(), 1),
        "results": context.results,
        # No folder, so a report never carries a home folder (#89); `conq test`
        # replaces it with the path relative to report.json.
        "log": log_path.name,
    }
