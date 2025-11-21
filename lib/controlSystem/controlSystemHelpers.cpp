#include "controlSytemHelpers.hpp"
#include "actionProcessor.hpp"

#define TAG "CTRL_HELPERS"
namespace controlSystem
{

    const char *getCommandNameForPin(uint8_t pin)
    {
        size_t index = pin;

        if (index >= NUM_COMMANDS) {
            ESP_LOGW(TAG, "Pin %u -> UNKNOWN", pin);
            return "UNKNOWN";
        }

        ESP_LOGI(TAG, "Pin %u -> %s -> id=0x%04lX", pin, commandConfigs[index].logTag, commandConfigs[index].commandId);

        return commandConfigs[index].logTag;
    }

} // namespace controlSystem