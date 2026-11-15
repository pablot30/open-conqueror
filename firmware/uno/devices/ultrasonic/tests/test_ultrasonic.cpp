#include "check.h"
#include "diagnostics.h"
#include "ultrasonic.h"

using oc::PingStep;
using oc::Ranger;

namespace {

// The echo a target at this many centimetres gives: 2 x 10 x cm mm at 343 m/s.
uint32_t echoUsForCm(uint32_t cm) { return ((cm * 20U * 1000U) + 171U) / 343U; }

void convertsRuler() {  // T3: 5, 10, 30, 60, 100 and 150 cm, exactly
  const uint32_t cms[] = {5U, 10U, 30U, 60U, 100U, 150U};
  for (uint32_t cm : cms) {
    const uint16_t mm = oc::echoWidthToMm(echoUsForCm(cm));
    CHECK(mm >= (cm * 10U) - 1U);
    CHECK(mm <= (cm * 10U) + 1U);
  }
  CHECK(oc::echoWidthToMm(5831U) == 1000U);  // 1 m is 5831 us at 343 m/s
}

void limitsTheRange() {
  CHECK(oc::echoWidthToMm(38000U) == oc::config::kUltrasonicMaxMm);
  CHECK(oc::echoWidthToMm(4000000000U) == oc::config::kUltrasonicMaxMm);  // no overflow
}

void pingsOnTheFirstCallAndThenEverySixtyMs() {
  Ranger r;
  PingStep s = r.update(1000U, false, 0U);
  CHECK(s.trigger && !s.resultReady);
  CHECK(r.waiting());
  s = r.update(1010U, true, echoUsForCm(30U));
  CHECK(s.resultReady && s.echoOk && !s.trigger);
  for (uint32_t t = 1020U; t < 1060U; t += 10U) {
    CHECK(!r.update(t, false, 0U).trigger);  // not before 60 ms
  }
  CHECK(r.update(1060U, false, 0U).trigger);
}

void neverPingsFasterThanThePeriod() {
  Ranger r;
  uint32_t triggers = 0U;
  uint32_t last = 0U;
  for (uint32_t t = 0U; t < 2000U; t += 10U) {
    // An echo comes 5 ms after every ping, which the next frame sees.
    const bool echo = r.waiting() && (t > last);
    const PingStep s = r.update(t, echo, echoUsForCm(100U));
    if (s.trigger) {
      CHECK((triggers == 0U) || ((t - last) >= oc::config::kUltrasonicPingPeriodMs));
      last = t;
      ++triggers;
    }
  }
  CHECK(triggers >= 30U);  // about one every 60 ms
}

void aMissingEchoIsAFailureNotARange() {  // T3, A5
  Ranger r;
  r.update(0U, false, 0U);
  r.update(10U, true, echoUsForCm(30U));  // a good one first
  const uint16_t good = r.lastDistanceMm();
  uint32_t t = 60U;
  PingStep s = r.update(t, false, 0U);
  CHECK(s.trigger);
  for (t = 70U; t <= 110U; t += 10U) {
    s = r.update(t, false, 0U);
    CHECK(!s.resultReady);  // still inside the 50 ms timeout
  }
  s = r.update(120U, false, 0U);  // 60 ms after the ping
  CHECK(s.resultReady && !s.echoOk);
  CHECK(r.lastDistanceMm() == good);  // the distance isn't replaced by a made-up one
  CHECK(s.trigger);  // and the next ping starts at once, as the period is over
}

void tooShortAnEchoIsNoise() {
  Ranger r;
  r.update(0U, false, 0U);
  const PingStep s = r.update(10U, true, 40U);
  CHECK(s.resultReady && !s.echoOk);
  CHECK(r.lastDistanceMm() == 0U);
}

void strayEchoWhileIdleIsIgnored() {
  Ranger r;
  r.update(0U, false, 0U);
  r.update(10U, true, echoUsForCm(30U));
  const PingStep s = r.update(20U, true, echoUsForCm(60U));  // not waiting
  CHECK(!s.resultReady);
  CHECK(r.lastDistanceMm() >= 295U && r.lastDistanceMm() <= 305U);
}

void survivesTheClockWrapping() {
  Ranger r;
  const uint32_t before = 0xFFFFFFF0U;
  CHECK(r.update(before, false, 0U).trigger);
  r.update(before + 10U, true, echoUsForCm(30U));
  CHECK(!r.update(before + 50U, false, 0U).trigger);
  CHECK(r.update(before + 60U, false, 0U).trigger);  // wrapped past zero
}

// Runs the ranger and the diagnostics for a while, 10 ms frames. A target gives
// its echo 5 ms after each ping; with none, no echo comes.
struct Sim {
  oc::Diagnostics diagnostics;
  Ranger ranger;
  uint16_t lastValid = 0U;
  uint32_t nowMs = 0U;
  uint32_t pingMs = 0U;
  bool pinged = false;
  bool targetPresent = true;
  oc::Signal<uint16_t> distance = {0U, oc::SignalStatus::kInit};

  void run(uint32_t ms) {
    const uint32_t end = nowMs + ms;
    for (; nowMs < end; nowMs += 10U) {
      const bool echo = targetPresent && ranger.waiting() && pinged && ((nowMs - pingMs) >= 10U);
      const PingStep s = ranger.update(nowMs, echo, echoUsForCm(60U));
      if (s.trigger) {
        pinged = true;
        pingMs = nowMs;
      }
      if (s.resultReady) {
        diagnostics.report(oc::EventId::kUltrasonicEcho,
                           s.echoOk ? oc::MonitorResult::kPassed : oc::MonitorResult::kFailed);
      }
      distance = oc::qualify<uint16_t>(ranger.lastDistanceMm(), diagnostics.status(oc::EventId::kUltrasonicEcho),
                                       lastValid, oc::config::kUltrasonicSafeMm);
    }
  }
};

void statusFollowsEchoes() {  // A5 example, with the diagnostics service
  Sim sim;
  CHECK(sim.distance.status == oc::SignalStatus::kInit);  // before the first echo
  sim.run(400U);
  CHECK(sim.distance.status == oc::SignalStatus::kValid);
  CHECK(sim.distance.value >= 595U && sim.distance.value <= 605U);

  sim.targetPresent = false;  // a cloth in front: no echo
  sim.run(130U);  // one miss, 50 ms timeout plus the rest of the period
  CHECK(sim.distance.status == oc::SignalStatus::kSubstitute);
  CHECK(sim.distance.value >= 595U && sim.distance.value <= 605U);  // the last valid one
  sim.run(600U);
  CHECK(sim.distance.status == oc::SignalStatus::kError);
  CHECK(sim.distance.value == oc::config::kUltrasonicSafeMm);  // never 150 cm
  CHECK(sim.diagnostics.inhibited(oc::kInhibitRanging));

  sim.targetPresent = true;
  sim.run(70U);
  CHECK(sim.distance.status == oc::SignalStatus::kSubstitute);  // healing
  sim.run(200U);
  CHECK(sim.distance.status == oc::SignalStatus::kValid);
}

}  // namespace

void runUnitTests() {
  convertsRuler();
  limitsTheRange();
  pingsOnTheFirstCallAndThenEverySixtyMs();
  neverPingsFasterThanThePeriod();
  aMissingEchoIsAFailureNotARange();
  tooShortAnEchoIsNoise();
  strayEchoWhileIdleIsIgnored();
  survivesTheClockWrapping();
  statusFollowsEchoes();
}
