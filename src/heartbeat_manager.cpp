#include "heartbeat_manager.h"

#include "protocol.h"
#include "connection_manager.h"
#include "logger.h"

unsigned long HeartbeatManager::lastHeartbeat = 0;
unsigned long HeartbeatManager::lastActivity = 0;
unsigned long HeartbeatManager::lastHeartbeatAck = 0;

void HeartbeatManager::checkAndSendHeartbeat()
{
  // Check for connection timeout (server not responding to heartbeats)
  if (lastHeartbeatAck > 0 && (unsigned long)(millis() - lastHeartbeatAck) > CONNECTION_TIMEOUT)
  {
    Logger::printLn("Connection timeout - server not responding to heartbeats");
    ConnectionManager::setConnected(false);
    return;
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

void HeartbeatManager::onHeartbeatAck()
{
  lastHeartbeatAck = millis();
}
