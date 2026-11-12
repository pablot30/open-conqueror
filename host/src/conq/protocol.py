"""The link protocol, as the host computer sees it.

Messages are short text inside braces at 9600 baud, so they pass through the
stock camera board's bridge unchanged. See firmware/README.md for the list.
"""

from __future__ import annotations

from dataclasses import asdict, dataclass, fields
from typing import Any

BAUD = 9600
PROTOCOL_VERSION = 3  # the firmware reports its own in the version message

STATES = {0: "standby", 1: "driving", 2: "stopped"}
FAULTS = {0x01: "link timeout", 0x02: "battery low", 0x04: "battery critical"}
# Signal statuses and diagnostic event states (docs/architecture.md, A5).
SIGNAL_STATUSES = {0: "valid", 1: "init", 2: "substitute", 3: "error", 4: "not available"}
EVENT_STATES = {0: "not tested", 1: "prefailed", 2: "failed", 3: "prepassed", 4: "passed"}
# Diagnostic events in the order the health message lists them.
EVENTS = ["battery sensor", "frame overrun", "link timeout", "gyro comms"]

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


def camera_message(pan: int, tilt: int) -> bytes:
    for value in (pan, tilt):
        if not 0 <= value <= 180:
            raise ValueError("camera angles are 0 to 180 degrees")
    return f"{{P,{pan},{tilt}}}".encode("ascii")


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
    longest_frame_us: int  # longest frame's work since the last telemetry (T2)
    battery_status: int | None = None  # signal status; protocol 2 and later

    @property
    def state_name(self) -> str:
        return STATES.get(self.state, str(self.state))

    @property
    def battery_status_name(self) -> str | None:
        if self.battery_status is None:
            return None
        return SIGNAL_STATUSES.get(self.battery_status, str(self.battery_status))

    @property
    def fault_names(self) -> list[str]:
        return [name for bit, name in FAULTS.items() if self.faults & bit]

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["state_name"] = self.state_name
        data["fault_names"] = self.fault_names
        data["battery_status_name"] = self.battery_status_name
        return data

    def describe(self) -> str:
        faults = ", ".join(self.fault_names) or "none"
        battery = f" ({self.battery_status_name})" if self.battery_status_name else ""
        return (f"{self.state_name}, PWM left {self.left_pwm} right {self.right_pwm}, "
                f"battery {self.battery_mv / 1000:.2f} V{battery}, faults: {faults}, "
                f"longest frame {self.longest_frame_us} us")


def parse_message(body: str) -> tuple[str, list[str]]:
    """Splits a message body (without braces) into its type and fields."""
    parts = body.split(",")
    return parts[0], parts[1:]


def parse_telemetry(values: list[str]) -> Telemetry | None:
    """Accepts protocol 1 (7 fields) and protocol 2 (8 fields)."""
    if len(values) not in (len(fields(Telemetry)) - 1, len(fields(Telemetry))):
        return None
    try:
        return Telemetry(*(int(v) for v in values))
    except ValueError:
        return None


@dataclass
class Health:
    """The health message: {H,skipped,event0,event1,...}."""

    skipped_frames: int
    events: dict[str, int]

    def to_dict(self) -> dict[str, Any]:
        return {"skipped_frames": self.skipped_frames,
                "events": {name: EVENT_STATES.get(state, str(state)) for name, state in self.events.items()}}

    def describe(self) -> str:
        events = ", ".join(f"{name}: {EVENT_STATES.get(state, state)}" for name, state in self.events.items())
        return f"{events}; frames skipped: {self.skipped_frames}"


@dataclass
class Sensors:
    """The sensor message: {G,heading,rate,gyro_status}."""

    heading_cdeg: int  # 0 to 35999 hundredths of a degree since power-on
    rate_cdps: int     # hundredths of a degree per second
    gyro_status: int   # signal status

    @property
    def gyro_status_name(self) -> str:
        return SIGNAL_STATUSES.get(self.gyro_status, str(self.gyro_status))

    def to_dict(self) -> dict[str, Any]:
        data = asdict(self)
        data["gyro_status_name"] = self.gyro_status_name
        return data

    def describe(self) -> str:
        return (f"heading {self.heading_cdeg / 100:.1f} deg, turning {self.rate_cdps / 100:.2f} deg/s "
                f"(gyro {self.gyro_status_name})")


def parse_sensors(values: list[str]) -> Sensors | None:
    try:
        numbers = [int(v) for v in values]
    except ValueError:
        return None
    return Sensors(*numbers[:3]) if len(numbers) >= 3 else None


def parse_health(values: list[str]) -> Health | None:
    try:
        numbers = [int(v) for v in values]
    except ValueError:
        return None
    if not numbers:
        return None
    names = EVENTS + [f"event {i}" for i in range(len(EVENTS), len(numbers) - 1)]
    return Health(numbers[0], dict(zip(names, numbers[1:])))


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
