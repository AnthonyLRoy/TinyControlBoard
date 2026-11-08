#include "app/ButtonEventQueue.hpp"
#include "app/ControlBoardInputDispatcher.hpp"
#include "esp_log.h"

namespace controlSystem
{
    ButtonEventQueue::~ButtonEventQueue()
    {
        stop();
    }

    // Starts the FreeRTOS queue and worker task that serializes button and rotary input events.
    bool ButtonEventQueue::start(ControlBoardInputDispatcher &rDispatcher,
                                 std::function<void(uint16_t, uint16_t)> onRemoteCommand)
    {
        mp_dispatcher = &rDispatcher;
        m_onRemoteCommand = std::move(onRemoteCommand);

        m_queue = xQueueCreate(k_depth, sizeof(ButtonEvent));
        if (!m_queue)
        {
            ESP_LOGE(k_logTag, "Failed to create button event queue");
            return false;
        }

        if (xTaskCreate(taskEntry, "action_task", k_taskStackSize, this, k_taskPriority, &m_taskHandle) != pdPASS)
        {
            ESP_LOGE(k_logTag, "Failed to create action task");
            vQueueDelete(m_queue);
            m_queue = nullptr;
            return false;
        }

        return true;
    }

    // Stops the worker task and releases the queue so the queue can be torn down cleanly.
    void ButtonEventQueue::stop()
    {
        if (m_taskHandle)
        {
            vTaskDelete(m_taskHandle);
            m_taskHandle = nullptr;
        }
        if (m_queue)
        {
            vQueueDelete(m_queue);
            m_queue = nullptr;
        }
    }
    // Queues a raw event for later processing by the dispatcher thread, dropping it gracefully if the queue is full.
    bool ButtonEventQueue::enqueue(const ButtonEvent &event, const char *p_eventName)
    {
        if (!m_queue)
        {
            ESP_LOGW(k_logTag, "Dropping %s event because queue is not initialized", p_eventName);
            return false;
        }

        if (xQueueSend(m_queue, &event, 0) == pdTRUE)
        {
            return true;
        }

        const uint32_t droppedEvents = m_droppedEvents.fetch_add(1) + 1;
        if ((droppedEvents % 16U) == 1U)
        {
            ESP_LOGW(k_logTag, "Button event queue full, dropped %lu events (latest=%s)",
                     static_cast<unsigned long>(droppedEvents), p_eventName);
        }
        return false;
    }

    bool ButtonEventQueue::enqueuePress(uint8_t pin)
    {
        return enqueue({ButtonEventType::Press, pin, 0}, "press");
    }

    void ButtonEventQueue::enqueueRelease(uint8_t pin)
    {
        enqueue({ButtonEventType::Release, pin, 0}, "release");
    }

    void ButtonEventQueue::enqueueRotary(int movement)
    {
        enqueue({ButtonEventType::Rotary, 0, static_cast<int8_t>(movement)}, "rotary");
    }

    bool ButtonEventQueue::enqueueRemoteCommand(uint16_t command, uint16_t releaseMs)
    {
        return enqueue({ButtonEventType::RemoteCommand, 0, 0, command, releaseMs}, "remote command");
    }

    void ButtonEventQueue::taskEntry(void *pvParam)
    {
        static_cast<ButtonEventQueue *>(pvParam)->run();
    }

    // Runs the background task that converts queued events into physical-button and rotary action dispatches.
    void ButtonEventQueue::run()
    {
        ButtonEvent event{};
        while (true)
        {
            if (xQueueReceive(m_queue, &event, portMAX_DELAY) == pdTRUE)
            {
                switch (event.type)
                {
                case ButtonEventType::Press:
                    mp_dispatcher->handleButtonPressed(event.buttonId);
                    break;
                case ButtonEventType::Release:
                    mp_dispatcher->handleButtonReleased(event.buttonId);
                    break;
                case ButtonEventType::Rotary:
                    mp_dispatcher->handleRotaryMovement(event.rotaryDelta);
                    break;
                case ButtonEventType::RemoteCommand:
                    m_onRemoteCommand(event.command, event.releaseMs);
                    break;
                }
            }
        }
    }
}
