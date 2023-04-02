#ifndef _H_NETWORK_CONFIG
#define _H_NETWORK_CONFIG

#include <stdint.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "/sd/wifi.json"
#endif

struct NetworkConfig
{
  char ssid[64];
  char password[64];
  char hostname[64];
  uint8_t flags;           
#define NETCFG_WIFI_AP (1<<0)  // if set, use AP mode, else use station mode
#define NETCFG_OVR_HOSTNAME (1<<1)   // if set, don't accept hostname from DHCP
  uint32_t ip;              // IP Address (network byte order!) (0 == use DHCP)
  uint32_t gw;              // gateway IP Address (network byte order!) (0 == use DHCP)
  uint32_t mask;            // network mas.  0 == use DHCP
  uint32_t dns[2];          // DNS address (x 2, network byte order, 0==use DHCP)
  uint8_t attempts;
  uint16_t attemptdelay;
  char ntp_server[64];      // i.e.: pool.ntp.org
  int  gmt_offset;          // in seconds. (PT= -8 = 60*60*-8)
  bool  use_dst;            // Use daylight saving time?
};

// bool saveNetworkConfig(const NetworkConfig &cfg);
bool loadNetworkConfig(NetworkConfig &cfg);

void networkBegin();
void networkLoop();

#endif // _H_NETWORK_CONFIG