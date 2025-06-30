#include "packet_handlers.h"

#include "protocol.h"
#include "config.h"
#include "gpio.h"
#include "logger.h"

void handleVerifyResult(const byte *payload, byte length)
{
  if (length != 1)
    return; // Invalid payload length

  if (payload[0] != 0x01)
  {
    Logger::printLn("Tag verification failed.");
    Gpio::playErrorChord();
    return;
  }

  Logger::printLn("Tag verification successful.");
  handleOpenRegister(nullptr, 0);
};

void handleSetDebug(const byte *payload, byte length)
{
  if (length != 1)
    return;

  debugMode = payload[0] == 0x01;
  Logger::printLn("Enabled debug mode."); // This will only print if debugMode is true
};

void handleOpenRegister(const byte *payload, byte length)
{
  if (length != 0)
    return;

  Gpio::beep();
  Gpio::openRegister();
};
