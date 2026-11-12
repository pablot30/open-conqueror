// Sends a colour to one WS2812 on D4 (PORTD bit 4, docs/hardware/pin-map.md).
// Written for a 16 MHz ATmega328P. Each bit is 19 clock cycles (1.19 us):
// a 0 is high for 5 cycles (312 ns), a 1 for 12 cycles (750 ns), inside the
// WS2812B datasheet's timing windows. The LED takes green, red, blue, most
// significant bit first. Our own driver: the common libraries allocate
// memory at run time, which T8 rules out.
#include <Arduino.h>

#include "led.h"

namespace oc {
namespace led_hw {

namespace {

const uint8_t kBit = _BV(4);  // D4

void sendByte(uint8_t value, uint8_t high, uint8_t low) {
  uint8_t bits = 8U;
  asm volatile(
      "1:                      \n\t"
      "out  %[port], %[high]   \n\t"  // cycle 0: line high
      "nop                     \n\t"
      "nop                     \n\t"
      "nop                     \n\t"
      "sbrs %[value], 7        \n\t"  // cycle 4: bit 1 skips the next line
      "out  %[port], %[low]    \n\t"  // cycle 5: a 0 ends here
      "nop                     \n\t"
      "nop                     \n\t"
      "nop                     \n\t"
      "nop                     \n\t"
      "nop                     \n\t"
      "nop                     \n\t"
      "out  %[port], %[low]    \n\t"  // cycle 12: a 1 ends here
      "nop                     \n\t"
      "nop                     \n\t"
      "lsl  %[value]           \n\t"
      "dec  %[bits]            \n\t"
      "brne 1b                 \n\t"  // 2 cycles back to cycle 0
      : [value] "+r"(value), [bits] "+r"(bits)
      : [port] "I"(_SFR_IO_ADDR(PORTD)), [high] "r"(high), [low] "r"(low));
}

}  // namespace

void begin() {
  digitalWrite(4, LOW);
  pinMode(4, OUTPUT);
}

void show(const Rgb& color) {
  const uint8_t savedSreg = SREG;
  cli();
  const uint8_t high = PORTD | kBit;
  const uint8_t low = PORTD & static_cast<uint8_t>(~kBit);
  sendByte(color.green, high, low);
  sendByte(color.red, high, low);
  sendByte(color.blue, high, low);
  SREG = savedSreg;
  // The LED latches the colour after 50 us or more of low. Frames are 10 ms
  // apart, so that always happens before the next update.
}

}  // namespace led_hw
}  // namespace oc
