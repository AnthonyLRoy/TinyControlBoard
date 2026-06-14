#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    bool ControlBoardInputDispatcher::isInputSuppressedInSleep(uint8_t buttonId) const
    {
        return m_backgroundStatus == ControlBoardWorkingStatus::sleeping &&
               buttonId != controlBoardButtons::k_power;
    }

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
            m_buttonLedBitmask ^= mask;
            const bool newState = (m_buttonLedBitmask >> buttonId) & 1u;
            mr_indicators.setButtonLed(buttonId, newState);
            break;
        }
        case LedPolicy::None:
        default:
            break;
        }
    }

    void ControlBoardInputDispatcher::clearMomentaryLedOnRelease(uint8_t buttonId, LedPolicy policy)
    {
        if (policy != LedPolicy::Momentary)
        {
            return;
        }
        mr_indicators.setButtonLed(buttonId, false);
    }

    void ControlBoardInputDispatcher::dispatchButtonAction(const ButtonConfig &config, bool isPressed)
    {
        if (!config.action)
        {
            return;
        }

        auto iaction = config.action->produce(isPressed);
        if (iaction)
        {
            m_onResponse(std::move(iaction));
        }
    }

    void ControlBoardInputDispatcher::handleButtonPressed(uint8_t buttonPressedId)
    {
        if (isInputSuppressedInSleep(buttonPressedId))
        {
            return;
        }

        mr_indicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        if (buttonPressedId >= controlBoardButtons::k_count)
        {
            return;
        }

        const ButtonConfig &config = mr_actionMap[buttonPressedId];
        applyLedOnPress(buttonPressedId, config.ledPolicy);
        dispatchButtonAction(config, true);
    }

    void ControlBoardInputDispatcher::handleButtonReleased(uint8_t buttonReleasedId)
    {
        if (buttonReleasedId >= controlBoardButtons::k_count)
        {
            return;
        }

        if (isInputSuppressedInSleep(buttonReleasedId))
        {
            return;
        }

        mr_indicators.setActivityStatus(m_backgroundStatus);

        const ButtonConfig &config = mr_actionMap[buttonReleasedId];
        dispatchButtonAction(config, false);
        clearMomentaryLedOnRelease(buttonReleasedId, config.ledPolicy);
    }

    void ControlBoardInputDispatcher::handleRotaryMovement(int direction)
    {
        if (m_backgroundStatus == ControlBoardWorkingStatus::sleeping)
        {
            return;
        }

        mr_indicators.setActivityStatus(ControlBoardWorkingStatus::doingWork);

        const ButtonConfig &config = mr_actionMap[controlBoardButtons::k_rotaryEventLeft];
        if (config.action)
        {
            auto iaction = config.action->produce(direction > 0);
            if (iaction)
            {
                m_onResponse(std::move(iaction));
            }
        }

        mr_indicators.setActivityStatus(m_backgroundStatus);
    }
}