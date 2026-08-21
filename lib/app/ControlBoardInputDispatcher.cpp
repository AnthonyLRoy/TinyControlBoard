#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    void ControlBoardInputDispatcher::setBackgroundStatus(ControlBoardWorkingStatus status)
    {
        m_backgroundStatus = status;
        if (status == ControlBoardWorkingStatus::sleeping)
        {
            resetToggleLeds();
        }
    }

    void ControlBoardInputDispatcher::resetToggleLeds()
    {
        m_buttonLedBitmask = 0;
        for (uint8_t buttonId = 0; buttonId < controlBoardButtons::k_count; ++buttonId)
        {
            if (mr_actionMap[buttonId].ledPolicy == LedPolicy::Toggle)
            {
                mr_indicators.setButtonLed(buttonId, false);
            }
        }
    }

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

        if (config.action)
        {
            auto iaction = config.action->produce(true);
            if (iaction)
            {
                m_onResponse(std::move(iaction));
            }
        }
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
        if (config.action)
        {
            auto iaction = config.action->produce(false);
            if (iaction)
            {
                m_onResponse(std::move(iaction));
            }
            applyLedOnRelease(buttonReleasedId, config.ledPolicy);
        }
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
            // Physical wiring/quadrature decode yields the opposite sign of what "left" means
            // here (confirmed: rotating left was selecting next track and vice versa).
            auto iaction = config.action->produce(direction < 0);
            if (iaction)
            {
                m_onResponse(std::move(iaction));
            }
        }

        mr_indicators.setActivityStatus(m_backgroundStatus);
    }

    void ControlBoardInputDispatcher::toggleButtonLed(uint8_t buttonId)
    {
        if (buttonId < controlBoardButtons::k_count)
        {
            applyLedOnPress(buttonId, LedPolicy::Toggle);
        }
    }
}