#include <Arduino.h>
#include "tft.h"
#include "log.h"
#include "filesystem.h"

// if we have SMOOTH_FONT defined, we need this becauese tft_espi.h defines FS_NO_GLOBALS
// using fs::File;

TFT_eSPI tft = TFT_eSPI();

void beginTFT()
{
    logInfo("Init TFT\n");
    // Initialise the TFT screen
    tft.init();
#ifdef ESP32_DMA
    // TODO: DMA seems to be f'd up.  maybe it needs a double buffer?
    tft.initDMA(true);
#endif // ESP32_DMA

    ledcSetup(TFT_BL_PWM, 5000, 8); // 5KHz, 8bit resolution
    ledcAttachPin(TFT_BL, TFT_BL_PWM);
    tftBacklight(255);

    // Set the rotation before we calibrate
    tft.setRotation(0);

    // Clear the screen
    tft.fillScreen(TFT_BLACK);


    logInfo("beginTFT: Done\n");
}

void tftBacklight(int value)
{
    ledcWrite(TFT_BL_PWM, value);
}

#define CALIBRATION_FILE "/TouchCalData2"

void tftTouchCalibrate(bool repeat)
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (repeat)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !repeat) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}
