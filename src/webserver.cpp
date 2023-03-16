#include <Arduino.h>
#include "webserver.h"
#include "log.h"

AsyncWebServer webserver(80);

void startWebServer()
{
    webserver.begin();
    logInfo("Webserver started");
}

void stopWebServer()
{
    logWarn("Webserver stop not implemented.");
}