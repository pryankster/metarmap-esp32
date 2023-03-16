#ifndef _H_CONFIG_
#define _H_CONFIG_

#include "wifi_config.h"

#ifndef WIFI_CONFIG_FILE
#define WIFI_CONFIG_FILE "/config/wificonfig.json"
#endif

bool saveWifiConfig(const Wificonfig &wifi);
bool loadWifiConfig(Wificonfig &wifi);

bool checkfile(const char *filename);

#endif // _H_CONFIG_