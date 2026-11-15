// The ultrasonic distance sensor on header P9: trigger on D13, echo on D12
// (docs/hardware/pin-map.md). ultrasonic.cpp is the logic, which the unit
// tests cover; ultrasonic_hw.cpp triggers the sensor and times the echo with
// the shared pin-change interrupt, so no frame ever waits for it (T2, T3).
//
// How the sensor works, as an HC-SR04-style module: a 10 microsecond high
// pulse on the trigger pin makes it send a burst, and the echo pin then goes
// high for as long as the sound takes there and back. Our robot's module
// has the same two pins; we haven't read its datasheet, so the numbers below
// are the usual ones for that type of module and the ruler test checks them.
#ifndef OC_ULTRASONIC_H
#define OC_ULTRASONIC_H

#include <stdint.h>

namespace oc {

namespace config {
// At most one ping this often, so one echo has died away before the next
// ping starts (a common rule of thumb is 50 to 60 ms for this type of module).
const uint32_t kUltrasonicPingPeriodMs = 60U;
// An echo that hasn't ended this long after the trigger is missing. This is
// longer than the 38 ms such modules send when nothing is in range, and
// shorter than the ping period.
const uint32_t kUltrasonicTimeoutMs = 50U;
// Echoes shorter than this are noise (about 1.7 cm, nearer than the
// module's 2 cm minimum range).
const uint32_t kUltrasonicMinEchoUs = 100U;
// Distances beyond this are reported as this, the module's usual 4 m range.
const uint16_t kUltrasonicMaxMm = 4000U;
// Added to every distance, for the ruler test (T3) to tune. Millimetres.
const int16_t kUltrasonicOffsetMm = 0;
// What the distance reads when its signal can't be trusted: touching. A
// control that ignores the status then fails towards stopping.
const uint16_t kUltrasonicSafeMm = 0U;
}  // namespace config

// Speed of sound in dry air at 20 degrees C: 343 m/s (331.3 + 0.606 x 20,
// the usual linear formula, rounded). Sound covers the distance twice, so
// one millimetre is 2 x 1000 / 343 = 5.83 microseconds of echo.
// Temperature changes it by about 0.18% per degree; we don't correct for it.
const uint32_t kSpeedOfSoundMmPerS = 343000U;

// Converts an echo's width in microseconds to millimetres, rounded and
// limited to the maximum range.
uint16_t echoWidthToMm(uint32_t widthUs);

// What one call to Ranger::update() found.
struct PingStep {
  bool trigger;      // send a ping now (the hardware layer does)
  bool resultReady;  // a ping just ended: echoOk says how
  bool echoOk;       // the monitor result: an echo of plausible width arrived in time
};

// Decides when to ping and what an echo means. Call every frame, with the
// echo the hardware layer has finished timing, if any.
class Ranger {
 public:
  PingStep update(uint32_t nowMs, bool echoComplete, uint32_t echoWidthUs);

  // The last distance a plausible echo gave, in millimetres. Zero until the
  // first one. Say how much to trust it with the signal status, not here.
  uint16_t lastDistanceMm() const { return distanceMm_; }
  bool waiting() const { return waiting_; }

 private:
  bool started_ = false;
  bool waiting_ = false;
  uint32_t triggeredMs_ = 0U;
  uint16_t distanceMm_ = 0U;
};

namespace ultrasonic_hw {
// Sets the pins and attaches the echo to the pin-change interrupt.
void begin();
// Sends the trigger pulse (10 microseconds) and clears any old echo.
void trigger();
// True once an echo has risen and fallen since the last trigger, with its
// width in microseconds. Reading it clears it.
bool takeEcho(uint32_t& widthUs);
}  // namespace ultrasonic_hw

}  // namespace oc

#endif  // OC_ULTRASONIC_H
