#include <Arduino.h>
#include <WiFi.h> // Wifi support
#include <ESPmDNS.h> // DNS functionality

#include "wifi_config.h"
#include "log.h"


void startDNS(Wificonfig &wifi)
{
    MDNS.begin(wifi.hostname);
    MDNS.addService("http", "tcp", 80);
}

// Start as WiFi station
bool startWifiStation(const Wificonfig &wifi)
{
    logInfo("Connecting to %s", wifi.ssid);
    if (String(WiFi.SSID()) != String(wifi.ssid))
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi.ssid, wifi.password);
        uint8_t attempts = wifi.attempts;
        while (WiFi.status() != WL_CONNECTED)
        {
            if(attempts == 0) {
                WiFi.disconnect();
                Serial.println("");
                return false;
            }
            delay(wifi.attemptdelay);
            Serial.print(".");
            attempts--;
        }
    }
    logInfo("Connected!  IP address: %s\n", WiFi.localIP());

    startDNS(wifi);

    return true;
}

// Start as WiFi AP
void startWifiAP(Wificonfig &wifi)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(wifi.ssid, wifi.password);
    logInfo("Access Point Started!  IP address: %s", WiFi.softAPIP());

    MDNS.begin(wifi.hostname);
    MDNS.addService("http", "tcp", 80);
}