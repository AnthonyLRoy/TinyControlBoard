#pragma once

#include <cstdint>

namespace controlSystem
{
    enum class ButtonEventType : uint8_t
    {
        Press,
        Release,
        Rotary,
        RemoteCommand,
    };

    struct ButtonEvent
    {
        ButtonEventType type = ButtonEventType::Press;
        uint8_t buttonId = 0;
        int8_t rotaryDelta = 0;
        uint16_t command = 0;
        uint16_t releaseMs = 0;
    };
}
