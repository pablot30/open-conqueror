"""Records our IR remote's key codes, so the firmware's key table can be filled in."""

REQUIREMENTS = ["F10"]
MOVES = False
SETUP = """Robot on a table, USB connected, Upload-Cam switch on Upload, main power
on. The remote that came with the robot, with a battery in it, in your hand.
Point it at the small black IR receiver at the front of the robot, a metre
away or less, and keep other remotes and strong sunlight away from it."""
BRIEF = """The tracks don't move. The test asks for each key in turn: the four arrows,
OK, then the number keys 1 to 6. When it names a key, press and release that
key once on the remote, aimed at the robot's front, and wait for the next
name. It waits up to 20 seconds for each key. The arrows and OK are
required; the number keys are recorded for increment 3 and may be skipped by
waiting. At the end the test prints the lines to paste into
firmware/uno/devices/ir/src/ir_keys.cpp. About 2 minutes."""


def run(t):
    from conq import irkeys

    robot = t.robot
    codes = irkeys.record_keys(robot, irkeys.REQUIRED + irkeys.OPTIONAL, t.say)
    for name in irkeys.REQUIRED:
        t.check(f"{name} key recorded", codes[name] is not None,
                "" if codes[name] is not None else "no code in 20 s", ["F10"])
    distinct = {(c["address"], c["command"]) for n, c in codes.items() if c is not None and n in irkeys.REQUIRED}
    recorded = [n for n in irkeys.REQUIRED if codes[n] is not None]
    unique = len(distinct) == len(recorded)
    t.check("the required keys have different codes", unique,
            "" if unique else "two keys gave the same code", ["F10"])
    t.check("key table (paste into ir_keys.cpp)", True, "\n".join(irkeys.table_lines(codes)), ["F10"])
