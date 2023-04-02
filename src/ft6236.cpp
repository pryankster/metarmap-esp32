#include "FT6236.h"
#include "log.h"

static bool firstTouch;

void beginFT2636()
{
    Wire.begin(I2C_PIN_SDA, I2C_PIN_SCL);
    uint8_t error, address;

    Wire.beginTransmission(TOUCH_I2C_ADD);
    error = Wire.endTransmission();

    if (error == 0)
    {
        logInfo("Touch device found at 0x%02x\n", TOUCH_I2C_ADD );
    }
    else
    {
        logError("Touch device NOT FOUND at 0x%02x\n", TOUCH_I2C_ADD );
    }
    firstTouch = true;
}

int readTouchReg(int reg)
{
    int data = 0;
    Wire.beginTransmission(TOUCH_I2C_ADD);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(TOUCH_I2C_ADD, 1);
    if (Wire.available())
    {
        data = Wire.read();
    }
    return data;
}

int getTouchPointX()
{
    int XL = 0;
    int XH = 0;

    XH = readTouchReg(TOUCH_REG_XH);
    //Serial.println(XH >> 6,HEX);
    if(XH >> 6 == 1)
        return -1;
    XL = readTouchReg(TOUCH_REG_XL);

    return ((XH & 0x0F) << 8) | XL;
}

int getTouchPointY()
{
    int YL = 0;
    int YH = 0;

    YH = readTouchReg(TOUCH_REG_YH);
    YL = readTouchReg(TOUCH_REG_YL);

    return ((YH & 0x0F) << 8) | YL;
}

bool ft6236_pos(int pos[2])
{
    int XL = 0;
    int XH = 0;
    int YL = 0;
    int YH = 0;

    XH = readTouchReg(TOUCH_REG_XH);
    if(XH >> 6 == 1)
    {
        pos[0] = -1;
        pos[1] = -1;
        return false;
    }
    XL = readTouchReg(TOUCH_REG_XL);
    YH = readTouchReg(TOUCH_REG_YH);
    YL = readTouchReg(TOUCH_REG_YL);

    pos[0] = ((XH & 0x0F) << 8) | XL;
    pos[1] = ((YH & 0x0F) << 8) | YL;

    // until the first touch, chip reports 0,0 as a valid touch
    if (firstTouch && pos[0] == 0 && pos[1] == 0) return false;

    firstTouch = false;
    return true;
}