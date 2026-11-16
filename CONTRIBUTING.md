# Contributing

Thanks for looking. This project is built mostly by AI agents with one maintainer, and the same rules apply to people and agents. They're in [AGENTS.md](AGENTS.md); the short version:

- **Open an issue first,** so we can agree what to build before anyone builds it.
- **One branch and pull request per issue.** Branches are named `<type>/<issue>-<short-description>`, titles follow [Conventional Commits](https://www.conventionalcommits.org/), and the description says `Closes #<number>` ([decision 0009](docs/decisions/0009-issues-branches-and-commits.md)).
- **Tests come with the change.** `python scripts/dev.py test` runs the firmware's unit tests on your computer, and CI runs them on macOS, Linux and Windows. Name the requirements a test checks ([how](firmware/testing/README.md)).
- **No vendor files.** Never add ELEGOO's code, manuals or firmware. Describe hardware facts in your own words and say how you know them.
- **The robot is physical.** Say in the pull request what you ran on a real robot and what you didn't.
- **Keep docs short and in plain English.**

Setup for macOS, Linux and Windows is in [tools](docs/tools.md). By contributing you agree that your work is licensed under the [Apache License 2.0](LICENSE), like the rest of the project.
