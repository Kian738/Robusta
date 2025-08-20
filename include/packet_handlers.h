#pragma once

#include "protocol.h"

#include <Arduino.h>

using HandlerFn = void (*)(const byte *payload, byte length);

struct PacketHandler
{
  PacketType type;
  HandlerFn fn;
};

void handleConnectRequest(const byte *payload, byte length); // Special packet, not in handlers array
void handleVerifyResponse(const byte *payload, byte length);
void handleSetDebug(const byte *payload, byte length);
void handleOpenRegister(const byte *payload, byte length);
void handleHeartbeatAck(const byte *payload, byte length);

constexpr PacketHandler handlers[] PROGMEM = {
    {PacketType::VERIFY_RESPONSE, handleVerifyResponse},
    {PacketType::SET_DEBUG, handleSetDebug},
    {PacketType::OPEN_REGISTER, handleOpenRegister},
    {PacketType::HEARTBEAT_ACK, handleHeartbeatAck}};
