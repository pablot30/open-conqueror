# 0007: How we test, release and trace our software

**Date:** 2026-11-11. **Status:** draft, for the maintainer to review.

## Context

Increment 1 ran on the robot on 2026-11-09, and the session showed four gaps:

- The robot tests were throwaway scripts outside the repo, so nobody can rerun them exactly.
- CI builds the firmware and then throws the image away, so no test result points to a binary.
- There's no version scheme or test report, and no agreed idea of when something is ready to release.
- `host/oc.py` only works as an interactive prompt, so an agent can't drive it, and its output is meant for people.

We want a light setup that closes these gaps for increment 1 and scales later, without the over-planning that [0006](0006-walking-skeleton-first.md) cut back.

## Decision

### Versions

- **[Semantic Versioning](https://semver.org): MAJOR.MINOR.PATCH.** PATCH for fixes, MINOR for new features, MAJOR when the protocol stops being compatible. Before 1.0.0, anything may change.
- **One version for the whole repo:** both firmwares and the computer tool. It lives in a `VERSION` file at the repo root. The build passes it to the firmware, and the computer tool reads it too.
- **While we develop, the version ends in `-dev`,** such as `0.1.0-dev`. Every firmware also reports its git commit (D4), so a running robot always tells you exactly which build it has.
- **0.1.0 is the first release,** at the end of stage 2: our UNO firmware does everything the stock UNO firmware does, including the modes, the IR remote and driving over Wi-Fi through the stock camera board.
- **1.0.0** comes when the whole robot runs our software with the stock feature set, at the end of stage 3.

### Test levels

| Level | What | Runs | Evidence |
|---|---|---|---|
| 1. Unit tests | Logic that doesn't touch hardware, on a computer | CI, on every pull request, on macOS, Linux and Windows | CI run |
| 2. Static checks | Warnings as errors, cppcheck, flash and RAM limits | CI, on every pull request | CI run |
| 3. Robot tests | Scripts that drive the robot and check its telemetry | By hand or by an agent, with the maintainer present | JSON log |
| 4. Increment check | Every robot test for the requirements in the increment | At the end of each increment | Test report |
| 5. Release check | Every robot test so far, plus time on the robot (see Releases) | On each release candidate | Test report and use log |

### Robot tests

- They live in the repo, in `tests/robot/`. Each one states the requirement IDs it checks, the setup (for example, tracks lifted), what the person does and what they should see.
- They check the telemetry themselves, and ask the person only for what a script can't see, such as which way a track turned. The answers go into the log.
- The person running them gets a clear brief before anything moves: which part to touch, what will move and for how long, and what to watch for.

### Traceability

One chain, from requirement to release:

```
requirement  →  test  →  result  →  release
requirements.md   unit test or     CI run or      tag, firmware image,
                  robot test       JSON log       test report
```

- Tests name the requirement IDs they check, as unit tests already do in comments.
- A script lists each requirement in the release and the tests that cover it, and flags any with none. Its output goes into the test report.

### Increments

An increment is development, not a release. It ends when its robot tests pass on a build of `main`. The test report names that commit and links the CI run that built it. Nothing is tagged.

### Releases

- **We release only what's worth using,** and only after it has proven itself on the robot over time. Passing tests isn't enough on its own.
- **Release candidates come first.** When a stage's features are complete, we tag a candidate such as `v0.1.0-rc.1` and use the robot with it across several sessions: full battery runs, every mode many times, and normal play. Any fix makes a new candidate. The detailed criteria are set when the first candidate is near, from what the increments taught us. As a start: no unexplained resets or stops, and no open bug that affects safety.
- **A release is a git tag** such as `v0.1.0` on `main`, on the same commit as the last candidate. Once published, a tag is never moved or deleted. The maintainer creates releases, the same way they merge pull requests.
- **A GitHub Release holds what we ship and what proves it:** the firmware image built by CI from that tag, the test report and the robot test logs. These are kept for good, unlike CI files, which expire after 90 days.
- **`CHANGELOG.md`** lists what each release changed, with requirement IDs.
- **Flashing a release** uses the image attached to it, so the binary we tested is the binary we install. Restoring the stock firmware is part of every release check (D6).
- Candidates are marked as pre-releases on GitHub.

### Tools work for people and agents

- Every tool has commands that run without a prompt, and gives JSON output when asked. Examples: `oc.py drive f 30 0.5`, `oc.py test tests/robot/stop.py --json`, `dev.py build --json`.
- The human view stays: the `oc.py` prompt and readable summaries.
- Logs are JSON Lines, one message per line with a timestamp, so people, agents and later a database can all read them.

## What waits

Each comes back when its trigger arrives:

| Later | Trigger |
|---|---|
| A results database (an SQLite file filled from the JSON logs first) | Comparing many runs across versions, in stage 4 or with unattended agent testing |
| Release manifests, calibration defaults in releases, signed images (D16, S12) | Stage 3, when two firmwares must stay in step and updates go over Wi-Fi |
| Coverage gates and the MISRA mapping (Q1, Q3) | The code settles, as [0006](0006-walking-skeleton-first.md) says |
| A separate version for each software unit (rest of D4) | A unit is reused outside this firmware |

## Consequences

- The walking skeleton isn't released. Its robot tests move into the repo, and the first robot session of increment 2 reruns them on a build of `main`, since the build we tested on 2026-11-09 came from uncommitted files.
- The first release, 0.1.0, waits for the end of stage 2 and its candidates. Until then, the increment test reports are the record of what works.
- Robot tests take a little longer to write than throwaway scripts, but they can be rerun by anyone, including an agent, which is the project's end goal.
