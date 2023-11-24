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
                           int8_t core = -1);
    virtual bool taskRunning();
    virtual void taskStop();
    virtual void taskSetFreq(const float freq);
    virtual void taskSetDelay(const uint16_t delayMs);
    virtual ulong taskGetNextWakeTimeMs();
    virtual int taskGetLowestStackLevel();
    virtual void loop();

    Task();
    virtual ~Task();

   protected:
    float _taskFreq = ATOLL_TASK_DEFAULT_FREQ;
    uint32_t _taskStack = ATOLL_TASK_DEFAULT_STACK;
    uint8_t _taskPriority = ATOLL_TASK_DEFAULT_PRIORITY;
    uint8_t _taskCore = ATOLL_TASK_DEFAULT_CORE;
    TickType_t _taskLastWakeTime = 0;
    TickType_t _taskNextWakeTime = 0;
    TickType_t _taskDelay = 0;
    ulong _taskLoopCount = 0;

    static void _taskLoop(void *p);
    void _taskSetFreqAndDelay(const float freq);
    void _taskSetFreqAndDelay(const TickType_t delayTicks);
    void _taskSetFreqAndDelay(const uint16_t delayMs);
    void _taskSetDelay();
    void _taskAbortDelay();
    void _taskDebug(const char *tag = "");
    ulong _taskDelayMs();
};

}  // namespace Atoll

#endif
