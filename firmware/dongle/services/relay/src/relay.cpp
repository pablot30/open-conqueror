#include "relay.h"

#include <string.h>

namespace oc {

namespace {

const char kHeartbeat[] = "{Heartbeat}";

bool startsWith(const char* text, const char* prefix) {
  return strncmp(text, prefix, strlen(prefix)) == 0;
}

}  // namespace

bool MessageReader::push(uint8_t b) {
  const char c = static_cast<char>(b);
  if (c == '{') {  // a new message starts, even inside one that never ended
    inside_ = true;
    overflow_ = false;
    length_ = 0;
  }
  if (!inside_) {
    return false;
  }
  if (length_ < relay::kMaxMessage) {
    buffer_[length_++] = c;
  } else {
    overflow_ = true;
  }
  if (c != '}') {
    return false;
  }
  inside_ = false;
  if (overflow_) {
    length_ = 0;
    return false;
  }
  buffer_[length_] = '\0';
  return true;
}

Route routeFromComputer(const char* message, size_t length) {
  if (length >= 3 && message[1] == 'W' && (message[2] == '}' || message[2] == ',' || message[2] == 'V')) {
    return Route::kForDongle;
  }
  return Route::kToRobot;
}

Route routeFromRobot(const char* message, size_t length) {
  if (length == sizeof(kHeartbeat) - 1 && memcmp(message, kHeartbeat, length) == 0) {
    return Route::kHeartbeat;
  }
  return Route::kToComputer;
}

int pickNetwork(const char* const names[], const int32_t rssi[], int count) {
  int best = -1;
  for (int i = 0; i < count; ++i) {
    if (names[i] == nullptr || !startsWith(names[i], relay::kNetworkPrefix)) {
      continue;
    }
    if (best < 0 || rssi[i] > rssi[best]) {
      best = i;
    }
  }
  return best;
}

}  // namespace oc
