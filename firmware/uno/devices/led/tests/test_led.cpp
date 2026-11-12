#include "check.h"
#include "led.h"

using oc::LedMode;
using oc::Rgb;

namespace {

const uint8_t kMax = oc::config::kLedMaxLevel;

void standbyBreathesViolet() {  // F5
  const Rgb dim = oc::ledColor(LedMode::kStandby, 0U);
  const Rgb bright = oc::ledColor(LedMode::kStandby, oc::config::kLedBreathPeriodMs / 2U);
  const Rgb again = oc::ledColor(LedMode::kStandby, oc::config::kLedBreathPeriodMs);
  CHECK(dim.red == dim.blue && dim.green == 0U);
  CHECK(dim.red == 2U);
  CHECK(bright.red == kMax && bright.blue == kMax && bright.green == 0U);
  CHECK(again == dim);
}

void lowBatteryFlashesRed() {  // F15
  const Rgb on = oc::ledColor(LedMode::kBatteryLow, 0U);
  const Rgb off = oc::ledColor(LedMode::kBatteryLow, oc::config::kLedLowFlashPeriodMs / 2U);
  CHECK(on.red == kMax && on.green == 0U && on.blue == 0U);
  CHECK(off.red == 0U && off.green == 0U && off.blue == 0U);
}

void criticalFlashesFaster() {  // F15
  CHECK(oc::config::kLedCriticalFlashPeriodMs < oc::config::kLedLowFlashPeriodMs);
  CHECK(oc::ledColor(LedMode::kBatteryCritical, 0U).red == kMax);
  CHECK(oc::ledColor(LedMode::kBatteryCritical, oc::config::kLedCriticalFlashPeriodMs / 2U).red == 0U);
}

void neverBrighterThanTheLimit() {
  for (uint32_t t = 0U; t < 6000U; t += 7U) {
    const Rgb c = oc::ledColor(LedMode::kStandby, t);
    CHECK(c.red <= kMax && c.blue <= kMax && c.green == 0U);
  }
}

void drivingAndOff() {
  const Rgb driving = oc::ledColor(LedMode::kDriving, 1234U);
  CHECK(driving.red == kMax / 2U && driving.blue == kMax / 2U && driving.green == 0U);
  const Rgb off = oc::ledColor(LedMode::kOff, 1234U);
  CHECK(off.red == 0U && off.green == 0U && off.blue == 0U);
}

}  // namespace

void runUnitTests() {
  standbyBreathesViolet();
  lowBatteryFlashesRed();
  criticalFlashesFaster();
  neverBrighterThanTheLimit();
  drivingAndOff();
}
