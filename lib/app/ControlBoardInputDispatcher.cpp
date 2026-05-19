#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    void ControlBoardInputDispatcher::applyLedOnPress(uint8_t buttonId, LedPolicy policy)
    {
        switch (policy)
        {
        case LedPolicy::Momentary:
            mr_indicators.setButtonLed(buttonId, true);
            break;
        case LedPolicy::Toggle:
        {
            const auto mask = static_cast<uint16_t>(1u << buttonId);
            const uint16_t prev = mr_systemState.buttonLedBitmask.fetch_xor(mask);
            const bool newState = !((prev >> buttonId) & 1u);
            mr_indicators.setButtonLed(buttonId, newState);
            break;
        }
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
        mr_indicators.setButtonLed(buttonId, false);
    }

    void ControlBoardInputDispatcher::handleButtonPressed(uint8_t buttonPressedId)
    {
        mr_indicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        if (buttonPressedId >= controlBoardButtons::k_count)
        {
            return;
        }

        const ButtonConfig &config = mr_actionMap[buttonPressedId];
        applyLedOnPress(buttonPressedId, config.ledPolicy);

        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(true);
            mr_responseSink.process(result);
        }
    }

    void ControlBoardInputDispatcher::handleButtonReleased(uint8_t buttonReleasedId)
    {
        mr_indicators.setActivityStatus(m_backgroundStatus);

        if (buttonReleasedId >= controlBoardButtons::k_count)
        {
            return;
        }

        const ButtonConfig &config = mr_actionMap[buttonReleasedId];
        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(false);
            mr_responseSink.process(result);
            applyLedOnRelease(buttonReleasedId, config.ledPolicy);
        }
    }

    void ControlBoardInputDispatcher::handleRotaryMovement(int direction)
    {
        mr_indicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        const ButtonConfig &config = mr_actionMap[controlBoardButtons::k_rotaryEventLeft];
        if (config.action)
        {
            const actions::ActionResponse result = config.action->execute(direction > 0);
            mr_responseSink.process(result);
        }

        mr_indicators.setActivityStatus(m_backgroundStatus);
    }
}