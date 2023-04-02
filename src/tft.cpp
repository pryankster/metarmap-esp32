#include <Arduino.h>
#include "tft.h"
#include "ft6236.h"
#include "log.h"
#include "filesystem.h"

// if we have SMOOTH_FONT defined, we need this becauese tft_espi.h defines FS_NO_GLOBALS
// using fs::File;

LGFX tft;

extern void colors(int);

void beginTFT()
{
    logInfo("Init TFT\n");
    // Initialise the TFT screen
    pinMode(TFT_PIN_CS, OUTPUT);
    digitalWrite(TFT_PIN_CS, LOW);

    tft.init();

    // Set the rotation before we calibrate
    tft.setRotation(0);

    // Clear the screen
    tft.fillScreen(TFT_BLACK);

    // configure PWM for, and turn on backlight.
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    ledcSetup(TFT_BL_PWM, 5000, 8); // 5KHz, 8bit resolution
    ledcAttachPin(TFT_BL, TFT_BL_PWM);
    tftBacklight(255);

    logInfo("beginTFT: Done\n");
}

void tftBacklight(int value)
{
    ledcWrite(TFT_BL_PWM, value);
    digitalWrite(TFT_BL, value ? HIGH : LOW);
}