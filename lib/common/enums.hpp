    #pragma once
    

    
    enum ControlBoardState
    {
        Standby,
        Active,
        Error,
        Maintenance
    };

    enum class ControlBoardWorkingStatus    
    {
        doingWork,
        Idle,
        sleeping,
        MaintenanceMode
    };





