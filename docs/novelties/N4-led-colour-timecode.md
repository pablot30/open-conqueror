# N4: The status LED as a colour timecode for syncing video

**Status:** idea. **First written:** 2026-11-09, in research report R8 ([dashboards and replay](../research/2026-11-09-dashboards-and-replay.md)). **Published here:** 2026-11-16.

## The problem

Phone videos of the robot are the easiest record of what it did, but their clock doesn't match the robot's recordings. Lining them up by hand gives one offset, and the two clocks drift apart during a long run.

## The idea

The robot's RGB status LED flashes a short colour sequence at intervals, encoding the recording's clock. The replay tool finds those flashes in the video and works out both the offset and the drift, with no manual step. Every recording and every video then share one timeline.

## Building blocks, all public

Visual timecodes and sync flashes are long established in film and motion capture. What's particular here is using a robot's single status LED and its link's time sync together.

## What we searched

No prior-art search was done. This is probably close to established practice, and it's published here so that it stays free to use.
