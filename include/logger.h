#pragma once

#include <Arduino.h>

// Todo: Log level and VA_ARG support
class Logger
{
private:
  static constexpr size_t LOG_BUFFER_SIZE = 128;

  static inline byte logIndex = 0;                     // Current index in the log buffer
  static inline char logBuffer[LOG_BUFFER_SIZE] = {0}; // Buffer for log messages

public:
  static void print(const char *message);
  static void print(int number, byte base = DEC);

  static void newLine();
  static void printLn(const char *message);
  static void printLn(int number, byte base = DEC);

  static void flush();
};
