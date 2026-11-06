# open-conqueror

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development, with everything we learn published along the way.

> **Status: stage 0, organizing.** There is no firmware here yet, so nothing in this repo runs on a robot. See the [roadmap](docs/roadmap.md).

## What this is

The ELEGOO Conqueror is an Arduino-based robot tank kit with tracks, line sensors, an ultrasonic sensor, a gyro and a Wi-Fi camera. This project replaces its software while leaving the hardware as it is:

1. Make the robot do everything it does out of the box, including video and phone control, running only our code.
2. Get as much as possible out of the stock hardware.
3. Upgrade hardware only where we can measure a real limit.

The software is built through AI-driven development, using [Claude Code](https://claude.com/claude-code). The longer-term question is how much of hardware development can run automatically, from writing code to testing it on a real robot.

## Documentation

| Document | Contents |
|---|---|
| [Vision](docs/vision.md) | The idea, goals, principles and what we're not doing yet |
| [Roadmap](docs/roadmap.md) | Stages, exit criteria, longer-term tracks and how we work |
| [Features](docs/features.md) | The checklist for the first goal |
| [Hardware](docs/hardware/README.md) | Parts, pin map, circuits, power, camera board and stock protocol |
| [Tools](docs/tools.md) | What to install, and how to update and remove it |
| [Decisions](docs/decisions/README.md) | Short records of why we chose things |
| [Notebook](docs/notebook/README.md) | What we learn along the way |

## Supported hardware

We target the Conqueror with the TB6612 motor driver shield and the ESP32-S3 camera board. ELEGOO also shipped DRV8835 shields and ESP32-WROVER camera boards, which this project doesn't cover yet.

## Not affiliated with ELEGOO

This project is not affiliated with or endorsed by ELEGOO. "ELEGOO" and "Conqueror" are used only to describe the hardware. We don't redistribute ELEGOO's code, manuals or firmware. You can get those from [ELEGOO](https://www.elegoo.com/download/).

## License

Apache-2.0. See [LICENSE](LICENSE).
