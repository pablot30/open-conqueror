# Roadmap

**Current stage: 2, increment 2.** Stages 0 and 1 are done. Increment 1 passed its robot tests on 2026-11-12 ([test report](test-reports/2026-11-12-increment-1/README.md)). The [architecture](architecture.md) is agreed ([0008](decisions/0008-architecture.md)) and the repo matches it. We finish a stage before starting the next.

| Stage | Work | Done when |
|---|---|---|
| 0. Organize | Vision, roadmap, requirements, hardware notes | The docs are agreed |
| 1. Know the robot | Back up the factory firmware, check the sensors and motors one at a time, answer the [open hardware questions](hardware/README.md#open-questions) | Both boards are backed up and verified, and we know the hardware well enough to write our own firmware |
| 2. Own the UNO | Our UNO firmware and protocol, plus a computer tool to control it, built in small increments that each run on the robot ([0006](decisions/0006-walking-skeleton-first.md)). The repo went public during this stage, once the maintainer was happy with its structure ([0005](decisions/0005-publish-with-first-working-firmware.md)) | Every stage 2 requirement in the [requirements](requirements.md) is checked on the robot, and release 0.1.0 has passed its release candidates ([0007](decisions/0007-testing-releases-and-traceability.md)) |
| 3. Own the camera board | Our ESP32-S3 firmware, with a Wi-Fi password and a web control page | No ELEGOO code runs on the robot, and every requirement is checked |
| 4. Stretch the stock hardware | Smoother line following, better heading hold, ultrasonic sweeps, vision and language commands running on a computer over Wi-Fi | Each improvement is measured against the stock firmware, restored from the backup and measured the same way |
| 5. Upgrade on evidence | Hardware changes where stage 4 hit a measured limit, probably starting with a UNO R4 WiFi and encoders | Decided when we get there |

Backups of the factory firmware stay private and never go in this repo.

## Stage 1 order

Everything here runs on the stock firmware.

1. **Identify and back up.** Confirm the USB chip and back up both firmwares, with the motors off.
2. **Read each sensor on its own,** using the [stock protocol](hardware/stock-protocol.md) from a computer.
3. **Test the motors and electrics.** With the tracks off the ground, check which channel drives which track.

We don't record the stock modes as a reference here. A phone video can't be compared fairly with measurements we'll make later, and the backups let us restore the stock firmware whenever we need to measure it. We also don't test the restore on its own: the first time we flash our own firmware in stage 2, we restore the stock firmware at the end of that session, which tests it.

Some measurements wait without holding up stage 2: the voltage on GPIO3, the motors' ratings, and noise on the 5 V rail and I2C bus, which needs an oscilloscope. A stage 2 test sketch can compare I2C errors and gyro noise with the motors on and off.

## Stage 2 order

The firmware grows in the increments listed in the [build order](requirements.md#build-order), starting with a walking skeleton: driving and stopping over USB, with the safety basics. Each increment is tested on the robot before the next starts, and ends with a test report rather than a release. The modes come last. Tools and process join an increment only when it needs them. Stage 2 ends with release candidates, used on the robot over several sessions, and then the first release, 0.1.0 ([0007](decisions/0007-testing-releases-and-traceability.md)).

## Longer-term tracks

These have no committed order. We'll choose based on what the earlier stages teach us.

| Track | What it means | Likely needs |
|---|---|---|
| Motor control | Closed-loop speed and accurate turns | Encoders |
| Sensor fusion | Combining gyro, encoders, distance and camera into a position estimate | Encoders, an overhead camera for ground truth |
| Self-driving | Mapping, planning and navigation | More sensors, probably an onboard computer |
| Small language models | Natural-language commands mapped to safe robot actions | Can start on a laptop |
| Automated development | An agent builds, flashes, tests and judges results on the real robot, unattended | Flashing without the Upload-Cam switch, a test stand, an overhead camera |
| Model Hardware Standard | Exposing the robot through Anthropic's [standard for agents operating devices](https://www.anthropic.com/news/model-hardware-standard-research-preview) | A public spec, which didn't exist when this was written |
| Health monitoring | Diagnostics that find faults, and prognostics that warn before a part fails, such as a battery losing capacity or a motor wearing out | Logged history across sessions. Battery sag under load is measurable today; motor wear needs current sensing or encoders |
| Behaviours and missions | Modes that combine and adapt, and missions the user designs, such as "search the room for the red ball" | The modes from stage 2, a way to describe missions, a simulator |
| Several robots | Robots of different kinds working together, such as a drone and the tank on one mission | A shared protocol, coordinates and clock, and a second robot |
| Own hardware | A custom board replacing the shield | What the other tracks teach us |

## How we work

- GitHub Issues hold the backlog, with one milestone per stage.
- Work happens on short branches with pull requests. The other vendor's model reviews each one, the author answers every comment, and the maintainer reads the review summary and open points before merging ([decision 0013](decisions/0013-review-by-the-other-vendor.md)).
- The AI agent writes code and docs and tests what it can on a computer. The maintainer handles the physical steps and signs off on the robot.
- Decisions go in [decisions](decisions/README.md), and lessons go in the [notebook](notebook/README.md).
