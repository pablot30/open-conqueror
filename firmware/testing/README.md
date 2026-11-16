# Unit test harness

Runs every unit's tests on a computer, with nothing beyond a C++ compiler: `python scripts/dev.py test`. Each unit's `tests/` defines `runUnitTests()`, which calls its test functions, and `CHECK(condition)` from `check.h` counts and reports failures.

## Naming the requirements a test checks

Put the requirement IDs in a comment on the line where the test function starts, separated by commas:

```cpp
void ignoresDrivingBeforeNeutral() {  // S4, S5: a reset mid-drive stays stopped
```

- Every requirement ID in that comment counts. Text after the IDs is a note.
- Architecture sections, such as A5, may be named too. They aren't requirements and aren't traced.
- A test that checks no requirement has no IDs. That's fine: many tests check a detail.
- `python scripts/dev.py trace` reads these comments and reports any ID that isn't in [requirements](../../docs/requirements.md).
