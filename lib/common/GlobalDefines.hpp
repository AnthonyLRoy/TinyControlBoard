#pragma once

#include "../board/boardIdentity.hpp"
#include "../protocol/uartProtocol.hpp"

#define CONTROL_BOARD_VERSION board::identity::kVersion
#define CONTROL_BOARD_NAME board::identity::kName
#define CONTROL_BOARD_MANUFACTURER board::identity::kManufacturer
#define CONTROL_BOARD_MODEL board::identity::kModel
#define CONTROL_BOARD_SERIAL_NUMBER board::identity::kSerialNumber
#define CONTROL_BOARD_FIRMWARE_VERSION board::identity::kFirmwareVersion