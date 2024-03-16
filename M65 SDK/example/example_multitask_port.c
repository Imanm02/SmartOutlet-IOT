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
 *   example_multitask_port.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This example demonstrates how to use multitask_port function with APIs in OpenCPU.
 *   Input the specified command through any uart port and the result will be 
 *   output through the debug port.
 *
 * Usage:
 * ------
 *   Compile & Run:
 *
 *     Set "C_PREDEF=-D __EXAMPLE_MULTITASK_PORT__" in gcc_makefile file. And compile the 
 *     app using "make clean/new".
 *     Download image bin to module to run.
 * 
 *   Operation:
 *            
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
#ifdef __EXAMPLE_MULTITASK_PORT__
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_system.h"
#include "ql_stdlib.h"

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



#define SERIAL_RX_BUFFER_LEN  2048
static u8 m_RxBuf_VirtualPort1[SERIAL_RX_BUFFER_LEN];
static u8 m_RxBuf_UartPort1[SERIAL_RX_BUFFER_LEN];
static u8 m_RxBuf_UartPort2[SERIAL_RX_BUFFER_LEN];

static void Callback_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);

/**************************************************************
* Main Task
***************************************************************/
void proc_main_task(s32 taskId)
{
    s32 ret;
    ST_MSG msg;
    Enum_SerialPort mySerialPort = VIRTUAL_PORT1;

    // Register & open virtual port
    ret = Ql_UART_Register(mySerialPort, Callback_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Fail to register serial port[%d], ret=%d-->\r\n", mySerialPort, ret);
    }
    ret = Ql_UART_Open(mySerialPort, 0, 0);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Fail to open serial port[%d], ret=%d-->\r\n", mySerialPort, ret);
    }
    Ql_Debug_Trace("<--OpenCPU: example_multitask_port-->\r\n");
    Ql_UART_ClrRxBuffer(mySerialPort);
    while(TRUE)
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

/**************************************************************
* Subtask1
***************************************************************/
void proc_subtask1(s32 TaskId)
{
    s32 ret;
    ST_MSG msg;
    ST_UARTDCB dcb;
    Enum_SerialPort mySerialPort = UART_PORT1;

    Ql_Debug_Trace("<--OpenCPU: proc_subtask1-->\r\n");

    dcb.baudrate = 115200;
    dcb.dataBits = DB_8BIT;
    dcb.stopBits = SB_ONE;
    dcb.parity   = PB_NONE;
    dcb.flowCtrl = FC_NONE;
    ret = Ql_UART_Register(mySerialPort, Callback_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Ql_UART_Register(mySerialPort=%d)=%d-->\r\n", mySerialPort, ret);
    }
    ret = Ql_UART_OpenEx(mySerialPort, &dcb);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Ql_UART_OpenEx(mySerialPort=%d)=%d-->\r\n", mySerialPort, ret);
    }
    Ql_UART_ClrRxBuffer(mySerialPort);
    while(TRUE)
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

/**************************************************************
* Subtask2
***************************************************************/
void proc_subtask2(s32 TaskId)
{
    s32 ret;
    ST_MSG msg;
    //ST_UARTDCB dcb;
    Enum_SerialPort mySerialPort = UART_PORT2;

    Ql_Debug_Trace("<--OpenCPU: proc_subtask2-->\r\n");

    ret = Ql_UART_Register(mySerialPort, Callback_UART_Hdlr, NULL);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Ql_UART_Register(mySerialPort=%d)=%d-->\r\n", mySerialPort, ret);
    }

    ret = Ql_UART_Open(mySerialPort, 115200, FC_NONE);
    if (ret < QL_RET_OK)
    {
        Ql_Debug_Trace("<--Ql_UART_OpenEx(mySerialPort=%d)=%d-->\r\n", mySerialPort, ret);
    }
    Ql_UART_ClrRxBuffer(mySerialPort);
    while(TRUE)
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

static void Callback_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType evt, bool level, void* customizedPara)
{
    s32 ret;
    //APP_DEBUG("<--CallBack_UART_Hdlr: port=%d, event=%d, level=%d, p=%x-->\r\n", port, evt, level, customizedPara);
    switch (evt)
    {
    case EVENT_UART_READY_TO_READ:
        {
            if (VIRTUAL_PORT1 == port || VIRTUAL_PORT2 == port)
            {// Print URC through uart1 port
                s32 totalBytes = ReadSerialPort(port, m_RxBuf_VirtualPort1, sizeof(m_RxBuf_VirtualPort1));
                s32 ret = Ql_UART_Write(UART_PORT1, m_RxBuf_VirtualPort1, totalBytes);
                if (ret < totalBytes)
                {
                    APP_DEBUG("<--Only part of bytes are written, %d/%d-->\r\n", ret, totalBytes);
                    // TODO: Need to handle event 'QL_EVENT_UART_READY_TO_WRITE'
                }
            }
            else if (UART_PORT1 == port || UART_PORT2 == port )
            {// Reflect the data
                u8* arrRxBuf[] = {m_RxBuf_UartPort1, m_RxBuf_UartPort2};
                s32 totalBytes = ReadSerialPort(port, arrRxBuf[port - UART_PORT1], SERIAL_RX_BUFFER_LEN);
                s32 ret = Ql_UART_Write(port, arrRxBuf[port - UART_PORT1], totalBytes);
                if (ret < totalBytes)
                {
                    APP_DEBUG("<--Only part of bytes are written, %d/%d-->\r\n", ret, totalBytes);
                    // TODO: Need to handle event 'QL_EVENT_UART_READY_TO_WRITE'
                }
            }
            break;
        }
    case EVENT_UART_READY_TO_WRITE:
        break;
    default:
        break;
    }
}
#endif // __EXAMPLE_MULTITASK_PORT__
