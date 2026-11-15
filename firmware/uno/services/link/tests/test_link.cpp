#include <cstring>

#include "check.h"
#include "link.h"

using oc::Command;
using oc::CommandType;

namespace {

// Feeds a string and returns the last complete command, or kNone.
Command feedAll(oc::Parser& parser, const char* text) {
  Command last = oc::parseMessage("");
  last.type = CommandType::kNone;
  for (const char* c = text; *c != '\0'; ++c) {
    const Command command = parser.feed(*c);
    if (command.type != CommandType::kNone) {
      last = command;
    }
  }
  return last;
}

void parsesDrive() {
  const Command command = oc::parseMessage("D,50,-100");
  CHECK(command.type == CommandType::kDrive);
  CHECK(command.left == 50);
  CHECK(command.right == -100);
}

void rejectsBadDrive() {
  CHECK(oc::parseMessage("D,101,0").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,0,-101").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,1000,0").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,5").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,5,").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,5,5,5").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D,-,5").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("D50,50").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("").type == CommandType::kInvalid);
}

void parsesStopAndVersion() {
  CHECK(oc::parseMessage("S").type == CommandType::kStop);
  CHECK(oc::parseMessage("V").type == CommandType::kVersion);
  const Command camera = oc::parseMessage("P,60,120");
  CHECK(camera.type == CommandType::kCamera && camera.pan == 60U && camera.tilt == 120U);
  CHECK(oc::parseMessage("P,0,180").type == CommandType::kCamera);
  CHECK(oc::parseMessage("P,181,90").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,-1,90").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,90").type == CommandType::kInvalid);
  CHECK(!camera.hasSpeed);
  const Command slow = oc::parseMessage("P,60,120,25");
  CHECK(slow.type == CommandType::kCamera && slow.hasSpeed && slow.speed == 25U);
  const Command jump = oc::parseMessage("P,60,120,0");
  CHECK(jump.type == CommandType::kCamera && jump.hasSpeed && jump.speed == 0U);
  CHECK(oc::parseMessage("P,60,120,999").type == CommandType::kCamera);
  CHECK(oc::parseMessage("P,60,120,1000").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,60,120,-1").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,60,120,").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,60,120,5,6").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("P,181,90,50").type == CommandType::kInvalid);
  CHECK(oc::parseMessage("SS").type == CommandType::kInvalid);
}

void bridgeDisconnectStops() {
  // The stock camera bridge sends this when a client disconnects.
  CHECK(oc::parseMessage("\"N\":100").type == CommandType::kStop);
  CHECK(oc::parseMessage("\"N\":1").type == CommandType::kInvalid);
}

void parserFindsMessagesInNoise() {
  oc::Parser parser;
  CHECK(feedAll(parser, "junk\r\n{D,10,20}\n").type == CommandType::kDrive);
  CHECK(feedAll(parser, "}}}").type == CommandType::kNone);
}

void newBraceRestarts() {
  oc::Parser parser;
  const Command command = feedAll(parser, "{D,10{S}");
  CHECK(command.type == CommandType::kStop);
}

void overlongMessageIsInvalid() {
  oc::Parser parser;
  CHECK(feedAll(parser, "{SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS}").type == CommandType::kInvalid);
  // The parser recovers for the next message.
  CHECK(feedAll(parser, "{S}").type == CommandType::kStop);
}

void writesTelemetry() {
  oc::MessageWriter message('T');
  message.field(static_cast<int32_t>(1234));
  message.field(static_cast<int32_t>(-182));
  message.field(static_cast<int32_t>(0));
  message.field("abc");
  CHECK(message.finish());
  CHECK(std::strcmp(message.data(), "{T,1234,-182,0,abc}") == 0);
  CHECK(message.length() == std::strlen("{T,1234,-182,0,abc}"));
}

void writesExtremeNumbers() {
  oc::MessageWriter message('X');
  message.field(static_cast<int32_t>(2147483647));
  message.field(static_cast<int32_t>(-2147483647 - 1));
  CHECK(message.finish());
  CHECK(std::strcmp(message.data(), "{X,2147483647,-2147483648}") == 0);
}

void refusesOverflow() {
  oc::MessageWriter message('X');
  for (int i = 0; i < 20; ++i) {
    message.field(static_cast<int32_t>(123456));
  }
  CHECK(!message.finish());
  CHECK(message.length() == 0U);
}

}  // namespace

void runUnitTests() {
  parsesDrive();
  rejectsBadDrive();
  parsesStopAndVersion();
  bridgeDisconnectStops();
  parserFindsMessagesInNoise();
  newBraceRestarts();
  overlongMessageIsInvalid();
  writesTelemetry();
  writesExtremeNumbers();
  refusesOverflow();
}
