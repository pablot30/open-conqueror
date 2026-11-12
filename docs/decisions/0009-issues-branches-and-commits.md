# 0009: Issues, branch names and commit messages

**Date:** 2026-11-13. **Status:** accepted.

## Context

Branch names like `increment2/loop-and-diagnostics` said when a change was made, but not what kind of change it was or why. Nothing tied a change to a reason except the pull request's description. The maintainer asked for a standard, with ticket numbers to keep track of why we do things.

## Decision

- **Every piece of work has a GitHub issue,** saying why it's needed, which requirement IDs it serves and when it's done. Issues belong to a milestone per stage. The label `needs-robot` marks work that waits for a session with the robot.
- **Branch names are `<type>/<issue>-<short-description>`,** in lowercase with hyphens: `feat/12-frame-and-diagnostics`, `fix/20-stop-not-held`, `docs/16-branch-naming`. The types follow [Conventional Commits](https://www.conventionalcommits.org/):

  | Type | For |
  |---|---|
  | `feat` | New behaviour |
  | `fix` | A bug fix |
  | `docs` | Docs only |
  | `refactor` | Restructuring with no change in behaviour |
  | `test` | Tests only |
  | `ci` | CI and build |
  | `chore` | Housekeeping |

  `release/<major>.<minor>` branches exist only if an old release needs fixes.
- **Pull requests close their issue** with `Closes #<number>` in the description, and their titles follow Conventional Commits, such as `feat(gyro): calibrate at power-on`.
- **Commit messages follow Conventional Commits:** `<type>(<scope>): <summary>`, where the scope is usually a unit, plus requirement IDs where they apply. Example: `feat(gyro): calibrate at power-on (F13, T6)`.
- **GitHub flow:** short branches off `main`, merged through pull requests. No `develop` branch. Git-flow's extra branches suit several releases maintained in parallel, which we don't have.
- **Checks:** the pre-push hook rejects branch names that don't fit, and CI checks each pull request's branch name, title and linked issue.

## Consequences

- Every change traces back to a reason: issue, branch, commits, pull request and, at the end of an increment, its test report.
- Changelogs can be generated from commit messages when releases start (0007).
- Renaming a branch on GitHub closes the pull requests opened from it, so branches are named correctly from the start. We learned this when renaming the first two branches: #10 and #11 were replaced by #17 and #18.
- Commits made before this decision keep their plain-English messages.
