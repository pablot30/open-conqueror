#include "check.h"
#include "motion.h"

using oc::HeadingHold;
using oc::HoldInput;
using oc::HoldOutput;
using oc::HoldState;
using oc::SignalStatus;

namespace {

// A simple robot: it turns at a rate that depends on the difference between
// its tracks, plus a steady drift from tracks that don't match. The gyro
// reads its heading exactly. Heading is in hundredths of a degree and
// counterclockwise is positive.
struct SimRobot {
  double headingCdeg = 0.0;
  double driftDps = 0.0;            // degrees per second, positive turns left
  double dpsPerPermille = 0.15;     // of correction, which adds to one track and takes from the other
  uint16_t reading() const {
    double h = headingCdeg;
    while (h < 0.0) {
      h += 36000.0;
    }
    while (h >= 36000.0) {
      h -= 36000.0;
    }
    return static_cast<uint16_t>(h);
  }
  void step(int16_t correctionPermille) {  // one frame, 10 ms
    const double dps = driftDps + (dpsPerPermille * 2.0 * correctionPermille);
    headingCdeg += dps * 100.0 * 0.01;
  }
};

HoldInput straight(const SimRobot& robot, SignalStatus gyro = SignalStatus::kValid) {
  return {true, 40, 40, robot.reading(), gyro};
}

// Runs a straight run of this many seconds and returns the largest heading
// error seen after the first two seconds.
double run(HeadingHold& hold, SimRobot& robot, double seconds, double& maxCorrection) {
  const uint16_t start = robot.reading();
  double worst = 0.0;
  maxCorrection = 0.0;
  for (int frame = 0; frame < static_cast<int>(seconds * 100.0); ++frame) {
    const HoldOutput out = hold.update(straight(robot));
    robot.step(out.correctionPermille);
    const double error = static_cast<double>(oc::headingError(robot.reading(), start)) / 100.0;
    if (frame > 200 && (error > worst || -error > worst)) {
      worst = (error > 0.0) ? error : -error;
    }
    const double c = (out.correctionPermille > 0) ? out.correctionPermille : -out.correctionPermille;
    if (c > maxCorrection) {
      maxCorrection = c;
    }
  }
  return worst;
}

void headingErrorTakesTheShortWay() {
  CHECK(oc::headingError(100U, 35900U) == 200);
  CHECK(oc::headingError(35900U, 100U) == -200);
  CHECK(oc::headingError(9000U, 0U) == 9000);
  CHECK(oc::headingError(0U, 9000U) == -9000);
  CHECK(oc::headingError(18000U, 0U) == -18000);
}

void holdsStraightDespiteDrift() {  // F1, T4
  const double drifts[] = {0.5, -0.5, 3.0, -3.0, 8.0};  // 0.5 degrees/s is T4
  for (double drift : drifts) {
    HeadingHold hold;
    SimRobot robot;
    robot.driftDps = drift;
    double maxCorrection = 0.0;
    const double worst = run(hold, robot, 20.0, maxCorrection);
    CHECK(worst < 1.0);                   // within a degree after settling
    CHECK(maxCorrection <= 50.0);          // a few percent at most
  }
}

void withoutHoldTheSameRobotDrifts() {  // the control for the test above
  SimRobot robot;
  robot.driftDps = 3.0;
  for (int frame = 0; frame < 2000; ++frame) {
    robot.step(0);
  }
  CHECK(robot.headingCdeg > 5000.0);  // 60 degrees in 20 s
}

void correctsAcrossTheWrapAround() {
  HeadingHold hold;
  SimRobot robot;
  robot.headingCdeg = 35950.0;  // just before north
  robot.driftDps = 4.0;
  double maxCorrection = 0.0;
  CHECK(run(hold, robot, 15.0, maxCorrection) < 1.0);
}

void signChecks() {  // drifting left needs a right turn, which is a negative correction
  HeadingHold hold;
  hold.update({true, 40, 40, 1000U, SignalStatus::kValid});  // captures 10 degrees
  HoldOutput out = hold.update({true, 40, 40, 1300U, SignalStatus::kValid});  // 3 degrees to the left
  CHECK(out.correctionPermille < 0);
  hold = HeadingHold();
  hold.update({true, 40, 40, 1000U, SignalStatus::kValid});
  out = hold.update({true, 40, 40, 700U, SignalStatus::kValid});
  CHECK(out.correctionPermille > 0);
}

void correctionIsLimited() {
  HeadingHold hold;
  hold.update({true, 40, 40, 0U, SignalStatus::kValid});
  HoldOutput out = hold.update({true, 40, 40, 9000U, SignalStatus::kValid});  // 90 degrees off
  CHECK(out.correctionPermille == -oc::config::kHoldMaxCorrectionPermille);
  for (int i = 0; i < 10000; ++i) {  // and the integral can't wind up past it
    out = hold.update({true, 40, 40, 9000U, SignalStatus::kValid});
  }
  CHECK(out.correctionPermille == -oc::config::kHoldMaxCorrectionPermille);
  out = hold.update({true, 40, 40, 0U, SignalStatus::kValid});  // back on target
  for (int i = 0; i < 500; ++i) {
    out = hold.update({true, 40, 40, 0U, SignalStatus::kValid});
  }
  CHECK(out.correctionPermille > -oc::config::kHoldMaxCorrectionPermille);  // it unwinds
}

void onlyForStraightRuns() {  // F1
  HeadingHold hold;
  CHECK(hold.update({true, 40, 40, 0U, SignalStatus::kValid}).state == HoldState::kHolding);
  CHECK(hold.update({true, 40, 20, 100U, SignalStatus::kValid}).state == HoldState::kOff);  // an arc
  CHECK(hold.update({true, -40, 40, 100U, SignalStatus::kValid}).state == HoldState::kOff);  // a spin
  CHECK(hold.update({true, 5, 5, 100U, SignalStatus::kValid}).state == HoldState::kOff);     // too slow
  CHECK(hold.update({false, 40, 40, 100U, SignalStatus::kValid}).state == HoldState::kOff);  // not driving
  CHECK(hold.update({true, 0, 0, 100U, SignalStatus::kValid}).correctionPermille == 0);
}

void capturesTheHeadingAtTheStartOfEachRun() {
  HeadingHold hold;
  hold.update({true, 40, 40, 5000U, SignalStatus::kValid});
  CHECK(hold.update({true, 40, 40, 5000U, SignalStatus::kValid}).correctionPermille == 0);
  hold.update({false, 0, 0, 5000U, SignalStatus::kValid});  // stopped
  // The robot was turned by hand, then driven again: the new run holds 9000.
  hold.update({true, 40, 40, 9000U, SignalStatus::kValid});
  CHECK(hold.update({true, 40, 40, 9000U, SignalStatus::kValid}).correctionPermille == 0);
  // Changing speed on a straight run keeps the same hold.
  hold.update({true, 40, 40, 9300U, SignalStatus::kValid});
  const HoldOutput faster = hold.update({true, 60, 60, 9300U, SignalStatus::kValid});
  CHECK(faster.correctionPermille != 0);
  // Forward to backward is a new command, with a new heading to hold.
  hold.update({true, -40, -40, 9300U, SignalStatus::kValid});
  CHECK(hold.update({true, -40, -40, 9300U, SignalStatus::kValid}).correctionPermille == 0);
}

void usesTheGyroOnlyWhileItIsValid() {  // A5
  HeadingHold hold;
  hold.update({true, 40, 40, 1000U, SignalStatus::kValid});
  const HoldOutput corrected = hold.update({true, 40, 40, 1300U, SignalStatus::kValid});
  CHECK(corrected.correctionPermille != 0 && corrected.state == HoldState::kHolding);

  // init, error and not available: no correction at all, and say so.
  const SignalStatus bad[] = {SignalStatus::kInit, SignalStatus::kError, SignalStatus::kNotAvailable};
  for (SignalStatus status : bad) {
    HeadingHold h;
    h.update({true, 40, 40, 1000U, SignalStatus::kValid});
    h.update({true, 40, 40, 1300U, SignalStatus::kValid});
    const HoldOutput out = h.update({true, 40, 40, 1300U, status});
    CHECK(out.correctionPermille == 0 && out.state == HoldState::kNoGyro);
  }
  // A run that starts with the gyro init drives without correction.
  HeadingHold start;
  CHECK(start.update({true, 40, 40, 0U, SignalStatus::kInit}).state == HoldState::kNoGyro);
  CHECK(start.update({true, 40, 40, 500U, SignalStatus::kInit}).correctionPermille == 0);
  // And holds the heading it has when the gyro becomes valid.
  start.update({true, 40, 40, 700U, SignalStatus::kValid});
  CHECK(start.update({true, 40, 40, 700U, SignalStatus::kValid}).correctionPermille == 0);
}

void aSubstituteKeepsTheLastCorrectionBrieflyThenGivesUp() {  // A5
  HeadingHold hold;
  hold.update({true, 40, 40, 1000U, SignalStatus::kValid});
  const HoldOutput last = hold.update({true, 40, 40, 1300U, SignalStatus::kValid});
  CHECK(last.correctionPermille != 0);
  HoldOutput out = last;
  for (uint8_t i = 0U; i < oc::config::kHoldSubstituteFrames; ++i) {
    out = hold.update({true, 40, 40, 1300U, SignalStatus::kSubstitute});
    CHECK(out.state == HoldState::kSubstitute && out.correctionPermille == last.correctionPermille);
  }
  out = hold.update({true, 40, 40, 1300U, SignalStatus::kSubstitute});  // too long
  CHECK(out.state == HoldState::kNoGyro && out.correctionPermille == 0);
  // Back to valid: hold where it is now, from a clean start.
  out = hold.update({true, 40, 40, 2000U, SignalStatus::kValid});
  CHECK(out.state == HoldState::kHolding && out.correctionPermille == 0);
}

void aShortSubstituteResumesTheSameHold() {
  HeadingHold hold;
  hold.update({true, 40, 40, 1000U, SignalStatus::kValid});
  hold.update({true, 40, 40, 1300U, SignalStatus::kValid});
  hold.update({true, 40, 40, 1300U, SignalStatus::kSubstitute});
  hold.update({true, 40, 40, 1300U, SignalStatus::kSubstitute});
  const HoldOutput out = hold.update({true, 40, 40, 1300U, SignalStatus::kValid});
  CHECK(out.state == HoldState::kHolding && out.correctionPermille < 0);  // still holding 1000
}

void asubstituteBeforeAnyCaptureGivesNothing() {
  HeadingHold hold;
  CHECK(hold.update({true, 40, 40, 0U, SignalStatus::kSubstitute}).state == HoldState::kNoGyro);
}

}  // namespace

void runUnitTests() {
  headingErrorTakesTheShortWay();
  holdsStraightDespiteDrift();
  withoutHoldTheSameRobotDrifts();
  correctsAcrossTheWrapAround();
  signChecks();
  correctionIsLimited();
  onlyForStraightRuns();
  capturesTheHeadingAtTheStartOfEachRun();
  usesTheGyroOnlyWhileItIsValid();
  aSubstituteKeepsTheLastCorrectionBrieflyThenGivesUp();
  aShortSubstituteResumesTheSameHold();
  asubstituteBeforeAnyCaptureGivesNothing();
}
