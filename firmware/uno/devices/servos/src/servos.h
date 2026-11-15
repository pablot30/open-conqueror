// The camera's pan and tilt servos (F14): centred at power-on, moved at a set
// speed, then released so they don't hum or jitter. They hold their position
// by friction. servos.cpp is the ramp logic; servos_hw.cpp drives the pins.
#ifndef OC_SERVOS_H
#define OC_SERVOS_H

#include <stdint.h>

namespace oc {

namespace config {
// F14: centre positions in degrees. 90 is the middle of an SG90's range;
// whether that points the camera straight ahead is checked on the robot.
const uint8_t kPanCentreDeg = 90U;
const uint8_t kTiltCentreDeg = 90U;
// F14: how fast the camera turns unless a move says otherwise, in degrees a
// second. 0 means as fast as the servo can go. Chosen by feel on the robot.
const uint16_t kServoDefaultSpeedDps = 100U;
// The frame rate the ramp counts in (work runs in 10 ms frames).
const uint32_t kServoFramesPerSecond = 100U;
// How long the servos get to settle after the last step before they're
// released. 0.5 s covers the SG90's slowest full turn.
const uint32_t kServoSettleFrames = 50U;  // 0.5 s
}  // namespace config

// Moves the camera angles towards a target at a set speed, one step a frame,
// and says when to write to the servos and when to release them.
// Angles are tracked in hundredths of a degree, so any speed adds up exactly
// and nothing drifts. The real position of an SG90 can't be read, so the
// ramp starts from the centre and trusts its own count.
class ServoRamp {
 public:
  enum class Action : uint8_t {
    kIdle,     // nothing to do
    kWrite,    // write pan() and tilt() to the servos
    kRelease,  // the move has settled: stop the control pulses
  };

  // Starts a move to these angles (0 to 180) from wherever the ramp is now.
  // speedDps is degrees a second; 0 means jump in one step.
  void setTarget(uint8_t panDeg, uint8_t tiltDeg, uint16_t speedDps);
  // Call once a frame.
  Action update(uint32_t frame);

  // The angle to send to the servo now, in whole degrees.
  uint8_t pan() const { return toDegrees(pan_); }
  uint8_t tilt() const { return toDegrees(tilt_); }
  bool moving() const { return active_; }

 private:
  static const int32_t kHundredths = 100;
  static uint8_t toDegrees(int32_t hundredths) {
    return static_cast<uint8_t>((hundredths + (kHundredths / 2)) / kHundredths);
  }
  static int32_t towards(int32_t from, int32_t to, int32_t step);

  int32_t pan_ = static_cast<int32_t>(config::kPanCentreDeg) * kHundredths;
  int32_t tilt_ = static_cast<int32_t>(config::kTiltCentreDeg) * kHundredths;
  int32_t panTarget_ = pan_;
  int32_t tiltTarget_ = tilt_;
  int32_t step_ = 0;  // hundredths of a degree a frame
  uint32_t reachedFrame_ = 0U;
  bool active_ = false;
  bool needWrite_ = false;  // write once even if already at the target
  bool settling_ = false;
};

namespace servos_hw {
// Attaches both servos and sends them to these angles.
void move(uint8_t panDeg, uint8_t tiltDeg);
// Stops the control pulses; the servos stop holding actively.
void release();
}  // namespace servos_hw

}  // namespace oc

#endif  // OC_SERVOS_H
