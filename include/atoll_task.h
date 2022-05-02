#ifndef __atoll_task_h
#define __atoll_task_h

#include "xtensa/core-macros.h"
#include <Arduino.h>

#include "atoll_log.h"

#ifndef ATOLL_TASK_DEFAULT_FREQ
#define ATOLL_TASK_DEFAULT_FREQ 1.0
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

    virtual const char *taskName() = 0;

    virtual void taskStart(float freq = -1,
                           uint32_t stack = 0,
                           int8_t priority = -1,
                           int8_t core = -1) {
        if (0 < freq) _taskFreq = freq;
        if (stack != 0) _taskStack = stack;
        if (0 <= priority) _taskPriority = (uint8_t)priority;
        if (0 <= core) _taskCore = (uint8_t)core;
        if (taskRunning()) {
            log_w("%s is running", taskName());
            taskStop();
        }
        _taskSetDelayFromFreq();
        log_i("Starting task '%s' at %.2fHz (delay: %dms), stack %d",
              taskName(), _taskFreq, _xTaskDelay, _taskStack);
        BaseType_t err = xTaskCreatePinnedToCore(
            _taskLoop,
            taskName(),
            _taskStack,
            this,
            _taskPriority,
            &taskHandle,
            _taskCore);
        if (pdPASS != err)
            log_e("Failed to start task '%s', error %d", taskName(), err);
        // else
        //     log_i("[Task] Started %s", taskName());
    }

    virtual bool taskRunning() {
        return NULL != taskHandle;
    }

    virtual void taskStop() {
        if (!taskRunning()) {
            log_i("%s is not running", taskName());
            return;
        }
        log_i("Stopping %s", taskName());
        if (NULL == taskHandle) {
            log_w("%s taskHandle is null", taskName());
            return;
        }
        TaskHandle_t t = taskHandle;
        taskHandle = NULL;
        vTaskDelete(t);
    }

    virtual void taskSetFreq(const float freq) {
        if (freq <= 0) return;
        log_i("%s new freq: %.2f", taskName(), freq);
        _taskFreq = freq;
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

   protected:
    float _taskFreq = ATOLL_TASK_DEFAULT_FREQ;
    uint32_t _taskStack = ATOLL_TASK_DEFAULT_STACK;
    uint8_t _taskPriority = ATOLL_TASK_DEFAULT_PRIORITY;
    uint8_t _taskCore = ATOLL_TASK_DEFAULT_CORE;
    TickType_t _xLastWakeTime = 0;
    TickType_t _xTaskDelay = 0;

    static void _taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->_xLastWakeTime = xTaskGetTickCount();
        for (;;) {
            ulong start = millis();
            thisPtr->loop();
            ulong loopTime = millis() - start;
            if (thisPtr->_xTaskDelay < loopTime)
                log_w("%s loop time %dms > taskDelay %dms (taskFreq %.2fHz)",
                      thisPtr->taskName(), loopTime, thisPtr->_xTaskDelay, thisPtr->_taskFreq);
            vTaskDelayUntil(&(thisPtr->_xLastWakeTime), thisPtr->_xTaskDelay);
        }
    }

    void _taskSetDelayFromFreq() {
        _xTaskDelay = pdMS_TO_TICKS(1000 / _taskFreq);
    }
};

}  // namespace Atoll

#endif
