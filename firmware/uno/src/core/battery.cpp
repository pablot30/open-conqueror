#include "battery.h"

#include "config.h"

namespace oc {

uint16_t batteryAdcToMillivolts(uint16_t adc) {
  // Vin = adc / 1023 * 5000 mV * (10 + 1.5) / 1.5
  //     = adc * 575000 / 15345
  // U, not UL: unsigned long is 64 bits on some computers, which would make
  // the expression wider than uint32_t. On the AVR, 575000U is 32 bits.
  const uint32_t millivolts = (static_cast<uint32_t>(adc) * 575000U) / 15345U;
  return static_cast<uint16_t>(millivolts);
}

void BatteryMonitor::sample(uint16_t adc) {
  const uint32_t reading = batteryAdcToMillivolts(adc);
  if (!hasSample_) {
    accumulator_ = reading << kFilterShift;
    hasSample_ = true;
  } else {
    // Exponential moving average: acc = acc - acc/16 + new
    accumulator_ = (accumulator_ - (accumulator_ >> kFilterShift)) + reading;
  }

  const uint16_t mv = millivolts();
  switch (level_) {
    case BatteryLevel::kOk:
      if (mv < config::kBatteryCriticalMv) {
        level_ = BatteryLevel::kCritical;
      } else if (mv < config::kBatteryLowMv) {
        level_ = BatteryLevel::kLow;
      } else {
        // Stay OK.
      }
      break;
    case BatteryLevel::kLow:
      if (mv < config::kBatteryCriticalMv) {
        level_ = BatteryLevel::kCritical;
      } else if (mv > config::kBatteryLowRecoverMv) {
        level_ = BatteryLevel::kOk;
      } else {
        // Stay low.
      }
      break;
    case BatteryLevel::kCritical:
    default:
      if (mv > config::kBatteryLowRecoverMv) {
        level_ = BatteryLevel::kOk;
      } else if (mv > config::kBatteryCriticalRecoverMv) {
        level_ = BatteryLevel::kLow;
      } else {
        level_ = BatteryLevel::kCritical;
      }
      break;
  }
}

uint16_t BatteryMonitor::millivolts() const {
  return static_cast<uint16_t>(accumulator_ >> kFilterShift);
}

uint16_t BatteryMonitor::pwmLimit() const {
  const uint16_t mv = millivolts();
  if (mv <= config::kMaxMotorMv) {
    return 255U;
  }
  return static_cast<uint16_t>((255UL * config::kMaxMotorMv) / mv);
}

}  // namespace oc
