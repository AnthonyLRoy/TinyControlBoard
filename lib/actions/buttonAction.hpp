#pragma once
#include "actionsResponse.hpp"
namespace actions
{
    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;
        virtual actionResponse execute() = 0;
    };
}