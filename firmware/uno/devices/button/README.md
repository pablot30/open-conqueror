# button

**Layer:** devices. **Version:** in `library.properties`.

Debounces the MODE button and reports each press once.

| | |
|---|---|
| **Requirements** | S1 (the MODE button stops the robot) |
| **Interface** | `Debouncer(settleMs)`, `update(pressed, nowMs)` returns true once per press |
| **Unit tests** | `tests/test_button.cpp` |
| **Monitors** | None: a button that isn't pressed isn't a fault |

Test this unit on its own with `python scripts/dev.py test button`.
