# 0011: A UNO R4 WiFi as the computer's Wi-Fi dongle

**Date:** 2026-11-16. **Status:** accepted.

## Context

The robot's USB cable is 20 cm long, so every test so far has run with the tracks lifted. Heading hold (#26), the ultrasonic retest (#30) and, later, the modes all need the robot on the floor. Driving over Wi-Fi was planned for increment 4, through the stock camera board's bridge on port 100.

Joining the robot's network from the computer would work, but the computer then leaves its own Wi-Fi and loses the internet, which an agent running the tests needs. The maintainer owns a UNO R4 WiFi that isn't used for anything.

The maintainer agreed this approach on 2026-11-15, recorded in #34.

## Decision

- **The R4 is a USB-to-Wi-Fi dongle on the computer's side.** It joins the robot's open network, opens port 100 and passes messages both ways. `conq` talks to it like any serial port, and the computer stays on its own Wi-Fi.
- **It's a tool, not a change to the robot,** so [0001](0001-stock-hardware-first.md) still holds: the robot keeps its stock hardware and stock camera firmware.
- **This brings part of increment 4 forward,** the Wi-Fi link only. The rest of increment 4 keeps its place in the [build order](../requirements.md#build-order).
- **The dongle has its own firmware,** in `firmware/dongle/`, built from units like the UNO's and built, tested and flashed by `dev.py` with `--target dongle`.
- **It's temporary.** In stage 3 our own camera firmware joins the home Wi-Fi, and we delete the dongle firmware.
- **The R4 may come back as the robot's own board** in stage 5, in place of the UNO, as the [roadmap](../roadmap.md) expects. That would be a hardware change, decided under 0001 on measured limits, and its own Wi-Fi would also make a dongle unnecessary.

## Consequences

- Floor tests can start in increment 2, and the robot tests run unchanged over USB or Wi-Fi.
- The link gains a hop and a slower path through the R4's own Wi-Fi chip. We measure its latency and dropouts before relying on it, and S3's 300 ms timeout still stops the robot if messages stop.
- The project now builds two firmwares, so CI builds both and the unit test harness covers both.
