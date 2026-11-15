#include "check.h"
#include "ir.h"
#include "safety.h"

using oc::IrCode;
using oc::IrKind;
using oc::Key;
using oc::RemoteAction;
using oc::RemoteDriver;
using oc::RemoteRequest;

namespace {

// Builds a frame's pulse widths, as the receiver would report them, with a
// fixed offset on bursts (receivers stretch them) and none on spaces.
uint8_t makeFrame(uint16_t* out, uint8_t addressLow, uint8_t addressHigh, uint8_t command, int16_t markOffset) {
  const uint8_t bytes[4] = {addressLow, addressHigh, command, static_cast<uint8_t>(command ^ 0xFFU)};
  uint8_t n = 0U;
  out[n++] = static_cast<uint16_t>(9000 + markOffset);
  out[n++] = 4500U;
  for (uint8_t byte = 0U; byte < 4U; ++byte) {
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
      out[n++] = static_cast<uint16_t>(562 + markOffset);
      out[n++] = (((bytes[byte] >> bit) & 1U) != 0U) ? 1687U : 562U;
    }
  }
  out[n++] = static_cast<uint16_t>(562 + markOffset);
  return n;
}

// A frame as a receiver recorded it, with jitter: address 0x00 0xFF, command
// 0x45, bursts about 80 microseconds long.
const uint16_t kRecorded[67] = {
    9061, 4379, 632, 505,  588, 431,  687, 490,  594, 468,  656, 429,  698, 486,  609, 426,  593,
    477,  635,  1555, 612, 1558, 652, 1601, 589, 1652, 654, 1562, 610, 1627, 662, 1621, 589, 1620,
    656,  1597, 588,  450, 587,  1618, 691, 439,  619, 475,  600, 491,  597, 1620, 621, 493,  686,
    509,  605,  1560, 656, 495,  663,  1571, 629, 1559, 652, 1638, 590, 494,  589, 1626, 608};

void decodesARecordedFrame() {  // F10
  const IrCode code = oc::decodeNec(kRecorded, 67U);
  CHECK(code.kind == IrKind::kFrame);
  CHECK(code.address == 0xFF00U);
  CHECK(code.command == 0x45U);
}

void decodesEveryCommandByte() {
  uint16_t widths[oc::kIrMaxPulses];
  for (uint16_t command = 0U; command < 256U; ++command) {
    const uint8_t n = makeFrame(widths, 0x00U, 0xFFU, static_cast<uint8_t>(command), 0);
    const IrCode code = oc::decodeNec(widths, n);
    CHECK(code.kind == IrKind::kFrame && code.command == command);
  }
}

void decodesExtendedAddresses() {
  uint16_t widths[oc::kIrMaxPulses];
  const uint8_t n = makeFrame(widths, 0x34U, 0x12U, 0x07U, 0);  // not the inverse: the extended form
  const IrCode code = oc::decodeNec(widths, n);
  CHECK(code.kind == IrKind::kFrame && code.address == 0x1234U && code.command == 0x07U);
}

void toleratesStretchedAndShortBursts() {
  uint16_t widths[oc::kIrMaxPulses];
  const int16_t offsets[] = {-200, -100, 0, 100, 200};
  for (int16_t offset : offsets) {
    const uint8_t n = makeFrame(widths, 0x00U, 0xFFU, 0x18U, offset);
    CHECK(oc::decodeNec(widths, n).kind == IrKind::kFrame);
  }
}

void acceptsAFrameWithoutItsEndBurst() {
  uint16_t widths[oc::kIrMaxPulses];
  const uint8_t n = makeFrame(widths, 0x00U, 0xFFU, 0x18U, 0);
  CHECK(oc::decodeNec(widths, static_cast<uint8_t>(n - 1U)).kind == IrKind::kFrame);
  CHECK(oc::decodeNec(widths, static_cast<uint8_t>(n - 2U)).kind == IrKind::kUnreadable);  // a bit missing
}

void decodesRepeatCodes() {
  const uint16_t repeat[3] = {9010U, 2240U, 590U};
  CHECK(oc::decodeNec(repeat, 3U).kind == IrKind::kRepeat);
  CHECK(oc::decodeNec(repeat, 2U).kind == IrKind::kRepeat);
  const uint16_t tooLong[5] = {9010U, 2240U, 590U, 600U, 600U};
  CHECK(oc::decodeNec(tooLong, 5U).kind == IrKind::kUnreadable);
}

void rejectsABrokenChecksum() {
  uint16_t widths[oc::kIrMaxPulses];
  const uint8_t n = makeFrame(widths, 0x00U, 0xFFU, 0x18U, 0);
  widths[2U + (2U * 17U) + 1U] = 1687U;  // a bit of the command flips from 0 to 1
  CHECK(oc::decodeNec(widths, n).kind == IrKind::kUnreadable);
}

