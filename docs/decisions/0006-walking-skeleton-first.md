# 0006: A walking skeleton first, everything else when it's needed

**Date:** 2026-11-09. **Status:** accepted.

## Context

Stage 2 planning grew faster than the firmware. The requirements asked for MISRA C++ with a rule-mapping table, 100% MC/DC coverage, description files in DBC or ARXML, wireless updates for both boards, signed releases, stored fault codes and breakpoint debugging, and three research reports were still planned. None of this firmware existed yet. Every detail made the plan harder to agree and pushed the first working code further away.

## Decision

- We build stage 2 in small increments that each run on the robot. The first is a **walking skeleton**: the UNO drives both tracks from a computer over USB, stops on command, on a button press and when the link goes quiet, protects the motors and battery, has a watchdog and reports its version and state. [Requirements](../requirements.md#build-order) lists what each increment contains.
- A requirement, tool or process step joins an increment only when that increment needs it. Everything else waits on a "later" list, with what would bring it back.
- The minimum quality bar from the start is pull requests, CI on macOS, Linux and Windows, compiler warnings as errors in our own code, one static analyzer (cppcheck), and unit tests on a computer for the logic that doesn't touch hardware. Coverage gates and the MISRA mapping wait until the code is stable enough to be worth it.
- Research reports R2 to R4 are dropped. The protocol is decided with the walking skeleton's code and documented next to it, wireless updates move to stage 3, and GitHub Issues is already our planning tool.

## Wireless updates for the UNO

The maintainer asked whether updating the UNO over Wi-Fi could come earlier. On the stock hardware it can't come before stage 3:

- The stock camera firmware only forwards text inside `{` and `}` at 9600 baud, so a binary firmware image can't pass through it ([camera board](../hardware/camera-board.md#working-with-the-stock-bridge)).
- The UNO's bootloader, Optiboot 4.4, only starts after an external reset, such as the reset button or the USB chip's reset line. Our reading of Optiboot's public source is that it skips straight to the firmware after a watchdog or power-on reset. Not checked on our robot.
- We know of no wire from the camera board to the UNO's reset pin. Not checked against the schematic.

So it needs our own camera firmware, plus either a reset path or a different bootloader. We'll look at this when stage 3 starts.

## Consequences

- The first working firmware arrives sooner, and it decides publishing ([0005](0005-publish-with-first-working-firmware.md)).
- The full requirements stay as the target. Only their order changes.
- Some decisions we skip now will cost rework later, such as the protocol format. We accept that: changing working code is cheaper than designing code that doesn't exist.
