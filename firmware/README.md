# Firmware

Our firmware for the robot. Only the UNO has firmware so far, at increment 1, the walking skeleton ([decision 0006](../docs/decisions/0006-walking-skeleton-first.md)). It drives both tracks from a computer and stops when it should. It doesn't use the gyro, LED, servos, ultrasonic sensor, line sensors or IR remote yet.

## Layout

| Folder | Contents |
|---|---|
| `uno/` | The Arduino sketch. `uno.ino` connects the parts |
| `uno/src/core/` | Logic that doesn't touch hardware: protocol, driving, battery, button. It builds on a computer too, for the unit tests |
| `uno/src/hal/` | The only code that touches pins |
| `tests/` | Unit tests for `core/`, built with CMake on the computer |

## Build, test and flash

Install the tools in [tools.md](../docs/tools.md) first. From the repo root, on any system:

```
python scripts/dev.py build
python scripts/dev.py test
python scripts/dev.py upload --port <port>
```

Use `python3` instead of `python` where that's the name. The build fails if our code has compiler warnings, or breaks the T8 limits: 80% of flash and 512 bytes of free RAM. Flashing needs the Upload-Cam switch on **Upload**.

## Protocol

Messages are short text inside braces, at 9600 baud. That's what the stock camera firmware's bridge forwards, so the same messages should work over Wi-Fi in increment 4. The firmware ignores anything outside braces.

**From the computer:**

| Message | Meaning |
|---|---|
| `{D,left,right}` | Drive. Each track is -100 to 100 percent of the allowed maximum. Resend at least every 300 ms, or the robot stops (S3) |
| `{D,0,0}` | Neutral: brake, and release a held stop. After power-on and after every stop, the robot ignores other drive messages until it gets this one |
| `{S}` | Stop: brake both tracks |
| `{"N":100}` | Stop. The stock bridge sends this when a client disconnects |
| `{V}` | Ask for the version |

**From the robot:**

| Message | Meaning |
|---|---|
| `{V,version,commit}` | Firmware version and git commit, at startup and on request (D4). The commit ends in `-dirty` if the build had uncommitted changes |
| `{T,ms,state,left,right,mV,faults,loop}` | Telemetry, 10 times a second. `state`: 0 standby, 1 driving, 2 stopped. `left` and `right`: motor PWM, -255 to 255. `mV`: battery at Vin. `faults`: 1 link timeout, 2 battery low, 4 battery critical. `loop`: longest loop time since the last message, in µs |
| `{E,1}` | A message wasn't understood |

## What it does for safety

| Requirement | How |
|---|---|
| S1 | `{S}` or the MODE button brakes both tracks. The stop holds until a neutral `{D,0,0}`, so drive messages the computer keeps resending can't restart the robot |
| S3 | Drive commands expire after 300 ms |
| S4 | A 250 ms watchdog resets the UNO if the loop hangs, and the driver's pull-downs keep the motors off through a reset |
| S5 | The driver stays in standby from power-on until a neutral `{D,0,0}` and then a drive command. If the UNO resets mid-drive, resent drive messages don't restart the motors |
| S9 | Under 7.0 V it reports a low battery. Under 6.4 V it brakes and won't drive until the battery is back above 6.8 V. The reading is filtered over about 0.3 s |
| S10 | PWM is limited so the motors' average voltage stays at or below 6 V |

The battery reading has no correction yet. It hasn't been compared with a multimeter, so the thresholds may be off by a few hundred millivolts.

## First flash on the robot

We ran this on 2026-11-09; see the [notebook](../docs/notebook/2026-11-09-first-flash-of-our-firmware.md). To repeat it:

1. Tracks off the ground, battery charged, switch on **Upload**, USB connected.
2. `python scripts/dev.py build`, then `upload`.
3. Start the tool: `python host/oc.py --port <port>`. Check the version message and `status`: standby, no motor output, a plausible battery voltage.
4. Drive each direction at 30% for 1 s with `go f 30`, `go b 30`, `go l 30` and so on. Check each track turns the right way.
5. Stops: `stop` mid-drive. Press MODE mid-drive. Close the tool mid-drive, which stops the resends: the robot should stop within about 300 ms.
6. Restore the stock firmware from the backup (D6).
