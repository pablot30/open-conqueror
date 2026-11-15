// The PCINT0 vector. Written for the ATmega328P: PCICR bit 0 enables the
// port B group, PCMSK0 selects pins, and PINB holds the pin levels
// (datasheet, "External interrupts"). Timer0 stays with millis() and the
// motor PWM, and Timer1 with the Servo library: no timer is used here.
#include <Arduino.h>

#include "pinchange.h"

namespace oc {
namespace pinchange {

namespace {

const uint8_t kFirstPin = 8U;
const uint8_t kPinCount = 6U;  // D8 to D13

EdgeHandler handlers[kPinCount] = {};
volatile uint8_t lastLevels = 0U;  // PINB at the previous interrupt

}  // namespace

bool attach(uint8_t arduinoPin, EdgeHandler handler) {
  if ((arduinoPin < kFirstPin) || (arduinoPin >= (kFirstPin + kPinCount)) || (handler == nullptr)) {
    return false;
  }
  const uint8_t bit = static_cast<uint8_t>(arduinoPin - kFirstPin);
  if (handlers[bit] != nullptr) {
    return false;
  }
  const uint8_t saved = SREG;
  noInterrupts();
  handlers[bit] = handler;
  lastLevels = PINB;
  PCMSK0 = static_cast<uint8_t>(PCMSK0 | _BV(bit));
  PCICR = static_cast<uint8_t>(PCICR | _BV(PCIE0));
  SREG = saved;
  return true;
}

}  // namespace pinchange
}  // namespace oc

ISR(PCINT0_vect) {
  const uint32_t atUs = micros();
  const uint8_t levels = PINB;
  const uint8_t changed = static_cast<uint8_t>(levels ^ oc::pinchange::lastLevels);
  oc::pinchange::lastLevels = levels;
  for (uint8_t bit = 0U; bit < oc::pinchange::kPinCount; ++bit) {
    if (((changed & _BV(bit)) != 0U) && (oc::pinchange::handlers[bit] != nullptr)) {
      oc::pinchange::handlers[bit]((levels & _BV(bit)) != 0U, atUs);
    }
  }
}
