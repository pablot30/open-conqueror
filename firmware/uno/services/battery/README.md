# battery

**Layer:** services. **Version:** in `library.properties`.

Filters the battery voltage, sets the low and critical levels, and limits motor PWM so the motors' average voltage stays at 6 V or below.

| | |
|---|---|
| **Requirements** | S9 (battery warning and cutoff), S10 (motor voltage limit) |
| **Interface** | `BatteryMonitor`: `sample(adc)`, `millivolts()`, `level()`, `pwmLimit()`; `batteryReadingPlausible(adc)`; thresholds in `config::` |
| **Unit tests** | `tests/test_battery.cpp` |
| **Monitors** | `battery sensor` in diagnostics: each raw reading is plausible, between 1 V and 10 V at Vin (`batteryReadingPlausible()`). 5 bad samples confirm it, and the robot won't drive |

Test this unit on its own with `python scripts/dev.py test battery`.
