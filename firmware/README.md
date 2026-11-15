# Firmware

Our firmware for the robot. Only the UNO has firmware so far, at increment 1, the walking skeleton ([decision 0006](../docs/decisions/0006-walking-skeleton-first.md)). It drives both tracks from a computer and stops when it should. It doesn't use the line sensors yet.

## Layout

The firmware is built from self-contained **units**, grouped in the four layers of the [architecture](../docs/architecture.md) (A2 and A4). Each unit has a README, a version in `library.properties`, its code in `src/` and its unit tests in `tests/`.

| Folder | Contents |
|---|---|
| `uno/uno.ino` | Connects the units. No logic of its own beyond that |
| `uno/sketch.yaml` | The build profile: board, pinned core version, and every unit |
| `uno/services/` | `safety` (the only code that decides what the motors do), `battery`, `link` (the protocol), `scheduler` (the 10 ms frame), `diagnostics` (monitors, events, signal status) |
| `uno/devices/` | `button` (MODE button debouncing), `gyro` (turn rate and heading), `servos` (camera pan and tilt), `led` (the status LED), `ultrasonic` (distance, timed without blocking), `ir` (the remote: decoder, key table, arrow driving) |
| `uno/hal/` | `board`: the motor driver, battery divider and MODE button pins. `pinchange`: the one pin-change interrupt that the ultrasonic echo and the IR receiver share. Device units keep their own hardware code in files ending in `_hw.cpp`, which only build for the board |
| `uno/modes/` | Arrives in increment 3, with the modes |
| `testing/` | The shared unit test harness, built with CMake on the host computer |

`python scripts/dev.py units` lists the units and their versions.

## Build, test and flash

Install the tools in [tools.md](../docs/tools.md) first. From the repo root, on any system:

```
python scripts/dev.py build          # add --json for a report agents can read
python scripts/dev.py test           # all units; or test one: dev.py test safety
python scripts/dev.py upload         # finds the port if only one is connected
```

Use `python3` instead of `python` where that's the name. The build fails if our code has compiler warnings, or breaks the T8 limits: 80% of flash and 512 bytes of free RAM. It reports the firmware version (from the `VERSION` file), the git commit and every unit's version. Flashing needs the Upload-Cam switch on **Upload**.

CI builds the same image on every pull request and keeps it, with the build report, as a download on the run's page.

## Protocol

This is protocol version 6. Messages are short text inside braces, at 9600 baud. That's what the stock camera firmware's bridge forwards, so the same messages should work over Wi-Fi in increment 4. The firmware ignores anything outside braces.

**From the computer:**

| Message | Meaning |
|---|---|
| `{D,left,right}` | Drive. Each track is -100 to 100 percent of the allowed maximum. Resend at least every 300 ms, or the robot stops (S3) |
| `{D,0,0}` | Neutral: brake, and release a held stop. After power-on and after every stop, the robot ignores other drive messages until it gets this one |
| `{S}` | Stop: brake both tracks |
| `{"N":100}` | Stop. The stock bridge sends this when a client disconnects |
| `{V}` | Ask for the version |
| `{P,pan,tilt}` | Point the camera: pan and tilt, each 0 to 180 degrees, 90 is the centre. The camera turns at 100 degrees a second, and the servos are released 0.5 s after they arrive |
| `{P,pan,tilt,speed}` | The same, at `speed` degrees a second, 0 to 999. 0 means as fast as possible (the servos jump). Added in protocol 4 |

**From the robot:**

