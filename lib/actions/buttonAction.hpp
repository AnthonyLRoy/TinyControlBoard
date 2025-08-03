#pragma once
#include "actionsResponse.hpp"

namespace actions
{
    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;
        virtual actionResponse execute(bool buttonMode) = 0;
    };
}