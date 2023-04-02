#include <Arduino.h>
#include <HTTPClient.h>
#include "metar.h"
#include "log.h"
#include "msgbox.h"

static const String metarUrl = "https://www.aviationweather.gov/adds/dataserver_current/httpparam?"
    "dataSource=metars&requestType=retrieve&hoursBeforeNow=6&mostRecentForEachStation=true&format=csv"
    "&stationString=";

bool update_airport_wx(airport_t **airports)
{
    HTTPClient http;
    airport_t *airport;
    airport_t **a;
    String url = metarUrl;
    time_t now;
    time(&now);
    bool first=true;
    for (a = airports; *a; a++) {
        airport = *a;
        const char *weather = (airport->weather ? airport->weather : airport->name);
        if (!first) url += ",";
        url += weather;
        first = false;

        // we set last metar fetch time, so we don't keep trying if it fails.
        // see comments in airports.c about why we do this!
        airport->last_metar = now;
        airport->cloud_idx = 0;
        airport->valid_metar = false;
        set_airport_metar(airport, "data unavailable");
    }

    http.begin(url.c_str());

    logInfo("Fetching %s\n",url.c_str());
    int rc = http.GET();
    if (rc <= 0) {
        http.end();
        logError("HTTP GET %s: ERROR: %d %s\n", url.c_str(), rc, http.errorToString(rc).c_str() );
        showMessagef(5000,"Error fetching METAR: %s", http.errorToString(rc)); 
        http.end();
        return false;
    }
    logDebug("HTTP GET %s: rc=%d size=%d\n", url.c_str(), rc, http.getSize());
    auto stream = http.getStreamPtr();
    if (stream == NULL) {
        logError("HTTP Get: failed to get stream pointer?\n");
        http.end();
        showMessage(5000, "Error fetching METAR: failed to get stream");
        return false;
    }
    // now read the stream...
    // first line should be "No warnings"
    String tmp;
    int num_results = 0;
    while (true) {
        tmp = stream->readStringUntil('\n');
        // logDebug("HTTP GET: READ: %s\n",tmp.c_str());
        if (tmp == "") {
            logError("HTTP GET: incomplete results\n");
            set_airport_metar(airport, "Error fetching METAR: incomplete results");
            http.end();
            return false;
        }
        if (tmp.endsWith(" results")) {
            num_results = atoi(tmp.c_str());
            if (num_results == 0) {
                logError("HTTP GET: No results returned\n");
                http.end();
                return false;
            }
            break;
        }
    }

    String keyStr = stream->readStringUntil('\n');
    int kLen = keyStr.length();
    int kStart = 0, kEnd;
    int cols = 0;
    // first, count columns.
    // logDebug("Counting columns\n");
    while (kStart < kLen) {
        cols++;
        kEnd = keyStr.indexOf(',',kStart);
        if (kEnd == -1) kEnd = kLen;
        kStart = kEnd + 1;
    }
    // logDebug("%d columns\n", cols);

    // now allocate holders for keys and values.
    String keys[cols];
    String vals[cols];

    // now split keys
    kStart = 0; 
    int col = 0;
    // logDebug("Splitting keys\n");
    while (kStart < kLen) {
        kEnd = keyStr.indexOf(',',kStart);
        if (kEnd == -1) kEnd = kLen;
        keys[col] = keyStr.substring(kStart,kEnd);
        // logDebug("Col %d: %s\n", col, keys[col].c_str());
        col++;
        kStart = kEnd + 1;
    }

    // find 'station' column...
    int station_col = -1;
    for(auto i = 0; i < cols; i++) {
        if (keys[i] == "station_id") {
            station_col = i;
            break;
        }
    }
    if (station_col == -1) {
        logError("No 'station_id' column returned in results?\n");
        return false;
    }
    // logDebug("station_id column = %d\n", station_col);

    // logDebug("Processing results\n");
    int n = 0;
    while (n++ < num_results) {
        String valStr = stream->readStringUntil('\n');
        // logDebug("RESULT %d: '%s'\n", n, valStr.c_str());
        int vLen = valStr.length();
        int vStart = 0, vEnd;
        col = 0;
        while (vStart < vLen) {
            vEnd = valStr.indexOf(',',vStart);
            if (vEnd == -1) vEnd = vLen;
            vals[col++] = valStr.substring(vStart,vEnd);
            vStart = vEnd + 1;
        }

        const char *station = vals[station_col].c_str();

        // logDebug("Looking for station %s\n", station);
        for(a = airports; *a; a++) {
            airport = *a;
            const char *weather = (airport->weather ? airport->weather : airport->name);
            if (strcmp(weather,station)==0) {
                logDebug("found %s: (%s)\n", weather, airport->full_name);
                for (auto i = 0; i < cols; i++) {
                    const char *k = keys[i].c_str();
                    const char *v = vals[i].c_str();
                    // logInfo("update_airport_wx[%d]: K: %s  V: %s\n", i, k, v);
                    set_airport_kv(airport,k,v);
                }
                airport->valid_metar = true;
                break;
            }
        }
        if (*a == NULL) {
            logError("Didn't find airport for station_id '%s'\n", station);
        }
    }
    http.end();
    return true;
}