#ifndef _H_TFT_
#define _H_TFT_

//#include <TFT_eSPI.h>
#include "TFT_parallel8_ili9488.hpp"

// Set the width and height of your screen here:
// #define SCREEN_WIDTH 480
// #define SCREEN_HEIGHT 320
// height is width, because we're rotated!
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

// extern TFT_eSPI tft;
extern LGFX_Parallel8_ILI9488 tft;

void beginTFT();

void tftBacklight(int value);
void tftTouchCalibrate(bool repeat);

#endif // _H_TFT_
