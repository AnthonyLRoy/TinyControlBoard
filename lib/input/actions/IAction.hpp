#pragma once

#include "protocol/uartProtocol.hpp"
#include <cstdint>

// Forward declaration only — headers that define ActionContext pull in
// ESP-IDF dependencies that must not appear in the host-test build.
namespace controlSystem { struct ActionContext; }

namespace actions
{
    /// Abstract base for all command objects produced by the action pipeline.
    /// Carries the data fields previously held by the Action struct and adds
    /// two virtual methods:
    ///   requiresPowerOn() — governs the power gate in ActionProcessor.
    ///   execute()         — performs the side-effects for this command.
    class IAction
    {
    public:
        virtual ~IAction() = default;
        virtual bool requiresPowerOn() const = 0;
        virtual void execute(controlSystem::ActionContext &ctx) = 0;

        // Data fields (same contract as the old Action struct, minus route).
        CommandId command = CMD_NO_ACTION;
        uint16_t  parameters[5]{0, 0, 0, 0, 0};
        uint16_t  releaseTimeMillis = 0;
        bool      isActive = false;
    };
}
