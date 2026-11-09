// The link protocol: messages are short text inside braces, so they pass
// through the stock camera firmware's bridge unchanged. See
// firmware/README.md for the message list.
#ifndef OC_CORE_PROTOCOL_H
#define OC_CORE_PROTOCOL_H

#include <stdint.h>

namespace oc {

enum class CommandType : uint8_t {
  kNone,     // no complete message yet
  kDrive,    // {D,left,right}, each -100 to 100 percent
  kStop,     // {S}, or the bridge's {"N":100} when a client disconnects
  kVersion,  // {V}
  kInvalid,  // a complete message we don't understand
};

struct Command {
  CommandType type;
  int8_t left;
  int8_t right;
};

// Parses the body of one message, without the braces.
Command parseMessage(const char* body);

// Collects bytes into messages. Bytes outside braces are ignored, and a
// new '{' starts over, so a corrupted message can't swallow the next one.
class Parser {
 public:
  // Feeds one byte. Returns the command when a message ends, otherwise kNone.
  Command feed(char c);

  static const uint8_t kMaxBodyLength = 24U;

 private:
  char body_[kMaxBodyLength + 1U] = {};
  uint8_t length_ = 0U;
  bool inMessage_ = false;
  bool overflow_ = false;
};

// Builds one outgoing message: {type,field,field,...}
class MessageWriter {
 public:
  explicit MessageWriter(char type);
  void field(int32_t value);
  void field(const char* text);
  // Closes the message. Returns false if it didn't fit, in which case
  // length() is 0 and nothing should be sent.
  bool finish();
  const char* data() const { return buffer_; }
  uint8_t length() const { return finished_ ? length_ : 0U; }

  static const uint8_t kCapacity = 63U;

 private:
  void put(char c);
  char buffer_[kCapacity + 1U] = {};
  uint8_t length_ = 0U;
  bool overflow_ = false;
  bool finished_ = false;
};

}  // namespace oc

#endif  // OC_CORE_PROTOCOL_H
