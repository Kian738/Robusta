#pragma once

#include <Arduino.h>

class Gpio
{
private:
  static constexpr byte BUZZER_PIN = 5;
  static constexpr byte REGISTER_OPEN_PIN = 4;
  static constexpr byte REGISTER_STATUS_PIN = 2;

  enum class RegisterState : byte
  {
    CLOSED,
    OPENED_EXTERNALLY,
    OPENING_COMMANDED,
    OPENED_BY_COMMAND
  };

  static RegisterState currentState;
  static bool lastPhysicalState;

public:
  static void init();

  static void beep();
  static void playStartupChord();
  static void playErrorChord();

  static bool isRegisterOpen();
  static void checkRegisterState();

  static void openRegister();
};
