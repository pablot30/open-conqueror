# Handover: a docs check and a traceability command

**Date:** 2026-11-16. **Written by:** Claude Fable 5.2, at the maintainer's request. **For:** the agent (Opus) that builds these two things.

**How we know:** read from `origin/main` at `78cf8a8` and the working tree on 2026-11-16. Nothing here was run on the robot, and nothing needs the robot.

The maintainer asked for both and has answered the open questions; their answers are marked "decided" below. Show them the plan for each part before building it.

## Before you start

- Read `AGENTS.md`. Each of the two parts gets its own issue, branch and pull request, under about 400 changed lines, reviewed by Codex (decision 0013).
- Branch from a fresh `main`. This file was written on `feat/34-conq-over-wifi`, which is behind it.
- Neither part needs a new tool installed. Use Python's standard library, as `scripts/dev.py` does.
- Neither part is a novelty, in my judgement: a traceability table and a consistency check on docs are both established practice. If your design turns into something we haven't seen elsewhere, follow the novelty rule in `AGENTS.md`.
- Build part 2 first. Part 1's first run can then use its output.

## Part 1: a regular check of the docs

### Why

The docs are how a new person or agent learns the project, and they drift. The project review of 2026-11-16 found three cases by hand: a "later" list that disagreed with what CI enforces, a journey four days behind, and a link to a decision that a branch didn't have. Review by the other vendor (0013) only looks at docs a pull request touches. Nothing looks at the docs no pull request touched.

### The name

`dev.py lint` already means cppcheck on the firmware. Call this the **docs check**, so the two aren't confused.

### Two halves

**A script, for what a script can decide.** Suggested: `python scripts/dev.py docs [--json]`.

| Check | Example of what it catches |
|---|---|
| Every relative link and anchor in a Markdown file resolves | A link to a decision that was renumbered |
| Every requirement ID mentioned in the docs exists in `docs/requirements.md` | A test report naming a requirement that was removed |
| Every decision, notebook entry, research report, novelty and test report is listed in its folder's `README.md`, and every row there has a file | A new notebook entry missing from the index |
| Every decision number is used once, with no gaps | Two branches both adding 0014 |
| Every unit folder has a `README.md` and `library.properties` | A unit added without its README |

`host/tests/test_project.py` already holds checks of this kind ("keep the host package, the robot tests and the docs in step"), including the regular expression that reads requirement IDs. Reuse it rather than writing a second one.

**An agent, for what needs judgement.** A script can't tell that two pages disagree. An agent reads the current docs and reports:

- Statements that contradict each other, or contradict the code or CI.
- Statements that were true and no longer are: the current stage, what's private, what "waits".
- "Later" lists whose trigger has already happened.
- Plain-English and length problems against the "Writing" rule in `AGENTS.md`.

### Rules for the agent's pass

- **Current docs get fixed. Records don't.** The vision, roadmap, requirements, architecture, playbook, tools list, `AGENTS.md` and the READMEs describe the project today, so fix them. Decisions, notebook entries, research reports and test reports are dated records of what was true then. Decision 0007 still says `oc.py`, and that is correct history. For a record, the only thing to flag is a missing "replaced by" note.
- **Findings come with evidence:** the two places that disagree, quoted, with file and line.
- **The output is a pull request with the fixes,** or issues for anything that needs the maintainer's decision. Not a report that nobody reads.
- **It runs every week, automatically** (see the decisions below), and by hand before each increment's test report and each release candidate.

### Where to write it down

One new playbook step, and one line in `AGENTS.md` saying when the docs check runs. No decision record is needed unless the maintainer wants the script as a required CI check.

### A good first run

The maintainer plans to make the repo public. That one event makes several current statements wrong: the pre-push hook as the reason `main` is protected (`AGENTS.md`, playbook step 9, `docs/tools.md`), the roadmap's "we publish the repo when", and the research index's notes on what is private. Run the first docs check right after publishing.

### Decided by the maintainer on 2026-11-16

