#include <Arduino.h>
#include <unistd.h>
#include <sys/stat.h>
#include "vfs_fs.h"
#include "webserver.h"
#include "log.h"

AsyncWebServer webserver(80);

static void get(AsyncWebServerRequest *req)
{
    struct stat sb;
}

static const char *wifiAuthModes[] = {
    "none",
    "WEP",              /**< authenticate mode : WEP */
    "WPA",          /**< authenticate mode : WPA_PSK */
    "WPA2_PSK",         /**< authenticate mode : WPA2_PSK */
    "WPA_WPA2_PSK",     /**< authenticate mode : WPA_WPA2_PSK */
    "WPA2_ENTERPRISE",  /**< authenticate mode : WPA2_ENTERPRISE */
    "WPA3_PSK",         /**< authenticate mode : WPA3_PSK */
    "WPA2_WPA3_PSK",    /**< authenticate mode : WPA2_WPA3_PSK */
    "WAPI_PSK",         /**< authenticate mode : WAPI_PSK */
};

static void scan(AsyncWebServerRequest *request)
{
  String json = "[";
  int n = WiFi.scanComplete();
  if(n == -2){
    logInfo("Start WiFi Scan...\n");
    WiFi.scanNetworks(true);
  } else if(n) {
    logInfo("Report WiFi Scan: results=%d\n",n);
    for (int i = 0; i < n; ++i){
      if(i) json += ",";
      json += "{";
      json += "\"rssi\":"+String(WiFi.RSSI(i));
      json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
      // json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
      // json += ",\"channel\":"+String(WiFi.channel(i));
      int t = (int) WiFi.encryptionType(i);
      if (t < 0 || t > sizeof(wifiAuthModes) / sizeof(wifiAuthModes[0])) { 
        t = -1;
      }
      json += ",\"secure\": \""+String( t < 0 ? "unknown" : wifiAuthModes[t]) + "\" ";
      json += "}";
    }
    WiFi.scanDelete();
    if(WiFi.scanComplete() == -2){
      logInfo("Re-start wifi scan...\n");
      WiFi.scanNetworks(true);
    }
  }
  json += "]";
  request->send(200, "application/json", json);
}

static void notFound(AsyncWebServerRequest *req)
{
    logInfo("web server: 404 - %s\n", req->url().c_str());
    req->send(404,"text/plain", "Not found");
}

void startWebServer()
{
    webserver.begin();
    logInfo("Webserver started");

    webserver.on("/api/scan", HTTP_GET,  scan );
    webserver.serveStatic( "/data", fs::VFS, "/sd/data" );
    webserver.serveStatic( "/", fs::VFS, "/sd/web" );
    webserver.onNotFound(notFound);
}

void stopWebServer()
{
    logWarn("Webserver stop not implemented.");
}