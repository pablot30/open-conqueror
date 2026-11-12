"""The connection to one robot over a serial port.

Robot keeps every message it sends and receives, with a timestamp, so
commands and robot tests can check what happened and save it as a JSON Lines
log.
"""

from __future__ import annotations

import json
import os
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import IO, Any, Callable

from . import protocol
from .protocol import Health, Sensors, Telemetry

BOOT_SECONDS = 2.0  # opening the port restarts the UNO through the USB chip's DTR line
RESEND_PERIOD_S = 0.1  # well inside the firmware's 300 ms link timeout


class PortError(Exception):
    pass


def usb_ports() -> list[dict[str, Any]]:
    """Serial ports with a USB vendor ID, which excludes Bluetooth and debug ports."""
    from serial.tools import list_ports

    return [{"port": p.device, "description": p.description, "vid": p.vid, "pid": p.pid}
            for p in list_ports.comports() if p.vid is not None]


def choose_port(requested: str | None) -> str:
    """The port given, else $CONQ_PORT, else the only USB serial port."""
    if requested:
        return requested
    if os.environ.get("CONQ_PORT"):
        return os.environ["CONQ_PORT"]
    ports = usb_ports()
    if len(ports) == 1:
        return str(ports[0]["port"])
    if not ports:
        raise PortError("No USB serial port found. Is the robot connected?")
    raise PortError("Several USB serial ports found; pick one with --port: "
                    + ", ".join(str(p["port"]) for p in ports))


@dataclass
class Message:
    t: float  # seconds since the connection opened
    direction: str  # "rx" from the robot, "tx" to it, "note" for test notes
    body: str

    def to_dict(self) -> dict[str, Any]:
        return {"t": round(self.t, 4), "dir": self.direction, "msg": self.body}


class Robot:
    def __init__(self, port: str, log_path: Path | None = None) -> None:
        import serial  # imported here so the rest of the package works without pyserial

        self.port = port
        try:
            self._serial = serial.Serial(port, protocol.BAUD, timeout=0.05)
        except serial.SerialException as error:
            raise PortError(f"Couldn't open {port}: {error}") from error
        self._start = time.monotonic()
        self._lock = threading.Lock()
        self.messages: list[Message] = []
        self._log: IO[str] | None = log_path.open("w", encoding="utf-8") if log_path else None
        self.on_message: Callable[[Message], None] | None = None
        self._running = True
        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    # --- Sending and receiving ---

    def now(self) -> float:
        return time.monotonic() - self._start

    def send(self, data: bytes) -> None:
        self._serial.write(data)
        self._record("tx", data.decode("ascii", errors="replace").strip("{}"))

    def note(self, text: str) -> None:
        """Adds a note to the log, such as the start of a test step."""
        self._record("note", text)

    def close(self) -> None:
        try:
            self.send(protocol.STOP)
            time.sleep(0.1)
        finally:
            self._running = False
            self._reader.join(timeout=1)
            self._serial.close()
            if self._log is not None:
                self._log.close()

    def _record(self, direction: str, body: str) -> None:
        message = Message(self.now(), direction, body)
        with self._lock:
            self.messages.append(message)
            if self._log is not None:
                self._log.write(json.dumps(message.to_dict()) + "\n")
                self._log.flush()
        if self.on_message is not None:
            self.on_message(message)

    def _read_loop(self) -> None:
        splitter = protocol.MessageSplitter()
        while self._running:
            try:
                data = self._serial.read(64)
            except Exception:  # the port went away; stop reading
                return
            for body in splitter.feed(data):
                self._record("rx", body)

    # --- Reading what arrived ---

    def received(self, since: float = 0.0, kind: str | None = None) -> list[Message]:
        with self._lock:
            return [m for m in self.messages if m.direction == "rx" and m.t >= since
                    and (kind is None or protocol.parse_message(m.body)[0] == kind)]

    def telemetry(self, since: float = 0.0, until: float | None = None) -> list[tuple[float, Telemetry]]:
        result = []
        for message in self.received(since, "T"):
            if until is not None and message.t > until:
                continue
            parsed = protocol.parse_telemetry(protocol.parse_message(message.body)[1])
            if parsed is not None:
                result.append((message.t, parsed))
        return result

    def health(self, since: float = 0.0) -> list[tuple[float, Health]]:
        result = []
        for message in self.received(since, "H"):
            parsed = protocol.parse_health(protocol.parse_message(message.body)[1])
            if parsed is not None:
                result.append((message.t, parsed))
        return result

    def sensors(self, since: float = 0.0) -> list[tuple[float, Sensors]]:
        result = []
        for message in self.received(since, "G"):
            parsed = protocol.parse_sensors(protocol.parse_message(message.body)[1])
            if parsed is not None:
                result.append((message.t, parsed))
        return result

    def latest(self) -> Telemetry | None:
        samples = self.telemetry()
        return samples[-1][1] if samples else None

    def wait_for(self, predicate: Callable[[], Any], timeout: float, poll: float = 0.02) -> Any:
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            value = predicate()
            if value:
                return value
            time.sleep(poll)
        return None

    # --- Commands ---

    def wait_for_boot(self) -> None:
        """Waits for the restart that opening the port causes, and the first telemetry."""
        self.wait_for(lambda: self.telemetry(), BOOT_SECONDS + 1.0)

    def version(self, timeout: float = 1.0) -> dict[str, str] | None:
        since = self.now()
        self.send(protocol.VERSION)
        message = self.wait_for(lambda: self.received(since, "V"), timeout)
        if not message:
            return None
        values = protocol.parse_message(message[-1].body)[1]
        keys = ["version", "commit", "protocol"]
        return {key: values[i] if i < len(values) else "" for i, key in enumerate(keys)}

    def stream(self, message: bytes, seconds: float) -> None:
        """Sends one message every 100 ms for this long."""
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            self.send(message)
            time.sleep(RESEND_PERIOD_S)

    def drive_for(self, left: int, right: int, seconds: float, stop: bool = True) -> None:
        """Drives for this long, as a deliberate new command: neutral first, then
        resends, then a stop unless stop is False."""
        message = protocol.drive_message(left, right)
        try:
            self.send(protocol.NEUTRAL)
            self.stream(message, seconds)
        finally:
            if stop:
                self.send(protocol.STOP)