- **The script half reports first.** It doesn't fail CI. Ask again after one increment.
- **The agent half runs automatically, once a week to start with.** Nobody has to remember it. The maintainer first chose "at the end of every session", then changed to this.
- **It skips the run when nothing has changed** on `main` since the last one.
- **It reads `main` only.** Open pull requests are unfinished, and Codex reviews each one's docs when it opens (0013).

### How to run it every week

Suggested by me, not yet confirmed by the maintainer.

1. **A scheduled cloud routine,** made with Claude Code's schedule skill. One has already worked on this repo: it opened pull request 20. It uses the maintainer's subscription, so the repo needs no token or secret, and the maintainer's computer can be off.
2. **Its instructions live in `AGENTS.md`,** in a "Docs check" section: update `main`, run `dev.py docs`, do the agent pass by the rules above, and open a pull request if anything was found. The routine's own prompt is then one sentence, "Run the docs check", and the same sentence works by hand in any Claude or Codex session.
3. **The skip test comes first, and needs no stored state:** if no commit has reached `main` in the last eight days, stop and report "nothing changed". Eight days, not seven, so a late run doesn't miss a day.
4. **It can't ask the maintainer anything.** Questions go into the pull request or an issue.

### Which model runs it

The maintainer decided on 2026-11-16 that Codex does the finding, with the Claude routine as the fallback:

- **Codex should do the finding, if it can run unattended on a schedule.** Claude writes most of the docs, and a model is poor at seeing its own gaps. That is decision 0013's reasoning, and it applies here too. If Claude runs the check, Codex only ever sees the lines Claude chose to fix, never what Claude missed.
- **Codex files one issue with its findings; it doesn't fix them.** A Claude session then fixes them in a pull request, and Codex reviews that pull request automatically, as it does today. Both vendors stay in the loop with nothing new to set up. If Codex wrote the fix as well, the rule in 0013 would need Claude to review it, and no automatic Claude review exists yet.
- **Fallback: the Claude routine above does both halves.** Use it if Codex can't be scheduled. It's weaker, but it's proven on this repo.

*To verify, before choosing:* whether Codex has scheduled or automated tasks that run in the cloud with the computer off, whether such a task can open an issue on this repo, and whether a comment that mentions Codex on an issue can start a task. I don't know any of these for certain. If the last one works, a tiny scheduled workflow that opens a "docs check" issue and mentions Codex would do, with no agent token in the repo.

Why not a GitHub Actions schedule that runs the agent itself: decision 0013 uses Codex's own GitHub integration, so the repo has no workflow that runs an agent and no token for one. A scheduled workflow would add both. It becomes the better choice if 0013's one-week count leads to Anthropic's GitHub Action being set up anyway.

*To verify:* that a routine can open a pull request on a public repo the same way, how the schedule is changed or removed, and what a run costs against the subscription.

### What the weekly run does about unmerged work

- **Its summary lists the open pull requests it didn't cover,** so nobody assumes they were checked.
- **No findings, no pull request.** Otherwise the maintainer gets one more thing to merge every week.
- **One open docs-check pull request at a time.** If last week's is still open, add to it or stop; don't stack a second one.
- **Keep the fix pull request small and docs-only,** so it rarely conflicts with the open ones. If it does, the open pull request's author updates from `main`, as `AGENTS.md` already says.
- **The fix pull request is itself a change to `main`,** so the week after a fix, the check runs once more and should find nothing. That's a useful confirmation, not waste.

## Part 2: from requirement to release, in one command

### Why

Decision 0007 promises one chain, "requirement → test → result → release", and "a script [that] lists each requirement in the release and the tests that cover it, and flags any with none". That script doesn't exist. The increment 1 test report's "Results by requirement" table was written by hand.

### What already exists

