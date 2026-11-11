# 0008: The architecture

**Date:** 2026-11-12. **Status:** accepted.

## Context

The walking skeleton didn't need an architecture, but increment 2 adds five devices at once, and the vision now reaches to combined modes, health monitoring and several robots working together. We discussed the structure one question at a time on 2026-11-11 and 2026-11-12. The details are in [architecture.md](../architecture.md). This record keeps the decisions and why we made them.

## Decision

**A1. Roles, not machines.** Each robot has a **guard** for real-time control and safety (the UNO), a **gateway** for Wi-Fi, the camera and messages (the camera board), and a **brain** for missions, maps and vision (a computer for now; it can move onto a robot later).

- Robots talk to each other directly, as well as to brains.
- Every robot has an ID, and every message says who sent it and who it's for.
- Only a robot's own guard decides what its motors do. Everyone else sends requests, and missions are given as goals.
- Logs are kept where there's room and sent on when there's a link.

**A2. The UNO firmware has four layers:** modes, services, devices and `hal/`.

- One safety supervisor is the only code that sets the motors.
- A fixed 10 ms loop, with no RTOS on the guard until a bigger microcontroller has work that can't be split into short steps.
- Behaviours combine on the robot by priority (subsumption). Missions and larger combinations run in the brain with behaviour trees.

**A3. The host side** is a Python package in `host/` with the `conq` command.

- Every command works without a prompt and can give JSON output.
- One command takes a new robot from stock to playing.
- A simulator and a hub arrive when they're needed.
- Our text protocol stays inside the robot. MAVLink is evaluated for the robot-to-world link in stage 3.

**A4. Self-contained units.** Each unit has its own folder with a README, a version, `src/` and `tests/`, laid out like an Arduino library.

- Robot tests live in `tests/robot/`.
- Per-robot folders wait for a second robot type.
- Support tiers: macOS is tier 1, and Linux and Windows are tier 2.

**A5. Health uses standard terms.** Units run **monitors** that report passed or failed. A **diagnostics** service debounces them into diagnostic events (not tested, prefailed, failed, prepassed, passed) and applies reactions from one table. Every sensor value carries a status: valid, init, substitute, error or not available. Fault memory and fault codes come later. The first iteration is part of increment 2.

## Why

- **Roles and IDs** keep several robots and other robot types possible without a rewrite, and they cost almost nothing now.
- **A single owner for the motors** makes "fail to a stop" structural. A stop can't be bypassed by a mode, a mission or another robot.
- **No RTOS** keeps the guard predictable and testable in 2 KB of RAM. Each task already has a time budget, so moving to an RTOS later stays cheap.
- **Self-contained units** can be understood, tested, versioned and owned on their own, by people or agents. Pigweed and Zephyr organise embedded code the same way.
- **Standard names** for health make the design familiar to people from automotive, aerospace and industrial automation, and to agents. Every concept in A5 comes from a public standard, cited in the architecture.
- **Where we chose another field's practice over the automotive habit:**
  - subsumption and behaviour trees from robotics, instead of large state machines;
  - the cyclic executive from aerospace, instead of an RTOS;
  - signal quality from industrial automation;
  - layering in AUTOSAR's spirit, without its tooling.

## Consequences

- Before increment 2, the repo is restructured to match: the firmware moves into units, `host/` becomes a package with `conq`, and robot tests move into the repo. This adds no new features.
- Increment 2 builds its devices as units, each with monitors and signal status from the start.
- What waits, and what brings it back, is listed at the end of [architecture.md](../architecture.md).
