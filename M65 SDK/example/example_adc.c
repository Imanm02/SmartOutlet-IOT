/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2019
*
*****************************************************************************/
/*****************************************************************************
*
* Filename:
* ---------
*   example_adc.c
*
* Project:
* --------
*   OpenCPU
*
* Description:
* ------------
*   This example demonstrates how to program ADC interface in OpenCPU.
*
*   All debug information will be output through DEBUG port.
*
* Usage:
* ------
*   Compile & Run:
*
*     Set "C_PREDEF=-D __EXAMPLE_ADC__" in gcc_makefile file. And compile the 
*     app using "make clean/new".
*     Download image bin to module to run.
*
* Author:
* -------
* -------
*
*============================================================================
*             HISTORY
*----------------------------------------------------------------------------
* 
****************************************************************************/
#ifdef __EXAMPLE_ADC__
#include "ql_trace.h"
#include "ql_system.h"
#include "ql_adc.h"
#include "ql_uart.h"
#include "ql_stdlib.h"
#include "ql_error.h"


#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   512
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT,...) {\
    Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);\
    Ql_sprintf(DBG_BUFFER,FORMAT,##__VA_ARGS__); \
    if (UART_PORT2 == (DEBUG_PORT)) \
    {\
        Ql_Debug_Trace(DBG_BUFFER);\
    } else {\
        Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8*)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER)));\
    }\
}
#else
#define APP_DEBUG(FORMAT,...) 
#endif

Enum_PinName adcPin = PIN_ADC0;
static u32 ADC_timer = 0x101;
static u32 ADC_time_Interval = 1000;
static s32 m_param = 0;
#define   ADC_COUNT    10

static void ADC_Timer_handler(u32 timerId, void* param);

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
     
}

// timer callback function
void ADC_Timer_handler(u32 timerId, void* param)
{
	 u16 adcvalue = 0;
	 
    *((s32*)param) +=1;
    if(ADC_timer == timerId)
    {
        // stack_timer repeat 
        if(*((s32*)param) >= ADC_COUNT)
        {
            Ql_Timer_Stop(ADC_timer);
			APP_DEBUG("<-- ADC closed(%d) -->\r\n",Ql_ADC_Close(adcPin));
        } 
		else
		{
			Ql_ADC_Read(adcPin,&adcvalue);
		    APP_DEBUG("<-- read voltage(mV)=%d -->\r\n",adcvalue);
		}
    }
}

void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;

    // Register & open UART port
    ret = Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to register serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }
    ret = Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("Fail to open serial port[%d], ret=%d\r\n", UART_PORT1, ret);
    }
    APP_DEBUG("\r\n<-- OpenCPU: ADC Example -->\r\n") 

	//open  adc ;
    ret = Ql_ADC_Open(adcPin,ADC_PERIOD_1MS);
    if(ret < 0)
    {
        APP_DEBUG("\r\n<--failed!!adc open failed-->\r\n",ret);        
    }
    APP_DEBUG("\r\n<--adc open successful-->\r\n");
	
    //register  a timer
    ret = Ql_Timer_Register(ADC_timer, ADC_Timer_handler, &m_param);
    if(ret <0)
    {
        APP_DEBUG("\r\n<--failed!!, Ql_Timer_Register: timer(%d) fail ,ret = %d -->\r\n",ADC_timer,ret);
    }
    APP_DEBUG("\r\n<--Register: timerId=%d, param = %d,ret = %d -->\r\n", ADC_timer ,m_param,ret); 

    //start a timer,repeat=true;
    ret = Ql_Timer_Start(ADC_timer,ADC_time_Interval,TRUE);
    if(ret < 0)
    {
        APP_DEBUG("\r\n<--failed!! stack timer Ql_Timer_Start ret=%d-->\r\n",ret);        
    }
    APP_DEBUG("\r\n<--adc timer Ql_Timer_Start(ID=%d,Interval=%d,) ret=%d-->\r\n",ADC_timer,ADC_time_Interval,ret);

    // Start message loop of this task
    while (TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
        case MSG_ID_USER_START:
            break;
        default:
            break;
        }
    }
}

#endif // __EXAMPLE_ADC__
