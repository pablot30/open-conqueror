# 0002: The first goal includes the phone app and camera

**Date:** 2026-11-03. **Status:** replaced by [0004](0004-own-protocol-and-control-page.md).

## Context

The app and camera are a large part of what the robot does out of the box. Leaving them out would make the first version simpler, but the robot would lose video and joystick control until they came back.

## Decision

Include them, in two steps: replace the UNO firmware first while the stock camera firmware and app keep working, then replace the camera firmware.

## Consequences

- The stock app is a ready-made test for our UNO firmware.
- Our UNO firmware has to speak the app's JSON protocol at 9600 baud, at least until stage 3.
- ELEGOO's protocol PDF is out of date, so we work from the firmware's behaviour.
- Our camera firmware must stay compatible with the app, so it can't change the network or ports freely.
- The UNO can't have a general communication timeout until stage 3, because the stock camera firmware doesn't send it a heartbeat.
