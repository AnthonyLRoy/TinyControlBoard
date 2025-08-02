#pragma once

namespace actions
{
    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;
        virtual bool execute() = 0;
    };
}