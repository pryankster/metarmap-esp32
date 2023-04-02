#include <Arduino.h>
#include "prefs.h"
#include "log.h"

saveprefs_t prefs;
bool prefs_dirty;

void reset_prefs()
{
    prefs.version = PREFS_VERSION;
    prefs.magic = PREFS_MAGIC;
    prefs.end_magic = PREFS_END_MAGIC;
    prefs.current_airport = 0;      // default airport is first airport.
    prefs.brightness = 0;           // max bright
    prefs.metric = 0;               // metric/imperial 
    prefs.default_screen = PREFS_SCREEN_METAR;
    for (int i = 0; i < PREFS_NUM_FAVORITES; i++ ) {
        prefs.favorite_airport[i] = -1;
    }
}

void save_prefs(bool force)
{
    if (!prefs_dirty && !force) return;

    logInfo("save_prefs: saving prefs (dirty=%s force=%s)\n", prefs_dirty ? "yes" : "no", force ? "yes" : "no");
    prefs.version = PREFS_VERSION;
    prefs.magic = PREFS_MAGIC;
    prefs.end_magic = PREFS_END_MAGIC;
    int fd = open(PREFS_FILE,O_WRONLY|O_CREAT|O_TRUNC,0666);
    if (fd < 0) {
        logError("save_prefs: failed to open %s for writing: %s\n", PREFS_FILE, strerror(errno));
        return; 
    }
    write(fd, (const uint8_t *)&prefs,sizeof(prefs));
    close(fd);
    prefs_dirty = false;
}

bool load_prefs()
{
    int fd = open(PREFS_FILE, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            logInfo("load_prefs: No preferences file.  Creating.\n");
            reset_prefs();
            save_prefs(true);
            return true;
        }
        logError("load_prefs: failed to read prefs %s: %s", PREFS_FILE, strerror(errno));
        return false;
    }
    saveprefs_t tmp;
    int n = read(fd, &tmp, sizeof(tmp));
    close(fd);

    if (n != sizeof(tmp)) {
        logError("load_prefs: %s short read.  got %d, expected %d\n", PREFS_FILE, n, sizeof(prefs));
        return false;
    }

    if (tmp.version != PREFS_VERSION) {
        logDebug("load_prefs: Invalid prefs: got version %02.2x, expected %02.2x\n", tmp.version, PREFS_VERSION);
        return false;
    }

    if (tmp.magic != PREFS_MAGIC) {
        logDebug("load_prefs: Invalid prefs: got magic %02.2x, expected %02.2x\n", tmp.magic, PREFS_MAGIC);
        return false;
    }

    if (tmp.end_magic != PREFS_END_MAGIC) {
        logDebug("load_prefs: Invalid prefs: got end-magic %02.2x, expected %02.2x\n", tmp.end_magic, PREFS_END_MAGIC);
        return false;
    }

    prefs = tmp;
    prefs_dirty = false;
    return true;
}