// Trigger and echo timing for the ultrasonic module (D13 trigger, D12 echo).
// The echo's rising and falling edges come from the shared pin-change
// interrupt in hal/pinchange, with micros() timestamps (4 microsecond steps
// at 16 MHz, about 0.7 mm). Nothing here waits for the echo.
#include <Arduino.h>

#include <pinchange.h>

#include "ultrasonic.h"

namespace oc {
namespace ultrasonic_hw {

namespace {

const uint8_t kTriggerPin = 13U;
const uint8_t kEchoPin = 12U;
// The module wants a trigger pulse of at least 10 microseconds. This is the
// only wait in the unit, and it is 10 microseconds.
const uint16_t kTriggerPulseUs = 10U;

volatile uint32_t riseUs = 0U;
volatile uint32_t widthUs_ = 0U;
volatile bool risen = false;
volatile bool complete = false;

// Runs in the interrupt.
void onEdge(bool high, uint32_t atUs) {
  if (high) {
    riseUs = atUs;
    risen = true;
  } else if (risen) {
    widthUs_ = atUs - riseUs;
    risen = false;
    complete = true;
  } else {
    // A falling edge with no rising one: a trigger glitch. Ignore it.
  }
}

}  // namespace

void begin() {
  pinMode(kTriggerPin, OUTPUT);
  digitalWrite(kTriggerPin, LOW);
  pinMode(kEchoPin, INPUT);
  pinchange::attach(kEchoPin, onEdge);
}

void trigger() {
  noInterrupts();
  risen = false;
  complete = false;
  interrupts();
  digitalWrite(kTriggerPin, HIGH);
  delayMicroseconds(kTriggerPulseUs);
  digitalWrite(kTriggerPin, LOW);
}

bool takeEcho(uint32_t& widthUs) {
  noInterrupts();
  const bool done = complete;
  widthUs = widthUs_;
  complete = false;
  interrupts();
  return done;
}

}  // namespace ultrasonic_hw
}  // namespace oc
