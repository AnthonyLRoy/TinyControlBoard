#pragma once


namespace actions
{

    struct actionResponse;
    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;
        virtual actionResponse execute(bool pressed) = 0;
    };
}