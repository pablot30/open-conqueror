# relay

**Layer:** services. **Version:** in `library.properties`.

The Wi-Fi dongle's logic, with no hardware in it. `dongle.ino` connects it to USB and Wi-Fi.

| | |
|---|---|
| **Requirements** | F11 (driving over Wi-Fi), S3 (a dead link is noticed and reopened; the robot's own timeout stops it) |
| **Interface** | `MessageReader::push()`, `routeFromComputer()`, `routeFromRobot()`, `pickNetwork()`, `LinkWatch` |
| **Unit tests** | `tests/test_relay.cpp` |

Test this unit on its own with `python scripts/dev.py test relay`.
