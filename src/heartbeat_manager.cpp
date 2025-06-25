#include "heartbeat_manager.h"

#include "protocol.h"

unsigned long HeartbeatManager::lastHeartbeat = 0;
unsigned long HeartbeatManager::lastActivity = 0;

void HeartbeatManager::SendIfNeeded()
{
  bool active = millis() - lastActivity < INACTIVITY_TIMEOUT;
  auto interval = active ? ACTIVE_HEARTBEAT : IDLE_HEARTBEAT;

  if (millis() - lastHeartbeat > interval)
  {
    Protocol::sendPacket(PacketType::HEARTBEAT);
    lastHeartbeat = millis();
  }
}
