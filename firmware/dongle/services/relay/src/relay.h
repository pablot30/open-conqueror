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
// Reconnecting. The R4's Wi-Fi chip has hung when asked to scan or connect
// again and again while the robot's network was gone, so steps are spaced
// out and a scan only happens when rejoining the known network keeps failing.
const uint32_t kSettleMs = 1000;      // after losing the link
const uint32_t kOpenPeriodMs = 2000;  // between attempts to open port 100
const uint32_t kJoinPeriodMs = 3000;  // between attempts to join the network
const uint32_t kScanPeriodMs = 5000;  // between scans that found nothing
const uint8_t kJoinsBeforeScan = 3;
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

// What the dongle should do next to reach the robot.
enum class Step : uint8_t {
  kWait,  // nothing yet
  kScan,  // look for the robot's network
  kJoin,  // join the network found earlier
  kOpen,  // on the network: open port 100
};

// Decides the next step after a lost link or a failed attempt, and when.
// dongle.ino asks the Wi-Fi chip whether it's on the network only when
// ready() says a step is due, which keeps commands to the chip rare.
class LinkPlan {
 public:
  bool ready(uint32_t now) const { return now - since_ >= wait_; }
  Step next(uint32_t now, bool onNetwork) const;
  void scanned(uint32_t now, bool found);
  void joined(uint32_t now, bool ok);
  void opened(uint32_t now, bool ok);
  void lost(uint32_t now) { after(now, relay::kSettleMs); }

 private:
  void after(uint32_t now, uint32_t wait) {
    since_ = now;
    wait_ = wait;
  }
  uint32_t since_ = 0;
  uint32_t wait_ = 0;
  bool known_ = false;
  uint8_t failedJoins_ = 0;
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
