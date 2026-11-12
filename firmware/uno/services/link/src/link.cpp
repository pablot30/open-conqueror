#include "link.h"

namespace oc {

namespace {

bool equals(const char* a, const char* b) {
  while ((*a != '\0') && (*a == *b)) {
    ++a;
    ++b;
  }
  return *a == *b;
}

// Reads an optional '-' and one to three digits. Advances text past them.
bool readInt(const char*& text, int16_t& value) {
  bool negative = false;
  if (*text == '-') {
    negative = true;
    ++text;
  }
  int16_t result = 0;
  uint8_t digits = 0U;
  while ((*text >= '0') && (*text <= '9')) {
    if (digits == 3U) {
      return false;
    }
    result = static_cast<int16_t>((result * 10) + (*text - '0'));
    ++digits;
    ++text;
  }
  if (digits == 0U) {
    return false;
  }
  value = negative ? static_cast<int16_t>(-result) : result;
  return true;
}

bool inPercentRange(int16_t value) { return (value >= -100) && (value <= 100); }

Command make(CommandType type) {
  Command command;
  command.type = type;
  command.left = 0;
  command.right = 0;
  command.pan = 0U;
  command.tilt = 0U;
  return command;
}

Command parseDrive(const char* text) {
  // text points just after "D"
  int16_t left = 0;
  int16_t right = 0;
  if ((*text != ',') || !readInt(++text, left) || (*text != ',') ||
      !readInt(++text, right) || (*text != '\0') || !inPercentRange(left) ||
      !inPercentRange(right)) {
    return make(CommandType::kInvalid);
  }
  Command command = make(CommandType::kDrive);
  command.left = static_cast<int8_t>(left);
  command.right = static_cast<int8_t>(right);
  return command;
}

bool inAngleRange(int16_t value) { return (value >= 0) && (value <= 180); }

Command parseCamera(const char* text) {
  // text points just after "P"
  int16_t pan = 0;
  int16_t tilt = 0;
  if ((*text != ',') || !readInt(++text, pan) || (*text != ',') ||
      !readInt(++text, tilt) || (*text != '\0') || !inAngleRange(pan) ||
      !inAngleRange(tilt)) {
    return make(CommandType::kInvalid);
  }
  Command command = make(CommandType::kCamera);
  command.pan = static_cast<uint8_t>(pan);
  command.tilt = static_cast<uint8_t>(tilt);
  return command;
}

}  // namespace

Command parseMessage(const char* body) {
  if (body[0] == 'D') {
    return parseDrive(body + 1);
  }
  if (body[0] == 'P') {
    return parseCamera(body + 1);
  }
  if (equals(body, "S") || equals(body, "\"N\":100")) {
    return make(CommandType::kStop);
  }
  if (equals(body, "V")) {
    return make(CommandType::kVersion);
  }
  return make(CommandType::kInvalid);
}

Command Parser::feed(char c) {
  if (c == '{') {
    inMessage_ = true;
    overflow_ = false;
    length_ = 0U;
    return make(CommandType::kNone);
  }
  if (!inMessage_) {
    return make(CommandType::kNone);
  }
  if (c == '}') {
    inMessage_ = false;
    if (overflow_) {
      return make(CommandType::kInvalid);
    }
    body_[length_] = '\0';
    return parseMessage(body_);
  }
  if (length_ < kMaxBodyLength) {
    body_[length_] = c;
    ++length_;
  } else {
    overflow_ = true;
  }
  return make(CommandType::kNone);
}

MessageWriter::MessageWriter(char type) {
  put('{');
  put(type);
}

void MessageWriter::put(char c) {
  // Leave room for the closing brace.
  if (length_ < (kCapacity - 1U)) {
    buffer_[length_] = c;
    ++length_;
  } else {
    overflow_ = true;
  }
}

void MessageWriter::field(int32_t value) {
  put(',');
  // Work with the magnitude as unsigned, so INT32_MIN doesn't overflow.
  uint32_t magnitude = static_cast<uint32_t>(value);
  if (value < 0) {
    put('-');
    magnitude = 0U - magnitude;
  }
  char digits[10];
  uint8_t count = 0U;
  do {
    digits[count] = static_cast<char>('0' + (magnitude % 10U));
    ++count;
    magnitude /= 10U;
  } while (magnitude != 0U);
  while (count > 0U) {
    --count;
    put(digits[count]);
  }
}

void MessageWriter::field(const char* text) {
  put(',');
  while (*text != '\0') {
    put(*text);
    ++text;
  }
}

bool MessageWriter::finish() {
  if (overflow_) {
    return false;
  }
  buffer_[length_] = '}';
  ++length_;
  buffer_[length_] = '\0';
  finished_ = true;
  return true;
}

}  // namespace oc
