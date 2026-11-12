// Battery voltage, filtered, with low and critical levels (S9) and the motor
// PWM limit that keeps the motors' average voltage down (S10).
#ifndef OC_BATTERY_H
#define OC_BATTERY_H

#include <stdint.h>

namespace oc {

namespace config {
// Battery sampling period. With the filter below, the reading settles in
// about 0.3 s, so a brief dip under load doesn't trip the cutoff.
const uint32_t kBatterySamplePeriodMs = 20U;

// S9: battery thresholds in millivolts, measured at Vin after the input
// diode. Each has a recovery level above it, so the state doesn't flicker.
const uint16_t kBatteryLowMv = 7000U;
const uint16_t kBatteryLowRecoverMv = 7200U;
const uint16_t kBatteryCriticalMv = 6400U;  // proposed
const uint16_t kBatteryCriticalRecoverMv = 6800U;

// S10: the motors' average voltage stays at or below this (proposed).
const uint16_t kMaxMotorMv = 6000U;
}  // namespace config

enum class BatteryLevel : uint8_t {
  kOk = 0,
  kLow = 1,       // warn
  kCritical = 2,  // motors stop and won't drive
};

// Converts a 10-bit reading on A3 to millivolts at Vin. The divider is
// 10 kOhm over 1.5 kOhm, and the reference is the 5 V rail. No correction
// is applied yet; see docs/hardware/README.md, open questions.
uint16_t batteryAdcToMillivolts(uint16_t adc);

// Monitor for the battery sensor (A5): true if one raw reading is plausible,
// between 1 V and 10 V at Vin. An open or shorted divider reads far outside.
// With main power off, USB feeds back about 4.4 V, which counts as plausible.
bool batteryReadingPlausible(uint16_t adc);

class BatteryMonitor {
 public:
  void sample(uint16_t adc);
  uint16_t millivolts() const;
  // Critical until the first sample arrives.
  BatteryLevel level() const { return level_; }
  // The highest PWM value (0 to 255) that keeps the motors' average voltage
  // at or below config::kMaxMotorMv.
  uint16_t pwmLimit() const;

 private:
  static const uint8_t kFilterShift = 4U;  // average over about 16 samples
  uint32_t accumulator_ = 0U;
  bool hasSample_ = false;
  BatteryLevel level_ = BatteryLevel::kCritical;
};

}  // namespace oc

#endif  // OC_BATTERY_H
