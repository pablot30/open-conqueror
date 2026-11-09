// Battery voltage, filtered, with low and critical levels (S9) and the motor
// PWM limit that keeps the motors' average voltage down (S10).
#ifndef OC_CORE_BATTERY_H
#define OC_CORE_BATTERY_H

#include <stdint.h>

namespace oc {

enum class BatteryLevel : uint8_t {
  kOk = 0,
  kLow = 1,       // warn
  kCritical = 2,  // motors stop and won't drive
};

// Converts a 10-bit reading on A3 to millivolts at Vin. The divider is
// 10 kOhm over 1.5 kOhm, and the reference is the 5 V rail. No correction
// is applied yet; see docs/hardware/README.md, open questions.
uint16_t batteryAdcToMillivolts(uint16_t adc);

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

#endif  // OC_CORE_BATTERY_H
