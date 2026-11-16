"""Project tasks that run the same way on macOS, Linux and Windows.

Usage:
    python scripts/dev.py setup [--json]                install the pinned Arduino cores and libraries
    python scripts/dev.py build [--target T] [--json]   compile a firmware (default: uno)
    python scripts/dev.py test [UNIT] [--json]          run the unit tests on this computer
    python scripts/dev.py lint                          run cppcheck on the firmware units
    python scripts/dev.py units [--base REF] [--json]   list the units; with --base, check version bumps
    python scripts/dev.py upload [--target T] [--port P]  flash a firmware
    python scripts/dev.py trace [--increment N] [--json]  each requirement's tests and latest robot result
    python scripts/dev.py docs [--json]                 check the docs' links, IDs and indexes

Targets: uno, the robot's UNO (Upload-Cam switch on Upload), and dongle, the
UNO R4 WiFi that links the computer to the robot over Wi-Fi.

--json prints one JSON object for agents and scripts instead of text for people.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path
from typing import Any

import docscheck
import traceability

ROOT = Path(__file__).resolve().parent.parent
FIRMWARE = ROOT / "firmware"
TESTING = FIRMWARE / "testing"
BUILD = ROOT / "build"
LAYERS = ("modes", "services", "devices", "hal")

# Each firmware is an Arduino sketch in firmware/<name>/, built from its units.
# The core and libraries are pinned here and installed once, globally, by
# `dev.py setup`, so every build on this computer shares one copy of each
# (docs/tools.md). size_limits: whether T8 applies.
TARGETS: dict[str, dict[str, Any]] = {
    "uno": {"fqbn": "arduino:avr:uno", "platform": ("arduino:avr", "1.8.8"),
            "libraries": [("Servo", "1.3.0")], "cppcheck_platform": "avr8", "size_limits": True},
    "dongle": {"fqbn": "arduino:renesas_uno:unor4wifi", "platform": ("arduino:renesas_uno", "1.6.0"),
               "libraries": [], "cppcheck_platform": "unix32", "size_limits": False},
}


def sketch_dir(target: str) -> Path:
    return FIRMWARE / target

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


def version(target: str) -> str:
    """A firmware's own version, from firmware/<target>/VERSION (decision 0012)."""
    return (sketch_dir(target) / "VERSION").read_text(encoding="utf-8").strip()


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


def find_units(target: str | None = None) -> list[dict[str, Any]]:
    """Every folder under firmware/<target>/<layer>/ that has a library.properties,
    for one target or all of them."""
    units = []
    for name in [target] if target else TARGETS:
        for layer in LAYERS:
            for props in sorted((sketch_dir(name) / layer).glob("*/library.properties")):
                unit_dir = props.parent
                values = read_properties(props)
                units.append({
                    "name": unit_dir.name,
                    "firmware": name,
                    "layer": layer,
                    "version": values.get("version", "unknown"),
                    "path": unit_dir.relative_to(ROOT).as_posix(),
                    "has_tests": any((unit_dir / "tests").glob("*.cpp")),
                })
    return units


def check_names(units: list[dict[str, Any]]) -> list[str]:
    """Unit names are unique across firmwares: each is one unit test executable."""
    seen: dict[str, str] = {}
    problems = []
    for unit in units:
        if unit["name"] in seen:
            problems.append(f"unit name {unit['name']} is used by {seen[unit['name']]} and {unit['path']}")
        seen.setdefault(unit["name"], unit["path"])
    return problems


def unbumped_units(units: list[dict[str, Any]], base: str) -> list[str]:
    """Units whose code changed since base without a new version (decision 0012).
    Compares with the working tree, so uncommitted changes count too."""
    git = require("git")
    merge_base = subprocess.run([git, "merge-base", base, "HEAD"], cwd=ROOT, text=True,
                                capture_output=True, check=False)
    if merge_base.returncode != 0:
        return [f"can't compare with {base}: {merge_base.stderr.strip()}"]
    ref = merge_base.stdout.strip()
    problems = []
    for unit in units:
        changed = subprocess.run([git, "diff", "--name-only", ref, "--", f"{unit['path']}/src"],
                                 cwd=ROOT, text=True, capture_output=True, check=False).stdout.strip()
        if not changed:
            continue
        old = subprocess.run([git, "show", f"{ref}:{unit['path']}/library.properties"],
                             cwd=ROOT, text=True, capture_output=True, check=False)
        if old.returncode != 0:
            continue  # a new unit
        old_version = next((line.partition("=")[2].strip() for line in old.stdout.splitlines()
                            if line.startswith("version=")), "")
        if old_version == unit["version"]:
            problems.append(f"{unit['path']}: code changed since {base}, but version is still {old_version}")
    return problems


