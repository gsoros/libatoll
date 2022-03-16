#ifndef __atoll_task_h
#define __atoll_task_h

#include "xtensa/core-macros.h"
#include <Arduino.h>

#define TASK_NAME_LEN 32
#define TASK_DEFAULT_CORE 1

namespace Atoll {

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    char taskName[TASK_NAME_LEN];
    uint16_t taskFreq = 10;     // desired task frequency in Hz
    uint32_t taskStack = 4096;  // task stack size in bytes
    uint8_t taskPriority = 1;
    uint8_t taskCore = TASK_DEFAULT_CORE;

    void taskStart() {
        taskStart(taskName, taskFreq, taskStack, taskPriority);
    }
    void taskStart(uint16_t freq) {
        taskStart(taskName, freq, taskStack, taskPriority);
    }
    void taskStart(const char *name) {
        taskStart(name, taskFreq, taskStack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq) {
        taskStart(name, freq, taskStack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq, uint32_t stack) {
        taskStart(name, freq, stack, taskPriority);
    }
    void taskStart(const char *name, uint16_t freq, uint32_t stack, uint8_t priority) {
        if (taskRunning()) taskStop();
        strncpy(taskName, name, TASK_NAME_LEN);
        taskFreq = freq;
        _taskSetDelayFromFreq();
        log_i("[Task] Starting %s at %dHz (delay: %dms), stack %d",
              name, freq, _xTaskDelay, stack);
        BaseType_t err = xTaskCreatePinnedToCore(
            _taskLoop,
            taskName,
            stack,
            this,
            priority,
            &taskHandle,
            taskCore);
        if (pdPASS != err)
            log_e("Failed to start task %s, error %d", taskName, err);
        // else
        //     log_i("[Task] Started %s", name);
    }

    bool taskRunning() {
        return NULL != taskHandle;
    }

    void taskStop() {
        if (NULL != taskHandle) {
            log_i("[Task] Stopping %s", taskName);
            vTaskDelete(taskHandle);
        }
        taskHandle = NULL;
    }

    void taskSetFreq(const uint16_t freq) {
        taskFreq = freq;
        _taskSetDelayFromFreq();
    }

    int taskGetLowestStackLevel() {
        return NULL == taskHandle
                   ? -1
                   : (int)uxTaskGetStackHighWaterMark(taskHandle);
    }

    virtual void loop(){};
    Task() {}   // avoid undefined reference to 'vtable...
    ~Task() {}  // avoid undefined reference to 'vtable...

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