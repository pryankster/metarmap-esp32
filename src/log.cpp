#include <Arduino.h>
#if MULTI_TASK
#include <semphr.h>
#endif // MULTI_TASK

#include "log.h"
#include "mutex.h"

static int log_lvl = LOG_DEBUG;

// TODO: use thread local storage to have different log levels per task?

extern "C" void setLogLevel(int lvl) 
{
    if (lvl < LOG_DEBUG || lvl > LOG_LVL_MAX) return;
    log_lvl = lvl;
}

extern "C" int getLogLevel() 
{
    return log_lvl;
}

const char *log_prefix[LOG_LVL_MAX] = {
    "[DEBUG]: ",     // DEBUG
    "",                 // INFO
    "[WARNING]: ", // WARN
    "[ERROR]: ",    // ERROR
    "[FATAL]: ",    // FATAL
};

char *lastMsg = NULL;
int msgCount = 0;

static void logHeader()
{
#if MULTI_TASK
    Serial.print("[");
    Serial.print(pcTaskGetName(NULL));
    Serial.print("@");
    Serial.print(xPortGetCoreID());
    Serial.print("]: ");
#endif
}

/* this code taken from ESP32 print library -- may allocate memory if string is > 64 chars */
static void vlogMessagef(int lvl, const char *fmt, va_list arg)
{
    bool showCount = false;
    bool showMessage = true;
    if (lvl != LOG_RAW && lvl < log_lvl) return;

    char loc_buf[64];
    char * temp = loc_buf;
    va_list copy;
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), fmt, copy);
    va_end(copy);

    if(len <= 0) return;

    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) return;
        len = vsnprintf(temp, len+1, fmt, arg);
    }

#if MULTI_TASK
    _lock();
    if (lastMsg == NULL || strlen(temp) > strlen(lastMsg)) {
        if (lastMsg != NULL) free(lastMsg); lastMsg = NULL;
        lastMsg = (char*)malloc(strlen(temp)+1);
        lastMsg[0] = 0;
    }
    // same message as last time?  just count it.
    if (strcmp(lastMsg,temp)==0) {
        msgCount++;
        // don't show repeated message.
        showMessage = false;

        // put out repeat messages every 100 ...
        if (msgCount >= 100) showCount = true;
    } else {
        // new, different message, flush existing count.
        if (msgCount > 0) showCount = true;
        strcpy(lastMsg,temp);
    }

    if (showCount) {
        logHeader();
        Serial.print(msgCount);
        Serial.println(" repeats");
        msgCount = 0;   // reset message count.
    }
#endif
    if (showMessage) {
        if (lvl != LOG_RAW) {
            logHeader();
            Serial.print(log_prefix[lvl]);
        }
        Serial.print(temp);
    }

done:
#if MULTI_TASK
    _release();
#endif

    if(temp != loc_buf){
        free(temp);
    }
}

extern "C" void logDebug(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_DEBUG, fmt, ap);
    va_end(ap);
}

extern "C" void logInfo(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_INFO, fmt, ap);
    va_end(ap);
}

extern "C" void logWarn(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_WARN, fmt, ap);
    va_end(ap);
}

extern "C" void logError(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_ERROR, fmt, ap);
    va_end(ap);
}

extern "C" void logFatal(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_FATAL, fmt, ap);
    va_end(ap);
    vTaskSuspend(NULL);
}

extern "C" void logRaw(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(LOG_RAW, fmt, ap);
    va_end(ap);
}