#include "packet_handlers.h"

#include "protocol.h"
#include "config.h"
#include "gpio.h"
#include "logger.h"

#include <MFRC522.h>

MFRC522 mfrc522(SS_PIN, RST_PIN);

void handleVerifyResult(const byte *payload, byte length)
{
  if (length != 1)
    return; // Invalid payload length

  if (payload[0] != 0x01)
  {
    Logger::print("Tag verification failed.");
    Gpio::playErrorChord();
    return;
  }

  Logger::print("Tag verification successful.");

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  // Register the tag
  digitalWrite(REGISTER_PIN, HIGH);
  delay(100);
  digitalWrite(REGISTER_PIN, LOW);
};

void handleSetDebug(const byte *payload, byte length)
{
  if (length != 1)
    return;

  debugMode = (bool)payload[0];
  Logger::print("Enabled debug mode."); // This will only print if debugMode is true
};

void handleFlushLog(const byte *payload, byte length)
{
  if (length != 0)
    return;

  Logger::flush();
  Logger::print("Log flushed.");
};
