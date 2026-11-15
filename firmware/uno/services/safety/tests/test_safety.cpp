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

void trimTurnsTheRobotLeftWhileDriving() {  // F1
  SafetySupervisor drive = armedAt(0U);
  drive.drive(50, 50, 0U);
  drive.setTrim(40);  // 4% of the allowed maximum
  MotorOutput output = drive.update(10U, 200U, true);
  CHECK(output.left == 100 - 8);   // left slower
  CHECK(output.right == 100 + 8);  // right faster
  drive.setTrim(-40);
  output = drive.update(20U, 200U, true);
  CHECK(output.left == 108 && output.right == 92);
  drive.drive(-50, -50, 20U);  // backward: the same differential turns the same way
  drive.setTrim(40);
  output = drive.update(30U, 200U, true);
  CHECK(output.left == -108 && output.right == -92);
}

void trimIsLimitedByTheSupervisor() {  // S10
  SafetySupervisor drive = armedAt(0U);
  drive.drive(50, 50, 0U);
  drive.setTrim(500);  // asks for 50%
  MotorOutput output = drive.update(10U, 200U, true);
  CHECK(output.left == 100 - 12 && output.right == 100 + 12);  // 6% at most
  drive.drive(100, 100, 10U);
  output = drive.update(20U, 200U, true);
  CHECK(output.right == 200);  // never above the PWM limit
  CHECK(output.left == 188);
}

void trimIsNotACommand() {  // S1, S3, S5
  SafetySupervisor drive = armedAt(0U);
  drive.drive(50, 50, 0U);
  for (uint32_t t = 10U; t <= 290U; t += 10U) {
    drive.setTrim(30);
    drive.update(t, 200U, true);  // trimming every frame...
  }
  CHECK(drive.update(310U, 200U, true).left == 0);  // ...doesn't keep the link alive
  CHECK(drive.linkTimedOut());
  drive.setTrim(30);
  const MotorOutput stopped = drive.update(320U, 200U, true);
  CHECK(stopped.left == 0 && stopped.right == 0);  // and doesn't move a stopped robot
  drive.drive(50, 50, 330U);  // held stop: needs a neutral command first
  CHECK(drive.update(340U, 200U, true).left == 0);
}

void trimDoesNotMoveAStandingRobot() {
  SafetySupervisor drive = armedAt(0U);
  drive.setTrim(50);
  const MotorOutput output = drive.update(10U, 200U, true);
  CHECK(output.left == 0 && output.right == 0);  // neutral stays neutral
}

void reportsTheCommandedTracks() {
  SafetySupervisor drive = armedAt(0U);
  drive.drive(30, 30, 0U);
  CHECK(drive.commandedLeft() == 30 && drive.commandedRight() == 30);
  drive.stop();
  CHECK(drive.commandedLeft() == 0 && drive.commandedRight() == 0);
}

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
  trimTurnsTheRobotLeftWhileDriving();
  trimIsLimitedByTheSupervisor();
  trimIsNotACommand();
  trimDoesNotMoveAStandingRobot();
  reportsTheCommandedTracks();
}
