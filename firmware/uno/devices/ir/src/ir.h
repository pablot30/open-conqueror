// The IR remote: the receiver on D9, a decoder for NEC-style frames, the
// table of our remote's keys, and the logic that turns arrow keys into
// driving (F10, S2). ir.cpp, ir_keys.cpp and remote.cpp are the logic, which
// the unit tests cover; ir_hw.cpp times the receiver's pulses with the shared
// pin-change interrupt.
//
// The NEC protocol, as described in public documents (for example the
// SB-Projects IR protocols page): a frame is a 9 ms burst, a 4.5 ms space,
// then 32 bits sent least significant bit first, then a 562 microsecond
// burst. Every bit is a 562 microsecond burst followed by a space: 562
// microseconds for 0, 1687 for 1. The 32 bits are an address byte, its
// inverse (or, in the extended form, an address high byte), a command byte
// and its inverse. A held key repeats as a 9 ms burst, a 2.25 ms space and
// a 562 microsecond burst, about every 108 ms. We have not yet seen our
// remote's frames: ir_keys records them, and anything this decoder can't
// read is reported as unreadable, so a different protocol shows up.
#ifndef OC_IR_H
#define OC_IR_H

#include <stdint.h>

namespace oc {

namespace config {
// Pulse width limits in microseconds. They are wide, because receiver
// modules stretch or shorten bursts by up to about 150 microseconds.
const uint16_t kIrLeadMarkMinUs = 7000U;   // nominal 9000
const uint16_t kIrLeadMarkMaxUs = 11000U;
const uint16_t kIrLeadSpaceMinUs = 3500U;  // nominal 4500
const uint16_t kIrLeadSpaceMaxUs = 5500U;
const uint16_t kIrRepeatSpaceMinUs = 1700U;  // nominal 2250
const uint16_t kIrRepeatSpaceMaxUs = 2800U;
const uint16_t kIrBitMarkMinUs = 250U;     // nominal 562
const uint16_t kIrBitMarkMaxUs = 1000U;
const uint16_t kIrSpaceMinUs = 250U;       // a 0 space, nominal 562
const uint16_t kIrSpaceOneMinUs = 1100U;   // spaces from here are 1s, nominal 1687
const uint16_t kIrSpaceMaxUs = 2400U;
// Unreadable bursts shorter than this many pulses are ambient light, not a
// remote, and aren't reported.
const uint8_t kIrReportMinPulses = 10U;
// Arrows drive at this speed, in percent of the allowed maximum. Slow, for
// a first test on a table.
const int8_t kRemoteSpeedPercent = 40;
// F10: the robot stops this long after the last code from the held key.
// Same as the link timeout (S3).
const uint32_t kRemoteReleaseMs = 300U;
}  // namespace config

// One frame is the lead mark and space, 32 bit pairs and the end mark.
const uint8_t kIrMaxPulses = 68U;

enum class IrKind : uint8_t {
  kNone = 0,        // nothing there
  kFrame = 1,       // a full, valid frame
  kRepeat = 2,      // the repeat code of a held key
  kUnreadable = 3,  // pulses we can't read as NEC
};

struct IrCode {
  IrKind kind;
  // The first two bytes of the frame, first byte in the low half: the
  // address and its inverse in the classic form.
  uint16_t address;
  uint8_t command;
};

// Decodes one frame from its pulse widths in microseconds, alternating
// burst and space and starting with the lead burst: widths[0] is the lead
// burst, widths[1] the lead space, and so on. A frame has 67 widths (the
// last is the end burst, which may be missing) and a repeat code 3 (or 2).
IrCode decodeNec(const uint16_t* widthsUs, uint8_t count);

// The keys the firmware knows about. Mode keys wait for increment 3.
enum class Key : uint8_t {
  kNone = 0,
  kUp,
  kDown,
  kLeft,
  kRight,
  kOk,
  kCount
};

// One key of the remote, as the receiver reports it.
struct KeyEntry {
  Key key;
  uint16_t address;
  uint8_t command;
  bool known;  // false until the code has been recorded on the robot
};

// Finds a code in a table. Entries that aren't known never match.
Key lookupKey(const KeyEntry* table, uint8_t count, const IrCode& code);
// Finds a code in our remote's table, in ir_keys.cpp.
Key keyFor(const IrCode& code);

// What the remote asks the safety supervisor to do.
enum class RemoteAction : uint8_t {
  kNone,
  kDrive,  // follow left and right; newPress says it's a deliberate new command
  kStop,   // brake, through the safety supervisor
};

struct RemoteRequest {
  RemoteAction action;
  bool newPress;  // a key pressed afresh: arm with a neutral command first
  int8_t left;
  int8_t right;
};

// Turns keys into requests. Arrows drive while held, and stop 300 ms after
// the last code. OK stops, and nothing after it drives again until a key is
// pressed afresh, so resent repeat codes can't undo a stop (S2, S5).
class RemoteDriver {
 public:
  // A full frame was received for this key.
  RemoteRequest onKey(Key key, uint32_t nowMs);
  // A repeat code was received: the key from the last frame is still held.
  RemoteRequest onRepeat(uint32_t nowMs);
  // Call every frame: stops when the held key stops sending codes.
  RemoteRequest update(uint32_t nowMs);

  bool holding() const { return holding_; }

 private:
  RemoteRequest driveFor(Key key, bool newPress) const;

  bool holding_ = false;
  Key held_ = Key::kNone;
  uint32_t lastCodeMs_ = 0U;
};

namespace ir_hw {
// Sets the receiver pin and attaches it to the pin-change interrupt.
void begin();
// If a whole frame has arrived and the line has been quiet since, copies its
// pulse widths out, returns true and starts listening for the next. Call
// every frame; it never waits.
bool takeFrame(uint16_t* widthsUs, uint8_t& count);
}  // namespace ir_hw

}  // namespace oc

#endif  // OC_IR_H
