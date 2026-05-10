#include "app/ControlBoardActionRegistry.hpp"

#include "input/actions/buttonActions.hpp"

namespace controlSystem
{
    void ControlBoardActionRegistry::populate(ControlBoardInputDispatcher::ActionMap &rActionMap) const
    {
        rActionMap[controlBoardButtons::kPower] = &actions::PowerButtonInstance;
        rActionMap[controlBoardButtons::kPrevTrack] = &actions::PreviousTrackInstance;
        rActionMap[controlBoardButtons::kNextTrack] = &actions::NextTrackInstance;
        rActionMap[controlBoardButtons::kSkipForward] = &actions::SkipForwardInstance;
        rActionMap[controlBoardButtons::kSkipBack] = &actions::SkipBackInstance;
        rActionMap[controlBoardButtons::kPlayPause] = &actions::PlayPauseInstance;
        rActionMap[controlBoardButtons::kStop] = &actions::StopInstance;
        rActionMap[controlBoardButtons::kCover] = &actions::CoverViewInstance;
        rActionMap[controlBoardButtons::kRepeat] = &actions::RepeatInstance;
        rActionMap[controlBoardButtons::kMenuSelect] = &actions::MenuSelectInstance;
        rActionMap[controlBoardButtons::kToggleDac] = &actions::ToggleDacInstance;
        rActionMap[controlBoardButtons::kToggleDisplay] = &actions::ToggleDisplayInstance;
        rActionMap[controlBoardButtons::kToggleMeter] = &actions::ToggleMeterDisplayInstance;
        rActionMap[controlBoardButtons::kRotaryEventLeft] = &actions::RotaryEventInstance;
        rActionMap[controlBoardButtons::kRotaryEventRight] = &actions::RotaryEventInstance;
        rActionMap[controlBoardButtons::kCycleBrightness] = &actions::CycleBrightnessInstance;
    }
}