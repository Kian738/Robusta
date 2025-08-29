#include "protocol.h"

#include "packet_handlers.h"
#include "connection_manager.h"
#include "logger.h"

Protocol::ParseState Protocol::parseState = Protocol::ParseState::WAIT_HI; // The packet section (byte) we are waiting for

byte Protocol::packetLength = 0;                               // Length of the current packet
byte Protocol::packetBuffer[Protocol::MAX_PAYLOAD_SIZE] = {0}; // Buffer for the packet body
byte Protocol::packetIndex = 0;                                // Current index in the packet body

void Protocol::handleIncomingSerial()
{
  while (Serial.available())
    parseByte(Serial.read());
}

void Protocol::sendPacket(PacketType type, const byte *payload, byte payloadLength)
{
  if (payloadLength > MAX_PAYLOAD_SIZE)
  {
    Logger::printLn("Error: Payload length exceeds maximum size.");
    return;
  }

  const byte length = payloadLength + 1; // 1 byte for type
  byte packet[MAX_PAYLOAD_SIZE + 5];     // 2 bytes for magic, 1 byte for length, 1 byte for type, 64 bytes for payload, and 1 byte for checksum

  byte i = 0;

  packet[i++] = MAGIC_HI;                // Magic high byte
  packet[i++] = MAGIC_LO;                // Magic low byte
  packet[i++] = length;                  // Length of the packet (excluding magic bytes, length, and checksum)
  packet[i++] = static_cast<byte>(type); // Packet type

  // Todo: The whole payload shall be encrypted. The call shall be generated via Deffie-Hellman key exchange <-- That would allow anybody to access the payload...
  if (payload && payloadLength > 0)
    memcpy(packet + i, payload, payloadLength); // Copy the payload into the packet
  i += payloadLength;

  packet[i++] = calculateChecksum(packet, payloadLength + 3); // Checksum, includes magic bytes (2), length  (1), [type, and payload]

  Serial.write(packet, i); // Send the packet over Serial
}

void Protocol::parseByte(byte b)
{
  switch (parseState)
  {
  case ParseState::WAIT_HI:
    if (b == MAGIC_HI)
      parseState = ParseState::WAIT_LO;
    break;

  case ParseState::WAIT_LO:
    parseState = b == MAGIC_LO ? ParseState::READ_LENGTH : ParseState::WAIT_HI;
    break;

  case ParseState::READ_LENGTH:
    if (b < 1 || b >= MAX_PAYLOAD_SIZE) // Min payload len is 1 byte for checksum
    {
      parseState = ParseState::WAIT_HI;
      return;
    }
    packetLength = b;
    packetIndex = 0;
    parseState = ParseState::READ_BODY;
    break;

  case ParseState::READ_BODY:
    packetBuffer[packetIndex++] = b;
    if (packetIndex > packetLength)
    {
      if (calculateChecksum(packetBuffer, packetLength) == packetBuffer[packetLength])
        handlePacket(packetBuffer, packetLength);
      parseState = ParseState::WAIT_HI; // Reset to wait for the next packet
    }
  default:
    break;
  }
}

void Protocol::handlePacket(const byte *data, byte length)
{
  const auto type = static_cast<PacketType>(data[0]);

  const auto payload = data + 1;
  const auto payloadLength = length - 1; // 1 byte for type

  if (!ConnectionManager::isConnected())
  {
    if (type == PacketType::CONNECT_REQUEST)
      handleConnectRequest(payload, payloadLength);

    return;
  }

  for (const auto &handler : handlers)
    if ((PacketType)pgm_read_byte(&handler.type) == type)
    {
      auto fn = (HandlerFn)pgm_read_word(&handler.fn);
      fn(payload, payloadLength);
      break;
    }
}

byte Protocol::calculateChecksum(const byte *data, size_t length)
{
  uint16_t sum = 0;
  for (byte i = 0; i < length; ++i)
    sum += data[i];

  return byte(sum & 0xFF);
}
