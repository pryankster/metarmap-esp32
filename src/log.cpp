#include <Arduino.h>

#include "log.h"

#define DEBUG_PFX "[DEBUG]: "
#define INFO_PFX ""
#define WARN_PFX "[WARNING]: "
#define ERROR_PFX "[ERROR]: "

/* this code taken from ESP32 print library -- may allocate memory if string is > 64 chars */
static void vlogMessagef(const char *pfx, const char *fmt, va_list arg)
{
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

    Serial.print(pfx);
    /*
    char *p = temp;
    while (len--) {
        if (*p == '\n') Serial.println("");
        Serial.print((char)*p++);
    }
    */
    Serial.print(temp);

    if(temp != loc_buf){
        free(temp);
    }
}

#if 0
static void logMessage(const char *pfx, const char *msg)
{
    Serial.print(pfx);
    Serial.println(msg);
}

void logDebug(const char *msg)
{
    logMessage(DEBUG_PFX, msg);
}

void logDebug(const String &msg)
{
    logMessage(DEBUG_PFX,msg.c_str());
}

void logInfo(const char *msg)
{
    logMessage(INFO_PFX, msg);
}

void logInfo(const String &msg)
{
    logMessage(INFO_PFX,msg.c_str());
}

void logWarn(const char *msg)
{
    logMessage(WARN_PFX, msg);
}

void logWarn(const String &msg)
{
    logMessage(WARN_PFX,msg.c_str());
}

void logError(const char *msg)
{
    logMessage(ERROR_PFX, msg);
}

void logError(const String &msg)
{
    logMessage(ERROR_PFX,msg.c_str());
}

void logDebug(const String &fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(DEBUG_PFX, fmt.c_str(), ap);
    va_end(ap);
}

void logInfo(const String &fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(INFO_PFX, fmt.c_str(), ap);
    va_end(ap);
}

void logWarn(const String &fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(WARN_PFX, fmt.c_str(), ap);
    va_end(ap);
}

void logError(const String &fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(ERROR_PFX, fmt.c_str(), ap);
    va_end(ap);
}
#endif

extern "C" void logDebug(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(DEBUG_PFX, fmt, ap);
    va_end(ap);
}

extern "C" void logInfo(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(INFO_PFX, fmt, ap);
    va_end(ap);
}

extern "C" void logWarn(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(WARN_PFX, fmt, ap);
    va_end(ap);
}

extern "C" void logError(const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vlogMessagef(ERROR_PFX, fmt, ap);
    va_end(ap);
}