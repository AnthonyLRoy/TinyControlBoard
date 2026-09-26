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

        // Creates the dispatcher bound to a button map, response callback, and indicator sink.
        ControlBoardInputDispatcher(ActionMap &rActionMap,
                                    ResponseHandler onResponse,
                                    IControlBoardIndicators &rIndicators)
            : mr_actionMap(rActionMap),
              m_onResponse(std::move(onResponse)),
              mr_indicators(rIndicators)
        {
        }

        // Handles a pressed button event and dispatches the mapped action if allowed.
        void handleButtonPressed(uint8_t buttonPressedId);
        // Handles a released button event and clears any press-only behavior.
        void handleButtonReleased(uint8_t buttonReleasedId);
        // Handles a rotary encoder movement and dispatches the corresponding action.
        void handleRotaryMovement(int direction);
        // Toggles the LED state for the specified button in response to user input.
        void toggleButtonLed(uint8_t buttonId);
        // Updates the background status to reflect the current board operating mode.
        void setBackgroundStatus(ControlBoardWorkingStatus status);
        // Clears the tracked toggle-LED state so all button LEDs return to a neutral state.
        void resetToggleLeds();

    private:
        // Returns true when a button press should be ignored while the board is asleep.
        bool isInputSuppressedInSleep(uint8_t buttonId) const;
        // Applies the press-time LED behavior defined by the button's LED policy.
        void applyLedOnPress(uint8_t buttonId, LedPolicy policy);
        // Applies the release-time LED behavior defined by the button's LED policy.
        void applyLedOnRelease(uint8_t buttonId, LedPolicy policy);

        ActionMap &mr_actionMap;
        ResponseHandler m_onResponse;
        IControlBoardIndicators &mr_indicators;
        ControlBoardWorkingStatus m_backgroundStatus = ControlBoardWorkingStatus::Idle;
        uint16_t m_buttonLedBitmask{0};
    };
}