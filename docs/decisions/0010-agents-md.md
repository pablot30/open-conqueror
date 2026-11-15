# 0010: The agent rules live in AGENTS.md

**Date:** 2026-11-15. **Status:** accepted.

## Context

The project's rules for agents were in `CLAUDE.md`, which only Claude Code reads. The maintainer now also works with Codex, which reads `AGENTS.md`. Keeping the same rules in two files means they drift apart.

`AGENTS.md` is the cross-tool format: Codex, Gemini CLI, Cursor, GitHub Copilot's coding agent and others read it. Claude Code reads it only when there is no `CLAUDE.md` or `CLAUDE.local.md`. A `CLAUDE.md` can import it with `@AGENTS.md` instead. The [architecture research](../research/2026-11-09-embedded-firmware-architecture.md#agentsmd-at-the-root-and-in-every-unit) recommended this, as question 13.

## Decision

- `AGENTS.md` at the repo root holds the rules for every agent and contributor.
- `CLAUDE.md` contains only `@AGENTS.md`. Claude Code needs the import because the maintainer keeps a private `CLAUDE.local.md`, and without it Claude Code would skip `AGENTS.md`.
- Rules that only one tool needs go in that tool's own file, below the import, and only if they come up.

## Consequences

- Rules are edited in one place, and any agent that supports `AGENTS.md` follows them.
- `CLAUDE.local.md` is still read only by Claude Code. Agents launched for the maintainer are told to read it too.
- The research also suggested an `AGENTS.md` in every unit. That waits until a unit needs rules of its own (0006).
