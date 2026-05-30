#pragma once

#include "indicators/activityStatus.hpp"
#include "input/actions/buttonAction.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include <array>
#include <cstdint>
#include <functional>

namespace controlSystem
{
    enum class LedPolicy : uint8_t
    {
        None,       // no LED feedback (e.g. rotary events, power button)
        Momentary,  // LED on while held, off on release
        Toggle,     // LED flips state 
    };

    struct ButtonConfig
    {
        actions::IActionSource *action = nullptr;
        LedPolicy ledPolicy = LedPolicy::None;
    };

    // Drives button LED state; separate from activity status so it can be
    // used independently (e.g. in tests that only care about LEDs).
    struct IButtonLedSink
    {
        virtual ~IButtonLedSink() = default;
        virtual void setButtonLed(uint8_t pin, bool enabled) = 0;
    };

    // Combined interface for components that need both activity-status feedback
    // and button LED control.
    struct IControlBoardIndicators : IActivityStatusSink, IButtonLedSink
    {
    };

    class ControlBoardInputDispatcher
    {
    public:
        using ActionMap = std::array<ButtonConfig, controlBoardButtons::k_count>;
                using ResponseHandler = std::function<void(std::unique_ptr<actions::IAction>)>;

        ControlBoardInputDispatcher(ActionMap &rActionMap,
                                    ResponseHandler onResponse,
                                    IControlBoardIndicators &rIndicators)
            : mr_actionMap(rActionMap),
              m_onResponse(std::move(onResponse)),
              mr_indicators(rIndicators)
        {
        }

        void handleButtonPressed(uint8_t buttonPressedId);
        void handleButtonReleased(uint8_t buttonReleasedId);
        void handleRotaryMovement(int direction);
        void setBackgroundStatus(ControlBoardWorkingStatus status) { m_backgroundStatus = status; }

    private:
        bool isInputSuppressedInSleep(uint8_t buttonId) const;
        void applyLedOnPress(uint8_t buttonId, LedPolicy policy);
        void applyLedOnRelease(uint8_t buttonId, LedPolicy policy);

        ActionMap &mr_actionMap;
        ResponseHandler m_onResponse;
        IControlBoardIndicators &mr_indicators;
        ControlBoardWorkingStatus m_backgroundStatus = ControlBoardWorkingStatus::Idle;
        uint16_t m_buttonLedBitmask{0};
    };
}