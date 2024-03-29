#pragma once


#include <Wire.h>

#define TOUCH_I2C_ADD 0x38

#define TOUCH_REG_XL 0x04
#define TOUCH_REG_XH 0x03
#define TOUCH_REG_YL 0x06
#define TOUCH_REG_YH 0x05

// I2C includes for touch display
const int i2c_touch_addr = TOUCH_I2C_ADD;
#define LCD_BL 46
#define SDA_FT6236 38
#define SCL_FT6236 39

#ifdef __cplusplus
extern "C" {
#endif

int readTouchReg(int reg);

int getTouchPointX();

int getTouchPointY();

void ft6236_pos(int pos[2]);

#ifdef __cplusplus
} /*extern "C"*/
#endif