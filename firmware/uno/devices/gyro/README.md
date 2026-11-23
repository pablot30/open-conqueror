# gyro

**Layer:** devices. **Version:** in `library.properties`. **Tested on the robot on 2026-11-20,** with `startup`, `gyro_turn` and `heading_hold`, in the [increment 2 test report](../../../../docs/test-reports/2026-11-20-increment-2/README.md).

Turn rate and heading from the MPU6050's Z axis. Calibrates its zero rate over the first second at power-on, starting again if the robot moves, then integrates the heading every frame.

| | |
|---|---|
| **Requirements** | F13 (calibration at power-on), T6 (ready in under 3 s), T4 (no dead band: drift down to 0.5 degrees/s shows up), F1 (heading for heading hold, increment 2 part 3) |
| **Interface** | `HeadingGyro`: `update(raw)`, `missedFrame()`, `calibrated()`, `rateCdps()`, `headingCdeg()`; `gyro_hw::begin()`, `gyro_hw::readZ(raw)` in `gyro_hw.cpp` |
| **Unit tests** | `tests/test_gyro.cpp`, for the logic. Files ending in `_hw.cpp` use the hardware and only build for the board |
| **Monitors** | `gyro comms` in diagnostics: the chip answers on I2C. Reads time out after 2 ms instead of hanging the frame. The signal is *init* until calibrated |

Registers from InvenSense's public MPU-6000/MPU-6050 register map. Whether a left turn reads positive depends on how the board is mounted; `tests/robot/gyro_turn.py` checks it.

Test this unit on its own with `python scripts/dev.py test gyro`.
