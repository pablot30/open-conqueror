#include "ir.h"

namespace oc {

// ===========================================================================
// OUR REMOTE'S KEY CODES. This is the one place to change them.
//
// Recorded on our robot on 2026-11-15 with the robot test ir_keys, using the
// remote that came with the kit. Another remote may send other codes: record
// it with `conq ir record` or tests/robot/ir_keys.py and paste the lines here.
//
// Format: {key, address, command, known}. The address is the first two
// bytes of the frame (first byte in the low half), as the `{K,...}` message
// reports it; the command is the third byte.
//
// Recorded for increment 3's mode keys, not used yet (address 0xFF00):
// 1: 0x16, 2: 0x19, 3: 0x0D, 4: 0x0C, 5: 0x18, 6: 0x5E.
// ===========================================================================
namespace {
const KeyEntry kKeyTable[] = {
    {Key::kUp, 0xFF00U, 0x46U, true},
    {Key::kDown, 0xFF00U, 0x15U, true},
    {Key::kLeft, 0xFF00U, 0x44U, true},
    {Key::kRight, 0xFF00U, 0x43U, true},
    {Key::kOk, 0xFF00U, 0x40U, true},
};
}  // namespace
// ===========================================================================

Key keyFor(const IrCode& code) {
  return lookupKey(kKeyTable, static_cast<uint8_t>(sizeof(kKeyTable) / sizeof(kKeyTable[0])), code);
}

}  // namespace oc
