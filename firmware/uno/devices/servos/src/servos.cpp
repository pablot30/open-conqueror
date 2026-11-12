#include "servos.h"

namespace oc {

bool ServoMove::finished(uint32_t frame) {
  if (moving_ && ((frame - startFrame_) >= config::kServoMoveFrames)) {
    moving_ = false;
    return true;
  }
  return false;
}

}  // namespace oc
