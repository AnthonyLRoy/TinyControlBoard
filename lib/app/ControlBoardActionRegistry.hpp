#pragma once

#include "app/ControlBoardInputDispatcher.hpp"
#include "input/actions/buttonAction.hpp"
#include <memory>
#include <vector>

namespace controlSystem
{
    class ControlBoardActionRegistry
    {
    public:
        /// Constructs one action source per button and writes them into rActionMap.
        /// Owned action sources live for the lifetime of this registry object.
        // Populates the button-to-action map with the board's configured action sources.
        void populate(ControlBoardInputDispatcher::ActionMap &rActionMap);
        // Resets any action state that persists across button activity cycles.
        void resetActionStates();

    private:
        std::vector<std::unique_ptr<actions::IActionSource>> m_ownedActions;
    };
}