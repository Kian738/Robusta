#pragma once

#include <Arduino.h>

class ConnectionManager
{
private:
  static bool connected;
  static bool initialized;
  static bool debugMode;

public:
  static bool isConnected() { return connected; }
  static bool isInitialized() { return initialized; }
  static bool isDebugMode() { return debugMode; }
  
  static void setConnected(bool state);
  static void setInitialized(bool state) { initialized = state; }
  static void setDebugMode(bool mode) { debugMode = mode; }
};
