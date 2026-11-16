# Security

## Reporting a problem

Please report a vulnerability privately, through GitHub's **Report a vulnerability** button on this repo's Security tab, not in a public issue. Say what's affected, how to reproduce it, and what it could do. We'll answer there.

This applies to anything that could let someone else control a robot, read data from it or from the computer running our tools, or make a robot move when it shouldn't. A robot that moves unexpectedly is a safety problem even with no attacker, so report that too.

## Known gaps

- **The stock camera firmware runs an open Wi-Fi network.** Anyone in range can join it and drive the robot ([camera board](docs/hardware/camera-board.md)). We keep the stock camera firmware until stage 3, when our own adds a password (requirement S11).
- **Our firmware has no authentication on its serial protocol.** Whatever is connected over USB, or through the camera board's bridge, can send commands. The safety supervisor still limits what a command can do: speed limits, the link timeout, and stops that hold.

This is a hobby project run by one maintainer, so there are no fixed response times.
