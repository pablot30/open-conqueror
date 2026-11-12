// MPU6050 access over I2C. Register addresses and values are from
// InvenSense's public MPU-6000/MPU-6050 register map, revision 4.2.
#include <Arduino.h>
#include <Wire.h>

#include "gyro.h"

namespace oc {
namespace gyro_hw {

namespace {

const uint8_t kAddress = 0x68U;      // AD0 low
const uint8_t kRegGyroConfig = 0x1BU;
const uint8_t kRegConfig = 0x1AU;    // digital low-pass filter
const uint8_t kRegGyroZHigh = 0x47U;
const uint8_t kRegPowerMgmt1 = 0x6BU;
const uint8_t kRegWhoAmI = 0x75U;
const uint8_t kWhoAmI = 0x68U;
// If the bus hangs, give up after this long rather than block the frame.
const uint32_t kI2cTimeoutUs = 2000U;

bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0U;
}

bool readRegisters(uint8_t reg, uint8_t* data, uint8_t count) {
  Wire.beginTransmission(kAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0U) {
    return false;
  }
  if (Wire.requestFrom(kAddress, count) != count) {
    return false;
  }
  for (uint8_t i = 0U; i < count; ++i) {
    data[i] = static_cast<uint8_t>(Wire.read());
  }
  return true;
}

}  // namespace

bool begin() {
  Wire.begin();
  Wire.setClock(400000UL);
  Wire.setWireTimeout(kI2cTimeoutUs, true);
  uint8_t id = 0U;
  if (!readRegisters(kRegWhoAmI, &id, 1U) || (id != kWhoAmI)) {
    return false;
  }
  // Wake up with the X gyro's clock, the datasheet's recommended source;
  // a 44 Hz low-pass filter; +-250 degrees/s.
  return writeRegister(kRegPowerMgmt1, 0x01U) && writeRegister(kRegConfig, 0x03U) &&
         writeRegister(kRegGyroConfig, 0x00U);
}

bool readZ(int16_t& raw) {
  uint8_t data[2] = {0U, 0U};
  if (!readRegisters(kRegGyroZHigh, data, 2U)) {
    return false;
  }
  raw = static_cast<int16_t>((static_cast<uint16_t>(data[0]) << 8) | data[1]);
  return true;
}

}  // namespace gyro_hw
}  // namespace oc
