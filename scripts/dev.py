"""Project tasks that run the same way on macOS, Linux and Windows.

Usage:
    python scripts/dev.py build [--json]            compile the UNO firmware
    python scripts/dev.py test [UNIT] [--json]      run the unit tests on this computer
    python scripts/dev.py lint                      run cppcheck on the firmware units
    python scripts/dev.py units [--json]            list the firmware units and their versions
    python scripts/dev.py upload [--port P]         flash the UNO (Upload-Cam switch on Upload)

--json prints one JSON object for agents and scripts instead of text for people.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent.parent
SKETCH = ROOT / "firmware" / "uno"
TESTING = ROOT / "firmware" / "testing"
BUILD = ROOT / "build"
FQBN = "arduino:avr:uno"
LAYERS = ("modes", "services", "devices", "hal")

# T8: at most 80% of flash, at least 512 bytes of RAM left.
MAX_FLASH_PERCENT = 80
MIN_FREE_RAM_BYTES = 512


# --- Helpers ----------------------------------------------------------------

def run(cmd: list[str], capture: bool = False, quiet: bool = False) -> subprocess.CompletedProcess[str]:
    if not quiet:
        print("+ " + " ".join(cmd), file=sys.stderr, flush=True)
    return subprocess.run(cmd, check=False, text=True, capture_output=capture)


def require(tool: str) -> str:
    path = shutil.which(tool)
    if path is None:
        sys.exit(f"{tool} not found. See docs/tools.md for how to install it.")
    return path


def emit(result: dict[str, Any], as_json: bool, text: str) -> int:
    if as_json:
        print(json.dumps(result, indent=2))
    else:
        print(text)
    return 0 if result.get("ok") else 1


def version() -> str:
    return (ROOT / "VERSION").read_text(encoding="utf-8").strip()


def git_commit() -> str:
    git = shutil.which("git")
    if git is None:
        return "unknown"
    head = subprocess.run([git, "rev-parse", "--short=7", "HEAD"], cwd=ROOT,
                          text=True, capture_output=True, check=False)
    if head.returncode != 0:
        return "unknown"
    status = subprocess.run([git, "status", "--porcelain"], cwd=ROOT,
                            text=True, capture_output=True, check=False)
    dirty = "-dirty" if status.stdout.strip() else ""
    return head.stdout.strip() + dirty


# --- Units ------------------------------------------------------------------

def read_properties(path: Path) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line and not line.lstrip().startswith("#"):
            key, _, value = line.partition("=")
            values[key.strip()] = value.strip()
    return values


def find_units() -> list[dict[str, Any]]:
    """Every folder under firmware/uno/<layer>/ that has a library.properties."""
    units = []
    for layer in LAYERS:
        for props in sorted((SKETCH / layer).glob("*/library.properties")):
            unit_dir = props.parent
            values = read_properties(props)
            units.append({
                "name": unit_dir.name,
                "layer": layer,
                "version": values.get("version", "unknown"),
                "path": unit_dir.relative_to(ROOT).as_posix(),
                "has_tests": (unit_dir / "tests").is_dir(),
            })
    return units


def profile_unit_dirs() -> set[str]:
    text = (SKETCH / "sketch.yaml").read_text(encoding="utf-8")
    return {match.strip() for match in re.findall(r"-\s*dir:\s*(\S+)", text)}


def check_profile(units: list[dict[str, Any]]) -> list[str]:
    """The profile in sketch.yaml must list exactly the unit folders."""
    on_disk = {Path(u["path"]).relative_to(SKETCH.relative_to(ROOT)).as_posix() for u in units}
    listed = profile_unit_dirs()
    problems = [f"unit {d} is missing from firmware/uno/sketch.yaml" for d in sorted(on_disk - listed)]
    problems += [f"sketch.yaml lists {d}, which isn't a unit folder" for d in sorted(listed - on_disk)]
    return problems


def units_cmd(args: argparse.Namespace) -> int:
    units = find_units()
    problems = check_profile(units)
    text = "\n".join(f"{u['layer']:<9} {u['name']:<12} {u['version']:<8} {u['path']}" for u in units)
    if problems:
        text += "\n" + "\n".join(problems)
    return emit({"ok": not problems, "units": units, "problems": problems}, args.json, text)


# --- Build ------------------------------------------------------------------

def write_build_info(commit: str) -> None:
    generated = SKETCH / "src" / "generated"
    generated.mkdir(exist_ok=True)
    content = (
        "// Written by scripts/dev.py before each build. Not in git.\n"
        "#ifndef OC_BUILD_INFO_H\n#define OC_BUILD_INFO_H\n"
        f'#define OC_FIRMWARE_VERSION "{version()}"\n'
        f'#define OC_GIT_COMMIT "{commit}"\n'
        "#endif\n"
    )
    target = generated / "build_info.h"
    # Rewrite only on change, so unchanged builds stay cached.
    if not target.exists() or target.read_text(encoding="utf-8") != content:
        target.write_text(content, encoding="utf-8", newline="\n")


def our_warnings(output: str) -> list[str]:
    """Compiler warnings in our own files. The Arduino core's don't count."""
    ours = str(SKETCH).replace("\\", "/")
    lines = []
    for line in output.splitlines():
        normalized = line.replace("\\", "/")
        if "warning:" in normalized and (ours in normalized or "/sketch/" in normalized):
            lines.append(line)
    return lines


def build(args: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    units = find_units()
    problems = check_profile(units)
    commit = git_commit()
    result: dict[str, Any] = {"ok": False, "version": version(), "commit": commit, "problems": problems}
    if problems:
        return emit(result, args.json, "\n".join(problems))

    write_build_info(commit)
    proc = run([cli, "compile", "--profile", "uno", "--warnings", "all", "--format", "json",
                "--build-path", str(BUILD / "uno"), str(SKETCH)], capture=True, quiet=args.json)
    try:
        report = json.loads(proc.stdout)
    except json.JSONDecodeError:
        result["problems"].append("arduino-cli gave no JSON report: " + (proc.stderr or proc.stdout)[-500:])
        return emit(result, args.json, result["problems"][-1])

    output = report.get("compiler_out", "") + report.get("compiler_err", "")
    if not report.get("success", False):
        result["problems"].append(report.get("error", "compile failed"))
        result["compiler_output"] = output[-4000:]
        return emit(result, args.json, output + "\n" + result["problems"][-1])

    builder = report.get("builder_result", {})
    sizes = {s["name"]: s for s in builder.get("executable_sections_size", [])}
    flash, ram = sizes.get("text", {}), sizes.get("data", {})
    flash_percent = round(100 * flash.get("size", 0) / max(flash.get("max_size", 1), 1), 1)
    free_ram = ram.get("max_size", 0) - ram.get("size", 0)
    versions = {lib["name"]: lib.get("version", "") for lib in builder.get("used_libraries", [])}
    warnings = our_warnings(output)

    result.update({
        "flash": {"used": flash.get("size"), "max": flash.get("max_size"), "percent": flash_percent},
        "ram": {"used": ram.get("size"), "max": ram.get("max_size"), "free": free_ram},
        "units": [{"name": u["name"], "layer": u["layer"], "version": versions.get(u["name"], u["version"])}
                  for u in units],
        "warnings": warnings,
        "hex": (BUILD / "uno" / "uno.ino.hex").relative_to(ROOT).as_posix(),
    })
    if warnings:
        problems.append(f"{len(warnings)} warning(s) in our code; we treat them as errors")
    if flash_percent > MAX_FLASH_PERCENT:
        problems.append(f"T8: flash use {flash_percent}% is over {MAX_FLASH_PERCENT}%")
    if free_ram < MIN_FREE_RAM_BYTES:
        problems.append(f"T8: {free_ram} bytes of RAM left, under {MIN_FREE_RAM_BYTES}")
    result["ok"] = not problems

    text = "\n".join(warnings + [
        f"Firmware {version()} ({commit})",
        f"Flash: {flash.get('size')} of {flash.get('max_size')} bytes ({flash_percent}%)",
        f"RAM:   {ram.get('size')} of {ram.get('max_size')} bytes used, {free_ram} free",
        "Units: " + ", ".join(f"{u['name']} {u['version']}" for u in result["units"]),
        f"Image: {result['hex']}",
    ] + problems)
    return emit(result, args.json, text)


# --- Unit tests -------------------------------------------------------------

def test(args: argparse.Namespace) -> int:
    cmake = require("cmake")
    ctest = require("ctest")
    out = BUILD / "tests"
    cache = out / "CMakeCache.txt"
    if cache.exists() and TESTING.as_posix() not in cache.read_text(encoding="utf-8", errors="replace"):
        shutil.rmtree(out)  # left over from an older layout; CMake can't reuse it
    names =[u["name"] for u in find_units() if u["has_tests"]]
    if args.unit and args.unit not in names:
        return emit({"ok": False, "problems": [f"no unit with tests called {args.unit}"], "units": names},
                    args.json, f"No unit with tests called {args.unit}. Units with tests: {', '.join(names)}")
    for step in ([cmake, "-S", str(TESTING), "-B", str(out)],
                 [cmake, "--build", str(out), "--config", "Debug"]):
        proc = run(step, capture=args.json, quiet=args.json)
        if proc.returncode != 0:
            return emit({"ok": False, "problems": ["unit test build failed"],
                         "output": (proc.stdout or "")[-4000:] + (proc.stderr or "")[-4000:]},
                        args.json, "Unit test build failed.")
    junit = out / "results.xml"
    cmd = [ctest, "--test-dir", str(out), "--build-config", "Debug", "--output-on-failure",
           "--output-junit", str(junit)]
    if args.unit:
        cmd += ["--tests-regex", f"^{args.unit}$"]
    proc = run(cmd, capture=args.json, quiet=args.json)
    results = []
    if junit.exists():
        for case in ElementTree.parse(junit).getroot().iter("testcase"):
            failed = case.find("failure") is not None or case.get("status") == "fail"
            output = case.findtext("system-out") or ""
            results.append({"unit": case.get("name"), "passed": not failed, "output": output.strip()[-2000:]})
    ok = proc.returncode == 0 and bool(results)
    if args.json:
        return emit({"ok": ok, "results": results}, True, "")
    return 0 if ok else 1


def lint(_: argparse.Namespace) -> int:
    cppcheck = require("cppcheck")
    paths = []
    for unit in find_units():
        unit_dir = ROOT / unit["path"]
        paths += [str(p) for p in (unit_dir / "src", unit_dir / "tests") if p.is_dir()]
    return run([
        cppcheck, "--language=c++", "--std=c++11", "--platform=avr8",
        "--enable=warning,performance,portability", "--inline-suppr",
        "--error-exitcode=1", "--quiet", f"-I{TESTING}",
        *paths,
    ]).returncode


# --- Upload -----------------------------------------------------------------

def find_port(cli: str) -> str | None:
    """The only USB serial port arduino-cli can see, or None."""
    proc = run([cli, "board", "list", "--format", "json"], capture=True, quiet=True)
    try:
        report = json.loads(proc.stdout or "{}")
    except json.JSONDecodeError:
        return None
    ports = report.get("detected_ports", report) if isinstance(report, dict) else report
    usb = [p["port"]["address"] for p in ports
           if p.get("port", {}).get("protocol") == "serial"
           and p.get("port", {}).get("properties", {}).get("vid")]
    return usb[0] if len(usb) == 1 else None


def upload(args: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    port = args.port or find_port(cli)
    if port is None:
        sys.exit("Couldn't pick a port: give one with --port. `arduino-cli board list` shows them.")
    print("The Upload-Cam switch must be on Upload.", file=sys.stderr)
    return run([cli, "upload", "--fqbn", FQBN, "--port", port,
                "--input-dir", str(BUILD / "uno"), str(SKETCH)]).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="task", required=True)
    p = sub.add_parser("build", help="compile the UNO firmware")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=build)
    p = sub.add_parser("test", help="run the unit tests")
    p.add_argument("unit", nargs="?", help="test only this unit, such as safety")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=test)
    sub.add_parser("lint", help="run cppcheck").set_defaults(func=lint)
    p = sub.add_parser("units", help="list the firmware units")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=units_cmd)
    p = sub.add_parser("upload", help="flash the UNO")
    p.add_argument("--port", help="serial port, such as COM3 or /dev/ttyUSB0; found automatically if only one")
    p.set_defaults(func=upload)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
