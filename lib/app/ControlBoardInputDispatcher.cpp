#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    void ControlBoardInputDispatcher::applyLedOnPress(uint8_t buttonId, LedPolicy policy)
    {
        if (!mpIndicators)
        {
            return;
        }
        switch (policy)
        {
        case LedPolicy::Momentary:
            mpIndicators->setButtonLed(buttonId, true);
            break;
        case LedPolicy::Toggle:
            mToggleLedState[buttonId] = !mToggleLedState[buttonId];
            mpIndicators->setButtonLed(buttonId, mToggleLedState[buttonId]);
            break;
        case LedPolicy::None:
        default:
            break;
        }
    }

    void ControlBoardInputDispatcher::applyLedOnRelease(uint8_t buttonId, LedPolicy policy)
    {
        if (!mpIndicators || policy != LedPolicy::Momentary)
        {
            return;
        }
        mpIndicators->setButtonLed(buttonId, false);
    }

    void ControlBoardInputDispatcher::handleButtonPressed(uint8_t buttonPressedId)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::doingWork);
        }

        if (buttonPressedId >= controlBoardButtons::kCount || !mpResponseSink)
        {
            return;
        }

        const ButtonConfig &config = mrActionMap[buttonPressedId];
        applyLedOnPress(buttonPressedId, config.ledPolicy);

        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(true);
            mpResponseSink->process(result);
        }
    }

    void ControlBoardInputDispatcher::handleButtonReleased(uint8_t buttonReleasedId)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(mBackgroundStatus);
        }

        if (buttonReleasedId >= controlBoardButtons::kCount || !mpResponseSink)
        {
            return;
        }

        const ButtonConfig &config = mrActionMap[buttonReleasedId];
        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(false);
            mpResponseSink->process(result);
            applyLedOnRelease(buttonReleasedId, config.ledPolicy);
        }
    }

    void ControlBoardInputDispatcher::handleRotaryMovement(int direction)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::doingWork);
        }

        const ButtonConfig &config = mrActionMap[controlBoardButtons::kRotaryEventLeft];
        if (mpResponseSink && config.action)
        {
            const actions::ActionResponse result = config.action->execute(direction > 0);
            mpResponseSink->process(result);
        }

        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(mBackgroundStatus);
        }
    }
}