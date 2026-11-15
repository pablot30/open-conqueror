#include "ir.h"

namespace oc {

namespace {

bool isArrow(Key key) {
  return (key == Key::kUp) || (key == Key::kDown) || (key == Key::kLeft) || (key == Key::kRight);
}

const RemoteRequest kNothing = {RemoteAction::kNone, false, 0, 0};

}  // namespace

RemoteRequest RemoteDriver::driveFor(Key key, bool newPress) const {
  const int8_t speed = config::kRemoteSpeedPercent;
  RemoteRequest request = {RemoteAction::kDrive, newPress, 0, 0};
  switch (key) {
    case Key::kUp:
      request.left = speed;
      request.right = speed;
      break;
    case Key::kDown:
      request.left = static_cast<int8_t>(-speed);
      request.right = static_cast<int8_t>(-speed);
      break;
    case Key::kLeft:  // spin left on the spot, as `conq drive l`
      request.left = static_cast<int8_t>(-speed);
      request.right = speed;
      break;
    case Key::kRight:
      request.left = speed;
      request.right = static_cast<int8_t>(-speed);
      break;
    default:
      return kNothing;
  }
  return request;
}

RemoteRequest RemoteDriver::onKey(Key key, uint32_t nowMs) {
  if (key == Key::kOk) {
    holding_ = false;
    held_ = Key::kNone;
    RemoteRequest stop = kNothing;
    stop.action = RemoteAction::kStop;
    return stop;
  }
  if (!isArrow(key)) {
    return kNothing;  // unknown keys, and mode keys until increment 3
  }
  // Some remotes resend the whole frame while a key is held instead of
  // sending repeat codes. The same key again before the release time is the
  // same press, not a deliberate new one.
  const bool sameHold = holding_ && (held_ == key) && ((nowMs - lastCodeMs_) < config::kRemoteReleaseMs);
  holding_ = true;
  held_ = key;
  lastCodeMs_ = nowMs;
  return driveFor(key, !sameHold);
}

RemoteRequest RemoteDriver::onRepeat(uint32_t nowMs) {
  if (!holding_) {
    return kNothing;  // after OK or a release, repeat codes never drive
  }
  lastCodeMs_ = nowMs;
  return driveFor(held_, false);
}

RemoteRequest RemoteDriver::update(uint32_t nowMs) {
  // Unsigned subtraction stays correct when millis() wraps around.
  if (holding_ && ((nowMs - lastCodeMs_) >= config::kRemoteReleaseMs)) {
    holding_ = false;
    held_ = Key::kNone;
    RemoteRequest stop = kNothing;
    stop.action = RemoteAction::kStop;
    return stop;
  }
  return kNothing;
}

}  // namespace oc
