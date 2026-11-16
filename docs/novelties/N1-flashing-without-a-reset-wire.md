# N1: Flashing the UNO over Wi-Fi with no reset wire

**Status:** idea. **First written:** 2026-11-08, in research report R3 ([security and wireless updates](../research/2026-11-08-security-and-wireless-updates.md)). **Published here:** 2026-11-16.

## The problem

The robot's camera board, an ESP32-S3, shares the UNO's serial line, so it could program the UNO over Wi-Fi. But the UNO's stock bootloader (Optiboot 4.4) only runs after an external reset, and no wire on the robot lets the ESP32 reset the UNO. Projects that flash an AVR over Wi-Fi solve this with a reset wire, which here would be a hardware change.

## The idea

Let the application ask for the bootloader:

1. The ESP32 sends the UNO firmware an authenticated "prepare for update" command.
2. The firmware makes the robot safe, motors off, and restarts itself through a real watchdog reset, so every pin returns to its reset state.
3. The bootloader is one built to accept a request from the application, and stays in programming mode instead of starting the app.
4. The ESP32 programs the UNO over the shared serial line.

The missing wire turns out to be a bootloader policy, not hardware: a 512-byte bootloader that also trusts the application removes the need for it. It takes one session with an ISP programmer to replace the bootloader. Nothing is soldered and no fuse changes.

## Building blocks, all public

- Optiboot 8.3 starts the bootloader on an application request ([optiboot.c](https://raw.githubusercontent.com/Optiboot/optiboot/master/optiboot/bootloaders/optiboot/optiboot.c)).
- QMK keyboard firmware enters its bootloader with a magic value, a forced watchdog reset and an early-startup jump ([QMK bootloader.c](https://git.poz.pet/poz/qmk_firmware/raw/commit/fca31693df15f62cc3fd5851a0d1bae40eb179c2/tmk_core/common/avr/bootloader.c)).
- esp-link flashes AVRs over Wi-Fi, using a reset wire ([esp-link](https://github.com/jeelabs/esp-link/blob/master/UC-FLASHING.md)).

## What we searched

Public open-source projects and documentation, on 2026-11-08. We found no project that combines these so a Wi-Fi co-processor flashes an AVR with no reset line. Patent databases weren't searched.
