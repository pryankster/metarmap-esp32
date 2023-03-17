#include <Arduino.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>

#include <FastLED.h>
#include "tft.h"
#include "filesystem.h"
#include "webserver.h"
#include "config.h" // WiFi, config 'n stuff.
#include "wifi_config.h"
#include "log.h"
#include "drawhelper.h"
#include "bmp.h"
#include "irkeyboard.h"
#include "touch.h"
#include "gui.h"
#include "clock.h"

const char *versionString = "0.0.0";

/*
#include "Fonts/Emmett__14pt7b.h"
#include "Fonts/Emmett__24pt7b.h"
#define SMALL_FONT &Emmett__14pt7b
#define LARGE_FONT &Emmett__24pt7b
*/

FASTLED_USING_NAMESPACE

#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
#define NUM_LEDS    3
CRGB leds[NUM_LEDS];

#define BRIGHTNESS          96

// Create instances of the structs
Wificonfig wificonfig;

void ledsOff(void);

// this must remain sorted by 'cmd', so that bsearch works.
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

const IRKeyMap keyMap[] = {
    { 0x00, 0x07, IR_KEY_BRIGHTNESS, IRKB_NO_REPEAT }, // ROW: 2, COL: 0; brightness (lightbulb)
    { 0x00, 0x08, '7', 0x00 }, // ROW: 5, COL: 0; 7 (prs)
    { 0x00, 0x09, IR_KEY_SETTINGS, IRKB_NO_REPEAT }, // ROW: 2, COL: 2; settings (gear)
    { 0x00, 0x0c, '4', 0x00 }, // ROW: 4, COL: 0; 4 (ghi)
    { 0x00, 0x0d, '3', 0x00 }, // ROW: 3, COL: 2; 3 (def)
    { 0x00, 0x15, IR_KEY_DOWN, IRKB_REPEAT }, // ROW: 2, COL: 1; down arrow
    { 0x00, 0x16, '1', 0x00 }, // ROW: 3, COL: 0; 1 
    { 0x00, 0x18, '5', 0x00 }, // ROW: 4, COL: 1; 5 (jkl)
    { 0x00, 0x19, '2', 0x00 }, // ROW: 3, COL: 1; 2 (abc)
    { 0x00, 0x1c, '8', 0x00 }, // ROW: 5, COL: 1; 8 (tuv)
    { 0x00, 0x40, IR_KEY_ENTER, IRKB_NO_REPEAT }, // ROW: 1, COL: 2; enter
    { 0x00, 0x42, IR_KEY_WEATHER, IRKB_NO_REPEAT }, // ROW: 6, COL: 0; weather (cloud)
    { 0x00, 0x43, IR_KEY_RIGHT, IRKB_REPEAT }, // ROW: 1, COL: 1; right arrow
    { 0x00, 0x44, IR_KEY_LEFT, IRKB_REPEAT }, // ROW: 1, COL: 0; left arrow
    { 0x00, 0x45, IR_KEY_POWER, IRKB_NO_REPEAT }, // ROW: 0, COL: 0; power
    { 0x00, 0x46, IR_KEY_UP, IRKB_REPEAT }, // ROW: 0, COL: 1; up
    { 0x00, 0x47, IR_KEY_BACK, IRKB_NO_REPEAT }, // ROW: 0, COL: 2; back
    { 0x00, 0x4a, IR_KEY_TIME, IRKB_NO_REPEAT }, // ROW: 6, COL: 2; time (clock)
    { 0x00, 0x52, '0', 0x00 }, // ROW: 6, COL: 1; 0 (qz)
    { 0x00, 0x5a, '9', 0x00 }, // ROW: 5, COL: 2; 9 (wxy)
    { 0x00, 0x5e, '6', 0x00 }, // ROW: 4, COL: 2; 6 (mno)
};
const int numKeys = (sizeof(keyMap) / sizeof(keyMap[0]));
IRKeyboard irkb(IR_RECEIVE_PIN, keyMap, numKeys);

// cheap pack-in remote with IR sensor.  (labels indicate
// factory overlay, not custom overlay)
// CH-   CH    CH+
// 0x45  0x46  0x47   
//
// |<<   >>|   >||
// 0x44  0x40  0x43
//
// -     +     EQ
// 0x07  0x15  0x09
//
// 0     100+  200+
// 0x16  0x19  0x0d
//
// 1     2     3
// 0x0c 0x18  0x5e
//
// 4     5     6
// 0x08  0x1c  0x5a
//
// 7     8     9
// 0x42  0x52  0x4a

/*
// handler for JPG decoder library
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap )
{
  if (y > tft.height() ) return 0;
  tft.pushImage(x,y,w,h,bitmap);
  return 1;
}

#define SPLASH_IMAGE "/splash.jpg"
void splash()
{
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  tft.setSwapBytes(true);
  TJpgDec.drawFsJpg(0,0,SPLASH_IMAGE);
}
*/

