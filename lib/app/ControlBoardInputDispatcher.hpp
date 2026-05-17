#pragma once

#include "input/actions/actionsResponse.hpp"
#include "activityStatus.hpp"
#include "input/actions/buttonAction.hpp"
#include "app/ControlBoardButtonIds.hpp"
#include <array>
#include <cstdint>

namespace controlSystem
{
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
        using ActionMap = std::array<actions::ButtonAction *, controlBoardButtons::kCount>;

        ControlBoardInputDispatcher(ActionMap &rActionMap,
                                    IActionResponseSink *pResponseSink,
                                    IControlBoardIndicators *pIndicators)
            : mrActionMap(rActionMap),
              mpResponseSink(pResponseSink),
              mpIndicators(pIndicators)
        {
        }

        void handleButtonPressed(uint8_t buttonPressedId);
        void handleButtonReleased(uint8_t buttonReleasedId);
        void handleRotaryMovement(int direction);
        void setBackgroundStatus(ControlBoardWorkingStatus status) { mBackgroundStatus = status; }

    private:
        ActionMap &mrActionMap;
        IActionResponseSink *mpResponseSink;
        IControlBoardIndicators *mpIndicators;
        ControlBoardWorkingStatus mBackgroundStatus = ControlBoardWorkingStatus::Idle;
    };
}