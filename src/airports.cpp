#include <Arduino.h>
#include <float.h>
#include <FastLED.h>
#include <WiFi.h>
#include "lvgl.h"

#include "squareline/ui.h"

#include "log.h"
#include "filesystem.h"
#include "airports.h"
#include "metar.h"
#include "kv_pair.h"
#include "prefs.h"

#include "esp_metar_map.h"

#include "mutex.h"

#define METAR_DOTS "       .   .   .   .   .   .   "

FASTLED_USING_NAMESPACE

#define LED_TYPE    WS2812
#define COLOR_ORDER RGB
static CRGB *leds;         // current color of LEDs, attached to FastLED
CRGB *t_leds;       // 'double buffer' of LEDs for fading.


int currentBrightness = 0;
int targetBrightness = 0;

int get_airport_brightness()
{
    return targetBrightness >> 8;
}

void set_airport_brightness(int val)
{
    if (val > 255) val = 255;
    if (val < 0) val = 0;
    val <<= 8;
    targetBrightness = val;
}

static airport_t *airports;
int num_airports;

#define WX_STALE_TIME (3600)        // seconds

static uint32_t prev_ticks = 0;
static int update_current = -1;

static String nextField(String &line)
{
    String field;
    int mark = line.indexOf(',');
    if (mark == -1) {
        field = line;
        line = "";
    } else {
        field = line.substring(0,mark);
        line = line.substring(mark+1);
    }
    return field;
}

// not threadsafe.
static bool parse_airport(String line, airport_t *airport)
{
    logInfo("Parsing: %s\n", line.c_str());
    String name;
    int mark;

    name = nextField(line);  // get ICAO code

    mark = name.indexOf('=');
    if (mark != -1) {
        // show weather from another code for this airport.
        airport->name = strdup(name.substring(0,mark).c_str());
        airport->weather = strdup(name.substring(mark+1).c_str());
    } else {
        airport->name = strdup(name.c_str());
        airport->weather = NULL;
    }

    // get full name.
    airport->full_name = strdup(nextField(line).c_str());

    // x/y coordinates
    airport->x = nextField(line).toFloat();
    airport->y = nextField(line).toFloat();

    // elevation
    airport->elevation = nextField(line).toFloat();

    logInfo("Airport: %s (weather: %s) %s x=%.2f y=%.2f elev=%.2f\n", 
        airport->name, airport->weather ? airport->weather : airport->name,
        airport->full_name, airport->x, airport->y, airport->elevation);

    return true;
}


// not threadsafe.
int load_airports()
{
    String tmp;
    FILE *f = fopen(data_path("airports.csv"), "r" );
    char linebuf[256];
    fgets(linebuf, sizeof(linebuf), f);
    num_airports = atoi(linebuf);
    logInfo("loadAirports: %d airports\n", num_airports);
    airports = (airport_t*)calloc(num_airports, sizeof(airport_t));
    for (int i = 0; i < num_airports; i++ ) {
        fgets(linebuf, sizeof(linebuf), f);
        parse_airport(String(linebuf),&(airports[i]));
    }
    fclose(f);
    return num_airports;
}

// not threadsafe.
static int _airport_index(const char *name)
{
    for (int i = 0; i < num_airports; i++ )
    {
        if (strcmp(name,airports[i].name)== 0) { return i; }
    }
    return -1;
}

// not threadsafe.
static airport_t *_get_airport(const char *name)
{
    int index = airport_index(name);
    if (index == -1) return NULL;
    return airports + index;
}

// not threadsafe.
static airport_t *_get_airport(int index)
{
    if (index < 0 || index >= num_airports) return NULL;
    return airports + index;
}

// threadsafe.
int airport_index(const char *name)
{
    _lock();
    auto rc = _airport_index(name);
    _release();
    return rc;
}

// threadsafe.
airport_t *get_airport(const char *name)
{
    _lock();
    auto rc = _get_airport(name);
    _release();
    return rc;
}

// threadsafe.
airport_t *get_airport(int index)
{
    _lock();
    auto rc = _get_airport(index);
    _release();
    return rc;
}

