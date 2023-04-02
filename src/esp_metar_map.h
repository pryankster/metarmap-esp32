#ifndef _H_ESP_METAR_MAP_
#define _H_ESP_METAR_MAP_

#include "irkeyboard.h"


#define IR_KEY_BRIGHTNESS 'B'
#define IR_KEY_DOWN LV_KEY_DOWN
#define IR_KEY_UP LV_KEY_UP
#define IR_KEY_LEFT LV_KEY_LEFT
#define IR_KEY_RIGHT LV_KEY_RIGHT
#define IR_KEY_POWER 'P'
#define IR_KEY_SETTINGS 'S'
#define IR_KEY_ENTER LV_KEY_ENTER
#define IR_KEY_TIME 'T'
#define IR_KEY_BACK LV_KEY_ESC
#define IR_KEY_WEATHER 'W'

extern IRKeyboard irkb;

#define NUM_BRIGHT_LEVELS 4
extern int ledBrightLevels[NUM_BRIGHT_LEVELS];
extern int tftBrightLevels[NUM_BRIGHT_LEVELS];

#endif // _H_ESP_METAR_MAP_