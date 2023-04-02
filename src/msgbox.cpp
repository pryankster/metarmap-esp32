#include <lvgl.h>
#include <stdarg.h>
#include <stdio.h>
#include "msgbox.h"
#include "log.h"

static lv_timer_t *msgBoxTimer;
static lv_obj_t *ui_statusPanel, *ui_statusText;

static void msgBoxTimeout(lv_timer_t *timer)
{
    logInfo("msgBoxTimer: called\n");
    dismissMsg();
}

static void msgBoxCreate(lv_obj_t *parent)
{
    ui_statusPanel = lv_obj_create(parent);
    lv_obj_set_width(ui_statusPanel, lv_pct(90));
    lv_obj_set_height(ui_statusPanel, LV_SIZE_CONTENT);    /// 140
    lv_obj_set_x(ui_statusPanel, 0);
    lv_obj_set_y(ui_statusPanel, 282);
    lv_obj_set_align(ui_statusPanel, LV_ALIGN_TOP_MID);
    lv_obj_set_flex_flow(ui_statusPanel, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(ui_statusPanel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(ui_statusPanel, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_bg_color(ui_statusPanel, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_statusPanel, 192, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_statusText = lv_label_create(ui_statusPanel);
    lv_obj_set_width(ui_statusText, lv_pct(100));
    lv_obj_set_height(ui_statusText, LV_SIZE_CONTENT);    /// 100
    lv_obj_set_align(ui_statusText, LV_ALIGN_CENTER);
    lv_label_set_text(ui_statusText, "Connecting WiFi...");
    lv_label_set_recolor(ui_statusText, "true");
    lv_obj_set_style_text_color(ui_statusText, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_statusText, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_statusText, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void showMessagef(int timeout_ms, const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    vshowMessagef(timeout_ms, fmt, ap);
    va_end(ap);
}

void vshowMessagef(int timeout_ms, const char *fmt, va_list arg )
{
    char loc_buf[64];
    char * temp = loc_buf;
    va_list copy;
    va_copy(copy, arg);
    int len = vsnprintf(temp, sizeof(loc_buf), fmt, copy);
    va_end(copy);

    if(len <= 0) return;

    if(len >= sizeof(loc_buf)){
        temp = (char*) malloc(len+1);
        if(temp == NULL) return;
        len = vsnprintf(temp, len+1, fmt, arg);
    }

    showMessage(timeout_ms, temp);

    if(temp != loc_buf){
        free(temp);
    }
}

void showMessage(int timeout_ms, const char *msg)
{
    if (ui_statusPanel == NULL) {
        logInfo("showMessage: create msgbox\n");
        msgBoxCreate(lv_layer_top());
    } else {
        logInfo("showMessage: use existing msgbox\n");
    }
    lv_label_set_text(ui_statusText, msg);

    if (timeout_ms) {
        if (msgBoxTimer == NULL) {
            logInfo("showMessage: create timer\n");
            msgBoxTimer = lv_timer_create(msgBoxTimeout, timeout_ms, NULL);
        } else {
            logInfo("showMessage: reset timer\n");
            lv_timer_set_period( msgBoxTimer, timeout_ms );
            lv_timer_reset( msgBoxTimer );
        }
    } else {
        logInfo("showMessage: no timeout, delete timer\n");
        if (msgBoxTimer != NULL) {
            lv_timer_del(msgBoxTimer);
            msgBoxTimer = NULL;
        }
    }
}

void dismissMsg()
{
    logInfo("dismissMsg");
    if (ui_statusPanel) {
        logInfo("dismissMsg: close msgbox\n");
        lv_obj_del(ui_statusPanel);
        ui_statusPanel = ui_statusText = NULL;
    }
    if (msgBoxTimer) {
        logInfo("dismissMsg: stop timer\n");
        lv_timer_del(msgBoxTimer);
        msgBoxTimer = NULL;
    }
}