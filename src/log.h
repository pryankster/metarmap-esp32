#ifndef _H_LOG_
#define _H_LOG_

/*
void logDebug(String msg);
void logInfo(String msg);
void logWarn(String msg);
void logError(String msg);

void logDebug(const char *msg);
void logInfo(const char *msg);
void logWarn(const char *msg);
void logError(const char *msg);

void logDebug(String fmt, ... );
void logInfo(String fmt, ... );
void logWarn(String fmt, ... );
void logError(String fmt, ... );
*/

extern "C" {
    void logDebug(const char *fmt, ... );
    void logInfo(const char *fmt, ... );
    void logWarn(const char *fmt, ... );
    void logError(const char *fmt, ... );
}

#endif // _H_LOG 