def units_cmd(args: argparse.Namespace) -> int:
    units = find_units()
    problems = check_names(units)
    if args.base:
        problems += unbumped_units(units, args.base)
    text = "\n".join(f"{u['firmware']:<7} {u['layer']:<9} {u['name']:<12} {u['version']:<8} {u['path']}"
                     for u in units)
    if problems:
        text += "\n" + "\n".join(problems)
    return emit({"ok": not problems, "units": units, "problems": problems}, args.json, text)


# --- Arduino cores and libraries --------------------------------------------

def cli_json(cli: str, *args: str) -> dict[str, Any]:
    proc = run([cli, *args, "--format", "json"], capture=True, quiet=True)
    try:
        return json.loads(proc.stdout or "{}")
    except json.JSONDecodeError:
        return {}


def installed(cli: str) -> tuple[dict[str, str], dict[str, str]]:
    """The installed cores and the libraries arduino-cli manages, each name to version."""
    platforms = {p["id"]: p.get("installed_version", "")
                 for p in cli_json(cli, "core", "list").get("platforms", [])}
    libraries = {}
    for entry in cli_json(cli, "lib", "list").get("installed_libraries", []):
        library = entry.get("library", {})
        if library.get("location") == "user":
            libraries[library.get("name", "")] = library.get("version", "")
    return platforms, libraries


def missing(cli: str, targets: list[str]) -> tuple[list[tuple[str, str]], list[tuple[str, str]]]:
    """Pinned cores and libraries that aren't installed at exactly their version."""
    platforms, libraries = installed(cli)
    cores, libs = [], []
    for target in targets:
        core = TARGETS[target]["platform"]
        if platforms.get(core[0]) != core[1] and core not in cores:
            cores.append(core)
        for lib in TARGETS[target]["libraries"]:
            if libraries.get(lib[0]) != lib[1] and lib not in libs:
                libs.append(lib)
    return cores, libs


