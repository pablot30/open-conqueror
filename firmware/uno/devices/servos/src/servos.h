// The camera's pan and tilt servos (F14): centred at power-on, then released
// so they don't hum or jitter. They hold their position by friction.
// servos.cpp is the timing logic; servos_hw.cpp drives the pins.
#ifndef OC_SERVOS_H
#define OC_SERVOS_H

#include <stdint.h>

namespace oc {

namespace config {
// F14: centre positions in degrees. 90 is the middle of an SG90's range;
// whether that points the camera straight ahead is checked on the robot.
const uint8_t kPanCentreDeg = 90U;
const uint8_t kTiltCentreDeg = 90U;
// How long the servos get to reach the centre before they're released.
const uint32_t kServoMoveFrames = 50U;  // 0.5 s
}  // namespace config

// Decides when a move is finished, so the servos can be released.
class ServoMove {
 public:
  void start(uint32_t frame) {
    startFrame_ = frame;
    moving_ = true;
  }
  // True once, on the frame the move is over.
  bool finished(uint32_t frame);
  bool moving() const { return moving_; }

 private:
  uint32_t startFrame_ = 0U;
  bool moving_ = false;
};

namespace servos_hw {
// Attaches both servos and sends them to these angles.
void move(uint8_t panDeg, uint8_t tiltDeg);
// Stops the control pulses; the servos stop holding actively.
void release();
}  // namespace servos_hw

}  // namespace oc

#endif  // OC_SERVOS_H
