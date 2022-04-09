#ifndef __atoll_task_h
#define __atoll_task_h

#include "xtensa/core-macros.h"
#include <Arduino.h>

#include "atoll_log.h"

#ifndef ATOLL_TASK_DEFAULT_FREQ
#define ATOLL_TASK_DEFAULT_FREQ 5
#endif

#ifndef ATOLL_TASK_DEFAULT_STACK
#define ATOLL_TASK_DEFAULT_STACK 4096
#endif

#ifndef ATOLL_TASK_DEFAULT_PRIORITY
#define ATOLL_TASK_DEFAULT_PRIORITY 1
#endif

#ifndef ATOLL_TASK_DEFAULT_CORE
#define ATOLL_TASK_DEFAULT_CORE 1
#endif

namespace Atoll {

class Task {
   public:
    TaskHandle_t taskHandle = NULL;
    float taskFreq = 10;  // desired task frequency in Hz

    virtual const char *taskName() = 0;

    virtual void taskStart(float freq = ATOLL_TASK_DEFAULT_FREQ,
                           uint32_t stack = ATOLL_TASK_DEFAULT_STACK,
                           uint8_t priority = ATOLL_TASK_DEFAULT_PRIORITY,
                           uint8_t core = ATOLL_TASK_DEFAULT_CORE) {
        if (taskRunning()) taskStop();
        taskFreq = freq;
        _taskSetDelayFromFreq();
        log_i("Starting task '%s' at %.2fHz (delay: %dms), stack %d",
              taskName(), freq, _xTaskDelay, stack);
        BaseType_t err = xTaskCreatePinnedToCore(
            _taskLoop,
            taskName(),
            stack,
            this,
            priority,
            &taskHandle,
            core);
        if (pdPASS != err)
            log_e("Failed to start task '%s', error %d", taskName(), err);
        // else
        //     log_i("[Task] Started %s", taskName());
    }

    virtual bool taskRunning() {
        return NULL != taskHandle;
    }

    virtual void taskStop() {
        if (NULL != taskHandle) {
            log_i("Stopping %s", taskName());
            vTaskDelete(taskHandle);
        }
        taskHandle = NULL;
    }

    virtual void taskSetFreq(const float freq) {
        log_i("%s new freq: %.2f", taskName(), freq);
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
    TickType_t _xTaskDelay;

    static void _taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->_xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            ulong start = millis();
            thisPtr->loop();
            ulong loopTime = millis() - start;
            if (thisPtr->_xTaskDelay < loopTime)
                log_w("%s loop time %dms > taskDelay %dms (taskFreq %.2fHz)",
                      thisPtr->taskName(), loopTime, thisPtr->_xTaskDelay, thisPtr->taskFreq);
            vTaskDelayUntil(&(thisPtr->_xLastWakeTime), thisPtr->_xTaskDelay);
        }
    }

    void _taskSetDelayFromFreq() {
        _xTaskDelay = pdMS_TO_TICKS(1000 / taskFreq);
    }
};

}  // namespace Atoll

#endif
