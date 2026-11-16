// Motion: heading hold (F1, T4). When both tracks are commanded equal, the
// robot should drive straight, so this holds the heading it had when the
// command started with a small PI controller on the gyro's heading, and
// asks for a small left/right correction. It never sets the motors: the
// correction goes to the safety supervisor as a trim, which limits it again.
// See docs/architecture.md, A2 (Motion) and A5 (signal status).
//
// The gyro is used only while its signal is valid, and briefly while it is a
// substitute (a missed read or two). Otherwise the robot drives without
// correction, and says so.
#ifndef OC_MOTION_H
#define OC_MOTION_H

#include <stdint.h>

#include <diagnostics.h>

namespace oc {

namespace config {
// Heading hold only works on a straight run at or above this speed. Below
// it the tracks hardly turn and a correction would be most of the command.
const int8_t kHoldMinSpeedPercent = 15;
// The correction never exceeds this, in tenths of a percent of the allowed
// maximum, added to one track and taken from the other ("a few percent").
const int16_t kHoldMaxCorrectionPermille = 50;
// Proportional gain: permille of correction for each degree of error. A
// first guess to be tuned on the floor.
const int16_t kHoldKpPermillePerDeg = 10;
// Integral gain: permille for each degree of error held for one second. It
// removes the steady drift of tracks that don't match. Also a first guess.
const int16_t kHoldKiPermillePerDegSec = 10;
// While the gyro is a substitute, keep the last correction for this many
// frames (200 ms), then drive without correction.
const uint8_t kHoldSubstituteFrames = 20U;
}  // namespace config

// What heading hold is doing. The numbers go out in the sensor message.
enum class HoldState : uint8_t {
  kOff = 0,         // not a straight run, so there's nothing to hold
  kHolding = 1,     // correcting from a valid gyro
  kSubstitute = 2,  // the gyro missed a read: the last correction, briefly
  kNoGyro = 3,      // a straight run, but the gyro can't be used: no correction
};

struct HoldInput {
  bool driving;           // the safety supervisor is following a command
  int8_t left;            // the commanded tracks, percent
  int8_t right;
  uint16_t headingCdeg;   // 0 to 35999, positive counterclockwise
  SignalStatus gyro;      // the heading's signal status
};

struct HoldOutput {
  int16_t correctionPermille;  // positive turns the robot left (counterclockwise)
  HoldState state;
};

class HeadingHold {
 public:
  // Call once per frame (100 times a second).
  HoldOutput update(const HoldInput& input);

  HoldState state() const { return state_; }
  int16_t correctionPermille() const { return correction_; }

 private:
  void reset();

  HoldState state_ = HoldState::kOff;
  bool captured_ = false;
  uint16_t target_ = 0U;
  int32_t integral_ = 0;   // error in hundredths of a degree, summed over frames
  int16_t correction_ = 0;
  uint8_t substituteFrames_ = 0U;
  int8_t direction_ = 0;   // 1 forward, -1 backward: a reversal starts a new hold
};

// The smallest signed difference between two headings, in hundredths of a
// degree: 'to' minus 'from', from -18000 to 17999.
int32_t headingError(uint16_t to, uint16_t from);

}  // namespace oc

#endif  // OC_MOTION_H
