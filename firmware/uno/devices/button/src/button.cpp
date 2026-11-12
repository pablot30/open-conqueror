#include "button.h"

namespace oc {

bool Debouncer::update(bool pressed, uint32_t nowMs) {
  if (pressed != raw_) {
    raw_ = pressed;
    changedMs_ = nowMs;
    return false;
  }
  if ((raw_ != stable_) && ((nowMs - changedMs_) >= settleMs_)) {
    stable_ = raw_;
    return stable_;
  }
  return false;
}

}  // namespace oc
