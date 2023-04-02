#include <Arduino.h>
#include <SPIFFS.h>

#include "tft.h"
#include "filesystem.h"
#include "ft6236.h"
#include "webserver.h"
#include "config.h" // WiFi, config 'n stuff.
#include "network_config.h"
#include "log.h"
#include "drawhelper.h"
#include "bmp.h"
#include "gui.h"
#include "clock.h"
#include "sdcard.h"
#include "airports.h"
#include "esp_metar_map.h"
#include "prefs.h"
#include "menu.h"

const char *versionString = "0.0.0";

/*
#include "Fonts/Emmett__14pt7b.h"
#include "Fonts/Emmett__24pt7b.h"
#define SMALL_FONT &Emmett__14pt7b
#define LARGE_FONT &Emmett__24pt7b
*/

// this must remain sorted by 'cmd', so that bsearch works.
const IRKeyMap keyMap[] = {
    { 0x00, 0x07, IR_KEY_BRIGHTNESS }, // ROW: 2, COL: 0; brightness (lightbulb)
    { 0x00, 0x08, '7', 0x00 }, // ROW: 5, COL: 0; 7 (prs)
    { 0x00, 0x09, IR_KEY_SETTINGS }, // ROW: 2, COL: 2; settings (gear)
    { 0x00, 0x0c, '4', 0x00 }, // ROW: 4, COL: 0; 4 (ghi)
    { 0x00, 0x0d, '3', 0x00 }, // ROW: 3, COL: 2; 3 (def)
    { 0x00, 0x15, IR_KEY_DOWN }, // ROW: 2, COL: 1; down arrow
    { 0x00, 0x16, '1', 0x00 }, // ROW: 3, COL: 0; 1 
    { 0x00, 0x18, '5', 0x00 }, // ROW: 4, COL: 1; 5 (jkl)
    { 0x00, 0x19, '2', 0x00 }, // ROW: 3, COL: 1; 2 (abc)
    { 0x00, 0x1c, '8', 0x00 }, // ROW: 5, COL: 1; 8 (tuv)
    { 0x00, 0x40, IR_KEY_ENTER }, // ROW: 1, COL: 2; enter
    { 0x00, 0x42, IR_KEY_WEATHER }, // ROW: 6, COL: 0; weather (cloud)
    { 0x00, 0x43, IR_KEY_RIGHT }, // ROW: 1, COL: 1; right arrow
    { 0x00, 0x44, IR_KEY_LEFT }, // ROW: 1, COL: 0; left arrow
    { 0x00, 0x45, IR_KEY_POWER }, // ROW: 0, COL: 0; power
    { 0x00, 0x46, IR_KEY_UP }, // ROW: 0, COL: 1; up
    { 0x00, 0x47, IR_KEY_BACK }, // ROW: 0, COL: 2; back
    { 0x00, 0x4a, IR_KEY_TIME }, // ROW: 6, COL: 2; time (clock)
    { 0x00, 0x52, '0', 0x00 }, // ROW: 6, COL: 1; 0 (qz)
    { 0x00, 0x5a, '9', 0x00 }, // ROW: 5, COL: 2; 9 (wxy)
    { 0x00, 0x5e, '6', 0x00 }, // ROW: 4, COL: 2; 6 (mno)
};

const int numKeys = (sizeof(keyMap) / sizeof(keyMap[0]));
IRKeyboard irkb(IR_RECEIVE_PIN, keyMap, numKeys);

int ledBrightLevels[NUM_BRIGHT_LEVELS] = {
    80, 40, 20, 10,
};

int tftBrightLevels[NUM_BRIGHT_LEVELS] = {
    255, 128, 96, 64
};

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

