#pragma once

#include "input/actions/actionsResponse.hpp"

namespace controlSystem
{
    struct IUartCommandSink
    {
        virtual ~IUartCommandSink() = default;
        virtual void sendUartCommand(const char *pLogTag, uint32_t commandId) = 0;
        virtual void sendUartMessage(const char *pLogTag, UartMessage &rMessage) = 0;
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

        IUartCommandSink &mrUartCommandSink;

        static constexpr const char *mspTag = "Uart_Dispatcher ";
    };
}