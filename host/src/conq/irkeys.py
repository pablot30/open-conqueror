"""Recording our IR remote's key codes on the robot.

The firmware reports every key it receives in a `{K,address,command,kind}`
message. This asks the person to press each key in turn, remembers its code,
and prints the lines to paste into firmware/uno/devices/ir/src/ir_keys.cpp.
"""

from __future__ import annotations

from typing import Callable

from .link import Robot

# The keys the firmware uses now, then the ones increment 3 will use. The
# first group is required; the second is recorded now to save a trip, and
# missing ones aren't a failure.
REQUIRED = ["up", "down", "left", "right", "ok"]
OPTIONAL = ["1", "2", "3", "4", "5", "6"]
KEY_ENUM = {"up": "kUp", "down": "kDown", "left": "kLeft", "right": "kRight", "ok": "kOk"}
WAIT_S = 20.0
RELEASE_S = 0.6  # quiet time that tells a new press from a held one


def record_key(robot: Robot, name: str, say: Callable[[str], None], timeout: float = WAIT_S) -> dict[str, int] | None:
    """Asks for one key and waits for a code. Returns {"address":..., "command":...} or None."""
    say(f"Press and release the {name.upper()} key once.")
    asked = robot.now()
    seen = robot.wait_for(lambda: [c for _, c in robot.ir_codes(asked) if c.is_key], timeout)
    if not seen:
        unreadable = [c for _, c in robot.ir_codes(asked) if c.kind == 2]
        if unreadable:
            say("  The robot saw pulses it couldn't read as NEC. The remote may use another protocol, "
                "or the receiver's polarity may differ. Send the log to the maintainer.")
        else:
            say("  Nothing arrived.")
        return None
    code = seen[0]
    robot.wait_for(lambda: False, RELEASE_S)  # let the key's repeats stop before the next question
    return {"address": code.address, "command": code.command}


def record_keys(robot: Robot, names: list[str], say: Callable[[str], None]) -> dict[str, dict[str, int] | None]:
    return {name: record_key(robot, name, say) for name in names}


def table_lines(codes: dict[str, dict[str, int] | None]) -> list[str]:
    """C++ lines for ir_keys.cpp. Keys the firmware doesn't know yet are comments."""
    lines = []
    for name, code in codes.items():
        if code is None:
            lines.append(f"// {name}: not recorded")
        elif name in KEY_ENUM:
            lines.append(f"    {{Key::{KEY_ENUM[name]}, 0x{code['address']:04X}U, 0x{code['command']:02X}U, true}},")
        else:
            lines.append(f"// key {name}: address 0x{code['address']:04X}, command 0x{code['command']:02X}"
                         " (for the mode keys, increment 3)")
    return lines
