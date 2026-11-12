# diagnostics

**Layer:** services. **Version:** in `library.properties`. **Not run on the robot yet.**

Diagnostics, first iteration ([architecture](../../../../docs/architecture.md), A5). Units' monitors report passed or failed; this unit debounces the reports into diagnostic events (not tested, prefailed, failed, prepassed, passed), applies reactions, and gives every sensor value a signal status (valid, init, substitute, error, not available).

| | |
|---|---|
| **Requirements** | A5. Reactions protect S9: without a plausible battery reading the robot won't drive |
| **Interface** | `Diagnostics::report(id, result)`, `status(id)`, `inhibited(reaction)`; `signalStatusFor(status)`; `qualify(measured, status, lastValid, safeDefault)`; the event list in `diagnostics.h` and its table in `diagnostics.cpp` |
| **Unit tests** | `tests/test_diagnostics.cpp` |
| **Monitors** | It has no monitors itself; it receives theirs. Events today: battery sensor, frame overrun, link timeout, gyro comms |

Test this unit on its own with `python scripts/dev.py test diagnostics`.

## How it works

| Idea | Public source |
|---|---|
| Units run *monitors* that report passed or failed | OBD-II monitors (SAE J1979); diagnostic monitors in AUTOSAR Classic |
| Event states *not tested, prefailed, failed, prepassed, passed*, and counter-based debouncing that resets the counter when the direction changes | The Diagnostic Event Manager specification of AUTOSAR Classic |
| Reactions that switch functions off while a fault is confirmed | AUTOSAR Classic's Function Inhibition Manager |
| Signal statuses *valid, init, substitute, error, not available* | Value quality in OPC UA (IEC 62541) and fieldbus device profiles; init and substitution values in AUTOSAR COM; "error" and "not available" values in SAE J1939 |

An event stays *not tested*, so its signal stays *init*, until enough passes confirm it. A confirmed fault goes back through *prepassed* before it's *passed* again. Its reactions stay on, and its signal holds the last valid value as a substitute, until then. If it fails again while healing, it's confirmed again straight away.

To add an event: add it to `EventId` before `kCount`, add its row to the table in `diagnostics.cpp`, and report it from the unit that monitors it. The health message lists events in `EventId` order.
