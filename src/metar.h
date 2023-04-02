#ifndef _H_METAR_H_
#define _H_METAR_H_

#include "airports.h"

bool update_airport_wx(airport_t **airports);
void set_airport_metar(airport_t *airport, const char *val);

#endif // _H_METAR_H_