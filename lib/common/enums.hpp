    #pragma once
    
#include <stdint.h>
    
    enum ControlBoardState
    {
        Standby,
        Active,
        Error,
        Maintenance
    };

  

    #define CONTROL_BOARD_VERSION "1.0.0"
    #define CONTROL_BOARD_NAME "TinyControlBoard"
    #define CONTROL_BOARD_MANUFACTURER "Danny Roy Inc."
    #define CONTROL_BOARD_MODEL "TCB-2023"
    #define CONTROL_BOARD_SERIAL_NUMBER "TCB-0001"
    #define CONTROL_BOARD_FIRMWARE_VERSION "1.0.0"



 
#define UART_PROTOCOL_VERSION 0x01
#define UART_START_BYTE 0xAA

#define PROTO_INDEX_VERSION     1
#define PROTO_INDEX_SRC_APP     2
#define PROTO_INDEX_TYPE        3
#define PROTO_INDEX_SEQUENCE    4
#define PROTO_INDEX_COMMAND_ID  5
#define PROTO_INDEX_PARAMS      7
#define PROTO_INDEX_CHECKSUM    17

