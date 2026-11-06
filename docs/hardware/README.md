# Hardware

Facts about the stock robot, in our own words. They come from ELEGOO's schematics, manual and firmware source, which are part of the kit download. Unless a page says otherwise, **nothing here has been measured on a robot yet**.

## Parts

| Part | Details |
|---|---|
| Controller | UNO R3-compatible board: ATmega328P at 16 MHz, 32 KB flash, 2 KB RAM. CH340-family USB-serial chip and Optiboot 4.4 bootloader ([measured on our robot](../notebook/2026-11-04-backing-up-the-uno.md)) |
| Motor shield | "Conqueror Car V1.1" with a TB6612FNG driver |
| Camera board | ESP32-S3-WROOM-1 with 8 MB flash and 8 MB PSRAM ([measured](../notebook/2026-11-06-backing-up-the-camera-board.md)), Wi-Fi, USB-C and an OV3660 camera on a DVP connector (sensor reported by the maintainer; a retailer guide lists an OV2640, so units may differ) |
| Drive | Two yellow TT-style gearmotors with a 1:48 gearbox (ratio from retailer listings), tracks, eight road wheels on sprung rocker arms, skid steering, no encoders |
| Motion sensing | GY-521 module (MPU6050) |
| Distance | Ultrasonic sensor on a pan/tilt mount with two SG90 servos |
| Line sensing | Three-channel ITR20001 module |
| Other | One WS2812 LED, MODE button, IR receiver and remote, Upload-Cam switch |
| Battery | Lithium pack with an on/off switch and USB charging. Retailer listings give 7.4 V nominal, a 7.2 to 8.4 V range and about 2 hours of line tracking, which fits the stock firmware's thresholds and suggests 2S Li-ion. Capacity isn't published |

## Pages

- [Pin map](pin-map.md)
- [Motor driver](motor-driver.md)
- [Power](power.md)
- [Camera board](camera-board.md)
- [Stock protocol](stock-protocol.md)

## Flashing the UNO

The UNO's USB port and the camera board share its only UART, so set the **Upload-Cam** switch to Upload before flashing and back to Cam to use Wi-Fi. The switch is a manual step, so the UNO can't be flashed unattended while it's in Cam.

## Open questions

To answer in stage 1:

- What are the motors' voltage and current ratings? ELEGOO doesn't publish them. See [Motors](motor-driver.md#motors).
- Why does the ultrasonic sensor [read 25 to 35% short](../notebook/2026-11-06-first-sensor-readings.md#the-ultrasonic-reads-short)? We suspect the stock code's timing method, which we can test in stage 2.
- What voltage does the ESP32's GPIO3 really see from the UNO's TX pin? See [5 V on the UART](camera-board.md#5-v-on-the-uart).
- How noisy are the 5 V rail and I2C bus while the motors run? The stock firmware pauses the motors around gyro and ultrasonic readings. This one waits for an oscilloscope, as the [roadmap](../roadmap.md#stage-1-order) explains.
- What is the battery's capacity? This is low priority, and we won't open the case just to find out.
- Do the predictions in the [first look at the stock firmware](../notebook/2026-11-03-first-look-at-stock-firmware.md) hold on the robot?
