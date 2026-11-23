# ultrasonic

**Layer:** devices. **Version:** in `library.properties`. **First run on the robot on 2026-11-15:** a table test against a tape measure. **Retested on the floor on 2026-11-20** (#30, [notebook](../../../../docs/notebook/2026-11-20-battery-and-ultrasonic-measurements.md)).

The distance sensor on header P9: trigger on D13, echo on D12. The logic sends one ping at most every 60 ms and turns an echo's width into millimetres. A pin-change interrupt times the echo, so no frame ever waits for it. The stock firmware blocks for up to 1 s here (T2) and reads 25 to 35% short (T3).

| | |
|---|---|
| **Requirements** | T3 (within ±1 cm from 5 to 150 cm), T2 (no blocking), A5 (signal status) |
| **Interface** | `Ranger::update(nowMs, echoComplete, echoWidthUs)` returns what to do this frame; `lastDistanceMm()`; `echoWidthToMm(us)`; `ultrasonic_hw::begin()`, `trigger()`, `takeEcho(widthUs)` in `ultrasonic_hw.cpp` |
| **Unit tests** | `tests/test_ultrasonic.cpp`, for the logic and for the logic with diagnostics (`tests/uses.txt`). `_hw.cpp` only builds for the board |
| **Monitors** | `ultrasonic echo` in diagnostics: after each ping, an echo of at least 100 µs arrives within 50 ms. It needs 5 misses in a row to fail and 3 good echoes to heal |
| **Robot tests** | `tests/robot/ultrasonic_ruler.py` (T3) |

## How it works

- **Timing.** The echo pin's two edges are timed with `micros()` inside the pin-change interrupt from [`hal/pinchange`](../../hal/pinchange/README.md), which D12 shares with the IR receiver. No timer is used: Timer0 is `millis()` and the motor PWM, and Timer1 is the Servo library. `micros()` counts in 4 µs steps at 16 MHz, about 0.7 mm. The trigger pulse is the one wait, 10 µs.
- **Conversion.** Distance is the echo's width × 343 / 2000 mm, for the speed of sound in dry air at 20 °C (331.3 + 0.606 × 20, the usual linear formula, rounded), halved because the sound goes there and back. We don't correct for temperature (about 0.18% a degree). `config::kUltrasonicOffsetMm` stays 0: the retest of 2026-11-20 found no offset or scale error to correct.
- **Our guess about the stock error:** `pulseIn` miscounts while other interrupts run ([notebook](../../../../docs/notebook/2026-11-06-first-sensor-readings.md)). Timing the edges in an interrupt avoids that, and the retest of 2026-11-20 found the stock shortfall gone.
- **Missing echoes.** Soft targets such as cloth give no echo, and the stock firmware read them as 150 cm, the same as an empty room. Here an echo that doesn't end within 50 ms is a failed monitor, not a distance. One miss makes the distance a *substitute* (the last valid value), several make it an *error* whose value is 0, so a control that ignores the status fails towards stopping, and the `kInhibitRanging` reaction marks obstacle avoidance and follow as unavailable until the sensor heals. The ping period (60 ms) is longer than the timeout (50 ms).
- **How we know the sensor type.** The module has a trigger and an echo pin, and the stock firmware divides the echo time by 58, which is what HC-SR04-style modules need. We haven't read the module's datasheet, so the 10 µs trigger, the 38 ms "nothing in range" pulse and the 4 m limit are the usual values for that type, not measured on our robot.

Test this unit on its own with `python scripts/dev.py test ultrasonic`.
