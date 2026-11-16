"""The script half of the docs check: what a script can decide about the docs.

Used by `python scripts/dev.py docs`. It reads the files git tracks, nothing
else. The judgement half, contradictions and statements no longer true, is an
agent's job: see "Docs check" in AGENTS.md.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path
from typing import Any
from urllib.parse import unquote

import traceability

LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)\s]+)(?:\s+\"[^\"]*\")?\)")
HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
FENCE = re.compile(r"^\s*(```|~~~)")
# Requirement IDs as the docs write them. Pins (D2), board names (ESP32-S3),
# architecture sections (A5) and research reports (R3) aren't requirements.
REQUIREMENT = re.compile(r"(?<![\w\-/.`])([FTSDQ]\d{1,2})(?![\w\-])")
INDEXED = {"docs/decisions": "*.md", "docs/notebook": "*.md", "docs/research": "*.md",
           "docs/novelties": "*.md", "docs/test-reports": "*/"}
UNIT_LAYERS = ("hal", "devices", "services")
# Folders where a letter and a number usually mean something else, such as pin D2.
NOT_REQUIREMENTS = ("docs/hardware/", "docs/research/")


def tracked_markdown(root: Path) -> list[Path]:
    git = shutil.which("git")
    if git:
        proc = subprocess.run([git, "ls-files", "*.md"], cwd=root, text=True, capture_output=True, check=False)
        if proc.returncode == 0:
            return [root / line for line in proc.stdout.splitlines() if line]
    return sorted(p for p in root.rglob("*.md") if ".venv" not in p.parts and "build" not in p.parts)


def slug(heading: str) -> str:
    """GitHub's anchor for a heading."""
    text = re.sub(r"[`*_]|\[([^\]]*)\]\([^)]*\)", lambda m: m.group(1) or "", heading).strip().lower()
    return re.sub(r"[^\w\- ]", "", text).replace(" ", "-")


def lines_outside_code(text: str) -> list[tuple[int, str]]:
    lines, fenced = [], False
    for number, line in enumerate(text.splitlines(), 1):
        if FENCE.match(line):
            fenced = not fenced
            continue
        if not fenced:
            lines.append((number, line))
    return lines


def anchors(path: Path, cache: dict[Path, set[str]]) -> set[str]:
    if path not in cache:
        found: set[str] = set()
        counts: dict[str, int] = {}
        for _, line in lines_outside_code(path.read_text(encoding="utf-8")):
            match = HEADING.match(line)
            if match:
                base = slug(match.group(2))
                n = counts.get(base, 0)
                counts[base] = n + 1
                found.add(base if n == 0 else f"{base}-{n}")
        cache[path] = found
    return cache[path]


def check_links(root: Path, files: list[Path]) -> list[dict[str, Any]]:
    problems, cache = [], {}
    for path in files:
        for number, line in lines_outside_code(path.read_text(encoding="utf-8")):
            for target in LINK.findall(re.sub(r"`[^`]*`", "", line)):
                if re.match(r"^[a-z][a-z0-9+.-]*:", target, re.IGNORECASE):
                    continue  # http:, https:, mailto: and the like
                file_part, _, anchor = target.partition("#")
                linked = (path.parent / unquote(file_part)).resolve() if file_part else path
                where = f"{path.relative_to(root).as_posix()}:{number}"
                if not linked.exists():
                    problems.append({"check": "link", "where": where, "detail": f"{target} doesn't exist"})
                elif anchor and linked.suffix == ".md" and anchor.lower() not in anchors(linked, cache):
                    problems.append({"check": "anchor", "where": where, "detail": f"{target}: no heading #{anchor}"})
    return problems


def check_requirement_ids(root: Path, files: list[Path]) -> list[dict[str, Any]]:
    known = set(traceability.requirement_ids((root / "docs" / "requirements.md").read_text(encoding="utf-8")))
    problems = []
    for path in files:
        relative = path.relative_to(root).as_posix()
        if relative.startswith(NOT_REQUIREMENTS):
            continue
        for number, line in lines_outside_code(path.read_text(encoding="utf-8")):
            for requirement in REQUIREMENT.findall(line):
                if requirement not in known:
                    problems.append({"check": "requirement", "where": f"{relative}:{number}",
                                     "detail": f"{requirement} isn't in docs/requirements.md"})
    return problems


def check_indexes(root: Path) -> list[dict[str, Any]]:
    problems = []
    for folder, pattern in INDEXED.items():
        index = root / folder / "README.md"
        if not index.exists():
            problems.append({"check": "index", "where": folder, "detail": "no README.md"})
            continue
        listed = {(index.parent / unquote(t.partition("#")[0])).resolve()
                  for t in LINK.findall(index.read_text(encoding="utf-8")) if not re.match(r"^[a-z]+:", t)}
        if pattern == "*/":
            entries = [p for p in sorted((root / folder).iterdir()) if p.is_dir()]
            missing = [p for p in entries if p.resolve() not in listed and (p / "README.md").resolve() not in listed]
        else:
            entries = [p for p in sorted((root / folder).glob(pattern)) if p.name != "README.md"]
            missing = [p for p in entries if p.resolve() not in listed]
        for path in missing:
            problems.append({"check": "index", "where": f"{folder}/README.md",
                             "detail": f"{path.name} isn't listed"})
    return problems


def check_decision_numbers(root: Path) -> list[dict[str, Any]]:
    numbers: dict[int, list[str]] = {}
    for path in (root / "docs" / "decisions").glob("[0-9][0-9][0-9][0-9]-*.md"):
        numbers.setdefault(int(path.name[:4]), []).append(path.name)
    problems = [{"check": "decision", "where": "docs/decisions", "detail": f"{n:04d} is used by {', '.join(sorted(names))}"}
                for n, names in sorted(numbers.items()) if len(names) > 1]
    if numbers:
        problems += [{"check": "decision", "where": "docs/decisions", "detail": f"{n:04d} is missing"}
                     for n in range(1, max(numbers) + 1) if n not in numbers]
    return problems


def check_units(root: Path) -> list[dict[str, Any]]:
    problems = []
    for layer in UNIT_LAYERS:
        for unit in sorted(root.glob(f"firmware/*/{layer}/*/")):
            for needed in ("README.md", "library.properties"):
                if not (unit / needed).exists():
                    problems.append({"check": "unit", "where": unit.relative_to(root).as_posix(),
                                     "detail": f"no {needed}"})
    return problems


def check(root: Path) -> dict[str, Any]:
    files = tracked_markdown(root)
    problems = (check_links(root, files) + check_requirement_ids(root, files) + check_indexes(root)
                + check_decision_numbers(root) + check_units(root))
    return {"ok": not problems, "files": len(files), "problems": problems}


def as_text(result: dict[str, Any]) -> str:
    if result["ok"]:
        return f"Docs check: {result['files']} files, no problems."
    lines = [f"{p['where']}: {p['detail']} ({p['check']})" for p in result["problems"]]
    return "\n".join(lines + [f"\nDocs check: {result['files']} files, {len(result['problems'])} problems."])
