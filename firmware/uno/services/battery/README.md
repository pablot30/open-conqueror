# battery

**Layer:** services. **Version:** in `library.properties`.

Filters the battery voltage, sets the low and critical levels, and limits motor PWM so the motors' average voltage stays at 6 V or below.

| | |
|---|---|
| **Requirements** | S9 (battery warning and cutoff), S10 (motor voltage limit) |
| **Interface** | `BatteryMonitor`: `sample(adc)`, `millivolts()`, `level()`, `pwmLimit()`; thresholds in `config::` |
| **Unit tests** | `tests/test_battery.cpp` |
| **Monitors** | None yet. They arrive with diagnostics in increment 2 |

Test this unit on its own with `python scripts/dev.py test battery`.
