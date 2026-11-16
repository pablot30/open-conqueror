# Project review and improvement brief

**Date:** 2026-11-16. **Written by:** Claude Fable 5.2, at the maintainer's request. **For:** the agent (Opus) that carries these improvements out.

**How we know:** read from the docs, the git history, `gh pr list` and a local build on 2026-11-16, at commit `72808ff` on `feat/34-conq-over-wifi`. Nothing here was measured on the robot. Facts about GitHub, Claude and Codex products are from memory and are marked *to verify*: check the current documentation before relying on them.

Read `AGENTS.md` first. Its rules apply to everything below: an issue per piece of work, a branch and pull request per issue, and the maintainer merges.

## Status of each item

| # | Item | Status |
|---|---|---|
| 1 | Automatic review on every pull request | **Agreed by the maintainer.** Do this first |
| 2 | Publish the repo | Proposed. The maintainer decides |
| 3 | Stop adding process until release 0.1.0 | Proposed |
| 4 | Bring part of the automation goal into stage 2 | Proposed |
| 5 | Put a time limit on the Wi-Fi dongle | Proposed |
| 6 | Budget RAM per increment | Proposed |
| 7 | Bring the journey and playbook up to date | Proposed |
| 8 | Don't build ahead of the architecture | Proposed |

For the proposed items, ask the maintainer before starting. Don't treat this brief as their approval.

## What works, and should not change

- Increments that each run on the robot (decision 0006).
- "Say how you know", and a notebook that records wrong conclusions.
- The guard, gateway and brain roles, one safety supervisor that alone sets the motors, and logic that builds and is tested on the computer.
- `conq` and `scripts/dev.py` with JSON output, which let an agent run robot tests.
- The build: 49.5% of flash, against 98% for the stock firmware.

## 1. Automatic review on every pull request

### The problem

All 20 merged pull requests have zero reviews (`gh pr list --state merged --json reviews`). PR 18 added 1,876 lines and merged six minutes after it opened. PRs 27, 28, 29 and 32 merged within 23 seconds of each other. The roadmap says "the maintainer reviews and merges", and only the second half happens. Unit tests and robot tests are the only real gates today.

One agent writes nearly all the code, so one model's blind spots go straight to `main`.

### The proposal

Review each pull request on GitHub, automatically, by a model from a different vendor than the one that wrote it. The maintainer has Claude and Codex subscriptions.

1. **Codex reviews every pull request.** Claude writes most of the code, so Codex is the independent reviewer. *To verify:* Codex has a GitHub integration with a code review setting that reviews pull requests automatically, or on an `@codex review` comment, and it is included in ChatGPT plans. If so, it needs no workflow file and no API key.
2. **Claude reviews as well, in a fresh context,** through Anthropic's GitHub Action. *To verify:* `anthropics/claude-code-action` runs on `pull_request` events and can sign in with a subscription token made by `claude setup-token`, stored as a repository secret. A fresh Claude session has none of the author's assumptions, and it can be told to check things Codex won't know to look for.
3. **Start with Codex alone for a week.** Count how many of its findings were real. Add the Claude review if the count justifies a second reviewer. This follows the project's own rule of adding tools when they're needed.
4. **When Codex writes the code, Claude is the reviewer.** The rule is "the other vendor reviews", not "Codex reviews".

### The feedback loop

```
pull request opened or updated
  → reviewer posts inline comments and one summary
  → the authoring agent answers every comment: a fix, or a reason not to
  → push → the reviewer looks again
  → after two rounds, stop and ask the maintainer
  → the maintainer reads the summary and the open points, then merges
```

- The authoring session should stay subscribed to its pull request. In the Claude desktop app, Auto-fix does this: it wakes the session on review comments and CI failures.
- Every comment ends resolved, with either a commit or a written reason. No silent dismissals.
- Two rounds at most. Two models can argue forever, and the maintainer's time is the scarce resource.
- The maintainer reads the review summary, not the whole diff. That is an honest description of the gate, and the roadmap's "How we work" should say so.

### What the reviewers check

