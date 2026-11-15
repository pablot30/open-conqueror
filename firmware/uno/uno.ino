// open-conqueror UNO firmware. Drives both tracks from the computer, stops on
// command, on the MODE button and when the link goes quiet, and reports its
// state and health. Work runs in 10 ms frames. See firmware/README.md.
// This file only connects the units; each unit lives in its own folder.

#include <avr/wdt.h>

#include <battery.h>
#include <board.h>
#include <button.h>
#include <diagnostics.h>
#include <gyro.h>
#include <led.h>
#include <link.h>
#include <safety.h>
#include <scheduler.h>
#include <servos.h>

#include "src/version.h"

namespace {

// The link protocol's version, sent in the version message. 1 was the
// walking skeleton; 2 added the battery status to telemetry, the health
// message, and this field; 3 adds the sensor message;
// 4 adds the optional speed in the camera message.
const int32_t kProtocolVersion = 4;

// Periodic work, in frames of 10 ms. Phases keep messages apart.
const uint32_t kBatteryEveryFrames = 2U;     // 20 ms
const uint32_t kTelemetryEveryFrames = 10U;  // 100 ms
const uint32_t kHealthEveryFrames = 100U;    // 1 s
const uint32_t kHealthPhase = 5U;            // between two telemetry messages
const uint32_t kSensorsEveryFrames = 20U;    // 200 ms
const uint32_t kSensorsPhase = 15U;
const uint32_t kLedEveryFrames = 2U;         // 50 updates a second
const uint32_t kLedPhase = 1U;
const uint32_t kGyroRetryFrames = 100U;      // try to wake a missing gyro once a second

// MODE button debounce time.
const uint32_t kModeButtonDebounceMs = 30U;

oc::FrameClock frameClock;
oc::Stopwatch frameWork;
oc::Parser parser;
oc::SafetySupervisor safety;
oc::BatteryMonitor battery;
oc::Diagnostics diagnostics;
oc::Debouncer modeButton(kModeButtonDebounceMs);
oc::HeadingGyro gyro;
oc::ServoRamp servoRamp;

bool gyroReady = false;
oc::Rgb ledShown = {1U, 1U, 1U};  // not a real colour, so the first update is sent

oc::MotorOutput lastOutput = {false, 0, 0};

// Sends a message only if it fits in the transmit buffer, so a frame never
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
  message.field(kProtocolVersion);
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
  if (safety.linkTimedOut()) {
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
  const oc::SignalStatus batteryStatus =
      oc::signalStatusFor(diagnostics.status(oc::EventId::kBatterySensor));
  oc::MessageWriter message('T');
  message.field(static_cast<int32_t>(nowMs));
  message.field(static_cast<int32_t>(safety.state()));
  message.field(lastOutput.left);
  message.field(lastOutput.right);
  message.field(static_cast<int32_t>(battery.millivolts()));
  message.field(static_cast<int32_t>(faultFlags()));
  message.field(static_cast<int32_t>(frameWork.longestUs()));
  message.field(static_cast<int32_t>(batteryStatus));
  message.finish();
  send(message);
  frameWork.reset();
}

// {H,skipped,event0,event1,...}: frames skipped since power-on, then every
// diagnostic event's state in EventId order.
void sendHealth() {
  oc::MessageWriter message('H');
  message.field(static_cast<int32_t>(frameClock.skipped()));
  for (uint8_t i = 0U; i < oc::Diagnostics::kEventCount; ++i) {
    message.field(static_cast<int32_t>(diagnostics.status(static_cast<oc::EventId>(i))));
  }
  message.finish();
  send(message);
}

void handle(const oc::Command& command, uint32_t nowMs) {
  switch (command.type) {
    case oc::CommandType::kDrive:
      safety.drive(command.left, command.right, nowMs);
      break;
    case oc::CommandType::kStop:
      safety.stop();
      break;
    case oc::CommandType::kVersion:
      sendVersion();
      break;
    case oc::CommandType::kCamera:
      servoRamp.setTarget(command.pan, command.tilt,
                          command.hasSpeed ? command.speed : oc::config::kServoDefaultSpeedDps);
      break;
    case oc::CommandType::kInvalid:
      sendError();
      break;
    case oc::CommandType::kNone:
    default:
      break;
  }
}

oc::MonitorResult resultOf(bool passed) {
  return passed ? oc::MonitorResult::kPassed : oc::MonitorResult::kFailed;
}

oc::SignalStatus gyroStatus() {
  if (!gyro.calibrated()) {
    return oc::SignalStatus::kInit;
  }
  return oc::signalStatusFor(diagnostics.status(oc::EventId::kGyroComms));
}

// {G,heading,rate,gyroStatus}: heading in hundredths of a degree since
// power-on, turn rate in hundredths of a degree per second.
void sendSensors() {
  oc::MessageWriter message('G');
  message.field(static_cast<int32_t>(gyro.headingCdeg()));
  message.field(gyro.rateCdps());
  message.field(static_cast<int32_t>(gyroStatus()));
  message.finish();
  send(message);
}

void updateGyro(uint32_t frames) {
  if (!gyroReady) {
    if (oc::every(frames, kGyroRetryFrames, 0U)) {
      gyroReady = oc::gyro_hw::begin();
    }
    diagnostics.report(oc::EventId::kGyroComms, resultOf(gyroReady));
    return;
  }
  int16_t raw = 0;
  const bool ok = oc::gyro_hw::readZ(raw);
  diagnostics.report(oc::EventId::kGyroComms, resultOf(ok));
  if (ok) {
    gyro.update(raw);
  } else {
    gyro.missedFrame();  // keeps the last rate, as a substitute
  }
}

oc::LedMode ledMode() {
  if (battery.level() == oc::BatteryLevel::kCritical) {
    return oc::LedMode::kBatteryCritical;
  }
  if (battery.level() == oc::BatteryLevel::kLow) {
    return oc::LedMode::kBatteryLow;
  }
  if (safety.state() == oc::DriveState::kDriving) {
    return oc::LedMode::kDriving;
  }
  return oc::LedMode::kStandby;
}

void updateLed(uint32_t nowMs) {
  const oc::Rgb color = oc::ledColor(ledMode(), nowMs);
  if (!(color == ledShown)) {
    oc::led_hw::show(color);
    ledShown = color;
  }
}

// Moves the camera one step and releases the servos once they've settled.
void updateServos(uint32_t frames) {
  switch (servoRamp.update(frames)) {
    case oc::ServoRamp::Action::kWrite:
      oc::servos_hw::move(servoRamp.pan(), servoRamp.tilt());
      break;
    case oc::ServoRamp::Action::kRelease:
      oc::servos_hw::release();
      break;
    case oc::ServoRamp::Action::kIdle:
      break;
  }
}

// One frame's work.
void runFrame(uint32_t frames, uint32_t nowMs) {
  while (Serial.available() > 0) {
    handle(parser.feed(static_cast<char>(Serial.read())), nowMs);
  }

  if (modeButton.update(oc::board::modeButtonPressed(), nowMs)) {
    safety.stop();  // S1
  }

  if (oc::every(frames, kBatteryEveryFrames, 0U)) {
    const uint16_t adc = oc::board::readBatteryAdc();
    battery.sample(adc);
    diagnostics.report(oc::EventId::kBatterySensor, resultOf(oc::batteryReadingPlausible(adc)));
  }

  const bool driveAllowed = (battery.level() != oc::BatteryLevel::kCritical) &&
                            !diagnostics.inhibited(oc::kInhibitDrive);
  lastOutput = safety.update(nowMs, battery.pwmLimit(), driveAllowed);
  oc::board::setMotors(lastOutput.enabled, lastOutput.left, lastOutput.right);
  diagnostics.report(oc::EventId::kLinkTimeout, resultOf(!safety.linkTimedOut()));

  updateGyro(frames);
  updateServos(frames);
  if (oc::every(frames, kLedEveryFrames, kLedPhase)) {
    updateLed(nowMs);
  }

  if (oc::every(frames, kTelemetryEveryFrames, 0U)) {
    sendTelemetry(nowMs);
  }
  if (oc::every(frames, kHealthEveryFrames, kHealthPhase)) {
    sendHealth();
  }
  if (oc::every(frames, kSensorsEveryFrames, kSensorsPhase)) {
    sendSensors();
  }
}

}  // namespace

void setup() {
  oc::board::init();
  Serial.begin(9600);  // the stock camera bridge's rate
  battery.sample(oc::board::readBatteryAdc());
  oc::led_hw::begin();
  // F14: jump to the centre; the first frame writes it.
  servoRamp.setTarget(oc::config::kPanCentreDeg, oc::config::kTiltCentreDeg, 0U);
  gyroReady = oc::gyro_hw::begin();  // F13: calibration runs in the first frames
  sendVersion();
  wdt_enable(WDTO_250MS);  // S4
}

void loop() {
  wdt_reset();
  const uint32_t startUs = micros();
  if (!frameClock.due(startUs)) {
    return;
  }
  runFrame(frameClock.frames(), millis());
  const uint32_t endUs = micros();
  frameWork.record(startUs, endUs);
  diagnostics.report(oc::EventId::kFrameOverrun,
                     resultOf((endUs - startUs) <= oc::config::kFrameBudgetUs));
}
