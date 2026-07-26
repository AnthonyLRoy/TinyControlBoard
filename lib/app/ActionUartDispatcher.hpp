#pragma once

#include "input/actions/IAction.hpp"

namespace controlSystem
{
    struct IUartCommandSink
    {
        virtual ~IUartCommandSink() = default;
        virtual void sendUartCommand(const char *p_logTag, uint32_t commandId) = 0;
        virtual void sendUartMessage(const char *p_logTag, UartMessage &rMessage) = 0;
    };

    class ActionUartDispatcher
    {
    public:
        explicit ActionUartDispatcher(IUartCommandSink &rUartCommandSink);

        bool handle(const actions::IAction &action);

    private:
        IUartCommandSink &mr_uartCommandSink;
        bool m_toggleStates[4]{};

        static constexpr const char *k_logTag = "Uart_Dispatcher ";
    };
}