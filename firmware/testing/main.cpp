#include "check.h"

namespace oc_test {
int failures = 0;
int checks = 0;
}  // namespace oc_test

int main() {
  runUnitTests();
  std::printf("%d checks, %d failed\n", oc_test::checks, oc_test::failures);
  return (oc_test::failures == 0) ? 0 : 1;
}
