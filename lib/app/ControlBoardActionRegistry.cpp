#include "app/ControlBoardActionRegistry.hpp"

#include "app/ActionCommandCatalog.hpp"
#include "input/actions/actionTemplates.hpp"
#include <memory>

namespace controlSystem
{
    namespace
    {
        using ActionMap = ControlBoardInputDispatcher::ActionMap;
        using OwnedActions = std::vector<std::unique_ptr<actions::IActionSource>>;

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

        actions::IActionSource *registerOwnedAction(OwnedActions &rOwnedActions,
                                                    ActionMap &rActionMap,
                                                    uint8_t buttonId,
                                                    LedPolicy ledPolicy,
                                                    std::unique_ptr<actions::IActionSource> action)
        {
            if (!action)
            {
                return nullptr;
            }

            auto *p_action = action.get();
            rActionMap[buttonId] = {p_action, ledPolicy};
            rOwnedActions.push_back(std::move(action));
            return p_action;
        }

        std::unique_ptr<actions::IActionSource> createActionSource(const ButtonRegistration &reg)
        {
            switch (reg.type)
            {
            case ActionSourceType::Simple:
                return std::make_unique<actions::SimpleCommandAction>(reg.command);

            case ActionSourceType::Toggle:
            {
                const auto *toggleSpec = findToggleCommandSpecBySemanticCommand(reg.command);
                if (toggleSpec == nullptr)
                {
                    return nullptr;
                }
                return std::make_unique<actions::DynamicToggleAction>(toggleSpec->onCommand, toggleSpec->offCommand);
            }

            case ActionSourceType::Timed:
                return std::make_unique<actions::DynamicTimedAction>(reg.command);

            case ActionSourceType::Rotary:
            default:
                return nullptr;
            }
        }

        actions::IActionSource *ensureSharedRotaryAction(OwnedActions &rOwnedActions)
        {
            auto rotary = std::make_unique<actions::DynamicRotaryAction>();
            auto *p_action = rotary.get();
            rOwnedActions.push_back(std::move(rotary));
            return p_action;
        }
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
                    p_sharedRotary = ensureSharedRotaryAction(m_ownedActions);
                }
                rActionMap[reg.buttonId] = {p_sharedRotary, reg.ledPolicy};
                continue;
            }

            registerOwnedAction(
                m_ownedActions,
                rActionMap,
                reg.buttonId,
                reg.ledPolicy,
                createActionSource(reg));
        }
    }
}