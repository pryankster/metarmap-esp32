#ifndef _H_WEBSERVER_
#define _H_WEBSERVER_

#include <AsyncTCP.h>          //Async Webserver support header
#include <ESPAsyncWebServer.h> //Async Webserver support header

extern AsyncWebServer webserver;

void startWebServer(void);
void stopWebServer(void);

#endif // _H_WEBSERVER_