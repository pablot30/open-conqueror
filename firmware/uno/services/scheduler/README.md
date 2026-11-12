# scheduler

**Layer:** services. **Version:** in `library.properties`. **Not run on the robot yet.**

The time-triggered frame: the firmware's work runs once every 10 ms, and each frame's work is timed.

| | |
|---|---|
| **Requirements** | T1 (100 frames a second), T2 (no frame's work over 5 ms; overruns are skipped and counted, never run late in a burst) |
| **Interface** | `FrameClock::due(nowUs)`, `frames()`, `skipped()`; `Stopwatch::record(startUs, endUs)`, `longestUs()`; `every(frames, period, phase)` |
| **Unit tests** | `tests/test_scheduler.cpp` |
| **Monitors** | None of its own. `uno.ino` reports each frame's work time to the `frame overrun` event in diagnostics |

Test this unit on its own with `python scripts/dev.py test scheduler`.
