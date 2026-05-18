#pragma once

#include "input/actions/actionsResponse.hpp"
#include "activityStatus.hpp"
#include "input/actions/buttonAction.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include "app/SystemState.hpp"
#include <array>
#include <cstdint>

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
        actions::ButtonAction *action = nullptr;
        LedPolicy ledPolicy = LedPolicy::None;
    };

    struct IActionResponseSink
    {
        virtual ~IActionResponseSink() = default;
        virtual void process(const actions::ActionResponse &response) = 0;
    };

    struct IControlBoardIndicators : IActivityStatusSink
    {
        virtual void setButtonLed(uint8_t pin, bool enabled) = 0;
    };

    class ControlBoardInputDispatcher
    {
    public:
        using ActionMap = std::array<ButtonConfig, controlBoardButtons::kCount>;

        ControlBoardInputDispatcher(ActionMap &rActionMap,
                                    IActionResponseSink *pResponseSink,
                                    IControlBoardIndicators *pIndicators,
                                    SystemState &rSystemState)
            : mrActionMap(rActionMap),
              mpResponseSink(pResponseSink),
              mpIndicators(pIndicators),
              mrSystemState(rSystemState)
        {
        }

        void handleButtonPressed(uint8_t buttonPressedId);
        void handleButtonReleased(uint8_t buttonReleasedId);
        void handleRotaryMovement(int direction);
        void setBackgroundStatus(ControlBoardWorkingStatus status) { mBackgroundStatus = status; }

    private:
        void applyLedOnPress(uint8_t buttonId, LedPolicy policy);
        void applyLedOnRelease(uint8_t buttonId, LedPolicy policy);

        ActionMap &mrActionMap;
        IActionResponseSink *mpResponseSink;
        IControlBoardIndicators *mpIndicators;
        ControlBoardWorkingStatus mBackgroundStatus = ControlBoardWorkingStatus::Idle;
        SystemState &mrSystemState;
    };
}