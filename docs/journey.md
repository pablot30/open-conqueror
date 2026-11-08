# Journey

The story of the project, one dated entry per milestone or memorable moment. It's raw material for write-ups and videos later. The technical details live in the [notebook](notebook/README.md) and the [decisions](decisions/README.md), so entries here stay short and link to them.

Photos and videos are too large for the repo. The maintainer keeps them privately, and entries name the files.

## 2026-11-03: The idea

The kit is assembled and working. A first read of ELEGOO's firmware finds it fills 98% of the UNO's flash and has bugs the compiler warns about. We agree on the idea: rebuild the software from scratch through AI-driven development, starting with the stock hardware. We pick the name `open-conqueror`. See the [first look at the stock firmware](notebook/2026-11-03-first-look-at-stock-firmware.md).

## 2026-11-04: First commit and the backup that lied

The docs get their first local commit. Backing up the UNO, the EEPROM read succeeds twice and still turns out to be a copy of flash, because the bootloader can't read EEPROM. **Story:** "a clean read can still be wrong". See [backing up the UNO](notebook/2026-11-04-backing-up-the-uno.md).

## 2026-11-06: The robot meets the computer

- **Sensor tests:** the first recording misses everything because there's no start signal. The fix is "reply go, wait 15 seconds".
- **The left/right mix-up:** standing in front of the robot swaps its left and right. We record a wrong conclusion and correct it a day later. **Story:** agree on conventions before testing.
- **The camera board:** a USB-C to USB-C cable can't power it, because it lacks two small resistors. The backup works over a USB-A cable.
- **Motors:** the first run, filmed by hand on a table, settles nothing. The second, from behind with a fixed phone, confirms the channels and a stock bug. Both videos were test footage; better ones get filmed later.

See the notebook entries for [sensors](notebook/2026-11-06-first-sensor-readings.md), the [camera board](notebook/2026-11-06-backing-up-the-camera-board.md) and [motor channels](notebook/2026-11-06-motor-channels.md).

## 2026-11-08: From poking at the robot to planning

- **The failed prediction:** reading the code said obstacle mode wouldn't start on a clear path. On the robot, it drove off.
- **Less measuring:** we decide not to film the stock modes as a reference, because the backups let us measure stock firmware later.
- **Publishing:** we decide to publish once our own firmware works, not before ([0005](decisions/0005-publish-with-first-working-firmware.md)). The repo goes to GitHub as private.
- **Requirements:** features, safety goals derived from hazards, and quality rules including full MC/DC coverage and no new static analysis findings. See the [requirements](requirements.md).
- **Protecting main:** GitHub can't protect a private repo's branches on the free plan, so a git hook does it for now.

## Ideas for content

- How an AI agent and an engineer split the work on real hardware: who touches the robot, and who writes and checks.
- Lessons from testing physical hardware with an AI in the loop: start signals, viewpoints, fixed cameras.
- Automotive engineering practice on a hobby robot: safety goals, DTC-style diagnostics, MC/DC coverage.
