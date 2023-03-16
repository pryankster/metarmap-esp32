#ifndef _H_WIFI_CONFIG
#define _H_WIFI_CONFIG

struct Wificonfig
{
  char ssid[64];
  char password[64];
  char wifimode[9];
  char hostname[64];
  uint8_t attempts;
  uint16_t attemptdelay;
};

void startDNS(const Wificonfig &wifi);
bool startWifiStation(const Wificonfig &wifi);
void startWifiAP(const Wificonfig &wifi);

#endif // _H_WIFI_CONFIG
