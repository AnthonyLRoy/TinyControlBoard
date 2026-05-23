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

        bool handle(const actions::Action &action);

    private:
        bool handleCoverViewCommand(const actions::Action &action);
        bool handleMeterCommand(const actions::Action &action);
        bool handleRotaryCommand(const actions::Action &action);
        bool handleRepeatCommand(const actions::Action &action);
        bool handleRandomCommand(const actions::Action &action);
        bool handleSimpleCommand(const actions::Action &action);

        IUartCommandSink &mr_uartCommandSink;

        static constexpr const char *k_logTag = "Uart_Dispatcher ";
    };
}