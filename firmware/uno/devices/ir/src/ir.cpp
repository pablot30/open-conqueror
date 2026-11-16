#include "ir.h"

namespace oc {

namespace {

bool within(uint16_t value, uint16_t low, uint16_t high) { return (value >= low) && (value <= high); }

bool isLeadMark(uint16_t us) { return within(us, config::kIrLeadMarkMinUs, config::kIrLeadMarkMaxUs); }

}  // namespace

IrCode decodeNec(const uint16_t* widthsUs, uint8_t count) {
  IrCode code = {IrKind::kNone, 0U, 0U};
  if ((widthsUs == nullptr) || (count == 0U)) {
    return code;
  }
  code.kind = IrKind::kUnreadable;
  if (!isLeadMark(widthsUs[0]) || (count < 2U)) {
    return code;
  }

  // A repeat code: lead mark, a short space, an end mark.
  if (within(widthsUs[1], config::kIrRepeatSpaceMinUs, config::kIrRepeatSpaceMaxUs)) {
    if (count <= 3U) {
      code.kind = IrKind::kRepeat;
    }
    return code;
  }

  // A frame: lead mark and space, 32 mark and space pairs, and an end mark.
  // The end mark's width isn't needed, so a frame without it still counts.
  if (!within(widthsUs[1], config::kIrLeadSpaceMinUs, config::kIrLeadSpaceMaxUs) || (count < 66U) ||
      (count > 67U)) {
    return code;
  }
  uint32_t bits = 0U;
  for (uint8_t i = 0U; i < 32U; ++i) {
    const uint16_t mark = widthsUs[2U + (2U * i)];
    const uint16_t space = widthsUs[3U + (2U * i)];
    if (!within(mark, config::kIrBitMarkMinUs, config::kIrBitMarkMaxUs) ||
        !within(space, config::kIrSpaceMinUs, config::kIrSpaceMaxUs)) {
      return code;
    }
    if (space >= config::kIrSpaceOneMinUs) {
      bits |= (static_cast<uint32_t>(1U) << i);  // least significant bit first
    }
  }
  const uint8_t commandByte = static_cast<uint8_t>((bits >> 16) & 0xFFU);
  const uint8_t commandInverse = static_cast<uint8_t>((bits >> 24) & 0xFFU);
  if (static_cast<uint8_t>(commandByte ^ commandInverse) != 0xFFU) {
    return code;  // the command and its inverse disagree: a bit was misread
  }
  code.kind = IrKind::kFrame;
  code.address = static_cast<uint16_t>(bits & 0xFFFFU);
  code.command = commandByte;
  return code;
}

Key lookupKey(const KeyEntry* table, uint8_t count, const IrCode& code) {
  if (code.kind != IrKind::kFrame) {
    return Key::kNone;
  }
  for (uint8_t i = 0U; i < count; ++i) {
    if (table[i].known && (table[i].address == code.address) && (table[i].command == code.command)) {
      return table[i].key;
    }
  }
  return Key::kNone;
}

}  // namespace oc
