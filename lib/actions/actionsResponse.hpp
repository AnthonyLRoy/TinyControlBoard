
#pragma once
#include "uart_protocol.hpp"

namespace actions
{
 struct actionResponse
    {
        bool messageCreated;
        UARTMessage message;
    };

}