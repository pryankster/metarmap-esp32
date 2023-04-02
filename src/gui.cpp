#include <lvgl.h>
#include "log.h"
#include "tft.h"
#include "gui.h"
#include "squareline/ui.h"
#include "irkeyboard.h"
#include "ft6236.h"

extern IRKeyboard irkb;

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 480;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t pixel_buf[screenWidth*GUI_BUFFER_LINES];
#if GUI_BUFFER_2
static lv_color_t pixel_buf2[screenWidth*GUI_BUFFER_LINES];
#endif

#if LV_USE_LOG != 0
static void my_print(const char *buf)
{
    logRaw("LVGL: %s", buf);
}
#endif

static void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1 );
    uint32_t h = (area->y2 - area->y1 + 1 );
    // logDebug("FLUSH: %d,%d x %d,%d\n", area->x1, area->y1, w, h);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushPixels ((uint16_t*) &color_p->full, w*h); // , true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

static void my_touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    int touch[2];
    bool touched = ft6236_pos(touch);
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch[0];
        data->point.y = touch[1];
        logInfo("Touch: %d,%d\n", touch[0], touch[1]);
    }
}

static void my_keyboard_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    if (irkb.keyAvailable()) {
        int k = irkb.getKey();
        if (k&IRKB_IS_REPEAT) return;
        logInfo("KEYBOARD: %s%c\n", (k&IRKB_IS_REPEAT) ? "(repeat) " : "", k&IRKB_KEY_MASK);
        data->key = k & 0xFF;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}


// really it's the IR remote...
lv_indev_t *keyboard;

// called when a screen comes in focus.  user_data should have group to set on screen.
// TODO: do we need to focus the first widget, or will it retain focus from last time?
static void focus_input_group(lv_event_t *e)
{
    lv_group_t *group = (lv_group_t*) e->user_data;
    if (group != NULL) {
        lv_indev_set_group(keyboard, group);
        logInfo("SET indev group for screen\n");
    }
}

void startGUI()
{
    logInfo("LVGL Startup\n");
    lv_init();
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif
#if GUI_BUFFER_2
    lv_disp_draw_buf_init( &draw_buf, pixel_buf, pixel_buf2, screenWidth * GUI_BUFFER_LINES);
#else
    lv_disp_draw_buf_init( &draw_buf, pixel_buf, NULL, screenWidth * GUI_BUFFER_LINES);
#endif

    // init LVGL display driver

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // init input device 
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    /*
    static lv_indev_drv_t remote_drv;
    lv_indev_drv_init(&remote_drv);
    remote_drv.type = LV_INDEV_TYPE_KEYPAD;
    remote_drv.read_cb = my_keyboard_read;
    keyboard = lv_indev_drv_register(&remote_drv);
    */

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t *cur_drv =NULL;
    for(;;) {
        cur_drv = lv_indev_get_next(cur_drv);
        if (!cur_drv) break;

        if (cur_drv->driver->type == LV_INDEV_TYPE_KEYPAD || cur_drv->driver->type == LV_INDEV_TYPE_ENCODER) {
            lv_indev_set_group(cur_drv,g);
        }
    }
    ui_init();

    logInfo("LVGL: setup done\n");
}

void pollGUI()
{
    lv_timer_handler();
    // delay(5);
}