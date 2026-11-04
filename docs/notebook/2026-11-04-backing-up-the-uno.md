# Backing up the UNO

**Date:** 2026-11-04. **How we know:** measured on our robot over USB, with the Upload-Cam switch in Upload and main power off.

## What we found

| Item | Result |
|---|---|
| USB-serial chip | USB ID `1A86:7523`, which is WCH's CH340 family |
| MCU signature | `1E 95 0F`. That's the ATmega328P's, though avrdude notes that some other chips share it |
| Bootloader | Optiboot 4.4 at 115200 baud, 512 bytes at `0x7E00`, identical to the copy in Arduino's `arduino:avr` 1.8.8 core |
| Application | Ends at about 31.9 KB, and its text strings match the stock firmware. We didn't compare it byte for byte with a build from source |

## How to take a backup

With avrdude from the `arduino:avr` core, the same on macOS, Linux and Windows apart from the port name:

```
avrdude -n -A -p atmega328p -c arduino -P <port> -b 115200 -U flash:r:flash.hex:i
avrdude -n -A -p atmega328p -c arduino -P <port> -b 115200 -U flash:v:flash.hex:i
```

`-n` stops avrdude writing to the board, and `-A` keeps the trailing `0xFF` bytes so that the image is complete. Depending on your install, you may also need `-C` to point at avrdude's config file. The second command checks the saved file against the board.

Opening the port resets the UNO, so the stock firmware restarts afterwards.

Keep the backup private. The kit comes with no license to redistribute its firmware.

## The lesson: a clean read can still be wrong

We also asked for the EEPROM. avrdude reported success, and two reads matched, but the file began with `0C 94`, the jump instructions of an AVR vector table. It turned out to be the first 1,024 bytes of flash, because this bootloader can't read EEPROM and returns flash without any error.

Matching reads only prove that a read is repeatable, so check the contents as well. We deleted that file.

**So the backup doesn't include EEPROM or fuses.** Reading them needs a sketch, which would overwrite the application, or an ISP programmer. The stock sketch's own source files never mention EEPROM, so we don't expect to need it, but we didn't check its libraries.

## Not tested yet

We haven't restored this backup. We can't call stage 1's "we can return to stock" done until we have, and the first time we flash anything will show whether it works.