typedef bool (*field_setter)(airport_t *airport, const char *key, const char *val);

// not threadsafe
static bool set_int(int &out, const char *val, int def_val)
{
    if (strlen(val) == 0) { out = def_val; return true; }
    out = atoi(val);
    return true;
}

// not threadsafe
static bool set_float(float &out, const char *val, float def_val)
{
    if (strlen(val) == 0) { 
        logDebug("set_float: returning default (%f)\n", def_val);
        out = def_val; 
        return true; 
    }
    out = strtof(val,NULL);
    logDebug("set_float: '%s' parses to %f\n", val, out );
    return true;
}

// not threadsafe
static bool set_charbuf(char *&out, const char *val, const char *def_val)
{
    if (out != NULL) free(out);
    out = NULL;
    if (val == NULL || strlen(val) == 0) val = def_val;
    if (val != NULL) out = strdup(val);
    return true;
}

// not threadsafe
static bool set_wind_dir(airport_t *airport, const char *key, const char *val)
{
    return set_int(airport->wind_dir, val, -1);
}

// not threadsafe
static bool set_wind_speed(airport_t *airport, const char *key, const char *val)
{
    return set_int(airport->wind_speed, val, -1);
}

// not threadsafe
static bool set_wind_gust(airport_t *airport, const char *key, const char *val)
{
    return set_int(airport->wind_gust, val, -1);
}

// not threadsafe
static bool set_vis(airport_t *airport, const char *key, const char *val)
{
    return set_float(airport->vis, val, -1.0);
}

// not threadsafe
static bool set_altimiter(airport_t *airport, const char *key, const char *val)
{
    return set_float(airport->altimiter, val, -1.0);
}

/* in a METAR, this is station elevation, which we don't really care about. 
static bool set_elevation(airport_t *airport, const char *key, const char *val)
{
    return set_float(airport->elevation, val, 0);
}
*/

kv_pair<int> sky_cover_map[] = {
    { "SKC", CLOUD_SKC },
    { "CLR", CLOUD_CLR },
    { "CAVOK", CLOUD_CAVOK },
    { "FEW", CLOUD_FEW },
    { "SCT", CLOUD_SCT },
    { "BKN", CLOUD_BKN },
    { "OVC", CLOUD_OVC },
    { "OVX", CLOUD_OVX },
    { NULL,  -1 },
};

const char *sky_cover[] = {
    "SKC", // CLOUD_SKC
    "CLR", // CLOUD_CLR 
    "CAVOK", // CLOUD_CAVOK 
    "FEW", // CLOUD_FEW 
    "SCT", // CLOUD_SCT 
    "BKN", // CLOUD_BKN 
    "OVC", // CLOUD_OVC 
    "OVX", // CLOUD_OVX 
};

// tricky, because the columns 'sky_cover' and 'cloud_base_ft_agl' appear
// three times in the CSV file we get from the METAR.  we store as an array
// clouds[3].  So we need to know when to reset the index counter. which we do when
// we start setting columns.
// the report SHOULD also always give 'sky_cover' first, so we increment cloud_idx
// here in set_clouds once we've seen cloud_base_ft_agl
// note that this handler is called for both keys!
// not threadsafe
static bool set_clouds(airport_t *airport, const char *key, const char *val)
{
    if (airport->cloud_idx >= WX_CLOUD_RECORDS ) return false; 
    logDebug("%s[%d] = %s\n", key, airport->cloud_idx, val);
    clouds_t *c = airport->clouds + airport->cloud_idx;
    switch(key[0]) {
        case 's':       // sky_cover
            {
                auto sc = match_kv( sky_cover_map, val );
                c->sky_cover = sc;
            }
            break;
        case 'c':       // cloud_base_ft_agl
            set_int(c->altitude, val, -1);
            airport->cloud_idx++;
            break;
    }
    return true;
}

// not threadsafe
static bool set_temp(airport_t *airport, const char *key, const char *val)
{
    return set_float(airport->temp_c, val, -999 );
}

// not threadsafe
static bool set_dew(airport_t *airport, const char *key, const char *val)
{
    return set_float(airport->dew_c, val, -999 );
}

