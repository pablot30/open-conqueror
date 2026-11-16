# ir

**Layer:** devices. **Version:** in `library.properties`. **First run on the robot on 2026-11-15:** our remote's codes were recorded, and the arrows drove the robot with its tracks lifted.

The IR remote: the receiver on D9, a decoder for NEC-style frames, the table of our remote's keys, and the logic that turns the arrow keys into driving. Arrows drive while held and stop 300 ms after the last code. OK stops, and the stop holds. Mode keys wait for increment 3.

| | |
|---|---|
| **Requirements** | F10 (arrows and OK only), S2 (OK stops without Wi-Fi), S3 (the 300 ms release), S5 (a stop is only undone by a deliberate command), T2 |
| **Interface** | `decodeNec(widthsUs, count)`; `keyFor(code)` and `lookupKey(table, n, code)`; `RemoteDriver`: `onKey(key, nowMs)`, `onRepeat(nowMs)`, `update(nowMs)` returning a `RemoteRequest`; `ir_hw::begin()`, `ir_hw::takeFrame(widths, count)` in `ir_hw.cpp` |
| **Unit tests** | `tests/test_ir.cpp`: the decoder on recorded-style timing lists, the key table, and driving with the safety supervisor (`tests/uses.txt`). `_hw.cpp` only builds for the board |
| **Monitors** | None. A remote nobody presses isn't a fault. Frames the decoder can't read are reported in the `{K,...}` message, so a remote that speaks another protocol shows up |
| **Robot tests** | `tests/robot/ir_keys.py` records the codes; `tests/robot/ir_drive.py` drives with them |

## Our remote's codes

The key table is in one marked place, `src/ir_keys.cpp`. It holds the codes of the remote that came with our kit, recorded on the robot on 2026-11-15; the mode keys 1 to 6 are recorded there too, for increment 3. A key not in the table does nothing. To record another remote, with the robot on a table and the Upload-Cam switch on Upload:

```
conq ir record            # asks for each key and prints the lines to paste
conq ir listen            # or: print every code for 10 seconds
conq test ir_keys          # the same, as a robot test
```

Paste the printed lines into `ir_keys.cpp`, flash, then run `conq test ir_drive`.

## How it works

- **Timing.** The receiver's edges are timed with `micros()` in the shared pin-change interrupt ([`hal/pinchange`](../../hal/pinchange/README.md)), which D9 shares with the ultrasonic echo. The interrupt only stores widths. Once the line has been quiet for 6 ms the frame is complete, and the main loop copies it out and decodes it, so no frame waits for a key.
- **Decoder.** `decodeNec` is a pure function from pulse widths to a code, with wide limits because receiver modules stretch bursts. It checks that the command and its inverse agree, accepts the classic and extended address forms, and recognises the repeat code. The protocol is NEC's, from public descriptions (see the header); our remote's frames decode with it.
- **Driving.** Each code from a held arrow refreshes the drive; 300 ms after the last one, `RemoteDriver` asks for a stop. The release goes through `safety.stop()`, not the link timeout, so letting go isn't logged as a link fault. A key pressed afresh sends a neutral command first, which arms the supervisor, so a deliberate press can drive after a stop. **Repeat codes never arm:** after OK, a release, or another source's stop (the MODE button), a key still held can't restart the robot. A quick re-press within 300 ms of the last code counts as the same hold and doesn't restart a stopped robot either; let go for a third of a second first.
- **Speed.** Arrows drive at 40% (`config::kRemoteSpeedPercent`), slow for a first test on a table.
- **Receiver polarity.** Receiver modules of the usual kind hold the output high when idle and low during a burst. Ours decodes that way, so it matches. A module the other way round would give unreadable frames.

Test this unit on its own with `python scripts/dev.py test ir`.
