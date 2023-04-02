#include <Arduino.h>
#include <mutex.h>

#include "airports.h"
#include "msgbox.h"
#include "prefs.h"
#include "log.h"
#include "gui.h"
#include "esp_metar_map.h"
#include "menu.h"

// forward declare menus.
extern menu_t main_menu;
extern menu_t clock_menu;
extern menu_t power_off_menu;

static void menu_goto(menu_t *menu);
static void menu_enter(menu_t *menu);
static void menu_leave(menu_t *menu);
static void menu_push(menu_t *menu);
static void menu_pop();

// this is the 'main' METAR screen: 
// power: turn off display / leds
// arrow keys:  move the selected airport
// number keys:  select 'favorite' airport
// back: nothing
// brightness: toggle led and display brightness
// gear: settings menu
// weather: nothing.
// clock: change to time display
extern void enter_main_menu(const menu_t *menu);
menu_t main_menu = {
    .title = "Main Menu",
    .enter = enter_main_menu,
    .input = (inputmap_t[]) {
        { .key = IR_KEY_UP, .press = airport_cursor_move },
        { .key = IR_KEY_DOWN, .press = airport_cursor_move },
        { .key = IR_KEY_LEFT, .press = airport_cursor_move },
        { .key = IR_KEY_RIGHT, .press = airport_cursor_move },
        { .key = IR_KEY_ENTER, .press = airport_blink_current },
        { .key = IR_KEY_TIME, .press = menu_goto, .user_data = (void*)&clock_menu },
        { .key = '1', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '2', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '3', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '4', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '5', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '6', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '7', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '8', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '9', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = '0', .press = menu_goto_favorite, .hold = menu_store_favorite },
        { .key = IR_KEY_BRIGHTNESS, .press = menu_toggle_brightness },
        { .key = IR_KEY_POWER, .press = menu_push, .user_data = (void*)&power_off_menu },
        INPUTMAP_END
    }
};

extern void enter_power_off_menu(const menu_t *menu);
extern void leave_power_off_menu(const menu_t *menu);
menu_t power_off_menu = {
    .title = "Power off",
    .enter = enter_power_off_menu,
    .leave = leave_power_off_menu,
    .input = (inputmap_t[]) {
        { .key = IR_KEY_POWER, .press = menu_pop },
        INPUTMAP_END
    }
};

extern void enter_clock_menu(const menu_t *menu);
menu_t clock_menu = {
    .title = "Time Menu",
    .enter = enter_clock_menu,
    .input = (inputmap_t[]) {
        { .key = IR_KEY_WEATHER, .press = menu_goto, .user_data = (void*)&main_menu },
        { .key = IR_KEY_BACK, .press = menu_goto, .user_data = (void*)&main_menu },
        INPUTMAP_END
    }
};

#define MAX_MENU_STACK (3)
static menucontext_t menu_stack[MAX_MENU_STACK];
static menucontext_t *menu_cur, *menu_end;

void menu_init(menu_t *initial_menu)
{
    // make sure menu stack is all null'd out.
    logInfo("clear stack\n");
    memset(menu_stack,0, sizeof(menucontext_t)*MAX_MENU_STACK);

    // point to end of stack.
    logInfo("end stack\n");
    menu_end = menu_stack + (MAX_MENU_STACK - 1);
    // point to start of stack.
    logInfo("start stack\n");
    menu_cur = menu_stack;

    logInfo("GOTO \n");
    menu_goto(initial_menu);
}


static inputmap_t *find_key(uint8_t key)
{
    menu_t *menu = menu_cur->menu;
    logInfo("find_key: menu = %x\n", menu);
    if (menu->input) {
        logInfo("find_key: Looking for key 0x%02x\n",key);
        for( inputmap_t *i = menu->input; i->key; i++ ) {
            if (i->key == key) {
                logInfo("find_key: found key 0x%02x\n", i->key); 
                return i;
            }
        }
        logInfo("find_key: did not find key 0x%02x\n", key);
        return NULL;
    } else {
        logInfo("find_key: No input on current menu.\n");
        return NULL;
    }
}


static uint32_t last_key_tick_start;       // when first keypress is seen
static uint32_t last_key_ticks;            // most recent keypress
static uint32_t key_repeat_count;
static uint8_t  last_key;                  // last seen keycode

void menu_loop()
{
    uint32_t cur_ticks = millis();
    if (irkb.keyAvailable()) {
        int k = irkb.getKey();
        bool repeat = k&IRKB_IS_REPEAT ? true : false;
        if (repeat) {
            // ignore repeats if we haven't seen a 
            if (last_key) {
                key_repeat_count++;
                logInfo("IRKEY: (REPEAT %d) 0x%02.2x (last=%d first=%d)\n", key_repeat_count, last_key, cur_ticks - last_key_ticks, cur_ticks - last_key_tick_start);
                last_key_ticks = cur_ticks;
            } else {
                logInfo("IRKEY: IGNORE REPEAT\n");
            }
        } else {
            key_repeat_count = 1;
            last_key_ticks = last_key_tick_start = cur_ticks;
            last_key = k & IRKB_KEY_MASK;
            logInfo("IRKEY: 0x%02.2x\n", last_key);
        }
    }
    if (last_key && cur_ticks - last_key_ticks > MENU_IDLE_KEY_DELAY) {
        logInfo("IRKEY: looking for 0x%02x\n", last_key);
        inputmap_t *i = find_key(last_key);
        if (i) {
            if (cur_ticks - last_key_tick_start > MENU_LONG_PRESS) {
                if (i->hold) {
                    logInfo("Calling HOLD callback %x\n", i->hold);
                    i->hold( menu_cur, i);
                } else {
                    logInfo("No HOLD callback for key\n");
                }
            } else {
                if (i->press) {
                    logInfo("Calling PRESS callback %x\n", i->hold);
                    i->press( menu_cur, i);
                } else {
                    logInfo("No PRESS callback for key\n");
                }
            }
        }
        last_key = 0;
    }
}

