#include "check.h"
#include "scheduler.h"

using oc::FrameClock;
using oc::Stopwatch;

namespace {

const uint32_t kPeriod = oc::config::kFramePeriodUs;

void firstCallStartsAFrame() {  // T1
  FrameClock clock;
  CHECK(clock.due(123U));
  CHECK(!clock.due(124U));
  CHECK(clock.frames() == 1U);
}

void runsOncePerPeriod() {  // T1
  FrameClock clock;
  CHECK(clock.due(0U));
  CHECK(!clock.due(kPeriod - 1U));
  CHECK(clock.due(kPeriod));
  CHECK(!clock.due(kPeriod + 5U));
  CHECK(clock.due(2U * kPeriod + 300U));  // a little late is fine
  CHECK(clock.due(3U * kPeriod));         // and doesn't drift
  CHECK(clock.frames() == 4U);
  CHECK(clock.skipped() == 0U);
}

void skipsAndCountsMissedFrames() {  // T2
  FrameClock clock;
  CHECK(clock.due(0U));
  CHECK(clock.due(kPeriod * 4U + 10U));  // frames 2, 3 and 4 were missed
  CHECK(clock.skipped() == 3U);
  CHECK(!clock.due(kPeriod * 4U + 20U));  // no burst of late frames
  CHECK(clock.due(kPeriod * 5U));
}

void survivesMicrosWrap() {  // T1
  FrameClock clock;
  CHECK(clock.due(0xFFFFFFFFU - 100U));
  CHECK(!clock.due(0xFFFFFFFFU));
  CHECK(clock.due(kPeriod - 101U));  // wrapped past zero
  CHECK(clock.skipped() == 0U);
}

void stopwatchKeepsLongest() {  // T2
  Stopwatch watch;
  watch.record(100U, 400U);
  watch.record(500U, 600U);
  CHECK(watch.longestUs() == 300U);
  watch.record(0xFFFFFF00U, 0x00000100U);  // across the wrap: 512 us
  CHECK(watch.longestUs() == 512U);
  watch.record(0U, 200000U);
  CHECK(watch.longestUs() == 0xFFFFU);
  watch.reset();
  CHECK(watch.longestUs() == 0U);
}

void everySpreadsWork() {
  CHECK(oc::every(10U, 10U, 0U));
  CHECK(!oc::every(11U, 10U, 0U));
  CHECK(oc::every(15U, 10U, 5U));
}

}  // namespace

void runUnitTests() {
  firstCallStartsAFrame();
  runsOncePerPeriod();
  skipsAndCountsMissedFrames();
  survivesMicrosWrap();
  stopwatchKeepsLongest();
  everySpreadsWork();
}