void rejectsNoiseAndOtherProtocols() {
  uint16_t widths[oc::kIrMaxPulses] = {};
  CHECK(oc::decodeNec(widths, 0U).kind == IrKind::kNone);
  CHECK(oc::decodeNec(nullptr, 5U).kind == IrKind::kNone);
  widths[0] = 300U;  // not a lead burst
  widths[1] = 300U;
  CHECK(oc::decodeNec(widths, 2U).kind == IrKind::kUnreadable);
  const uint16_t other[12] = {2400U, 600U, 1200U, 600U, 600U, 600U, 1200U, 600U, 600U, 600U, 1200U, 600U};
  CHECK(oc::decodeNec(other, 12U).kind == IrKind::kUnreadable);  // a Sony-style frame
  // The line has the opposite polarity: spaces first.
  uint16_t inverted[oc::kIrMaxPulses];
  const uint8_t n = makeFrame(inverted, 0x00U, 0xFFU, 0x18U, 0);
  CHECK(oc::decodeNec(inverted + 1, static_cast<uint8_t>(n - 1U)).kind == IrKind::kUnreadable);
  // A bit mark that is far too long.
  const uint8_t m = makeFrame(widths, 0x00U, 0xFFU, 0x18U, 0);
  widths[10] = 3000U;
  CHECK(oc::decodeNec(widths, m).kind == IrKind::kUnreadable);
}

void findsKeysInATableAndIgnoresUnknownEntries() {
  const oc::KeyEntry table[] = {
      {Key::kUp, 0xFF00U, 0x18U, true},
      {Key::kOk, 0xFF00U, 0x38U, true},
      {Key::kDown, 0xFF00U, 0x4AU, false},  // not recorded yet
  };
  IrCode code = {IrKind::kFrame, 0xFF00U, 0x38U};
  CHECK(oc::lookupKey(table, 3U, code) == Key::kOk);
  code.command = 0x4AU;
  CHECK(oc::lookupKey(table, 3U, code) == Key::kNone);
  code.command = 0x18U;
  code.address = 0x1234U;
  CHECK(oc::lookupKey(table, 3U, code) == Key::kNone);  // same command, other remote
  code = {IrKind::kRepeat, 0xFF00U, 0x38U};
  CHECK(oc::lookupKey(table, 3U, code) == Key::kNone);
}

void theShippedTableStartsUnrecorded() {
  // Until our remote's codes are recorded (ir_keys), no key matches, so
  // nothing can drive by mistake.
  const IrCode code = {IrKind::kFrame, 0x0000U, 0x00U};
  CHECK(oc::keyFor(code) == Key::kNone);
}

// --- Driving ---

void arrowsDriveWhileHeldAndStopThreeHundredMsAfter() {  // F10
  RemoteDriver r;
  RemoteRequest q = r.onKey(Key::kUp, 1000U);
  CHECK(q.action == RemoteAction::kDrive && q.newPress);
  CHECK(q.left == oc::config::kRemoteSpeedPercent && q.right == oc::config::kRemoteSpeedPercent);
  uint32_t last = 1000U;
  for (uint32_t t = 1108U; t < 1500U; t += 108U) {
    last = t;
    q = r.onRepeat(t);
    CHECK(q.action == RemoteAction::kDrive && !q.newPress);
    CHECK(r.update(t + 10U).action == RemoteAction::kNone);
  }
  CHECK(r.update(last + 290U).action == RemoteAction::kNone);
  CHECK(r.update(last + 300U).action == RemoteAction::kStop);  // 300 ms after the last code
  CHECK(!r.holding());
  CHECK(r.update(last + 310U).action == RemoteAction::kNone);  // once
}

void eachArrowHasItsDirection() {
  RemoteDriver r;
  RemoteRequest q = r.onKey(Key::kDown, 0U);
  CHECK(q.left < 0 && q.right < 0 && q.left == q.right);
  q = r.onKey(Key::kLeft, 50U);
  CHECK(q.left < 0 && q.right > 0);  // spin left, like `conq drive l`
  CHECK(q.newPress);                 // another key is a new press
  q = r.onKey(Key::kRight, 100U);
  CHECK(q.left > 0 && q.right < 0);
}

void okStopsAndRepeatsCannotRestart() {  // S2
  RemoteDriver r;
  r.onKey(Key::kUp, 0U);
  CHECK(r.onKey(Key::kOk, 50U).action == RemoteAction::kStop);
  CHECK(r.onRepeat(100U).action == RemoteAction::kNone);
  CHECK(r.onRepeat(200U).action == RemoteAction::kNone);
  CHECK(r.update(500U).action == RemoteAction::kNone);  // already stopped: no second stop either
  CHECK(r.onKey(Key::kUp, 600U).newPress);               // a new press drives again
}

void unknownKeysAndRepeatsWithoutAKeyDoNothing() {
  RemoteDriver r;
  CHECK(r.onKey(Key::kNone, 0U).action == RemoteAction::kNone);
  CHECK(r.onRepeat(10U).action == RemoteAction::kNone);
  CHECK(!r.holding());
}

