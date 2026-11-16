// Firmware version and git commit (D4). scripts/dev.py writes both into
// generated/build_info.h before each build: the version from this firmware's
// VERSION file, the commit from git. Builds without it, such as from the
// Arduino IDE, report "unknown".
#ifndef OC_VERSION_H
#define OC_VERSION_H

#if defined(__has_include)
#if __has_include("generated/build_info.h")
#include "generated/build_info.h"
#endif
#endif

#ifndef OC_FIRMWARE_VERSION
#define OC_FIRMWARE_VERSION "unknown"
#endif

#ifndef OC_GIT_COMMIT
#define OC_GIT_COMMIT "unknown"
#endif

#endif  // OC_VERSION_H
