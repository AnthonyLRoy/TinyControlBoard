
#pragma once
#include "powerLed.hpp"




namespace controlSystem
{


    class ControlBoard
    {
    public:
        ControlBoard() = default;
        ~ControlBoard() = default;

        // Initialize the control board
        void init();

    };// namespace controlSystem
}