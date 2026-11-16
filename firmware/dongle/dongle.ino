// The Wi-Fi dongle: an Arduino UNO R4 WiFi, plugged into the computer by USB,
// that joins the robot's own Wi-Fi and passes messages both ways between USB
// and the stock camera board's port 100. To the computer it looks like the
// robot on a serial port. See README.md.
#include <WiFiS3.h>

#include "relay.h"
#include "src/version.h"

namespace {

const IPAddress kRobotAddress(192, 168, 4, 1);  // the camera board, docs/hardware/camera-board.md
const uint16_t kRobotPort = 100;
const uint32_t kStatusPeriodMs = 1000;
const uint32_t kRetryPeriodMs = 1000;
const uint32_t kCheckPeriodMs = 1000;
const int kConnectTimeoutMs = 3000;
const char kHeartbeat[] = "{Heartbeat}";

WiFiClient robot;
oc::MessageReader fromComputer;
oc::MessageReader fromRobot;
oc::LinkWatch watch;
oc::LinkState state = oc::LinkState::kSearching;
char network[33] = {};  // the robot's network name, found by scanning
uint32_t lastStatus = 0;
uint32_t lastAttempt = 0;
uint32_t lastCheck = 0;

void sendStatus() {
  const int32_t rssi = state == oc::LinkState::kConnected || state == oc::LinkState::kConnecting
                           ? WiFi.RSSI() : 0;
  Serial.print("{W,");
  Serial.print(static_cast<int>(state));
  Serial.print(',');
  Serial.print(rssi);
  Serial.print('}');
  lastStatus = millis();
}

void setState(oc::LinkState next) {
  if (next != state) {
    state = next;
    sendStatus();
  }
}

void sendVersion() {
  Serial.print("{WV,");
  Serial.print(OC_FIRMWARE_VERSION);
  Serial.print(',');
  Serial.print(OC_GIT_COMMIT);
  Serial.print(',');
  Serial.print(WiFi.firmwareVersion());
  Serial.print('}');
}

// Messages from the computer: to the robot if it's connected, else dropped.
// The robot's 300 ms drive timeout (S3) covers anything lost here.
void pumpComputer() {
  while (Serial.available() > 0) {
    if (!fromComputer.push(static_cast<uint8_t>(Serial.read()))) {
      continue;
    }
    const char* message = fromComputer.message();
    const size_t length = fromComputer.length();
    if (oc::routeFromComputer(message, length) == oc::Route::kForDongle) {
      if (message[2] == 'V') {
        sendVersion();
      } else {
        sendStatus();
      }
    } else if (state == oc::LinkState::kConnected) {
      robot.write(reinterpret_cast<const uint8_t*>(message), length);
    }
  }
}

// Every call into WiFiClient is a text command to the R4's Wi-Fi chip and
// takes about 5 ms, so the loop makes as few as it can. One read fetches
// everything waiting; asking available() first would double the cost.
void pumpRobot(uint32_t now) {
  uint8_t chunk[64];
  int count = 0;
  do {
    count = robot.read(chunk, sizeof(chunk));
    for (int i = 0; i < count; ++i) {
      if (!fromRobot.push(chunk[i])) {
        continue;
      }
      watch.received(now);
      if (oc::routeFromRobot(fromRobot.message(), fromRobot.length()) == oc::Route::kHeartbeat) {
        robot.write(reinterpret_cast<const uint8_t*>(kHeartbeat), sizeof(kHeartbeat) - 1);
      } else {
        Serial.write(fromRobot.message(), fromRobot.length());
      }
    }
  } while (count == static_cast<int>(sizeof(chunk)));
}

// connected() costs two commands, so it runs once a second. The silence
// timeout notices a dead link in between.
bool linkLost(uint32_t now) {
  if (watch.timedOut(now)) {
    return true;
  }
  if (now - lastCheck < kCheckPeriodMs) {
    return false;
  }
  lastCheck = now;
  return !robot.connected();
}

void search() {
  sendStatus();  // the scan blocks for a few seconds
  const int count = WiFi.scanNetworks();
  const char* names[16] = {};
  int32_t rssi[16] = {};
  const int listed = count < 16 ? count : 16;
  for (int i = 0; i < listed; ++i) {
    names[i] = WiFi.SSID(static_cast<uint8_t>(i));
    rssi[i] = WiFi.RSSI(static_cast<uint8_t>(i));
  }
  const int best = oc::pickNetwork(names, rssi, listed);
  if (best >= 0) {
    strncpy(network, names[best], sizeof(network) - 1);
    setState(oc::LinkState::kJoining);
  }
}

void step(uint32_t now) {
  switch (state) {
    case oc::LinkState::kSearching:
      if (now - lastAttempt >= kRetryPeriodMs) {
        lastAttempt = now;
        search();
      }
      break;
    case oc::LinkState::kJoining:
      // Open network: the stock camera firmware has no password.
      setState(WiFi.begin(network) == WL_CONNECTED ? oc::LinkState::kConnecting
                                                   : oc::LinkState::kSearching);
      lastAttempt = millis();
      break;
    case oc::LinkState::kConnecting:
      if (WiFi.status() != WL_CONNECTED) {
        setState(oc::LinkState::kSearching);
      } else if (now - lastAttempt >= kRetryPeriodMs) {
        lastAttempt = now;
        if (robot.connect(kRobotAddress, kRobotPort)) {
          watch.connected(millis());
          setState(oc::LinkState::kConnected);
        }
      }
      break;
    case oc::LinkState::kConnected:
      pumpRobot(now);
      if (linkLost(millis())) {
        robot.stop();
        lastAttempt = millis();
        setState(WiFi.status() == WL_CONNECTED ? oc::LinkState::kConnecting
                                               : oc::LinkState::kSearching);
      }
      break;
  }
}

}  // namespace

void setup() {
  // The R4's USB socket reaches this chip through its ESP32-S3, as a USB-to-serial
  // converter, so the computer must open the port at this same rate.
  Serial.begin(115200);
  robot.setConnectionTimeout(kConnectTimeoutMs);
  sendVersion();
}

void loop() {
  const uint32_t now = millis();
  pumpComputer();
  step(now);
  if (millis() - lastStatus >= kStatusPeriodMs) {
    sendStatus();
  }
}
