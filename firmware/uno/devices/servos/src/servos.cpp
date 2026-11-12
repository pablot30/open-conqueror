#include "servos.h"

namespace oc {

int32_t ServoRamp::towards(int32_t from, int32_t to, int32_t step) {
  if (to > from) {
    return ((to - from) > step) ? (from + step) : to;  // never overshoots
  }
  return ((from - to) > step) ? (from - step) : to;
}

void ServoRamp::setTarget(uint8_t panDeg, uint8_t tiltDeg, uint16_t speedDps) {
  panTarget_ = static_cast<int32_t>(panDeg) * kHundredths;
  tiltTarget_ = static_cast<int32_t>(tiltDeg) * kHundredths;
  if (speedDps == 0U) {
    step_ = 180 * kHundredths;  // more than the whole range: one step
  } else {
    // degrees a second * 100 hundredths / frames a second; never less than 1
    // so a very slow speed still arrives.
    step_ = static_cast<int32_t>((static_cast<uint32_t>(speedDps) * 100U) /
                                 config::kServoFramesPerSecond);
    if (step_ < 1) {
      step_ = 1;
    }
  }
  active_ = true;
  needWrite_ = true;
  settling_ = false;
}

ServoRamp::Action ServoRamp::update(uint32_t frame) {
  if (!active_) {
    return Action::kIdle;
  }
  bool write = needWrite_;
  needWrite_ = false;
  if ((pan_ != panTarget_) || (tilt_ != tiltTarget_)) {
    pan_ = towards(pan_, panTarget_, step_);
    tilt_ = towards(tilt_, tiltTarget_, step_);
    write = true;
  }
  if (!settling_ && (pan_ == panTarget_) && (tilt_ == tiltTarget_)) {
    settling_ = true;
    reachedFrame_ = frame;
  }
  if (write) {
    return Action::kWrite;
  }
  if (settling_ && ((frame - reachedFrame_) >= config::kServoSettleFrames)) {
    active_ = false;
    settling_ = false;
    return Action::kRelease;
  }
  return Action::kIdle;
}

}  // namespace oc
