# Test report: increment 2, sensors and status

**Date:** 2026-11-20. **Result: passed, with one judgement.** Every increment 2 robot test passed on our robot on one firmware commit, except three tape readings in the ultrasonic test. The maintainer judged those to come from the tape method, not the sensor (see T3 below). F15 isn't covered.

| | |
|---|---|
| **Firmware** | `0.1.0-dev`, commit `bf9a708`, reported by the robot in every test |
| **Built by** | `scripts/dev.py build` on the maintainer's Mac. CI built the same commit in [run 43752024828](https://github.com/pablot30/open-conqueror/actions/runs/43752024828) |
| **Firmware since** | Changes on `main` after `bf9a708` touch only versioning, the build and docs, not behaviour |
| **Tests** | `conq test`, from `tests/robot/` as of [#84](https://github.com/pablot30/open-conqueror/pull/84) |
| **Setup** | Floor tests (`heading_hold`, `ultrasonic_ruler`) ran on a hard floor over the robot's own Wi-Fi (`tcp:192.168.4.1:100`). The rest ran over USB with the Upload-Cam switch on Upload: tracks lifted for `directions`, `stops` and `ir_drive`, on a table or the floor for `startup`, `gyro_turn` and `health`. The maintainer watched from behind the robot |
| **Evidence** | [report.json](report.json) and one JSON Lines log per test in [logs/](logs/) |

## Results by requirement

| Requirement | Test | Result |
|---|---|---|
| F1: hold a straight line with the gyro, 2 m | `heading_hold` | Passed. 2.14 m forward in 14 s, 3.5 cm to the side, heading −0.4° forward and +0.3° back ([notebook](../../notebook/2026-11-20-heading-hold-on-the-floor.md)) |
| T4: heading hold corrects small drift | `heading_hold`, `startup`, `gyro_turn` | Passed. Heading within about 1° over 14 s with corrections under 4.2%; at rest the rate stayed under 0.06°/s; a hand-turned quarter turn read 80.1°, counted as positive for left |
| F13: calibrate the gyro at power-on | `startup` | Passed. Valid 1.06 s after power-on, heading moved 0.01° in 10 s while still |
| T6: ready within 3 s of power-on | `startup` | Passed. Gyro valid after 1.06 s |
| F5: standby, LED breathing violet | `version_and_standby`, `startup` | Passed. Standby with no motor output, and the maintainer saw the LED breathe violet |
| F14: centre the camera at power-on | `startup`, `camera` | Passed. The camera ended up facing straight ahead; every pan and tilt move was made, smoothly |
| F10: arrows drive, OK stops (arrows and OK only, as #14 scoped it) | `ir_drive` | Passed. Each arrow drove while held and stopped within about a third of a second of release, with no link fault; OK stopped the robot |
| S2: OK stops the robot even with Wi-Fi down | `ir_drive` | Passed. Over USB, with no Wi-Fi in use, OK stopped the robot and the stop held while the computer kept sending drive commands |
| T1: control loop at least 100 times a second | `health`, `version_and_standby` | Passed. No frames skipped, longest frame 1.66 ms, telemetry 10 times a second |
| T3: ultrasonic within 1 cm, 5 to 150 cm | `ultrasonic_ruler` | Passed by judgement. Within 1 cm at 5, 10, 30 and 200 cm; about 2 cm short at 60, 100 and 150 cm. The 2 m point rules out a scale error, so the misses are put down to the tape method ([notebook](../../notebook/2026-11-20-battery-and-ultrasonic-measurements.md), #30). `report.json` marks this test failed, as its checks did |
| F15: a low battery flashes the LED red | none | **Not covered.** Needs a bench supply turned down to the threshold |

### Increment 1, run again

| Requirement | Test | Result |
|---|---|---|
| F2, F3, F4 | `directions` | Passed. All eight moves gave the expected output (63 to 65 of 255 at 30%, 31 on the slower track of a diagonal) and stopped; the maintainer confirmed the tracks |
| S1, S3, S5 | `stops`, `mode_button`, `version_and_standby` | Passed. A stop showed 69 ms after the command; the link timeout stopped the robot 308 ms after the last message; three MODE presses each stopped it and held for 1.5 s |
| D4 | `version_and_standby` | Passed. `0.1.0-dev`, `bf9a708`, protocol 8, matching `VERSION` |

## Notes

- **`stops` over Wi-Fi:** it failed over the Wi-Fi dongle on 2026-11-16 because of that link's latency ([decision 0011](../../decisions/0011-wifi-dongle.md)). Here it passed over USB, and then again over the robot's own Wi-Fi: the stop showed 90 ms after the command, and the link timeout stopped the robot 284 ms after the last message, with no link drops. The report keeps the USB run; the Wi-Fi run's log is kept outside the repo.
- **The battery ran low by the end:** VIN dipped to 7.00 V under load during `directions`, the warning level. Charge before the next session.
- **Measured on the way** (see the notebook entries linked above):
  - the battery reading is 0.4% low at VIN, and the input diode drops about 0.45 V
  - the robot covers about 15 cm/s at 40%
  - the gyro drifts about 1.2° a minute at rest
- **Not covered:** F15, and S4's watchdog reset, which needs a test build that hangs on purpose.
