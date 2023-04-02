#ifndef _H_FT6236_
#define _H_FT6236_

#include <Wire.h>

#define TOUCH_I2C_ADD 0x38

#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05

void beginFT2636();

int readTouchReg(int reg);

int getTouchPointX();

int getTouchPointY();

bool ft6236_pos(int pos[2]);

#endif // _H_FT6236_