// not threadsafe
static bool set_report_time(airport_t *airport, const char *key, const char *val)
{
    return true;
}

// not threadsafe
static bool set_metar(airport_t *airport, const char *key, const char *val)
{
    return set_charbuf(airport->metar, val, "");
}

// 'public' interface.
// threadsafe
void set_airport_metar(airport_t *airport, const char *val)
{
    _lock();
    set_charbuf(airport->metar, val, "");
    _release();
}

kv_pair<int> flight_category_map[] = {
    { "VFR",  WX_COND_VFR },
    { "MVFR", WX_COND_MVFR },
    { "IFR",  WX_COND_IFR },
    { "LIFR", WX_COND_LIFR },
    { NULL, -1 },
};

// not threadsafe
static bool set_flight_category(airport_t *airport, const char *key, const char *val)
{
    logInfo("Set flight category %s = %s\n", airport->name, val);
    airport->wx_cond = (int) match_kv(flight_category_map, val);
    return true;
}

  /*
  float press_alt;
  time_t lastMetar; // when did we last check METAR for this airport
  bool lightning;   // if true, lightning is present.
  int lastFlash;    // last lightning flash, in ticks()
  */
kv_pair<field_setter> metar_fields[] = {
    { "raw_text", set_metar },
    // { "station_id", ... },
    // { "observation_time", set_obs_time }, // V: 2023-03-18T23:47:00Z
    // {"latitude", set_latitude }, //  V: 37.33
    // {"longitude", set_longitude }, //  V: -121.82
    {"temp_c", set_temp },                  //  V: 21.0
    {"dewpoint_c", set_dew },               //  V: 6.0
    {"wind_dir_degrees", set_wind_dir },    //  V: 160
    {"wind_speed_kt", set_wind_speed },     //  V: 12
    {"wind_gust_kt", set_wind_gust },       //  V:
    {"visibility_statute_mi", set_vis },    //  V: 10.0
    {"altim_in_hg", set_altimiter },        //  V: 29.940945
    // {"sea_level_pressure_mb", ... }      //  V:
    // {"corrected", ... },                 //  V:
    // {"auto", ... },                      //  V:
    // {"auto_station", ... },              //  V:
    // {"maintenance_indicator_on", ... },  //  V:
    // {"no_signal", ... },                 //  V:
    // {"lightning_sensor_off", ... },      //  V:
    // {"freezing_rain_sensor_off", ... },  //  V:
    // {"present_weather_sensor_off", ... },//  V:
    // {"wx_string", ... },                 //  V: 
    {"sky_cover", set_clouds },             //  V: OVC
    {"cloud_base_ft_agl", set_clouds },     //  V: 15000
    {"flight_category", set_flight_category },  // V: VFR
    // {"three_hr_pressure_tendency_mb", ... }, //  V:
    // {"maxT_c", ... },                //  V:
    // {"minT_c", ... },                //  V:
    // {"maxT24hr_c", ... },            //  V:
    // {"minT24hr_c", ... },            //  V:
    // {"precip_in", ... },             //  V:
    // {"pcp3hr_in", ... },             //  V:
    // {"pcp6hr_in", ... },             //  V:
    // {"pcp24hr_in", ... },            //  V:
    // {"snow_in", ... },               //  V:
    // {"vert_vis_ft", ... },           // V:
    // {"metar_type", ... },            //  V: METAR
    // {"elevation_m", set_elevation },           // V: 37.0 (station elevation -- we don't care)
    { NULL, NULL }
};

// threadsafe
static bool _set_airport_kv(airport_t *airport, const char *key, const char *val)
{
    field_setter handler = (field_setter) match_kv(metar_fields, key);
    if (handler == NULL) return false;
    logDebug("set_airport_kv: Setting %s = %s\n", key, val);
    return handler(airport, key, val);
}

bool set_airport_kv(airport_t *airport, const char *key, const char *val)
{
    _lock();
    auto rc = _set_airport_kv(airport,key,val);
    _release();
    return rc;
}

static const char *wxConditionStrings[WX_COND_MAX] = { "VFR", "MVFR", "IFR", "LIFR" };

