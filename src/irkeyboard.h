#ifndef _H_IRKEYBOARD_
#define _H_IRKEYBOARD_

struct IRKeyMap {
    uint8_t addr;
    uint8_t cmd;
    char    ch;     
    uint8_t flags;
};

#define IRKB_NO_REPEAT (1<<0) // if set, key WILL NOT repeat, even if repeat is enabled
#define IRKB_REPEAT    (1<<1) // if set, key WILL repeat, even if repeat is disabled

#define IRKB_IS_REPEAT (1<<8)   // masked onto value returned by getKey()
#define IRKB_KEY_MASK (0xFF)    // mask to get character from getKey()
#define IRKB_NO_KEY_AVAILABLE (~0)  // return value from getKey() is NO key is available.

class IRKeyboard {
public:
    // 'keyMap' MUST be sorted, we use 'bsearch' to find keys.
    IRKeyboard(int pin, const IRKeyMap *const keyMap, int numKeys );

    // return or ignore repeat keys? (see notes about IRDB_NO_REPEAT above)
    void setIgnoreRepeat(bool _ignoreRepeat) { ignoreRepeat = _ignoreRepeat; }
    bool getIgnoreRepeat() { return ignoreRepeat; }

    // call this in loop.
    void loop();

    // return true if a key is available.
    bool keyAvailable();

    // returns -1 if no key is available,
    // returns key in lower 8 bits.  if 0x100 (1<<8) is set, key is a repeat.
    int getKey();
    IRKeyMap *getKeyRaw();

    void pushKey(IRKeyMap *key, bool isRepeat);

private:
    bool ignoreRepeat;
    uint8_t keyAvail;
    IRKeyMap lastKey;
    int pin;
    const IRKeyMap *keyMap;
    int numKeys;

    IRKeyMap *findKey(uint8_t addr, uint8_t cmd);
};

#endif // _H_IRKEYBOARD_