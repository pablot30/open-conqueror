"""The link protocol, as the host computer sees it.

Messages are short text inside braces at 9600 baud, so they pass through the
stock camera board's bridge unchanged. See firmware/README.md for the list.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, fields
from typing import Any

BAUD = 9600

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

STOP = b"{S}"
NEUTRAL = b"{D,0,0}"  # releases a held stop; the firmware needs it before driving
VERSION = b"{V}"


def drive_message(left: int, right: int) -> bytes:
    for value in (left, right):
        if not -100 <= value <= 100:
            raise ValueError("speeds are -100 to 100 percent")
    return f"{{D,{left},{right}}}".encode("ascii")


def direction_speeds(direction: str, speed: int) -> tuple[int, int]:
    """Left and right percentages for a direction such as "f" or "bl"."""
    if direction not in DIRECTIONS:
        raise ValueError("direction must be one of: " + " ".join(DIRECTIONS))
    if not 1 <= speed <= 100:
        raise ValueError("speed is 1 to 100 percent")
    left, right = DIRECTIONS[direction]
    return round(left * speed), round(right * speed)


@dataclass
class Telemetry:
    ms: int
    state: int
    left_pwm: int
    right_pwm: int
    battery_mv: int
    faults: int
    longest_loop_us: int

    @property
    def state_name(self) -> str:
        return STATES.get(self.state, str(self.state))

    @property
    def fault_names(self) -> list[str]:
        return [name for bit, name in FAULTS.items() if self.faults & bit]

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["state_name"] = self.state_name
        data["fault_names"] = self.fault_names
        return data

    def describe(self) -> str:
        faults = ", ".join(self.fault_names) or "none"
        return (f"{self.state_name}, PWM left {self.left_pwm} right {self.right_pwm}, "
                f"battery {self.battery_mv / 1000:.2f} V, faults: {faults}, "
                f"longest loop {self.longest_loop_us} us")


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
