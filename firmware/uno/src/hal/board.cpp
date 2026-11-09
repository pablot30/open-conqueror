#include "board.h"

#include <Arduino.h>

namespace oc {
namespace board {

namespace {

const uint8_t kPinModeButton = 2U;   // pulled up, low when pressed
const uint8_t kPinStandby = 3U;      // TB6612 STBY, both motors
const uint8_t kPinPwmRight = 5U;     // channel A
const uint8_t kPinPwmLeft = 6U;      // channel B
const uint8_t kPinDirRight = 7U;     // AIN1; AIN2 is its inverse
const uint8_t kPinDirLeft = 8U;      // BIN1; BIN2 is its inverse
const uint8_t kPinBattery = A3;

// Direction pin high drives the track forward. Source: the stock firmware,
// confirmed on our robot in docs/notebook/2026-11-06-motor-channels.md.
void setChannel(uint8_t dirPin, uint8_t pwmPin, int16_t pwm) {
  const bool forward = (pwm >= 0);
  const int16_t magnitude = forward ? pwm : static_cast<int16_t>(-pwm);
  digitalWrite(dirPin, forward ? HIGH : LOW);
  // PWM 0 is a short brake on the TB6612.
  analogWrite(pwmPin, (magnitude > 255) ? 255 : magnitude);
}

}  // namespace

void init() {
  digitalWrite(kPinStandby, LOW);
  pinMode(kPinStandby, OUTPUT);
  digitalWrite(kPinPwmRight, LOW);
  digitalWrite(kPinPwmLeft, LOW);
  pinMode(kPinPwmRight, OUTPUT);
  pinMode(kPinPwmLeft, OUTPUT);
  pinMode(kPinDirRight, OUTPUT);
  pinMode(kPinDirLeft, OUTPUT);
  pinMode(kPinModeButton, INPUT);  // the shield has its own pull-up
}

void setMotors(const MotorOutput& output) {
  if (!output.enabled) {
    digitalWrite(kPinStandby, LOW);
  }
  setChannel(kPinDirRight, kPinPwmRight, output.right);
  setChannel(kPinDirLeft, kPinPwmLeft, output.left);
  if (output.enabled) {
    digitalWrite(kPinStandby, HIGH);
  }
}

uint16_t readBatteryAdc() { return static_cast<uint16_t>(analogRead(kPinBattery)); }

bool modeButtonPressed() { return digitalRead(kPinModeButton) == LOW; }

}  // namespace board
}  // namespace oc
