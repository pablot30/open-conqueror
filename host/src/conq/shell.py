"""The interactive prompt: `conq shell`. For people; agents use the other commands."""

from __future__ import annotations

import cmd

from . import protocol
from .link import Message, Robot


class Shell(cmd.Cmd):
    intro = "Connected. Type help for commands. Ctrl+C stops the robot while it drives."
    prompt = "conq> "

    def __init__(self, robot: Robot) -> None:
        super().__init__()
        self.robot = robot
        robot.on_message = self._show

    def _show(self, message: Message) -> None:
        if message.direction != "rx":
            return
        kind, values = protocol.parse_message(message.body)
        if kind == "V":
            print(f"\n[robot] firmware {', '.join(values)}")
        elif kind == "E":
            print(f"\n[robot] error {', '.join(values)}: message not understood")
        elif kind != "T":
            print(f"\n[robot] {message.body}")

    def _drive(self, left: int, right: int, seconds: float) -> None:
        try:
            self.robot.drive_for(left, right, seconds)
        except KeyboardInterrupt:
            print()

    def do_go(self, arg: str) -> None:
        """go DIR [SPEED] [SECONDS]: drive one of f b l r fl fr bl br.
        SPEED is 1 to 100 percent (default 40), SECONDS defaults to 1."""
        parts = arg.split()
        try:
            direction = parts[0]
            speed = int(parts[1]) if len(parts) > 1 else 40
            seconds = float(parts[2]) if len(parts) > 2 else 1.0
            left, right = protocol.direction_speeds(direction, speed)
        except (IndexError, ValueError) as error:
            print(f"Usage: go DIR [SPEED] [SECONDS] ({error})")
            return
        self._drive(left, right, seconds)

    def do_drive(self, arg: str) -> None:
        """drive LEFT RIGHT [SECONDS]: set each track, -100 to 100 percent."""
        parts = arg.split()
        try:
            left, right = int(parts[0]), int(parts[1])
            seconds = float(parts[2]) if len(parts) > 2 else 1.0
            protocol.drive_message(left, right)
        except (IndexError, ValueError) as error:
            print(f"Usage: drive LEFT RIGHT [SECONDS] ({error})")
            return
        self._drive(left, right, seconds)

    def do_stop(self, _: str) -> None:
        """stop: brake both tracks."""
        self.robot.send(protocol.STOP)

    def do_version(self, _: str) -> None:
        """version: ask the firmware for its version and git commit."""
        print(self.robot.version() or "No answer.")

    def do_status(self, _: str) -> None:
        """status: show the latest telemetry."""
        latest = self.robot.latest()
        print(latest.describe() if latest else "No telemetry yet.")

    def do_quit(self, _: str) -> bool:
        """quit: stop the robot and exit."""
        return True

    do_EOF = do_quit
