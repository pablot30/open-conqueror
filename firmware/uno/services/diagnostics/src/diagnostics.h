// Diagnostics, first iteration (docs/architecture.md, A5). Units run
// monitors and report passed or failed. This service debounces each report
// into a diagnostic event, applies reactions, and turns event states into
// signal statuses. The names follow public standards: OBD-II monitors
// (SAE J1979), the event states and counter debouncing of AUTOSAR Classic's
// Diagnostic Event Manager, and value quality as in OPC UA (IEC 62541).
// No fault memory or fault codes yet.
#ifndef OC_DIAGNOSTICS_H
#define OC_DIAGNOSTICS_H

#include <stdint.h>

namespace oc {

// What a monitor found this time.
enum class MonitorResult : uint8_t {
  kPassed = 0,
  kFailed = 1,
};

// A diagnostic event's debounced state. The numbers go out in the health
// message, so don't renumber them.
enum class EventStatus : uint8_t {
  kNotTested = 0,  // no report yet since power-on
  kPrefailed = 1,  // failing, not yet confirmed
  kFailed = 2,     // fault confirmed
  kPrepassed = 3,  // passing again, not yet healed
  kPassed = 4,     // healthy
};

// A sensor value's status. Only kValid may be acted on, except that control
// loops may use a substitute briefly. The numbers go out in telemetry.
enum class SignalStatus : uint8_t {
  kValid = 0,         // fresh and trusted
  kInit = 1,          // started, no trusted value yet
  kSubstitute = 2,    // a replacement while a fault is confirmed or heals
  kError = 3,         // fault confirmed
  kNotAvailable = 4,  // not fitted, switched off or out of service
};

// The status a signal gets from the event that monitors it.
SignalStatus signalStatusFor(EventStatus status);

// A value and its status.
template <typename T>
struct Signal {
  T value;
  SignalStatus status;
};

// Qualifies a measurement: valid values are kept as the last valid one, a
// substitute holds the last valid value, and an error or init gives the
// safe default.
template <typename T>
Signal<T> qualify(T measured, EventStatus event, T& lastValid, T safeDefault) {
  Signal<T> signal;
  signal.status = signalStatusFor(event);
  switch (signal.status) {
    case SignalStatus::kValid:
      lastValid = measured;
      signal.value = measured;
      break;
    case SignalStatus::kSubstitute:
      signal.value = lastValid;
      break;
    case SignalStatus::kInit:
    case SignalStatus::kError:
    case SignalStatus::kNotAvailable:
    default:
      signal.value = safeDefault;
      break;
  }
  return signal;
}

// Every diagnostic event. The order is the order in the health message, so
// add new events at the end, before kCount.
enum class EventId : uint8_t {
  kBatterySensor = 0,  // the battery reading is plausible
  kFrameOverrun = 1,   // each frame's work finished within its budget (T2)
  kLinkTimeout = 2,    // drive commands keep arriving while driving (S3)
  kGyroComms = 3,      // the gyro answers on I2C
  kUltrasonicEcho = 4,  // an echo of plausible width arrives after each ping (T3)
  kCount
};

// What a confirmed fault switches off. Bits, so an event can have several.
enum Reaction : uint8_t {
  kReactNone = 0x00U,
  kInhibitDrive = 0x01U,  // the safety supervisor won't drive
  kInhibitRanging = 0x02U,  // modes that need the distance (obstacle avoidance, follow) are unavailable
};

// How one event debounces and what it does when it fails.
struct EventConfig {
  uint8_t failSteps;  // consecutive failed reports to confirm a fault
  uint8_t passSteps;  // consecutive passed reports to heal it
  uint8_t reactions;  // Reaction bits applied from failed until passed again
};

class Diagnostics {
 public:
  // Counter-based debouncing. A report in the other direction first resets
  // the counter, as AUTOSAR's "jump" option does.
  void report(EventId id, MonitorResult result);

  EventStatus status(EventId id) const;
  // True if any confirmed fault has this reaction. A confirmed fault keeps
  // its reactions until it has healed (passed), not just while it's failing.
  bool inhibited(Reaction reaction) const;

  static const uint8_t kEventCount = static_cast<uint8_t>(EventId::kCount);

 private:
  int8_t counter_[kEventCount] = {};
  EventStatus status_[kEventCount] = {};
  bool confirmed_[kEventCount] = {};  // failed since the last pass, healing included
};

// The configuration of one event, from the table in diagnostics.cpp.
EventConfig eventConfig(EventId id);

}  // namespace oc

#endif  // OC_DIAGNOSTICS_H
