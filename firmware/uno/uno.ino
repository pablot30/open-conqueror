// open-conqueror UNO firmware, increment 1: the walking skeleton.
// Drives both tracks from the computer, stops on command, on the MODE button
// and when the link goes quiet, and reports its state. See firmware/README.md.

#include <avr/wdt.h>

#include "src/core/battery.h"
#include "src/core/config.h"
#include "src/core/debounce.h"
#include "src/core/drive.h"
#include "src/core/protocol.h"
#include "src/hal/board.h"
#include "src/version.h"

namespace {

oc::Parser parser;
oc::DriveControl driveControl;
oc::BatteryMonitor battery;
oc::Debouncer modeButton(oc::config::kButtonDebounceMs);

uint32_t lastBatterySampleMs = 0U;
uint32_t lastTelemetryMs = 0U;
uint16_t longestLoopUs = 0U;
oc::MotorOutput lastOutput = {false, 0, 0};

// Sends a message only if it fits in the transmit buffer, so the loop never
// waits on the serial port (T2).
void send(const oc::MessageWriter& message) {
  const uint8_t length = message.length();
  // One more byte for the newline, which makes the output readable in a
  // serial monitor. Parsers ignore it, because it's outside the braces.
  if ((length > 0U) && (Serial.availableForWrite() > static_cast<int>(length))) {
    Serial.write(message.data(), length);
    Serial.write('\n');
  }
}

void sendVersion() {
  oc::MessageWriter message('V');
  message.field(OC_FIRMWARE_VERSION);
  message.field(OC_GIT_COMMIT);
  message.finish();
  send(message);
}

void sendError() {
  oc::MessageWriter message('E');
  message.field(static_cast<int32_t>(1));  // 1: message not understood
  message.finish();
  send(message);
}

uint8_t faultFlags() {
  uint8_t flags = 0U;
  if (driveControl.linkTimedOut()) {
    flags |= 0x01U;
  }
  if (battery.level() == oc::BatteryLevel::kLow) {
    flags |= 0x02U;
  }
  if (battery.level() == oc::BatteryLevel::kCritical) {
    flags |= 0x04U;
  }
  return flags;
}

void sendTelemetry(uint32_t nowMs) {
  oc::MessageWriter message('T');
  message.field(static_cast<int32_t>(nowMs));
  message.field(static_cast<int32_t>(driveControl.state()));
  message.field(lastOutput.left);
  message.field(lastOutput.right);
  message.field(static_cast<int32_t>(battery.millivolts()));
  message.field(static_cast<int32_t>(faultFlags()));
  message.field(static_cast<int32_t>(longestLoopUs));
  message.finish();
  send(message);
  longestLoopUs = 0U;
}

void handle(const oc::Command& command, uint32_t nowMs) {
  switch (command.type) {
    case oc::CommandType::kDrive:
      driveControl.drive(command.left, command.right, nowMs);
      break;
    case oc::CommandType::kStop:
      driveControl.stop();
      break;
    case oc::CommandType::kVersion:
      sendVersion();
      break;
    case oc::CommandType::kInvalid:
      sendError();
      break;
    case oc::CommandType::kNone:
    default:
      break;
  }
}

}  // namespace

void setup() {
  oc::board::init();
  Serial.begin(9600);  // the stock camera bridge's rate
  battery.sample(oc::board::readBatteryAdc());
  sendVersion();
  wdt_enable(WDTO_250MS);  // S4
}

void loop() {
  wdt_reset();
  const uint32_t startUs = micros();
  const uint32_t nowMs = millis();

  while (Serial.available() > 0) {
    handle(parser.feed(static_cast<char>(Serial.read())), nowMs);
  }

  if (modeButton.update(oc::board::modeButtonPressed(), nowMs)) {
    driveControl.stop();  // S1
  }

  if ((nowMs - lastBatterySampleMs) >= oc::config::kBatterySamplePeriodMs) {
    lastBatterySampleMs = nowMs;
    battery.sample(oc::board::readBatteryAdc());
  }

  lastOutput = driveControl.update(nowMs, battery.pwmLimit(),
                                   battery.level() != oc::BatteryLevel::kCritical);
  oc::board::setMotors(lastOutput);

  if ((nowMs - lastTelemetryMs) >= oc::config::kTelemetryPeriodMs) {
    lastTelemetryMs = nowMs;
    sendTelemetry(nowMs);
  }

  const uint32_t elapsedUs = micros() - startUs;
  if (elapsedUs > longestLoopUs) {
    longestLoopUs = (elapsedUs > 0xFFFFU) ? 0xFFFFU : static_cast<uint16_t>(elapsedUs);
  }
}
