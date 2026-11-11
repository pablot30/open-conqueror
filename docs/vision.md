# Vision

## The idea

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development, with everything we learn published along the way.

## Who it's for

People who like to play with electronics, learn about AI and think about whole systems, not just one function at a time.

## Why

Physical AI is moving quickly, and an affordable robot kit is a good place to learn in the open. AI agents are already strong at pure software. Hardware is harder: code has to be flashed, the robot has to move, and the result has to be measured in the real world. We want to find out what works and publish it.

The tank is the first target, not the final product. What we learn should apply to other hardware like it.

## Goals

**First goal: our software on stock hardware.** The robot meets the [requirements](requirements.md), including video and phone control through our own web page, and no ELEGOO code runs on it.

**Next: get the most out of the stock hardware.** Smoother line following, better heading hold, ultrasonic sweeps, and vision and language commands running on a computer over Wi-Fi.

**Then: make it yours.** Change the hardware where we've measured a limit the stock parts can't get past, and make the robot easy to modify.

**Beyond: an ecosystem where the limit is your imagination.** Modes that combine and adapt. A robot that knows its own health and warns you before a motor or battery fails. Several robots of different kinds working together on missions you design, such as a drone and the tank searching the house for something.

**End goal: fully automated software development for hardware like this.** Building, flashing, testing on the real robot and judging the results, without a person needed at each step.

## Principles

**Fun to play, safe to fail, simple to grow, open to people and agents.**

When these conflict, the order is: safe, then working on the robot, then simple, then fun, then complete.

### Fun to play

- **Every increment ends with something you can play with,** not only a passing test.
- **Plug and play.** From a new robot to playing in minutes: one guided setup backs up the stock firmware, flashes ours, sets up Wi-Fi and checks that everything works. The same for people and agents, with as little friction as possible.

### Safe to fail

- **Fail to a stop.** When in doubt, or when communication is lost, the motors stop, and they stay stopped until someone deliberately drives again.
- **Know the robot's health.** Diagnosing a part is as much our job as driving it.
- **Measure before changing, and say how you know.** Record how something behaves before trying to improve it.

### Simple to grow

- **Simple first.** Add tools, layers and parts only when we need them, and keep a "later" list with what would bring each one back.
- **Clean seams.** Logic stays separate from hardware, and interfaces are versioned, so the project can grow without rewrites.
- **Professional where it pays:** safety, tests, traceability and reproducible builds. Not certification paperwork.
- **Stock hardware first, then upgrade on evidence.** Replace software before replacing parts.
- **Follow the documented behaviour.** The target is what the manual describes. We fix stock bugs rather than reproduce them.
- **Borrow from every field.** Use the best practice that fits, whether it comes from automotive, robotics, aerospace, web or security, and check it against sources rather than habit.

### Open to people and agents

- **Every tool works for both:** commands that run without a prompt, readable output for people and JSON for agents.
- **Work in the open.** Decisions and lessons go in this repo, including what failed.
- **Clean implementation.** Nothing from ELEGOO's kit is copied here. Facts go in our own words.
- **Any computer, kept clean.** Everything works on macOS, Linux and Windows, and installs only what it needs.

## Not now

New hardware, an RTOS, ROS 2, AI features on the robot and automated flashing. Some of these may come later; see the [roadmap](roadmap.md).
