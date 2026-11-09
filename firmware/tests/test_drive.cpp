#include "check.h"
#include "config.h"
#include "drive.h"

using oc::DriveControl;
using oc::DriveState;
using oc::MotorOutput;

namespace {

void startsInStandby() {  // S5
  DriveControl drive;
  const MotorOutput output = drive.update(0U, 255U, true);
  CHECK(drive.state() == DriveState::kStandby);
  CHECK(!output.enabled);
  CHECK(output.left == 0);
  CHECK(output.right == 0);
}

void stopBeforeDriveStaysInStandby() {  // S5
  DriveControl drive;
  drive.stop();
  CHECK(!drive.update(0U, 255U, true).enabled);
}

void drivesScaledToLimit() {  // F3, S10
  DriveControl drive;
  drive.drive(100, -50, 0U);
  const MotorOutput output = drive.update(10U, 182U, true);
  CHECK(drive.state() == DriveState::kDriving);
  CHECK(output.enabled);
  CHECK(output.left == 182);
  CHECK(output.right == -91);
}

void limitNeverExceeds255() {
  DriveControl drive;
  drive.drive(100, 100, 0U);
  CHECK(drive.update(0U, 1000U, true).left == 255);
}

void stopBrakes() {  // F4, S1
  DriveControl drive;
  drive.drive(80, 80, 0U);
  drive.stop();
  const MotorOutput output = drive.update(10U, 255U, true);
  CHECK(drive.state() == DriveState::kStopped);
  CHECK(output.enabled);  // braking, not coasting
  CHECK(output.left == 0);
  CHECK(output.right == 0);
}

void linkTimeoutStops() {  // S3
  DriveControl drive;
  drive.drive(50, 50, 1000U);
  CHECK(drive.update(1000U + oc::config::kLinkTimeoutMs, 255U, true).left != 0);
  CHECK(!drive.linkTimedOut());
  const MotorOutput output = drive.update(1001U + oc::config::kLinkTimeoutMs, 255U, true);
  CHECK(output.left == 0);
  CHECK(drive.state() == DriveState::kStopped);
  CHECK(drive.linkTimedOut());
  // A new drive command clears the flag.
  drive.drive(50, 50, 2000U);
  CHECK(!drive.linkTimedOut());
}

void linkTimeoutSurvivesMillisWrap() {  // S3
  DriveControl drive;
  drive.drive(50, 50, 0xFFFFFF00U);
  CHECK(drive.update(0x00000010U, 255U, true).left != 0);
  CHECK(drive.update(0x00000100U, 255U, true).left == 0);
}

void criticalBatteryStops() {  // S9
  DriveControl drive;
  drive.drive(50, 50, 0U);
  CHECK(drive.update(10U, 255U, false).left == 0);
  CHECK(drive.state() == DriveState::kStopped);
}

}  // namespace

void runDriveTests() {
  startsInStandby();
  stopBeforeDriveStaysInStandby();
  drivesScaledToLimit();
  limitNeverExceeds255();
  stopBrakes();
  linkTimeoutStops();
  linkTimeoutSurvivesMillisWrap();
  criticalBatteryStops();
}