static lv_color_t wxConditionColors[WX_COND_MAX] = {
    lv_color_make(0,255,0),         // VFR - green
    lv_color_make(0,128,255),       // MVFR - blue
    lv_color_make(255,0,0),         // IFR - red
    lv_color_make(255,0,250)        // LIRF - purple
};
static lv_color_t invalid_wx_txt = lv_color_make(255,255,0);

static CRGB wxConditionLEDColors[WX_COND_MAX] = {
    CRGB(0,255,0),         // VFR - green
    CRGB(0,128,255),       // MVFR - blue
    CRGB(255,0,0),         // IFR - red
    CRGB(255,0,250)        // LIRF - purple
};
// yellow means no WX for airport yet.
static CRGB invalid_wx = CRGB::Yellow;
static CRGB lightning = CRGB::White;
static CRGB blink_off = CRGB::Black;

// not threadsafe.
static bool _show_airport(int n)
{
    if ( n < 0 || n >= num_airports) {
        logError("show_airport: Invalid index %d", n);
        return false;
    }
    update_current = n;
    logInfo("show_airport: index %d %s (%s)\n", n, airports[n].name, airports[n].full_name );
    return true;
}

bool show_airport(int n)
{
    _lock();
    auto rc = _show_airport(n);
    _release();
    return rc;
}

// find the closest airport to the current airport, in a given 
// direction (IR_KEY_{UP,DOWN,LEFT,RIGHT}) -- if 'dir' == 0,
// just find closest airport.
// not threadsafe.
bool _next_airport(char dir)
{
    airport_t *cur = airports + prefs.current_airport;
    float min_dist = FLT_MAX;
    int closest_index = -1;
    logDebug("next airport (%d: %s) dir: %c", prefs.current_airport, cur->name, dir);
    // 'xmult' and 'ymult' are used as 'penalties' for the distance in the 'wrong'
    // axis my multiplying the distance on that axis. i.e. making it 3 times 
    // more expensive to go 1KM up/down when looking for the closest east/west 
    float xmult, ymult;
    switch(dir) {
        case IR_KEY_DOWN:
        case IR_KEY_UP:
            xmult = 2; ymult = 1;
            break;
        case IR_KEY_LEFT:
        case IR_KEY_RIGHT:
            xmult = 1; ymult = 2;
            break;
        default:
            xmult = 1; ymult = 1;
            break;
    }

    for ( int i = 0; i < num_airports; i++ ) {
        airport_t *ap = airports + i;
        // skip current.
        if (i == prefs.current_airport) continue;

        switch(dir) {
            case IR_KEY_UP:
                if (ap->y > cur->y) {
                    logDebug("Skipping %s (%s), y=%.2f > %.2f\n", ap->name, ap->full_name, ap->y, cur->y); 
                    continue;
                }
                break;
            case IR_KEY_DOWN:
                if (ap->y < cur->y) {
                    logDebug("Skipping %s (%s), y=%.2f < %.2f\n", ap->name, ap->full_name, ap->y, cur->y); 
                    continue;
                }
                break;
            case IR_KEY_LEFT:
                if (ap->x > cur->x) {
                    logDebug("Skipping %s (%s), x=%.2f > %.2f\n", ap->name, ap->full_name, ap->x, cur->x); 
                    continue;
                }
                break;
            case IR_KEY_RIGHT:
                if (ap->x < cur->x) {
                    logDebug("Skipping %s (%s), x=%.2f < %.2f\n", ap->name, ap->full_name, ap->x, cur->x); 
                    continue;
                }
                break;
        }

        float dx = (ap->x - cur->x) * xmult;
        float dy = (ap->y - cur->y) * ymult;
        // since we're just comparing against other distances,
        // we don't need the SQRT().
        float dist = dx*dx + dy*dy;

        if (dist < min_dist) {
            logDebug("CLOSER: %d: %s (%s) dist=%f (was=%f)\n", i, ap->name, ap->full_name, dist, min_dist );
            closest_index = i;
            min_dist = dist;
        } else {
            logDebug("%d: %s (%s) dist=%f (closest=%f)\n", i, ap->name, ap->full_name, dist, min_dist );
        }
    }
    if (closest_index < 0 || closest_index >= num_airports) {
        logInfo("No close airport found? (%d, min dist: %f)\n", closest_index, min_dist);
        return false;
    }
    _show_airport(closest_index);
    return true;
}

