#include "check.h"
#include "safety.h"

using oc::SafetySupervisor;
using oc::DriveState;
using oc::MotorOutput;

namespace {

// A controller that has had its neutral command, so it accepts driving.
SafetySupervisor armedAt(uint32_t nowMs) {
  SafetySupervisor drive;
  drive.drive(0, 0, nowMs);
  return drive;
}

void startsInStandby() {  // S5
  SafetySupervisor drive;
  const MotorOutput output = drive.update(0U, 255U, true);
  CHECK(drive.state() == DriveState::kStandby);
  CHECK(!output.enabled);
  CHECK(output.left == 0);
  CHECK(output.right == 0);
}

void stopBeforeDriveStaysInStandby() {  // S5
  SafetySupervisor drive;
  drive.stop();
  CHECK(!drive.update(0U, 255U, true).enabled);
}

void drivesScaledToLimit() {  // F3, S10
  SafetySupervisor drive = armedAt(0U);
  drive.drive(100, -50, 0U);
  const MotorOutput output = drive.update(10U, 182U, true);
  CHECK(drive.state() == DriveState::kDriving);
  CHECK(output.enabled);
  CHECK(output.left == 182);
  CHECK(output.right == -91);
}

void limitNeverExceeds255() {
  SafetySupervisor drive = armedAt(0U);
  drive.drive(100, 100, 0U);
  CHECK(drive.update(0U, 1000U, true).left == 255);
}

void stopBrakes() {  // F4, S1
  SafetySupervisor drive = armedAt(0U);
  drive.drive(80, 80, 0U);
  drive.stop();
  const MotorOutput output = drive.update(10U, 255U, true);
  CHECK(drive.state() == DriveState::kStopped);
  CHECK(output.enabled);  // braking, not coasting
  CHECK(output.left == 0);
  CHECK(output.right == 0);
}

void linkTimeoutStops() {  // S3
  SafetySupervisor drive = armedAt(1000U);
  drive.drive(50, 50, 1000U);
  CHECK(drive.update(1000U + oc::config::kLinkTimeoutMs, 255U, true).left != 0);
  CHECK(!drive.linkTimedOut());
  const MotorOutput output = drive.update(1001U + oc::config::kLinkTimeoutMs, 255U, true);
  CHECK(output.left == 0);
  CHECK(drive.state() == DriveState::kStopped);
  CHECK(drive.linkTimedOut());
  // Resent commands after the timeout don't restart it, and leave the flag.
  drive.drive(50, 50, 2000U);
  CHECK(drive.update(2000U, 255U, true).left == 0);
  CHECK(drive.linkTimedOut());
  // A neutral command, then a new drive command, clears it.
  drive.drive(0, 0, 2100U);
  drive.drive(50, 50, 2200U);
  CHECK(drive.update(2200U, 255U, true).left != 0);
  CHECK(!drive.linkTimedOut());
}

void linkTimeoutSurvivesMillisWrap() {  // S3
  SafetySupervisor drive = armedAt(0xFFFFFF00U);
  drive.drive(50, 50, 0xFFFFFF00U);
  CHECK(drive.update(0x00000010U, 255U, true).left != 0);
  CHECK(drive.update(0x00000100U, 255U, true).left == 0);
}

void criticalBatteryStops() {  // S9
  SafetySupervisor drive = armedAt(0U);
  drive.drive(50, 50, 0U);
  CHECK(drive.update(10U, 255U, false).left == 0);
  CHECK(drive.state() == DriveState::kStopped);
}

void ignoresDrivingBeforeNeutral() {  // S4, S5: a reset mid-drive stays stopped
  SafetySupervisor drive;
  drive.drive(50, 50, 0U);
  const MotorOutput output = drive.update(10U, 255U, true);
  CHECK(drive.state() == DriveState::kStandby);
  CHECK(!output.enabled);
  drive.drive(0, 0, 20U);
  CHECK(drive.state() == DriveState::kDriving);
  CHECK(drive.update(30U, 255U, true).left == 0);  // neutral brakes
  drive.drive(50, 50, 40U);
  CHECK(drive.update(50U, 255U, true).left != 0);
}

void stopHoldsAgainstResentCommands() {  // S1
  SafetySupervisor drive = armedAt(0U);
  drive.drive(60, 60, 0U);
  drive.stop();
  for (uint32_t t = 100U; t <= 1000U; t += 100U) {
    drive.drive(60, 60, t);  // the computer keeps resending
    CHECK(drive.update(t, 255U, true).left == 0);
    CHECK(drive.state() == DriveState::kStopped);
  }
  drive.drive(0, 0, 1100U);
  drive.drive(60, 60, 1200U);
  CHECK(drive.update(1200U, 255U, true).left != 0);
}

void stopDisarmsEvenWhenNotDriving() {  // S1
  SafetySupervisor drive = armedAt(0U);
  drive.stop();
  drive.drive(60, 60, 10U);
  CHECK(drive.update(10U, 255U, true).left == 0);
}

void criticalBatteryHoldsStop() {  // S9
  SafetySupervisor drive = armedAt(0U);
  drive.drive(50, 50, 0U);
  CHECK(drive.update(10U, 255U, false).left == 0);
  // Battery recovered, but the old command stream doesn't restart it.
  drive.drive(50, 50, 20U);
  CHECK(drive.update(20U, 255U, true).left == 0);
}

}  // namespace

void runUnitTests() {
  startsInStandby();
  stopBeforeDriveStaysInStandby();
  drivesScaledToLimit();
  limitNeverExceeds255();
  stopBrakes();
  linkTimeoutStops();
  linkTimeoutSurvivesMillisWrap();
  criticalBatteryStops();
  ignoresDrivingBeforeNeutral();
  stopHoldsAgainstResentCommands();
  stopDisarmsEvenWhenNotDriving();
  criticalBatteryHoldsStop();
}
