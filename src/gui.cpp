#include <lvgl.h>
#include "log.h"
#include "tft.h"
#include "gui.h"
#include "squareline/ui.h"
#include "irkeyboard.h"

extern IRKeyboard irkb;

static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 480;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth*GUI_BUFFER_LINES];
// static lv_color_t buf2[screenWidth*GUI_BUFFER_LINES];

#if LV_USE_LOG != 0
static void my_print(const char *buf)
{
    logDebug("LVGL: %s", buf);
}
#endif

static void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1 );
    uint32_t h = (area->y2 - area->y1 + 1 );
    // logDebug("FLUSH: %d,%d x %d,%d\n", area->x1, area->y1, w, h);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors ((uint16_t*) &color_p->full, w*h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

static void my_touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY, 600);
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
        logInfo("Touch: %d,%d\n", touchX, touchY);
    }
}

static void my_keyboard_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    if (irkb.keyAvailable()) {
        int k = irkb.getKey();
        if (k&IRKB_IS_REPEAT) return;
        logInfo("IR: %s%c\n", (k&IRKB_IS_REPEAT) ? "(repeat) " : "", k&IRKB_KEY_MASK);
        data->key = k & 0xFF;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

#if 0
const char *screen_target(lv_obj_t *t)
{
    if (t == ui_TitleScreen) return "titleScreen";
    if (t == ui_SettingsScreen) return "settingsScreen";
    if (t == ui_WiFiWizard1) return "wifiWizard1";
    static char buffer[18];
    sprintf(buffer,"%08.8x", t);
    return buffer;
}

const char *const event_to_str(lv_event_code_t code)
{
    static char buf[16];
    switch(code) {
    case LV_EVENT_PRESSED: return ("PRESSED");
    case LV_EVENT_PRESSING: return ("PRESSING");
    case LV_EVENT_PRESS_LOST: return ("PRESS_LOST");
    case LV_EVENT_SHORT_CLICKED: return ("SHORT_CLICKED");
    case LV_EVENT_LONG_PRESSED: return ("LONG_PRESSED");
    case LV_EVENT_LONG_PRESSED_REPEAT: return ("LONG_PRESSED_REPEAT");
    case LV_EVENT_CLICKED: return ("CLICKED");
    case LV_EVENT_RELEASED: return ("RELEASED");
    case LV_EVENT_SCROLL_BEGIN: return ("SCROLL_BEGIN");
    case LV_EVENT_SCROLL_END: return ("SCROLL_END");
    case LV_EVENT_SCROLL: return ("SCROLL");
    case LV_EVENT_GESTURE: return ("GESTURE");
    case LV_EVENT_KEY: return ("KEY");
    case LV_EVENT_FOCUSED: return ("FOCUSED");
    case LV_EVENT_DEFOCUSED: return ("DEFOCUSED");
    case LV_EVENT_LEAVE: return ("LEAVE");
    case LV_EVENT_HIT_TEST: return ("HIT_TEST");
    case LV_EVENT_COVER_CHECK: return ("COVER_CHECK");
    case LV_EVENT_REFR_EXT_DRAW_SIZE: return ("REFR_EXT_DRAW_SIZE");
    case LV_EVENT_DRAW_MAIN_BEGIN: return ("DRAW_MAIN_BEGIN");
    case LV_EVENT_DRAW_MAIN: return ("DRAW_MAIN");
    case LV_EVENT_DRAW_MAIN_END: return ("DRAW_MAIN_END");
    case LV_EVENT_DRAW_POST_BEGIN: return ("DRAW_POST_BEING");
    case LV_EVENT_DRAW_POST: return ("DRAW_POST");
    case LV_EVENT_DRAW_POST_END: return ("DRAW_POST_END");
    case LV_EVENT_DRAW_PART_BEGIN: return ("DRAW_PART_BEGIN");
    case LV_EVENT_DRAW_PART_END: return ("DRAW_PART_END");
    case LV_EVENT_VALUE_CHANGED: return ("VALUE_CHANGED");
    case LV_EVENT_INSERT: return ("INSERT");
    case LV_EVENT_REFRESH: return ("REFRESH");
    case LV_EVENT_READY: return ("READY");
    case LV_EVENT_CANCEL: return ("CANCEL");
    case LV_EVENT_DELETE: return ("DELETE");
    case LV_EVENT_CHILD_CHANGED: return ("CHILD_CHANGED");
    case LV_EVENT_CHILD_CREATED: return ("CHILD_CREATED");
    case LV_EVENT_CHILD_DELETED: return ("CHILD_DELETED");
    case LV_EVENT_SCREEN_UNLOAD_START: return ("UNLOAD_START");
    case LV_EVENT_SCREEN_LOAD_START: return ("LOAD_START");
    case LV_EVENT_SCREEN_LOADED: return ("LOADED");
    case LV_EVENT_SCREEN_UNLOADED: return ("UNLOADED");
    case LV_EVENT_SIZE_CHANGED: return ("SIZE_CHANGED");
    case LV_EVENT_STYLE_CHANGED: return ("STYLE_CHANGED");
    case LV_EVENT_LAYOUT_CHANGED: return ("LAYOUT_CHANGED");
    case LV_EVENT_GET_SELF_SIZE: return ("GET_SELF_SIZE");
    default:
        sprintf(buf,"%d",code);
        return buf;
    }
}

void screen_event(lv_event_t * e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    logInfo("Screen event: code=%d %s, target=%s\n", event_code, event_to_str(event_code), screen_target(target));
}
#endif

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
    logInfo("LVGL Startup");
    lv_init();
#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, screenWidth * GUI_BUFFER_LINES);

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

    static lv_indev_drv_t remote_drv;
    lv_indev_drv_init(&remote_drv);
    remote_drv.type = LV_INDEV_TYPE_KEYPAD;
    remote_drv.read_cb = my_keyboard_read;
    keyboard = lv_indev_drv_register(&remote_drv);

    // TODO: make 'keypad' device for remote?

    ui_init();

    lv_group_t *g;

    /* title screen group */
    g = lv_group_create();
    lv_group_set_wrap(g, true);
    lv_group_add_obj(g, ui_Button8);

    lv_obj_add_event_cb(ui_TitleScreen, focus_input_group, LV_EVENT_SCREEN_LOADED, g);

    /* settings screen group */
    g = lv_group_create();
    lv_group_set_wrap(g, true);
    lv_group_add_obj( g, ui_Button6 );
    lv_group_add_obj( g, ui_Button2 );
    lv_group_add_obj( g, ui_Button1 );
    lv_group_add_obj( g, ui_Button5 );
    lv_group_add_obj( g, ui_Button7 );

    lv_obj_add_event_cb(ui_SettingsScreen, focus_input_group, LV_EVENT_SCREEN_LOADED, g);

    /* wizard1 screen group */
    g = lv_group_create();
    lv_group_set_wrap(g, true);
    lv_group_add_obj(g, ui_Button3 );
    lv_group_add_obj(g, ui_Button4 );

    lv_obj_add_event_cb(ui_WiFiWizard1, focus_input_group, LV_EVENT_SCREEN_LOADED, g);

    lv_style_t focus_style;
    lv_style_init(&focus_style);

    lv_style_set_outline_color(&focus_style, lv_palette_main(LV_PALETTE_YELLOW)) ; // LV_COLOR_MAKE(255,240,0));
    lv_style_set_outline_width(&focus_style, 3);
    lv_style_set_outline_pad(&focus_style, 3);

    lv_obj_add_style(ui_TitleScreen, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style(ui_SettingsScreen, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style(ui_WiFiWizard1, &focus_style, LV_STATE_FOCUSED );

    lv_obj_add_style( ui_Button6, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style( ui_Button2, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style( ui_Button1, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style( ui_Button5, &focus_style, LV_STATE_FOCUSED );
    lv_obj_add_style( ui_Button7, &focus_style, LV_STATE_FOCUSED );

    // let all objects know to recompute style
    lv_obj_report_style_change(NULL);

    logInfo("LVGL: setup done");
}

void pollGUI()
{
    lv_timer_handler();
    delay(5);
}