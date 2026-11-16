# open-conqueror

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development, with everything we learn published along the way.

> **Status: stage 2, increment 2.** Our UNO firmware drives the robot from a computer or the IR remote, stops safely, and reads the gyro, ultrasonic and battery. Heading hold for straight runs is built and waits for its floor test. Next: floor tests over Wi-Fi, then the modes. See the [roadmap](docs/roadmap.md) and the [firmware](firmware/README.md).

## What's new here

Ideas from this project that we haven't found anywhere else, published openly so anyone can use them: wireless flashing with no reset wire, updates that survive a power cut, two boards that always match, and more. See the [novelties](docs/novelties/README.md).

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
| [Requirements](docs/requirements.md) | What the first goal must do, including safety, and how we check it |
| [Architecture](docs/architecture.md) | How the software is split across machines, and how each part is built |
| [Hardware](docs/hardware/README.md) | Parts, pin map, circuits, power, camera board and stock protocol |
| [Firmware](firmware/README.md) | How to build, test and flash, and the protocol |
| [Tools](docs/tools.md) | What to install, and how to update and remove it |
| [Novelties](docs/novelties/README.md) | Ideas from this project we haven't found anywhere else |
| [Research](docs/research/README.md) | Reports written before big decisions |
| [Test reports](docs/test-reports/README.md) | What each increment was tested against on the robot, with the logs |
| [Decisions](docs/decisions/README.md) | Short records of why we chose things |
| [Notebook](docs/notebook/README.md) | What we learn along the way |
| [Playbook](docs/playbook.md) | How we run the project, step by step, as it grows |
| [Journey](docs/journey.md) | The project's story, milestone by milestone |

## Supported hardware

We target the Conqueror with the TB6612 motor driver shield and the ESP32-S3 camera board. ELEGOO also shipped DRV8835 shields and ESP32-WROVER camera boards, which this project doesn't cover yet.

## Not affiliated with ELEGOO

This project is not affiliated with or endorsed by ELEGOO. "ELEGOO" and "Conqueror" are used only to describe the hardware. We don't redistribute ELEGOO's code, manuals or firmware. You can get those from [ELEGOO](https://www.elegoo.com/download/).

## License

Apache-2.0. See [LICENSE](LICENSE).
