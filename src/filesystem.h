#ifndef _H_FILESYSTEM_
#define _H_FILESYSTEM_

// Define the filesystem to be used. For now just SPIFFS.
// you want to port to something else, it must have the same
// interface as SPIFFS.
#define FILESYSTEM SPIFFS

#include <SPIFFS.h>     // Filesystem support header
//#include <LittleFS.h>   // Filesystem support header

// returns FALSE on error.
bool beginFilesystem();

#endif // _H_FILESYSTEM_
