
#pragma once
#include "uart_protocol.hpp"

namespace actions
{
 struct actionResponse
    {
       
        bool active = false;
        commandID  command = CMD_NO_ACTION;
        uint16_t parameters[5]{0,0,0,0,0};  
        uint16_t releaseTimeMilliSecs = 0;
        bool KeepLedActive= false;

        actionResponse() = default;
    };
}