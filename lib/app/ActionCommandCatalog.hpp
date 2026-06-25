#pragma once

#include "app/ActionCommandRoute.hpp"
#include "protocol/uartProtocol.hpp"

namespace controlSystem
{
    struct ToggleCommandSpec
    {
        CommandId semanticCommand;
        CommandId onCommand;
        CommandId offCommand;
        ActionCommandRoute route;
        const char *p_uartLogTag;
    };

    namespace detail
    {
        constexpr ToggleCommandSpec k_toggleCommandSpecs[] = {
            {CMD_TOGGLE_COVER_VIEW, CMD_COVER_VIEW_ON,   CMD_COVER_VIEW_OFF,   ActionCommandRoute::UartDispatch, "Cover_View"},
            {CMD_TOGGLE_METER,      CMD_TOGGLE_METER_ON, CMD_TOGGLE_METER_OFF, ActionCommandRoute::UartDispatch, "Meter"},
            {CMD_TOGGLE_REPEAT,     CMD_REPEAT_ON,       CMD_REPEAT_OFF,       ActionCommandRoute::UartDispatch, "Repeat"},
            {CMD_TOGGLE_RANDOM,     CMD_RANDOM_ON,       CMD_RANDOM_OFF,       ActionCommandRoute::UartDispatch, "Random"},
            {CMD_TOGGLE_DAC,        CMD_TOGGLE_DAC_ON,   CMD_TOGGLE_DAC_OFF,   ActionCommandRoute::Relay,        nullptr},
        };
    } // namespace detail

    constexpr const ToggleCommandSpec *findToggleCommandSpecBySemanticCommand(CommandId command)
    {
        for (const auto &spec : detail::k_toggleCommandSpecs)
        {
            if (spec.semanticCommand == command)
            {
                return &spec;
            }
        }

        return nullptr;
    }

    constexpr const ToggleCommandSpec *findToggleCommandSpecByStateCommand(CommandId command)
    {
        for (const auto &spec : detail::k_toggleCommandSpecs)
        {
            if (spec.onCommand == command || spec.offCommand == command)
            {
                return &spec;
            }
        }

        return nullptr;
    }

    constexpr ActionCommandRoute classifyOnStateCommand(CommandId command)
    {
        if (command == CMD_NO_ACTION)
        {
            return ActionCommandRoute::None;
        }

        if (command == CMD_SYS_POWER)
        {
            return ActionCommandRoute::PowerStateTransition;
        }

        if (const auto *toggleSpec = findToggleCommandSpecByStateCommand(command))
        {
            return toggleSpec->route;
        }

        if (const auto *toggleSpec = findToggleCommandSpecBySemanticCommand(command))
        {
            return toggleSpec->route;
        }

        switch (command)
        {
        case CMD_SYS_RPI_SHUTDOWN:
        case CMD_EXIT_ITEM:
            return ActionCommandRoute::System;

        case CMD_CYCLE_BRIGHTNESS:
        case CMD_TOGGLE_DISPLAY:
            return ActionCommandRoute::Brightness;

        default:
            return ActionCommandRoute::UartDispatch;
        }
    }
}