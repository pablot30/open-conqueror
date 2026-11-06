# Motor channels

**Date:** 2026-11-06. **How we know:** measured on our robot running its factory firmware, with main power on and the UNO on USB with the switch in Upload. A computer sent N:1 at speed 100 out of 255 for about 2 seconds per step, then N:100 to stop. The maintainer watched and filmed each step. Left and right are the robot's own, looking the way it drives.

## Results

| Step | Command | What happened |
|---|---|---|
| 1 | Channel A forward | Right track forward |
| 2 | Channel A reverse | Right track backward |
| 3 | Channel B forward | Left track forward |
| 4 | Channel B reverse | Left track backward |
| 5 | Both forward | Robot forward |
| 6 | Both reverse, then channel A forward on its own | Robot backward, then **robot forward** |

**The schematic is right:** channel A drives the right track and channel B the left, and the stock firmware's "forward" drives both tracks forward. Speed 100 was enough to move the robot on a table.

## A stock bug, confirmed

In the second half of step 6, only channel A was told to run, yet the robot drove straight forward, so the left track ran forward too. That is prediction 2 from the [first look at the stock firmware](2026-11-03-first-look-at-stock-firmware.md#predictions-to-test-in-stage-1). Code meant to leave the other motor alone turns into "forward", at that motor's last speed. Our firmware must not repeat this.

## What we learned about testing

The first run was inconclusive, and the repeat worked because we changed four things:

- **One fixed viewpoint:** the maintainer stood, and the phone sat, behind the robot. Facing the robot's front swaps left and right, which had already cost us a [wrong conclusion about the line sensors](2026-11-06-first-sensor-readings.md#left-and-right-match-elegoos-naming).
- **A steady camera:** a phone held in the hand moved too much to show which way the robot turned.
- **One line per step:** answers like "3: left forward" leave no room for doubt.
- **The sound in the video:** the motor noise lined up with the computer's command log to within 0.1 s, so the timing of every step could be checked afterwards. An automated test rig could use this.

Both runs happened on a table, because the tracks touched it. A stand that keeps the tracks clear would make the video easier to read, and it would stop the robot pulling on its short USB cable.