#define BLOCK_SIZE (16)
uint16_t block[BLOCK_SIZE * BLOCK_SIZE];

void colors(int n)
{
    for (int i = 0; i < n; i++ ) {
      uint16_t color = random16();
      for (int i = 0; i < BLOCK_SIZE * BLOCK_SIZE; i++ ) {
        block[i] = color;
      } 
      int x = random(0,LV_HOR_RES_MAX-BLOCK_SIZE);
      int y = random(0,LV_VER_RES_MAX-BLOCK_SIZE);
      tft.startWrite();
      tft.setAddrWindow(x, y, BLOCK_SIZE, BLOCK_SIZE);
      tft.pushPixels ((uint16_t*) block, BLOCK_SIZE*BLOCK_SIZE, true);
      tft.endWrite();
    }
}


void setup() 
{
    // Use serial port
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    logInfo("STARTUP!\n");

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE,FASTLED_DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);
    ledsOff();

    // first three LEDS: red, green, blue.
    leds[0] = CRGB::Red;
    leds[1] = CRGB::Green;
    leds[2] = CRGB::Blue;
    FastLED.show();  

    // make sure that SD card is not listening..
    /*
    pinMode( SDCARD_CS, OUTPUT );
    digitalWrite( SDCARD_CS, LOW );
    */

    // --------------- Init Display -------------------------

    // does not return on error.
    beginTFT();

    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    logDebug("CPU Freq: %d\n", getCpuFrequencyMhz());
    setCpuFrequencyMhz(240);

    if ( ! beginFilesystem() ) {
      drawErrorMessage("Failed to init FILESYSTEM! Did you upload the data folder?\n");
      while (1)
        yield(); // We stop here
    }

    tftTouchCalibrate(false);

    startGUI();

    beginClock();

    // touch_calibrate(false);

    //------------------ Load Wifi Config ----------------------------------------------

    if (!loadWifiConfig(wificonfig)) {
        logWarn("Failed to load WiFi Credentials!\n");
    }
    else
    {
        logInfo("WiFi Credentials Loaded\n");
    }

    if (wakeup_reason > 0)
    {
      // But we do draw something to indicate we are waking up
      tft.setTextFont(2);
      tft.println("Waking up...\n");
    }
    else
    {
      // splash();
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(BC_DATUM);
      // tft.setFreeFont(SMALL_FONT);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      char buf[32];
      sprintf(buf,"Version: %s", versionString);
      tft.drawString( buf, SCREEN_WIDTH/2, 150);
      delay(1000);
      logInfo("Loading version %s\n", versionString);
    }
}

/*
void printFileList(String path)
{
  logDebug("File List: '%s'\n", path);
  fs::File root = FILESYSTEM.open(path);
  int filecount = 0;

  if (root.isDirectory())
  {
    fs::File file = root.openNextFile();
    while (file)
    {
      logDebug("%s\n", file.name());

      file = root.openNextFile();
    }
    file.close();
  }
  logDebug("--- end of file list ---\n");
  root.close();
}
*/

void loop()
{
    loopClock();

    pollGUI();

    FastLED.show();

    irkb.loop();

#if 0
    if (irkb.keyAvailable()) {
      int k = irkb.getKey();
      logInfo("IRKEY: %s%c\n", (k&IRKB_IS_REPEAT) ? "(repeat) " : "", k&IRKB_KEY_MASK);
      switch(k &0xFF) {
        case IR_KEY_POWER:  // power
            logInfo("Toggle repeat\n");
            irkb.setIgnoreRepeat(!irkb.getIgnoreRepeat());
            break;
        case IR_KEY_TIME:
            setEnableClock(!getEnableClock());
            logInfo("Clock Enabled: %s\n", getEnableClock() ? "YES" : "NO");
            break;
        case IR_KEY_DOWN:
            logInfo("Begin Colors!\n");
            colors(1000);
            logInfo("Done Colors!\n");
            break;
        case IR_KEY_SETTINGS:
            logInfo("Settings button pressed\n");
            if (lv_scr_act() == ui_SettingsScreen) {
              logInfo("fade on title screen\n");
              lv_scr_load_anim(ui_TitleScreen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
            } else {
              logInfo("fade on settings screen\n");
              lv_scr_load_anim(ui_SettingsScreen, LV_SCR_LOAD_ANIM_OVER_LEFT, 500, 0, false);
            }
            break;
      }
    }
#endif

}

void ledsOff()
{
  for (int i = 0; i < NUM_LEDS; i++) { leds[i] = CRGB::Black; }
  FastLED.show();  
}