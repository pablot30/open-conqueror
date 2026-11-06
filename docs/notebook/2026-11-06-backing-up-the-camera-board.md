# Backing up the camera board

**Date:** 2026-11-06. **How we know:** measured on our robot's camera board with esptool 5.6.0, with the 4-pin cable unplugged and the board powered over USB-C. The maintainer never flashed this robot, so this is its factory firmware.

## What we found

| Item | Result |
|---|---|
| Chip | ESP32-S3, revision v0.2, 240 MHz dual core |
| Memory | 8 MB of PSRAM in the chip, and 8 MB of quad-SPI flash from GigaDevice |
| USB | The chip's built-in USB-Serial/JTAG, with Espressif's USB ID |
| Partitions | Bootloader, partition table at `0x8000`, NVS, two 3.2 MB app slots, a SPIFFS area and a core dump area |
| Firmware | ELEGOO's camera firmware in the first app slot, built in October 2023 with ESP-IDF 4.4.6. That matches the Arduino-ESP32 2.0.14 that ELEGOO specifies. The second app slot, SPIFFS and core dump areas are empty |

## Getting connected

Two things tripped us up:

1. **A USB-C to USB-C cable doesn't power the board.** A computer's USB-C port only turns its power on when the device announces itself through resistors on the CC pins, and this board doesn't have them. A USB-A to USB-C cable works, because USB-A ports always supply power.
2. **esptool couldn't reset the board into its bootloader by itself.** Holding BOOT, pressing RESET and then releasing BOOT worked every time. After that, every esptool command needs `--before no-reset`.

Unplug the 4-pin cable before connecting USB-C. The USB-C port's 5 V is wired straight to the robot's 5 V rail, so otherwise the computer would power the whole robot.

## How to take a backup

The commands are the same on macOS, Linux and Windows, apart from the port name:

```
esptool --chip esp32s3 --port <port> --before no-reset --after no-reset read-flash 0 ALL flash.bin
esptool --chip esp32s3 --port <port> --before no-reset --after no-reset verify-flash 0 flash.bin
```

A full read of 8 MB takes about 35 seconds. We read the flash twice and compared the two copies, then verified the file against the board. We also checked the contents: a valid bootloader, a partition table, and the firmware's own strings. A clean read on the UNO had turned out to be [the wrong memory](2026-11-04-backing-up-the-uno.md#the-lesson-a-clean-read-can-still-be-wrong).

Keep the backup private. Besides ELEGOO's firmware, it holds the board's MAC address and its stored settings.

## Not tested yet

As with the UNO, we haven't restored this backup.
