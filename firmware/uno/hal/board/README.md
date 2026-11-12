# board

**Layer:** hal. **Version:** in `library.properties`.

The only code that touches the UNO's pins: motor driver pins, the battery divider and the MODE button. Pin numbers come from [the pin map](../../../../docs/hardware/pin-map.md).

| | |
|---|---|
| **Requirements** | S4 (motors off through a reset, with the driver's pull-downs), S5 (driver in standby at power-on) |
| **Interface** | `init()`, `setMotors(enabled, left, right)`, `readBatteryAdc()`, `modeButtonPressed()` |
| **Unit tests** | None: it needs the real board. Covered by the robot tests. |
| **Monitors** | None yet. They arrive with diagnostics in increment 2 |

Test this unit on its own with `python scripts/dev.py test board`.
