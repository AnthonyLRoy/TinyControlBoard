// uart_protocol.h
#pragma once
#include <stdint.h>
#include "string.h"

#define UART_PROTOCOL_VERSION 0x01
#define UART_START_BYTE 0xAA

// Message structure: total 20 bytes
// Indices for field parsing
#define PROTO_INDEX_VERSION     1
#define PROTO_INDEX_SRC_APP     2
#define PROTO_INDEX_TYPE        3
#define PROTO_INDEX_SEQUENCE    4
#define PROTO_INDEX_COMMAND_ID  5
#define PROTO_INDEX_PARAMS      7
#define PROTO_INDEX_CHECKSUM    17

// Enumerated types for messageType
enum MessageType : uint8_t {
    MSG_COMMAND = 0x01,
    MSG_STATUS  = 0x02,
    MSG_ACK     = 0x03,
    MSG_NACK    = 0x04
};

#define UART_PACKET_SIZE 20

enum commandID : uint16_t {
     //track control commands
    CMD_NEXT_TRACK = 0x0100,
    CMD_PREVIOUS_TRACK    = 0x0101,
    CMD_PLAY_PAUSE = 0x0102,
    CMD_STOP       = 0x0103,

    // PI Control commands
    CMD_GET_PI_STATUS  = 0x0201,
    CMD_SET__PI_STATUS  = 0x0202,
};

// Source application IDs
enum AppID : uint8_t {
    APP_ESP32 = 0x01,
    APP_PI    = 0x02
};

struct UARTMessage {
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

uint8_t calculate_checksum(const uint8_t *data, size_t len);
void serialize_message( UARTMessage &msg, uint8_t *buffer);
bool deserialize_message(const uint8_t *buffer, UARTMessage &msg);