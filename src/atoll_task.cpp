#include "atoll_task.h"

using namespace Atoll;

void Task::taskStart(float freq,
                     uint32_t stack,
                     int8_t priority,
                     int8_t core) {
    if (0 < freq) _taskFreq = freq;
    if (0 < stack) taskSetStack(stack);
    if (0 <= priority) _taskPriority = (uint8_t)priority;
    if (0 <= core) _taskCore = (uint8_t)core;
    if (taskRunning()) {
        log_w("stopping %s", taskName());
        taskStop();
    }
    _taskSetFreqAndDelay(_taskFreq);
    if (_taskFreq <= 0.0f) {
        log_e("not creating task for %s because freq is %.2fHz", taskName(), _taskFreq);
        return;
    }
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
          taskName(), _taskFreq, taskDelayMs(), _taskStack, heap, xPortGetFreeHeapSize());
}

bool Task::taskRunning() {
    return NULL != taskHandle;
}

void Task::taskStop() {
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

void Task::taskSetFreq(const float freq) {
    if (freq <= 0.0f) {
        // log_d("%s not setting zero freq", taskName());
        return;
    }
    _taskSetFreqAndDelay(freq);
    _taskDebug("taskSetFreq");
}

void Task::taskSetDelay(const uint16_t delayMs) {
    if (0 == delayMs) {
        log_w("%s setting zero delay", taskName());
    }
    _taskSetFreqAndDelay(delayMs);
    _taskDebug("taskSetDelay");
}

void Task::taskSetStack(const uint32_t stack) {
    if (0 == stack) {
        log_e("%s not setting zero stack", taskName());
        return;
    }
    log_d("%s setting stack %d", taskName(), stack);
    _taskStack = stack;
}

ulong Task::taskGetNextWakeTimeMs() {
    return pdTICKS_TO_MS(_taskNextWakeTime);
}

int Task::taskGetLowestStackLevel() {
    return NULL == taskHandle
               ? -1
               : (int)uxTaskGetStackHighWaterMark(taskHandle);
}

void Task::loop(){};
Task::Task(){};
Task::~Task() {}

void Task::_taskLoop(void *p) {
    Task *thisPtr = (Task *)p;
    thisPtr->_taskLastWakeTime = xTaskGetTickCount();
    for (;;) {
        // log_d("%s loop %d", thisPtr->taskName(), thisPtr->_taskLoopCount);
        if (1 != thisPtr->_taskLoopCount) {  // skip loop #1
            ulong start = millis();
            thisPtr->loop();
            ulong loopTime = millis() - start;
            /*
            if (thisPtr->_taskDelay < loopTime) {
                log_w("%s loop time %dms > taskDelay %dms (taskFreq %.2fHz)",
                      thisPtr->taskName(), loopTime,
                      pdTICKS_TO_MS(thisPtr->_taskDelay), thisPtr->_taskFreq);
            }
            */
        }
        thisPtr->_taskLastWakeTime = xTaskGetTickCount();
        thisPtr->_taskNextWakeTime = thisPtr->_taskLastWakeTime + thisPtr->_taskDelay;
        // log_i("%s delaying for %dms", thisPtr->taskName(),
        //       pdTICKS_TO_MS(thisPtr->_taskNextWakeTime) - millis());
        if (thisPtr->_taskLoopCount)  // don't delay first loop
            xTaskDelayUntil(&(thisPtr->_taskLastWakeTime), thisPtr->_taskDelay);
        thisPtr->_taskLoopCount++;
    }
}

void Task::_taskSetFreqAndDelay(const float freq) {
    _taskFreq = 0.0001f < freq ? freq : 0.0001f;
    if (_taskFreq <= 0.0001f) {
        log_w("%s freq is zero, setting max delay", taskName());
        _taskDelay = (TickType_t)UINT32_MAX;
    } else
        _taskDelay = pdMS_TO_TICKS(1000 / _taskFreq);
    _taskSetDelay();
}

void Task::_taskSetFreqAndDelay(const TickType_t delayTicks) {
    _taskDelay = 0 < delayTicks ? delayTicks : 1;
    ulong ms = taskDelayMs();
    if (0 == ms) {
        log_w("%s delay is zero ms, setting max freq", taskName());
        _taskFreq = 1000.0f;
    } else {
        // log_i("freq: %f", 1000.0 / ms);
        _taskFreq = 1000.0 / ms;
    }
    //_taskAbortDelay();
    _taskSetDelay();
}

void Task::_taskSetFreqAndDelay(const uint16_t delayMs) {
    _taskSetFreqAndDelay(pdMS_TO_TICKS(delayMs));
}

void Task::_taskSetDelay() {
    TickType_t t = pdMS_TO_TICKS(millis());
    TickType_t lastWake = t < _taskLastWakeTime ? 0 : t - _taskLastWakeTime;
    TickType_t delay = _taskDelay <= lastWake ? 1 : _taskDelay - lastWake;
    _taskNextWakeTime = _taskLastWakeTime + delay;
    if (nullptr != taskHandle) {
        // log_i("delaying %s for %dms", taskName(), pdTICKS_TO_MS(delay));
        xTaskDelayUntil(&_taskLastWakeTime, delay);
    }
}

void Task::_taskAbortDelay() {
    if (NULL == taskHandle) {
        // log_w("taskHandle is null");
        return;
    }
    _taskDebug("_taskAbortDelay");
    xTaskAbortDelay(taskHandle);
}

void Task::_taskDebug(const char *tag) {
    // log_d("%s %s freq: %.2f, delay: %d", tag, taskName(), _taskFreq, taskDelayMs());
}

ulong Task::taskDelayMs() {
    return pdTICKS_TO_MS(_taskDelay);
}