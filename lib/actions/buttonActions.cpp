
#include "buttonActions.hpp"
#include <esp_timer.h>
namespace actions
{

    ToggleTrack toggleTrackInstance;
    VolumeUp volumeUpInstance;
    PowerButton PowerButtonInstance;

    actionResponse ToggleTrack::execute(bool buttonMode)
    {
        static bool state = false;
        state = !state;
        ESP_LOGI("ButtonActions", "ToggleTrack: %s", state ? "ON" : "OFF");
        actionResponse response;
        response.command = CMD_NEXT_TRACK;
        return response;
    }

    actionResponse VolumeUp::execute(bool buttonMode)
    {
        static int volume = 0;
        if (volume < 100)
            volume += 5;
        ESP_LOGI("ButtonActions", "VolumeUp: %d", volume);
        actionResponse response;
        response.command = CMD_NEXT_TRACK;
        return response;
    }

    actionResponse PowerButton::execute(bool buttonMode)
    {
        static int64_t lastActionTime = esp_timer_get_time();
        static int64_t  timeDifferenceInSeconds =0;
        actionResponse response;

        if (buttonMode == true)  // button is pressed
        {
            lastActionTime = esp_timer_get_time();
        }
        else
        {
            timeDifferenceInSeconds =  (esp_timer_get_time() - lastActionTime) / 1000000;

            if (timeDifferenceInSeconds < 5  )  // put in sleep Mode 
            {  
                 

            };
            
        }

        response.command  = CMD_SYS_POWER;
        return response;
    }


    // Implement more actions here
}
