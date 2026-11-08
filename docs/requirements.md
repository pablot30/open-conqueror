# Requirements

What our software must do on the stock robot, and how we'll check each requirement. This replaces the earlier feature list. Stage 2 covers the UNO, stage 3 the camera board.

The behaviour comes from ELEGOO's manual, except where [decision 0004](decisions/0004-own-protocol-and-control-page.md) replaces the stock app with our own tool and web page. Where the manual is silent, the stock firmware is the reference. We don't reproduce stock bugs.

**Status: draft.** Numbers marked *proposed* are starting points, to be confirmed by measurements on the robot.

Each requirement has an ID so that issues, tests and commits can refer to it:

- **F:** a feature, what the robot does.
- **T:** a technical goal, how well it does it.
- **SG and S:** safety goals, and the safety requirements that achieve them.
- **D:** a development requirement, what we need in order to build and test it.

## Features

### Driving

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F1 | Drive forward and backward, holding a straight line with the gyro | 2 | Drive 2 m on a flat floor and measure how far it drifts sideways |
| F2 | Turn left and right on the spot | 2 | Watch it turn on the robot |
| F3 | Drive in eight directions at a variable speed | 2 | Command each direction from the computer tool |
| F4 | Stop on command | 2 | Covered by S1 |

### Modes

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F5 | **Standby:** motors stopped, LED breathing violet | 2 | Watch it on the robot |
| F6 | **Line tracking:** follows a dark line, LED green. If it loses the line, it searches briefly, then stops | 2 | Run a taped loop on a light floor |
| F7 | **Obstacle avoidance:** drives forward until something is within 20 cm, turns towards a clear direction and carries on, LED yellow | 2 | Run it on open floor with a box in its path |
| F8 | **Follow:** follows an object within 20 cm. With nothing ahead, it turns 90° left, then 90° right, then stops, LED blue | 2 | Lead it with a box, then take the box away |

### Control

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F9 | **MODE button** cycles through line tracking, obstacle avoidance, follow and standby | 2 | Press it on the robot |
| F10 | **IR remote:** arrows drive, OK stops, 1 to 3 select modes, and 4 to 6 raise, reset and lower the line-tracking threshold | 2 | Use each key |
| F11 | **Computer tool:** driving, mode selection, camera pan and tilt, LED brightness, and live sensor readings | 2 | Use each function |
| F12 | **Web control page:** eight-direction driving, mode buttons, live video, resolution, camera pan and tilt, LED brightness | 3 | Use each function from a phone and a computer |

### Status

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F13 | Calibrate the gyro at power-on, with the robot standing still | 2 | Watch the startup |
| F14 | Centre the camera servos at power-on | 2 | Watch the startup |
| F15 | Show a low battery by flashing the LED red | 2 | Run it from a bench supply turned down to the threshold |

### Camera board

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| F16 | Join a Wi-Fi network with a password, or open its own network with a password | 3 | Connect a phone and the computer |
| F17 | Video stream and still capture | 3 | View both in a browser |
| F18 | Carry the link between the UNO and the computer tool or web page | 3 | Covered by F11 and F12 |

## Technical goals

| ID | Goal | How we check |
|---|---|---|
| T1 | The control loop runs at least 100 times a second *(proposed)* | Firmware reports its loop timing |
| T2 | Nothing blocks the main loop for more than 5 ms *(proposed)*. The stock firmware blocks for up to 1 s on the ultrasonic and the servos | Firmware reports the longest loop time |
| T3 | Ultrasonic distance within ±1 cm from 5 to 150 cm. The stock firmware reads 25 to 35% short | Compare with a ruler |
| T4 | Heading hold corrects drift down to 0.5° per second *(proposed)*. The stock firmware ignores anything below about 4.5° per second | Drive straight and log the gyro |
| T5 | Each line sensor has its own calibration, stored on the robot. Ours differ by about 100 counts on the same surface | Calibrate, power off and on, and compare readings |
| T6 | From power-on to ready, including gyro calibration, in under 3 s *(proposed)* | Time it |
| T7 | Commands from the computer reach the motors in under 50 ms over Wi-Fi, typically *(proposed)*. Sensor data streams at 10 times a second or more | Log the timing in the computer tool |
| T8 | The UNO firmware uses at most 80% of flash and leaves at least 512 bytes of RAM free. It uses no dynamic memory | Build report |

## Safety

We follow the idea behind automotive safety work, kept light: list what could go wrong, set a goal against each hazard, and derive requirements that achieve the goals.

Two hardware limits shape this. The robot has **no current sensing and no wheel encoders**, so it can't directly tell when a motor is stalled. And **soft objects don't echo**, so the ultrasonic can miss them.

### Hazards and safety goals

