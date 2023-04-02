#ifndef _H_MUTEX_
#define _H_MUTEX_

#if MULTI_TASK
#include <semphr.h>
#endif // MULTI_TASK

// include this once when a compilation unit needs a global mutext
// it declares a static 'mutex' and 'mutex_buffer', along with 
// inline static functions to _lock() and _release() the mutex.

#if MULTI_TASK
static StaticSemaphore_t mutex_buffer;
static SemaphoreHandle_t mutex = xSemaphoreCreateMutexStatic(&mutex_buffer);
#endif // MULTI_TASK

static inline bool _lock(TickType_t ticks = portMAX_DELAY)
{
#if MULTI_TASK
    return xSemaphoreTakeRecursive(mutex, ticks) ? true : false;
#else
    return true;
#endif
}

static inline void _release()
{
#if MULTI_TASK
    xSemaphoreGiveRecursive(mutex);
#endif
}

#endif // _H_MUTEX_