#pragma once

#include <MFRC522.h>

// A simple wrappper for the MFRC522 library
class NfcReader
{
private:
  // Define the SPI pins for the MFRC522
  static constexpr byte RST_PIN = 9;
  static constexpr byte SS_PIN = 10;

  static MFRC522 mfrc522;

public:
  static void init();

  static bool checkForTag();
  static void cleanUp();

  static MFRC522::Uid &getUid() { return mfrc522.uid; }
};