| Hazard | Safety goal |
|---|---|
| The user wants it to stop, but it doesn't | **SG1:** The user can always stop the robot, through any control they're using |
| The link drops or a computer program crashes, and the robot keeps driving | **SG2:** The robot stops when it loses its controller |
| The robot starts moving by itself: at power-on, after a reset, while being flashed, or when switching modes | **SG3:** The robot only moves when told to |
| It keeps pushing against a wall, overheating the motors or driver | **SG4:** The robot doesn't keep driving when it isn't making progress |
| It drives off a table or keeps running when picked up | **SG5:** The robot stops when it loses the floor |
| The battery is drained too far and damaged | **SG6:** The robot protects its battery |
| The motors get more voltage than they're rated for | **SG7:** The robot keeps the motors within their limits |
| Someone else joins its network and drives it | **SG8:** Only its owner can control the robot |

### Safety requirements

| ID | Requirement | Goal | Stage | How we check |
|---|---|---|---|---|
| S1 | A stop from the IR remote, the MODE button, the computer tool or the web page stops the motors within 150 ms *(proposed)* | SG1 | 2 | Time it with a log or a video |
| S2 | The IR remote's OK key stops the robot even when Wi-Fi is down | SG1 | 2 | Turn Wi-Fi off and press OK |
| S3 | Movement commanded over the link stops if no valid message arrives for 300 ms *(proposed)* | SG2 | 2 | Unplug the link, or stop the computer tool mid-drive |
| S4 | A watchdog resets the UNO if its firmware hangs, and the motors are off after any reset | SG2, SG3 | 2 | Force a hang in a test build |
| S5 | The motors stay off from power-on until an explicit drive or mode command | SG3 | 2 | Power on and watch |
| S6 | Every mode change stops the motors first. The stock firmware keeps the previous mode's motor command | SG3 | 2 | Switch modes while driving |
| S7 | If the robot is commanded to move but the gyro and ultrasonic show no progress for 2 s, it stops *(proposed; best effort without encoders)* | SG4 | 2 | Drive it against a wall |
| S8 | When the line sensors see no floor, the robot stops, in every mode including remote driving. Thresholds come from calibration, and black floors need checking for false stops | SG5 | 2 | Lift it, and drive it slowly towards a table edge with a hand ready to catch it |
| S9 | Below 7.0 V the robot warns. Below 6.4 V *(proposed)* it stops the motors and won't drive. Readings are filtered so a brief dip under load doesn't trigger it | SG6 | 2 | Bench supply |
| S10 | The motors' average voltage stays at or below 6 V until we know their real rating *(proposed)* | SG7 | 2 | Calculate from the PWM limit and battery voltage |
| S11 | Wi-Fi uses a password, and the link only accepts commands from our tool or page. Credentials never go in the repo | SG8 | 3 | Try to connect without the password |

## Development requirements

| ID | Requirement | Stage | How we check |
|---|---|---|---|
| D1 | Telemetry, logs and control over Wi-Fi, so tests run without a cable | 2 | Run a test with the USB cable unplugged |
| D2 | Update the UNO firmware over Wi-Fi | 2 | Flash it with no cable connected |
| D3 | Update the camera board firmware over Wi-Fi | 2 | Same |
| D4 | Every firmware reports its overall version and git commit at startup and on request. Each software unit, such as a driver, has its own version too | 2 | Ask the robot |
| D5 | The logic that doesn't depend on hardware has unit tests that run on a computer, and CI runs them on macOS, Linux and Windows | 2 | CI results |
| D6 | We can always return to the stock firmware | 2 | Restore the backups at the end of the first flashing session |
| D7 | **Telemetry:** the robot streams its state to the computer: versions, mode, sensor readings, motor commands and internal signals, at a rate we can choose | 2 | View it live in the computer tool |
| D8 | **Logs:** both boards send timestamped log messages with a severity level, and the computer can record them | 2 | Record a session and read it back |
| D9 | **Evaluation tool:** on the computer, we can view telemetry and logs live, record them, replay them later and plot signals. It may be our own tool, an existing viewer, or both | 2 | Analyse a recorded test run |
| D10 | **Online calibration:** calibration values, such as line sensor thresholds and controller gains, can be read and changed while the robot runs, then saved on the robot. Saved values carry a check so that corrupted values fall back to safe defaults | 2 | Change a value live, then power off and on |
| D11 | **Diagnostics:** monitors detect faults, such as a low battery, a missing gyro, the ultrasonic timing out, the link dropping or an unexpected reset. Each fault is stored on the robot as a fault code with its status and a snapshot of the conditions when it happened. The computer can read and clear them, as with diagnostic trouble codes in cars | 2 | Cause a fault on purpose, then read its code and snapshot |
| D12 | **Debugging:** each board can be debugged beyond log messages, for example with breakpoints | 2 | Stop at a breakpoint on each board |

## Deliberate differences from stock

- **Bugs get fixed:** see the [first look at the stock firmware](notebook/2026-11-03-first-look-at-stock-firmware.md) and the [motor channels](notebook/2026-11-06-motor-channels.md).
- **No EleRobot app:** our own tool and page replace it, and its graphical programming and DIY mode are dropped.
- **Wi-Fi gets a password:** today anyone in range can join and drive the robot.
- **Stops work in every mode:** stopping on link loss (S3), on losing the floor (S8) and on lack of progress (S7) apply everywhere, not just in the automatic modes.
