"""Project tasks that run the same way on macOS, Linux and Windows.

Usage:
    python scripts/dev.py build             compile the UNO firmware
    python scripts/dev.py test              run the unit tests on this computer
    python scripts/dev.py lint              run cppcheck on our firmware code
    python scripts/dev.py upload --port P   flash the UNO (Upload-Cam switch on Upload)
"""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SKETCH = ROOT / "firmware" / "uno"
TESTS = ROOT / "firmware" / "tests"
BUILD = ROOT / "build"
FQBN = "arduino:avr:uno"

# T8: at most 80% of flash, at least 512 bytes of RAM left.
MAX_FLASH_PERCENT = 80
MIN_FREE_RAM_BYTES = 512


def run(cmd: list[str], capture: bool = False) -> subprocess.CompletedProcess[str]:
    print("+ " + " ".join(cmd), flush=True)
    return subprocess.run(cmd, check=False, text=True, capture_output=capture)


def require(tool: str) -> str:
    path = shutil.which(tool)
    if path is None:
        sys.exit(f"{tool} not found. See docs/tools.md for how to install it.")
    return path


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


def write_build_info() -> None:
    generated = SKETCH / "src" / "generated"
    generated.mkdir(exist_ok=True)
    content = (
        "// Written by scripts/dev.py before each build. Not in git.\n"
        "#ifndef OC_BUILD_INFO_H\n#define OC_BUILD_INFO_H\n"
        f'#define OC_GIT_COMMIT "{git_commit()}"\n'
        "#endif\n"
    )
    target = generated / "build_info.h"
    # Rewrite only on change, so unchanged builds stay cached.
    if not target.exists() or target.read_text(encoding="utf-8") != content:
        target.write_text(content, encoding="utf-8", newline="\n")


def check_size(output: str) -> bool:
    ok = True
    flash = re.search(r"Sketch uses (\d+) bytes \((\d+)%\)", output)
    ram = re.search(r"leaving (\d+) bytes for local variables", output, re.IGNORECASE)
    if flash is None or ram is None:
        print("Couldn't read the size report from arduino-cli.")
        return False
    if int(flash.group(2)) > MAX_FLASH_PERCENT:
        print(f"T8: flash use {flash.group(2)}% is over {MAX_FLASH_PERCENT}%.")
        ok = False
    if int(ram.group(1)) < MIN_FREE_RAM_BYTES:
        print(f"T8: {ram.group(1)} bytes of RAM left, under {MIN_FREE_RAM_BYTES}.")
        ok = False
    return ok


def our_warnings(output: str) -> list[str]:
    """Compiler warnings in our own files. The Arduino core's don't count."""
    lines = []
    for line in output.splitlines():
        if "warning:" not in line:
            continue
        normalized = line.replace("\\", "/")
        if "/sketch/" in normalized or str(SKETCH).replace("\\", "/") in normalized:
            lines.append(line)
    return lines


def build(_: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    write_build_info()
    result = run([cli, "compile", "--profile", "uno", "--warnings", "all",
                  "--build-path", str(BUILD / "uno"), str(SKETCH)], capture=True)
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    if result.returncode != 0:
        return result.returncode
    warnings = our_warnings(result.stdout + result.stderr)
    if warnings:
        print(f"{len(warnings)} warning(s) in our code; we treat them as errors.")
        return 1
    return 0 if check_size(result.stdout) else 1


def test(_: argparse.Namespace) -> int:
    cmake = require("cmake")
    ctest = require("ctest")
    out = BUILD / "tests"
    steps = [
        [cmake, "-S", str(TESTS), "-B", str(out)],
        [cmake, "--build", str(out), "--config", "Debug"],
        [ctest, "--test-dir", str(out), "--build-config", "Debug", "--output-on-failure"],
    ]
    for step in steps:
        code = run(step).returncode
        if code != 0:
            return code
    return 0


def lint(_: argparse.Namespace) -> int:
    cppcheck = require("cppcheck")
    return run([
        cppcheck, "--language=c++", "--std=c++11", "--platform=avr8",
        "--enable=warning,performance,portability", "--inline-suppr",
        "--error-exitcode=1", "--quiet",
        str(SKETCH / "src"), str(TESTS),
    ]).returncode


def upload(args: argparse.Namespace) -> int:
    cli = require("arduino-cli")
    print("The Upload-Cam switch must be on Upload, and the tracks off the ground.")
    return run([cli, "upload", "--fqbn", FQBN, "--port", args.port,
                "--input-dir", str(BUILD / "uno"), str(SKETCH)]).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="task", required=True)
    sub.add_parser("build", help="compile the UNO firmware").set_defaults(func=build)
    sub.add_parser("test", help="run the unit tests").set_defaults(func=test)
    sub.add_parser("lint", help="run cppcheck").set_defaults(func=lint)
    up = sub.add_parser("upload", help="flash the UNO")
    up.add_argument("--port", required=True, help="serial port, such as COM3 or /dev/ttyUSB0")
    up.set_defaults(func=upload)
    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
