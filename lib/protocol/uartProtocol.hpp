#pragma once

#include <stdint.h>
#include "string.h"

namespace protocol
{
inline constexpr uint8_t k_version = 0x01;
inline constexpr uint8_t k_startByte = 0xAA;
inline constexpr uint8_t k_indexVersion = 1;
inline constexpr uint8_t k_indexSrcApp = 2;
inline constexpr uint8_t k_indexType = 3;
inline constexpr uint8_t k_indexSequence = 4;
inline constexpr uint8_t k_indexCommandId = 5;
inline constexpr uint8_t k_indexPayloadLen = 7;  // byte that carries N
inline constexpr uint8_t k_headerSize = 8;         // bytes 0-7
inline constexpr uint8_t k_legacyPayloadSize = 10; // 5×uint16 params
inline constexpr uint8_t k_maxNowPlayingLen = 60;
inline constexpr uint8_t k_maxPayloadSize = 60;
// header(8) + max-payload(60) + checksum(1)
inline constexpr uint8_t k_maxPacketSize = k_headerSize + k_maxPayloadSize + 1;
// header(8) + legacy-params(10) + checksum(1)
inline constexpr uint8_t k_commandPacketSize = k_headerSize + k_legacyPayloadSize + 1;
} // namespace protocol

#define UART_PROTOCOL_VERSION protocol::k_version
#define UART_START_BYTE protocol::k_startByte

#define PROTO_INDEX_VERSION protocol::k_indexVersion
#define PROTO_INDEX_SRC_APP protocol::k_indexSrcApp
#define PROTO_INDEX_TYPE protocol::k_indexType
#define PROTO_INDEX_SEQUENCE protocol::k_indexSequence
#define PROTO_INDEX_COMMAND_ID protocol::k_indexCommandId
#define PROTO_INDEX_PARAMS protocol::k_indexParams
#define PROTO_INDEX_CHECKSUM protocol::k_indexChecksum

enum MessageType : uint8_t
{
    MSG_COMMAND        = 0x01,
    MSG_STATUS         = 0x02,
    MSG_ACK            = 0x03,
    MSG_NACK           = 0x04,
    MSG_NOW_PLAYING    = 0x05,
    // payload: elapsed_s(u16 LE) + duration_s(u16 LE) + is_playing(u8)
    MSG_TRACK_PROGRESS = 0x06,
};

#define UART_PACKET_SIZE protocol::k_maxPacketSize

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
    CMD_TOGGLE_RANDOM = 0x011F,
    CMD_SET_BRIGHTNESS_UP = 0x0120,
    CMD_SET_BRIGHTNESS_DOWN = 0x0121,
    CMD_SELECT_PANEL_PLAYBACK = 0x0122,
    CMD_SELECT_PANEL_RADIO    = 0x0123,
    CMD_SELECT_PANEL_PLAYLIST = 0x0124,
    CMD_SELECT_PANEL_FOLDER   = 0x0125,
    CMD_SELECT_PANEL_TAG      = 0x0126,
    CMD_SELECT_PANEL_ALBUM    = 0x0127
};

enum PowerCommand : uint8_t
{
    POWER_ACTIVE = 0x01,
    POWER_SLEEP = 0x02,
    POWER_DEEP_SLEEP = 0x03
};

enum AppId : uint8_t
{
    APP_ESP32 = 0x01,
    APP_PI = 0x02
};

struct UartMessage
{
    uint8_t  startByte;
    uint8_t  version;
    uint8_t  srcApp;
    uint8_t  msgType;
    uint8_t  sequence;
    uint16_t commandId;
    // Outgoing commands: set params[], leave payloadLen = 0 (serialize fills it).
    // Incoming MSG_NOW_PLAYING: read nowPlayingText / nowPlayingLen.
    uint16_t params[5];
    uint8_t  nowPlayingText[protocol::k_maxNowPlayingLen + 1];
    uint8_t  nowPlayingLen;
    // Incoming MSG_TRACK_PROGRESS: elapsed/duration in seconds and playback state.
    uint16_t trackElapsedSec;
    uint16_t trackDurationSec;
    bool     trackIsPlaying;
    uint8_t  checksum;

    UartMessage()
        : startByte(UART_START_BYTE), version(UART_PROTOCOL_VERSION), srcApp(APP_ESP32),
          msgType(MSG_COMMAND), sequence(0), commandId(0), nowPlayingLen(0),
          trackElapsedSec(0), trackDurationSec(0), trackIsPlaying(false), checksum(0)
    {
        memset(params, 0, sizeof(params));
        memset(nowPlayingText, 0, sizeof(nowPlayingText));
    }
};

uint8_t calculateChecksum(const uint8_t *p_data);
// Returns the total packet size written into p_buffer.
uint8_t serializeMessage(UartMessage &rMsg, uint8_t *p_buffer);
bool deserializeMessage(const uint8_t *p_buffer, UartMessage &rMsg);
