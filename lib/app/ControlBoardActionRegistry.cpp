#include "app/ControlBoardActionRegistry.hpp"

#include "input/actions/actionTemplates.hpp"
#include "protocol/uartProtocol.hpp"
#include <memory>

namespace controlSystem
{
    namespace
    {
        enum class ActionSourceType : uint8_t { Simple, Toggle, Timed, Rotary };

        struct ButtonRegistration
        {
            uint8_t          buttonId;
            ActionSourceType type;
            CommandId        cmd1;     // main command (or CMD_ON for Toggle)
            CommandId        cmd2;     // CMD_OFF for Toggle; ignored otherwise
            LedPolicy        ledPolicy;
        };

        // -----------------------------------------------------------------------
        // Button registration table.
        // To add a new button: append one row — no other file needs to change
        // (beyond defining the CMD_* constant in uartProtocol.hpp).
        // -----------------------------------------------------------------------
        constexpr ButtonRegistration k_buttons[] = {
            { controlBoardButtons::k_power,            ActionSourceType::Timed,   CMD_SYS_POWER,       CMD_NO_ACTION,        LedPolicy::None      },
            { controlBoardButtons::k_prevTrack,        ActionSourceType::Simple,  CMD_PREVIOUS_TRACK,  CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_nextTrack,        ActionSourceType::Simple,  CMD_NEXT_TRACK,      CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_skipForward,      ActionSourceType::Simple,  CMD_SKIP_FORWARD,    CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_skipBack,         ActionSourceType::Simple,  CMD_SKIP_BACK,       CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_playPause,        ActionSourceType::Simple,  CMD_PLAY_PAUSE,      CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_toggleDisplay,    ActionSourceType::Simple,  CMD_TOGGLE_DISPLAY,  CMD_NO_ACTION,        LedPolicy::Toggle    },
            { controlBoardButtons::k_cover,            ActionSourceType::Toggle,  CMD_COVER_VIEW_ON,   CMD_COVER_VIEW_OFF,   LedPolicy::Toggle    },
            { controlBoardButtons::k_repeat,           ActionSourceType::Toggle,  CMD_REPEAT_ON,       CMD_REPEAT_OFF,       LedPolicy::Toggle    },
            { controlBoardButtons::k_toggleRandom,     ActionSourceType::Toggle,  CMD_RANDOM_ON,       CMD_RANDOM_OFF,       LedPolicy::Toggle    },
            { controlBoardButtons::k_toggleDac,        ActionSourceType::Toggle,  CMD_TOGGLE_DAC_ON,   CMD_TOGGLE_DAC_OFF,   LedPolicy::Toggle    },
            { controlBoardButtons::k_nextPanel,        ActionSourceType::Simple,  CMD_NEXT_MENU_ITEM,  CMD_NO_ACTION,        LedPolicy::Momentary },
            { controlBoardButtons::k_toggleMeter,      ActionSourceType::Toggle,  CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF, LedPolicy::Toggle    },
            { controlBoardButtons::k_rotaryEventLeft,  ActionSourceType::Rotary,  CMD_ROTARY_ACTION,   CMD_NO_ACTION,        LedPolicy::None      },
            { controlBoardButtons::k_rotaryEventRight, ActionSourceType::Rotary,  CMD_ROTARY_ACTION,   CMD_NO_ACTION,        LedPolicy::None      },
            { controlBoardButtons::k_cycleBrightness,  ActionSourceType::Simple,  CMD_CYCLE_BRIGHTNESS,CMD_NO_ACTION,        LedPolicy::Momentary },
        };
    } // namespace

    void ControlBoardActionRegistry::populate(ControlBoardInputDispatcher::ActionMap &rActionMap)
    {
        m_ownedActions.clear();
        m_ownedActions.reserve(controlBoardButtons::k_count);

        // Left and right rotary slots share one action instance because
        // ControlBoardInputDispatcher::handleRotaryMovement() always reads k_rotaryEventLeft.
        actions::IActionSource *p_sharedRotary = nullptr;

        for (const auto &reg : k_buttons)
        {
            if (reg.type == ActionSourceType::Rotary)
            {
                if (p_sharedRotary == nullptr)
                {
                    auto rotary = std::make_unique<actions::DynamicRotaryAction>();
                    p_sharedRotary = rotary.get();
                    m_ownedActions.push_back(std::move(rotary));
                }
                rActionMap[reg.buttonId] = {p_sharedRotary, reg.ledPolicy};
                continue;
            }

            std::unique_ptr<actions::IActionSource> action;
            switch (reg.type)
            {
            case ActionSourceType::Simple:
                action = std::make_unique<actions::SimpleCommandAction>(reg.cmd1);
                break;
            case ActionSourceType::Toggle:
                action = std::make_unique<actions::DynamicToggleAction>(reg.cmd1, reg.cmd2);
                break;
            case ActionSourceType::Timed:
                action = std::make_unique<actions::DynamicTimedAction>(reg.cmd1);
                break;
            default:
                break;
            }

            rActionMap[reg.buttonId] = {action.get(), reg.ledPolicy};
            m_ownedActions.push_back(std::move(action));
        }
    }
}