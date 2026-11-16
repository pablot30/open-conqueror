# Research

Reports written before big decisions. Each one gathers sources, states what's uncertain, and ends with the decisions it informs. The decisions themselves go in [decisions](../decisions/README.md).

The eight R reports were written on 8 and 9 November 2026, while stage 2 planning was at its largest. [Decision 0006](../decisions/0006-walking-skeleton-first.md) then cut stage 2 back to a walking skeleton, so read them as background: where a decision or the [architecture](../architecture.md) says otherwise, those win.

| Report | Topic | Where it's used |
|---|---|---|
| [R1](2026-11-08-code-quality-and-static-analysis.md) | Code quality: coding standard, static analysis, coverage including MC/DC, CI costs, traceability | Warnings as errors, cppcheck and CI from [0006](../decisions/0006-walking-skeleton-first.md); traceability in [0007](../decisions/0007-testing-releases-and-traceability.md) |
| [R2](2026-11-08-robot-link-protocol-and-diagnostics.md) | The link: protocol, description files, telemetry, recordings, calibration, diagnostics | Background for the protocol in increment 4 and the health model in the [architecture](../architecture.md) |
| [R3](2026-11-08-security-and-wireless-updates.md) | Security and updates: bootloader, wireless flashing, signed firmware | Needed in stage 3. Its three new ideas are novelties [N1 to N3](../novelties/README.md) |
| [R4](2026-11-09-work-tracking-and-planning.md) | Work tracking and planning tools | We use GitHub Issues and pull requests |
| [R5](2026-11-09-embedded-toolchain-and-build.md) | Toolchain and build for both boards and the host computer | arduino-cli with a pinned core, CMake for unit tests; the ESP32 choice waits for stage 3 |
| [R6](2026-11-09-embedded-firmware-architecture.md) | Firmware architecture: layers, scheduling, safety, versions, system split | Cut down into the [architecture](../architecture.md) |
| [R7](2026-11-09-releases-and-software-factory.md) | Releases and the software factory | [0007](../decisions/0007-testing-releases-and-traceability.md) and [0012](../decisions/0012-versions-and-a-stable-main.md) cover releases for now, and differ from it on versions |
| [R8](2026-11-09-dashboards-and-replay.md) | Dashboards and replay tools | Needed in increment 4. Its two new ideas are novelties [N4 and N5](../novelties/README.md) |
| [Review](2026-11-16-project-review.md) | A review of the project and its progress after increment 2's first parts, by Claude Fable 5.2 at the maintainer's request | Automatic pull request review in [0013](../decisions/0013-review-by-the-other-vendor.md); the other items wait for the maintainer |
| [Handover](2026-11-16-docs-check-and-traceability-handover.md) | A plan for a regular docs check and a requirement-to-result trace command, by Claude Fable 5.2 | `dev.py trace` ([#51](https://github.com/pablot30/open-conqueror/issues/51)); the docs check is [#52](https://github.com/pablot30/open-conqueror/issues/52) |

R3, R7 and R8 were kept out of the repo at first, because of ideas that might have been patentable. On 2026-11-16 the maintainer decided to publish those ideas openly, as [novelties](../novelties/README.md).