def setup(args: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    cores, libs = missing(cli, list(TARGETS))
    problems = []
    if cores:
        run([cli, "core", "update-index"], quiet=args.json, capture=args.json)
    for name, ver in cores:
        if run([cli, "core", "install", f"{name}@{ver}"], quiet=args.json, capture=args.json).returncode != 0:
            problems.append(f"couldn't install {name}@{ver}")
    if libs:
        run([cli, "lib", "update-index"], quiet=args.json, capture=args.json)
    for name, ver in libs:
        if run([cli, "lib", "install", f"{name}@{ver}"], quiet=args.json, capture=args.json).returncode != 0:
            problems.append(f"couldn't install {name}@{ver}")
    installed_now = [f"{n}@{v}" for n, v in cores + libs]
    text = ("Installed: " + ", ".join(installed_now)) if installed_now else "Everything pinned is already installed."
    return emit({"ok": not problems, "installed": installed_now, "problems": problems},
                args.json, "\n".join([text] + problems))


# --- Build ------------------------------------------------------------------

def write_build_info(target: str, commit: str) -> None:
    generated = sketch_dir(target) / "src" / "generated"
    generated.mkdir(exist_ok=True)
    content = (
        "// Written by scripts/dev.py before each build. Not in git.\n"
        "#ifndef OC_BUILD_INFO_H\n#define OC_BUILD_INFO_H\n"
        f'#define OC_FIRMWARE_VERSION "{version(target)}"\n'
        f'#define OC_GIT_COMMIT "{commit}"\n'
        "#endif\n"
    )
    target = generated / "build_info.h"
    # Rewrite only on change, so unchanged builds stay cached.
    if not target.exists() or target.read_text(encoding="utf-8") != content:
        target.write_text(content, encoding="utf-8", newline="\n")


def our_warnings(target: str, output: str) -> list[str]:
    """Compiler warnings in our own files. The Arduino core's don't count."""
    ours = str(sketch_dir(target)).replace("\\", "/")
    lines = []
    for line in output.splitlines():
        normalized = line.replace("\\", "/")
        if "warning:" in normalized and (ours in normalized or "/sketch/" in normalized):
            lines.append(line)
    return lines


def build(args: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    target = args.target
    units = find_units(target)
    cores, libs = missing(cli, [target])
    problems = [f"{name} {ver} isn't installed. Run: python scripts/dev.py setup" for name, ver in cores + libs]
    commit = git_commit()
    result: dict[str, Any] = {"ok": False, "target": target, "version": version(target), "commit": commit,
                              "problems": problems}
    if problems:
        return emit(result, args.json, "\n".join(problems))

    write_build_info(target, commit)
    cmd = [cli, "compile", "--fqbn", TARGETS[target]["fqbn"], "--warnings", "all", "--format", "json",
           "--build-path", str(BUILD / target)]
    for unit in units:
        cmd += ["--library", str(ROOT / unit["path"])]
    proc = run(cmd + [str(sketch_dir(target))], capture=True, quiet=args.json)
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
    warnings = our_warnings(target, output)
    image = next((BUILD / target / f"{target}.ino{ext}" for ext in (".hex", ".bin")
                  if (BUILD / target / f"{target}.ino{ext}").exists()), BUILD / target)

    result.update({
        "flash": {"used": flash.get("size"), "max": flash.get("max_size"), "percent": flash_percent},
        "ram": {"used": ram.get("size"), "max": ram.get("max_size"), "free": free_ram},
        "units": [{"name": u["name"], "layer": u["layer"], "version": versions.get(u["name"], u["version"])}
                  for u in units],
        "warnings": warnings,
        "image": image.relative_to(ROOT).as_posix(),
    })
    if warnings:
        problems.append(f"{len(warnings)} warning(s) in our code; we treat them as errors")
    limits = TARGETS[target]["size_limits"]
    if limits and flash_percent > MAX_FLASH_PERCENT:
        problems.append(f"T8: flash use {flash_percent}% is over {MAX_FLASH_PERCENT}%")
    if limits and free_ram < MIN_FREE_RAM_BYTES:
        problems.append(f"T8: {free_ram} bytes of RAM left, under {MIN_FREE_RAM_BYTES}")
    result["ok"] = not problems

    text = "\n".join(warnings + [
        f"Firmware {target} {version(target)} ({commit})",
        f"Flash: {flash.get('size')} of {flash.get('max_size')} bytes ({flash_percent}%)",
        f"RAM:   {ram.get('size')} of {ram.get('max_size')} bytes used, {free_ram} free",
        "Units: " + ", ".join(f"{u['name']} {u['version']}" for u in result["units"]),
        f"Image: {result['image']}",
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
    names = [u["name"] for u in find_units() if u["has_tests"]]
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
    failed = False
    for target, settings in TARGETS.items():
        paths = []
        for unit in find_units(target):
            unit_dir = ROOT / unit["path"]
            paths += [str(p) for p in (unit_dir / "src", unit_dir / "tests") if p.is_dir()]
        if not paths:
            continue
        failed |= run([
            cppcheck, "--language=c++", "--std=c++11", f"--platform={settings['cppcheck_platform']}",
            "--enable=warning,performance,portability", "--inline-suppr",
            "--error-exitcode=1", "--quiet", f"-I{TESTING}",
            *paths,
        ]).returncode != 0
    return 1 if failed else 0


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
    target = args.target
    port = args.port or find_port(cli)
    if port is None:
        sys.exit("Couldn't pick a port: give one with --port. `arduino-cli board list` shows them.")
    if target == "uno":
        print("The Upload-Cam switch must be on Upload.", file=sys.stderr)
    proc = run([cli, "upload", "--fqbn", TARGETS[target]["fqbn"], "--port", port,
                "--input-dir", str(BUILD / target), str(sketch_dir(target))], capture=True)
    print(proc.stdout, end="")
    print(proc.stderr, end="", file=sys.stderr)
    if proc.returncode != 0 and "not in sync" in (proc.stdout + proc.stderr):
        print("The bootloader didn't answer. Check that the Upload-Cam switch is on Upload.", file=sys.stderr)
    return proc.returncode


def trace(args: argparse.Namespace) -> int:
    result = traceability.trace(ROOT, args.increment)
    return emit(result, args.json, traceability.as_table(result))


def docs(args: argparse.Namespace) -> int:
    result = docscheck.check(ROOT)
    return emit(result, args.json, docscheck.as_text(result))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="task", required=True)
    p = sub.add_parser("setup", help="install the pinned Arduino cores and libraries")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=setup)
    p = sub.add_parser("build", help="compile a firmware")
    p.add_argument("--target", choices=sorted(TARGETS), default="uno", help="which firmware (default: uno)")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=build)
    p = sub.add_parser("test", help="run the unit tests")
    p.add_argument("unit", nargs="?", help="test only this unit, such as safety")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=test)
    sub.add_parser("lint", help="run cppcheck").set_defaults(func=lint)
    p = sub.add_parser("units", help="list the firmware units")
    p.add_argument("--base", help="also check that every unit changed since this git ref has a new version")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=units_cmd)
    p = sub.add_parser("upload", help="flash a firmware")
    p.add_argument("--target", choices=sorted(TARGETS), default="uno", help="which firmware (default: uno)")
    p.add_argument("--port", help="serial port, such as COM3 or /dev/ttyUSB0; found automatically if only one")
    p.set_defaults(func=upload)
    p = sub.add_parser("trace", help="each requirement's tests and latest robot result (report only)")
    p.add_argument("--increment", type=int, help="only the requirements of this increment, such as 1")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=trace)
    p = sub.add_parser("docs", help="check the docs' links, requirement IDs and indexes (the script half)")
    p.add_argument("--json", action="store_true", help="print a JSON report")
    p.set_defaults(func=docs)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
