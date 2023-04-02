#ifndef _H_LOG_
#define _H_LOG_

extern "C" int getLogLevel();
extern "C" void setLogLevel(int lvl);

#define LOG_RAW (-1)
#define LOG_DEBUG 0
#define LOG_INFO 1
#define LOG_WARN 2
#define LOG_ERROR 3
#define LOG_FATAL 4
#define LOG_LVL_MAX (5)

extern "C" {
    void logDebug(const char *fmt, ... );
    void logInfo(const char *fmt, ... );
    void logWarn(const char *fmt, ... );
    void logError(const char *fmt, ... );
    void logRaw(const char *fmt, ... );
}

#endif // _H_LOG 