// Decides what the motors do, from commands, time and the battery.
#ifndef OC_CORE_DRIVE_H
#define OC_CORE_DRIVE_H

#include <stdint.h>

namespace oc {

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

class DriveControl {
 public:
  // A drive command, each side -100 to 100 percent of the allowed maximum.
  void drive(int8_t leftPercent, int8_t rightPercent, uint32_t nowMs);
  void stop();

  // Call every loop. pwmLimit is the highest PWM allowed (S10), and
  // driveAllowed is false while the battery is critical (S9).
  MotorOutput update(uint32_t nowMs, uint16_t pwmLimit, bool driveAllowed);

  DriveState state() const { return state_; }
  bool linkTimedOut() const { return linkTimedOut_; }

 private:
  DriveState state_ = DriveState::kStandby;
  int8_t leftPercent_ = 0;
  int8_t rightPercent_ = 0;
  uint32_t lastCommandMs_ = 0U;
  bool linkTimedOut_ = false;
};

}  // namespace oc

#endif  // OC_CORE_DRIVE_H
