# servos

**Layer:** devices. **Version:** in `library.properties`. **Not run on the robot yet.**

The camera's pan (D10) and tilt (D11) servos. Centred at power-on. Moves ramp: each 10 ms frame the angle steps towards the target at a set speed (100 degrees a second unless a move says otherwise; speed 0 jumps). Half a second after both servos arrive they are released, so they don't hum or jitter; they hold by friction.

An SG90 can't report its position, so the ramp counts its own angle in hundredths of a degree, and assumes the camera is at the centre at power-on (it sends the servos there first). If someone turns the camera by hand while it's released, the next move starts from the wrong angle and the camera may jump at first.

| | |
|---|---|
| **Requirements** | F14 (centre at power-on, move at a set speed) |
| **Interface** | `ServoRamp`: `setTarget(pan, tilt, speedDps)`, `update(frame)` (says write, release or nothing), `pan()`, `tilt()`; `servos_hw::move(pan, tilt)`, `servos_hw::release()` in `servos_hw.cpp` |
| **Unit tests** | `tests/test_servos.cpp`, for the logic. Files ending in `_hw.cpp` use the hardware and only build for the board |
| **Monitors** | None: an SG90 gives no feedback. |

Uses the Arduino Servo library 1.3.0 (pinned in `sketch.yaml`), which takes Timer1. Whether 90 degrees points the camera straight ahead is checked on the robot.

Test this unit on its own with `python scripts/dev.py test servos`.
