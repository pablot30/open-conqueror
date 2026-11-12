#include "check.h"
#include "gyro.h"

using oc::HeadingGyro;

namespace {

void calibrate(HeadingGyro& gyro, int16_t zeroRate) {
  for (uint8_t i = 0U; i < oc::config::kGyroCalibrationFrames; ++i) {
    gyro.update(static_cast<int16_t>(zeroRate + ((i % 2U) == 0U ? 3 : -3)));
  }
}

void calibratesWhenStill() {  // F13
  HeadingGyro gyro;
  CHECK(!gyro.calibrated());
  calibrate(gyro, -40);
  CHECK(gyro.calibrated());
  CHECK(gyro.calibrationRestarts() == 0U);
  gyro.update(-40);  // the zero rate reads as no turn
  CHECK(gyro.rateCdps() == 0);
}

void restartsIfMoved() {  // F13
  HeadingGyro gyro;
  for (uint8_t i = 0U; i < oc::config::kGyroCalibrationFrames; ++i) {
    gyro.update(i < 50U ? static_cast<int16_t>(0) : static_cast<int16_t>(500));
  }
  CHECK(!gyro.calibrated());
  CHECK(gyro.calibrationRestarts() == 1U);
  calibrate(gyro, 10);
  CHECK(gyro.calibrated());
}

void convertsRate() {  // T4
  HeadingGyro gyro;
  calibrate(gyro, 20);
  gyro.update(20 + 131);  // 1 degree/s
  CHECK(gyro.rateCdps() == 100);
  gyro.update(20 - 1310);  // -10 degrees/s
  CHECK(gyro.rateCdps() == -1000);
}

void integratesHeading() {  // F1
  HeadingGyro gyro;
  calibrate(gyro, 0);
  for (int i = 0; i < 100; ++i) {
    gyro.update(131 * 90);  // 90 degrees/s for 1 s
  }
  CHECK(gyro.headingCdeg() == 9000U);
  for (int i = 0; i < 200; ++i) {
    gyro.update(-131 * 90);  // back 180 degrees: wraps to 270
  }
  CHECK(gyro.headingCdeg() == 27000U);
  gyro.resetHeading();
  CHECK(gyro.headingCdeg() == 0U);
}

void resolvesSlowDrift() {  // T4: 0.5 degrees/s must show up
  HeadingGyro gyro;
  calibrate(gyro, 0);
  for (int i = 0; i < 1000; ++i) {
    gyro.update(i % 2 == 0 ? 66 : 65);  // about 0.5 degrees/s for 10 s
  }
  const uint16_t heading = gyro.headingCdeg();
  CHECK(heading >= 490U && heading <= 510U);  // about 5 degrees
}

void missedFrameKeepsLastRate() {  // A5 substitute
  HeadingGyro gyro;
  calibrate(gyro, 0);
  for (int i = 0; i < 50; ++i) {
    gyro.update(131 * 20);
  }
  for (int i = 0; i < 50; ++i) {
    gyro.missedFrame();
  }
  CHECK(gyro.headingCdeg() == 2000U);  // 20 degrees/s for 1 s
}

}  // namespace

void runUnitTests() {
  calibratesWhenStill();
  restartsIfMoved();
  convertsRate();
  integratesHeading();
  resolvesSlowDrift();
  missedFrameKeepsLastRate();
}
