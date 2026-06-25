#pragma once

#include "power/powerState.hpp"
#include <atomic>

namespace controlSystem
{
    class DelayedBootRecoveryState
    {
    public:
        void clear()
        {
            m_pending.store(false, std::memory_order_release);
        }

        void markBootTimedOut()
        {
            m_pending.store(true, std::memory_order_release);
        }

        bool isPending() const
        {
            return m_pending.load(std::memory_order_acquire);
        }

        bool consumeIfRecoverableState(ControlBoardPowerState powerState)
        {
            if (powerState != ControlBoardPowerState::SLEEP &&
                powerState != ControlBoardPowerState::TURNING_ON)
            {
                return false;
            }

            bool expected = true;
            return m_pending.compare_exchange_strong(expected, false,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_acquire);
        }

    private:
        std::atomic<bool> m_pending{false};
    };
}