bool next_airport(char dir)
{
    _lock();
    auto rc = _next_airport(dir);
    _release();
    return rc;
}

extern lv_img_dsc_t KCCR;

static bool load4bpp(const char *name, lv_img_dsc_t &dsc) 
{
    int got;

    logInfo("Loading %s\n", name);
    if (dsc.data) free((void*)dsc.data);
    dsc.data = NULL;
    dsc.data_size = 0;

    dsc.header.cf = LV_IMG_CF_INDEXED_4BIT;
    dsc.header.always_zero = 0;
    dsc.header.reserved = 0;

    int fd = open(name,O_RDONLY);
    if (fd == -1) {
        logError("Failed to open %s\n", name);
        return false;
    }
    uint8_t size[2];
    if (read(fd,size,2) != 2) {
        logError("Failed to read size of %s\n", name);
        goto bail;
    }
    logInfo("loading %s: w=%d, h=%d\n", name, size[0], size[1]);

    dsc.header.w = size[0];
    dsc.header.h = size[1];
    dsc.data_size = (dsc.header.w>>1) * dsc.header.h + (16 * 4);
    dsc.data = (uint8_t*) calloc(dsc.data_size,1);
    got = read(fd,(void*)dsc.data,dsc.data_size);
    if (got != dsc.data_size) {
        logError("failed to read image data from %s (got %d, expected %d)\n", name, got, dsc.data_size);
        goto bail;
    }
    close(fd);
    return true;
bail:
    if (dsc.data) free((void*)dsc.data); dsc.data = NULL;
    close(fd);
    return false;
}

#define AIRPORT_DATA_BUFFER_SIZE (256)
char airportDataBuffer[AIRPORT_DATA_BUFFER_SIZE];

const char *sprintfBuf(char *&pBuf, char *pEnd, const char *fmt, ... )
{
    va_list ap;
    va_start(ap,fmt);
    const char *p = pBuf;

    // logDebug("Formatting: %d bytes left\n", (pEnd - pBuf));
    int len = vsnprintf(pBuf, pEnd - pBuf, fmt, ap);
    // logDebug("printf returns %d\n", len);
    if (len < 0) {
        // logError("Failed to append '%s'\n", fmt);
        p = "ERR";
        goto done;
    }
    // advance buffer.
    pBuf += len+1;
done:
    va_end(ap);
    // logDebug("Done formatting: %d bytes left, return '%s'\n", (pEnd - pBuf), p);
    return p;
}

