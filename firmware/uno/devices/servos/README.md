# servos

**Layer:** devices. **Version:** in `library.properties`. **Not run on the robot yet.**

The camera's pan (D10) and tilt (D11) servos. Centred at power-on, then released after 0.5 s so they don't hum or jitter; they hold by friction.

| | |
|---|---|
| **Requirements** | F14 (centre at power-on) |
| **Interface** | `ServoMove`: `start(frame)`, `finished(frame)`; `servos_hw::move(pan, tilt)`, `servos_hw::release()` in `servos_hw.cpp` |
| **Unit tests** | `tests/test_servos.cpp`, for the logic. Files ending in `_hw.cpp` use the hardware and only build for the board |
| **Monitors** | None: an SG90 gives no feedback. |

Uses the Arduino Servo library 1.3.0 (pinned in `sketch.yaml`), which takes Timer1. Whether 90 degrees points the camera straight ahead is checked on the robot.

Test this unit on its own with `python scripts/dev.py test servos`.
