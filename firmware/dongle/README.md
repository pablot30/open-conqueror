# Wi-Fi dongle

**Parked, 2026-11-16: it hangs.** Three times on its first day the R4's Wi-Fi chip stopped completely, once with nothing changing, and only unplugging it helped. Floor tests use a long USB cable instead. See the [notebook](../../docs/notebook/2026-11-16-wifi-dongle-latency.md) and the update in [decision 0011](../../docs/decisions/0011-wifi-dongle.md).

Firmware for an Arduino UNO R4 WiFi that stays plugged into the computer by USB and links it to the robot over Wi-Fi. The robot's 20 cm USB cable rules out every floor test; with the dongle, `conq` and the robot tests run unchanged with the robot driving free ([#34](https://github.com/pablot30/open-conqueror/issues/34), [decision 0011](../../docs/decisions/0011-wifi-dongle.md)).

The dongle is a tool on the computer's side. Nothing on the robot changes: it keeps the stock camera firmware, which already bridges Wi-Fi to the UNO ([camera board](../../docs/hardware/camera-board.md)).

```
computer ──USB──> UNO R4 WiFi ──Wi-Fi──> camera board, port 100 ──9600 baud──> robot's UNO
```

The computer stays on its own Wi-Fi.

**Open the port at 115200 baud.** The R4's USB socket goes through its ESP32-S3, which converts USB to serial for the main chip, so the rate matters: at the robot's 9600 you only get garbage. Measured on 2026-11-16.

## What it does

1. Scans for networks whose names start with `ELEGOO-` and joins the strongest. The stock network is open, so there's no password to store, and the robot's name stays out of the repo.
2. Opens TCP port 100 on 192.168.4.1, the camera board.
3. Passes `{...}` messages both ways. Bytes outside braces are dropped, as the UNO would drop them.
4. Answers the camera board's `{Heartbeat}` each second and keeps it off the USB side.
5. If the camera board goes quiet for 3.5 s, or the connection or network drops, it starts again from the step that failed.

While it isn't connected, messages from the computer are dropped. The robot's 300 ms drive timeout (S3) stops it, and when the dongle's connection closes, the camera board sends the UNO a stop (`{"N":100}`).

## Messages

Messages for the dongle itself start with `W`, so they never collide with the robot's. The dongle answers them and doesn't pass them on.

| Direction | Message | Meaning |
|---|---|---|
| To the dongle | `{W}` | Ask for the link state now |
| To the dongle | `{WV}` | Ask for the version |
| From the dongle | `{W,state,rssi}` | Link state, once a second and on every change. `state`: 0 looking for the robot's network, 1 joining it, 2 opening port 100, 3 connected. `rssi`: signal strength in dBm, 0 when not on the network |
| From the dongle | `{WV,version,commit,wifi}` | Firmware version and git commit at startup and on request, and the version of the R4's own Wi-Fi chip firmware |

## Build and flash

Plug the R4 into the computer by USB; it isn't connected to the robot. From the repo root:

```
python scripts/dev.py build --target dongle
python scripts/dev.py upload --target dongle      # add --port if more than one board is plugged in
```

`python scripts/dev.py setup` installs the `arduino:renesas_uno` core first, once ([tools.md](../../docs/tools.md)). If the upload fails, double-press the R4's reset button to put it in its bootloader and try again.

## Limits

- **Two robots in range:** it joins the stronger one. Choosing a robot by name comes when someone needs it.
- **The R4's Wi-Fi is a separate chip,** an ESP32-S3 driven over a serial line with text commands, so every read costs a few milliseconds.
- **Round trips take about 0.07 to 0.1 s.** While driving, with 10 messages a second, messages from the robot were never more than 0.2 s apart, under the robot's 0.3 s stop timeout (S3). Measured on 2026-11-16; see the [notebook](../../docs/notebook/2026-11-16-wifi-dongle-latency.md).
- **Link freezes of about 1.5 s** came from the camera board after hours of running, and went away when the robot was restarted. Restart the robot before a session over Wi-Fi.
- **It has hung twice,** once straight after the robot was switched off and on: unplug it and plug it back in. The reconnect logic has changed since; see the [notebook](../../docs/notebook/2026-11-16-wifi-dongle-latency.md).
- The robot's network has no password, so anyone in range can join it. That's the stock firmware's behaviour, and it changes with our own camera firmware in stage 3.

## Layout

| Folder | Contents |
|---|---|
| `dongle.ino` | Connects USB, Wi-Fi and the relay unit |
| `services/relay/` | Message framing, heartbeat, network choice and link timeout, with unit tests |
| `src/version.h` | The version and commit, written in by `dev.py` |
