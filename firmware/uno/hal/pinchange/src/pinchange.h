// Shared pin-change interrupt for the UNO's port B pins D8 to D13 (PCINT0
// vector). The ultrasonic echo (D12) and the IR receiver (D9) are both on
// port B, and a vector can only be defined once, so this unit owns it and
// passes each edge to the handler registered for its pin. Handlers run
// inside the interrupt: keep them to a few instructions.
//
// Pin to port bit: ATmega328P datasheet, "Alternate functions of port B"
// and the Arduino UNO pin mapping (D8 is PB0 ... D13 is PB5).
#ifndef OC_PINCHANGE_H
#define OC_PINCHANGE_H

#include <stdint.h>

namespace oc {
namespace pinchange {

// Called on every edge of one pin: the new level and micros() at the edge.
typedef void (*EdgeHandler)(bool high, uint32_t atUs);

// Registers a handler for one of D8 to D13 and enables its interrupt.
// Returns false for any other pin or if the pin already has a handler.
bool attach(uint8_t arduinoPin, EdgeHandler handler);

}  // namespace pinchange
}  // namespace oc

#endif  // OC_PINCHANGE_H