void setup() 
{

    // delay(1000);
    // Use serial port
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    setLogLevel(LOG_INFO);

    logInfo("STARTUP!\n");

    beginSDCard();

    // start touch device
    beginFT2636();

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

    // load preferences
    load_prefs();

    airportsBegin();

    // TODO: load brightLevel from settings?
    set_airport_brightness(ledBrightLevels[prefs.brightness]);

    startGUI();

    beginClock();

    setEnableClock(true);

    logInfo("show_airport\n");
    // make sure prefs is valid.
    if (prefs.current_airport >= num_airports || prefs.current_airport < 0) {
        prefs.current_airport = 0;
        prefs_dirty = true;
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
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      char buf[32];
      sprintf(buf,"Version: %s", versionString);
      tft.drawString( buf, SCREEN_WIDTH/2, 150);
      delay(1000);
      logInfo("Version %s\n", versionString);
    }

    show_airport(prefs.current_airport);

    switch(prefs.default_screen) {
        case PREFS_SCREEN_CLOCK:
            logInfo("load CLOCK screen\n");
            menu_init(&clock_menu);
            // lv_scr_load_anim(ui_TitleScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
            break;
        default:
            // fall through.
        case PREFS_SCREEN_METAR:
            logInfo("load METAR screen\n");
            menu_init(&main_menu);
            // lv_scr_load_anim(ui_MetarScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
            break;
    }

    logInfo("network_begin\n");
    networkBegin();

}

void show_mem()
{
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buf[64];
  sprintf(buf,"[ mem: %6d ]", esp_get_free_heap_size());
  tft.drawString( buf, 1, 1);
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

static int ir_repeat_count = 0;

void loop()
{
    show_mem();

    loopClock();

    pollGUI();

    airportsLoop();

    networkLoop();

    irkb.loop();

    save_prefs(false);

    menu_loop();

  #if 0
    if (irkb.keyAvailable()) {
      int k = irkb.getKey();
      bool repeat = k&IRKB_IS_REPEAT ? true : false;
      if (repeat) ir_repeat_count ++;
      else ir_repeat_count = 0;
      k &= IRKB_KEY_MASK;
      logInfo("IRKEY: %s%c\n", repeat ? "(repeat) " : "", k);
      if (k >= '0' && k <= '9') {
          k -= '0';
          if (ir_repeat_count % 10 == 0) {
            logInfo("REPEAT key %d, %d times.\n", k, ir_repeat_count);
          }
      }
      switch(k) {
        case IR_KEY_POWER:  // power
            logInfo("Toggle repeat\n");
            irkb.setIgnoreRepeat(!irkb.getIgnoreRepeat());
            break;
        case IR_KEY_TIME:
            if (lv_scr_act() != ui_TitleScreen) {
              lv_scr_load_anim(ui_TitleScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
              if (prefs.default_screen != PREFS_SCREEN_CLOCK) {
                prefs.default_screen = PREFS_SCREEN_CLOCK;
                prefs_dirty = true;
              }
            }
            break;
        case IR_KEY_BRIGHTNESS:
            prefs.brightness++; prefs.brightness = prefs.brightness % NUM_BRIGHT_LEVELS;
            prefs_dirty = true;
            // set LED Brightness
            logInfo("Bright Level %d (%d)", prefs.brightness, ledBrightLevels[prefs.brightness]);
            set_airport_brightness(ledBrightLevels[prefs.brightness]);
            // TODO: set PWM brightness of backlight.
            break;
        case IR_KEY_WEATHER:
            if (lv_scr_act() != ui_MetarScreen) {
              lv_scr_load_anim(ui_MetarScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
              if (prefs.default_screen != PREFS_SCREEN_METAR) {
                prefs.default_screen = PREFS_SCREEN_METAR;
                prefs_dirty = true;
              }
            }
            break;
        case IR_KEY_RIGHT:
        case IR_KEY_DOWN:
        case IR_KEY_LEFT:
        case IR_KEY_UP:
            if (!repeat) {
              if (next_airport(k)) airport_blink(true);
            }
            break;
        case IR_KEY_ENTER:
            airport_blink(true);
            break;
        case IR_KEY_SETTINGS:
            logInfo("Settings button pressed\n");
            break;
      }
    }
#endif
}
