#include "app/ControlBoardActionRegistry.hpp"

#include "app/ActionCommandCatalog.hpp"
#include "input/actions/actionTemplates.hpp"
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
            CommandId        command;
            LedPolicy        ledPolicy;
        };

        // -----------------------------------------------------------------------
        // Button registration table.
        // To add a new button: append one row — no other file needs to change
        // beyond defining the command and, for toggles, its shared spec.
        // -----------------------------------------------------------------------
        constexpr ButtonRegistration k_buttons[] = {
            { controlBoardButtons::k_power,            ActionSourceType::Timed,   CMD_SYS_POWER,         LedPolicy::None      },
            { controlBoardButtons::k_prevTrack,        ActionSourceType::Simple,  CMD_PREVIOUS_TRACK,    LedPolicy::Momentary },
            { controlBoardButtons::k_nextTrack,        ActionSourceType::Simple,  CMD_NEXT_TRACK,        LedPolicy::Momentary },
            { controlBoardButtons::k_skipForward,      ActionSourceType::Simple,  CMD_SKIP_FORWARD,      LedPolicy::Momentary },
            { controlBoardButtons::k_skipBack,         ActionSourceType::Simple,  CMD_SKIP_BACK,         LedPolicy::Momentary },
            { controlBoardButtons::k_playPause,        ActionSourceType::Simple,  CMD_PLAY_PAUSE,        LedPolicy::Momentary },
            { controlBoardButtons::k_toggleDisplay,    ActionSourceType::Simple,  CMD_TOGGLE_DISPLAY,    LedPolicy::Toggle    },
            { controlBoardButtons::k_cover,            ActionSourceType::Toggle,  CMD_TOGGLE_COVER_VIEW, LedPolicy::Toggle    },
            { controlBoardButtons::k_repeat,           ActionSourceType::Toggle,  CMD_TOGGLE_REPEAT,     LedPolicy::Toggle    },
            { controlBoardButtons::k_toggleRandom,     ActionSourceType::Toggle,  CMD_TOGGLE_RANDOM,     LedPolicy::Toggle    },
            { controlBoardButtons::k_toggleDac,        ActionSourceType::Toggle,  CMD_TOGGLE_DAC,        LedPolicy::Toggle    },
            { controlBoardButtons::k_nextPanel,        ActionSourceType::Simple,  CMD_NEXT_MENU_ITEM,    LedPolicy::Momentary },
            { controlBoardButtons::k_toggleMeter,      ActionSourceType::Toggle,  CMD_TOGGLE_METER,      LedPolicy::Toggle    },
            { controlBoardButtons::k_rotaryEventLeft,  ActionSourceType::Rotary,  CMD_ROTARY_ACTION,     LedPolicy::None      },
            { controlBoardButtons::k_rotaryEventRight, ActionSourceType::Rotary,  CMD_ROTARY_ACTION,     LedPolicy::None      },
            { controlBoardButtons::k_cycleBrightness,  ActionSourceType::Simple,  CMD_CYCLE_BRIGHTNESS,  LedPolicy::Momentary },
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
                action = std::make_unique<actions::SimpleCommandAction>(reg.command);
                break;
            case ActionSourceType::Toggle:
            {
                const auto *toggleSpec = findToggleCommandSpecBySemanticCommand(reg.command);
                if (toggleSpec == nullptr)
                {
                    continue;
                }
                action = std::make_unique<actions::DynamicToggleAction>(toggleSpec->onCommand, toggleSpec->offCommand);
                break;
            }
            case ActionSourceType::Timed:
                action = std::make_unique<actions::DynamicTimedAction>(reg.command);
                break;
            default:
                break;
            }

            rActionMap[reg.buttonId] = {action.get(), reg.ledPolicy};
            m_ownedActions.push_back(std::move(action));
        }
    }
}