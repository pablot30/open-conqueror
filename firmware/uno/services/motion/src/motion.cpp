#include "motion.h"

namespace oc {

namespace {

const int32_t kFramesPerSecond = 100;
const int32_t kCentidegreesPerDegree = 100;

int32_t clamp(int32_t value, int32_t low, int32_t high) {
  if (value < low) {
    return low;
  }
  return (value > high) ? high : value;
}

}  // namespace

int32_t headingError(uint16_t to, uint16_t from) {
  int32_t difference = static_cast<int32_t>(to) - static_cast<int32_t>(from);
  if (difference >= 18000) {
    difference -= 36000;
  } else if (difference < -18000) {
    difference += 36000;
  } else {
    // already the short way round
  }
  return difference;
}

void HeadingHold::reset() {
  captured_ = false;
  integral_ = 0;
  correction_ = 0;
  substituteFrames_ = 0U;
}

HoldOutput HeadingHold::update(const HoldInput& input) {
  const bool straight = input.driving && (input.left == input.right) &&
                        ((input.left >= config::kHoldMinSpeedPercent) ||
                         (input.left <= -config::kHoldMinSpeedPercent));
  if (!straight) {
    reset();
    direction_ = 0;
    state_ = HoldState::kOff;
    return {0, state_};
  }
  const int8_t direction = (input.left > 0) ? static_cast<int8_t>(1) : static_cast<int8_t>(-1);
  if (direction != direction_) {
    reset();  // forward to backward is a new command: hold the new heading
    direction_ = direction;
  }

  switch (input.gyro) {
    case SignalStatus::kValid:
      substituteFrames_ = 0U;
      if (!captured_) {
        // The start of the run, or the gyro came back: hold the heading
        // the robot has now.
        captured_ = true;
        target_ = input.headingCdeg;
        integral_ = 0;
        correction_ = 0;
      } else {
        const int32_t error = headingError(target_, input.headingCdeg);
        const int32_t integralLimit =
            (static_cast<int32_t>(config::kHoldMaxCorrectionPermille) * kCentidegreesPerDegree * kFramesPerSecond) /
            static_cast<int32_t>(config::kHoldKiPermillePerDegSec);
        const int32_t proportional = (error * config::kHoldKpPermillePerDeg) / kCentidegreesPerDegree;
        const int32_t candidate = clamp(integral_ + error, -integralLimit, integralLimit);
        const int32_t withCandidate = proportional + ((candidate * config::kHoldKiPermillePerDegSec) /
                                                      (kCentidegreesPerDegree * kFramesPerSecond));
        // No wind-up: while the output is at its limit, the integral stops
        // growing in the direction that keeps it there.
        const bool saturated = (withCandidate > config::kHoldMaxCorrectionPermille) ||
                               (withCandidate < -config::kHoldMaxCorrectionPermille);
        const bool growsSaturation = ((withCandidate > 0) && (error > 0)) || ((withCandidate < 0) && (error < 0));
        if (!(saturated && growsSaturation)) {
          integral_ = candidate;
        }
        const int32_t integralTerm = (integral_ * config::kHoldKiPermillePerDegSec) /
                                     (kCentidegreesPerDegree * kFramesPerSecond);
        correction_ = static_cast<int16_t>(clamp(proportional + integralTerm, -config::kHoldMaxCorrectionPermille,
                                                 config::kHoldMaxCorrectionPermille));
      }
      state_ = HoldState::kHolding;
      break;

    case SignalStatus::kSubstitute:
      // A substitute heading integrates the last rate: good for a moment.
      if (captured_ && (substituteFrames_ < config::kHoldSubstituteFrames)) {
        ++substituteFrames_;
        state_ = HoldState::kSubstitute;  // keeps the last correction
      } else {
        reset();
        state_ = HoldState::kNoGyro;
      }
      break;

    case SignalStatus::kInit:
    case SignalStatus::kError:
    case SignalStatus::kNotAvailable:
    default:
      reset();
      state_ = HoldState::kNoGyro;
      break;
  }
  return {correction_, state_};
}

}  // namespace oc
