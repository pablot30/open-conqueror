// Pan on D10 and tilt on D11 (docs/hardware/pin-map.md). The Servo library
// uses Timer1, which the rest of the firmware leaves free.
#include <Arduino.h>
#include <Servo.h>

#include "servos.h"

namespace oc {
namespace servos_hw {

namespace {
const uint8_t kPinPan = 10U;
const uint8_t kPinTilt = 11U;
Servo pan;
Servo tilt;
}  // namespace

void move(uint8_t panDeg, uint8_t tiltDeg) {
  if (!pan.attached()) {
    pan.attach(kPinPan);
  }
  if (!tilt.attached()) {
    tilt.attach(kPinTilt);
  }
  pan.write(panDeg);
  tilt.write(tiltDeg);
}

void release() {
  pan.detach();
  tilt.detach();
}

}  // namespace servos_hw
}  // namespace oc
