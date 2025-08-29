#include "connection_manager.h"
#include "logger.h"
#include "heartbeat_manager.h"

bool ConnectionManager::connected = false;
bool ConnectionManager::initialized = false;
bool ConnectionManager::debugMode = false;

void ConnectionManager::setConnected(bool state)
{
  if (state && !connected)
    Logger::printLn("Connection established.");
  else if (!state && connected)
    Logger::printLn("Connection lost.");

  connected = state;
}
