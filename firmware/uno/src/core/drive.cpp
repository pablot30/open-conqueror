#include "drive.h"

#include "config.h"

namespace oc {

namespace {

int16_t scale(int8_t percent, uint16_t pwmLimit) {
  const int32_t pwm = (static_cast<int32_t>(percent) * static_cast<int32_t>(pwmLimit)) / 100;
  return static_cast<int16_t>(pwm);
}

}  // namespace

void DriveControl::drive(int8_t leftPercent, int8_t rightPercent, uint32_t nowMs) {
  state_ = DriveState::kDriving;
  leftPercent_ = leftPercent;
  rightPercent_ = rightPercent;
  lastCommandMs_ = nowMs;
  linkTimedOut_ = false;
}

void DriveControl::stop() {
  if (state_ == DriveState::kDriving) {
    state_ = DriveState::kStopped;
  }
  leftPercent_ = 0;
  rightPercent_ = 0;
}

MotorOutput DriveControl::update(uint32_t nowMs, uint16_t pwmLimit, bool driveAllowed) {
  if (state_ == DriveState::kDriving) {
    // Unsigned subtraction stays correct when millis() wraps around.
    if ((nowMs - lastCommandMs_) > config::kLinkTimeoutMs) {
      linkTimedOut_ = true;
      stop();
    } else if (!driveAllowed) {
      stop();
    } else {
      // Keep driving.
    }
  }

  MotorOutput output;
  output.enabled = (state_ != DriveState::kStandby);
  if (pwmLimit > 255U) {
    pwmLimit = 255U;
  }
  output.left = scale(leftPercent_, pwmLimit);
  output.right = scale(rightPercent_, pwmLimit);
  return output;
}

}  // namespace oc
