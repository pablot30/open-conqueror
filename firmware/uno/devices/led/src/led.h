// The status LED, one WS2812 on D4: violet breathing in standby (F5),
// steady violet while driving, red flashing on a low battery (F15) and
// faster on a critical one. led.cpp is the patterns; led_hw.cpp sends them.
#ifndef OC_LED_H
#define OC_LED_H

#include <stdint.h>

namespace oc {

namespace config {
// At most 64 of 255 per colour: bright enough indoors, and gentle on the
// UNO's 5 V rail, which feeds everything except the motors.
const uint8_t kLedMaxLevel = 64U;
const uint32_t kLedBreathPeriodMs = 3000U;
const uint32_t kLedLowFlashPeriodMs = 1000U;       // F15: 1 Hz
const uint32_t kLedCriticalFlashPeriodMs = 250U;   // 4 Hz
}  // namespace config

struct Rgb {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

inline bool operator==(const Rgb& a, const Rgb& b) {
  return (a.red == b.red) && (a.green == b.green) && (a.blue == b.blue);
}

enum class LedMode : uint8_t {
  kOff,
  kStandby,          // F5
  kDriving,
  kBatteryLow,       // F15
  kBatteryCritical,  // F15, faster
};

// The colour for a mode at this moment.
Rgb ledColor(LedMode mode, uint32_t nowMs);

namespace led_hw {
void begin();
// Sends one colour to the LED. Interrupts are off for about 30 us.
void show(const Rgb& color);
}  // namespace led_hw

}  // namespace oc

#endif  // OC_LED_H
