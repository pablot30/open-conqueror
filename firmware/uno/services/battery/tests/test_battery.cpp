#include "battery.h"
#include "check.h"

using oc::BatteryLevel;
using oc::BatteryMonitor;

namespace {

// The ADC reading that gives roughly this many millivolts at Vin.
uint16_t adcFor(uint32_t millivolts) {
  return static_cast<uint16_t>((millivolts * 15345UL + 287500UL) / 575000UL);
}

void settle(BatteryMonitor& battery, uint32_t millivolts) {
  for (int i = 0; i < 200; ++i) {
    battery.sample(adcFor(millivolts));
  }
}

void convertsAdc() {
  CHECK(oc::batteryAdcToMillivolts(0U) == 0U);
  CHECK(oc::batteryAdcToMillivolts(1023U) == 38333U);
  CHECK(oc::batteryAdcToMillivolts(200U) == 7494U);
}

void criticalBeforeFirstSample() {  // S9, S5
  BatteryMonitor battery;
  CHECK(battery.level() == BatteryLevel::kCritical);
}

void firstSampleSetsLevel() {
  BatteryMonitor battery;
  battery.sample(adcFor(8000U));
  CHECK(battery.level() == BatteryLevel::kOk);
  CHECK(battery.millivolts() > 7950U);
}

void levelsHaveHysteresis() {  // S9
  BatteryMonitor battery;
  settle(battery, 8000U);
  settle(battery, 6900U);
  CHECK(battery.level() == BatteryLevel::kLow);
  settle(battery, 7100U);  // above low, below recovery
  CHECK(battery.level() == BatteryLevel::kLow);
  settle(battery, 6300U);
  CHECK(battery.level() == BatteryLevel::kCritical);
  settle(battery, 6600U);  // above critical, below recovery
  CHECK(battery.level() == BatteryLevel::kCritical);
  settle(battery, 6900U);
  CHECK(battery.level() == BatteryLevel::kLow);
  settle(battery, 7300U);
  CHECK(battery.level() == BatteryLevel::kOk);
}

void briefDipDoesNotCut() {  // S9
  BatteryMonitor battery;
  settle(battery, 7600U);
  // Two samples (40 ms) at a much lower voltage, as under a motor start.
  battery.sample(adcFor(5500U));
  battery.sample(adcFor(5500U));
  CHECK(battery.level() != BatteryLevel::kCritical);
}

void pwmLimitKeepsMotorsAtSixVolts() {  // S10
  BatteryMonitor battery;
  settle(battery, 8400U);
  const uint16_t limit = battery.pwmLimit();
  CHECK(limit >= 180U);
  CHECK(limit <= 183U);
  // Average motor voltage = Vin * limit / 255
  CHECK((static_cast<uint32_t>(battery.millivolts()) * limit) / 255U <= 6000U);
  settle(battery, 5000U);
  CHECK(battery.pwmLimit() == 255U);
}

void plausibilityMonitor() {  // A5
  CHECK(!oc::batteryReadingPlausible(0U));      // open divider
  CHECK(!oc::batteryReadingPlausible(1023U));   // shorted, or 38 V
  CHECK(oc::batteryReadingPlausible(adcFor(7400U)));
  CHECK(oc::batteryReadingPlausible(adcFor(4400U)));  // USB only
  CHECK(!oc::batteryReadingPlausible(adcFor(500U)));
  CHECK(!oc::batteryReadingPlausible(adcFor(12000U)));
}

}  // namespace

void runUnitTests() {
  plausibilityMonitor();
  convertsAdc();
  criticalBeforeFirstSample();
  firstSampleSetsLevel();
  levelsHaveHysteresis();
  briefDipDoesNotCut();
  pwmLimitKeepsMotorsAtSixVolts();
}
