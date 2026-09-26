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
        // Deletes the queue and task resources when the dispatcher is torn down.
        ~ButtonEventQueue();

        /// Creates the queue and spawns the dispatch task.
        /// Must be called after rDispatcher is fully constructed.
        // Starts the queue and worker task that drains raw button and rotary events.
        bool start(ControlBoardInputDispatcher &rDispatcher);

        /// Deletes the task and queue; safe to call more than once.
        // Stops the worker task and releases the queue used for event dispatch.
        void stop();

        /// Sends an event from ISR/callback context. Never blocks.
        // Queues a raw button or rotary event for the background dispatcher thread.
        bool enqueue(const ButtonEvent &event, const char *p_eventName);

        // Queues a button press event for the given input pin.
        void enqueuePress(uint8_t pin);
        // Queues a button release event for the given input pin.
        void enqueueRelease(uint8_t pin);
        // Queues a signed rotary movement event from the encoder.
        void enqueueRotary(int movement);

    private:
        // FreeRTOS trampoline that calls the queue task loop for the dispatcher.
        static void taskEntry(void *pvParam);
        // Runs the background task that converts queued events into action dispatches.
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
