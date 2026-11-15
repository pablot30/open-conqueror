#include "check.h"
#include "diagnostics.h"

using oc::Diagnostics;
using oc::EventId;
using oc::EventStatus;
using oc::MonitorResult;
using oc::SignalStatus;

namespace {

const MonitorResult kPass = MonitorResult::kPassed;
const MonitorResult kFail = MonitorResult::kFailed;
const EventId kBattery = EventId::kBatterySensor;  // 5 to fail, 5 to pass

void report(Diagnostics& d, EventId id, MonitorResult r, int times) {
  for (int i = 0; i < times; ++i) {
    d.report(id, r);
  }
}

void startsNotTested() {
  Diagnostics d;
  CHECK(d.status(kBattery) == EventStatus::kNotTested);
  CHECK(!d.inhibited(oc::kInhibitDrive));
}

void needsConfirmedPassesBeforeValid() {
  Diagnostics d;
  report(d, kBattery, kPass, 4);
  CHECK(d.status(kBattery) == EventStatus::kNotTested);  // still init
  d.report(kBattery, kPass);
  CHECK(d.status(kBattery) == EventStatus::kPassed);
}

void debouncesFailures() {
  Diagnostics d;
  report(d, kBattery, kPass, 5);
  d.report(kBattery, kFail);
  CHECK(d.status(kBattery) == EventStatus::kPrefailed);
  report(d, kBattery, kFail, 3);
  CHECK(d.status(kBattery) == EventStatus::kPrefailed);
  d.report(kBattery, kFail);
  CHECK(d.status(kBattery) == EventStatus::kFailed);
  CHECK(d.inhibited(oc::kInhibitDrive));
}

void aSingleGlitchDoesNotConfirm() {
  Diagnostics d;
  report(d, kBattery, kPass, 5);
  report(d, kBattery, kFail, 4);
  d.report(kBattery, kPass);  // the counter jumps back: no fault
  CHECK(d.status(kBattery) == EventStatus::kPrepassed);
  report(d, kBattery, kFail, 4);
  CHECK(d.status(kBattery) == EventStatus::kPrefailed);
}

void healsThroughPrepassed() {
  Diagnostics d;
  report(d, kBattery, kFail, 5);
  CHECK(d.status(kBattery) == EventStatus::kFailed);
  d.report(kBattery, kPass);
  CHECK(d.status(kBattery) == EventStatus::kPrepassed);
  CHECK(d.inhibited(oc::kInhibitDrive));  // reactions hold until healed
  d.report(kBattery, kFail);
  CHECK(d.status(kBattery) == EventStatus::kFailed);  // a fail while healing stays failed
  report(d, kBattery, kPass, 5);
  CHECK(d.status(kBattery) == EventStatus::kPassed);
  CHECK(!d.inhibited(oc::kInhibitDrive));
}

void unconfirmedFaultHasNoReaction() {
  Diagnostics d;
  report(d, kBattery, kFail, 4);
  CHECK(d.status(kBattery) == EventStatus::kPrefailed);
  CHECK(!d.inhibited(oc::kInhibitDrive));
  d.report(kBattery, kPass);
  CHECK(d.status(kBattery) == EventStatus::kPrepassed);
  CHECK(!d.inhibited(oc::kInhibitDrive));
}

void eventsAreIndependent() {
  Diagnostics d;
  d.report(EventId::kFrameOverrun, kFail);  // confirms in one step
  CHECK(d.status(EventId::kFrameOverrun) == EventStatus::kFailed);
  CHECK(d.status(kBattery) == EventStatus::kNotTested);
  CHECK(!d.inhibited(oc::kInhibitDrive));  // overruns have no reaction yet
}

void missingEchoesGoSubstituteThenError() {  // A5 example, T3
  Diagnostics d;
  const EventId id = EventId::kUltrasonicEcho;
  report(d, id, kPass, 3);
  CHECK(d.status(id) == EventStatus::kPassed);
  d.report(id, kFail);  // one missed echo: a substitute, no reaction
  CHECK(oc::signalStatusFor(d.status(id)) == SignalStatus::kSubstitute);
  CHECK(!d.inhibited(oc::kInhibitRanging));
  report(d, id, kFail, 4);  // several: an error, and ranging modes are unavailable
  CHECK(oc::signalStatusFor(d.status(id)) == SignalStatus::kError);
  CHECK(d.inhibited(oc::kInhibitRanging));
  CHECK(!d.inhibited(oc::kInhibitDrive));  // driving is not affected
  report(d, id, kPass, 2);  // healing: still a substitute, still unavailable
  CHECK(oc::signalStatusFor(d.status(id)) == SignalStatus::kSubstitute);
  CHECK(d.inhibited(oc::kInhibitRanging));
  d.report(id, kPass);
  CHECK(oc::signalStatusFor(d.status(id)) == SignalStatus::kValid);
  CHECK(!d.inhibited(oc::kInhibitRanging));
}

void signalStatusFollowsEvent() {  // A5 mapping
  CHECK(oc::signalStatusFor(EventStatus::kNotTested) == SignalStatus::kInit);
  CHECK(oc::signalStatusFor(EventStatus::kPassed) == SignalStatus::kValid);
  CHECK(oc::signalStatusFor(EventStatus::kPrefailed) == SignalStatus::kSubstitute);
  CHECK(oc::signalStatusFor(EventStatus::kPrepassed) == SignalStatus::kSubstitute);
  CHECK(oc::signalStatusFor(EventStatus::kFailed) == SignalStatus::kError);
}

void qualifyHoldsLastValid() {  // A5 substitute values
  uint16_t lastValid = 0U;
  oc::Signal<uint16_t> s = oc::qualify<uint16_t>(7400U, EventStatus::kPassed, lastValid, 0U);
  CHECK(s.value == 7400U && s.status == SignalStatus::kValid && lastValid == 7400U);
  s = oc::qualify<uint16_t>(50U, EventStatus::kPrefailed, lastValid, 0U);
  CHECK(s.value == 7400U && s.status == SignalStatus::kSubstitute);
  s = oc::qualify<uint16_t>(50U, EventStatus::kFailed, lastValid, 0U);
  CHECK(s.value == 0U && s.status == SignalStatus::kError);
  s = oc::qualify<uint16_t>(7300U, EventStatus::kPrepassed, lastValid, 0U);
  CHECK(s.value == 7400U && s.status == SignalStatus::kSubstitute);
  s = oc::qualify<uint16_t>(7300U, EventStatus::kNotTested, lastValid, 0U);
  CHECK(s.value == 0U && s.status == SignalStatus::kInit);
}

}  // namespace

void runUnitTests() {
  startsNotTested();
  needsConfirmedPassesBeforeValid();
  debouncesFailures();
  aSingleGlitchDoesNotConfirm();
  healsThroughPrepassed();
  unconfirmedFaultHasNoReaction();
  eventsAreIndependent();
  missingEchoesGoSubstituteThenError();
  signalStatusFollowsEvent();
  qualifyHoldsLastValid();
}
