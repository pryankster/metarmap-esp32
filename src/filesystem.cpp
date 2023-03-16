#include <Arduino.h>
#include "filesystem.h"
#include "log.h"

bool beginFilesystem()
{
    if (!FILESYSTEM.begin())
    {
        logError("FILESYSTEM initialisation failed!\n");
    return false;
    }
    logInfo("FILESYSTEM initialised.\n");

    // show free space.
    logInfo("Free Space: %d bytes\n", FILESYSTEM.totalBytes() - FILESYSTEM.usedBytes());

    return true;
}
