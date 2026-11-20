# Battery reading and ultrasonic, against a multimeter and a tape

**Date:** 2026-11-20. **How we know:** measured on our robot running our firmware (0.1.0-dev, commit `bf9a708`), read over the robot's own Wi-Fi. The maintainer held a multimeter's probes on the robot while a script averaged the robot's own readings over the same 5 seconds. Distances were read off a tape measure by eye. Nothing moved during either test.

## Battery

| Where | Multimeter | Robot |
|---|---|---|
| Pack, open circuit (unplugged) | 7.872 V | — |
| Pack, robot on | 7.819 to 7.84 V | — |
| UNO VIN pin, after the diode | 7.371 V | 7.342 V (49 readings, 7.336 to 7.344) |

- **The SS34 diode between the pack and VIN drops about 0.45 V** at the robot's idle current.
- **Our reading of VIN is 0.029 V (0.4%) low.** One step of the UNO's 10-bit converter is about 37 mV at this divider, so the reading is as good as the hardware allows. No correction.
- **The pack sags about 0.04 V** from open circuit to the robot idling, with the camera board's Wi-Fi on.
- **The robot's thresholds are on VIN.** Low at 7.0 V and critical at 6.4 V there mean about 7.45 V and 6.85 V at the pack.
- **The first try read 4.18 V.** The probes were on the wrong points, and that cost us a wrong guess about one cell. Name the probe points with every reading.

## Ultrasonic

With `tests/robot/ultrasonic_ruler.py`, the robot on a hard floor and a book as the target, nothing within 25 cm of the lane:

| Tape | Robot | Error |
|---|---|---|
| 5.0 cm | 4.9 | −0.1 |
| 10.0 | 10.2 | +0.2 |
| 30.0 | 29.4 | −0.6 |
| 60.0 | 58.0 | −2.0 |
| 100.0 | 98.1 | −1.9 |
| 150.0 | 147.4 | −2.6 |
| about 199.6 | 199.4 (40 readings over 10 s, 199.2 to 199.8) | −0.2 |

- The first six points fit a scale error of about −1.9%, but that predicted about 196 cm at 2 m, and the robot read 199.4. **There is no scale error.**
- The 2 cm misses at 60 to 150 cm most likely came from the method: where the zero sits on the sensor, the tape's line, the book's angle and reading by eye. **Errors of 1 to 2 cm can't be told apart from the sensor with a tape read by eye.**
- **The sensor is steady to about 2 mm** at 2 m.
- **The UNO's clock is right:** its uptime agreed with the computer's to 0.005% over 551 s, so echo timing isn't stretched.
- The 25 to 35% shortfall [seen with the stock firmware](2026-11-06-first-sensor-readings.md#the-ultrasonic-reads-short) is gone with ours.
- T3 asks for 1 cm. It passed at 5, 10, 30 and 200 cm. A better check would need a fixed reference, such as floor marks measured once from a stop block.
