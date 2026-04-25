#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    void ControlBoardInputDispatcher::handleButtonPressed(uint8_t buttonPressedId)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::doingWork);
            if (buttonPressedId > 0)
            {
                mpIndicators->setButtonLed(buttonPressedId, true);
            }
        }

        if (buttonPressedId >= controlBoardButtons::kCount || !mpResponseSink)
        {
            return;
        }

        if (mrActionMap[buttonPressedId])
        {
            const actions::ActionResponse result = mrActionMap[buttonPressedId]->execute(true);
            mpResponseSink->process(result);
        }
    }

    void ControlBoardInputDispatcher::handleButtonReleased(uint8_t buttonReleasedId)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::Idle);
        }

        if (buttonReleasedId >= controlBoardButtons::kCount || !mpResponseSink)
        {
            return;
        }

        if (mrActionMap[buttonReleasedId])
        {
            const actions::ActionResponse result = mrActionMap[buttonReleasedId]->execute(false);
            mpResponseSink->process(result);

            if (mpIndicators && buttonReleasedId > 0)
            {
                mpIndicators->setButtonLed(buttonReleasedId, result.keepLedActive);
            }
        }
    }

    void ControlBoardInputDispatcher::handleRotaryMovement(int direction)
    {
        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::doingWork);
        }

        if (mpResponseSink && mrActionMap[controlBoardButtons::kRotaryEventLeft])
        {
            const actions::ActionResponse result =
                mrActionMap[controlBoardButtons::kRotaryEventLeft]->execute(direction > 0);
            mpResponseSink->process(result);
        }

        if (mpIndicators)
        {
            mpIndicators->setActivityStatus(ControlBoardWorkingStatus::Idle);
        }
    }
}