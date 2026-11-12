// Debounces a push button and reports each new press once.
#ifndef OC_BUTTON_H
#define OC_BUTTON_H

#include <stdint.h>

namespace oc {

class Debouncer {
 public:
  explicit Debouncer(uint32_t settleMs) : settleMs_(settleMs) {}

  // pressed is the raw input. Returns true once per press, after the input
  // has stayed pressed for settleMs.
  bool update(bool pressed, uint32_t nowMs);

 private:
  uint32_t settleMs_;
  uint32_t changedMs_ = 0U;
  bool raw_ = false;
  bool stable_ = false;
};

}  // namespace oc

#endif  // OC_BUTTON_H
