#pragma once

#include <cstdint>

namespace controlSystem::controlBoardButtons
{
    inline constexpr uint8_t kPower = 0;
    inline constexpr uint8_t kPrevTrack = 1;
    inline constexpr uint8_t kNextTrack = 2;
    inline constexpr uint8_t kSkipForward = 3;
    inline constexpr uint8_t kSkipBack = 4;
    inline constexpr uint8_t kPlayPause = 5;
    inline constexpr uint8_t kStop = 6;
    inline constexpr uint8_t kCover = 7;
    inline constexpr uint8_t kRepeat = 8;
    inline constexpr uint8_t kMenuSelect = 9;
    inline constexpr uint8_t kToggleDac = 10;
    inline constexpr uint8_t kToggleDisplay = 11;
    inline constexpr uint8_t kToggleMeter = 12;
    inline constexpr uint8_t kRotaryEventLeft = 13;
    inline constexpr uint8_t kRotaryEventRight = 14;
    inline constexpr uint8_t kCycleBrightness = 15;
    inline constexpr uint8_t kCount = 16;
}