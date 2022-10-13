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
            log_w("stopping %s", taskName());
            taskStop();
        }
        _taskSetFreqAndDelay(_taskFreq);
        uint32_t heap = xPortGetFreeHeapSize();
        BaseType_t err = xTaskCreatePinnedToCore(
            _taskLoop,
            taskName(),
            _taskStack,
            this,
            _taskPriority,
            &taskHandle,
            _taskCore);
        if (pdPASS != err) {
            log_e("Failed to start task '%s', error %d", taskName(), err);
            return;
        }
        log_i("Started task '%s' at %.2fHz (delay: %dms), stack: %d, heap: %d before, %d after",
              taskName(), _taskFreq, _taskDelayMs(), _taskStack, heap, xPortGetFreeHeapSize());
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
        if (freq <= 0.0f) {
            log_e("not setting zero freq");
            return;
        }
        _taskSetFreqAndDelay(freq);
        _taskDebug("taskSetFreq");
    }

    virtual void taskSetDelay(const uint16_t delayMs) {
        if (0 == delayMs) {
            log_w("setting zero delay");
        }
        _taskSetFreqAndDelay(delayMs);
        _taskDebug("taskSetDelay");
    }

    ulong taskGetNextWakeTimeMs() {
        return pdTICKS_TO_MS(_taskNextWakeTime);
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
    TickType_t _taskLastWakeTime = 0;
    TickType_t _taskNextWakeTime = 0;
    TickType_t _taskDelay = 0;

    static void _taskLoop(void *p) {
        Task *thisPtr = (Task *)p;
        thisPtr->_taskLastWakeTime = xTaskGetTickCount();
        for (;;) {
            // log_i("%s", thisPtr->taskName());
            ulong start = millis();
            thisPtr->loop();
            ulong loopTime = millis() - start;
            if (thisPtr->_taskDelay < loopTime)
                // log_w("%s loop time %dms > taskDelay %dms (taskFreq %.2fHz)",
                //       thisPtr->taskName(), loopTime,
                //       pdTICKS_TO_MS(thisPtr->_taskDelay), thisPtr->_taskFreq);
                thisPtr->_taskLastWakeTime = xTaskGetTickCount();
            thisPtr->_taskNextWakeTime = thisPtr->_taskLastWakeTime + thisPtr->_taskDelay;
            // log_i("%s delaying for %dms", thisPtr->taskName(),
            //       pdTICKS_TO_MS(thisPtr->_taskNextWakeTime) - millis());
            vTaskDelayUntil(&(thisPtr->_taskLastWakeTime), thisPtr->_taskDelay);
        }
    }

    void _taskSetFreqAndDelay(const float freq) {
        _taskFreq = 0.0001f < freq ? freq : 0.0001f;
        if (_taskFreq <= 0.0001f) {
            log_w("%s freq is zero, setting max delay", taskName());
            _taskDelay = (TickType_t)UINT32_MAX;
        } else
            _taskDelay = pdMS_TO_TICKS(1000 / _taskFreq);
        _taskAbortDelay();
    }

    void _taskSetFreqAndDelay(const TickType_t delayTicks) {
        _taskDelay = 0 < delayTicks ? delayTicks : 1;
        ulong ms = _taskDelayMs();
        if (0 == ms) {
            log_w("%s delay is zero ms, setting max freq", taskName());
            _taskFreq = 1000.0f;
        } else
            _taskFreq = 1000.0 / ms;
        _taskAbortDelay();
    }

    void _taskSetFreqAndDelay(const uint16_t delayMs) {
        _taskSetFreqAndDelay(pdMS_TO_TICKS(delayMs));
    }

    void _taskAbortDelay() {
        if (NULL == taskHandle) {
            // log_w("taskHandle is null");
            return;
        }
        _taskDebug("_taskAbortDelay");
        xTaskAbortDelay(taskHandle);
    }

    void _taskDebug(const char *tag = "") {
        // log_i("%s %s freq: %.2f, delay: %d", tag, taskName(),
        //       _taskFreq, _taskDelayMs());
    }

    ulong _taskDelayMs() {
        return pdTICKS_TO_MS(_taskDelay);
    }
};

}  // namespace Atoll

#endif
