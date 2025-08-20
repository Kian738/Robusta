#include "gpio.h"

#include "logger.h"
#include "protocol.h"

Gpio::RegisterState Gpio::currentState = Gpio::RegisterState::CLOSED;
bool Gpio::lastPhysicalState = false;

void Gpio::init()
{
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(REGISTER_OPEN_PIN, OUTPUT);
  pinMode(REGISTER_STATUS_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);        // Ensure buzzer is off
  digitalWrite(REGISTER_OPEN_PIN, LOW); // Ensure register pin is low

  lastPhysicalState = isRegisterOpen();
  currentState = lastPhysicalState ? RegisterState::OPENED_EXTERNALLY : RegisterState::CLOSED;

  Logger::printLn("GPIO initialized.");
}

void Gpio::beep()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);

  Logger::printLn("Buzzer beeped.");
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

  Logger::printLn("Startup sound played.");
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

  Logger::printLn("Error sound played.");
}

bool Gpio::isRegisterOpen()
{
  return digitalRead(REGISTER_STATUS_PIN) == LOW;
}

void Gpio::checkRegisterState()
{
  auto physicalState = isRegisterOpen();

  if (physicalState != lastPhysicalState)
  {
    Logger::print("Physical register state changed: ");
    Logger::printLn(physicalState ? "OPEN" : "CLOSED");

    currentState = physicalState
                       ? currentState == RegisterState::OPENING_COMMANDED
                             ? RegisterState::OPENED_BY_COMMAND
                             : RegisterState::OPENED_EXTERNALLY
                       : RegisterState::CLOSED;

    if (currentState != RegisterState::OPENED_BY_COMMAND) // We will never have the state OPENING_COMMANDED here
    {
      auto payload = static_cast<byte>(currentState);
      Protocol::sendPacket(PacketType::REGISTER_STATE, &payload, 1);
    }

    lastPhysicalState = physicalState;
  }
  else if (currentState == RegisterState::OPENING_COMMANDED && millis() % 1000 == 0)
  {
    Logger::printLn("Warning: Register didn't open after command");
    currentState = RegisterState::CLOSED;
  }
}

void Gpio::openRegister()
{
  currentState = RegisterState::OPENING_COMMANDED;

  digitalWrite(REGISTER_OPEN_PIN, HIGH);
  delay(100);
  digitalWrite(REGISTER_OPEN_PIN, LOW);

  Logger::printLn("Register opened.");
}
