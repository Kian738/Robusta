#pragma once

#include <Arduino.h>

class Gpio
{
private:
  static constexpr byte BUZZER_PIN = 5;
  static constexpr byte REGISTER_PIN = 4;

public:
  static void init();

  static void beep();
  static void playStartupChord();
  static void playErrorChord();
};
