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
 *   example_mqtt.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   This example demonstrates how to use MQTT function with APIs in OpenCPU.
 *
 * Usage:
 * ------
 *   Compile & Run:
 *
 *     Set "C_PREDEF=-D __EXAMPLE_MQTT__" in gcc_makefile file. And compile the 
 *     app using "make clean/new".
 *     Download image bin to module to run.
 * 
 * note:
 *     
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/
#ifdef __EXAMPLE_MQTT__
#include "custom_feature_def.h"
#include "ql_stdlib.h"
#include "ql_common.h"
#include "ql_system.h"
#include "ql_type.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_uart.h"
#include "ql_timer.h"
#include "ril_network.h"
#include "ril_mqtt.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_system.h"


#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT  UART_PORT1
#define DBG_BUF_LEN   1024
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


/*****************************************************************
* define process state
******************************************************************/
typedef enum{
    STATE_NW_QUERY_STATE,
    STATE_MQTT_CFG,
    STATE_MQTT_OPEN,
    STATE_MQTT_CONN,
    STATE_MQTT_SUB,
    STATE_MQTT_PUB,
    STATE_MQTT_TUNS,
    STATE_MQTT_CLOSE,
    STATE_MQTT_DISC,
    STATE_MQTT_TOTAL_NUM
}Enum_ONENETSTATE;
static u8 m_mqtt_state = STATE_NW_QUERY_STATE;

/****************************************************************************
* Definition for APN
****************************************************************************/
#define APN      "CMNET\0"
#define USERID   ""
#define PASSWD   ""

/*****************************************************************
* MQTT  timer param
******************************************************************/
#define MQTT_TIMER_ID         0x200
#define MQTT_TIMER_PERIOD     500

/*****************************************************************
* Server Param
******************************************************************/
#define SRVADDR_BUFFER_LEN    100

//#define HOST_NAME             "yourproductkey.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define HOST_NAME             "other cloud platform"
#define HOST_PORT             1883

/*****************************************************************
* Uart   param
******************************************************************/
#define SERIAL_RX_BUFFER_LEN  2048
static Enum_SerialPort m_myUartPort  = UART_PORT1;
static u8 g_RxBuf_Uart1[SERIAL_RX_BUFFER_LEN];

/*****************************************************************
*  MQTT Param
******************************************************************/
MQTT_Urc_Param_t*	  mqtt_urc_param_ptr = NULL;
ST_MQTT_topic_info_t  mqtt_topic_info_t;
bool DISC_flag  = TRUE;
bool CLOSE_flag = TRUE;

/*****************************************************************
*  Sample Param
******************************************************************/
Enum_ConnectID connect_id = ConnectID_0;
u32 pub_message_id = 0;
u32 sub_message_id = 0;

u8 product_key[] =   "your-productkey\0";   //<ali cloud needs it.
u8 device_name[]=    "your-devicename\0";   //<ali cloud needs it.
u8 device_secret[] = "your-devicesecret\0"; //<ali cloud needs it.
u8 clientID[] =      "your-clientID\0";
u8 username[] =      "your-username\0";
u8 passwd[] =        "your-passwd\0";

static u8 test_data[128] =  "hello cloud,this is quectel test code!!!\0"; //<first packet data
static u8 test_topic[128] = "The topic that need to be subscribed and published\0"; //<topic

/*****************************************************************
* Uart callback function
******************************************************************/
static void proc_handle(u8 *pData,s32 len);
static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen);
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara);

/*****************************************************************
* Timer callback function
******************************************************************/
static void Callback_Timer(u32 timerId, void* param);

/*****************************************************************
* MQTT recv callback function
******************************************************************/
static void mqtt_recv(u8* buffer,u32 length);


