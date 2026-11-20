# Test reports

One report for each increment, written when its robot tests pass ([0007](../decisions/0007-testing-releases-and-traceability.md)). Each names the exact firmware commit, links the CI run that built it, maps every requirement to its result, and keeps the raw logs. `python scripts/dev.py trace --increment N` writes the requirements table from the tests and reports; check it against what the report says before pasting it in.

| Report | Firmware | Result |
|---|---|---|
| [Increment 1, the walking skeleton](2026-11-12-increment-1/README.md) | `0.1.0-dev`, `64e636d` | Passed |
| [Increment 2, sensors and status](2026-11-20-increment-2/README.md) | `0.1.0-dev`, `bf9a708` | Passed, T3 by judgement; F15 not covered |