// not threadsafe
// updates the GUI elements associated with the current airport.
static void _update_current_airport(bool force = false)
{
    char *pBuf, *pEnd;
    const char *p;

    pBuf = airportDataBuffer;
    pEnd = airportDataBuffer+AIRPORT_DATA_BUFFER_SIZE;

    if (update_current == -1 && !force) return;

    prefs.current_airport = update_current;
    prefs_dirty = true;

    update_current = -1;

    airport_t *a = airports + prefs.current_airport;
    logDebug("update_current_airport: %d (%s; %s)\n", prefs.current_airport, a->name, a->full_name);

    if (a->valid_metar) {
        p = wxConditionStrings[a->wx_cond];
        lv_obj_set_style_text_color(ui_AirportCodeLabel, wxConditionColors[a->wx_cond], LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        p = "???";
        lv_obj_set_style_text_color(ui_AirportCodeLabel, invalid_wx_txt, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    lv_label_set_text_static(ui_AirportCodeLabel, sprintfBuf(pBuf, pEnd, "%s - %s", a->name, p));

    lv_label_set_text_static(ui_AirportNameLabel, a->full_name);

    logDebug("format metar (%x)\n", a->metar );
    bool valid;
    if (a->metar == NULL || strlen(a->metar) == 0) {
        p = "METAR: NO DATA" METAR_DOTS;
        valid = false;
    } else {
        p = sprintfBuf(pBuf, pEnd, "METAR: %s" METAR_DOTS, a->metar );
        valid = a->valid_metar;
    }

    lv_label_set_text_static( ui_MetarTicker, p );

    p = sprintfBuf(pBuf, pEnd, data_path("%s.4bp"), a->name);
    static lv_img_dsc_t dsc;
    if (load4bpp(p, dsc)) {
        lv_img_set_src(ui_AirportImage, &dsc );
    }

    // ui_WindLabel
    if (!valid) {
        p = "---";
        lv_label_set_text_static( ui_WindLabel, p );
        lv_label_set_text_static( ui_VisibilityLabel, p );
        lv_label_set_text_static( ui_CloudsLabel, p );
        lv_label_set_text_static( ui_AltimiterLabel, p );
        lv_label_set_text_static( ui_TemperatureLabel, p );
        lv_label_set_text_static( ui_DewpointLabel, p );
        lv_obj_add_flag(ui_windArrowImage, LV_OBJ_FLAG_HIDDEN );
    } else {
        if (a->wind_dir == 0 && a->wind_speed == 0) {
            // CALM
            lv_obj_add_flag(ui_windArrowImage, LV_OBJ_FLAG_HIDDEN );
            p = sprintfBuf(pBuf, pEnd, "Calm");
        } else if (a->wind_dir == 0) {
            // VARIABLE
            lv_obj_add_flag(ui_windArrowImage, LV_OBJ_FLAG_HIDDEN );
            p = sprintfBuf(pBuf, pEnd, "Var.\n%d KTS", a->wind_speed);
        } else {
            lv_obj_clear_flag(ui_windArrowImage, LV_OBJ_FLAG_HIDDEN );
            lv_img_set_angle(ui_windArrowImage, (int)(((a->wind_dir+180) % 360) * 10));
            // TODO: calculate pref. runway and xwind component, color arrow accordingly.
            // lv_obj_set_style_img_recolor(ui_windArrowImage, lv_color_hex(0xF8034F), LV_PART_MAIN | LV_STATE_DEFAULT);
            p = sprintfBuf(pBuf, pEnd, "%d\n%d KTS", a->wind_dir, a->wind_speed);
        }
        if (a->wind_gust > 0) {
            pBuf--; // clobber trailing '\0' from previous sprintfBuf.
            sprintfBuf(pBuf,pEnd, "\nG %d", a->wind_gust);
        }
        lv_label_set_text_static( ui_WindLabel, p );

        // ui_VisibilityLabel
        p = sprintfBuf(pBuf, pEnd, "%.0f SM", a->vis);
        lv_label_set_text_static( ui_VisibilityLabel, p );

        // ui_CloudsLabel
        p = sprintfBuf(pBuf, pEnd, "");
        bool cover = false;
        for (int i = 0 ; i < a->cloud_idx; i++ ) {
            clouds_t *c = a->clouds + i;
            if (c->sky_cover == -1 || c->altitude == -1) break;
            pBuf--;
            cover = true;
            sprintfBuf(pBuf, pEnd,"%s %d\n", sky_cover[c->sky_cover], c->altitude);
        }
        if (cover) {
            // clobber trailing newline.
            if (pEnd[-1] == '\n') pEnd[-1] = '\0';    
            lv_label_set_text_static( ui_CloudsLabel, p );
        } else {
            lv_label_set_text_static( ui_CloudsLabel, "Clear" );
        }

        // ui_AltimiterLabel
        float pa = PA(a->altimiter, a->elevation);
        float isa = ISA(pa);
        float da = DA(pa,a->temp_c,isa);
        p = sprintfBuf(pBuf, pEnd, "%.2f\" Hg\nDA %d'", a->altimiter, (int)da);
        lv_label_set_text_static( ui_AltimiterLabel, p );

        // ui_TemperatureLabel
        p = sprintfBuf(pBuf, pEnd, "%d C\n%d F", (int)a->temp_c, (int)CtoF(a->temp_c));
        lv_label_set_text_static( ui_TemperatureLabel, p );

        // ui_DewpointLabel
        p = sprintfBuf(pBuf, pEnd, "%d C\n%d F", (int)a->dew_c, (int)CtoF(a->dew_c));
        lv_label_set_text_static( ui_DewpointLabel, p );
    }
    logDebug("done. %d/%d bytes used.\n", pEnd - pBuf, AIRPORT_DATA_BUFFER_SIZE );
}

void ledsOff()
{
  for (int i = 0; i < num_airports; i++) { t_leds[i] = leds[i] = CRGB::Black; }
  FastLED.show();  
}

static void airport_refresh_task(void *params);

// not threadsafe
void airportsBegin()
{
    // TODO: display something if this fails.
    load_airports();

    // tell FastLED about the LED strip configuration
    // allocate twice as many as we have airports, because we always fade from t_leds[n] -> leds[n].
    leds = (CRGB*) calloc(num_airports*2, sizeof(CRGB));
    t_leds = leds + num_airports;

    FastLED.addLeds<LED_TYPE,FASTLED_DATA_PIN,COLOR_ORDER>(leds, num_airports).setCorrection(UncorrectedColor);

    // set master brightness control
    FastLED.setBrightness(10);
    ledsOff();

    // start airport refresh task.
    TaskHandle_t handle;
    BaseType_t rc;
    rc = xTaskCreate(airport_refresh_task, "refresh", 
            10000,      // stack size
            NULL,       // parameters
            tskIDLE_PRIORITY,  // prio
            &handle);
    if (rc != pdPASS) {
        logError("airportsBegin: failed to start airport_refresh_task\n");
    } else {
        logInfo("airportsBegin: created refresh task\n");
    }
}

static bool airport_blink_state = false;
static int airport_blink_timer = 0;
static int last_airport = -1;
static int airport_blink_enable = 0;

// not threadsafe
void airport_blink(bool enable, int how_long)
{
    if (enable) {
        logInfo("Enable blink %d\n", how_long);
        airport_blink_enable = how_long;
        airport_blink_timer = AIRPORT_BLINK_RATE;
        airport_blink_state = true;
    } else {
        logInfo("Disable blink\n");
        airport_blink_enable = 0;
        airport_blink_timer = 0;
    }
}

static bool fetch_next_airport()
{
    time_t now;
    time(&now);
    bool updated = false;
#define MAX_AIRPORT_UPDATES (32)
    airport_t *updates[MAX_AIRPORT_UPDATES+1];
    int n = 0;
    bool update_cur = false;

    for ( int i = 0 ; i < num_airports; i++ ) {
        // start with current airport, work our way around.
        auto num = (prefs.current_airport + i) % num_airports;
        auto a = airports + num;
        auto age = now - a->last_metar;
        if (age > WX_STALE_TIME) {
            logDebug("[%d] WX for %s is %d seconds old.\n", now, a->name, age);
            updates[n++] = a;
            updates[n] = NULL;
            if (n == MAX_AIRPORT_UPDATES) {
                logDebug("Updating %d airports...\n", n);
                _lock();
                update_airport_wx(updates);
                n = 0;
                _release();
                updated = true;
            }
            // trigger refresh of GUI
            if (num == prefs.current_airport) update_cur = true;
        }
    }
    // any left over airports ... update them.
    if (n > 0) {
        logDebug("Updating %d airports (end)...\n", n);
        _lock();
        update_airport_wx(updates);
        n = 0;
        _release();
    }
    if (update_cur == true) {
        show_airport(prefs.current_airport);
    }
    return updated;
}

static void airport_refresh_task(void *params)
{
    logInfo("airport_refresh_task_begin\n");
    while( true ) {
        if (WiFi.status() != WL_CONNECTED) {
            logDebug("wifi not ready.  sleeping.\n");
            // wait 3 seconds.
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
            continue;
        }
        // logDebug("fetch next airport.\n");
        bool fetched = fetch_next_airport();
        // logDebug("sleeping.\n");
        if (!fetched) vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

// shamelessly stolen from https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
void nblendU8TowardU8( uint8_t &cur, const uint8_t &target, uint8_t amount)
{
    if (cur == target) return;
    if (cur < target) {
        uint8_t d = target - cur;
        d = scale8_video(d, amount);
        cur += d;
    } else {
        uint8_t d = target - cur;
        d = scale8_video(d, amount);
        cur -= d;
    }
}

void fadeTowardColor(CRGB& cur, const CRGB& target, uint8_t amount)
{
    nblendU8TowardU8( cur.r, target.r, amount );
    nblendU8TowardU8( cur.g, target.g, amount );
    nblendU8TowardU8( cur.b, target.b, amount );
}

void airportsLoop()
{
    int ticks = millis();
    int elapsed = ticks - prev_ticks;

    // 30hz update rate..
    if (elapsed < 1000/30) return;

    prev_ticks = ticks;

    _update_current_airport();

    // is the blinking 'cursor' enabled?
    if (airport_blink_enable > 0) {
        airport_blink_enable -= elapsed;
        if (airport_blink_enable <= 0) {
            airport_blink(false);
        }
    }

    // blink the cursor.
    if (airport_blink_timer > 0) {
        airport_blink_timer -= elapsed;
        if (airport_blink_timer <= 0) {
            logInfo("BLINK %d (%d)\n", !airport_blink_state, airport_blink_timer);
            airport_blink_timer = AIRPORT_BLINK_RATE;
            airport_blink_state = !airport_blink_state;
        }
    }

    // update airport colors.
    // TODO: dusk/night/dawn
    for (int i = 0; i < num_airports; i++ ) {
        airport_t *ap = airports + i;

        // blink this airport (cursor)?
        if (i == prefs.current_airport && airport_blink_enable && airport_blink_state) {
            // logInfo("leds[%s] = blink_off (blink_enable=%d, blink_state=%d, blink_timer=%d)\n",ap->name, airport_blink_enable, airport_blink_state, airport_blink_timer);
            t_leds[i] = blink_off;
            continue;
        }
        if (ap->metar == NULL || ap->valid_metar == false) {
            // no weather for this airport.
            // if (i == prefs.current_airport) logInfo("leds[%s] = invalid_wx\n",ap->name);
            t_leds[i] = invalid_wx;
            continue;
        }

        // now set LED according to condition.

        // what about lightning?
        if (ap->lightning) {
            ap->last_flash -= elapsed;
            if (ap->last_flash <= 0) {
                // no fading, so set leds and t_leds to same color.
                leds[i] = t_leds[i] = lightning;
                // if (i == prefs.current_airport) logInfo("leds[%s] = lightning\n",ap->name);
                // 1-3 seconds later for lightning
                // TODO: lightning intensity?
                ap->last_flash += random(1000,3000);
            }
            continue;
        }

        if (ap->wx_cond < 0 || ap->wx_cond >= WX_COND_MAX) {
            logError("Invalid wx_cond for %s: %d\n", ap->name, ap->wx_cond);
            if (i == prefs.current_airport) logInfo("leds[%s] = ERROR\n",ap->name);
            t_leds[i] = CRGB::Violet;
            ap->wx_cond = 0;
            continue;
        }
        t_leds[i] = wxConditionLEDColors[ap->wx_cond];
        // if (i == prefs.current_airport) logInfo("leds[%s] = wx cond %d (0x%04.4x)\n",ap->name,ap->wx_cond,leds[i]);
    }

    for (int i = 0; i < num_airports; i++ ) {
        // fade LEDs.
#define FADE_SPEED (5)
        // fadeTowardColor( leds[i], t_leds[i], FADE_SPEED);
        leds[i].r = INT_LERP(leds[i].r, t_leds[i].r, 0.2);
        leds[i].g = INT_LERP(leds[i].g, t_leds[i].g, 0.2);
        leds[i].b = INT_LERP(leds[i].b, t_leds[i].b, 0.2);
    }

    if (targetBrightness != currentBrightness) {
        currentBrightness = INT_LERP(currentBrightness, targetBrightness, 0.1);
        FastLED.setBrightness(currentBrightness>>8);
    }
    FastLED.show();
}
