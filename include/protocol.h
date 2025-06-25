#pragma once

#include <Arduino.h>

// Todo: Packet to enable tag registration mode
enum class PacketType : byte
{
  VERIFY_UID = 0x01, // Verify the UID of a tag
  VERIFY_RESULT,     // Result of the verification
  HEARTBEAT,         // Heartbeat packet to check if the device is active
  SET_DEBUG,         // Set debug mode
  STATUS,            // Status packet
  LOG,               // Log packet
  FLUSH_LOG,         // Flush the log buffer
};

class Protocol
{
public:
  static constexpr size_t MAX_PAYLOAD_SIZE = 64;

private:
  enum class ParseState : byte
  {
    WAIT_HI,
    WAIT_LO,
    READ_LENGTH,
    READ_BODY,
  };

  static constexpr uint8_t MAGIC_HI = 0x52; // 'R'
  static constexpr uint8_t MAGIC_LO = 0x4F; // 'O'

  static ParseState parseState;

  static byte packetLength;
  static byte packetBuffer[MAX_PAYLOAD_SIZE];
  static byte packetIndex;

public:
  static void handleIncomingSerial();
  static void sendPacket(PacketType type, const byte *payload = nullptr, byte length = 0);

private:
  static void parseByte(byte b);
  static void handlePacket(const byte *data, byte length);

  static byte calculateChecksum(const byte *data, size_t length);
};
