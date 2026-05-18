#pragma once

#include "input/actions/actionsResponse.hpp"

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

        bool handle(const actions::ActionResponse &response);

    private:
        bool handleCoverViewCommand(const actions::ActionResponse &response);
        bool handleMeterCommand(const actions::ActionResponse &response);
        bool handleRotaryCommand(const actions::ActionResponse &response);
        bool handleRepeatCommand(const actions::ActionResponse &response);
        bool handleRandomCommand(const actions::ActionResponse &response);
        bool handleSimpleCommand(const actions::ActionResponse &response);

        IUartCommandSink &mr_uartCommandSink;

        static constexpr const char *k_logTag = "Uart_Dispatcher ";
    };
}