#pragma once

#include <mutex> // optional if you need thread safety

enum class ControlBoardPowerState
{
    OFF,
    SHUTTING_DOWN,
    ON,
    TURNING_ON,
    SLEEP,
    GOING_TO_SLEEP,
    DEEPSLEEP,
    GOING_INTO_DEEP_SLEEP,
};

class PowerStateManager
{
public:
    // Singleton access
    static PowerStateManager &getInstance()
    {
        static PowerStateManager sInstance;
        return sInstance;
    }

    // Get and set state
    void setPowerState(ControlBoardPowerState newState)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mPowerState = newState;
    }

    ControlBoardPowerState getPowerState() const
    {
        std::lock_guard<std::mutex> lock(mMutex);
        return mPowerState;
    }

private:
    mutable std::mutex mMutex;
    // Private constructor/destructor
    PowerStateManager() = default;
    ~PowerStateManager() = default;

    // Delete copy and move
    PowerStateManager(const PowerStateManager &) = delete;
    PowerStateManager &operator=(const PowerStateManager &) = delete;

    ControlBoardPowerState mPowerState = ControlBoardPowerState::OFF;
};