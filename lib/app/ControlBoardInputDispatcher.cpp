#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    // Updates the board's background state and clears toggle LED memory when the system enters sleep.
    void ControlBoardInputDispatcher::setBackgroundStatus(ControlBoardWorkingStatus status)
    {
        m_backgroundStatus = status;
        if (status == ControlBoardWorkingStatus::sleeping)
        {
            resetToggleLeds();
        }
    }

    // Clears any remembered toggle-LED state so the next wake cycle starts from a neutral LED state.
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

    // Suppresses non-power button input while the board is asleep, except for the power button itself.
    bool ControlBoardInputDispatcher::isInputSuppressedInSleep(uint8_t buttonId) const
    {
        return m_backgroundStatus == ControlBoardWorkingStatus::sleeping &&
               buttonId != controlBoardButtons::k_power;
    }

    // Applies the configured LED behavior for the moment a button is pressed.
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

    // Clears press-time LED feedback when a momentary LED is released.
    void ControlBoardInputDispatcher::applyLedOnRelease(uint8_t buttonId, LedPolicy policy)
    {
        if (policy != LedPolicy::Momentary)
        {
            return;
        }
        mr_indicators.setButtonLed(buttonId, false);
    }

    // Handles a physical press by verifying sleep suppression, updating LED feedback, and emitting the mapped action.
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

    // Handles a button release by emitting the release-phase action and clearing any momentary LED state.
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

    // Handles rotary motion by translating encoder direction into the appropriate action and activity-state update.
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

    // Mirrors a remote toggle event onto the button's LED state so hardware and remote state stay in sync.
    void ControlBoardInputDispatcher::toggleButtonLed(uint8_t buttonId)
    {
        if (buttonId < controlBoardButtons::k_count)
        {
            applyLedOnPress(buttonId, LedPolicy::Toggle);

            // Keep the button's own toggle action in sync so a later physical
            // press continues from the state the remote (app) toggle left it in,
            // instead of emitting a stale ON/OFF command.
            const bool newState = (m_buttonLedBitmask >> buttonId) & 1u;
            if (actions::IActionSource *p_action = mr_actionMap[buttonId].action)
            {
                p_action->syncExternalState(newState);
            }
        }
    }
}