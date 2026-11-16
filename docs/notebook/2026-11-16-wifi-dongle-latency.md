# The Wi-Fi dongle's first runs: latency, and a hang

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

## The hang

After the last good run, the dongle sat for about 15 minutes with no program on the computer reading its port. Then it sent nothing at all, even its own status, and an upload failed with `No device found` after the 1200-baud reset. On the R4 WiFi, that reset is done by the ESP32-S3, so the USB chip itself seems stuck, though the board was still listed. Unplugging the R4 should restart both chips; not yet tried.

We don't know the cause. Two ideas to test, one at a time: the ESP32-S3 stops taking serial data while no program has the port open, so the dongle's writes to USB block; or the steady stream of commands to the Wi-Fi chip wears it down over time. The first version of the dongle survived a similar idle period earlier the same day, with a similar number of Wi-Fi commands a second.
