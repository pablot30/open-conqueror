// The gyro: turn rate and heading from the MPU6050's Z axis (F1, F13, T4,
// T6). gyro.cpp is the logic, which the unit tests cover; gyro_hw.cpp talks
// to the chip over I2C.
#ifndef OC_GYRO_H
#define OC_GYRO_H

#include <stdint.h>

namespace oc {

namespace config {
// F13: calibrate over this many frames (1 s at 100 Hz), robot standing still.
const uint8_t kGyroCalibrationFrames = 100U;
// If the raw readings spread more than this during calibration, the robot
// moved, and calibration starts again. 150 counts is about 1.1 degrees/s.
const int16_t kGyroStillSpanRaw = 150;
}  // namespace config

// Raw counts per degree per second at the +-250 degrees/s range (MPU-6000/
// MPU-6050 register map, GYRO_CONFIG with FS_SEL 0).
const int32_t kGyroRawPerDps = 131;
// Frames per second; the heading integrates one reading per frame.
const int32_t kGyroFramesPerSecond = 100;

class HeadingGyro {
 public:
  // Feeds one raw Z reading, once per frame. Until calibrated, readings
  // calibrate the zero rate; after that they update the rate and heading.
  void update(int16_t raw);
  // For a frame whose reading failed: keeps integrating the last rate.
  void missedFrame();
  void resetHeading() { heading_ = 0; }

  bool calibrated() const { return calibrated_; }
  uint8_t calibrationRestarts() const { return restarts_; }
  // Turn rate in hundredths of a degree per second, zero rate removed.
  // Positive is counterclockwise seen from above if the chip faces up.
  int32_t rateCdps() const { return rateX100_ / kGyroRawPerDps; }
  // Heading since power-on or the last reset, 0 to 35999 hundredths of a
  // degree, in the same direction as the rate.
  uint16_t headingCdeg() const;

 private:
  void integrate();

  // Calibration
  int32_t sum_ = 0;
  int16_t min_ = 0;
  int16_t max_ = 0;
  uint8_t count_ = 0U;
  uint8_t restarts_ = 0U;
  bool calibrated_ = false;
  int32_t biasX100_ = 0;  // zero rate in hundredths of a raw count
  // Rate and heading
  int32_t rateX100_ = 0;  // raw counts x 100, zero rate removed
  int32_t heading_ = 0;   // sum of rateX100_ over frames, wrapped to one turn
};

namespace gyro_hw {
// Wakes the MPU6050 and sets the +-250 degrees/s range. Returns false if
// it doesn't answer or doesn't identify as an MPU6050.
bool begin();
// Reads the Z rate. Returns false if the I2C read failed.
bool readZ(int16_t& raw);
}  // namespace gyro_hw

}  // namespace oc

#endif  // OC_GYRO_H
