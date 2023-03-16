#ifndef _H_TOUCH_
#define _H_TOUCH_

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The FILESYSTEM file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData"

extern void touch_calibrate(bool forceCal);

#endif