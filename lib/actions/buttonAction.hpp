#pragma once


namespace actions
{

    struct ActionResponse;
    class ButtonAction
    {
    public:
        virtual ~ButtonAction() = default;
        virtual ActionResponse execute(bool isPressed) = 0;
    };
}