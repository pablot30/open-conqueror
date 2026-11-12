#include "scheduler.h"

namespace oc {

bool FrameClock::due(uint32_t nowUs) {
  if (!started_) {
    started_ = true;
    next_ = nowUs + config::kFramePeriodUs;
    ++frames_;
    return true;
  }
  // Signed difference, so the comparison survives micros() wrapping around.
  const int32_t late = static_cast<int32_t>(nowUs - next_);
  if (late < 0) {
    return false;
  }
  if (static_cast<uint32_t>(late) >= config::kFramePeriodUs) {
    const uint32_t missed = static_cast<uint32_t>(late) / config::kFramePeriodUs;
    skipped_ = (missed > (0xFFFFU - skipped_)) ? 0xFFFFU : static_cast<uint16_t>(skipped_ + missed);
    next_ += missed * config::kFramePeriodUs;
  }
  next_ += config::kFramePeriodUs;
  ++frames_;
  return true;
}

void Stopwatch::record(uint32_t startUs, uint32_t endUs) {
  const uint32_t elapsed = endUs - startUs;
  const uint16_t clipped = (elapsed > 0xFFFFU) ? static_cast<uint16_t>(0xFFFFU) : static_cast<uint16_t>(elapsed);
  if (clipped > longestUs_) {
    longestUs_ = clipped;
  }
}

}  // namespace oc
