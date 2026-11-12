#include "gyro.h"

namespace oc {

namespace {

// One full turn in the units of heading_: 360 degrees x 131 counts per
// degree per second x 100 frames per second x 100 (the x100 in rateX100_).
const int32_t kFullTurn = 360L * kGyroRawPerDps * kGyroFramesPerSecond * 100L;

}  // namespace

void HeadingGyro::update(int16_t raw) {
  if (!calibrated_) {
    if (count_ == 0U) {
      sum_ = 0;
      min_ = raw;
      max_ = raw;
    }
    sum_ += raw;
    if (raw < min_) {
      min_ = raw;
    }
    if (raw > max_) {
      max_ = raw;
    }
    ++count_;
    if (count_ >= config::kGyroCalibrationFrames) {
      if ((static_cast<int32_t>(max_) - min_) <= config::kGyroStillSpanRaw) {
        // The mean, in hundredths of a count.
        biasX100_ = (sum_ * 100L) / static_cast<int32_t>(count_);
        calibrated_ = true;
      } else if (restarts_ < 0xFFU) {
        ++restarts_;  // the robot moved; try again
      } else {
        // Keep trying, but stop counting.
      }
      count_ = 0U;
    }
    return;
  }
  rateX100_ = (static_cast<int32_t>(raw) * 100L) - biasX100_;
  integrate();
}

void HeadingGyro::missedFrame() {
  if (calibrated_) {
    integrate();
  }
}

void HeadingGyro::integrate() {
  heading_ += rateX100_;
  if (heading_ >= kFullTurn) {
    heading_ -= kFullTurn;
  } else if (heading_ < 0) {
    heading_ += kFullTurn;
  } else {
    // In range.
  }
}

uint16_t HeadingGyro::headingCdeg() const {
  // 36000 hundredths of a degree per kFullTurn.
  return static_cast<uint16_t>(heading_ / (kFullTurn / 36000L));
}

}  // namespace oc