| Message | Meaning |
|---|---|
| `{V,version,commit,protocol}` | Firmware version, git commit and protocol version, at startup and on request (D4). The commit ends in `-dirty` if the build had uncommitted changes |
| `{T,ms,state,left,right,mV,faults,frame,bstat}` | Telemetry, 10 times a second. `state`: 0 standby, 1 driving, 2 stopped. `left` and `right`: motor PWM, -255 to 255. `mV`: battery at Vin. `faults`: 1 link timeout, 2 battery low, 4 battery critical. `frame`: the longest frame's work since the last message, in µs (T2). `bstat`: the battery reading's signal status: 0 valid, 1 init, 2 substitute, 3 error, 4 not available |
| `{H,skipped,e0,e1,...}` | Health, once a second. `skipped`: frames skipped since power-on. Then each diagnostic event's state, in the order of `EventId` in [diagnostics.h](uno/services/diagnostics/src/diagnostics.h): 0 not tested, 1 prefailed, 2 failed, 3 prepassed, 4 passed. Events today: battery sensor, frame overrun, link timeout, gyro comms, ultrasonic echo |
| `{G,heading,rate,gstat,dist,dstat}` | Sensors, 5 times a second. `heading`: 0 to 35999 hundredths of a degree since power-on, positive counterclockwise seen from above. `rate`: hundredths of a degree per second. `gstat`: the gyro's signal status, *init* until it has calibrated. `dist`: the ultrasonic distance in millimetres, meaningful only when `dstat` is 0 (valid); the last valid distance when 2 (substitute), and 0 otherwise. `dstat`: its signal status. Protocol 3 had the first three fields only |
| `{K,address,command,kind}` | An IR remote event. `kind` 0: a key's code (`address` is the frame's first two bytes, `command` its third), 1: that key's repeat code, sent once per hold, 2: a burst of pulses that couldn't be read (`address` is the pulse count), 3: the held key stopped sending codes and the robot stopped |
| `{E,1}` | A message wasn't understood |

**Link budget.** At 9600 baud the link carries about 960 bytes a second. Worst case, telemetry (about 40 bytes, 10 a second), health and the sensor message (about 25 bytes, 5 a second) use about 53% of it, a little over our 50% aim. Protocol 4's distance fields cost about 3 percentage points. A shorter encoding or slower telemetry while stopped would bring it back under; that needs the maintainer's decision.

## How it runs

The firmware does its work in **frames of 10 ms** (T1), timed by `micros()`. Each frame reads the link and the MODE button, updates the safety supervisor and the motors, and reports to diagnostics. The battery is sampled every 2nd frame, telemetry goes out every 10th and health every 100th. If a frame's work takes longer than 5 ms (T2), the `frame overrun` event fails; if the firmware falls a whole frame behind, the missed frames are skipped and counted, not run late in a burst.

## What's linked in

Our code is Apache-2.0. The firmware image also contains the Arduino AVR core, its Wire library and the Servo library, which are LGPL-2.1. That's fine for an open-source project: anyone can rebuild the image from this repo with other versions of them. Release notes will list these with their licences.

## What it does for safety

| Requirement | How |
|---|---|
| S1 | `{S}` or the MODE button brakes both tracks. The stop holds until a neutral `{D,0,0}`, so drive messages the computer keeps resending can't restart the robot |
| S2 | OK on the IR remote brakes both tracks through the safety supervisor, and the stop holds like any other |
| S3 | Drive commands expire after 300 ms |
| S4 | A 250 ms watchdog resets the UNO if the loop hangs, and the driver's pull-downs keep the motors off through a reset |
| S5 | The driver stays in standby from power-on until a neutral `{D,0,0}` and then a drive command. If the UNO resets mid-drive, resent drive messages don't restart the motors |
| S9 | Under 7.0 V it reports a low battery. Under 6.4 V it brakes and won't drive until the battery is back above 6.8 V. The reading is filtered over about 0.3 s |
| S10 | PWM is limited so the motors' average voltage stays at or below 6 V |

The battery reading has no correction yet. It hasn't been compared with a multimeter, so the thresholds may be off by a few hundred millivolts.

## Driving and testing from the host computer

`conq`, from the [host package](../host/pyproject.toml), talks to the robot over USB. Every command also takes `--json`.

```
conq version                  # firmware version and commit
conq status                   # latest telemetry
conq drive f 30 0.5           # forward at 30% for half a second, then stop
conq stop
conq shell                    # interactive prompt, for people
conq test --list              # the robot tests in tests/robot/
conq test                     # run them all; logs and a report go to build/robot-tests/
```

Robot tests that move the robot ask for confirmation, or need `--confirm-moves` when run by an agent or script. Questions only a person can answer, such as which way a track turned, can be answered in advance with `--answer tracks_correct=yes`.

We first ran our firmware on 2026-11-09; see the [notebook](../docs/notebook/2026-11-09-first-flash-of-our-firmware.md). Going back to the stock firmware is tested, so it isn't needed after every session.
