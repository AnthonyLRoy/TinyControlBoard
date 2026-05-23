#pragma once

#include "app/ButtonEvent.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

namespace controlSystem
{
    class ControlBoardInputDispatcher;

    /// RAII wrapper around the FreeRTOS queue and task that receives raw
    /// button/rotary events from ISR context and dispatches them to a
    /// ControlBoardInputDispatcher on a dedicated task.
    class ButtonEventQueue
    {
    public:
        ~ButtonEventQueue();

        /// Creates the queue and spawns the dispatch task.
        /// Must be called after rDispatcher is fully constructed.
        bool start(ControlBoardInputDispatcher &rDispatcher);

        /// Deletes the task and queue; safe to call more than once.
        void stop();

        /// Sends an event from ISR/callback context. Never blocks.
        bool enqueue(const ButtonEvent &event, const char *p_eventName);

        void enqueuePress(uint8_t pin);
        void enqueueRelease(uint8_t pin);
        void enqueueRotary(int movement);

    private:
        static void taskEntry(void *pvParam);
        void run();

        static constexpr uint8_t k_depth = 16;
        static constexpr uint32_t k_taskStackSize = 4096;
        static constexpr UBaseType_t k_taskPriority = 5;
        static constexpr const char *k_logTag = "ButtonEventQueue";

        QueueHandle_t m_queue = nullptr;
        TaskHandle_t m_taskHandle = nullptr;
        ControlBoardInputDispatcher *mp_dispatcher = nullptr;
        uint32_t m_droppedEvents = 0;
    };
}
