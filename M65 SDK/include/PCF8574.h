#ifndef __PCF8574_H__
#define __PCF8574_H__

#include "ql_type.h"

/** defines the data direction (reading from I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_READ    1

/** defines the data direction (writing to I2C device) in i2c_start(),i2c_rep_start() */
#define I2C_WRITE   0
#define PCF8574_ADDRBASE (0x20) //device base address

#define PCF8574_MAXDEVICES 1 //max devices, depends on address (3 bit)
#define PCF8574_MAXPINS 8 //max pin per device

//pin status
volatile u8 pcf8574_pinstatus[PCF8574_MAXDEVICES];

void pcf8574_init();
extern u8 pcf8574_getoutput(u8 deviceid);
extern u8 pcf8574_getoutputpin(u8 deviceid, u8 pin);
extern u8 pcf8574_setoutput(u8 deviceid, u8 data);
extern u8 pcf8574_setoutputpins(u8 deviceid, u8 pinstart, u8 pinlength, u8 data);
extern u8 pcf8574_setoutputpin(u8 deviceid, u8 pin, u8 data);
extern u8 pcf8574_setoutputpinhigh(u8 deviceid, u8 pin);
extern u8 pcf8574_setoutputpinlow(u8 deviceid, u8 pin);
extern u8 pcf8574_getinput(u8 deviceid);
extern u8 pcf8574_getinputpin(u8 deviceid, u8 pin);


#endif // __PCF8574_H__