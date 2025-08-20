#include "logger.h"
#include "protocol.h"
#include "nfc_reader.h"
#include "gpio.h"
#include "heartbeat_manager.h"
#include "connection_manager.h"

#include <Arduino.h>

constexpr unsigned long BAUD_RATE = 115200;
constexpr unsigned long AVAILABLE_INTERVAL = 1000;

static unsigned long lastAvailablePacket = 0;

void setup()
{
  Serial.begin(BAUD_RATE);

  while (!Serial)
    ;
}

void loop()
{
  Protocol::handleIncomingSerial();

  if (!ConnectionManager::isConnected())
  {
    if ((unsigned long)(millis() - lastAvailablePacket) > AVAILABLE_INTERVAL)
    {
      Protocol::sendPacket(PacketType::AVAILABLE);
      lastAvailablePacket = millis();
    }
    return; // Wait for connection
  }

  HeartbeatManager::checkAndSendHeartbeat();

  Gpio::checkRegisterState();

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

  Protocol::sendPacket(PacketType::VERIFY_REQUEST, uid.uidByte, uid.size);

  NfcReader::cleanUp();
}
