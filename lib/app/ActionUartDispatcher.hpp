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
        // Creates the dispatcher bound to the UART sink used to send outbound commands.
        explicit ActionUartDispatcher(IUartCommandSink &rUartCommandSink);

        // Sends the requested action to the UART sink and keeps the toggle state in sync.
        bool handle(const actions::IAction &action);
        // Clears all remembered toggle states so the next command sequence starts fresh.
        void resetToggleStates();

    private:
        IUartCommandSink &mr_uartCommandSink;
        bool m_toggleStates[4]{};

        static constexpr const char *k_logTag = "Uart_Dispatcher ";
    };
}