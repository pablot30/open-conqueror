# Test reports

One report for each increment, written when its robot tests pass ([0007](../decisions/0007-testing-releases-and-traceability.md)). Each names the exact firmware commit, links the CI run that built it, maps every requirement to its result, and keeps the raw logs. `python scripts/dev.py trace --increment N` writes the requirements table from the tests and reports; check it against what the report says before pasting it in.

`conq test` writes each run to `build/robot-tests/<time>/` in the same layout as a report: `report.json`, with the logs in `logs/`. Its paths are relative to `report.json` and it writes no absolute paths, so a run can be copied in as it is. When a report gathers several runs, merge their `reports` lists into one `report.json`. Check the logs for personal data before committing them.

| Report | Firmware | Result |
|---|---|---|
| [Increment 1, the walking skeleton](2026-11-12-increment-1/README.md) | `0.1.0-dev`, `64e636d` | Passed |
