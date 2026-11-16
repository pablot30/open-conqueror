# N2: A write order that means a half-written UNO image never runs

**Status:** idea. **First written:** 2026-11-08, in research report R3 ([security and wireless updates](../research/2026-11-08-security-and-wireless-updates.md)). **Published here:** 2026-11-16.

## The problem

The UNO has one firmware slot. If the power fails while a new image is being written, the chip holds part new, part old code. The next start runs it, with the motors and everything else in an unknown state.

## The idea

Write the pages in an order that keeps the application from ever starting until the image is whole:

1. First write a tiny placeholder into the application's first page. Whatever starts the application, the placeholder sends it straight back to the bootloader.
2. Write and verify every other page.
3. Write the real first page last.

A reset at any point lands back in the bootloader, with all outputs off and the serial line still open for another try. Two short windows remain, each about one page write long (an estimate), while the first page itself is being rewritten.

## Building blocks, all public

- Optiboot writes flash page by page ([optiboot.c](https://raw.githubusercontent.com/Optiboot/optiboot/master/optiboot/bootloaders/optiboot/optiboot.c)).
- On the UNO the reset vector points at the bootloader ([boards.txt](https://raw.githubusercontent.com/arduino/ArduinoCore-avr/master/boards.txt)).
- urboot has an option that protects the reset vector ([urboot](https://github.com/stefanrueger/urboot)).

## What we searched

Public open-source projects and documentation, on 2026-11-08. We found no published power-fail scheme for Optiboot updates, and no measurement of how Optiboot behaves when power fails. Patent databases weren't searched.
