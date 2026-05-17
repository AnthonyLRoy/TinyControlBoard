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
    virtual ~IActivityStatusSink() = default;
    virtual void setActivityStatus(ControlBoardWorkingStatus status) = 0;
};
