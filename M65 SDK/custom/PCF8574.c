#include "ril.h"
#include "ril_util.h"
#include "ril_sms.h"
#include "ril_telephony.h"
#include "ril_system.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_system.h"
#include "ql_memory.h"
#include "ql_timer.h"
#include "ql_gpio.h"
#include "ql_adc.h"
#include "ql_iic.h"
#include "PCF8574.h"



void pcf8574_init()
{
    u8 i = 0;
    for (i = 0; i < PCF8574_MAXDEVICES; i++)
        pcf8574_pinstatus[i] = 0;
}

u8 pcf8574_getoutput(u8 deviceid)
{
    u8 data = -1;
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES))
    {
        data = pcf8574_pinstatus[deviceid];
    }
    return data;
}

u8 pcf8574_getoutputpin(u8 deviceid, u8 pin)
{
    u8 data = -1;
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pin >= 0 && pin < PCF8574_MAXPINS))
    {
        data = pcf8574_pinstatus[deviceid];
        data = (data >> pin) & 0b00000001;
    }
    return data;
}

u8 pcf8574_setoutput(u8 deviceid, u8 data)
{
    s32 ret;
    u8 a[1]={0};
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES))
    {
        pcf8574_pinstatus[deviceid] = data;
        a[0]=data;
        ret = Ql_IIC_Write(1, (((PCF8574_ADDRBASE + deviceid) << 1) | I2C_WRITE), a, 1);

        return 0;
    }
    return -1;
}

u8 pcf8574_setoutputpins(u8 deviceid, u8 pinstart, u8 pinlength, u8 data)
{
    // example:
    // actual data is         0b01101110
    // want to change              ---
    // pinstart                    4
    // data                        101   (pinlength 3)
    // result                 0b01110110
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pinstart - pinlength + 1 >= 0 && pinstart - pinlength + 1 >= 0 && pinstart < PCF8574_MAXPINS && pinstart > 0 && pinlength > 0))
    {
        s32 ret;
        u8 b = 0;
        u8 a[1]={0};
        b = pcf8574_getinput(0);
        u8 mask = ((1 << pinlength) - 1) << (pinstart - pinlength + 1);
        data <<= (pinstart - pinlength + 1);
        data &= mask;
        b &= ~(mask);
        b |= data;
        pcf8574_pinstatus[deviceid] = b;
        a[0]=b;
        // update device
        ret = Ql_IIC_Write(1, (((PCF8574_ADDRBASE + deviceid) << 1) | I2C_WRITE), a, 1);

        return 0;
    }
    return -1;
}

u8 pcf8574_setoutputpin(u8 deviceid, u8 pin, u8 data)
{
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pin >= 0 && pin < PCF8574_MAXPINS))
    {
        s32 ret;
        u8 a[1]={0};
        u8 b = 0;
        b = pcf8574_getinput(0);//pcf8574_pinstatus[deviceid];
        b = (data != 0) ? (b | (1 << pin)) : (b & ~(1 << pin));
        pcf8574_pinstatus[deviceid] = b;
        a[0]=b;
        // update device
        ret = Ql_IIC_Write(1, (((PCF8574_ADDRBASE + deviceid) << 1) | I2C_WRITE), a, 1);

        return 0;
    }
    return -1;
}

u8 pcf8574_setoutputpinhigh(u8 deviceid, u8 pin)
{
    return pcf8574_setoutputpin(deviceid, pin, 1);
}

u8 pcf8574_setoutputpinlow(u8 deviceid, u8 pin)
{
    return pcf8574_setoutputpin(deviceid, pin, 0);
}

u8 pcf8574_getinput(u8 deviceid)
{
    s32 ret;
    u8 data[1]={0};
    
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES))
    {
        ret = Ql_IIC_Read(1, (((PCF8574_ADDRBASE + deviceid) << 1) | I2C_READ), data, 1); // read data


        // data[0] = ~data[0];

    }
    return data[0];
}

u8 pcf8574_getinputpin(u8 deviceid, u8 pin)
{
    u8 data = -1;
    if ((deviceid >= 0 && deviceid < PCF8574_MAXDEVICES) && (pin >= 0 && pin < PCF8574_MAXPINS))
    {
        data = pcf8574_getinput(deviceid);
        if (data != -1)
        {
            data = (data >> pin) & 0b00000001;
        }
    }
    return data;
}