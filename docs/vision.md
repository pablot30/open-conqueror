# Vision

## The idea

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development, with everything we learn published along the way.

## Why

Physical AI is moving quickly, and an affordable robot kit is a good place to learn in the open. AI agents are already strong at pure software. Hardware is harder: code has to be flashed, the robot has to move, and the result has to be measured in the real world. We want to find out what works and publish it.

The tank is the first target, not the final product. What we learn should apply to other hardware like it.

## Goals

**First goal: our software on stock hardware.** The robot does everything on the [feature list](features.md), including video and phone control through our own web page, and no ELEGOO code runs on it.

**Next: get the most out of the stock hardware.** Smoother line following, better heading hold, ultrasonic sweeps, and vision and language commands running on a computer over Wi-Fi.

**Then: upgrade on evidence.** Add hardware only where we've measured a limit the stock parts can't get past.

**End goal: fully automated software development for hardware like this.** Building, flashing, testing on the real robot and judging the results, without a person needed at each step.

## Principles

- **Simple first.** Add tools, layers and parts only when we need them.
- **Stock hardware first.** Replace software before replacing parts.
- **Follow the documented behaviour.** The target is what the manual describes. We fix stock bugs rather than reproduce them.
- **Measure before changing.** Record how something behaves before trying to improve it.
- **Fail to a stop.** When in doubt, or when communication is lost, the motors stop.
- **Work in the open.** Decisions and lessons go in this repo, including what failed.
- **Clean implementation.** Nothing from ELEGOO's kit is copied here. Facts go in our own words.

## Not now

New hardware, an RTOS, ROS 2, AI features on the robot and automated flashing. Some of these may come later; see the [roadmap](roadmap.md).
