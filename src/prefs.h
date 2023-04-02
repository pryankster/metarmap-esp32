#ifndef _H_PREFS_
#define _H_PREFS_

#include "filesystem.h"

struct saveprefs_t {
  uint8_t magic;
  uint8_t version;
  uint8_t current_airport;
  uint8_t brightness;

#define PREFS_SCREEN_METAR 'M'
#define PREFS_SCREEN_CLOCK 'C'
  uint8_t default_screen;

#define PREFS_NUM_FAVORITES (10)
  uint8_t favorite_airport[PREFS_NUM_FAVORITES];
  uint8_t metric;
  uint8_t end_magic;
};
#define PREFS_FILE "/spiffs/prefs.bin"
#define PREFS_VERSION 1
#define PREFS_MAGIC 'P'
#define PREFS_END_MAGIC 'X'

extern saveprefs_t prefs;
extern bool prefs_dirty;

void save_prefs(bool force);
bool load_prefs();

#endif // _H_PREFS_
