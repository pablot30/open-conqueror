# open-conqueror

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development. Start with `docs/vision.md`, `docs/roadmap.md` and `docs/architecture.md`.

## Current stage

Stage 2, increment 2. Increment 1, the walking skeleton, passed its robot tests on 2026-11-12 (`docs/test-reports/`). The architecture is agreed (`docs/architecture.md`, decision 0008), and the repo matches it. Increment 2 was built in parts, each a pull request: the 10 ms frame and diagnostics, the devices (gyro, servos, LED, ultrasonic, IR receiver), then IR driving and heading hold. All are merged; heading hold waits for its floor test on a long USB cable, and the increment ends with its test report. We build in small increments that each run on the robot, and add tools and process only when an increment needs them. See `docs/decisions/0006-walking-skeleton-first.md` and the build order in `docs/requirements.md`.

## Rules

- **Stock hardware first.** Don't propose hardware changes until stage 5, unless a decision at hand depends on one. See `docs/decisions/0001-stock-hardware-first.md`.
- **No vendor files.** Never add ELEGOO's code, manuals or firmware to this repo, and never modify a local copy of the kit. Describe hardware facts in our own words.
- **Say how you know.** Hardware facts state their source and whether they were measured on a robot.
- **Requirements.** `docs/requirements.md` defines the first goal, with IDs that issues, tests and commits refer to. Follow the manual's behaviour, and don't reproduce stock bugs.
- **Novelties.** When something we create looks new, propose it in an issue labelled `novelty`, saying what was searched. It gets a page in `docs/novelties/` only after the maintainer approves. See `docs/novelties/README.md`.
- **Record what matters.** Decisions go in `docs/decisions/`, and technical lessons in `docs/notebook/`. Lessons about how to run the project go in `docs/playbook.md`, and milestones and memorable moments in `docs/journey.md`.
- **The robot is physical.** Don't flash it or run its motors without the maintainer's OK. Before any step where the maintainer touches or watches the robot, say which part and where it is, what will move and for how long, what to do and when, and what to watch for.
- **People and agents.** Every tool gets commands that run without a prompt, readable output for people and JSON for agents. See `docs/vision.md`.
- **Units.** Firmware is built from self-contained units, each with its README, version, `src/` and `tests/`. See `docs/architecture.md`.
- **Public sources.** Practices borrowed from automotive, aerospace or elsewhere come from public standards, cited, with our own names.
- **Cross-platform.** Everything must work on macOS, Linux and Windows. Use each language's path APIs and avoid bash-only scripts.
- **No secrets.** Wi-Fi credentials and personal data stay out of the repo.
- **Keep computers clean.** Install with the system package manager or a project virtual environment, and only when a step needs it. List every tool in `docs/tools.md` with how to remove it, and remove what we stop needing.

## Git

- Commit or push only when asked.
- Every piece of work has a GitHub issue. Branches are named `<type>/<issue>-<short-description>`, pull requests close their issue with `Closes #<number>`, and commit messages and pull request titles follow Conventional Commits. See `docs/decisions/0009-issues-branches-and-commits.md`.
- Work on short branches with pull requests. The maintainer merges.
- Keep the issue's checklist true. Tick a box when its pull request merges or its robot test passes, and link the result (a pull request comment or log) on the issue. A result that only lives in a pull request comment or `build/` is invisible to the next session.
- When a pull request builds on another, update it from `main` as soon as the first is merged. Never rename a branch that has an open pull request: GitHub closes the pull request.
- Never force-push, push to `main` directly, or delete `main` or a `release/` branch. `main` changes only through merged pull requests. A GitHub ruleset enforces this, and `.githooks/pre-push` catches mistakes before they reach GitHub.
- Delete work branches once they're merged, locally and on GitHub, so only live work is left.
- Commits use a GitHub noreply address.
- The other vendor's model reviews every pull request: Codex when Claude wrote it, and the other way round. Answer every review comment with a commit or a reason, and stop after two rounds to ask the maintainer. Aim for pull requests under about 400 changed lines. See `docs/decisions/0013-review-by-the-other-vendor.md`.

## Code Review Rules

For the reviewer of a pull request. Flag a change that breaks any of these:

### Safety

- Only the `safety` unit decides what the motors do. Every source of commands (the computer, the IR remote, the MODE button, modes) goes through it.
- A stop holds until a deliberate new command: a neutral `{D,0,0}`, then drive (S1, S5). Drive commands still expire after 300 ms (S3).
- Nothing blocks the 10 ms frame (T1, T2): no `delay()`, busy waits or polling loops in units.

### Limits

- The UNO firmware stays within T8: at most 80% of flash and at least 512 bytes of RAM free. No dynamic memory (`new`, `malloc`, `String`) on the UNO.

### Tests and traceability

- Tests check behaviour, would fail if the code were wrong, and name the requirement IDs they cover.
- A unit whose code changes gets a new version in its `library.properties` (decision 0012).

### Docs and the repo

- Hardware facts say how they're known, and whether they were measured on a robot.
- Docs that the pull request touches, or that describe code it changes, still match the code.
- No ELEGOO code, manuals or firmware; no secrets, Wi-Fi credentials or personal data.

## Docs check

The docs drift, and review only sees the docs a pull request touches. The docs check looks at all of them on `main`. It runs once a week by itself, and by hand before each increment's test report and each release candidate. When asked to "run the docs check":

1. **Update `main`.** If no commit has reached `main` in the last eight days, stop and report "nothing changed".
2. **Run `python scripts/dev.py docs`** and fix what it reports: broken links and anchors, unknown requirement IDs, records missing from their folder's index, decision numbers, unit folders without their files.
3. **Read the docs for what a script can't see:** pages that contradict each other or the code and CI, statements that were true and no longer are (the current stage, what's private, what "waits"), "later" lists whose trigger has happened, and breaks of the Writing rule below. Quote both places, with file and line.
4. **Fix current docs; flag records.** The vision, roadmap, requirements, architecture, playbook, tools list, this file and the READMEs describe today, so fix them. Decisions, notebook entries, research reports and test reports are dated records: leave them, and only flag a missing "replaced by" note.
5. **Open one small, docs-only pull request** with the fixes, and an issue for anything the maintainer must decide. No findings, no pull request. If last week's docs-check pull request is still open, add to it rather than open a second.
6. **Say what wasn't covered:** list the open pull requests, since their docs aren't on `main` yet. Nobody can be asked during a scheduled run, so questions go in the pull request or an issue.

## Writing

Keep docs short and in plain English, and write them for someone who found the repo today.
