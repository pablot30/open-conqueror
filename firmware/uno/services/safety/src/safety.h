// The safety supervisor: the only code that decides what the motors do,
// from commands, time and the battery. See docs/architecture.md, A2.
#ifndef OC_SAFETY_H
#define OC_SAFETY_H

#include <stdint.h>

namespace oc {

namespace config {
// S3: drive commands expire after this long without a new one (proposed).
const uint32_t kLinkTimeoutMs = 300U;
// The most that heading hold may add to one track and take from the other,
// in tenths of a percent of the allowed maximum. The supervisor limits it
// itself, whatever it is asked for.
const int16_t kMaxTrimPermille = 60;
}  // namespace config

enum class DriveState : uint8_t {
  kStandby = 0,  // S5: driver in standby since power-on, motors coast
  kDriving = 1,  // following the latest drive command
  kStopped = 2,  // motors braked after a stop, timeout or low battery
};

// What to send to the motor driver. PWM is -255 to 255, negative is backward.
struct MotorOutput {
  bool enabled;  // false keeps the driver in standby
  int16_t left;
  int16_t right;
};

// After power-on and after every stop, drive commands are ignored until a
// neutral one (both sides 0) arrives. A stream of resent drive commands
// therefore can't undo a stop: driving again takes a deliberate new command.
class SafetySupervisor {
 public:
  // A drive command, each side -100 to 100 percent of the allowed maximum.
  void drive(int8_t leftPercent, int8_t rightPercent, uint32_t nowMs);
  // Brakes, and holds the motors stopped until the next neutral command.
  void stop();
  // A small correction from heading hold, in tenths of a percent of the
  // allowed maximum: positive adds to the right track and takes from the
  // left, which turns the robot left. It changes the output while driving
  // and nothing else: it isn't a command, so it doesn't refresh the link
  // timeout (S3), arm the supervisor or outlast a stop.
  void setTrim(int16_t permille);

  // Call every loop. pwmLimit is the highest PWM allowed (S10), and
  // driveAllowed is false while the battery is critical (S9).
  MotorOutput update(uint32_t nowMs, uint16_t pwmLimit, bool driveAllowed);

  DriveState state() const { return state_; }
  bool linkTimedOut() const { return linkTimedOut_; }
  // The tracks the latest accepted command asks for, in percent. Zero when
  // stopped.
  int8_t commandedLeft() const { return leftPercent_; }
  int8_t commandedRight() const { return rightPercent_; }

 private:
  DriveState state_ = DriveState::kStandby;
  int8_t leftPercent_ = 0;
  int8_t rightPercent_ = 0;
  uint32_t lastCommandMs_ = 0U;
  bool linkTimedOut_ = false;
  int16_t trimPermille_ = 0;
  bool armed_ = false;  // true once a neutral command has followed the last stop
};

}  // namespace oc

#endif  // OC_SAFETY_H
