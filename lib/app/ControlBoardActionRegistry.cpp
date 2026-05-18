#include "app/ControlBoardActionRegistry.hpp"

#include "input/actions/buttonActions.hpp"

namespace controlSystem
{
    void ControlBoardActionRegistry::populate(ControlBoardInputDispatcher::ActionMap &rActionMap) const
    {
        rActionMap[controlBoardButtons::k_power]           = {&actions::PowerButtonInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::k_prevTrack]       = {&actions::PreviousTrackInstance,      LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_nextTrack]       = {&actions::NextTrackInstance,           LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_skipForward]     = {&actions::SkipForwardInstance,        LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_skipBack]        = {&actions::SkipBackInstance,           LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_playPause]       = {&actions::PlayPauseInstance,          LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_stop]            = {&actions::StopInstance,               LedPolicy::Momentary};
        rActionMap[controlBoardButtons::k_cover]           = {&actions::CoverViewInstance,          LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_repeat]          = {&actions::RepeatInstance,             LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_toggleRandom]    = {&actions::ToggleRandomInstance,       LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_toggleDac]       = {&actions::ToggleDacInstance,          LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_toggleDisplay]   = {&actions::ToggleDisplayInstance,      LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_toggleMeter]     = {&actions::ToggleMeterDisplayInstance, LedPolicy::Toggle};
        rActionMap[controlBoardButtons::k_rotaryEventLeft] = {&actions::RotaryEventInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::k_rotaryEventRight]= {&actions::RotaryEventInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::k_cycleBrightness] = {&actions::CycleBrightnessInstance,    LedPolicy::Momentary};
    }
}