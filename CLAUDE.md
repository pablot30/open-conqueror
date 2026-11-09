# open-conqueror

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development. Start with `docs/vision.md` and `docs/roadmap.md`.

## Current stage

Stage 2, increment 1: the walking skeleton. Stages 0 and 1 are done. We build in small increments that each run on the robot, and add tools and process only when an increment needs them. See `docs/decisions/0006-walking-skeleton-first.md` and the build order in `docs/requirements.md`.

## Rules

- **Stock hardware first.** Don't propose hardware changes until stage 5, unless a decision at hand depends on one. See `docs/decisions/0001-stock-hardware-first.md`.
- **No vendor files.** Never add ELEGOO's code, manuals or firmware to this repo, and never modify a local copy of the kit. Describe hardware facts in our own words.
- **Say how you know.** Hardware facts state their source and whether they were measured on a robot.
- **Requirements.** `docs/requirements.md` defines the first goal, with IDs that issues, tests and commits refer to. Follow the manual's behaviour, and don't reproduce stock bugs.
- **Record what matters.** Decisions go in `docs/decisions/`, and technical lessons in `docs/notebook/`. Lessons about how to run the project go in `docs/playbook.md`, and milestones and memorable moments in `docs/journey.md`.
- **The robot is physical.** Don't flash it or run its motors without the maintainer's OK.
- **Cross-platform.** Everything must work on macOS, Linux and Windows. Use each language's path APIs and avoid bash-only scripts.
- **No secrets.** Wi-Fi credentials and personal data stay out of the repo.
- **Keep computers clean.** Install with the system package manager or a project virtual environment, and only when a step needs it. List every tool in `docs/tools.md` with how to remove it, and remove what we stop needing.

## Git

- Commit or push only when asked.
- Work on short branches with pull requests. The maintainer merges.
- Never force-push, push to `main` directly, or delete branches on GitHub. `main` changes only through merged pull requests. GitHub can't enforce this on a private repo on the free plan, so `.githooks/pre-push` does it locally.
- Commits use a GitHub noreply address.

## Writing

Keep docs short and in plain English, and write them for someone who found the repo today.
