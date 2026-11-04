# Features: the first goal

The first goal is done when everything here works with only our software running on the robot. Stage 2 covers the UNO items, and stage 3 covers the camera board.

The target is what ELEGOO's manual describes, except that [our own control page replaces the stock app](decisions/0004-own-protocol-and-control-page.md). Where the manual is silent, the stock firmware's behaviour is the reference. We don't reproduce stock bugs. Items marked *(to check)* need observing on a real robot in stage 1.

## Driving

- [ ] Forward and backward, holding a straight line with the gyro
- [ ] Turn left and right
- [ ] Eight directions at variable speed
- [ ] Stop

## Modes

- [ ] **Standby:** motors stopped, LED breathing violet
- [ ] **Line tracking:** follows a dark line, LED green. If it loses the line it searches briefly, then stops
- [ ] **Obstacle avoidance:** drives forward until something is within 20 cm, turns towards a clear direction and continues, LED yellow
- [ ] **Auto follow:** follows an object within 20 cm. With nothing ahead, it rotates 90° left, then 90° right, then stops, LED blue *(to check: stock firmware's search pattern looks different from the manual's)*

## Control

- [ ] **MODE button:** cycles through line tracking, obstacle avoidance, auto follow and standby
- [ ] **IR remote:** arrows drive, OK stops, 1 to 3 select modes, and 4 to 6 raise, reset and lower the line-tracking threshold
- [ ] **Computer tool (stage 2):** driving, mode selection, camera pan and tilt, and LED brightness
- [ ] **Web control page (stage 3):** eight-direction driving, mode buttons, live video, resolution selection, camera pan and tilt, and LED brightness

## Status and safety

- [ ] LED flashes red when the battery is low (stock threshold: 7.0 V)
- [ ] Stops when lifted off the ground in line tracking, obstacle avoidance and auto follow
- [ ] Stops when the computer tool or web page disconnects or goes quiet
- [ ] Calibrates the gyro at power-on, with the tank standing still
- [ ] Centres the camera servos at power-on

## Camera board (stage 3)

- [ ] A Wi-Fi network with a password
- [ ] The web control page, served by the camera board
- [ ] Video stream and still capture
- [ ] The camera settings the page uses
- [ ] Link between the page and the UNO
- [ ] Stop command to the UNO when the page disconnects

## Deliberate differences from stock

- Bugs get fixed. See the [first look at the stock firmware](notebook/2026-11-03-first-look-at-stock-firmware.md).
- Our own web page replaces the EleRobot app. Its graphical programming and DIY mode are dropped.
- The Wi-Fi network gets a password, because today anyone in range can join it and drive the robot.
- From stage 2, the UNO stops any movement commanded over the link if the link goes quiet.
