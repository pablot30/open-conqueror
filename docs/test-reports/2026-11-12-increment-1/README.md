# Test report: increment 1, the walking skeleton

**Date:** 2026-11-12. **Result: passed.** Every robot test for increment 1 passed on our robot.

| | |
|---|---|
| **Firmware** | `0.1.0-dev`, commit `64e636d`, reported by the robot |
| **Built by** | `scripts/dev.py build` on the maintainer's Mac. CI built the same commit in [run 43197691167](https://github.com/pablot30/open-conqueror/actions/runs/43197691167) and kept its image |
| **Tests** | `conq test`, from `tests/robot/` at the same commit |
| **Setup** | Robot on a table with its tracks on the surface, USB connected, Upload-Cam switch on Upload, main power on. The maintainer watched from behind the robot |
| **Evidence** | [report.json](report.json) and one JSON Lines log per test in [logs/](logs/) |

## Results by requirement

| Requirement | Test | Result |
|---|---|---|
| D4: version and commit reported | `version_and_standby` | Passed. The robot reported `0.1.0-dev` and `64e636d`, matching `VERSION` |
| S5: motors off from power-on | `version_and_standby`, `stops` | Passed. Standby with no motor output; drive commands ignored until a neutral one |
| T2: no blocking over 5 ms | `version_and_standby` | Passed. Longest loop 1.25 ms |
| F2, F3: turn on the spot, drive in eight directions | `directions` | Passed. Every move sent the expected output (63 of 255 at 30%, 31 on the slower track of a diagonal), and the maintainer confirmed every move turned the tracks the right way |
| S10: motors' average voltage at most 6 V | `directions` | Passed. Output at 30% matched a limit of about 211 of 255 at 7.2 V, which averages 6 V at full command |
| F4: stop on command | `directions`, `stops` | Passed. Stopped after every move |
| S1: a stop from the computer or the MODE button, and it holds | `stops`, `mode_button` | Passed. The stop showed in telemetry 61 ms after the command. Three MODE presses each stopped the robot and held for 1.5 s while drive commands kept coming |
| S3: stop after 300 ms without messages | `stops` | Passed. Stopped within 290 ms, with the link-timeout fault set, and stayed stopped when messages returned |
| S4: no driving after a reset until a neutral command | `stops` | Passed for power-on, which is the same path. A watchdog reset wasn't forced |

## Notes

- **One answer came after the run.** The `directions` test asks whether every move turned the tracks the right way. The test was run without the answer and reported "incomplete"; the maintainer answered yes in chat straight after, and the report records it.
- **Not covered:** S9's battery thresholds need a bench supply or a test build, and S4's watchdog reset needs a test build that hangs on purpose. Both are on the list for later.
- The first upload attempt failed because the Upload-Cam switch was on Cam.
- Our firmware stays on the robot. Restoring the stock firmware was tested on 2026-11-09, so it isn't repeated after every session.
