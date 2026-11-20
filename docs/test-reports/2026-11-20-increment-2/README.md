# Test report: increment 2, sensors and status

**Status: draft, not yet passed.** Every requirement has been tested on our robot, but over five days and four firmware commits. The report has to rest on the one the robot runs now, `bf9a708`, so the tests that ran on older commits run again, in one session (#87).

| | |
|---|---|
| **Firmware** | `0.1.0-dev`, commit `bf9a708`, reported by the robot |
| **Built by** | `scripts/dev.py build` on the maintainer's Mac. CI built the same commit in [run 43752024828](https://github.com/pablot30/open-conqueror/actions/runs/43752024828) |
| **Firmware since** | Changes on `main` after `bf9a708` touch only versioning, the build and docs, not behaviour |
| **Tests** | `conq test`, from `tests/robot/` |
| **Links** | USB until 2026-11-16. From 2026-11-20, the robot's own Wi-Fi (`tcp:192.168.4.1:100`, [#63](https://github.com/pablot30/open-conqueror/issues/63)) |
| **Evidence** | The raw logs are kept outside the repo until the final run. They are copied in with it |

## Results by requirement

| Requirement | Test | Latest result | Commit |
|---|---|---|---|
| F1: hold a straight line with the gyro, 2 m | `heading_hold` | **Passed** 2026-11-20 on a hard floor: 2.14 m, 3.5 cm to the side, heading −0.4° forward and +0.3° back ([notebook](../../notebook/2026-11-20-heading-hold-on-the-floor.md)) | `bf9a708` |
| T4: heading hold corrects small drift | `heading_hold`, `gyro_turn` | **Passed** 2026-11-20: the heading stayed within about 1° over 14 s, with corrections under 4.2% | `bf9a708` |
| T3: ultrasonic within 1 cm, 5 to 150 cm | `ultrasonic_ruler` | **Passed at 5, 10, 30 and 200 cm**; about 2 cm short at 60, 100 and 150 cm, judged to come from the tape method ([notebook](../../notebook/2026-11-20-battery-and-ultrasonic-measurements.md), #30) | `bf9a708` |
| T1: control loop at least 100 times a second | `health` | **Passed** 2026-11-18 over Wi-Fi: no frames skipped, longest frame 1.6 ms, and the same again in every status read on 2026-11-20 | `bf9a708` |
| F13: calibrate the gyro at power-on | `startup` | Passed 2026-11-13: valid 1.10 s after power-on, still to within 0.07°/s | `dab3810`, rerun |
| T6: ready within 3 s of power-on | `startup` | Passed 2026-11-13: gyro valid after 1.10 s | `dab3810`, rerun |
| F5: standby, LED breathing violet | `startup` | Passed 2026-11-13, confirmed by the maintainer | `dab3810`, rerun |
| F14: centre the camera at power-on | `startup`, `camera` | **Not recorded.** The question went unanswered in every run so far | rerun |
| F10: arrows drive, OK stops (arrows and OK only, as #14 scoped it) | `ir_keys`, `ir_drive` | Passed 2026-11-15: each arrow drove while held and stopped after release; OK stopped and held | `8ca87f6`, rerun |
| S2: OK stops the robot even with Wi-Fi down | `ir_drive` | Passed 2026-11-15, over USB with no Wi-Fi in use | `8ca87f6`, rerun |
| F15: a low battery flashes the LED red | none | **Not covered.** Needs a bench supply turned down to the threshold | — |

Increment 1's safety tests (`stops`, `mode_button`, `directions`) also run again on `bf9a708`, to show nothing regressed. `stops` failed over the Wi-Fi dongle on 2026-11-16 because of that link's latency ([decision 0011](../../decisions/0011-wifi-dongle.md)). It hasn't run over the robot's own Wi-Fi yet.

## The session still to run

All on `bf9a708`, with the maintainer at the robot. About 30 minutes:

1. **Over USB,** because these need the restart that opening the port causes: `version_and_standby` and `startup` (F5, F13, F14, T6). Answer the camera and LED questions this time.
2. **Over the robot's Wi-Fi, tracks lifted:** `camera` (F14), `directions`, `stops`, `mode_button`.
3. **On the floor:** `gyro_turn`, then `ir_drive` with the remote (F10, S2), and `health` (T1).

If everything passes, this report is marked passed with the logs from that session, and #14 closes.

## Notes

- **Measured on the way:**
  - the battery reading is 0.4% low at VIN, and the input diode drops about 0.45 V
  - the robot covers about 15 cm/s at 40%
  - the gyro drifts about 1.2° a minute at rest

  See the notebook entries linked above.
- **Not covered in this increment:** F15, for want of a bench supply.
