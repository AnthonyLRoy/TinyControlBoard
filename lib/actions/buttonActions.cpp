
#include "ButtonActions.hpp"

namespace actions
{


    ToggleTrack toggleTrackInstance;
    VolumeUp volumeUpInstance;

    actionResponse ToggleTrack::execute()
    {
        static bool state = false;
        state = !state;
        ESP_LOGI("ButtonActions", "ToggleTrack: %s", state ? "ON" : "OFF");
        actionResponse response;
        response.messageCreated = true;
        return response;
    }

    actionResponse VolumeUp::execute()
    {
        static int volume = 0;
        if (volume < 100)
            volume += 5;
        ESP_LOGI("ButtonActions", "VolumeUp: %d", volume);
        actionResponse response;
        response.messageCreated = true;
        return response;
    }

    // Implement more actions here
}
