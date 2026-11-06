# Motor driver

Sources: the shield's schematic and the TB6612FNG datasheet.

## Wiring

- Channel A drives the right track and channel B the left, as the schematic says. We [confirmed this on our robot](../notebook/2026-11-06-motor-channels.md), along with the stock firmware's forward direction.
- An inverter ties IN2 to the inverse of IN1, so each motor has one direction pin.
- VM is the battery voltage after the input diode, and VCC is 5 V.
- Bulk capacitance on VM is 10 µF plus 100 nF, with 100 nF across each motor header.
- PGND goes straight to ground, so there is no current sensing.
- The motor headers have two pins, so there are no encoder inputs.

## What each motor can do

| Direction pin | PWM | Result |
|---|---|---|
| High | High | Drive one way |
| Low | High | Drive the other way |
| Either | Low | Short brake |

PWM therefore always alternates between drive and brake. The only way to coast is to pull STBY low, which affects both motors together.

## Motors

Retailer listings give a 1:48 gearbox: the motor turns 48 times for each turn of the output shaft, which trades speed for torque. ELEGOO publishes no voltage, current or speed ratings for these motors.

Vendors of generic yellow 1:48 TT motors rate them at 3 to 6 V, and we don't know if ours are the same. The driver is fed from a battery that reaches 8.4 V, and the stock firmware's speed limit is the full PWM value of 255, so full speed may exceed the motors' rating. We should settle this before choosing a speed limit for our own firmware.

## Driver ratings

| Limit | Value |
|---|---|
| Output current per channel | 1.2 A average, 3.2 A peak |
| Motor supply | Up to 13.5 V recommended, 15 V maximum |
| PWM frequency | Up to 100 kHz |
| Input pull-downs | 200 kΩ on IN1, IN2, PWM and STBY |
| Input high threshold | 0.7 × VCC, so 3.5 V on this shield |

The internal pull-downs mean a reset MCU leaves the driver in standby. The input threshold means a 3.3 V MCU would need level shifting to drive this shield within spec.
