#pragma once

#include "hal/uart/UartCommandSink.hpp"
#include "input/actions/IAction.hpp"

namespace controlSystem
{
    class ActionUartDispatcher
    {
    public:
        explicit ActionUartDispatcher(transport::uart::IUartCommandSink &rUartCommandSink);

        bool handle(const actions::IAction &action);

    private:
        transport::uart::IUartCommandSink &mr_uartCommandSink;

        static constexpr const char *k_logTag = "Uart_Dispatcher ";
    };
}