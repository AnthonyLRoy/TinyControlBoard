#pragma once

enum class ControlBoardWorkingStatus {
    doingWork,
    Idle,
    SolidIdle,
    sleeping,
    Active
};

struct IActivityStatusSink
{
    // Allows implementations to be destroyed through the activity-status interface.
    virtual ~IActivityStatusSink() = default;
    // Receives a new board activity state for display or other status feedback.
    virtual void setActivityStatus(ControlBoardWorkingStatus status) = 0;
};
