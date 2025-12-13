// uart_protocol.h
#pragma once
#include <stdint.h>
#include "string.h"

#define UART_PROTOCOL_VERSION 0x01
#define UART_START_BYTE 0xAA

// Message structure: total 20 bytes
// Indices for field parsing
#define PROTO_INDEX_VERSION 1
#define PROTO_INDEX_SRC_APP 2
#define PROTO_INDEX_TYPE 3
#define PROTO_INDEX_SEQUENCE 4
#define PROTO_INDEX_COMMAND_ID 5
#define PROTO_INDEX_PARAMS 7
#define PROTO_INDEX_CHECKSUM 17

// Enumerated types for messageType
enum MessageType : uint8_t
{
    MSG_COMMAND = 0x01,
    MSG_STATUS = 0x02,
    MSG_ACK = 0x03,
    MSG_NACK = 0x04
};

#define UART_PACKET_SIZE 18

enum  commandID : uint16_t
{

    // system
    CMD_NO_ACTION = 0x000,
    CMD_SYS_POWER = 0x0001,
    CMD_SYS_RPI_SHUTDOWN = 0x0002,
    CMD_SYS_HEARTBEAT = 0x0003,
    CMD_SYS_NOHEARTBEAT = 0x0004,
    // track control commands
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
 


};

enum PowerCommand : uint8_t
{
    POWER_ACTIVE = 0x01,
    POWER_SLEEP = 0x02,
    POSER_DEEP_SLEEP = 0x03
};

// Source application IDs
enum AppID : uint8_t
{
    APP_ESP32 = 0x01,
    APP_PI = 0x02
};

struct UARTMessage
{
    uint8_t start_byte;
    uint8_t version;
    uint8_t src_app;
    uint8_t msg_type;
    uint8_t sequence;
    uint16_t command_id;
    uint16_t params[5];
    uint8_t checksum;

    UARTMessage()
        : start_byte(UART_START_BYTE), version(UART_PROTOCOL_VERSION), src_app(APP_ESP32),
          msg_type(MSG_COMMAND), sequence(0), command_id(0), checksum(0)
    {
        memset(params, 0, sizeof(params));
    }
};

uint8_t calculate_checksum(const uint8_t *data);
void serialize_message(UARTMessage &msg, uint8_t *buffer);
bool deserialize_message(const uint8_t *buffer, UARTMessage &msg);