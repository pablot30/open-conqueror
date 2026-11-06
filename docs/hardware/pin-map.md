# Pin map

For the TB6612 shield (V1.1). Source: the shield's schematic, cross-checked with the stock firmware. The DRV8835 shield uses different motor pins.

**All 20 UNO I/O pins are in use.** Adding anything means freeing a pin first.

Left and right here mean the robot's own, looking the way it drives. The motor track sides still come from the schematic, so they may turn out to be swapped like the line sensors were.

| Pin | Use | Notes |
|---|---|---|
| D0 | UART RX from camera board | Passes through the Upload-Cam switch |
| D1 | UART TX to camera board | Wired directly in both switch positions |
| D2 | MODE button | 10 kΩ pull-up, external interrupt INT0 |
| D3 | TB6612 STBY | Shared by both motors |
| D4 | WS2812 data | One LED, 100 kΩ pull-down |
| D5 | PWMA, right track | Timer0, about 976 Hz by default |
| D6 | PWMB, left track | Timer0, about 976 Hz by default |
| D7 | AIN1, right track direction | AIN2 is its inverse, through an SN74LVC2G14 |
| D8 | BIN1, left track direction | BIN2 is its inverse, through the same part |
| D9 | IR receiver | Also drives a green activity LED |
| D10 | Pan servo | Header P10 |
| D11 | Tilt servo | Header P1 |
| D12 | Ultrasonic echo | Header P9 |
| D13 | Ultrasonic trigger | Header P9 |
| A0 | Line sensor, robot's left | Header P7. The schematic and firmware call it right ([measured](../notebook/2026-11-06-first-sensor-readings.md#left-and-right-are-swapped)) |
| A1 | Line sensor, middle | Header P7 |
| A2 | Line sensor, robot's right | Header P7. The schematic and firmware call it left |
| A3 | Battery voltage | 10 kΩ and 1.5 kΩ divider, measured after the input diode |
| A4 | I2C SDA | GY-521 |
| A5 | I2C SCL | GY-521 |

The MPU6050's interrupt pin isn't connected, so there's no data-ready signal.

## UART and the Upload-Cam switch

| Switch | USB to UNO | Camera board to UNO | UNO to both |
|---|---|---|---|
| Upload | Works, so flashing and USB commands work | Disconnected | Connected |
| Cam | Unreliable, because the camera board also drives D0 | Works | Connected |

In Cam, a computer on USB can read what the UNO sends but can't see what the camera board sends.
