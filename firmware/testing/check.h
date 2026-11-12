// A minimal test helper, so unit tests need nothing beyond a C++ compiler.
// Each unit's tests define runUnitTests(), and main.cpp calls it.
#ifndef OC_TESTING_CHECK_H
#define OC_TESTING_CHECK_H

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

void runUnitTests();

#endif  // OC_TESTING_CHECK_H
