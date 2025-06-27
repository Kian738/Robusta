#pragma once

#include <Arduino.h>

class HeartbeatManager
{
private:
  static constexpr unsigned long ACTIVE_HEARTBEAT = 5000;     // 5 seconds
  static constexpr unsigned long IDLE_HEARTBEAT = 240000;     // 4 minutes
  static constexpr unsigned long INACTIVITY_TIMEOUT = 600000; // 10 minutes

  static unsigned long lastHeartbeat;
  static unsigned long lastActivity;

  friend class NfcReader; // Allow Protocol access to lastActivity
public:
  static void updateActivity() { lastActivity = millis(); }
  static void sendIfNeeded();
};
