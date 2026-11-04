# open-conqueror

Open-source software for the ELEGOO Conqueror tank, rebuilt from scratch through AI-driven development. Start with `docs/vision.md` and `docs/roadmap.md`.

## Current stage

Stage 0, organizing. Don't write firmware until the roadmap reaches that stage and the maintainer agrees.

## Rules

- **Stock hardware first.** Don't propose hardware changes until stage 5, unless a decision at hand depends on one. See `docs/decisions/0001-stock-hardware-first.md`.
- **No vendor files.** Never add ELEGOO's code, manuals or firmware to this repo, and never modify a local copy of the kit. Describe hardware facts in our own words.
- **Say how you know.** Hardware facts state their source and whether they were measured on a robot.
- **Feature target.** `docs/features.md` defines the first goal. Follow the manual's behaviour, and don't reproduce stock bugs.
- **Record what matters.** Decisions go in `docs/decisions/`, and lessons in `docs/notebook/`.
- **The robot is physical.** Don't flash it or run its motors without the maintainer's OK.
- **Cross-platform.** Everything must work on macOS, Linux and Windows. Use each language's path APIs and avoid bash-only scripts.
- **No secrets.** Wi-Fi credentials and personal data stay out of the repo.

## Git

- Commit or push only when asked.
- Work on short branches with pull requests. The maintainer merges.
- Commits use a GitHub noreply address.

## Writing

Keep docs short and in plain English, and write them for someone who found the repo today.
