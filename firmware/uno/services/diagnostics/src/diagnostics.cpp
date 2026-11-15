#include "diagnostics.h"

namespace oc {

namespace {

// One row per EventId, in the same order.
const EventConfig kEvents[Diagnostics::kEventCount] = {
    // kBatterySensor: 5 bad samples (100 ms) confirm it. Without a battery
    // reading the cutoff (S9) can't work, so the robot doesn't drive.
    {5U, 5U, kInhibitDrive},
    // kFrameOverrun: reported, no reaction yet.
    {1U, 100U, kReactNone},
    // kLinkTimeout: the safety supervisor already stops (S3); this records it.
    {1U, 1U, kReactNone},
    // kGyroComms: 3 failed reads (30 ms) confirm it, 10 good ones heal it.
    // Heading hold will react to it; nothing else needs the gyro yet.
    {3U, 10U, kReactNone},
    // kUltrasonicEcho: one ping a 60 ms, so 5 missed echoes (about 300 ms)
    // confirm it, and 3 good ones (about 180 ms) heal it. Until then the
    // distance is a substitute, then an error. It reads no echo for soft
    // targets such as cloth, so this can fail with the sensor working.
    {5U, 3U, kInhibitRanging},
};

int8_t clampSteps(uint8_t steps) {
  if (steps == 0U) {
    return 1;
  }
  return (steps > 127U) ? static_cast<int8_t>(127) : static_cast<int8_t>(steps);
}

}  // namespace

SignalStatus signalStatusFor(EventStatus status) {
  switch (status) {
    case EventStatus::kPassed:
      return SignalStatus::kValid;
    case EventStatus::kPrefailed:
    case EventStatus::kPrepassed:
      return SignalStatus::kSubstitute;
    case EventStatus::kFailed:
      return SignalStatus::kError;
    case EventStatus::kNotTested:
    default:
      return SignalStatus::kInit;
  }
}

EventConfig eventConfig(EventId id) { return kEvents[static_cast<uint8_t>(id)]; }

void Diagnostics::report(EventId id, MonitorResult result) {
  const uint8_t index = static_cast<uint8_t>(id);
  if (index >= kEventCount) {
    return;
  }
  const EventConfig config = kEvents[index];
  int8_t& counter = counter_[index];
  EventStatus& status = status_[index];

  if (result == MonitorResult::kFailed) {
    const int8_t limit = clampSteps(config.failSteps);
    if (counter < 0) {
      counter = 0;
    }
    if (counter < limit) {
      ++counter;
    }
    if ((counter >= limit) || confirmed_[index]) {
      // Confirmed, or a fault that was confirmed fails again while healing.
      status = EventStatus::kFailed;
      confirmed_[index] = true;
    } else {
      status = EventStatus::kPrefailed;
    }
  } else {
    const int8_t limit = clampSteps(config.passSteps);
    if (counter > 0) {
      counter = 0;
    }
    if (counter > -limit) {
      --counter;
    }
    if (counter <= -limit) {
      status = EventStatus::kPassed;
      confirmed_[index] = false;
    } else if ((status == EventStatus::kFailed) || (status == EventStatus::kPrefailed) ||
               (status == EventStatus::kPrepassed)) {
      status = EventStatus::kPrepassed;
    } else {
      // Stays passed, or stays not tested until the first pass is confirmed,
      // so a signal is init until it can be trusted.
    }
  }
}

EventStatus Diagnostics::status(EventId id) const {
  const uint8_t index = static_cast<uint8_t>(id);
  return (index < kEventCount) ? status_[index] : EventStatus::kNotTested;
}

bool Diagnostics::inhibited(Reaction reaction) const {
  for (uint8_t i = 0U; i < kEventCount; ++i) {
    if (confirmed_[i] && ((kEvents[i].reactions & reaction) != 0U)) {
      return true;
    }
  }
  return false;
}

}  // namespace oc
