# Heading hold on the floor

**Date:** 2026-11-20. **How we know:** measured on our robot running our firmware (0.1.0-dev, commit `bf9a708`, protocol 8), driven over the robot's own Wi-Fi (`--port tcp:192.168.4.1:100`) with `tests/robot/heading_hold.py`. The battery read about 7.6 V at rest. The maintainer measured distances with a tape measure. Left and right are the robot's own, looking the way it drives.

## Results

All runs were at 40% (PWM 80 of 255, since the motor limit is 200).

| Run | Surface | Drive | Distance | Heading change | Correction | Sideways at the end |
|---|---|---|---|---|---|---|
| 1, 2 | Carpet | 5 s forward, 5 s back | about 70 cm | −0.2° forward, −0.9° back | −3.7% to +2.8% | too small to see |
| 3 | Hard floor | 14 s forward | 214 cm | −0.4° | −3.3% to −0.2% | 3.5 cm right |
| 3 | Hard floor | 14 s back | back to the start mark | +0.3° | +0.2% to +4.2% | 2.5 cm left |

Heading hold was active in every sensor message of every run, and the Wi-Fi link never dropped. F1 asks for 2 m within 10 cm, and run 3 passed it.

## What we learned

- **The robot is slow at 40%:** about 15 cm/s on a hard floor and 14 cm/s on carpet. The test used to say "about 2 m in 5 s", which was a guess. Carpet barely changes straight-line speed. Turning on carpet is a different question (#70).
- **The tracks don't match, and the mismatch changes with direction.** Holding the heading took about −2% going forward and about +3.5% going backward. A fixed trim per direction, learned from runs like these, could start the controller already corrected.
- **Heading hold keeps the heading, not the lane.** It holds the heading the robot had when the command started. If the robot sits 1° off the lane at the start, it ends about 3.7 cm off after 2.14 m. That matches the 3.5 cm we measured, and the robot looked slightly diagonal to the lane at the end. Holding a line needs a position, which we don't have without encoders.
- **Tuning wasn't needed.** Corrections stayed under 4.2%, inside the 5% limit, and the heading stayed within about 1°. Kp, Ki, the 15% minimum and the 5% limit stay as they are for now.
- **The gyro heading drifts while the robot stands still:** from 274.8° to 277.3° over 121 s, about 1.2° a minute. Over a 14 s run that is 0.3°, so heading hold doesn't notice it. Turning to an absolute heading later will need a better bias estimate.
