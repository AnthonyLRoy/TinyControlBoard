#pragma once

#include <cstdint>

namespace controlSystem
{
    enum class ButtonEventType : uint8_t
    {
        Press,
        Release,
        Rotary,
    };

    struct ButtonEvent
    {
        ButtonEventType type = ButtonEventType::Press;
        uint8_t buttonId = 0;
        int8_t rotaryDelta = 0;
    };
}
