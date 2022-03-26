#ifndef __atoll_task_h
#define __atoll_task_h

#include "xtensa/core-macros.h"
#include <Arduino.h>

#define TASK_DEFAULT_CORE 1

namespace Atoll {

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    uint16_t taskFreq = 10;     // desired task frequency in Hz
    uint32_t taskStack = 4096;  // task stack size in bytes
    uint8_t taskPriority = 1;
    uint8_t taskCore = TASK_DEFAULT_CORE;

    virtual const char *taskName() = 0;

    virtual void taskStart() {
        taskStart(taskName(), taskFreq, taskStack, taskPriority);
    }
    virtual void taskStart(uint16_t freq) {
        taskStart(taskName(), freq, taskStack, taskPriority);
    }
    virtual void taskStart(const char *name) {
        taskStart(name, taskFreq, taskStack, taskPriority);
    }
    virtual void taskStart(const char *name, uint16_t freq) {
        taskStart(name, freq, taskStack, taskPriority);
    }
    virtual void taskStart(const char *name, uint16_t freq, uint32_t stack) {
        taskStart(name, freq, stack, taskPriority);
    }
    virtual void taskStart(const char *name, uint16_t freq, uint32_t stack, uint8_t priority) {
        if (taskRunning()) taskStop();
        taskFreq = freq;
        _taskSetDelayFromFreq();
        log_i("Starting task '%s' at %dHz (delay: %dms), stack %d",
              name, freq, _xTaskDelay, stack);
        BaseType_t err = xTaskCreatePinnedToCore(
            _taskLoop,
            name,
            stack,
            this,
            priority,
            &taskHandle,
            taskCore);
        if (pdPASS != err)
            log_e("Failed to start task '%s', error %d", name, err);
        // else
        //     log_i("[Task] Started %s", name);
    }

    virtual bool taskRunning() {
        return NULL != taskHandle;
    }

    virtual void taskStop() {
        if (NULL != taskHandle) {
            log_i("[Task] Stopping %s", taskName());
            vTaskDelete(taskHandle);
        }
        taskHandle = NULL;
    }

    virtual void taskSetFreq(const uint16_t freq) {
        log_i("%s new freq: %d", taskName(), freq);
        taskFreq = freq;
        _taskSetDelayFromFreq();
    }

    virtual int taskGetLowestStackLevel() {
        return NULL == taskHandle
                   ? -1
                   : (int)uxTaskGetStackHighWaterMark(taskHandle);
    }

    virtual void loop(){};
    Task(){};
    virtual ~Task() {}

   private:
    TickType_t _xLastWakeTime;
    TickType_t _xTaskDelay;  // TODO unit is ticks but actually =ms??

    static void _taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->_xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            vTaskDelayUntil(&(thisPtr->_xLastWakeTime), thisPtr->_xTaskDelay);
            thisPtr->loop();
        }
    }

    void _taskSetDelayFromFreq() {
        //_xTaskDelay = 1000 / taskFreq / portTICK_PERIOD_MS;
        _xTaskDelay = pdMS_TO_TICKS(1000 / taskFreq);
    }
};

}  // namespace Atoll

#endif
