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
 *   example_dfota_http.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This example demonstrates how to use dfota_http function with TCP APIs in OpenCPU.
 *   Input the specified command through any uart port and the result will be 
 *   output through the debug port.
 *   App bin must be put in server.It will be used to upgrade data through the air.
 *
 * Usage:
 * ------
 *   Compile & Run:
 *
 *     Set "C_PREDEF=-D __EXAMPLE_DFOTA_HTTP__" in gcc_makefile file. And compile the 
 *     app using "make clean/new".
 *     Download image bin to module to run.
 * 
 *   Operation:
 *  
 *     step 1: you must put your application bin in your server.
 *     step 2: replace the "APP_PACK_URL" with your own .
 *     step 3: input string : start dfota=XXXX, XXXX stands for URL.
 *
 *     The URL format for http is:   http://hostname:port/filePath/fileName                                
 *     NOTE:  if ":port" is be ignored, it means the port is http default port(80) 
 *
 *     eg1: http://23.11.67.89/file/xxx.pack
 *     eg2: http://www.quectel.com:8080/file/xxx.pack
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#ifdef __EXAMPLE_DFOTA_HTTP__
#include "custom_feature_def.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_error.h"
#include "ql_gprs.h"
#include "ql_stdlib.h"
#include "ql_timer.h"
#include "dfota_http.h"
#include "ril.h"
#include "ril_network.h"

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

/****************************************************************************
* Define local param
****************************************************************************/
#define APN      "cmnet"
#define USERID   ""
#define PASSWD   ""

#define APP_PACK_URL   "http://220.180.xxx.xxx:8043/Test/MC25_DFotahttp_app.pack"  

#define URL_LEN 512
#define READ_BUF_LEN 1024

static u8 m_URL_Buffer[URL_LEN];
static u8 m_Read_Buffer[READ_BUF_LEN];

/****************************************************************************
* Define local function
****************************************************************************/
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);

/****************************************************************************
* Main task
****************************************************************************/

void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG  msg;

    // Register & open UART port
    Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT1, 115200, FC_NONE);

    Ql_UART_Register(UART_PORT2, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(UART_PORT2, 115200, FC_NONE);

    APP_DEBUG("\r\n<-- OpenCPU: dfota http! -->\r\n");  
    while(TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
            case MSG_ID_RIL_READY:
			{
                APP_DEBUG("<-- RIL is ready! -->\r\n");
                Ql_RIL_Initialize();
            }
            break;
			case MSG_ID_URC_INDICATION:
                switch (msg.param1)
                {
                    case URC_SIM_CARD_STATE_IND:
                    if (SIM_STAT_READY == msg.param2)
                    {
                        APP_DEBUG("<-- SIM card is ready -->\r\n");
                    }
                    break;
		            case URC_GPRS_NW_STATE_IND:
                    if (NW_STAT_REGISTERED == msg.param2 || NW_STAT_REGISTERED_ROAMING == msg.param2)
                    {
                        APP_DEBUG("<-- Users can start DFOTA test-->\r\n");
                    }
					else
					{
                        APP_DEBUG("<-- GPRS network status:%d,please wait!!! -->\r\n", msg.param2);
                    }
					break;
		        }
                break;
            default:
                break;
        }
        
    }
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
	//APP_DEBUG("CallBack_UART_Hdlr: port=%d, event=%d, level=%d, p=%x\r\n", port, msg, level, customizedPara);
	u32 rdLen=0;
	char *p=NULL;
	char *q = NULL;

	if(UART_PORT1 == port || UART_PORT2 == port)
	{
		switch (msg)
		{
			case EVENT_UART_READY_TO_READ:
			{
				
				Ql_memset(m_Read_Buffer, 0x0, sizeof(m_Read_Buffer));
				rdLen = ReadSerialPort(port, m_Read_Buffer, READ_BUF_LEN);

				//start dfota=xxx.pack
				p = Ql_strstr(m_Read_Buffer, "start dfota=");
				if(p)
				{
					ST_GprsConfig apnCfg;
					Ql_memcpy(apnCfg.apnName,	APN, Ql_strlen(APN));
					Ql_memcpy(apnCfg.apnUserId, USERID, Ql_strlen(USERID));
					Ql_memcpy(apnCfg.apnPasswd, PASSWD, Ql_strlen(PASSWD));

					q = Ql_strstr(m_Read_Buffer, "\r\n");
					if(q)
					{
						*q = '\0';
					}
					p += Ql_strlen("start dfota=");
					Ql_memset(m_URL_Buffer, 0, URL_LEN);
					
					//http://hostname:port/filePath/fileName
					Ql_sprintf(m_URL_Buffer, "%s%s",APP_PACK_URL,p);
					APP_DEBUG("\r\n<-- URL:%s-->\r\n",m_URL_Buffer);
					
					Ql_DFOTA_StartUpgrade(m_URL_Buffer, &apnCfg, NULL);
					break;
				}
				APP_DEBUG("\r\n<-- Not found this command, please check you command -->\r\n");
			 }
				break;
			default:
			break;
		}

	}
} 
#endif /* __EXAMPLE_FOTA_HTTP__ */

