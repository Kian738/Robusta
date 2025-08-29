#pragma once

#include <Arduino.h>

class HeartbeatManager
{
private:
  static constexpr unsigned long ACTIVE_HEARTBEAT = 5UL * 1000;         // 5 seconds
  static constexpr unsigned long IDLE_HEARTBEAT = 4UL * 60 * 1000;      // 4 minutes
  static constexpr unsigned long INACTIVITY_TIMEOUT = 10UL * 60 * 1000; // 10 minutes
  static constexpr unsigned long CONNECTION_TIMEOUT = 10UL * 1000;      // 10 seconds

  static bool isWaitingForAck;

  static unsigned long lastHeartbeat;
  static unsigned long lastActivity;

  friend class NfcReader; // Allow NfcReader access to lastActivity
  friend void handleHeartbeatAck(const byte *payload, byte length); // Allow PacketHandlers to reset isWaitingForAck
public:
  static void checkAndSendHeartbeat();
};
