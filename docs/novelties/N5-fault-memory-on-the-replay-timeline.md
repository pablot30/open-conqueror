# N5: Fault memory on the replay timeline, all from one description file

**Status:** idea. **First written:** 2026-11-09, in research report R8 ([dashboards and replay](../research/2026-11-09-dashboards-and-replay.md)). **Published here:** 2026-11-16.

## The problem

When a robot run goes wrong, the questions are what failed, when, and what the sensors read at that moment. Robotics replay tools show signals on a timeline. Automotive diagnostics keep coded faults with status changes and snapshots. The two rarely meet, and each tool needs its own hand-written configuration.

## The idea

One description file, the same one that defines the link protocol ([R2](../research/2026-11-08-robot-link-protocol-and-diagnostics.md)), also generates the evaluation tools: the calibration editor's fields and limits, the fault panel's codes and snapshot layout, the timeline's event markers and the viewer layouts. Each fault's status changes, in the style of ISO 14229, are drawn on the replay timeline next to the snapshot taken at that moment.

## Building blocks, all public

- Parameter screens generated from metadata, as in QGroundControl, and telemetry dictionaries are established.
- ISO 14229 (UDS) defines fault status bits and snapshots.
- Generating code from one protocol description is established, and is already the plan in report R2.

## What we searched

The sources read for report R8, on 2026-11-09. This combination wasn't found in them. No dedicated prior-art search was done, and patent databases weren't searched.
