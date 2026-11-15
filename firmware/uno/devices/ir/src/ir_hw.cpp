// Times the IR receiver's pulses on D9 with the shared pin-change interrupt
// in hal/pinchange. Receiver modules of the usual kind demodulate the 38 kHz
// carrier and hold their output high when idle and low during a burst. We
// haven't checked that on our robot: if the polarity is the other way round,
// frames show up as unreadable in the `{K,...}` message.
//
// The interrupt stores the width of each pulse. takeFrame() decides the
// frame is over when the line has been quiet longer than the longest space
// inside a frame (4.5 ms), and copies it out with the interrupt listening
// to nothing, so no interrupt is ever kept off for a copy.
#include <Arduino.h>

#include <pinchange.h>

#include "ir.h"

namespace oc {
namespace ir_hw {

namespace {

const uint8_t kReceiverPin = 9U;
// Quiet for this long means the frame has ended. Longer than the longest
// space inside a frame, shorter than the gap before the next one.
const uint32_t kFrameGapUs = 6000U;

volatile uint16_t widths[kIrMaxPulses];
volatile uint8_t count = 0U;
volatile uint32_t lastEdgeUs = 0U;
volatile bool inFrame = false;
volatile bool frameReady = false;  // the interrupt ignores edges while a frame is being copied out

// Runs in the interrupt.
void onEdge(bool high, uint32_t atUs) {
  if (frameReady) {
    return;
  }
  if (!inFrame) {
    if (!high) {  // a frame starts with the line going low
      inFrame = true;
      count = 0U;
      lastEdgeUs = atUs;
    }
    return;
  }
  const uint32_t width = atUs - lastEdgeUs;
  lastEdgeUs = atUs;
  if (count < kIrMaxPulses) {
    widths[count] = (width > 65535U) ? static_cast<uint16_t>(65535U) : static_cast<uint16_t>(width);
    count = static_cast<uint8_t>(count + 1U);
  }
}

}  // namespace

void begin() {
  pinMode(kReceiverPin, INPUT);
  pinchange::attach(kReceiverPin, onEdge);
}

bool takeFrame(uint16_t* widthsUs, uint8_t& pulses) {
  noInterrupts();
  const bool ended = inFrame && ((micros() - lastEdgeUs) > kFrameGapUs);
  if (ended) {
    inFrame = false;
    frameReady = true;
  }
  interrupts();
  if (!ended) {
    return false;
  }
  pulses = count;
  for (uint8_t i = 0U; i < pulses; ++i) {
    widthsUs[i] = widths[i];
  }
  frameReady = false;
  return true;
}

}  // namespace ir_hw
}  // namespace oc
