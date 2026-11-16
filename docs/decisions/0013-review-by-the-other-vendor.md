# 0013: Every pull request is reviewed by the other vendor's model

**Date:** 2026-11-16. **Status:** accepted.

## Context

None of the first 20 merged pull requests had a review. One was 1,876 lines and merged six minutes after it opened. The roadmap said "the maintainer reviews and merges", but only the merging happened. Unit tests, CI and robot tests were the only real gates. One agent, Claude, writes nearly all the code, so one model's blind spots go straight to `main`. A project review on 2026-11-16 ([item 1](../research/2026-11-16-project-review.md)) proposed fixing this, and the maintainer agreed.

## Decision

- **A model from the other vendor reviews every pull request.** Claude writes most of the code, so Codex reviews. When Codex writes the code, Claude reviews.
- **Codex reviews automatically,** through its GitHub integration: the maintainer connects the repo in Codex's settings and turns on automatic review there. It posts a standard GitHub review and flags only serious problems (P0 and P1). A comment `@codex review` asks for one by hand. No workflow file or API key is needed. Source: OpenAI's page "Review GitHub pull requests with Codex", read on 2026-11-16.
- **The review rules live in `AGENTS.md`,** under `## Code Review Rules`, which Codex follows. They name the checks only this project has: the safety supervisor, the 10 ms frame, held stops, T8, traceable tests and hardware facts.
- **The author answers every comment,** with a commit or a written reason, and resolves it. No silent dismissals.
- **Two rounds at most.** If reviewer and author still disagree after two rounds, the author stops and asks the maintainer.
- **The maintainer reads the review summary and the open points, then merges.** That's the gate, stated honestly: the maintainer doesn't read every diff.
- **Pull requests aim for under about 400 changed lines.** A guideline, not a CI check: larger ones can't be reviewed well, by a model or a person.
- **Codex alone for the first week.** Then we count how many of its findings were real, and decide whether a fresh Claude session should review as well, through Anthropic's GitHub Action.

## Consequences

- Every pull request gets a second look at no cost to the maintainer's time. It isn't proof: a model can't see the robot, so robot tests stay the gate for behaviour.
- Until the repo is public, GitHub can't require resolved conversations before merging on the free plan, so the loop is a convention, like the rest of [0009](0009-issues-branches-and-commits.md).
- After the first week, the playbook gets a step with what the reviews caught.
- Still to confirm when it's switched on: which ChatGPT plans include Codex review and with what limits, whether it reviews private repos, and whether it reviews again after each push. OpenAI's page doesn't say.
