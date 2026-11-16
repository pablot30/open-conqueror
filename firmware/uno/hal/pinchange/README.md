# pinchange

**Layer:** hal. **Version:** in `library.properties`. **First run on the robot on 2026-11-15,** under the ultrasonic and IR remote tests.

The one place that defines the pin-change interrupt for port B (the `PCINT0` vector), covering D8 to D13. The ultrasonic echo (D12) and the IR receiver (D9) are both on port B, and an interrupt vector can only be defined once, so each unit registers a handler here instead of defining its own. Pin numbers come from [the pin map](../../../../docs/hardware/pin-map.md).

| | |
|---|---|
| **Requirements** | T2 (nothing waits for a sensor), T3 and F10 (they need timed edges) |
| **Interface** | `pinchange::attach(pin, handler)`, where `handler(high, atUs)` gets the new level and `micros()` at the edge |
| **Unit tests** | None: it needs the real board. Covered by the robot tests `ultrasonic_ruler` and `ir_keys` |
| **Monitors** | None. The units that use it monitor their own signals |

Handlers run inside the interrupt, so they must be a few instructions long. The vector compares `PINB` with its previous value to find which pins changed, so two pins changing together are both handled. It takes no timer. Register addresses are from the ATmega328P datasheet ("External interrupts").

Test this unit on its own with `python scripts/dev.py test pinchange`.
