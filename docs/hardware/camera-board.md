# Camera board

For the ESP32-S3-WROOM-1 version. Sources: its schematic, the stock firmware source and ELEGOO's camera FAQ. The ESP32-WROVER version differs.

## Hardware

- **USB:** USB-C wired directly to GPIO19 and GPIO20, with no USB-serial chip.
- **Link to the shield:** a 4-pin cable carrying RX on GPIO3, TX on GPIO40, ground and 5 V.
- **Camera:** 24-pin DVP ribbon connector. The sensor on our robot is an OV3660, as reported by the maintainer. A retailer guide lists an OV2640, so other units may differ.
- **Other:** green LED on GPIO46, boot and reset buttons. Spare GPIOs reach only module pads and test points, not a header.
- **Build settings ELEGOO specifies:** arduino-esp32 2.0.14, ESP32S3 Dev Module, 8 MB flash, OPI PSRAM, USB CDC on boot.

### 5 V on the UART

**The wiring, from both schematics:** the UNO's TX pin reaches GPIO3 with no resistor, divider or level shifter. The Upload-Cam switch only breaks the UNO's RX line, so TX stays connected in both positions.

**The limit, from the [ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf) v2.2:** GPIO3 is powered from VDD3P3_RTC, and Table 5-4 gives a maximum input high of VDD + 0.3 V. That's 3.6 V on this board's 3.3 V supply. The datasheet makes no 5 V tolerance claim.

**Why it matters:** a UART line idles high, so by our reading the pin sits about 1.4 V over the limit whenever the UNO's serial port is running, not just during short pulses. The stock robot works like this, so it doesn't fail straight away, but that says nothing about how long the pin will last.

**To measure in stage 1:** the voltage on GPIO3 with the UART idle. Well below 5 V would mean a protection diode inside the ESP32 is clamping the pin and carrying current. A fix would be a hardware change, so we'll decide once we've measured it.

## Stock firmware behaviour

| Item | Behaviour |
|---|---|
| Wi-Fi | Access point named `ELEGOO-` plus a chip ID, on channel 9, with **no password** |
| Address | 192.168.4.1 |
| Port 80 | Web page, `/control`, `/status`, `/capture`, and `/Test`, which is also a video stream |
| Port 81 | `/stream` video |
| Port 100 | TCP bridge forwarding `{...}` frames to the UNO at 9600 baud, with no authentication |
| Heartbeat | Exchanges `{Heartbeat}` with the client each second, and drops the client after more than 3 missed |
| On disconnect | Sends `{"N":100}` to the UNO, which stops the robot |

Anyone in range can therefore join the network and drive the robot. A computer joined to this network also leaves its normal Wi-Fi, which matters if it needs the internet at the same time.

The firmware also sends factory-test frames to the UNO, such as `{Factory}`, `{WA_NO}` and `{WA_OK}`. The UNO has to ignore them.

## Working with the stock bridge

Until stage 3, our UNO firmware talks to the outside world through this stock firmware, which limits our protocol in a few ways:

- Messages must be text inside `{` and `}`, at 9600 baud. The bridge removes spaces and forwards everything else unchanged.
- The client on port 100 has to answer `{Heartbeat}`, or the bridge drops it.
- The UNO has to treat `{"N":100}` as stop, because that's what the bridge sends when a client disconnects.
- Only port 100 reaches the UNO. The web server on port 80 handles the camera only, and a browser can't open a raw TCP connection, so a web page can't drive the robot through this firmware.

Video needs nothing from us, because a browser can show the stream on port 81 directly.
