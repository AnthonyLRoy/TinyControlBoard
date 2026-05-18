#include "app/ControlBoardActionRegistry.hpp"

#include "input/actions/buttonActions.hpp"

namespace controlSystem
{
    void ControlBoardActionRegistry::populate(ControlBoardInputDispatcher::ActionMap &rActionMap) const
    {
        rActionMap[controlBoardButtons::kPower]           = {&actions::PowerButtonInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::kPrevTrack]       = {&actions::PreviousTrackInstance,      LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kNextTrack]       = {&actions::NextTrackInstance,           LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kSkipForward]     = {&actions::SkipForwardInstance,        LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kSkipBack]        = {&actions::SkipBackInstance,           LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kPlayPause]       = {&actions::PlayPauseInstance,          LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kStop]            = {&actions::StopInstance,               LedPolicy::Momentary};
        rActionMap[controlBoardButtons::kCover]           = {&actions::CoverViewInstance,          LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kRepeat]          = {&actions::RepeatInstance,             LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kToggleRandom]    = {&actions::ToggleRandomInstance,       LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kToggleDac]       = {&actions::ToggleDacInstance,          LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kToggleDisplay]   = {&actions::ToggleDisplayInstance,      LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kToggleMeter]     = {&actions::ToggleMeterDisplayInstance, LedPolicy::Toggle};
        rActionMap[controlBoardButtons::kRotaryEventLeft] = {&actions::RotaryEventInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::kRotaryEventRight]= {&actions::RotaryEventInstance,        LedPolicy::None};
        rActionMap[controlBoardButtons::kCycleBrightness] = {&actions::CycleBrightnessInstance,    LedPolicy::Momentary};
    }
}