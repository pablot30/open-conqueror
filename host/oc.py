"""Computer tool for the open-conqueror UNO firmware.

Connects over USB serial and gives a small command prompt for driving the
robot and watching its state. Type "help" at the prompt for the commands.

    python host/oc.py --port /dev/ttyUSB0      (COM3 on Windows)

Keep the tracks off the ground until you trust what the robot does.
"""

from __future__ import annotations

import argparse
import cmd
import csv
import threading
import time
from dataclasses import dataclass, fields
from pathlib import Path
from typing import IO, Any

BAUD = 9600  # the stock camera bridge's rate, so the protocol works through it later
RESEND_PERIOD_S = 0.1  # well inside the firmware's 300 ms link timeout

STATES = {0: "standby", 1: "driving", 2: "stopped"}
FAULTS = {0x01: "link timeout", 0x02: "battery low", 0x04: "battery critical"}

# Track speeds as fractions of the requested speed: (left, right).
DIRECTIONS = {
    "f": (1.0, 1.0),
    "b": (-1.0, -1.0),
    "l": (-1.0, 1.0),   # spin left on the spot
    "r": (1.0, -1.0),   # spin right on the spot
    "fl": (0.5, 1.0),
    "fr": (1.0, 0.5),
    "bl": (-0.5, -1.0),
    "br": (-1.0, -0.5),
}


# --- Messages ---------------------------------------------------------------

def drive_message(left: int, right: int) -> bytes:
    for value in (left, right):
        if not -100 <= value <= 100:
            raise ValueError("speeds are -100 to 100 percent")
    return f"{{D,{left},{right}}}".encode("ascii")


STOP = b"{S}"
VERSION = b"{V}"


@dataclass
class Telemetry:
    ms: int
    state: int
    left_pwm: int
    right_pwm: int
    battery_mv: int
    faults: int
    longest_loop_us: int

    def describe(self) -> str:
        faults = [name for bit, name in FAULTS.items() if self.faults & bit] or ["none"]
        return (f"{STATES.get(self.state, self.state)}, PWM left {self.left_pwm} right "
                f"{self.right_pwm}, battery {self.battery_mv / 1000:.2f} V, "
                f"faults: {', '.join(faults)}, longest loop {self.longest_loop_us} us")


def parse_message(body: str) -> tuple[str, list[str]]:
    """Splits a message body (without braces) into its type and fields."""
    parts = body.split(",")
    return parts[0], parts[1:]


def parse_telemetry(values: list[str]) -> Telemetry | None:
    if len(values) != len(fields(Telemetry)):
        return None
    try:
        return Telemetry(*(int(v) for v in values))
    except ValueError:
        return None


class MessageSplitter:
    """Collects bytes from the robot into message bodies."""

    def __init__(self) -> None:
        self._body: list[str] | None = None

    def feed(self, data: bytes) -> list[str]:
        bodies = []
        for char in data.decode("ascii", errors="replace"):
            if char == "{":
                self._body = []
            elif char == "}" and self._body is not None:
                bodies.append("".join(self._body))
                self._body = None
            elif self._body is not None:
                self._body.append(char)
        return bodies


# --- Link -------------------------------------------------------------------

class Robot:
    def __init__(self, port: str) -> None:
        import serial  # imported here so the message code runs without pyserial

        # Opening the port resets the UNO through the USB chip's DTR line.
        self._serial = serial.Serial(port, BAUD, timeout=0.1)
        self._lock = threading.Lock()
        self.latest: Telemetry | None = None
        self._log: tuple[IO[str], Any] | None = None
        self._running = True
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    def send(self, message: bytes) -> None:
        with self._lock:
            self._serial.write(message)

    def close(self) -> None:
        self.send(STOP)
        self._running = False
        self._reader.join(timeout=1)
        self.stop_log()
        self._serial.close()

    def start_log(self, path: Path) -> None:
        self.stop_log()
        handle = path.open("w", newline="", encoding="utf-8")
        writer = csv.writer(handle)
        writer.writerow(["pc_time_s"] + [f.name for f in fields(Telemetry)])
        self._log = (handle, writer)

    def stop_log(self) -> None:
        if self._log is not None:
            self._log[0].close()
            self._log = None

    def _read_loop(self) -> None:
        splitter = MessageSplitter()
        while self._running:
            data = self._serial.read(64)
            for body in splitter.feed(data):
                self._handle(body)

    def _handle(self, body: str) -> None:
        kind, values = parse_message(body)
        if kind == "T":
            telemetry = parse_telemetry(values)
            if telemetry is not None:
                self.latest = telemetry
                if self._log is not None:
                    self._log[1].writerow([f"{time.time():.3f}"] + values)
        elif kind == "V":
            print(f"\n[robot] firmware {', '.join(values)}")
        elif kind == "E":
            print(f"\n[robot] error {', '.join(values)}: message not understood")
        else:
            print(f"\n[robot] {body}")


