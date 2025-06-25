#include "gpio.h"

#include "logger.h"

void Gpio::init()
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(REGISTER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);   // Ensure buzzer is off
  digitalWrite(REGISTER_PIN, LOW); // Ensure register pin is low

  Logger::print("GPIO initialized.");
}

void Gpio::beep()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  Logger::print("Buzzer beeped.");
}

void Gpio::playStartupChord()
{
  tone(BUZZER_PIN, 523, 100); // C5
  delay(175);

  const int notes[] = {440, 349, 659}; // A4, F4, E5
  const int noteDuration = 120;

  for (byte i = 0; i < 3; i++)
  {
    tone(BUZZER_PIN, notes[i], noteDuration);
    delay(noteDuration);
  }

  Logger::print("Startup sound played.");
}

void Gpio::playErrorChord()
{
  const int notes[] = {440, 349, 440, 523}; // A4, F4, A4, C5
  const int noteDuration = 100;

  for (byte i = 0; i < 4; i++)
  {
    tone(BUZZER_PIN, notes[i], noteDuration);
    delay(noteDuration);
  }

  Logger::print("Error sound played.");
}

void Gpio::openRegister()
{
  digitalWrite(REGISTER_PIN, HIGH);
  delay(100);
  digitalWrite(REGISTER_PIN, LOW);

  Logger::print("Register opened.");
}
