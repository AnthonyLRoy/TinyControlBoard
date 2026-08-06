#pragma once

#include <atomic>
#include <cstdint>
#include "power/powerState.hpp"
#include "protocol/uartProtocol.hpp"

namespace controlSystem
{
    // Central, thread-safe runtime state. Uses only std::atomic — no FreeRTOS headers.
    struct SystemState
    {
        std::atomic<ControlBoardPowerState> powerState{ControlBoardPowerState::OFF};
        // One bit per button (bit[n] = buttonId n). Updated by ControlBoard::setButtonLed.
        std::atomic<uint16_t> buttonLedBitmask{0};
        // Incremented by UART RX task each time nowPlayingText is fully updated.
        std::atomic<uint8_t>  nowPlayingVersion{0};
        char nowPlayingText[protocol::k_maxNowPlayingLen + 1]{};
        // The version brackets a coherent track-progress snapshot.
        std::atomic<uint16_t> trackElapsedSeconds{0};
        std::atomic<uint16_t> trackDurationSeconds{0};
        std::atomic<bool>     trackIsPlaying{false};
        std::atomic<bool>     trackProgressUpdating{false};
        std::atomic<uint8_t>  trackProgressVersion{0};
    };
}
