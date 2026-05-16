#pragma once

#include <stdint.h>
#include "string.h"

namespace protocol
{
inline constexpr uint8_t kVersion = 0x01;
inline constexpr uint8_t kStartByte = 0xAA;
inline constexpr uint8_t kIndexVersion = 1;
inline constexpr uint8_t kIndexSrcApp = 2;
inline constexpr uint8_t kIndexType = 3;
inline constexpr uint8_t kIndexSequence = 4;
inline constexpr uint8_t kIndexCommandId = 5;
inline constexpr uint8_t kIndexParams = 7;
inline constexpr uint8_t kIndexChecksum = 17;
inline constexpr uint8_t kPacketSize = 18;
} // namespace protocol

#define UART_PROTOCOL_VERSION protocol::kVersion
#define UART_START_BYTE protocol::kStartByte

#define PROTO_INDEX_VERSION protocol::kIndexVersion
#define PROTO_INDEX_SRC_APP protocol::kIndexSrcApp
#define PROTO_INDEX_TYPE protocol::kIndexType
#define PROTO_INDEX_SEQUENCE protocol::kIndexSequence
#define PROTO_INDEX_COMMAND_ID protocol::kIndexCommandId
#define PROTO_INDEX_PARAMS protocol::kIndexParams
#define PROTO_INDEX_CHECKSUM protocol::kIndexChecksum

enum MessageType : uint8_t
{
    MSG_COMMAND = 0x01,
    MSG_STATUS = 0x02,
    MSG_ACK = 0x03,
    MSG_NACK = 0x04
};

#define UART_PACKET_SIZE protocol::kPacketSize

enum CommandId : uint16_t
{
    CMD_NO_ACTION = 0x000,
    CMD_SYS_POWER = 0x0001,
    CMD_SYS_RPI_SHUTDOWN = 0x0002,
    CMD_SYS_HEARTBEAT = 0x0003,
    CMD_SYS_NOHEARTBEAT = 0x0004,
    CMD_NEXT_TRACK = 0x0100,
    CMD_PREVIOUS_TRACK = 0x0101,
    CMD_PLAY_PAUSE = 0x0102,
    CMD_STOP_TRACK = 0x0103,
    CMD_SKIP_FORWARD = 0x0104,
    CMD_SKIP_BACK = 0x0105,
    CMD_PREV_MENU_ITEM = 0x0106,
    CMD_NEXT_MENU_ITEM = 0x0107,
    CMD_ITEM_SELECT = 0x0108,
    CMD_EXIT_ITEM = 0x0109,
    CMD_DISPLAY_OFF = 0x010B,
    CMD_TOGGLE_METER_ON = 0x010C,
    CMD_TOGGLE_METER_OFF = 0x010D,
    CMD_DISPLAY_ON = 0x010E,
    CMD_TOGGLE_DAC_ON = 0x010A,
    CMD_TOGGLE_DAC_OFF = 0x010F,
    CMD_ROTARY_LEFT = 0x0110,
    CMD_ROTARY_RIGHT = 0x0111,
    CMD_ROTARY_ACTION = 0x0112,
    CMD_TOGGLE_DAC = 0x0113,
    CMD_TOGGLE_DISPLAY = 0x0114,
    CMD_TOGGLE_METER = 0x0115,
    CMD_CYCLE_BRIGHTNESS = 0x0116,
    CMD_COVER_VIEW_ON = 0x0117,
    CMD_COVER_VIEW_OFF = 0x0118,
    CMD_TOGGLE_COVER_VIEW = 0x0119,
    CMD_REPEAT_ON = 0x011A,
    CMD_REPEAT_OFF = 0x011B,
    CMD_TOGGLE_REPEAT = 0x011C,
    CMD_RANDOM_ON = 0x011D,
    CMD_RANDOM_OFF = 0x011E,
    CMD_TOGGLE_RANDOM = 0x011F
};

enum PowerCommand : uint8_t
{
    POWER_ACTIVE = 0x01,
    POWER_SLEEP = 0x02,
    POSER_DEEP_SLEEP = 0x03
};

enum AppId : uint8_t
{
    APP_ESP32 = 0x01,
    APP_PI = 0x02
};

struct UartMessage
{
    uint8_t startByte;
    uint8_t version;
    uint8_t srcApp;
    uint8_t msgType;
    uint8_t sequence;
    uint16_t commandId;
    uint16_t params[5];
    uint8_t checksum;

    UartMessage()
        : startByte(UART_START_BYTE), version(UART_PROTOCOL_VERSION), srcApp(APP_ESP32),
          msgType(MSG_COMMAND), sequence(0), commandId(0), checksum(0)
    {
        memset(params, 0, sizeof(params));
    }
};

uint8_t calculateChecksum(const uint8_t *pData);
void serializeMessage(UartMessage &rMsg, uint8_t *pBuffer);
bool deserializeMessage(const uint8_t *pBuffer, UartMessage &rMsg);
