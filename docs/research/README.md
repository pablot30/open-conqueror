# Research

Reports written before big decisions. Each one gathers sources, states what's uncertain, and ends with the decisions it informs. The decisions themselves go in [decisions](../decisions/README.md).

All eight were written on 8 and 9 November 2026, while stage 2 planning was at its largest. [Decision 0006](../decisions/0006-walking-skeleton-first.md) then cut stage 2 back to a walking skeleton, so read them as background: where a decision or the [architecture](../architecture.md) says otherwise, those win.

| Report | Topic | Where it's used |
|---|---|---|
| [R1](2026-11-08-code-quality-and-static-analysis.md) | Code quality: coding standard, static analysis, coverage including MC/DC, CI costs, traceability | Warnings as errors, cppcheck and CI from [0006](../decisions/0006-walking-skeleton-first.md); traceability in [0007](../decisions/0007-testing-releases-and-traceability.md) |
| [R2](2026-11-08-robot-link-protocol-and-diagnostics.md) | The link: protocol, description files, telemetry, recordings, calibration, diagnostics | Background for the protocol in increment 4 and the health model in the [architecture](../architecture.md) |
| R3 | Security and updates: bootloader, wireless flashing, signed firmware | **Not published yet.** It contains ideas that might be patentable, to be discussed first. Needed in stage 3 |
| [R4](2026-11-09-work-tracking-and-planning.md) | Work tracking and planning tools | We use GitHub Issues and pull requests |
| [R5](2026-11-09-embedded-toolchain-and-build.md) | Toolchain and build for both boards and the host computer | arduino-cli with a pinned core, CMake for unit tests; the ESP32 choice waits for stage 3 |
| [R6](2026-11-09-embedded-firmware-architecture.md) | Firmware architecture: layers, scheduling, safety, versions, system split | Cut down into the [architecture](../architecture.md) |
| R7 | Releases and the software factory | **Not published yet,** because its release flow depends on an idea in R3. [0007](../decisions/0007-testing-releases-and-traceability.md) covers releases for now |
| R8 | Dashboards and replay tools | **Not published yet.** Two of its ideas need the same check as R3's. Needed in increment 4 |

R3, R7 and R8 are kept privately until the maintainer decides on their possibly novel ideas, because the repo's history will become public.
