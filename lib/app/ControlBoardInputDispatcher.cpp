#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    void ControlBoardInputDispatcher::applyLedOnPress(uint8_t buttonId, LedPolicy policy)
    {
        switch (policy)
        {
        case LedPolicy::Momentary:
            mrIndicators.setButtonLed(buttonId, true);
            break;
        case LedPolicy::Toggle:
            mrSystemState.buttonLedStates[buttonId] = !mrSystemState.buttonLedStates[buttonId];
            mrIndicators.setButtonLed(buttonId, mrSystemState.buttonLedStates[buttonId]);
            break;
        case LedPolicy::None:
        default:
            break;
        }
    }

    void ControlBoardInputDispatcher::applyLedOnRelease(uint8_t buttonId, LedPolicy policy)
    {
        if (policy != LedPolicy::Momentary)
        {
            return;
        }
        mrIndicators.setButtonLed(buttonId, false);
    }

    void ControlBoardInputDispatcher::handleButtonPressed(uint8_t buttonPressedId)
    {
        mrIndicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        if (buttonPressedId >= controlBoardButtons::kCount)
        {
            return;
        }

        const ButtonConfig &config = mrActionMap[buttonPressedId];
        applyLedOnPress(buttonPressedId, config.ledPolicy);

        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(true);
            mrResponseSink.process(result);
        }
    }

    void ControlBoardInputDispatcher::handleButtonReleased(uint8_t buttonReleasedId)
    {
        mrIndicators.setActivityStatus(mBackgroundStatus);

        if (buttonReleasedId >= controlBoardButtons::kCount)
        {
            return;
        }

        const ButtonConfig &config = mrActionMap[buttonReleasedId];
        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(false);
            mrResponseSink.process(result);
            applyLedOnRelease(buttonReleasedId, config.ledPolicy);
        }
    }

    void ControlBoardInputDispatcher::handleRotaryMovement(int direction)
    {
        mrIndicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        const ButtonConfig &config = mrActionMap[controlBoardButtons::kRotaryEventLeft];
        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(direction > 0);
            mrResponseSink.process(result);
        }

        mrIndicators.setActivityStatus(mBackgroundStatus);
    }
}