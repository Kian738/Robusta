#include "packet_handlers.h"

#include "protocol.h"
#include "nfc_reader.h"
#include "connection_manager.h"
#include "heartbeat_manager.h"
#include "gpio.h"
#include "logger.h"

void handleConnectRequest(const byte *payload, byte length)
{
  if (length != 1)
    return; // Invalid payload length

  ConnectionManager::setDebugMode(payload[0] == 0x01);
  Logger::printLn("Starting with debug mode."); // Printing only works when isDebugMode is true

  if (!ConnectionManager::isInitialized())
  {
    Logger::printLn("Starting NFC Reader...");
    NfcReader::init();

    Logger::printLn("Setting up GPIO...");
    Gpio::init();

    ConnectionManager::setInitialized(true);
  }

  Logger::printLn("Playing startup chord...");
  Gpio::playStartupChord();

  ConnectionManager::setConnected(true);

  auto resPayload = static_cast<byte>(Gpio::isRegisterOpen());
  Protocol::sendPacket(PacketType::CONNECT_RESPONSE, &resPayload, 1);
}

void handleVerifyResponse(const byte *payload, byte length)
{
  if (length != 1)
    return;

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

  ConnectionManager::setDebugMode(payload[0] == 0x01);
  Logger::printLn("Enabled debug mode.");
};

void handleOpenRegister(const byte *payload, byte length)
{
  if (length != 0)
    return;

  Gpio::beep();
  Gpio::openRegister();
};

void handleHeartbeatAck(const byte *payload, byte length)
{
  if (length != 0)
    return;

  HeartbeatManager::onHeartbeatAck();
};
