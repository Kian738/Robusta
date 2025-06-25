#include "logger.h"

#include "config.h"
#include "protocol.h"

byte Logger::logIndex = 0;                       // Current index in the log buffer
char Logger::logBuffer[Logger::LOG_BUFFER_SIZE]; // Buffer for log messages

void Logger::print(const char *message)
{
  if (!debugMode)
    return;

  while (*message)
  {
    if (logIndex >= sizeof(logBuffer) - 1)
      flush(); // Flush the log if it is full

    logBuffer[logIndex++] = *message++;
  }
}

void Logger::print(int number, byte base)
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

void Logger::newLine()
{
  if (!debugMode)
    return;

  if (logIndex >= sizeof(logBuffer) - 1)
    flush();

  logBuffer[logIndex++] = '\n';
  flush();
}

void Logger::printLn(const char *message)
{
  print(message);
  newLine();
}

void Logger::printLn(int number, byte base)
{
  print(number, base);
  newLine();
}

void Logger::flush()
{
  if (logIndex == 0)
    return; // Nothing to flush

  size_t sent = 0;
  while (sent < logIndex)
  {
    auto chunkSize = (logIndex - sent) > Protocol::MAX_PAYLOAD_SIZE
                         ? Protocol::MAX_PAYLOAD_SIZE
                         : (logIndex - sent);

    Protocol::sendPacket(PacketType::LOG,
                         reinterpret_cast<const byte *>(logBuffer + sent),
                         chunkSize);
    sent += chunkSize;
  }

  logIndex = 0; // Reset the log index after flushing
}