static void menu_leave(menu_t *menu)
{
    if (menu && menu->leave) {
        logInfo("leave menu callback\n");
        (*menu->leave)(menu);
    }
}

static void menu_enter(menu_t *menu)
{
    if (menu && menu->enter) {
        logInfo("enter menu callback\n");
        (*menu->enter)(menu);
    }
}

static void menu_goto(menu_t *menu)
{
    // unwind any push'd menus
    do {
        logInfo("unwind\n");
        menu_leave(menu_cur->menu);
        menu_cur--;
    } while (menu_cur >= menu_stack);

    logInfo("reset menu stack\n");
    menu_cur = menu_stack;
    logInfo("assign menu\n");
    menu_cur->menu = menu;
    logInfo("assign item\n");
    menu_cur->item = menu->items;
    logInfo("enter menu\n");
    menu_enter(menu);
}

static void menu_push(menu_t *menu)
{
    // don't push too many. (TODO: Log it!)
    if (menu_cur >= menu_end) return;
    // note that 'leave' isn't called for menu until pop.
    menu_cur++;
    menu_cur->menu = menu;
    menu_cur->item = menu->items;
    menu_enter(menu);
}

static void menu_pop()
{
    // don't underflow. TODO: log it!
    if (menu_cur == menu_stack) return;
    menu_leave(menu_cur->menu);
    menu_cur->menu = NULL;
    menu_cur->item = NULL;
    menu_cur--;
}

void menu_goto(menucontext_t *ctx, inputmap_t *input)
{
    menu_goto((menu_t*)input->user_data);
}

void menu_push(menucontext_t *ctx, inputmap_t *input)
{
    menu_push((menu_t*)input->user_data);
}

void menu_pop(menucontext_t *ctx, inputmap_t *input)
{
    menu_pop();
}

void airport_cursor_move(menucontext_t *ctx, inputmap_t *input)
{
    if (next_airport(input->key)) airport_blink(true);
}

void airport_blink_current(menucontext_t *ctx, inputmap_t *input)
{
    logInfo("blink current\n");
    airport_blink(true);
}

void menu_goto_favorite(menucontext_t *ctx, inputmap_t *input)
{
    int fav = input->key - '0';
    if (prefs.favorite_airport[fav] >= num_airports) {
        showMessagef(3000, "No favorite saved in slot #%d", fav);
        return;
    }
    logInfo("Go to favorite %d\n", fav);
    show_airport(prefs.favorite_airport[fav]);
    airport_blink(true);
    // can't use 'current_airport' here, because it hasn't been updated yet!
    showMessagef(3000, "Recall Favorite %d: %s", fav, get_airport(prefs.favorite_airport[fav])->name);
}

void menu_store_favorite(menucontext_t *ctx, inputmap_t *input)
{
    logInfo("Store current as favorite %d\n", input->key);
    int fav = input->key - '0';
    prefs.favorite_airport[fav] = prefs.current_airport;
    showMessagef(3000, "Store Favorite %d: %s", fav, get_airport(prefs.current_airport)->name);
    prefs_dirty = true;
}

void menu_toggle_brightness(menucontext_t *ctx, inputmap_t *input)
{
    prefs.brightness++; prefs.brightness = prefs.brightness % NUM_BRIGHT_LEVELS;
    prefs_dirty = true;
    // set LED Brightness
    logInfo("Bright Level %d (%d)", prefs.brightness, ledBrightLevels[prefs.brightness]);
    set_airport_brightness(ledBrightLevels[prefs.brightness]);
    // TODO: set PWM brightness of backlight.
}

void enter_clock_menu(const menu_t *menu)
{
    logInfo("Enter clock menu\n");
    if (lv_scr_act() != ui_TitleScreen) {
        lv_scr_load_anim(ui_TitleScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        if (prefs.default_screen != PREFS_SCREEN_CLOCK) {
            prefs.default_screen = PREFS_SCREEN_CLOCK;
            prefs_dirty = true;
        }
    }
}

void enter_main_menu(const menu_t *menu)
{
    logInfo("Enter main menu\n");
    if (lv_scr_act() != ui_MetarScreen) {
        lv_scr_load_anim(ui_MetarScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        if (prefs.default_screen != PREFS_SCREEN_METAR) {
            prefs.default_screen = PREFS_SCREEN_METAR;
            prefs_dirty = true;
        }
    }
}

extern void enter_power_off_menu(const menu_t *menu)
{
    logInfo("POWER OFF\n");
}

extern void leave_power_off_menu(const menu_t *menu)
{
    logInfo("POWER ON\n");
}