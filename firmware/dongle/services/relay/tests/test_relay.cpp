#include <stdint.h>
#include <string.h>

#include "check.h"
#include "relay.h"

using oc::LinkWatch;
using oc::MessageReader;
using oc::Route;

namespace {

// Pushes text and returns how many messages it completed; the last stays in reader.
int pushAll(MessageReader& reader, const char* text) {
  int complete = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    if (reader.push(static_cast<uint8_t>(*p))) {
      ++complete;
    }
  }
  return complete;
}

void readsOneMessage() {
  MessageReader reader;
  CHECK(pushAll(reader, "{D,30,30}") == 1);
  CHECK(strcmp(reader.message(), "{D,30,30}") == 0);
  CHECK(reader.length() == 9U);
}

void dropsBytesOutsideBraces() {
  MessageReader reader;
  CHECK(pushAll(reader, "noise\r\n{S}\n") == 1);
  CHECK(strcmp(reader.message(), "{S}") == 0);
}

void restartsOnANewBrace() {  // a message cut short by a lost byte is dropped
  MessageReader reader;
  CHECK(pushAll(reader, "{T,12,1{V}") == 1);
  CHECK(strcmp(reader.message(), "{V}") == 0);
}

void dropsOverlongMessages() {
  MessageReader reader;
  char text[oc::relay::kMaxMessage + 3];
  memset(text, 'x', sizeof(text));
  text[0] = '{';
  text[sizeof(text) - 2] = '}';
  text[sizeof(text) - 1] = '\0';
  CHECK(pushAll(reader, text) == 0);
  CHECK(pushAll(reader, "{S}") == 1);  // and recovers at the next message
  CHECK(strcmp(reader.message(), "{S}") == 0);
}

void keepsTheLongestAllowedMessage() {
  MessageReader reader;
  char text[oc::relay::kMaxMessage + 1];
  memset(text, 'x', sizeof(text));
  text[0] = '{';
  text[oc::relay::kMaxMessage - 1] = '}';
  text[oc::relay::kMaxMessage] = '\0';
  CHECK(pushAll(reader, text) == 1);
  CHECK(reader.length() == oc::relay::kMaxMessage);
}

void routesHeartbeats() {
  CHECK(oc::routeFromRobot("{Heartbeat}", 11) == Route::kHeartbeat);
  CHECK(oc::routeFromRobot("{T,1,0,0,0,7400,0,812,0}", 24) == Route::kToComputer);
  CHECK(oc::routeFromRobot("{Heartbeats}", 12) == Route::kToComputer);
}

void routesDongleMessages() {
  CHECK(oc::routeFromComputer("{W}", 3) == Route::kForDongle);
  CHECK(oc::routeFromComputer("{WV}", 4) == Route::kForDongle);
  CHECK(oc::routeFromComputer("{W,1}", 5) == Route::kForDongle);
  CHECK(oc::routeFromComputer("{V}", 3) == Route::kToRobot);
  CHECK(oc::routeFromComputer("{D,0,0}", 7) == Route::kToRobot);
  CHECK(oc::routeFromComputer("{WA_OK}", 7) == Route::kToRobot);  // not ours
}

void picksTheStrongestRobotNetwork() {
  const char* names[] = {"home", "ELEGOO-1A2B", "ELEGOO-3C4D", "ELEGOO"};
  const int32_t rssi[] = {-30, -70, -55, -20};
  CHECK(oc::pickNetwork(names, rssi, 4) == 2);
}

void findsNoRobotNetwork() {
  const char* names[] = {"home", nullptr};
  const int32_t rssi[] = {-30, -40};
  CHECK(oc::pickNetwork(names, rssi, 2) == -1);
  CHECK(oc::pickNetwork(names, rssi, 0) == -1);
}

void timesOutASilentLink() {
  LinkWatch watch;
  watch.connected(1000U);
  CHECK(!watch.timedOut(1000U + oc::relay::kLinkTimeoutMs));
  CHECK(watch.timedOut(1001U + oc::relay::kLinkTimeoutMs));
  watch.received(4000U);
  CHECK(!watch.timedOut(4000U + oc::relay::kLinkTimeoutMs));
}

void survivesTheMillisWrap() {
  LinkWatch watch;
  watch.received(0xFFFFFF00U);
  CHECK(!watch.timedOut(0x00000100U));  // 512 ms later
  CHECK(watch.timedOut(0xFFFFFF00U + oc::relay::kLinkTimeoutMs + 1U));
}

void scansFirstThenOpens() {
  oc::LinkPlan plan;
  CHECK(plan.next(0U, false) == oc::Step::kScan);
  plan.scanned(100U, true);
  CHECK(plan.next(100U, false) == oc::Step::kJoin);
  plan.joined(200U, true);
  CHECK(plan.next(200U, true) == oc::Step::kOpen);
}

void rescansSlowlyWhenNothingFound() {
  oc::LinkPlan plan;
  plan.scanned(0U, false);
  CHECK(plan.next(oc::relay::kScanPeriodMs - 1U, false) == oc::Step::kWait);
  CHECK(plan.next(oc::relay::kScanPeriodMs, false) == oc::Step::kScan);
}

void rejoinsTheKnownNetworkWithoutScanning() {
  oc::LinkPlan plan;
  plan.scanned(0U, true);
  plan.joined(0U, true);
  plan.opened(0U, true);
  plan.lost(10000U);
  CHECK(plan.next(10000U + oc::relay::kSettleMs - 1U, false) == oc::Step::kWait);
  CHECK(plan.next(10000U + oc::relay::kSettleMs, false) == oc::Step::kJoin);
  CHECK(plan.next(10000U + oc::relay::kSettleMs, true) == oc::Step::kOpen);  // still on the network
}

void scansAgainAfterFailedJoins() {
  oc::LinkPlan plan;
  plan.scanned(0U, true);
  uint32_t now = 0U;
  for (uint8_t i = 0U; i < oc::relay::kJoinsBeforeScan; ++i) {
    CHECK(plan.next(now, false) == oc::Step::kJoin);
    plan.joined(now, false);
    CHECK(plan.next(now + oc::relay::kJoinPeriodMs - 1U, false) == oc::Step::kWait);
    now += oc::relay::kJoinPeriodMs;
  }
  CHECK(plan.next(now, false) == oc::Step::kScan);
  plan.scanned(now, true);
  CHECK(plan.next(now, false) == oc::Step::kJoin);  // the count starts again
}

void spacesOutFailedOpens() {
  oc::LinkPlan plan;
  plan.scanned(0U, true);
  plan.joined(0U, true);
  plan.opened(500U, false);
  CHECK(plan.next(500U + oc::relay::kOpenPeriodMs - 1U, true) == oc::Step::kWait);
  CHECK(plan.next(500U + oc::relay::kOpenPeriodMs, true) == oc::Step::kOpen);
}

}  // namespace

void runUnitTests() {
  readsOneMessage();
  dropsBytesOutsideBraces();
  restartsOnANewBrace();
  dropsOverlongMessages();
  keepsTheLongestAllowedMessage();
  routesHeartbeats();
  routesDongleMessages();
  picksTheStrongestRobotNetwork();
  findsNoRobotNetwork();
  timesOutASilentLink();
  survivesTheMillisWrap();
  scansFirstThenOpens();
  rescansSlowlyWhenNothingFound();
  rejoinsTheKnownNetworkWithoutScanning();
  scansAgainAfterFailedJoins();
  spacesOutFailedOpens();
}
