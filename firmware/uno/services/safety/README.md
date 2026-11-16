# safety

**Layer:** services. **Version:** in `library.properties`.

The safety supervisor: the only code that decides what the motors do. It applies arming (a neutral command before driving), stops, the link timeout and the battery cutoff ([architecture](../../../../docs/architecture.md), A2).

| | |
|---|---|
| **Requirements** | F4, S1 (stops hold until a neutral command), S3 (300 ms link timeout), S4 and S5 (no driving after power-on or a reset until a neutral command), S9 (cutoff) |
| **Interface** | `SafetySupervisor`: `drive(left, right, nowMs)`, `stop()`, `setTrim(permille)` for heading hold's correction, `linkTimedOut()`, `update(nowMs, pwmLimit, driveAllowed)` returns the `MotorOutput` |
| **Unit tests** | `tests/test_safety.cpp` |
| **Monitors** | `link timeout` in diagnostics: `uno.ino` reports `linkTimedOut()` every frame |

Test this unit on its own with `python scripts/dev.py test safety`.
