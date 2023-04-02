#include <Arduino.h>
#include <ArduinoJson.h> // Using ArduinoJson to read and write config files

#include "webserver.h"
#include "network_config.h"
#include "log.h"
#include "filesystem.h"
#include "clock.h"
#include "metar.h"
#include "msgbox.h"

#include <WiFi.h> // Wifi support
#include <ESPmDNS.h> // DNS functionality

struct wifi_state {
    int state;
#define WIFI_IDLE (0)
#define WIFI_DISCONNECTED (1)
#define WIFI_CONNECTING (2)
#define WIFI_RECONNECT_DELAY (3)
#define WIFI_CONNECTED (4)
    int attempts;
    int timer;
};

static wifi_state state;
NetworkConfig cfg;

bool loadNetworkConfig(NetworkConfig &cfg)
{
    int fd = open(CONFIG_FILE,O_RDONLY);
    if (fd < 0) {
        logError("Failed to open network config '%s'", CONFIG_FILE);
        return false;
    }
    int size = lseek(fd, 0, SEEK_END);
    char *buffer = (char*) calloc(size+1, 1);
    if (buffer == NULL) {
        close(fd);
        logError("Failed to allocate %d bytes for network config", size+1);
        return false;
    }
    lseek(fd,0,SEEK_SET);
    int n = read(fd, buffer, size);
    close(fd);
    if (n != size) {
        free(buffer);
        logError("Failed to read network config '%s'", CONFIG_FILE);
        return false;
    }
    DynamicJsonDocument doc(n);
    deserializeJson(doc,buffer);
    free(buffer);

    logInfo("----- JSON ----\n");
    serializeJson(doc,Serial);
    logInfo("------END JSON ----\n");

    if (doc.containsKey("ssid")) {
        strncpy(cfg.ssid, (const char*) doc["ssid"], sizeof(cfg.ssid)-1);
        logInfo("SSID: %s\n", cfg.ssid);
    }
    if (doc.containsKey("password")) {
        strncpy(cfg.password, (const char*) doc["password"], sizeof(cfg.password)-1);
        logInfo("PASS: %s\n", cfg.password);
    }
    if (doc.containsKey("hostname")) {
        strncpy(cfg.hostname, (const char*) doc["hostname"], sizeof(cfg.hostname)-1);
        logInfo("HOST: %s\n", cfg.hostname);
    }
    if (doc.containsKey("ntpServer")) {
        strncpy(cfg.ntp_server, (const char*) doc["ntpServer"], sizeof(cfg.ntp_server)-1);
        logInfo("NTP:  %s\n", cfg.ntp_server);
    }
    if (doc.containsKey("gmtOffset")) {
        cfg.gmt_offset = ((int)doc["gmtOffset"]) * (60*60);
        logInfo("GMT:  %d\n", cfg.gmt_offset);
    }
    if (doc.containsKey("useDst")) {
        cfg.use_dst = doc["useDst"];
        logInfo("DST:  %d\n", cfg.use_dst);
    }

  // uint32_t ip;              // IP Address (network byte order!) (0 == use DHCP)
  // uint32_t gw;              // gateway IP Address (network byte order!) (0 == use DHCP)
  // uint32_t mask;            // network mas.  0 == use DHCP
  // uint32_t dns[2];          // DNS address (x 2, network byte order, 0==use DHCP)
  // uint8_t attempts;
  // uint16_t attemptdelay;
  // char ntp_server[64];      // i.e.: pool.ntp.org
    return true;
}

// Start as WiFi station
void networkBegin()
{
    static NetworkConfig defaultConfig = {
      "Immobilehome-24",
      "6504838649",
      "metarsfo.local",
      0,
      0, // ip
      0, // gw
      0, // mask
      { 0, 0 },  // dns
      10, // attempts
      5000, // attempt delay
      "pool.ntp.org",       // ntp server
      60*60*-8,     // pacific time, -8 HRS from GMT
      true,
    };

    cfg = defaultConfig;

    loadNetworkConfig(cfg);

    // TODO: load network configuration from sd card.

    logInfo("networkBegin()\n");
    state.state = WIFI_DISCONNECTED;
    state.timer = 0;
}

void network_retry()
{
    state.attempts--;
    if (state.attempts <= 0) {
        WiFi.disconnect();
        logError("WiFi: did not connect, out of attempts.\n");
        showMessage(10000, "Could not connect WiFi");
        state.state = WIFI_IDLE;
    }
    logInfo("WiFi: retry in 3 sec (attempts=%d).\n", cfg.attempts);
    showMessage(3000, "Retrying WiFi ... ");
    state.timer = 3000;
}

