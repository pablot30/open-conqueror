# Power

Sources: the shield and camera board schematics, the manual and the stock firmware.

## Power path

1. The battery connects to the shield's POWER_IN header.
2. An SS34 Schottky diode feeds Vin, which supplies the TB6612's VM and the UNO's VIN.
3. The UNO's 5 V rail supplies everything else on the shield: both servos, the ultrasonic sensor, line sensors, GY-521, LED and camera board.
4. The camera board makes its 3.3 V from that 5 V rail with an AMS1117 regulator.

## Battery voltage

A3 reads Vin through a 10 kΩ and 1.5 kΩ divider, after the diode. The stock firmware scales the reading and adds 8%. It warns below 7.0 V, and one of ELEGOO's tutorials treats above 7.8 V as fully charged.

The 8% may be standing in for the diode's voltage drop, which would be better as a fixed offset.

**Measured on our robot on 2026-11-20** with a multimeter, our firmware running, nothing moving ([notebook](../notebook/2026-11-20-battery-and-ultrasonic-measurements.md)):

- The diode drops about 0.45 V: 7.82 V at the pack, 7.37 V at the UNO's VIN pin.
- Our reading of VIN is 0.03 V (0.4%) low, under one step of the UNO's converter. It needs no correction.
- Our thresholds are set on VIN, so at the pack they sit about 0.45 V higher.

## Signs to investigate

- The manual warns that a low battery can drop the Wi-Fi or freeze the camera.
- The stock firmware cuts motor drive around each gyro read, and stops the motors for ultrasonic readings. A source comment gives ranging accuracy as the reason.

We should look at the 5 V rail and I2C bus with a scope while the motors run before trusting sensor data.
