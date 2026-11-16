"""Requirement → test → result, read from the repo's own files (decision 0007).

Used by `python scripts/dev.py trace`. It reads files only: no robot, no network.

Sources:
- docs/requirements.md: the requirement IDs, and the build order that puts each
  in an increment.
- Unit tests: the comment on the line where a test function starts names the
  requirements it checks, such as `void startsInStandby() {  // S5`. Section
  names from the architecture, such as A5, may appear too; they aren't
  requirements. See firmware/testing/README.md.
- tests/robot/*.py: each robot test's REQUIREMENTS list.
- docs/test-reports/*/report.json: the robot results, newest folder last.
"""

from __future__ import annotations

import ast
import json
import re
import shutil
import subprocess
from pathlib import Path
from typing import Any

ID = re.compile(r"\b([A-Z]{1,2}\d+)\b")
ARCHITECTURE_SECTION = re.compile(r"^A\d+$")
TEST_FUNCTION = re.compile(r"^void (\w+)\(\) \{[ \t]*//(.*)$", re.MULTILINE)
NOT_PASSED = {"failed": 3, "incomplete": 2, "skipped": 1, "unanswered": 1}


def requirement_ids(text: str) -> list[str]:
    """Requirement IDs in the order their table rows appear in requirements.md."""
    return re.findall(r"^\| ([A-Z]{1,2}\d+) \|", text, flags=re.MULTILINE)


def build_order(text: str, repeated: list[str] | None = None) -> dict[str, Any]:
    """Maps each requirement ID to its increment: a number, "every" or "later".
    IDs found in more than one row are added to `repeated`, if given."""
    section = text.split("## Build order", 1)[-1].split("\n## ", 1)[0]
    increments: dict[str, Any] = {}
    for line in section.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 2 or cells[0] in ("Increment", "") or set(cells[0]) <= set("-"):
            continue
        name = cells[0].strip("*").strip()
        number = re.match(r"(\d+)\.", name)
        key: Any = int(number.group(1)) if number else name.split()[0].lower()
        for requirement in ID.findall(cells[1]):
            if requirement in increments and repeated is not None:
                repeated.append(requirement)
            increments[requirement] = key
    return increments


def unit_test_tags(root: Path) -> list[dict[str, Any]]:
    """Every tagged test function: its unit, name and the IDs on its first line."""
    tags = []
    for path in sorted(root.glob("firmware/*/*/*/tests/*.cpp")):
        unit = path.parent.parent.name
        for match in TEST_FUNCTION.finditer(path.read_text(encoding="utf-8")):
            ids = [i for i in ID.findall(match.group(2)) if not ARCHITECTURE_SECTION.match(i)]
            if ids:
                tags.append({"unit": unit, "test": match.group(1), "ids": ids,
                             "file": path.relative_to(root).as_posix()})
    return tags


def robot_test_tags(root: Path) -> dict[str, list[str]]:
    """Each robot test's REQUIREMENTS, read without importing the test."""
    tags = {}
    for path in sorted((root / "tests" / "robot").glob("*.py")):
        for node in ast.parse(path.read_text(encoding="utf-8")).body:
            if (isinstance(node, ast.Assign) and any(getattr(t, "id", "") == "REQUIREMENTS" for t in node.targets)
                    and isinstance(node.value, ast.List)):
                tags[path.stem] = [e.value for e in node.value.elts if isinstance(e, ast.Constant)]
    return tags


def robot_results(root: Path) -> list[tuple[str, dict[str, Any]]]:
    """(report folder, report.json) pairs, oldest first. Folders start with their date."""
    reports = []
    for path in sorted((root / "docs" / "test-reports").glob("*/report.json")):
        reports.append((path.parent.relative_to(root).as_posix(), json.loads(path.read_text(encoding="utf-8"))))
    return reports


