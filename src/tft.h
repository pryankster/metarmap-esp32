#ifndef _H_TFT_
#define _H_TFT_

#include <TFT_eSPI.h>

// Set the width and height of your screen here:
// #define SCREEN_WIDTH 480
// #define SCREEN_HEIGHT 320
// height is width, because we're rotated!
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 480

extern TFT_eSPI tft;

void beginTFT();

void tftBacklight(int value);
void tftTouchCalibrate(bool repeat);

#endif // _H_TFT_
