#include "heartbeat_manager.h"

#include "protocol.h"
#include "connection_manager.h"
#include "logger.h"

bool HeartbeatManager::isWaitingForAck = false;

unsigned long HeartbeatManager::lastHeartbeat = 0;
unsigned long HeartbeatManager::lastActivity = 0;

void HeartbeatManager::checkAndSendHeartbeat()
{
  // Check for connection timeout (server not responding to heartbeats)
  if (isWaitingForAck)
  {
    if ((unsigned long)(millis() - lastHeartbeat) > CONNECTION_TIMEOUT)
    {
      Logger::printLn("Server not responding to heartbeats");
      ConnectionManager::setConnected(false);
    }
    return; // Wait for heartbeat ack, before sending another heartbeat
  }

  bool active = lastActivity && (unsigned long)(millis() - lastActivity) < INACTIVITY_TIMEOUT;
  auto interval = active ? ACTIVE_HEARTBEAT : IDLE_HEARTBEAT;

  // Send heartbeat if needed
  if ((unsigned long)(millis() - lastHeartbeat) > interval)
  {
    Protocol::sendPacket(PacketType::HEARTBEAT);
    lastHeartbeat = millis();
  }
}