| Link in the chain | Where it is today | Machine-readable? |
|---|---|---|
| Requirement IDs | Table rows in `docs/requirements.md` | Yes. `requirement_ids()` in `host/tests/test_project.py` reads them |
| Which increment a requirement belongs to | The "Build order" table in `docs/requirements.md` | Partly. It uses ranges and notes, such as "F6 to F9" and "S1 (computer tool and MODE button)" |
| Robot tests to requirements | `REQUIREMENTS = [...]` in each file in `tests/robot/` | Yes. `test_project.py` already rejects unknown IDs |
| Robot results | `report.json` in each folder of `docs/test-reports/`: every check has its requirement IDs, a status and the firmware's version and commit | Yes |
| Unit tests to requirements | A comment on each test function's first line, such as `void startsInStandby() {  // S5` | By convention only. Nothing checks it |
| Unit results | `dev.py test --json`, one result per unit | Yes, per unit, not per test function |
| Release | Git tags. None exist yet | Not yet |

So the data is there, and the join is missing.

### What to build

`python scripts/dev.py trace [--increment N] [--json]`. It reads files in the repo only: no robot, no network.

One row per requirement:

```json
{
  "id": "S3",
  "increment": 1,
  "unit_tests": [{"unit": "safety", "test": "stopsWhenLinkGoesQuiet"}],
  "robot_tests": ["stops"],
  "latest_robot_result": {
    "status": "passed",
    "report": "docs/test-reports/2026-11-12-increment-1",
    "firmware": {"version": "0.1.0-dev", "commit": "64e636d"}
  },
  "flags": []
}
```

The text output is the same thing as a table, in the shape of the "Results by requirement" table in the increment 1 report, so a test report can include it as it is.

Flags, in order of value:

1. **No test.** A requirement in the chosen increment that no unit test or robot test names.
2. **No result.** A requirement with a robot test and no report containing it.
3. **Not passed.** The latest result is failed, skipped, unanswered or incomplete.
4. **Unknown ID.** A test names a requirement that doesn't exist. Robot tests are already covered; unit tests aren't.
5. **Possibly stale.** The firmware has changed since the commit the latest result was measured on. Start simple: say how many commits have touched `firmware/uno/` since. Don't try to work out which unit affects which requirement.

### Steps

1. **Make the unit test tag a rule.** Keep the existing comment, and define it: on the test function's first line, `// ` followed by requirement IDs separated by commas. Write that in `firmware/testing/` and check that every ID exists. A test with no tag is allowed, since some tests cover no requirement. Read `firmware/testing/check.h` first; if the harness already registers tests by name, a tag given at registration is cleaner than a comment. Check `host/tests/` for the same need on the Python side.
2. **Read the build order.** Either parse the ranges, or change the table so each row lists plain IDs. Changing the table is simpler and makes the doc clearer, but it edits `docs/requirements.md`, so ask the maintainer.
3. **Write `trace`,** with unit tests for the parsing and the flags, using small made-up inputs.
4. **Run it on increment 1 and compare** with the hand-written table in `docs/test-reports/2026-11-12-increment-1/README.md`. They should agree, including the two gaps that report names: S9's battery thresholds and S4's watchdog reset. If they don't, find out which is wrong.
5. **Use it for increment 2's test report.**

### What to leave out

- **No CI gate yet.** Report only, for at least one increment, then ask the maintainer. Flag 1 on the current increment is the first candidate for a gate.
- **No database.** Decision 0007 says an SQLite file waits for "comparing many runs across versions".
- **No release column until there are tags.** Leave room for it in the JSON.
- **Safety goals (SG1 to SG8) aren't traced directly.** Each safety requirement names its goal, so a later version can roll results up to the goals.
- **No graph tool or generated wiki.** The maintainer and I discussed both on 2026-11-16 and left them for stage 3 at the earliest.

### Decided on 2026-11-16

- **The build order table may be rewritten** to list plain IDs, and improved where that makes it clearer. The maintainer agreed.
- **`trace` goes in `scripts/dev.py`.** The maintainer left this to us. `dev.py` holds the commands that work on the repo (build, test, lint), and `conq` holds the ones that talk to a robot. `trace` only reads files.

## Done means

- **Part 2:** `dev.py trace --increment 1` reproduces the hand-written table, `--json` works, its unit tests run in CI on all three systems, and increment 2's test report uses it.
- **Part 1:** `dev.py docs` runs clean on `main`, the weekly routine has run once and ended in a merged pull request or a "nothing changed" report, and the playbook has a step for it.