Both tools read `AGENTS.md` (decision 0010; *to verify* that Codex's review follows a review section there). Add a short "Review" section, with checks specific to this project:

- Only the safety supervisor sets the motors, and every command source goes through it.
- Nothing can block the 10 ms loop (T2). No waits in drivers.
- Stops hold until a deliberate new command (S1), and the link timeout still applies (S3).
- Flash and RAM stay within T8. No dynamic memory.
- Tests check behaviour, name their requirement IDs, and would fail if the code were wrong.
- Hardware facts say how they're known.
- No vendor code, no secrets, no personal data.
- Docs changed by the pull request still match the code.

### Limits to state plainly

- A model can't see the robot. Robot tests stay the gate for behaviour.
- A 1,900-line pull request can't be reviewed well by anyone. Aim for under about 400 changed lines, and split larger work. Record this as a guideline first, not a CI check (see item 3).
- Review by a model isn't proof. It's a second look that costs the maintainer nothing.

### Things to check before building it

- **Secrets.** Tokens go in GitHub repository secrets, never in the repo. Use the `pull_request` trigger, not `pull_request_target`, so a fork can never reach them once the repo is public.
- **Permissions.** Give the review workflow `contents: read` and `pull-requests: write`, and nothing more. Reviewers comment; they don't push.
- **Actions minutes.** A private repo on the free plan has a monthly limit, and macOS and Windows runners use it up faster than Linux. *To verify* the current numbers, and how much the three-system CI already uses. A public repo has no such limit, which is one more reason for item 2.
- **Required checks.** "Require conversations to be resolved before merging" needs branch protection, which a private repo on the free plan doesn't have. Until the repo is public, the loop is a convention.

### How to deliver it

One issue, one decision record (the next free number; 0012 is taken on `main`), one pull request. That pull request is the first one the new reviewers review. Add a playbook step once it has run for a week, with what it caught.

## 2. Publish the repo

Decision 0005 ties publishing to the first working firmware. That ran on 2026-11-09. The roadmap now says "when the maintainer is happy with its structure", which has no test and can move forever. The vision says "work in the open".

Publishing also gives real branch protection in place of `.githooks/pre-push`, free CI minutes and required review conversations.

Before publishing, check the whole history, not only the current files, for vendor content, secrets, personal data and commit emails. The playbook records that history was rewritten once already for an email.

## 3. Stop adding process until release 0.1.0

Decision 0006 cut the plan back because process had outgrown the firmware. It is growing again:

- The architecture's "later" list says a check on unit version bumps waits until "units start being reused or released on their own". CI already runs that check on every pull request.
- Decision 0007 says a separate version per unit waits until "a unit is reused outside this firmware". Every unit has one now.
- CI checks branch names and pull request titles, for one maintainer.

None of these is wrong. Together they cost time on every change, and nobody outside is using the units yet. Suggested rule: no new process rule or CI gate before 0.1.0, except item 1. Then either remove the version-bump check or update the two "later" lists to say why it came early.

## 4. Bring part of the automation goal into stage 2

The end goal is unattended build, flash, test and judge. Its blockers are the Upload-Cam switch, a test stand and a camera that sees the robot. All three sit in "longer-term tracks" with no order. Stage 2 is instead gated on matching stock features, which teach the least.

Without changing hardware, robot tests can judge more by themselves:

- Use the gyro to confirm which way the robot turned, in place of asking the person.
- Use the ultrasonic against a wall at a known distance to confirm forward and backward.
- Record how many questions each robot test asks a person, and drive that number down.

Then write down, as a notebook entry, what exactly stops a fully unattended run today. That list is the project's most original result, and it should exist early.

## 5. Put a time limit on the Wi-Fi dongle

Decision 0011 calls the dongle temporary, to be deleted in stage 3. It now has its own units, versions, tests and CI build, and two days of work. The notebook entry of 2026-11-16 records hangs and freezes that aren't explained, and the freezes come from the stock camera firmware.

Suggested: agree a limit, such as two more sessions. If floor tests still aren't reliable by then, stop and choose between a long USB cable for the remaining stage 2 tests and starting stage 3 early. Don't polish firmware that is planned for deletion.

## 6. Budget RAM per increment

The build leaves 1,040 bytes of RAM free, and T8's floor is 512. Still to come: four modes, line sensor calibration, more monitors and fault memory. Flash has room; RAM doesn't. Add a row per increment to the requirements or the build report with the RAM each one may use, and check the stack's high-water mark on the robot, since the build report shows only static use.

## 7. Bring the journey and playbook up to date

`docs/journey.md` stops at 2026-11-12, and the playbook at step 16. Since then: three devices, IR driving, heading hold, the telemetry budget, the dongle and its failures. These docs are the raw material for the write-ups the vision promises.

## 8. Don't build ahead of the architecture

The architecture is good and should stay as written. Robot IDs in messages are cheap. The risk is building for robots that don't exist: the drone, MAVLink, and a five-state signal status for a robot with one ultrasonic and one gyro. Keep these as direction. When a unit needs less than the full model, build less and say so in its README.

## Corrections to the first review

The first review, given in chat, criticised a "factory" skill for running several agents. The maintainer isn't using it. It is a personal skill outside this repo, and nothing in the repo depends on it. Ignore that point. Its one good idea, review by the other vendor, is item 1.

## Suggested order

1. Item 1, on its own pull request.
2. Ask the maintainer about items 2, 3 and 5. They are decisions, not work.
3. Items 6 and 7 are small and can ride along with normal work.
4. Item 4 belongs with the next robot tests that get written.