static uint32_t last_ms;
void networkLoop()
{
    uint32_t cur_ms = millis();
    int32_t elapsed = cur_ms - last_ms;
    last_ms = cur_ms;

    if (state.timer > 0) {
        state.timer -= elapsed;
        if (state.timer > 0) return;
        // clobber underflow.
        state.timer = 0;
    }

    // run state machine.
    switch(state.state) {
    case WIFI_IDLE:
        logInfo("WIFI_IDLE\n");
        state.timer = 1000;
        break;
    case WIFI_DISCONNECTED:
        // TODO: don't do anything unless cfg is valid.
        logInfo("Connecting to %s\n", cfg.ssid);
        showMessage(3000, "Connecting WiFi ... ");
        // TODO: connect in AP mode, too?
        WiFi.mode(WIFI_STA);
        WiFi.begin(cfg.ssid, cfg.password);
        state.attempts = cfg.attempts;
        state.timer = 0;
        state.state = WIFI_CONNECTING;
        break;
    case WIFI_CONNECTING:
        { 
            auto status = WiFi.status();
            switch(WiFi.status()) {
                case WL_CONNECTED:
                    {
                        showMessagef(3000, "WiFi connected to network %s", cfg.ssid);
                        auto hn = WiFi.getHostname();
                        logInfo("WIFI_CONNECTING: WiFi connected, start MDNS DHCP hn: '%s', cfg hn: '%s'\n", hn, cfg.hostname);
                        state.state = WIFI_CONNECTED;
                        MDNS.begin(cfg.hostname);
                        logInfo("MDNS hostname: %s\n", cfg.hostname);
                        MDNS.addService("http", "tcp", 80);
                        logInfo("Setting clock with NTP from %s\n", cfg.ntp_server);
                        configTime(cfg.gmt_offset, cfg.use_dst ? 3600 : 0, cfg.ntp_server);
                        setEnableClock(true);
                        startWebServer();
                    }
                    break;
                case WL_DISCONNECTED:
                    // still waiting for a connection
                    showMessage(3000, "WiFi disconnected!");
                    logInfo("WIFI_CONNECTING: waiting for connection\n");
                    state.timer = 100;
                    stopWebServer();
                    break;
                case WL_NO_SSID_AVAIL:
                    logInfo("WIFI_CONNECTING: No SSID available.\n");
                    network_retry();
                    break;
                case WL_CONNECTION_LOST:
                    showMessage(3000, "WiFi connection lost!");
                    logInfo("WIFI_CONNECTING: WiFi connection lost?.\n");
                    network_retry();
                    break;
                case WL_CONNECT_FAILED:
                    showMessage(3000, "WiFi connection failed!");
                    logInfo("WIFI_CONNECTING: WiFi connection failed.\n");
                    network_retry();
                    break;
                case WL_IDLE_STATUS:
                    logInfo("WIFI_CONNECTING: WiFi idle state\n");
                    break;
                default:
                    state.state = WIFI_IDLE;
                    state.timer = 1000;
                    logError("WIFI_CONNECTING: some other state? %d ", status );
                    break;
            }
        }
        break;
    case WIFI_CONNECTED:
        {
            if (WiFi.status() != WL_CONNECTED) {
                showMessage(3000, "WiFi disconnected!");
                logError("WiFi disconnected. retry in 10 sec\n");
                // TODO: do auto-reconnection here.
                state.state = WIFI_DISCONNECTED;
                state.timer = 10000;
            }
            state.timer = 30000;
            auto rssi = WiFi.RSSI();
            auto ip = WiFi.localIP();
            auto gw = WiFi.gatewayIP();
            auto nmask = WiFi.subnetMask();
            logInfo("WIFI_CONNECTED: RSSI=%d, IP=%s, GW=%s, MASK=%s\n", rssi, ip.toString(), gw.toString(), nmask.toString());
        }
        break;  
    default:
        logInfo("WIFI: Unknown state?  %d", state.state);
        state.timer = 1000;
        break;
    }
}

/*
// Start as WiFi AP
void startWifiAP(NetworkConfig &cfg)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg.ssid, cfg.password);
    logInfo("Access Point Started!  IP address: %s", WiFi.softAPIP());

    MDNS.begin(cfg.hostname);
    MDNS.addService("http", "tcp", 80);
}
*/

/*
bool saveNetworkConfig(const NetworkConfig &cfg)
{
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
*/