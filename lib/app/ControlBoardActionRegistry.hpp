#pragma once

#include "app/ControlBoardInputDispatcher.hpp"

namespace controlSystem
{
    class ControlBoardActionRegistry
    {
    public:
        void populate(ControlBoardInputDispatcher::ActionMap &rActionMap) const;
    };
}