#pragma once

#include <cstdint>

namespace controlSystem::controlBoardButtons
{
    inline constexpr uint8_t k_power = 0;
    inline constexpr uint8_t k_prevTrack = 1;
    inline constexpr uint8_t k_nextTrack = 2;
    inline constexpr uint8_t k_skipForward = 3;
    inline constexpr uint8_t k_skipBack = 4;
    inline constexpr uint8_t k_playPause = 5;
    inline constexpr uint8_t k_stop = 6;
    inline constexpr uint8_t k_cover = 7;
    inline constexpr uint8_t k_repeat = 8;
    inline constexpr uint8_t k_toggleRandom = 9;
    inline constexpr uint8_t k_toggleDac = 10;
    inline constexpr uint8_t k_toggleDisplay = 11;
    inline constexpr uint8_t k_toggleMeter = 12;
    inline constexpr uint8_t k_rotaryEventLeft = 13;
    inline constexpr uint8_t k_rotaryEventRight = 14;
    inline constexpr uint8_t k_cycleBrightness = 15;
    inline constexpr uint8_t k_count = 16;
}