void aResentFrameIsTheSamePressButANewOneAfterARelease() {
  RemoteDriver r;
  CHECK(r.onKey(Key::kUp, 0U).newPress);
  CHECK(!r.onKey(Key::kUp, 108U).newPress);  // remote resends whole frames
  CHECK(!r.onKey(Key::kUp, 216U).newPress);
  CHECK(r.update(516U).action == RemoteAction::kStop);
  CHECK(r.onKey(Key::kUp, 520U).newPress);
}

void survivesTheClockWrapping() {
  RemoteDriver r;
  r.onKey(Key::kUp, 0xFFFFFFF0U);
  CHECK(r.update(0x00000040U).action == RemoteAction::kNone);  // 80 ms later, wrapped
  CHECK(r.update(0x0000012CU - 0x10U).action == RemoteAction::kStop);  // 300 ms after: 0xFFFFFFF0 + 300
}

// --- With the safety supervisor ---

struct Rig {
  oc::SafetySupervisor safety;
  RemoteDriver remote;
  uint32_t nowMs = 0U;

  void apply(const RemoteRequest& q) {
    if (q.action == RemoteAction::kStop) {
      safety.stop();
    } else if (q.action == RemoteAction::kDrive) {
      if (q.newPress) {
        safety.drive(0, 0, nowMs);  // a deliberate new command arms the supervisor
      }
      safety.drive(q.left, q.right, nowMs);
    } else {
      // nothing
    }
  }
  oc::MotorOutput frame(uint32_t advanceMs) {
    nowMs += advanceMs;
    apply(remote.update(nowMs));
    return safety.update(nowMs, 255U, true);
  }
};

void drivesThroughTheSafetySupervisorAndStopsWithoutAFault() {  // F10, S3
  Rig rig;
  CHECK(rig.frame(10U).enabled == false);  // standby
  rig.apply(rig.remote.onKey(Key::kUp, rig.nowMs));  // the first press arms and drives
  oc::MotorOutput out = rig.frame(10U);
  CHECK(out.enabled && out.left > 0 && out.right > 0);
  for (int i = 0; i < 3; ++i) {  // held: repeats every 108 ms
    for (int f = 0; f < 11; ++f) {
      out = rig.frame(10U);
    }
    rig.apply(rig.remote.onRepeat(rig.nowMs));
    CHECK(out.left > 0);
  }
  for (int f = 0; f < 40; ++f) {  // released
    out = rig.frame(10U);
  }
  CHECK(out.left == 0 && out.right == 0);
  CHECK(rig.safety.state() == oc::DriveState::kStopped);
  CHECK(!rig.safety.linkTimedOut());  // a release is not a link fault
}

void stopHoldsAgainstRepeatsAndResentFrames() {  // S2
  Rig rig;
  rig.apply(rig.remote.onKey(Key::kUp, rig.nowMs));
  rig.frame(10U);
  rig.apply(rig.remote.onKey(Key::kOk, rig.nowMs));
  oc::MotorOutput out = rig.frame(10U);
  CHECK(out.left == 0 && out.right == 0);
  for (int i = 0; i < 10; ++i) {
    rig.apply(rig.remote.onRepeat(rig.nowMs));
    out = rig.frame(108U);
    CHECK(out.left == 0 && out.right == 0);
  }
}

void anotherStopSourceHoldsAgainstTheHeldKey() {  // S1 with F10
  Rig rig;
  rig.apply(rig.remote.onKey(Key::kUp, rig.nowMs));
  rig.frame(10U);
  rig.safety.stop();  // the MODE button, say
  for (int i = 0; i < 5; ++i) {
    rig.apply(rig.remote.onRepeat(rig.nowMs));  // the key is still held
    const oc::MotorOutput out = rig.frame(60U);
    CHECK(out.left == 0 && out.right == 0);
  }
  rig.frame(350U);  // released: the key stops sending codes
  rig.apply(rig.remote.onKey(Key::kUp, rig.nowMs));  // pressed again: deliberate
  CHECK(rig.frame(10U).left > 0);
}

}  // namespace

void runUnitTests() {
  decodesARecordedFrame();
  decodesEveryCommandByte();
  decodesExtendedAddresses();
  toleratesStretchedAndShortBursts();
  acceptsAFrameWithoutItsEndBurst();
  decodesRepeatCodes();
  rejectsABrokenChecksum();
  rejectsNoiseAndOtherProtocols();
  findsKeysInATableAndIgnoresUnknownEntries();
  theShippedTableStartsUnrecorded();
  arrowsDriveWhileHeldAndStopThreeHundredMsAfter();
  eachArrowHasItsDirection();
  okStopsAndRepeatsCannotRestart();
  unknownKeysAndRepeatsWithoutAKeyDoNothing();
  aResentFrameIsTheSamePressButANewOneAfterARelease();
  survivesTheClockWrapping();
  drivesThroughTheSafetySupervisorAndStopsWithoutAFault();
  stopHoldsAgainstRepeatsAndResentFrames();
  anotherStopSourceHoldsAgainstTheHeldKey();
}
