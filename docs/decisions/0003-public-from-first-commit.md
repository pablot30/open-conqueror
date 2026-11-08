# 0003: Public from the first commit, under Apache-2.0

**Date:** 2026-11-03. **Status:** accepted, except that [0005](0005-publish-with-first-working-firmware.md) moves publishing to when our first firmware works.

## Context

Publishing what we learn is part of the goal. Working in public from the start also keeps licensing and secrets clean, because there's no private history to clean up later.

## Decision

- The repository is public from its first commit, and that commit contains documentation only.
- Everything in it is under Apache-2.0. We can add separate licenses if hardware designs arrive.
- ELEGOO's code, manuals and firmware never go in the repo, because the kit comes with no license to redistribute them. Facts about the hardware go in our own words.

## Consequences

- Factory firmware backups stay in a private location, outside the repo.
- Wi-Fi credentials and other secrets stay out of the repo.
- Commits use a GitHub noreply address rather than a personal email.