def latest_result(requirement: str, reports: list[tuple[str, dict[str, Any]]]) -> dict[str, Any] | None:
    """The newest report that checked this requirement, and its worst status for it."""
    for folder, report in reversed(reports):
        statuses, firmware = [], None
        for test in report.get("reports", []):
            checks = [c for c in test.get("results", []) if requirement in c.get("requirements", [])]
            if checks:
                statuses += [c["status"] for c in checks]
            elif requirement in test.get("requirements", []):
                statuses.append(test.get("status", "unknown"))
            else:
                continue
            firmware = firmware or test.get("firmware")
        if statuses:
            worst = max(statuses, key=lambda s: NOT_PASSED.get(s, 0 if s == "passed" else 2))
            status = "incomplete" if worst in ("skipped", "unanswered") else worst
            return {"status": status, "report": folder, "firmware": firmware}
    return None


def commits_since(root: Path, commit: str, path: str = "firmware/uno") -> int | None:
    git = shutil.which("git")
    if git is None or not commit:
        return None
    proc = subprocess.run([git, "rev-list", "--count", f"{commit}..HEAD", "--", path], cwd=root,
                          text=True, capture_output=True, check=False)
    return int(proc.stdout.strip()) if proc.returncode == 0 and proc.stdout.strip().isdigit() else None


def trace(root: Path, increment: int | None = None, since=commits_since) -> dict[str, Any]:
    text = (root / "docs" / "requirements.md").read_text(encoding="utf-8")
    known = requirement_ids(text)
    repeated: list[str] = []
    order = build_order(text, repeated)
    unit_tags = unit_test_tags(root)
    robot_tags = robot_test_tags(root)
    reports = robot_results(root)

    unknown = [{"id": i, "where": f"{t['file']}: {t['test']}"} for t in unit_tags for i in t["ids"] if i not in known]
    unknown += [{"id": i, "where": f"tests/robot/{name}.py"} for name, ids in robot_tags.items()
                for i in ids if i not in known]

    rows = []
    for requirement in known:
        if increment is not None and order.get(requirement) != increment:
            continue
        unit_tests = [{"unit": t["unit"], "test": t["test"]} for t in unit_tags if requirement in t["ids"]]
        robot_tests = [name for name, ids in robot_tags.items() if requirement in ids]
        result = latest_result(requirement, reports)
        flags = []
        if not unit_tests and not robot_tests:
            flags.append("no test")
        if robot_tests and result is None:
            flags.append("no result")
        if result and result["status"] != "passed":
            flags.append("not passed")
        if result and result.get("firmware"):
            count = since(root, result["firmware"].get("commit", ""))
            if count:
                flags.append(f"possibly stale: {count} firmware commits since")
        rows.append({"id": requirement, "increment": order.get(requirement), "unit_tests": unit_tests,
                     "robot_tests": robot_tests, "latest_robot_result": result, "flags": flags})
    build_order_problems = [f"{i} is in no row of the build order" for i in known if i not in order]
    build_order_problems += [f"{i} is in more than one row of the build order" for i in repeated]
    build_order_problems += [f"{i} is in the build order but isn't a requirement" for i in order if i not in known]
    return {"ok": True, "increment": increment, "requirements": rows, "unknown_ids": unknown,
            "build_order_problems": build_order_problems}


def as_table(result: dict[str, Any]) -> str:
    """The trace as a Markdown table, in the shape a test report uses."""
    lines = ["| Requirement | Unit tests | Robot tests | Latest robot result | Flags |", "|---|---|---|---|---|"]
    for row in result["requirements"]:
        units: dict[str, int] = {}
        for test in row["unit_tests"]:
            units[test["unit"]] = units.get(test["unit"], 0) + 1
        unit_text = ", ".join(f"{unit} ({count})" for unit, count in units.items()) or "none"
        robot_text = ", ".join(f"`{name}`" for name in row["robot_tests"]) or "none"
        r = row["latest_robot_result"]
        result_text = "none"
        if r:
            commit = (r.get("firmware") or {}).get("commit", "?")
            result_text = f"{r['status'].capitalize()}, {Path(r['report']).name}, `{commit}`"
        lines.append(f"| {row['id']} | {unit_text} | {robot_text} | {result_text} | {'; '.join(row['flags'])} |")
    notes = [f"Unknown requirement {item['id']} in {item['where']}" for item in result["unknown_ids"]]
    notes += result["build_order_problems"]
    return "\n".join(lines + ([""] + notes if notes else []))
