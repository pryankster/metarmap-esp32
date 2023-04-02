#include <Arduino.h>

#include "irkeyboard.h"
#include "log.h"
#include <IRremote.hpp>

//+=============================================================================
// Configure the Arduino
//
IRKeyboard::IRKeyboard(int _pin, const IRKeyMap *const _keyMap, int _numKeys )
{
    // Just to know which program is running on my Arduino
    logInfo( "IRKeyboard: " VERSION_IRREMOTE "\n");

    pin = _pin;
    keyMap = _keyMap;
    numKeys = _numKeys;

    IrReceiver.begin(_pin, false);
}

static int compareKeys(const void *a, const void *b)
{
    IRKeyMap *ka = (IRKeyMap*)a, *kb = (IRKeyMap *)b;
    int rc = ka->addr - kb->addr;
    if (rc == 0) {
        rc = ka->cmd - kb->cmd;
    }
    return rc;
}

IRKeyMap *IRKeyboard::findKey(uint8_t addr, uint8_t cmd)
{
    IRKeyMap k, *p;
    k.addr = addr;
    k.cmd = cmd;
    p = (IRKeyMap*) bsearch(&k, keyMap, numKeys, sizeof(IRKeyMap), compareKeys );
    if (p == NULL) return NULL;
    return p;
}

// call this in the arduino loop.  it doesn't return anything, but sets up internal state.
void IRKeyboard::loop() 
{
    char ch = 0xFF;
    if (IrReceiver.decode()) {
        if (! (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW)) {
            if (IrReceiver.decodedIRData.protocol == NEC) {
                IRKeyMap *p = findKey(IrReceiver.decodedIRData.address, IrReceiver.decodedIRData.command);
                // found a key?
                /*
                logInfo("IR: '%c' Addr: 0x%02.2x, CMD: 0x%02.2x, Raw: 0x%08.8x\n", 
                    (p == NULL ? ' ' : p->ch),
                    IrReceiver.decodedIRData.address,
                    IrReceiver.decodedIRData.command,
                    IrReceiver.decodedIRData.decodedRawData);
                */

                if (p != NULL) {
                    // decodedRawData being 0 seems to indicate a repeat.
                    if (IrReceiver.decodedIRData.decodedRawData == 0) {
                        if ( (ignoreRepeat && (p->flags & IRKB_REPEAT)) || (!ignoreRepeat && !(p->flags & IRKB_NO_REPEAT)) ) {
                            pushKey(p,true);
                        }
                    } else {
                        pushKey(p,false);
                    }
                }
            }
        }
        IrReceiver.resume();                            // Prepare for the next value
    }
}

void IRKeyboard::pushKey(IRKeyMap *key, bool isRepeat)
{
    // TODO: maybe complain if there's a key here?
    keyAvail = true;
    lastKey = *key;
    lastKey.flags = isRepeat ? IRKB_REPEAT : 0;
}

// return true if a key is available.
bool IRKeyboard::keyAvailable()
{
    return keyAvail;
}

IRKeyMap *IRKeyboard::getKeyRaw()
{
    if (!keyAvail) return NULL;
    keyAvail = false;
    return &lastKey;
}

// returns IRKB_NO_KEY_AVAILABLE if no key is available
int IRKeyboard::getKey()
{
    IRKeyMap *k = getKeyRaw();
    if ( k == NULL) return IRKB_NO_KEY_AVAILABLE;
    return k->ch | ((k->flags & IRKB_REPEAT) ? IRKB_IS_REPEAT : 0);
}

