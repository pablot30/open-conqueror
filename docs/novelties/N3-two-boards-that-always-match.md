# N3: A two-board update that always ends as a matched pair

**Status:** idea. **First written:** 2026-11-08, in research report R3 ([security and wireless updates](../research/2026-11-08-security-and-wireless-updates.md)). Research report R7's release flow builds on it. **Published here:** 2026-11-16.

## The problem

A release updates two chips: the camera board's ESP32-S3 and the UNO. They must match, or their messages may not fit together. The ESP32 has two firmware slots and can roll back; the UNO has one slot and can't. A truly atomic update across both isn't possible.

## The idea

Use the ESP32's own rollback window as the transaction:

1. The new ESP32 firmware starts unconfirmed, as ESP-IDF allows.
2. While still unconfirmed, it flashes the UNO and asks for the UNO's version.
3. Only when the UNO reports the expected version does the ESP32 confirm itself.
4. On any failure, crash or power cut before that, the ESP32 restores the previous UNO image, which it kept, and falls back to its own previous firmware.

The robot always ends up with a matched pair: both new or both old.

## Building blocks, all public

- ESP-IDF's rollback states: a new app stays pending until it confirms itself, and boots the previous app otherwise ([ESP-IDF OTA](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/system/ota.html)).
- A published ESP32 plus STM32 updater, which its author calls "not fully atomic" ([Scope Creep Labs](https://scopecreeplabs-site.pages.dev/blog/stm32-esp32-ota)).

## What we searched

Public open-source projects and documentation, on 2026-11-08. We found no library or design that coordinates an ESP32 and a single-slot AVR this way. Patent databases weren't searched.
