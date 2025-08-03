
#pragma once
#include "uart_protocol.hpp"
#include "ControlBoard.hpp"

namespace actions
{
 struct actionResponse
    {
        bool messageCreated;
        UARTMessage message;
        ControlBoardState  ledStatus;
    };
}