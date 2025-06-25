#include "protocol.h"
#include "logger.h"
#include "nfc_reader.h"
#include "gpio.h"
#include "heartbeat_manager.h"

#include <Arduino.h>

constexpr unsigned long BAUD_RATE = 115200;

void setup()
{
  Serial.begin(BAUD_RATE);

  while (!Serial)
    ;

  Logger::print("Starting NFC Reader...");
  NfcReader::init();

  Gpio::init();

  Logger::print("Playing startup chord...");
  Gpio::playStartupChord();
}

void loop()
{
  Protocol::handleIncomingSerial();
  HeartbeatManager::SendIfNeeded();

  if (!NfcReader::checkForTag())
    return;

  Logger::print("Read UID tag: 0x");
  const auto &uid = NfcReader::getUid();
  for (byte i = 0; i < uid.size; i++)
  {
    if (uid.uidByte[i] < 0x10)
      Logger::print("0");
    Logger::print(uid.uidByte[i], HEX);
  }
  Logger::newLine();

  Protocol::sendPacket(PacketType::VERIFY_UID, uid.uidByte, uid.size);

  NfcReader::cleanUp();
}
