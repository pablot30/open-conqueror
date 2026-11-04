# 0001: Stock hardware first

**Date:** 2026-11-03. **Status:** accepted.

## Context

The long-term goals eventually need more than the stock robot offers: it has no encoders, every UNO pin is in use, and flashing needs a person to flip a switch. Swapping parts early would remove those limits, but it would also widen the scope before we understand the robot.

## Decision

Build our own software on the unchanged robot first, and change hardware only once we've measured a limit we can't get past.

## Consequences

- The first goal has a clear finish line: the robot does what it did, on our code.
- Firmware has to fit an ATmega328P, with 32 KB flash and 2 KB RAM.
- A person flips the Upload-Cam switch for every flash, so automated flashing waits for a later stage.
