// Firmware version (D4). Bump it in the pull request that changes behaviour.
// The git commit comes from build_info.h, which scripts/dev.py writes before
// each build. Builds without it, such as from the Arduino IDE, say "unknown".
#ifndef OC_VERSION_H
#define OC_VERSION_H

#define OC_FIRMWARE_VERSION "0.1.0"

#if defined(__has_include)
#if __has_include("generated/build_info.h")
#include "generated/build_info.h"
#endif
#endif

#ifndef OC_GIT_COMMIT
#define OC_GIT_COMMIT "unknown"
#endif

#endif  // OC_VERSION_H
