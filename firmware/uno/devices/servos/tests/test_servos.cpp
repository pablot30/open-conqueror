#include "check.h"
#include "servos.h"

namespace {

void releasesAfterTheMove() {  // F14
  oc::ServoMove move;
  CHECK(!move.moving());
  move.start(1000U);
  CHECK(move.moving());
  CHECK(!move.finished(1000U + oc::config::kServoMoveFrames - 1U));
  CHECK(move.finished(1000U + oc::config::kServoMoveFrames));
  CHECK(!move.finished(1000U + oc::config::kServoMoveFrames + 1U));  // only once
  CHECK(!move.moving());
}

void survivesFrameCounterWrap() {
  oc::ServoMove move;
  move.start(0xFFFFFFF0U);
  CHECK(!move.finished(5U));
  CHECK(move.finished(0xFFFFFFF0U + oc::config::kServoMoveFrames));
}

}  // namespace

void runUnitTests() {
  releasesAfterTheMove();
  survivesFrameCounterWrap();
}
