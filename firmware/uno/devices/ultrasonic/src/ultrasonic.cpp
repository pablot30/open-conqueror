#include "ultrasonic.h"

namespace oc {

uint16_t echoWidthToMm(uint32_t widthUs) {
  // widthUs x 343000 mm/s / 1e6 / 2 = widthUs x 343 / 2000. Rounded.
  // The width is limited first so the product can't overflow 32 bits.
  const uint32_t limitedUs = (widthUs > 100000U) ? 100000U : widthUs;
  const int32_t mm = static_cast<int32_t>(((limitedUs * 343U) + 1000U) / 2000U) +
                     static_cast<int32_t>(config::kUltrasonicOffsetMm);
  if (mm < 0) {
    return 0U;
  }
  if (mm > static_cast<int32_t>(config::kUltrasonicMaxMm)) {
    return config::kUltrasonicMaxMm;
  }
  return static_cast<uint16_t>(mm);
}

PingStep Ranger::update(uint32_t nowMs, bool echoComplete, uint32_t echoWidthUs) {
  PingStep step = {false, false, false};

  if (waiting_) {
    if (echoComplete) {
      waiting_ = false;
      step.resultReady = true;
      if (echoWidthUs >= config::kUltrasonicMinEchoUs) {
        step.echoOk = true;
        distanceMm_ = echoWidthToMm(echoWidthUs);
      }
    } else if ((nowMs - triggeredMs_) > config::kUltrasonicTimeoutMs) {
      // No echo, or one that never ended. Not "nothing there": the monitor fails.
      waiting_ = false;
      step.resultReady = true;
    } else {
      // Still waiting.
    }
  }

  // Unsigned subtraction stays correct when millis() wraps around.
  if (!waiting_ && (!started_ || ((nowMs - triggeredMs_) >= config::kUltrasonicPingPeriodMs))) {
    started_ = true;
    waiting_ = true;
    triggeredMs_ = nowMs;
    step.trigger = true;
  }
  return step;
}

}  // namespace oc
