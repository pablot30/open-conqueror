#include "check.h"
#include "debounce.h"

namespace {

void reportsPressOnceAfterSettling() {
  oc::Debouncer button(30U);
  CHECK(!button.update(true, 0U));
  CHECK(!button.update(true, 29U));
  CHECK(button.update(true, 30U));
  CHECK(!button.update(true, 100U));  // still held: no second press
  CHECK(!button.update(false, 200U));
  CHECK(!button.update(false, 230U));  // release is not a press
}

void ignoresBounce() {
  oc::Debouncer button(30U);
  CHECK(!button.update(true, 0U));
  CHECK(!button.update(false, 5U));
  CHECK(!button.update(true, 10U));
  CHECK(!button.update(true, 35U));
  CHECK(button.update(true, 40U));
}

}  // namespace

void runDebounceTests() {
  reportsPressOnceAfterSettling();
  ignoresBounce();
}
