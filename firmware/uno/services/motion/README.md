# motion

**Layer:** services. **Version:** in `library.properties`. **Not run on the robot yet.**

Heading hold. While both tracks are commanded equal, at 15% or faster, the robot should drive straight, so this holds the heading it had when the run began. A small PI controller on the gyro's heading asks for a correction: a few percent added to one track and taken from the other. Speed holding and the modes come later.

| | |
|---|---|
| **Requirements** | F1 (hold a straight line with the gyro), T4 (correct drift down to 0.5 degrees a second), A5 (use a signal only while its status allows it) |
| **Interface** | `HeadingHold::update(HoldInput)` returns a `HoldOutput` (correction in tenths of a percent, positive turns left, and a `HoldState`); `headingError(to, from)`. The constants are in `config::` in `motion.h` |
| **Unit tests** | `tests/test_motion.cpp`, with a simple simulated robot that turns at a rate set by its track difference plus a steady drift |
| **Monitors** | None of its own. It reacts to the gyro's: see below |
| **Robot tests** | `tests/robot/heading_hold.py` (floor, 2 m, over the robot's Wi-Fi or a long USB cable) |

## What it does with the gyro's status

| Gyro signal | Heading hold |
|---|---|
| **valid** | Holds the heading captured at the start of the run |
| **substitute** | Keeps the last correction for up to 200 ms (the heading integrates the last rate), then drives without correction |
| **init, error, not available** | Drives without correction, and drops the captured heading. When the gyro is valid again it holds the heading it has *then* |
| `gyro comms` event confirmed (reaction `kInhibitHeadingHold`) | Treated as *error* until the event heals, even while the signal reads substitute |

The state goes out in the sensor message: 0 off (not a straight run), 1 holding, 2 substitute, 3 no gyro. Driving is never stopped because of the gyro.

## How it works

- **Straight run.** Both tracks equal and at least 15% either way. Anything else (a turn, an arc, a stop) clears the controller. Changing speed on a straight run keeps the hold; going from forward to backward starts a new one.
- **Controller.** The error is the shortest way round from the target to the heading. The output is `Kp × error + Ki × ∫error`, limited to ±5%, with no wind-up: the integral stops growing while the output is at its limit. `Kp` is 1% for each degree and `Ki` 1% for each degree held for a second. **These two numbers are first guesses** and the floor test is where they get tuned; so are the 15% minimum and the 5% limit.
- **Sign.** A positive heading is counterclockwise, as in the gyro unit. Drifting left (heading rises) gives a negative correction, which slows the right track and speeds up the left. A robot turns the same way for a given track difference whether it drives forward or backward.
- **The correction is a trim, not a command.** `SafetySupervisor::setTrim` applies it to the motors only while driving, limits it to 6% itself, and doesn't count it as a command: it can't keep the link timeout from firing (S3), arm the supervisor or restart a stopped robot (S1, S5). The supervisor remains the only code that sets the motors.

Test this unit on its own with `python scripts/dev.py test motion`.
