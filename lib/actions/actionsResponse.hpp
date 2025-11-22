
#pragma once
#include "uart_protocol.hpp"

namespace actions
{
 struct actionResponse
    {
       
        bool active = false;
        commandID  command = CMD_NO_ACTION;
        char parameters[10]{};  
        uint16_t releaseTimeMilliSecs = 0;
        bool KeepLedActive= false;

        actionResponse() = default;
    };
}