# First look at the stock firmware

**Date:** 2026-11-03. **Scope:** ELEGOO's UNO firmware, TB6612 version dated 2024-06-05.

**How we know:** we read the source and compiled it, but **haven't run any of this on the robot yet**. The robot works out of the box, and these notes are about what to do differently in a rewrite.

## Size

Compiled for the UNO with `arduino:avr` 1.8.8 and the library versions in the kit:

| Resource | Used | Available |
|---|---|---|
| Flash | 31,652 bytes (98%) | 32,256 bytes |
| RAM, static | 1,153 bytes (56%) | 2,048 bytes, leaving 895 for the stack |

A rough breakdown by symbol name puts about 15 KB of the 31 KB in libraries and support code: float and C library routines, ArduinoJson, FastLED, I2C and MPU6050 drivers, String and Serial, and Servo. Another 11 KB couldn't be attributed. So 98% looks like a result of how the firmware is built rather than a hard limit.

## Likely bugs

Compiler warnings or a small host test support these:

- **Single-motor commands:** `direction_void` has the value 3 but passes through a `boolean` parameter, where it becomes `true`. Its `case` can never run. A command for one motor therefore also runs the other forward at its last speed, and "stop" means PWM 0 with STBY left high.
- **Obstacle mode on a clear path:** the call that should drive forward passes the speed variable as the direction, which matches no direction and commands nothing. The tutorial version of this code passes `Forward`.
- **Line threshold limits:** the threshold is a `uint8_t` but gets compared with 600, so the upper limit never applies.
- **Gyro setup:** the retry counter starts uninitialized, and the setup result is ignored.

## Other things we'd change

These come from reading the source:

- **Slow heading drift goes uncorrected.** The heading code ignores gyro steps below 0.05° per sample. At roughly 11 ms per sample, that hides anything below about 4.5°/s.
- **No overall heading.** Yaw is only integrated while driving straight.
- **Motor drive gets cut around each gyro read.** We estimate about 0.5 ms in every 10 ms, assuming 100 kHz I2C.
- **Blocking calls.** The ultrasonic read has no timeout, so it can block for up to 1 s, and servo moves block for 450 ms to 1 s.
- **All timers are taken.** Timer0 runs `millis()` and the motor PWM at about 976 Hz, Timer1 runs the servos and Timer2 runs the IR receiver.
- **No timeout on the UNO.** After N:3, the robot drives until another command arrives. If the camera board hangs, nothing stops it.
- **Slow, unchecked link.** JSON at 9600 baud takes about 26 ms for a typical command, with no checksum, and the receive buffer can grow without limit.

## Predictions to test in stage 1

1. In obstacle mode with a clear path, the tank won't start moving by itself. After avoiding something, it keeps its last motor command.
2. After running both motors in reverse with N:1, a command for one motor makes the other run forward.
3. On a surface where the tank veers slowly, the straight-line hold doesn't correct it.

## Worth keeping as ideas

- Lift detection: all three line sensors reading above 950 means the tank is off the ground.
- Low-battery detection that waits for repeated low readings.
- Stopping when the app disconnects.
- Gyro-based straight-line hold.
- The IR remote as a stop channel that doesn't depend on Wi-Fi.
