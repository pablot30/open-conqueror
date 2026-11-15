#include "safety.h"

namespace oc {

namespace {

int16_t scale(int8_t percent, uint16_t pwmLimit) {
  const int32_t pwm = (static_cast<int32_t>(percent) * static_cast<int32_t>(pwmLimit)) / 100;
  return static_cast<int16_t>(pwm);
}

int32_t clampPwm(int32_t pwm, uint16_t pwmLimit) {
  const int32_t limit = static_cast<int32_t>(pwmLimit);
  if (pwm > limit) {
    return limit;
  }
  return (pwm < -limit) ? -limit : pwm;
}

}  // namespace

void SafetySupervisor::drive(int8_t leftPercent, int8_t rightPercent, uint32_t nowMs) {
  if (!armed_) {
    if ((leftPercent != 0) || (rightPercent != 0)) {
      return;  // still holding a stop, or standby since power-on
    }
    armed_ = true;
  }
  state_ = DriveState::kDriving;
  leftPercent_ = leftPercent;
  rightPercent_ = rightPercent;
  lastCommandMs_ = nowMs;
  linkTimedOut_ = false;
}

void SafetySupervisor::setTrim(int16_t permille) {
  if (permille > config::kMaxTrimPermille) {
    permille = config::kMaxTrimPermille;
  } else if (permille < -config::kMaxTrimPermille) {
    permille = static_cast<int16_t>(-config::kMaxTrimPermille);
  } else {
    // within the limit
  }
  trimPermille_ = permille;
}

void SafetySupervisor::stop() {
  armed_ = false;
  trimPermille_ = 0;
  if (state_ == DriveState::kDriving) {
    state_ = DriveState::kStopped;
  }
  leftPercent_ = 0;
  rightPercent_ = 0;
}

MotorOutput SafetySupervisor::update(uint32_t nowMs, uint16_t pwmLimit, bool driveAllowed) {
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
  int32_t left = scale(leftPercent_, pwmLimit);
  int32_t right = scale(rightPercent_, pwmLimit);
  if ((state_ == DriveState::kDriving) && ((leftPercent_ != 0) || (rightPercent_ != 0))) {
    const int32_t trim = (static_cast<int32_t>(trimPermille_) * static_cast<int32_t>(pwmLimit)) / 1000;
    left = clampPwm(left - trim, pwmLimit);
    right = clampPwm(right + trim, pwmLimit);
  }
  output.left = static_cast<int16_t>(left);
  output.right = static_cast<int16_t>(right);
  return output;
}

}  // namespace oc
