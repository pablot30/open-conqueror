// Tunable constants for the UNO firmware. Values marked "proposed" come from
// docs/requirements.md and still need checking on the robot.
#ifndef OC_CORE_CONFIG_H
#define OC_CORE_CONFIG_H

#include <stdint.h>

namespace oc {
namespace config {

// S3: drive commands expire after this long without a new one (proposed).
const uint32_t kLinkTimeoutMs = 300U;

// Telemetry period. 10 messages a second fit easily in 9600 baud.
const uint32_t kTelemetryPeriodMs = 100U;

// Battery sampling period. With the filter below, the reading settles in
// about 0.3 s, so a brief dip under load doesn't trip the cutoff.
const uint32_t kBatterySamplePeriodMs = 20U;

// S9: battery thresholds in millivolts, measured at Vin after the input
// diode. Each has a recovery level above it, so the state doesn't flicker.
const uint16_t kBatteryLowMv = 7000U;
const uint16_t kBatteryLowRecoverMv = 7200U;
const uint16_t kBatteryCriticalMv = 6400U;  // proposed
const uint16_t kBatteryCriticalRecoverMv = 6800U;

// S10: the motors' average voltage stays at or below this (proposed).
const uint16_t kMaxMotorMv = 6000U;

// MODE button debounce time.
const uint32_t kButtonDebounceMs = 30U;

}  // namespace config
}  // namespace oc

#endif  // OC_CORE_CONFIG_H
