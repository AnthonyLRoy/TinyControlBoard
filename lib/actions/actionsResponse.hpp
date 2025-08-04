
#pragma once
#include "uart_protocol.hpp"

namespace actions
{
 struct actionResponse
    {
        bool active;
        commandID  command;
        char parameters[10];   
    };
}