# led

**Layer:** devices. **Version:** in `library.properties`. **Not run on the robot yet.**

The status LED, one WS2812 on D4: violet breathing in standby, steady violet while driving, red flashing on a low battery (1 Hz) and faster on a critical one (4 Hz). At most 64 of 255 per colour.

| | |
|---|---|
| **Requirements** | F5 (standby breathing violet), F15 (low battery flashing red) |
| **Interface** | `ledColor(mode, nowMs)`; `led_hw::begin()`, `led_hw::show(color)` in `led_hw.cpp` |
| **Unit tests** | `tests/test_led.cpp`, for the logic. Files ending in `_hw.cpp` use the hardware and only build for the board |
| **Monitors** | None: the LED gives no feedback. |

`led_hw.cpp` is our own driver for a 16 MHz UNO: common libraries allocate memory at run time, which T8 rules out. Interrupts are off for about 30 us per update, and updates are sent only when the colour changes.

Test this unit on its own with `python scripts/dev.py test led`.
