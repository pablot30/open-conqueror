# The Wi-Fi dongle's first runs: latency, freezes and hangs

**Date:** 2026-11-16. **How we know:** measured on our robot (UNO firmware `bf9a708`, protocol 8, stock camera firmware), standing still with the Upload-Cam switch on Cam, through our dongle firmware on a UNO R4 WiFi about 1 m away, signal -40 dBm. Scripts on the computer sent only `{V}` (version), `{S}` (stop) and `{W}`, so nothing moved. The camera board's behaviour was read in the stock source, not measured.

## Results

| Build | Load | Round trip `{V}` | Telemetry | Lost |
|---|---|---|---|---|
| First dongle | `{V}` every 0.35 s | median 295 ms, max 304 | bunched, gaps up to 254 ms | none |
| With a filler byte every 20 ms | same | replies late or missing | gaps up to 1.5 s | 18 of 40 |
| Fewer Wi-Fi chip commands per loop | `{V}` every 0.15 s | median 92 ms, max 255 | gaps up to 261 ms | none |
| Same | 10 writes a second, like driving, for 60 s | median 73 ms, max 131 | all 600, gaps up to 196 ms | none |

## What we learned

- **The R4's USB socket goes through its ESP32-S3,** which converts USB to serial for the main chip. The baud rate the computer opens the port at therefore matters: at 9600 we got only garbage from a sketch running at 115200.
- **Every WiFiClient call is a text command to the Wi-Fi chip,** about 5 ms each, and a read can take 45 ms. `connected()` costs two commands, and `available()` before `read()` one more. The first loop made about three per pass and ran 56 times a second. Reading straight away and checking `connected()` once a second brought it to about 180, and the round trip from 295 ms to about 90 ms.
- **The camera board sends each robot message as its own small write,** with the ESP32 Arduino defaults, which leave Nagle's algorithm on (read in the stock source). Telemetry still arrives in bunches up to about 0.25 s apart when the computer sends nothing, which fits Nagle's algorithm waiting on delayed acknowledgements. When the computer sends 10 messages a second, as it does while driving, the largest gap fell to 196 ms, under S3's 300 ms.
- **A filler byte to hurry those acknowledgements made things worse:** replies went missing and one run counted more telemetry than was sent. We don't know why, so the dongle doesn't use one.
- **The UNO drops a message when its send buffer is full** (`uno.ino`), by design. Asking for `{V}` 10 times a second on top of telemetry fills the 9600-baud line to about 80%, and 11% of messages were dropped. That's our test overloading the robot, not the link.

## Freezes from the camera board

In the afternoon, the same dongle firmware gave link freezes of 1.2 to 1.6 s, 13 to 19 a minute, at the driving rate. Every message still arrived, but late. During each freeze the dongle kept sending its own status messages and its loop never stopped for more than 153 ms, so the stall was between the dongle and the camera board, not in the dongle or its USB side. Signal was steady at -37 to -48 dBm, and moving the R4 made no difference.

A `stops` test run during this period, tracks lifted, failed three checks. A stop command was held up in a freeze, and the robot stopped through its 300 ms link timeout instead, 0.3 to 0.4 s later than asked. Drive commands held up in another freeze arrived after their stop had been sent, so the robot did that drive about 0.4 s late. In both cases the timeout bounded the overrun: a freeze longer than 0.3 s stops the robot, and the stop holds until a new neutral command.

**Restarting the robot cleared the freezes:** 0 in the next 60 s, longest gap 195 ms. The camera board had been running for hours, and earlier the dongle had hung while connected to it. We don't know which of those caused the freezes, or whether they come back. Until we do, restart the robot before a session over Wi-Fi.

## Hangs

The R4 hung twice. It sent nothing at all, even its own status, and uploads failed with `No device found` after the 1200-baud reset. On the R4 WiFi that reset is done by the ESP32-S3, so the Wi-Fi chip itself seems stuck, though the board stayed listed. Unplugging the R4 and plugging it back in brought it back both times.

- The first time, the robot's network may have dropped while nobody was watching. The dongle then sat for about 15 minutes before we noticed.
- The second time, it was straight after the robot was switched off and on, which drops the robot's network.

Arduino's forum has several reports of the R4 WiFi losing Wi-Fi and staying off until it's power-cycled, and of scans hanging it. Our first reconnect logic, after a lost link, checked the Wi-Fi state on every pass of the loop, scanned again after any failed join, and tried again every second. The dongle firmware now rejoins the network it already knows without scanning. It spaces attempts 1 to 5 s apart and scans only after three failed joins. Whether that stops the hangs is to be tested by switching the robot off and on.
