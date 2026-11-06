# First sensor readings

**Dates:** 2026-11-04 and 2026-11-06. **How we know:** measured on our robot running its factory firmware. It was connected over USB with the Upload-Cam switch in Upload and main power off. A computer sent the [stock commands](../hardware/stock-protocol.md) N:21, N:22 and N:23 about six times a second, which only read sensors and never start the motors. The maintainer moved targets by hand, and distances were measured with a ruler from the ultrasonic sensor's front face.

## Results

| Sensor | Result |
|---|---|
| Serial link | About 3,000 requests per 110-second run, none unanswered |
| Gyro | Answers on I2C with the MPU6050's usual ID (0x68), but the stock firmware can't report motion data |
| Ultrasonic | Steady to ±1 cm, but **reads short**: 22 to 23 cm for a target at 30 cm, and 6 to 7 cm at 10 cm. A book and a black board gave the same results |
| Line sensors | Higher readings mean less light reflected. White paper reads 90 to 240, our wooden table 370 to 480, and nothing underneath 940 to 970 |
| Line sensor order | **Left and right are swapped**: see below |
| Lift detection | Works, but with little margin: see below |

## Left and right are swapped

Standing behind the robot and looking the way it drives, the sensor on the robot's **left** is the one on A0, which the schematic and the firmware call **right**. The firmware still has the opposite pin mapping, commented out, so ELEGOO probably changed it at some point.

A likely explanation is that ELEGOO names left and right as seen when facing the robot. If so, the motor channels may be named the same way. We'll check when we run the motors.

From now on, **left and right mean the robot's own left and right**, looking the way it drives.

## The ultrasonic reads short

The firmware divides the echo time by 58, the usual factor for centimetres, so the conversion isn't the problem.

Our guess: the stock code times the echo with Arduino's `pulseIn`, which counts loop cycles. Interrupts running during the count, such as the IR receiver's timer, would make the echo look shorter than it is. We haven't verified this. Stage 2 can test it by timing the echo with a hardware timer.

## Lift detection has little margin

The stock firmware treats the tank as lifted only when all three line sensors read above 950. Lifted about 5 cm, with nothing underneath, our sensors reached 956 to 970, which crossed the threshold after about three seconds. Lifted higher while being turned round, they read 975 to 1008. With a hand under the sensors, the threshold was never crossed. Our firmware should set this threshold from measurements.

N:23 answers `true` when the tank is lifted, even though the firmware's internal flag means the opposite.

## Other things to note

- **One sensor reads low:** on every surface, the robot's left sensor (A0) reads about 100 to 120 lower than the other two: 374 against about 470 on the table, and about 90 against about 210 on white paper. The difference stayed with that sensor when we turned the robot round, so it doesn't come from lighting or the robot's shadow. Each sensor needs its own calibration.
- **Room light barely matters, at least in our room:** the maintainer noticed that the robot casts a shadow right under its sensors. The sensors measure the reflection of their own infrared LEDs, but the stock firmware doesn't separate that from ambient light, so we tested it. Covering the front with a cloth changed the readings by no more than 5 counts. The room was lit by an old fluorescent tube lamp, which gives off little infrared, and no flicker showed in the readings. Daylight or an incandescent bulb could behave differently, and we haven't tested either.
- **Soft targets look like empty space:** with a cloth just in front of the ultrasonic sensor, it read 150 cm, its maximum. A missing echo looks the same as nothing in range, so obstacle avoidance can't see soft objects.
- **Our table may count as a line:** by our reading of the code, stock line tracking treats readings between 250 and 850 as being on the line. Our table reads inside that range, so the stock line-tracking mode might treat the whole table as a line. We'll see when we record the modes.

## What we learned about testing

- **Give a clear start signal:** our first recording missed everything because the test started without one. Now the maintainer replies "go" and waits 15 seconds, which covers the board's reset and the gyro calibration.
- **Keep hands and other objects away from the sensor being tested:** a helper's hand under the line sensors spoiled the lift test.
- **Hold things still:** a target held still for ten seconds is far easier to read than one being moved.
