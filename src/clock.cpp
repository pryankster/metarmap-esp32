#include <ESP32Time.h>

#include "clock.h"
#include "log.h"
#include "squareline/ui.h"

static bool clockIsEnabled = false;
static ESP32Time rtc;
// will be 'true' once wifi is running and we can get time from an NTP server.
static bool timeValid = false;
static int8_t lastMinute = -1;

void beginClock()
{
    // force flag to be 'different' so setEnableClock will do it's thing.
    logInfo("beginClock\n");
    clockIsEnabled = true;  
    setEnableClock(false);
    rtc.setTime(0,15,22,6,3,2023);
}

bool getEnableClock() {
    return clockIsEnabled;
}

void setEnableClock(bool flag) {
    if (flag == clockIsEnabled) return;
    clockIsEnabled = flag;
    if (!clockIsEnabled) {
        lv_obj_add_flag( ui_timeBgHr10, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag( ui_timeBgHr1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag( ui_timeBgMin10, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag( ui_timeBgMin1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag( ui_timeDateLabel, LV_OBJ_FLAG_HIDDEN);
    } else {
        // force clock refresh.
        lastMinute = -1;
        lv_obj_clear_flag( ui_timeBgHr10, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag( ui_timeBgHr1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag( ui_timeBgMin10, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag( ui_timeBgMin1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag( ui_timeDateLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

static const char *digits[10] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
static char dateBuffer[51];
// call this in 'loop()';
void loopClock()
{
    int m = rtc.getMinute();

    // only update once a minute.
    if (!clockIsEnabled || lastMinute == m) return;

    int h = rtc.getHour(true);
    logInfo("Update Clock: %d %d : %d %d\n", h/10, h%10, m/10, m%10);
    lv_label_set_text_static(ui_timeHr10, digits[h/10]);
    lv_label_set_text_static(ui_timeHr1, digits[h%10]);
    lv_label_set_text_static(ui_timeMin10, digits[m/10]);
    lv_label_set_text_static(ui_timeMin1, digits[m%10]);
    struct tm timeinfo = rtc.getTimeStruct();
    strftime(dateBuffer, 50, "%A, %B %d %Y", &timeinfo);
    logInfo("Date: %s\n", dateBuffer);
    lv_label_set_text_static(ui_timeDateLabel, dateBuffer); // rtc.getDate(true).c_str() );
    lastMinute = m;
}