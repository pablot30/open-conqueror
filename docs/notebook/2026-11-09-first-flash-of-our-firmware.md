# First flash of our firmware

**Date:** 2026-11-09. **How we know:** measured on our robot running our increment 1 firmware (version 0.1.0, built from commit `247fdc9`, then the same with the fix below). A computer drove it over USB and logged every telemetry message. The maintainer watched and checked that each track turned the right way. No video.

## Setup

The robot sat on a table with its tracks on the surface, main power on and the switch on Upload. The USB cable is only about 20 cm long, so every move lasted 0.5 s at 30% and was followed by its opposite, which kept the robot near where it started.

The first upload attempts failed with `not in sync: resp=0x00` because the switch was still on Cam. They succeeded once it was moved to Upload. That message is worth recognising: it means the bootloader isn't answering, and the switch is the first thing to check.

## Results

| Check | Requirement | Result |
|---|---|---|
| Version at startup and on request | D4 | `0.1.0, 247fdc9` |
| Motors off from power-on | S5 | Standby, no output |
| Eight directions at 30% | F2, F3 | Every track turned the right way. Motor output was 62 out of 255, and 31 on the slower track of a diagonal |
| Motor voltage limit | S10 | 62 at 30% means about 205 at 100% with the battery at 7.45 V, which averages about 6.0 V |
| Stop command | S1 | Stopped by the next telemetry message, 110 to 170 ms after sending. Telemetry comes 10 times a second, so this can't confirm the 150 ms target |
| Link goes quiet | S3 | Still driving 279 ms after the last message and stopped by 330 ms, with the link-timeout fault set |
| MODE button | S1 | Stops the robot. Checked after the fix below, with two presses |
| Longest loop | T2 | 1.44 ms |
| Battery | S9 | 7.45 V at rest, down to about 7.25 V with both tracks at 30%. Not yet compared with a multimeter |
| Garbled message | | Answered with `{E,1}` |

## A stop that didn't hold

The first firmware let any drive message restart the motors after a stop. The computer tool resends its drive command every 100 ms, so a MODE press would have stopped the robot for at most 0.1 s. A watchdog reset in the middle of a drive had the same gap: the next drive message from the computer would have started the motors again.

The fix: after power-on and after every stop, the firmware ignores drive messages until a neutral one, `{D,0,0}`, arrives. The computer tool sends a neutral message at the start of each command typed at its prompt, but not in its resends. We checked this on the robot with a drive command of 1%, too weak to turn the tracks. Drive messages before the neutral one left the robot in standby. After a stop command, a link timeout or a MODE press, 1 to 1.5 s of resent drive messages left it stopped. A neutral message, then a drive command, started it again. S1 in the [requirements](../requirements.md) now says this.

Unit tests can't find this kind of bug if they test each command on its own. The new tests stop the robot and then keep sending drive commands, like the computer tool does.

## What we learned about testing

- **Let the test wait for the person.** Two MODE button checks ran on a countdown that the maintainer never saw in time, and recorded nothing. The third waited up to two minutes for presses and caught them.
- **Telemetry at 10 Hz can miss a short event.** The first firmware's MODE stop lasted under 100 ms, so it could vanish between two telemetry messages. A test that holds a state open for longer, here by streaming a 1% drive command, makes the event visible.
- **Check the test script too.** Our MODE script once read the last telemetry message from before it re-armed the robot, and reported a press that never happened. The log showed the robot driving the whole time.
