# 0004: Our own protocol and control page, not the stock app

**Date:** 2026-11-04. **Status:** accepted. Replaces [0002](0002-include-app-and-camera.md).

## Context

0002 kept ELEGOO's EleRobot app working, so that it could serve as a ready-made test for our UNO firmware. That would tie our firmware to a protocol we didn't design, for an app we plan to replace anyway.

## Decision

- Our firmware doesn't support the EleRobot app or its commands. We design our own protocol in stage 2.
- Video and phone control stay in the first goal, through our own web page. It runs in a browser, so it works on phones and computers with no app store.
- The UNO firmware is still replaced first, while the stock camera firmware keeps running. The camera firmware follows in stage 3.
- Stage 2 uses a small computer tool to control and test the UNO. A web page can't do this yet, because the only route to the UNO through the stock camera firmware is raw TCP on port 100, which browsers can't open. The page arrives in stage 3, served by our camera firmware.
- The app's graphical programming and DIY mode are dropped.

## Consequences

- There's no phone control during stage 2, only the computer tool, the IR remote and the MODE button.
- We have to write the tool ourselves, but automated testing needs something like it anyway.
- Until stage 3, our protocol has to [fit the stock camera firmware's bridge](../hardware/camera-board.md#working-with-the-stock-bridge).
- The UNO can stop by itself when the link goes quiet from stage 2, a stage earlier than 0002 allowed, because our tool can send regular messages through the bridge.
- Our camera firmware is free to change the network and ports.
- The stock protocol stays documented, because stage 1 uses it to query the stock firmware.
- Restoring the firmware backup brings the stock app back.
