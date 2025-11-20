#include "controlSytemHelpers.hpp"
#include "actionProcessor.hpp"

#define TAG "CTRL_HELPERS"
namespace controlSystem
{

const char* getCommandNameForPin(uint8_t pin)
{

    ESP_LOGI(TAG, "Getting command name for pin: %u", pin); 
    size_t index = pin ;

    if (index >= NUM_COMMANDS)
        return "UNKNOWN";

    return commandConfigs[index].logTag;   // or .label or .id — whichever is correct
}

} // namespace controlSystem