#ifndef _H_AIRPORTS_
#define _H_AIRPORTS_

#include <stdint.h>
#include <time.h>

// t is a float [0 - 1.0], but result is cast back to int.
#define INT_LERP(a, b, t) (int)( (((b)-(a)) * (t)) + (a) )

#define CLOUD_INVALID (-1)
#define CLOUD_SKC (0)
#define CLOUD_CLR (1)
#define CLOUD_CAVOK (2)
#define CLOUD_FEW (3)
#define CLOUD_SCT (4)
#define CLOUD_BKN (5)
#define CLOUD_OVC (6)
#define CLOUD_OVX (7)

struct clouds_t {
    int sky_cover;   // CLOUD_XXX above
    int altitude;
};

#define WX_COND_VFR (0)
#define WX_COND_MVFR (1)
#define WX_COND_IFR (2)
#define WX_COND_LIFR (3)

#define WX_COND_MAX (4)

#define WX_CLOUD_RECORDS (4)

struct airport_t {
  char *name;
  char *weather;
  char *full_name;
  float elevation;
  uint8_t wx_cond;   // WX_COND_XXX above
  float x;
  float y;
  int wind_dir;
  int wind_speed;
  int wind_gust;
  float vis;
  int cloud_idx;
  clouds_t clouds[WX_CLOUD_RECORDS];
  float altimiter;
  float press_alt;
  float temp_c;
  float dew_c;
  char report_time[8];  // 'DDHHMMZ\0'
  char *metar;      // metar text string.
  time_t last_metar; // when did we last check METAR for this airport 
  bool lightning;   // if true, lightning is present.
  int last_flash;    // last lightning flash, in ticks()
  bool valid_metar; // if true, we successfully parsed the last metar.
};

void airportsBegin();
void airportsLoop();

#define AIRPORT_BLINK_RATE (250)
#define AIRPORT_BLINK_TIME (5*1000)

extern int num_airports;
int load_airports();
int airport_index(const char *name);
airport_t *get_airport(const char *name);
airport_t *get_airport(int index);
bool show_airport(int n);
bool next_airport(char dir);
void leds_off(void);
void airport_blink(bool enable, int how_long = AIRPORT_BLINK_TIME);

bool set_airport_kv(airport_t *airport, const char *key, const char *val);

int get_airport_brightness();
void set_airport_brightness(int val);

#define CtoF(c) (((c)*9.0/5.0)+32.0)
#define PA(alt, elev) ((29.92 - (alt)) * 1000.0 + (elev))
#define ISA(e) (15.0 - (((e)/1000.0)*2.0))
#define DA(pa,oat,isa) ((pa) + (120.0 * ((oat) - (isa))))

#endif // _H_AIRPORTS_