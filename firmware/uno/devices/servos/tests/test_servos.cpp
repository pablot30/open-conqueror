#include "check.h"
#include "servos.h"

namespace {

using oc::ServoRamp;
using Action = oc::ServoRamp::Action;

const uint16_t kSpeed = oc::config::kServoDefaultSpeedDps;

// Runs frames from `first` until the ramp stops writing; returns how many
// frames wrote. Fails the test if it never finishes.
uint32_t runUntilSettled(ServoRamp& ramp, uint32_t first, uint32_t* lastFrame = nullptr) {
  uint32_t writes = 0U;
  for (uint32_t i = 0U; i < 100000U; ++i) {
    const uint32_t frame = first + i;  // wraps like the real counter
    const Action action = ramp.update(frame);
    if (action == Action::kWrite) {
      ++writes;
    } else if (action == Action::kRelease) {
      if (lastFrame != nullptr) {
        *lastFrame = frame;
      }
      return writes;
    }
  }
  CHECK(false);
  return writes;
}

void startsAtTheCentreAndIsIdle() {
  ServoRamp ramp;
  CHECK(!ramp.moving());
  CHECK(ramp.pan() == oc::config::kPanCentreDeg);
  CHECK(ramp.tilt() == oc::config::kTiltCentreDeg);
  CHECK(ramp.update(0U) == Action::kIdle);
}

void reachesTheTargetWithoutOvershooting() {  // F14
  ServoRamp ramp;
  ramp.setTarget(135U, 30U, 37U);  // a speed that doesn't divide the distance
  uint8_t lastPan = ramp.pan();
  uint8_t lastTilt = ramp.tilt();
  for (uint32_t frame = 0U; frame < 400U; ++frame) {
    ramp.update(frame);
    CHECK(ramp.pan() >= lastPan && ramp.pan() <= 135U);
    CHECK(ramp.tilt() <= lastTilt && ramp.tilt() >= 30U);
    lastPan = ramp.pan();
    lastTilt = ramp.tilt();
  }
  CHECK(ramp.pan() == 135U);
  CHECK(ramp.tilt() == 30U);
}

void takesTheRightNumberOfFrames() {
  ServoRamp ramp;
  // 90 degrees at 100 deg/s is 0.9 s: 90 frames. Each frame writes.
  ramp.setTarget(180U, 90U, 100U);
  for (uint32_t frame = 0U; frame < 89U; ++frame) {
    CHECK(ramp.update(frame) == Action::kWrite);
    CHECK(ramp.pan() < 180U);
  }
  CHECK(ramp.update(89U) == Action::kWrite);
  CHECK(ramp.pan() == 180U);
  // Half the speed takes twice as long; the longer axis decides.
  ServoRamp slow;
  slow.setTarget(0U, 90U, 50U);  // 90 degrees at 50 deg/s: 180 frames
  CHECK(runUntilSettled(slow, 0U) == 180U);
}

void speedZeroJumps() {
  ServoRamp ramp;
  ramp.setTarget(0U, 180U, 0U);
  CHECK(ramp.update(0U) == Action::kWrite);
  CHECK(ramp.pan() == 0U);
  CHECK(ramp.tilt() == 180U);
}

void aNewTargetContinuesFromTheCurrentAngle() {
  ServoRamp ramp;
  ramp.setTarget(180U, 90U, kSpeed);
  for (uint32_t frame = 0U; frame < 30U; ++frame) {
    ramp.update(frame);
  }
  CHECK(ramp.pan() == 120U);
  ramp.setTarget(0U, 90U, kSpeed);  // reverses from 120, not from 90 or 180
  ramp.update(30U);
  CHECK(ramp.pan() == 119U);
  ramp.setTarget(150U, 90U, kSpeed);
  ramp.update(31U);
  CHECK(ramp.pan() == 120U);
}

void releasesAfterSettling() {
  ServoRamp ramp;
  ramp.setTarget(100U, 90U, kSpeed);  // 10 frames
  uint32_t releaseFrame = 0U;
  runUntilSettled(ramp, 1000U, &releaseFrame);
  // The last step is on frame 1009; release comes the settle time later.
  CHECK(releaseFrame == 1009U + oc::config::kServoSettleFrames);
  CHECK(!ramp.moving());
  CHECK(ramp.update(releaseFrame + 1U) == Action::kIdle);  // only once
}

void aMoveToWhereItIsStillWritesOnce() {  // power-on centring
  ServoRamp ramp;
  ramp.setTarget(90U, 90U, 0U);
  CHECK(ramp.update(0U) == Action::kWrite);
  CHECK(runUntilSettled(ramp, 1U) == 0U);
}

void aNewTargetRestartsTheSettleTime() {
  ServoRamp ramp;
  ramp.setTarget(91U, 90U, kSpeed);
  CHECK(ramp.update(0U) == Action::kWrite);
  ramp.setTarget(92U, 90U, kSpeed);
  CHECK(ramp.update(oc::config::kServoSettleFrames) == Action::kWrite);
  CHECK(ramp.update(oc::config::kServoSettleFrames + 1U) == Action::kIdle);
}

void survivesFrameCounterWrap() {
  ServoRamp ramp;
  ramp.setTarget(95U, 90U, kSpeed);  // 5 frames
  uint32_t releaseFrame = 0U;
  runUntilSettled(ramp, 0xFFFFFFFEU, &releaseFrame);  // wraps during the move
  CHECK(releaseFrame == (0xFFFFFFFEU + 4U + oc::config::kServoSettleFrames));
}

}  // namespace

void runUnitTests() {
  startsAtTheCentreAndIsIdle();
  reachesTheTargetWithoutOvershooting();
  takesTheRightNumberOfFrames();
  speedZeroJumps();
  aNewTargetContinuesFromTheCurrentAngle();
  releasesAfterSettling();
  aMoveToWhereItIsStillWritesOnce();
  aNewTargetRestartsTheSettleTime();
  survivesFrameCounterWrap();
}
