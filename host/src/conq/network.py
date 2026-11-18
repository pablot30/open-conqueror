"""A connection to the robot over its own Wi-Fi network, without the dongle.

The stock camera board bridges TCP port 100 on 192.168.4.1 to the UNO
(docs/hardware/camera-board.md). It sends {Heartbeat} each second and drops a
client that doesn't answer. This link does what the dongle does
(firmware/dongle/README.md): it answers each heartbeat, keeps heartbeats out of
the robot's messages, treats 3.5 s of silence as a lost connection, and then
opens the connection again every 2 s. Messages sent while it's down are
dropped; the robot's 300 ms drive timeout (S3) stops it.

Only the Python standard library is used.
"""

from __future__ import annotations

import socket
import threading
import time
from typing import Callable

PREFIX = "tcp:"
DEFAULT_PORT = 100
HEARTBEAT = b"{Heartbeat}"
LINK_TIMEOUT_S = 3.5  # as the dongle's kLinkTimeoutMs
REOPEN_PERIOD_S = 2.0  # as the dongle's kOpenPeriodMs
OPEN_TIMEOUT_S = 3.0
MAX_MESSAGE = 96  # braces included, as the dongle: longer messages are dropped


def is_network_port(port: str) -> bool:
    return port.lower().startswith(PREFIX)


def parse_address(port: str) -> tuple[str, int]:
    """'tcp:192.168.4.1:100' or 'tcp:192.168.4.1' to (host, port)."""
    rest = port[len(PREFIX):] if is_network_port(port) else port
    host, _, number = rest.rpartition(":") if ":" in rest else (rest, "", "")
    if not host:
        raise ValueError(f"{port}: give the address as tcp:HOST or tcp:HOST:PORT, such as tcp:192.168.4.1:100")
    try:
        tcp_port = int(number) if number else DEFAULT_PORT
    except ValueError:
        raise ValueError(f"{port}: the TCP port must be a number") from None
    if not 0 < tcp_port < 65536:
        raise ValueError(f"{port}: the TCP port must be 1 to 65535")
    return host, tcp_port


class NetworkLink:
    """Reads and writes like a serial port. on_change(connected, reason) is
    called from the reading thread when the connection is lost or opened again."""

    def __init__(self, port: str, on_change: Callable[[bool, str], None] | None = None) -> None:
        self.address = parse_address(port)
        self.on_change = on_change
        self._lock = threading.Lock()  # writes come from two threads
        self._socket: socket.socket | None = None
        self._body: bytearray | None = None
        self._last_rx = 0.0
        self._last_attempt = 0.0
        self._closed = False
        error = self._open()
        if error:
            host, tcp_port = self.address
            raise OSError(f"couldn't reach the robot at {host}:{tcp_port} ({error}). Is this computer "
                          "on the robot's ELEGOO-... Wi-Fi network, and the robot switched on?")

    def _open(self) -> str:
        """Opens the connection; returns why it failed, or '' if it worked."""
        self._last_attempt = time.monotonic()
        try:
            sock = socket.create_connection(self.address, timeout=OPEN_TIMEOUT_S)
        except OSError as error:
            return str(error) or type(error).__name__
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.settimeout(0.05)
        self._body = None
        self._last_rx = time.monotonic()
        with self._lock:
            self._socket = sock
        return ""

    def _lose(self, reason: str) -> None:
        with self._lock:
            sock, self._socket = self._socket, None
        if sock is None:
            return
        sock.close()
        self._last_attempt = time.monotonic()
        if self.on_change is not None:
            self.on_change(False, reason)

    def write(self, data: bytes) -> None:
        with self._lock:
            if self._socket is None:
                return  # dropped while the connection is down
            try:
                self._socket.sendall(data)
            except OSError:
                pass  # the reading thread notices and reports the loss

    def read(self, size: int = 64) -> bytes:
        """The robot's bytes, as whole {...} messages without heartbeats.
        Returns b'' after about 50 ms with nothing to read."""
        sock = self._socket
        if sock is None:
            if self._closed or time.monotonic() - self._last_attempt < REOPEN_PERIOD_S:
                time.sleep(0.05)
                return b""
            if self._open() == "" and self.on_change is not None:
                self.on_change(True, "")
            return b""
        try:
            data = sock.recv(size)
        except socket.timeout:
            data = None
        except OSError as error:
            self._lose(f"the connection failed: {error}")
            return b""
        if data == b"":
            self._lose("the camera board closed the connection")
            return b""
        if data is None:
            if time.monotonic() - self._last_rx > LINK_TIMEOUT_S:
                self._lose(f"no message from the camera board for {LINK_TIMEOUT_S} s")
            return b""
        self._last_rx = time.monotonic()
        return self._split(data)

    def _split(self, data: bytes) -> bytes:
        out = bytearray()
        for byte in data:
            if byte == ord("{"):  # a new message starts, even inside one that never ended
                self._body = bytearray()
            if self._body is None:
                continue  # bytes outside braces are dropped
            self._body.append(byte)
            if len(self._body) > MAX_MESSAGE:
                self._body = None  # too long: dropped
            elif byte == ord("}"):
                message, self._body = bytes(self._body), None
                if message == HEARTBEAT:
                    self.write(HEARTBEAT)
                else:
                    out += message
        return bytes(out)

    def close(self) -> None:
        self._closed = True
        with self._lock:
            sock, self._socket = self._socket, None
        if sock is not None:
            sock.close()
