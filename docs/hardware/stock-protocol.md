# Stock protocol

How ELEGOO's EleRobot app talks to the stock UNO firmware. **This is reference only, not a target.** Our firmware [uses its own protocol](../decisions/0004-own-protocol-and-control-page.md). We keep this page because stage 1 uses these commands to read sensors and run motors on the stock firmware from a computer, without writing any firmware.

**Source:** the stock firmware's command handler, version TB6612 2024-06-05. ELEGOO's protocol PDF disagrees with the firmware in several places, so we treat the firmware as the reference. None of this has been tested on a robot yet.

## Format

- JSON objects at 9600 baud, 8N1, ended by `}`.
- `N` is the command number. `D1`, `D2` and so on are parameters, and `T` is a time in milliseconds.
- `H` is an optional ID that the reply echoes, as in `{H_ok}` or `{H_true}`.
- There's no checksum, sequence number or timeout.

## Commands

Speeds are 0 to 255.

| N | Purpose | Parameters | Reply |
|---|---|---|---|
| 1 | Run motors | D1: 0 both, 1 and 2 one motor. D2: speed. D3: 0 stop, 1 forward, 2 reverse | `{H_ok}` |
| 2 | Drive for a time | D1: 1 left, 2 right, 3 forward, 4 backward. D2: speed. T: duration | `{H_ok}` when finished |
| 3 | Drive until told otherwise | D1: as N:2. D2: speed | `{H_ok}` |
| 4 | Set both track speeds, forward only | D1 and D2: speeds. Both 0 stops | `{H_ok}` |
| 5 | Move servos | D1: 1 pan, 2 tilt, 3 both. D2: angle 0 to 180 | `{H_ok}` |
| 7 | Set LED for a time | D1: position. D2, D3, D4: red, green, blue. T: duration | `{H_ok}` when finished |
| 8 | Set LED | D1: position. D2, D3, D4: red, green, blue | `{H_ok}` |
| 21 | Ultrasonic | D1: 1 obstacle within 20 cm, 2 distance in cm | `{H_true}`, `{H_false}` or `{H_<cm>}` |
| 22 | Line sensor | D1: 0 left, 1 middle, 2 right | `{H_<raw value>}` |
| 23 | Lifted off the ground? | None | `{H_true}` or `{H_false}` |
| 100 | Stop everything, go to standby | None | `{ok}` |
| 101 | Select mode | D1: 1 line tracking, 2 obstacle avoidance, 3 auto follow | `{ok}` |
| 102 | Joystick | D1: 1 forward, 2 backward, 3 left, 4 right, 5 left forward, 6 left backward, 7 right forward, 8 right backward, 9 stop. D2: speed | None |
| 105 | LED brightness | D1: 1 up, 2 down, in steps of 5 | None |
| 106 | Camera step | D1: 1 and 2 tilt by 10°, 3 and 4 pan by 10°, 5 centre | None |
| 110 | Stop everything, go to programming mode | None | `{H_ok}` |

For N:7 and N:8, D1 selects an LED position (0 all, 1 left, 2 front, 3 right, 4 back, 5 centre), but the robot has only one LED.

The PDF also lists commands 6, 9, 10, 103 and 104, which this firmware doesn't implement.

## Things to confirm in stage 1

- Which track N:1's D1=1 and N:4's D1 drive. See the [open questions](README.md#open-questions).
- That the commands and replies match this table on the real robot.