void proc_main_task(s32 taskId)
{
    ST_MSG msg;
    s32 ret;

    //<Register & open UART port
    Ql_UART_Register(m_myUartPort, CallBack_UART_Hdlr, NULL);
    Ql_UART_Open(m_myUartPort, 115200, FC_NONE);

    APP_DEBUG("//<------------OpenCPU: MQTT Client.------------\r\n");

    //<register state timer 
    Ql_Timer_Register(MQTT_TIMER_ID, Callback_Timer, NULL);

	//register MQTT recv callback
    ret = Ql_Mqtt_Recv_Register(mqtt_recv);
	APP_DEBUG("//<register recv callback,ret = %d\r\n",ret);

    while(TRUE)
    {
        Ql_OS_GetMessage(&msg);
        switch(msg.message)
        {
            case MSG_ID_RIL_READY:
                APP_DEBUG("//<RIL is ready\r\n");
                Ql_RIL_Initialize();
                break;
    		case MSG_ID_URC_INDICATION:
        		{     
        			switch (msg.param1)
                    {
            		    case URC_SIM_CARD_STATE_IND:
                            {
                    			APP_DEBUG("//<SIM Card Status:%d\r\n", msg.param2);
                				if(SIM_STAT_READY == msg.param2)
                				{
                                   Ql_Timer_Start(MQTT_TIMER_ID, MQTT_TIMER_PERIOD, TRUE);
                				   APP_DEBUG("//<state timer start,ret = %d\r\n",ret);
                				}
                		    }
                            break;
        				case URC_MQTT_OPEN:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if(0 == mqtt_urc_param_ptr->result)
            					{
                 					APP_DEBUG("//<Open a MQTT client successfully\r\n");
                                    m_mqtt_state = STATE_MQTT_CONN;
            					}
            					else
            					{
            						APP_DEBUG("//<Open a MQTT client failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
            				}
                            break;
            		    case URC_MQTT_CONN:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if(0 == mqtt_urc_param_ptr->result)
            					{
                    		        APP_DEBUG("//<Connect to MQTT server successfully\r\n");
            						m_mqtt_state = STATE_MQTT_SUB;
            					}
            					else
            					{
            						APP_DEBUG("//<Connect to MQTT server failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
                		    }
                            break;
                        case URC_MQTT_SUB:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if((0 == mqtt_urc_param_ptr->result)&&(128 != mqtt_urc_param_ptr->sub_value[0]))
            					{
                    		        APP_DEBUG("//<Subscribe topics successfully\r\n");
            						m_mqtt_state = STATE_MQTT_PUB;
            					}
            					else
            					{
            						APP_DEBUG("//<Subscribe topics failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
                		    }
            			    break;
        				case URC_MQTT_PUB:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if(0 == mqtt_urc_param_ptr->result)
            					{
                    		        APP_DEBUG("//<Publish messages to MQTT server successfully\r\n");
            						m_mqtt_state = STATE_MQTT_TOTAL_NUM;
            					}
            					else
            					{
            						APP_DEBUG("//<Publish messages to MQTT server failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
                		    }
            			    break;
        			    case URC_MQTT_CLOSE:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if(0 == mqtt_urc_param_ptr->result)
            					{
                    		        APP_DEBUG("//<Closed MQTT socket successfully\r\n");
            					}
            					else
            					{
            						APP_DEBUG("//<Closed MQTT socket failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
                		    }
            			    break;
                        case URC_MQTT_DISC:
            				{
            					mqtt_urc_param_ptr = msg.param2;
            					if(0 == mqtt_urc_param_ptr->result)
            					{
                    		        APP_DEBUG("//<Disconnect MQTT successfully\r\n");
            					}
            					else
            					{
            						APP_DEBUG("//<Disconnect MQTT failure,error = %d\r\n",mqtt_urc_param_ptr->result);
            					}
                		    }
            			    break;
        		        default:
            		        //APP_DEBUG("<-- Other URC: type=%d\r\n", msg.param1);
            	            break;
        			}
        		}
    		    break;
        	default:
                break;
        }
    }
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/u8* pBuffer, /*[in]*/u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) // Serial Port Error!
    {
        APP_DEBUG("//<Fail to read from port[%d]\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void* customizedPara)
{
    char *p = NULL;
    s32 ret;
    
    switch (msg)
    {
        case EVENT_UART_READY_TO_READ:
            {
                if(m_myUartPort == port)
                {
                    s32 totalBytes = ReadSerialPort(port, g_RxBuf_Uart1, sizeof(g_RxBuf_Uart1));
                    if (totalBytes <= 0)
                    {
                        APP_DEBUG("//<No data in UART buffer!\r\n");
                        return;
                    }
                    p = Ql_strstr(g_RxBuf_Uart1,"DISC");
    				if(p)
    				{
    			        ret = RIL_MQTT_QMTDISC(connect_id);
                        if(RIL_AT_SUCCESS == ret)
                        {
                            APP_DEBUG("//<Start disconnect MQTT socket\r\n");
                            if(TRUE == DISC_flag)
                                DISC_flag  = FALSE;
                            m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                        }else
                        {
                            APP_DEBUG("//<Disconnect MQTT socket failure,ret = %d\r\n",ret); 
                            m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                        }
    					break;
    				}
                    p = Ql_strstr(g_RxBuf_Uart1,"CLOSE");
    				if(p)
    				{
    					ret = RIL_MQTT_QMTCLOSE(connect_id);
                        if (RIL_AT_SUCCESS == ret)
                        {
                            APP_DEBUG("//<Start closed MQTT socket\r\n");
                            if(TRUE == CLOSE_flag)
                                CLOSE_flag = FALSE;
                            m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                        }else
                        {
                            APP_DEBUG("//<Closed MQTT socket failure,ret = %d\r\n",ret);
                            m_mqtt_state = STATE_MQTT_TOTAL_NUM;
        			    }
    					break;
    				}
                    p = Ql_strstr(g_RxBuf_Uart1,"RECONN");
    				if(p)
    				{
                        if((FALSE == DISC_flag)||(FALSE == CLOSE_flag))
                        {
                            m_mqtt_state = STATE_MQTT_OPEN;
                            APP_DEBUG("\r\n");
                        }
    					break;
    				}
                    //<The rest of the UART data is published as data.
                    {
                        if((TRUE == DISC_flag)&&(TRUE == CLOSE_flag))
                        {
                            pub_message_id++;  // The range is 0-65535. It will be 0 only when<qos>=0.
            				ret = RIL_MQTT_QMTPUB(connect_id,pub_message_id,QOS1_AT_LEASET_ONCE,0,test_topic,totalBytes,g_RxBuf_Uart1);
                            if (RIL_AT_SUCCESS == ret)
                            {
                                APP_DEBUG("//<Start publish a message to server\r\n");
                            }else
                            {
                                APP_DEBUG("//<Publish a message to server failure,ret = %d\r\n",ret);
                            }
                        }
                        else
                        {
                            //<No connection to the cloud platform, just echo.
                            APP_DEBUG("\r\n//<No connection to the cloud platform, just echo.\r\n");
                            Ql_UART_Write(m_myUartPort, g_RxBuf_Uart1, totalBytes);
                        }
                    }
                }
            }
            break;
        case EVENT_UART_READY_TO_WRITE:
            break;
        default:
            break;
    }
}

static void mqtt_recv(u8* buffer,u32 length)
{
	APP_DEBUG("//<data:%s,len:%d\r\n",buffer,length);
}

static void Callback_Timer(u32 timerId, void* param)
{
    s32 ret;
    
    if(MQTT_TIMER_ID == timerId)
    {
        switch(m_mqtt_state)
        {        
            case STATE_NW_QUERY_STATE:
            {
                s32 cgreg = 0;
                ret = RIL_NW_GetGPRSState(&cgreg);
                APP_DEBUG("//<Network State:cgreg = %d\r\n",cgreg);
                if((cgreg == NW_STAT_REGISTERED)||(cgreg == NW_STAT_REGISTERED_ROAMING))
                {
                    //<Set PDP context 0
                    RIL_NW_SetGPRSContext(0);
                    APP_DEBUG("//<Set PDP context 0 \r\n");
                	//<Set APN
                	ret = RIL_NW_SetAPN(1, APN, USERID, PASSWD);
                	APP_DEBUG("//<Set APN \r\n");
                    //PDP activated
                    ret = RIL_NW_OpenPDPContext();
                    if(ret == RIL_AT_SUCCESS)
                	{
                	    APP_DEBUG("//<Activate PDP context,ret = %d\r\n",ret);
                	    m_mqtt_state = STATE_MQTT_CFG;
                	}
                }
                break;
            }
            case STATE_MQTT_CFG:
            {
                //ret = RIL_MQTT_QMTCFG_Ali(connect_id,product_key,device_name,device_secret);//<This configuration is required to connect to Ali Cloud.
                RIL_MQTT_QMTCFG_Showrecvlen(connect_id,ShowFlag_1);//<This sentence must be configured. The configuration will definitely succeed, so there is no need to care about.
                ret = RIL_MQTT_QMTCFG_Version_Select(connect_id,Version_3_1_1);
                if(RIL_AT_SUCCESS == ret)
                {
                    //APP_DEBUG("//<Ali Platform configure successfully\r\n");
                    APP_DEBUG("//<Select version 3.1.1 successfully\r\n");
                    m_mqtt_state = STATE_MQTT_OPEN;
                }
                else
                {
                    //APP_DEBUG("//<Ali Platform configure failure,ret = %d\r\n",ret);
                    APP_DEBUG("//<Select version 3.1.1 failure,ret = %d\r\n",ret);
                }
                break;
            }
			case STATE_MQTT_OPEN:
            {
                ret = RIL_MQTT_QMTOPEN(connect_id,HOST_NAME,HOST_PORT);
                if(RIL_AT_SUCCESS == ret)
                {
                    APP_DEBUG("//<Start opening a MQTT client\r\n");
                    if(FALSE == CLOSE_flag)
                        CLOSE_flag = TRUE;
                    m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                }
                else
                {
                    APP_DEBUG("//<Open a MQTT client failure,ret = %d-->\r\n",ret);
                }
                break;
            }
            case STATE_MQTT_CONN:
            {
			    ret = RIL_MQTT_QMTCONN(connect_id,clientID,username,passwd);
	            if(RIL_AT_SUCCESS == ret)
                {
                    APP_DEBUG("//<Start connect to MQTT server\r\n");
                    if(FALSE == DISC_flag)
                        DISC_flag = TRUE;
                    m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                }
                else
                {
                    APP_DEBUG("//<connect to MQTT server failure,ret = %d\r\n",ret);
                }
                break;
            }
			case STATE_MQTT_SUB:
            {				
                mqtt_topic_info_t.count = 1;
				mqtt_topic_info_t.topic[0] = (u8*)Ql_MEM_Alloc(sizeof(u8)*256);
				
				Ql_memset(mqtt_topic_info_t.topic[0],0,256);
				Ql_memcpy(mqtt_topic_info_t.topic[0],test_topic,Ql_strlen(test_topic));
                mqtt_topic_info_t.qos[0] = QOS1_AT_LEASET_ONCE;
				sub_message_id++;  //< 1-65535.
				
				ret = RIL_MQTT_QMTSUB(connect_id,sub_message_id,&mqtt_topic_info_t);
				
				Ql_MEM_Free(mqtt_topic_info_t.topic[0]);
	            mqtt_topic_info_t.topic[0] = NULL;
                if(RIL_AT_SUCCESS == ret)
                {
                    APP_DEBUG("//<Start subscribe topic\r\n");
                    m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                }
                else
                {
                    APP_DEBUG("//<Subscribe topic failure,ret = %d\r\n",ret);
                }
                break;
            }
            case STATE_MQTT_PUB:
            {
				pub_message_id++;  //< The range is 0-65535. It will be 0 only when<qos>=0.
				ret = RIL_MQTT_QMTPUB(connect_id,pub_message_id,QOS1_AT_LEASET_ONCE,0,test_topic,Ql_strlen(test_data),test_data);
                if(RIL_AT_SUCCESS == ret)
                {
                    APP_DEBUG("//<Start publish a message to MQTT server\r\n");
                    m_mqtt_state = STATE_MQTT_TOTAL_NUM;
                }
                else
                {
                    APP_DEBUG("//<Publish a message to MQTT server failure,ret = %d\r\n",ret);
                }
                break;
            }
			case STATE_MQTT_TOTAL_NUM:
            {
                //<do nothing
			    break;
            }
            default:
                break;
        }    
    }
}

#endif // __EXAMPLE_MQTT__

