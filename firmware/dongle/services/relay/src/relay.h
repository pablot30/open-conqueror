// The dongle's logic, with no hardware in it: cutting the byte streams into
// {...} messages, answering the camera board's heartbeat, choosing the robot's
// network and noticing when the link has died. dongle.ino connects it to USB
// and Wi-Fi.
#ifndef OC_RELAY_H
#define OC_RELAY_H

#include <stddef.h>
#include <stdint.h>

namespace oc {

namespace relay {
// Longest message we pass on, braces included. Ours are under 60 bytes.
const size_t kMaxMessage = 96;
// The camera board sends a heartbeat each second. Without any message for this
// long, the connection is treated as dead and opened again.
const uint32_t kLinkTimeoutMs = 3500;
// The stock camera firmware's access point name starts with this.
const char kNetworkPrefix[] = "ELEGOO-";
}  // namespace relay

// Collects bytes into one {...} message at a time. Bytes outside braces are
// dropped, and so is a message longer than kMaxMessage.
class MessageReader {
 public:
  // Returns true when b completes a message, which message() then holds,
  // braces included, until the next push().
  bool push(uint8_t b);
  const char* message() const { return buffer_; }
  size_t length() const { return length_; }

 private:
  char buffer_[relay::kMaxMessage + 1] = {};
  size_t length_ = 0;
  bool inside_ = false;
  bool overflow_ = false;
};

// What to do with a whole message.
enum class Route : uint8_t {
  kToRobot,     // from the computer: send it over Wi-Fi
  kToComputer,  // from the robot: send it over USB
  kHeartbeat,   // from the camera board: answer it, don't pass it on
  kForDongle,   // from the computer, starting with {W: the dongle answers it
};

Route routeFromComputer(const char* message, size_t length);
Route routeFromRobot(const char* message, size_t length);

// The index of the strongest network whose name starts with kNetworkPrefix,
// or -1 if there's none.
int pickNetwork(const char* const names[], const int32_t rssi[], int count);

// The link's state, as the dongle reports it in {W,state,rssi}.
enum class LinkState : uint8_t {
  kSearching = 0,   // looking for the robot's network
  kJoining = 1,     // joining it
  kConnecting = 2,  // on the network, opening port 100
  kConnected = 3,   // talking to the robot
};

// Notices a dead connection: the camera board stops sending, but the socket
// still looks open, for example when the robot is switched off.
class LinkWatch {
 public:
  void connected(uint32_t now) { last_ = now; }
  void received(uint32_t now) { last_ = now; }
  bool timedOut(uint32_t now) const { return now - last_ > relay::kLinkTimeoutMs; }

 private:
  uint32_t last_ = 0;
};

}  // namespace oc

#endif  // OC_RELAY_H
