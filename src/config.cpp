#include <Arduino.h>
#include <ArduinoJson.h> // Using ArduinoJson to read and write config files

#include "webserver.h"
#include "config.h"
#include "log.h"
#include "filesystem.h"

#if 0
void configmode()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  Serial.println("[INFO]: Entering Config Mode");
  tft.println("Connecting to Wifi...");

  if (String(wificonfig.ssid) == "YOUR_WIFI_SSID" || String(wificonfig.password) == "YOUR_WIFI_PASSWORD") // Still default
  {
    tft.println("WiFi Config still set to default! Starting as AP.");
    Serial.println("[WARNING]: WiFi Config still set to default! Configurator started as AP.");
    startDefaultAP();
    tft.println("Started as AP because WiFi settings are still set to default.");
    tft.println("To configure, connect to 'FreeTouchDeck' with password 'defaultpass'");
    tft.println("Then go to http://freetouchdeck.local");
    tft.print("The IP is: ");
    tft.println(WiFi.softAPIP());
    drawSingleButton(140, 180, 200, 80, generalconfig.menuButtonColour, TFT_WHITE, "Restart");
    return;
  }

  if (String(wificonfig.ssid) == "FAILED" || String(wificonfig.password) == "FAILED" || String(wificonfig.wifimode) == "FAILED") // The wificonfig.json failed to load
  {
    tft.println("WiFi Config Failed to load! Starting as AP.");
    Serial.println("[WARNING]: WiFi Config Failed to load! Configurator started as AP.");
    startDefaultAP();
    tft.println("Started as AP because WiFi settings failed to load.");
    tft.println("To configure, connect to 'FreeTouchDeck' with password 'defaultpass'");
    tft.println("Then go to http://freetouchdeck.local");
    tft.print("The IP is: ");
    tft.println(WiFi.softAPIP());
    drawSingleButton(140, 180, 200, 80, generalconfig.menuButtonColour, TFT_WHITE, "Restart");
    return;
  }

  if (strcmp(wificonfig.wifimode, "WIFI_STA") == 0)
  {
    if(!startWifiStation()){
      startDefaultAP();
      Serial.println("[WARNING]: Could not connect to AP, so started as AP.");
      tft.println("Started as AP because WiFi connection failed.");
      tft.println("To configure, connect to 'FreeTouchDeck' with password 'defaultpass'");
      tft.println("Then go to http://freetouchdeck.local");
      tft.print("The IP is: ");
      tft.println(WiFi.softAPIP());
      drawSingleButton(140, 180, 200, 80, generalconfig.menuButtonColour, TFT_WHITE, "Restart");
    }
    else
    {
      tft.println("Started as STA and in config mode.");
      tft.println("To configure:");
      tft.println("http://freetouchdeck.local");
      tft.print("The IP is: ");
      tft.println(WiFi.localIP());
      drawSingleButton(140, 180, 200, 80, generalconfig.menuButtonColour, TFT_WHITE, "Restart");
    }

  }
  else if (strcmp(wificonfig.wifimode, "WIFI_AP") == 0)
  {
    startWifiAP();
    tft.println("Started as AP and in config mode.");
    tft.println("To configure:");
    tft.println("http://freetouchdeck.local");
    tft.print("The IP is: ");
    tft.println(WiFi.softAPIP());
    drawSingleButton(140, 180, 200, 80, generalconfig.menuButtonColour, TFT_WHITE, "Restart");
  }
}
#endif

bool saveWifiConfig(const Wificonfig &wifi)
{
    if (wifi.wifimode != "WIFI_STA" && wifi.wifimode != "WIFI_AP")
    {
        logError("WiFi Mode not supported. Try WIFI_STA or WIFI_AP.\n");
        return false;
    }

    DynamicJsonDocument doc(384);

    JsonObject wificonfigobject = doc.to<JsonObject>();

    wificonfigobject["ssid"] = wifi.ssid;
    wificonfigobject["password"] = wifi.password;
    wificonfigobject["wifimode"] = wifi.wifimode;
    wificonfigobject["wifihostname"] = wifi.hostname;
    wificonfigobject["attempts"] = wifi.attempts;
    wificonfigobject["attemptdelay"] = wifi.attemptdelay;

    FILESYSTEM.remove(WIFI_CONFIG_FILE);
    File file = FILESYSTEM.open(WIFI_CONFIG_FILE, "w");
    if (serializeJsonPretty(doc, file) == 0)
    {
        logError("Failed to write WiFi config\n");
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool loadWifiConfig(Wificonfig &wifi)
{
    if (!FILESYSTEM.exists(WIFI_CONFIG_FILE))
    {
        logError("Config file '" WIFI_CONFIG_FILE "' not found!\n");
        return false;
    }
    File configfile = FILESYSTEM.open(WIFI_CONFIG_FILE);

    DynamicJsonDocument doc(256);

    DeserializationError error = deserializeJson(doc, configfile);

    strlcpy(wifi.ssid, doc["ssid"] | "FAILED", sizeof(wifi.ssid));
    strlcpy(wifi.password, doc["password"] | "FAILED", sizeof(wifi.password));
    strlcpy(wifi.wifimode, doc["wifimode"] | "FAILED", sizeof(wifi.wifimode));
    strlcpy(wifi.hostname, doc["wifihostname"] | "freetouchdeck", sizeof(wifi.hostname));

    uint8_t attempts = doc["attempts"] | 10 ;
    wifi.attempts = attempts;

    uint16_t attemptdelay = doc["attemptdelay"] | 500 ;
    wifi.attemptdelay = attemptdelay;

    configfile.close();

    if (error)
    {
        logError("deserializeJson() error: %s\n", error.c_str());
        return false;
    }

    return true;
}

/*
bool checkfile(const char *filename)
{
  if (!FILESYSTEM.exists(filename))
  {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(1, 3);
    tft.setTextFont(2);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.printf("%s not found!\n\n", filename);
    tft.setTextSize(1);
    tft.printf("If this has happend after confguration, the data on the ESP may \nbe corrupted.");
    return false;
  }
  else
  {
    return true;
  }
}
*/