
#include "ButtonActions.hpp"

namespace actions
{

    ToggleTrack toggleTrackInstance;
    VolumeUp volumeUpInstance;

    bool ToggleTrack::execute()
    {
        static bool state = false;
        state = !state;
        ESP_LOGI("ButtonActions", "ToggleTrack: %s", state ? "ON" : "OFF");
        return state;
    }

    bool VolumeUp::execute()
    {
        static int volume = 0;
        if (volume < 100)
            volume += 5;
        ESP_LOGI("ButtonActions", "VolumeUp: %d", volume);
        return true;
    }

    // Implement more actions here
}
