#ifndef _H_MENU_
#define _H_MENU_

#include <stdint.h>

// fwd declare
struct menu_t;
struct menuitem_t;

struct menucontext_t {
    menu_t *menu;
    menuitem_t *item;
};

struct menuitem_t {
    const char *label;
    void *user_data;
    bool (*is_active)(menucontext_t *ctx);
    bool (*is_selected)(menucontext_t *ctx);
    void (*handler)(menucontext_t *ctx);
};

struct inputmap_t {
    uint8_t key;
    void (*press)(menucontext_t *ctx, inputmap_t *input);
    void (*hold)(menucontext_t *ctx, inputmap_t *input);
    void *user_data;
};

struct menu_t {
    const char *title;
    const char *label;
    void *user_data;
    void (*enter)(const menu_t *menu);
    void (*leave)(const menu_t *menu);
    inputmap_t *input;
    menuitem_t *items;
};


// forward declare menu handler functions
extern void airport_cursor_move(menucontext_t *ctx, inputmap_t *input);
extern void airport_blink_current(menucontext_t *ctx, inputmap_t *input);
extern void menu_goto_favorite(menucontext_t *ctx, inputmap_t *input);
extern void menu_toggle_brightness(menucontext_t *ctx, inputmap_t *input);

extern void menu_goto(menucontext_t *ctx, inputmap_t *input);
extern void menu_push(menucontext_t *ctx, inputmap_t *input);
extern void menu_pop(menucontext_t *ctx, inputmap_t *input);
extern void menu_store_favorite(menucontext_t *ctx, inputmap_t *input);

#define INPUTMAP_END { .key = 0 }

// forward declare menus.
extern menu_t main_menu;
extern menu_t clock_menu;
extern menu_t power_off_menu;

void menu_init(menu_t *initial_menu);
void menu_loop();

// how many MS of no input before we decide that we've gotten a keypress
#define MENU_IDLE_KEY_DELAY (125)
// how long of a press until we count it as a long-press
#define MENU_LONG_PRESS (1000)

#endif // _H_MENU_