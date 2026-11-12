// The only part of the firmware that touches the UNO's pins. Pin numbers
// come from docs/hardware/pin-map.md (TB6612 shield V1.1).
#ifndef OC_BOARD_H
#define OC_BOARD_H

#include <stdint.h>

namespace oc {
namespace board {

// Puts every output in a safe state: motor driver in standby (S5).
void init();

// Sets the driver's standby pin, direction pins and PWM. PWM is -255 to
// 255, negative is backward. enabled false puts the driver in standby.
void setMotors(bool enabled, int16_t left, int16_t right);

// Raw 10-bit reading of the battery divider on A3.
uint16_t readBatteryAdc();

// True while the MODE button is held down.
bool modeButtonPressed();

}  // namespace board
}  // namespace oc

#endif  // OC_BOARD_H
