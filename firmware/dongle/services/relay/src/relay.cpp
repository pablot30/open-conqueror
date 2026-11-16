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

Step LinkPlan::next(uint32_t now, bool onNetwork) const {
  if (!ready(now)) {
    return Step::kWait;
  }
  if (onNetwork) {
    return Step::kOpen;
  }
  if (!known_ || failedJoins_ >= relay::kJoinsBeforeScan) {
    return Step::kScan;
  }
  return Step::kJoin;
}

void LinkPlan::scanned(uint32_t now, bool found) {
  known_ = found;
  failedJoins_ = 0;
  after(now, found ? 0U : relay::kScanPeriodMs);
}

void LinkPlan::joined(uint32_t now, bool ok) {
  if (ok) {
    failedJoins_ = 0;
    after(now, 0U);
  } else {
    if (failedJoins_ < 255U) {
      ++failedJoins_;
    }
    after(now, relay::kJoinPeriodMs);
  }
}

void LinkPlan::opened(uint32_t now, bool ok) { after(now, ok ? 0U : relay::kOpenPeriodMs); }

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
