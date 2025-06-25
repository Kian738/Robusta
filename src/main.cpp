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

constexpr uint8_t MAGIC_HI = 0x52; // 'R'
constexpr uint8_t MAGIC_LO = 0x4F; // 'O'
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
  LOG,               // Log packet
  FLUSH_LOG,         // Flush the log buffer
};

using HandlerFn = void (*)(const byte *payload, byte length);

struct PacketHandler
{
  PacketType type;
  HandlerFn fn;
};

void handleVerifyResult(const byte *payload, byte length);
void handleSetDebug(const byte *payload, byte length);
void handleFlushLog(const byte *payload, byte length);

constexpr PacketHandler handlers[] PROGMEM = {
    {PacketType::VERIFY_RESULT, handleVerifyResult},
    {PacketType::SET_DEBUG, handleSetDebug},
    {PacketType::FLUSH_LOG, handleFlushLog}};

bool debugMode = true; // Debug mode flag

byte logIndex = 0;                    // Current index in the log buffer
char logBuffer[2 * MAX_PAYLOAD_SIZE]; // Buffer for log messages

MFRC522 mfrc522(SS_PIN, RST_PIN);

auto parseState = ParseState::WAIT_HI; // The packet section (byte) we are waiting for
byte seq = 1;                          // Current sequence number for the packet
byte expectedSeq = 1;                  // Expected sequence number for the next packet
// Todo: Implement the reliable packet delivery with sequence numbers

byte packetLength = 0;               // Length of the current packet
byte packetBuffer[MAX_PAYLOAD_SIZE]; // Buffer for the packet body
byte packetIndex = 0;                // Current index in the packet body

unsigned long lastHeartbeat = 0;
unsigned long lastActivity = 0;

void sendPacket(PacketType type, const byte *payload = nullptr, byte payloadLength = 0);

void flushLog()
{
  if (logIndex == 0)
    return; // Nothing to flush

  size_t sent = 0;
  while (sent < logIndex)
  {
    auto chunkSize = (logIndex - sent) > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : (logIndex - sent);
    sendPacket(PacketType::LOG,
               reinterpret_cast<const byte *>(logBuffer + sent),
               chunkSize);

    sent += chunkSize;
  }

  logIndex = 0; // Reset the log index after flushing
}

// Todo: Make this take VA_ARGS
void print(const char *message)
{
  if (!debugMode)
    return;

  while (*message)
  {
    if (logIndex >= sizeof(logBuffer) - 1)
      flushLog(); // Flush the log if it is full

    logBuffer[logIndex++] = *message++;
  }
}

void print(int number, byte base = DEC)
{
  if (!debugMode)
    return;

  char buffer[32] = {0};

  if (base == 10)
    ltoa(number, buffer, 10);
  else if (base == 16)
  {
    ltoa(number, buffer, 16);
    for (char *p = buffer; *p; p++)
      *p = toupper(*p);
  }
  else
    snprintf(buffer, sizeof(buffer), "Invalid base: %u", base);

  print(buffer);
}

void newLine()
{
  if (!debugMode)
    return;

  if (logIndex >= sizeof(logBuffer) - 1)
    flushLog();

  logBuffer[logIndex++] = '\n';
  flushLog();
}

void printLn(const char *message)
{
  print(message);
  newLine();
}

void printLn(int number, byte base = 10)
{
  print(number, base);
  newLine();
}

void initMFRC522()
{
  SPI.begin();
  mfrc522.PCD_Init();
  print("MFRC522 initialized.");
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

  print("Startup sound played.");
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

  print("Error sound played.");
}

void setup()
{
  Serial.begin(BAUD_RATE);

  while (!Serial)
    ;

  print("Starting MFRC522...");
  initMFRC522();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(REGISTER_PIN, OUTPUT);

  print("Playing startup chord...");
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
    print("Error: Payload length exceeds maximum size.");
    return;
  }

  byte length = payloadLength + 2;   // 1 byte for sequence, 1 byte for type
  byte packet[MAX_PAYLOAD_SIZE + 6]; // 2 bytes for magic, 1 byte for length, 1 byte for sequence, 1 byte for type, 64 bytes for payload, and 1 byte for checksum

  byte i = 0;

  packet[i++] = MAGIC_HI; // Magic high byte
  packet[i++] = MAGIC_LO; // Magic low byte
  packet[i++] = length;   // Length of the packet (excluding magic bytes, length, and checksum)

  packet[i++] = seq++; // Sequence number for the packet
  if (seq == 0)
    seq = 1;                             // Reset sequence number to 1 if it overflows
  packet[i++] = static_cast<byte>(type); // Packet type

  // Todo: The whole payload shall be encrypted. The call shall be generated via Deffie-Hellman key exchange
  if (payload && payloadLength > 0)
    memcpy(packet + i, payload, payloadLength); // Copy the payload into the packet
  i += payloadLength;

  packet[i++] = calculateChecksum(packet, payloadLength + 3); // Checksum, includes magic bytes (2), length  (1), [sequence, type, and payload]

  Serial.write(packet, i); // Send the packet over Serial
}

void handlePacket(const byte *data, byte length)
{
  auto receivedSeq = data[0];
  auto type = static_cast<PacketType>(data[1]);

  auto payload = data + 2;
  auto payloadLength = length - 2; // 1 byte for sequence, 1 byte for type

  if (receivedSeq != expectedSeq++)
  {
    print("Warning: Unexpected sequence number. Expected: ");
    printLn(expectedSeq - 1);
    return;
  }

  for (auto &handler : handlers)
    if ((PacketType)pgm_read_byte(&handler.type) == type)
    {
      auto fn = (HandlerFn)pgm_read_word(&handler.fn);
      fn(payload, payloadLength);
      break;
    }
}

void handleVerifyResult(const byte *payload, byte length)
{
  if (length != 1)
    return; // Invalid payload length

  if (payload[0] != 0x01)
  {
    print("Tag verification failed.");
    playErrorChord();
    return;
  }

  print("Tag verification successful.");

  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  // Register the tag
  digitalWrite(REGISTER_PIN, HIGH);
  delay(1000);
  digitalWrite(REGISTER_PIN, LOW);
};

void handleSetDebug(const byte *payload, byte length)
{
  if (length != 1)
    return;

  debugMode = (bool)payload[0];
  print("Enabled debug mode."); // This will only print if debugMode is true
};

void handleFlushLog(const byte *payload, byte length)
{
  if (length != 0)
    return;

  flushLog();
  print("Log flushed.");
};

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
    sendPacket(PacketType::HEARTBEAT);
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

  print("Read UID tag: 0x");
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    if (mfrc522.uid.uidByte[i] < 0x10)
      print("0");
    print(mfrc522.uid.uidByte[i], HEX);
  }
  newLine();

  sendPacket(PacketType::VERIFY_UID, mfrc522.uid.uidByte, mfrc522.uid.size);

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
