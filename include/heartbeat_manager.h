#pragma once

#include <Arduino.h>

class HeartbeatManager
{
private:
  static constexpr unsigned long ACTIVE_HEARTBEAT = 5UL * 1000;         // 5 seconds
  static constexpr unsigned long IDLE_HEARTBEAT = 4UL * 60 * 1000;      // 4 minutes
  static constexpr unsigned long INACTIVITY_TIMEOUT = 10UL * 60 * 1000; // 10 minutes
  static constexpr unsigned long CONNECTION_TIMEOUT = 10UL * 1000;      // 10 seconds

  static unsigned long lastHeartbeat;
  static unsigned long lastActivity;
  static unsigned long lastHeartbeatAck;

  friend class NfcReader; // Allow NfcReader access to lastActivity
public:
  static void checkAndSendHeartbeat();
  static void onHeartbeatAck(); // Called when HEARTBEAT_ACK is received
};
