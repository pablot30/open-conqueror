// The time-triggered frame: the firmware does its work once every 10 ms (T1),
// and measures how long each frame's work takes (T2). No RTOS: see
// docs/architecture.md, A2.
#ifndef OC_SCHEDULER_H
#define OC_SCHEDULER_H

#include <stdint.h>

namespace oc {

namespace config {
// T1: the control loop runs 100 times a second.
const uint32_t kFramePeriodUs = 10000U;
// T2: no frame's work may take longer than this.
const uint32_t kFrameBudgetUs = 5000U;
}  // namespace config

class FrameClock {
 public:
  // Call as often as possible. Returns true once per frame. If the firmware
  // fell more than a whole frame behind, the missed frames are skipped, not
  // run late in a burst, and counted.
  bool due(uint32_t nowUs);

  uint32_t frames() const { return frames_; }
  uint16_t skipped() const { return skipped_; }

 private:
  uint32_t next_ = 0U;
  bool started_ = false;
  uint32_t frames_ = 0U;
  uint16_t skipped_ = 0U;
};

// Keeps the longest duration seen since the last reset, saturating at
// 65535 us.
class Stopwatch {
 public:
  void record(uint32_t startUs, uint32_t endUs);
  uint16_t longestUs() const { return longestUs_; }
  void reset() { longestUs_ = 0U; }

 private:
  uint16_t longestUs_ = 0U;
};

// True every `period` frames, on the frame where frames % period == phase.
// Phases spread periodic work, such as messages, across frames.
inline bool every(uint32_t frames, uint32_t period, uint32_t phase) {
  return (frames % period) == phase;
}

}  // namespace oc

#endif  // OC_SCHEDULER_H
