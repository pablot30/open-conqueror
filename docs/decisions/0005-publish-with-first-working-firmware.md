# 0005: Publish once our first firmware works on the robot

**Date:** 2026-11-08. **Status:** accepted. Replaces the "public from the first commit" part of [0003](0003-public-from-first-commit.md).

## Context

0003 planned to publish the documentation before any code. The maintainer would rather publish once there is something that runs, so the first thing visitors see is working firmware, not only plans.

## Decision

- The repository stays private until our own firmware runs on the robot and does something we've checked.
- Everything else in 0003 still holds: Apache-2.0, no ELEGOO files, no secrets, and commits under a GitHub noreply address.

## Consequences

- Publishing makes the whole history public, not just the latest files. Commits made before then must meet the same standard as public ones.
- Firmware backups stay private, as before.
- Stage 0 ends once the docs are agreed. Publishing moves to stage 2.

## Update, 2026-11-11

Our firmware first ran on the robot on 2026-11-09. The maintainer chose to keep the repo private a little longer, until they're happy with its structure, so publishing is now their call rather than tied to a milestone.
