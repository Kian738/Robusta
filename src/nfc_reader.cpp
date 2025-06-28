#include "nfc_reader.h"

#include "config.h"
#include "logger.h"
#include "heartbeat_manager.h"

#include <SPI.h>

MFRC522 NfcReader::mfrc522(SS_PIN, RST_PIN);

void NfcReader::init()
{
  SPI.begin();

  mfrc522.PCD_Init();
  mfrc522.PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max); // ! Todo: Remove

  Logger::printLn("NFC Reader initialized.");
}

bool NfcReader::checkForTag()
{
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return false;

  HeartbeatManager::lastActivity = millis();
  return true;
}

void NfcReader::cleanUp()
{
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  Logger::printLn("Tag cleaned up.");
}
