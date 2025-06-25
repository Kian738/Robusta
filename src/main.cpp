#include <Arduino.h>

#include <SPI.h>
#include <MFRC522.h>

constexpr unsigned long BAUD_RATE = 115200;
constexpr unsigned long ACTIVE_HEARTBEAT = 5000;     // 5 seconds
constexpr unsigned long IDLE_HEARTBEAT = 180000;     // 3 minutes
constexpr unsigned long INACTIVITY_TIMEOUT = 600000; // 10 minutes

// Define the SPI pins for the MFRC522
constexpr byte RST_PIN = 9;
constexpr byte SS_PIN = 10;
constexpr byte BUZZER_PIN = 5;
constexpr byte REGISTER_PIN = 6;

constexpr uint8_t MAGIC_HI = 0x4B; // 'K'
constexpr uint8_t MAGIC_LO = 0x4E; // 'N'
constexpr size_t MAX_PAYLOAD_SIZE = 64;

enum class ParseState : byte
{
  WAIT_HI,
  WAIT_LO,
  READ_LENGTH,
  READ_BODY,
};

// Todo: Packet to enable tag registration mode
enum class PacketType : byte
{
  VERIFY_UID = 0x01, // Verify the UID of a tag
  VERIFY_RESULT,     // Result of the verification
  HEARTBEAT,         // Heartbeat packet to check if the device is active
  SET_DEBUG,         // Set debug mode
  STATUS,            // Status packet
};

bool debugMode = false; // Debug mode flag

MFRC522 mfrc522(SS_PIN, RST_PIN);

auto parseState = ParseState::WAIT_HI; // The packet section (byte) we are waiting for
byte seq = 1;                          // Current sequence number for the packet
byte expectedSeq = 1;                  // Expected sequence number for the next packet

byte packetLength = 0;               // Length of the current packet
byte packetBuffer[MAX_PAYLOAD_SIZE]; // Buffer for the packet body
byte packetIndex = 0;                // Current index in the packet body

unsigned long lastHeartbeat = 0;
unsigned long lastActivity = 0;

// Todo: Create a Log method that uses our Serial protocol with reliability

void initMFRC522()
{
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("MFRC522 initialized.");
}

void playStartupChord()
{
  const int notes[] = {262, 330, 392, 494}; // C4, E4, G4, B4 (Cmaj7)
  const int noteDuration = 150;

  for (int i = 0; i < 4; i++)
  {
    tone(BUZZER_PIN, notes[i], noteDuration);
    delay(noteDuration);
  }

  // Small pause and a final high note G5 ping
  tone(BUZZER_PIN, 784, 100);
  delay(100);
  noTone(BUZZER_PIN);

  Serial.println("Startup sound played.");
}

void playErrorChord()
{
  const int notes[] = {440, 349, 440, 523}; // A4, F4, A4, C5
  const int noteDuration = 100;

  for (int i = 0; i < 4; i++)
  {
    tone(BUZZER_PIN, notes[i], noteDuration);
    delay(noteDuration);
  }

  Serial.println("Error sound played.");
}

void setup()
{
  Serial.begin(BAUD_RATE);

  while (!Serial)
    ;

  Serial.println("Starting MFRC522...");
  initMFRC522();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(REGISTER_PIN, OUTPUT);

  Serial.println("Playing startup chord...");
  playStartupChord();
}

byte calculateChecksum(const byte *data, size_t length)
{
  uint16_t sum = 0;
  for (byte i = 0; i < length; ++i)
    sum += data[i];
  return byte(sum & 0xFF);
}

void sendPacket(PacketType type, const byte *payload, byte payloadLength)
{
  if (payloadLength > MAX_PAYLOAD_SIZE)
  {
    Serial.println("Error: Payload length exceeds maximum size.");
    return;
  }

  byte length = payloadLength + 2;   // 1 byte for sequence, 1 byte for type
  byte packet[MAX_PAYLOAD_SIZE + 6]; // 2 bytes for magic, 1 byte for length, 1 byte for sequence, 1 byte for type, 64 bytes for payload, and 1 byte for checksum

  byte i = 0;

  packet[i++] = MAGIC_HI; // Magic high byte
  packet[i++] = MAGIC_LO; // Magic low byte
  packet[i++] = length;   // Length of the packet (excluding magic bytes, length, and checksum)

  packet[i++] = seq++; // Sequence number for the packet
  if (seq > 255)
    seq = 1;
  packet[i++] = static_cast<byte>(type); // Packet type

  // Todo: The whole payload shall be encrypted. The call shall be generated via Deffie-Hellman key exchange
  if (payload && payloadLength > 0)
    memcpy(packet + i, payload, payloadLength); // Copy the payload into the packet
  i += payloadLength;

  packet[i++] = calculateChecksum(packet, payloadLength + 3); // Checksum, includes magic bytes (2), length  (1), [sequence, type, and payload]

  Serial.write(packet, packetLength + i); // Send the packet over Serial
}

void handlePacket(const byte *data, byte length)
{
  auto receivedSeq = data[0];
  auto type = static_cast<PacketType>(data[1]);

  auto payload = data + 2;
  auto payloadLength = length - 2; // 1 byte for sequence, 1 byte for type

  if (receivedSeq != expectedSeq++)
  {
    Serial.print("Warning: Unexpected sequence number. Expected: ");
    Serial.print(expectedSeq - 1);
    return;
  }

  switch (type)
  {
  case PacketType::VERIFY_RESULT:
    if (payloadLength != 1)
      return;
    if (payload[0] == 0x01)
    {
      // Todo: Implement a packet struct where we can pass a function to handle the packet
      Serial.println("Tag verification successful.");

      digitalWrite(BUZZER_PIN, HIGH);
      delay(100);
      digitalWrite(BUZZER_PIN, LOW);

      // Register the tag
      digitalWrite(REGISTER_PIN, HIGH);
      delay(1000);
      digitalWrite(REGISTER_PIN, LOW);
    }
    else
    {
      Serial.println("Tag verification failed.");
      playErrorChord();
    }
    break;

  case PacketType::SET_DEBUG:
    if (payloadLength != 1)
      return;

    debugMode = (bool)payload[0];
    Serial.print("Debug mode set to: ");
    Serial.println(debugMode ? "ON" : "OFF"); // Todo: Just hardcode the goddamn ON
    break;

  default:
    break;
  }
}

void parseByte(byte b)
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
    if (b < 2 || b >= MAX_PAYLOAD_SIZE)
    {
      parseState = ParseState::WAIT_HI; // Invalid length, reset
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

void handleIncomingSerial()
{
  while (Serial.available())
    parseByte(Serial.read());
}

void SendHbIfNeeded()
{
  bool active = millis() - lastActivity < INACTIVITY_TIMEOUT;
  auto hbInterval = active ? ACTIVE_HEARTBEAT : IDLE_HEARTBEAT;

  if (millis() - lastHeartbeat > hbInterval)
  {
    sendPacket(PacketType::HEARTBEAT, nullptr, 0);
    lastHeartbeat = millis();
  }
}

bool checkForTag()
{
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return false;

  lastActivity = millis();
  return true;
}

void loop()
{
  handleIncomingSerial();
  SendHbIfNeeded();

  if (!checkForTag())
    return;

  Serial.print("Read UID tag: 0x");
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    if (mfrc522.uid.uidByte[i] < 0x10)
      Serial.print("0");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  sendPacket(PacketType::VERIFY_UID, mfrc522.uid.uidByte, mfrc522.uid.size);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
