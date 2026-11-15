#include "ir.h"

namespace oc {

// ===========================================================================
// OUR REMOTE'S KEY CODES. This is the one place to fill in.
//
// We haven't recorded them yet, so every entry is unknown and no key does
// anything. To fill it in, put the robot on a table with the Upload-Cam
// switch on Upload, and run `conq ir record`, or the robot test ir_keys. It
// asks you to press each key, and prints these lines, ready to paste. Set
// `known` to true as you paste each one.
//
// Format: {key, address, command, known}. The address is the first two
// bytes of the frame (first byte in the low half), as the `{K,...}` message
// reports it; the command is the third byte.
// ===========================================================================
namespace {
const KeyEntry kKeyTable[] = {
    {Key::kUp, 0x0000U, 0x00U, false},
    {Key::kDown, 0x0000U, 0x00U, false},
    {Key::kLeft, 0x0000U, 0x00U, false},
    {Key::kRight, 0x0000U, 0x00U, false},
    {Key::kOk, 0x0000U, 0x00U, false},
};
}  // namespace
// ===========================================================================

Key keyFor(const IrCode& code) {
  return lookupKey(kKeyTable, static_cast<uint8_t>(sizeof(kKeyTable) / sizeof(kKeyTable[0])), code);
}

}  // namespace oc
