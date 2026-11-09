// A minimal test helper, so the tests need nothing beyond a C++ compiler.
#ifndef OC_TESTS_CHECK_H
#define OC_TESTS_CHECK_H

#include <cstdio>

namespace oc_test {
extern int failures;
extern int checks;
}  // namespace oc_test

#define CHECK(condition)                                                       \
  do {                                                                         \
    ++oc_test::checks;                                                         \
    if (!(condition)) {                                                        \
      ++oc_test::failures;                                                     \
      std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
    }                                                                          \
  } while (false)

void runProtocolTests();
void runDriveTests();
void runBatteryTests();
void runDebounceTests();

#endif  // OC_TESTS_CHECK_H