# --- Prompt -----------------------------------------------------------------

class Shell(cmd.Cmd):
    intro = "Connected. Type help for commands. Ctrl+C stops the robot while it drives."
    prompt = "oc> "

    def __init__(self, robot: Robot) -> None:
        super().__init__()
        self.robot = robot

    def _drive_for(self, left: int, right: int, seconds: float) -> None:
        message = drive_message(left, right)
        end = time.monotonic() + seconds
        try:
            while time.monotonic() < end:
                self.robot.send(message)
                time.sleep(RESEND_PERIOD_S)
        except KeyboardInterrupt:
            print()
        finally:
            self.robot.send(STOP)

    def do_go(self, arg: str) -> None:
        """go DIR [SPEED] [SECONDS]: drive one of f b l r fl fr bl br.
        SPEED is 1 to 100 percent (default 40), SECONDS defaults to 1."""
        parts = arg.split()
        if not parts or parts[0] not in DIRECTIONS:
            print("Direction must be one of: " + " ".join(DIRECTIONS))
            return
        try:
            speed = int(parts[1]) if len(parts) > 1 else 40
            seconds = float(parts[2]) if len(parts) > 2 else 1.0
        except ValueError:
            print("Speed and seconds must be numbers.")
            return
        speed = max(1, min(100, speed))
        left, right = DIRECTIONS[parts[0]]
        self._drive_for(round(left * speed), round(right * speed), seconds)

    def do_drive(self, arg: str) -> None:
        """drive LEFT RIGHT [SECONDS]: set each track, -100 to 100 percent."""
        parts = arg.split()
        try:
            left, right = int(parts[0]), int(parts[1])
            seconds = float(parts[2]) if len(parts) > 2 else 1.0
            drive_message(left, right)
        except (IndexError, ValueError) as error:
            print(f"Usage: drive LEFT RIGHT [SECONDS] ({error})")
            return
        self._drive_for(left, right, seconds)

    def do_stop(self, _: str) -> None:
        """stop: brake both tracks."""
        self.robot.send(STOP)

    def do_version(self, _: str) -> None:
        """version: ask the firmware for its version and git commit."""
        self.robot.send(VERSION)
        time.sleep(0.3)

    def do_status(self, _: str) -> None:
        """status: show the latest telemetry."""
        latest = self.robot.latest
        print(latest.describe() if latest else "No telemetry yet.")

    def do_log(self, arg: str) -> None:
        """log FILE.csv: record telemetry to a file. log off: stop recording."""
        if arg.strip() == "off":
            self.robot.stop_log()
            print("Recording stopped.")
        elif arg.strip():
            self.robot.start_log(Path(arg.strip()))
            print(f"Recording to {arg.strip()}.")
        else:
            print("Usage: log FILE.csv, or log off")

    def do_quit(self, _: str) -> bool:
        """quit: stop the robot and exit."""
        return True

    do_EOF = do_quit


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", required=True, help="serial port, such as COM3 or /dev/ttyUSB0")
    args = parser.parse_args()
    robot = Robot(args.port)
    try:
        time.sleep(2)  # the UNO restarts when the port opens
        Shell(robot).cmdloop()
    except KeyboardInterrupt:
        pass
    finally:
        robot.close()


if __name__ == "__main__":
    main()
