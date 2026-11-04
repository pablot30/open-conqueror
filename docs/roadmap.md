# Roadmap

**Current stage: 0.** We finish a stage before starting the next.

| Stage | Work | Done when |
|---|---|---|
| 0. Organize | Vision, roadmap, feature list, hardware notes, public repo | The docs are agreed and public |
| 1. Know the robot | Back up the factory firmware, check the sensors and motors one at a time, record how each mode behaves, answer the [open hardware questions](hardware/README.md#open-questions) | We can return to stock and have a reference to compare against |
| 2. Own the UNO | Our UNO firmware and protocol, plus a computer tool that controls it through the stock camera firmware | Every UNO item on the [feature list](features.md) is ticked on the robot |
| 3. Own the camera board | Our ESP32-S3 firmware, with a Wi-Fi password and a web control page | No ELEGOO code runs on the robot and the whole feature list is ticked |
| 4. Stretch the stock hardware | Smoother line following, better heading hold, ultrasonic sweeps, vision and language commands running on a computer over Wi-Fi | Each improvement is measured against stage 1 |
| 5. Upgrade on evidence | Hardware changes where stage 4 hit a measured limit, probably starting with a UNO R4 WiFi and encoders | Decided when we get there |

Backups of the factory firmware stay private and never go in this repo.

## Stage 1 order

Watching the modes teaches us little about the hardware, so they come last. Everything here runs on the stock firmware.

1. **Identify and back up.** Confirm the USB chip and back up both firmwares, with the motors off.
2. **Read each sensor on its own,** using the [stock protocol](hardware/stock-protocol.md) from a computer.
3. **Test the motors and electrics.** With the tracks off the ground, check which channel drives which track. Then measure the voltage on GPIO3.
4. **Record each mode.** A short video of each gives us the reference to compare against later.

Looking at noise on the 5 V rail and I2C bus needs an oscilloscope, which we don't have yet. That measurement doesn't hold up stage 2, where a test sketch can compare I2C errors and gyro noise with the motors on and off.

## Stage 2 order

The firmware is built from the bottom up, one driver at a time and tested on the robot, with the modes last.

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
| Own hardware | A custom board replacing the shield | What the other tracks teach us |

## How we work

- GitHub Issues hold the backlog, with one milestone per stage.
- Work happens on short branches with pull requests. The maintainer reviews and merges.
- The AI agent writes code and docs and tests what it can on a computer. The maintainer handles the physical steps and signs off on the robot.
- Decisions go in [decisions](decisions/README.md), and lessons go in the [notebook](notebook/README.md).
