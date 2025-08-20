#pragma once

#include "protocol.h"

#include <Arduino.h>

using HandlerFn = void (*)(const byte *payload, byte length);

struct PacketHandler
{
  PacketType type;
  HandlerFn fn;
};

void handleVerifyResult(const byte *payload, byte length);
void handleSetDebug(const byte *payload, byte length);
void handleOpenRegister(const byte *payload, byte length);

constexpr PacketHandler handlers[] PROGMEM = {
    {PacketType::START, handleStart},
    {PacketType::VERIFY_RESULT, handleVerifyResult},
    {PacketType::SET_DEBUG, handleSetDebug},
    {PacketType::OPEN_REGISTER, handleOpenRegister}};
