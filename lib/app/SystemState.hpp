#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>
#include <mutex>
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
        // Incremented by UART RX task each time now-playing text is fully updated.
        std::atomic<uint8_t>  nowPlayingVersion{0};
        void setNowPlaying(const uint8_t *text, uint8_t len)
        {
            std::lock_guard<std::mutex> lock(m_nowPlayingMutex);
            std::memcpy(m_nowPlayingText.data(), text, len);
            m_nowPlayingText[len] = '\0';
            nowPlayingVersion.fetch_add(1, std::memory_order_release);
        }
        std::array<char, protocol::k_maxNowPlayingLen + 1> getNowPlaying() const
        {
            std::lock_guard<std::mutex> lock(m_nowPlayingMutex);
            return m_nowPlayingText;
        }
        // The version brackets a coherent track-progress snapshot.
        std::atomic<uint16_t> trackElapsedSeconds{0};
        std::atomic<uint16_t> trackDurationSeconds{0};
        std::atomic<bool>     trackIsPlaying{false};
        std::atomic<bool>     trackProgressUpdating{false};
        std::atomic<uint8_t>  trackProgressVersion{0};
    private:
        mutable std::mutex m_nowPlayingMutex;
        std::array<char, protocol::k_maxNowPlayingLen + 1> m_nowPlayingText{};
    };
}
