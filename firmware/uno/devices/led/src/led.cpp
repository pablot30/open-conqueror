#include "led.h"

namespace oc {

namespace {

Rgb violet(uint8_t level) {
  Rgb c;
  c.red = level;
  c.green = 0U;
  c.blue = level;
  return c;
}

Rgb red(uint8_t level) {
  Rgb c;
  c.red = level;
  c.green = 0U;
  c.blue = 0U;
  return c;
}

// A triangle wave from 2 up to the maximum and back over one period.
uint8_t breath(uint32_t nowMs) {
  const uint32_t half = config::kLedBreathPeriodMs / 2U;
  const uint32_t phase = nowMs % config::kLedBreathPeriodMs;
  const uint32_t rising = (phase < half) ? phase : (config::kLedBreathPeriodMs - phase);
  const uint32_t level = 2U + ((rising * (config::kLedMaxLevel - 2U)) / half);
  return static_cast<uint8_t>(level);
}

bool flashOn(uint32_t nowMs, uint32_t periodMs) { return (nowMs % periodMs) < (periodMs / 2U); }

}  // namespace

Rgb ledColor(LedMode mode, uint32_t nowMs) {
  switch (mode) {
    case LedMode::kStandby:
      return violet(breath(nowMs));
    case LedMode::kDriving:
      return violet(config::kLedMaxLevel / 2U);
    case LedMode::kBatteryLow:
      return red(flashOn(nowMs, config::kLedLowFlashPeriodMs) ? config::kLedMaxLevel : 0U);
    case LedMode::kBatteryCritical:
      return red(flashOn(nowMs, config::kLedCriticalFlashPeriodMs) ? config::kLedMaxLevel : 0U);
    case LedMode::kOff:
    default:
      return violet(0U);
  }
}

}  // namespace oc
