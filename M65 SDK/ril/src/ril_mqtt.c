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
 *   ril_mqtt.c
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements MQTT related APIs.
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
#include "ril_mqtt.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"

#ifdef __OCPU_RIL_SUPPORT__

#define RIL_MQTT_DEBUG_ENABLE 0
#if RIL_MQTT_DEBUG_ENABLE > 0
#define RIL_MQTT_DEBUG_PORT  UART_PORT1
static char DBG_Buffer[1024];
#define RIL_MQTT_DEBUG(BUF,...)  QL_TRACE_LOG(RIL_MQTT_DEBUG_PORT,BUF,1024,__VA_ARGS__)
#else
#define RIL_MQTT_DEBUG(BUF,...)
#endif

static s32 ATResponse_QMTPUB_handler(char* line, u32 len, void* userdata);

static s32 ATResponse_Handler(char* line, u32 len, void* userData)
{
    RIL_MQTT_DEBUG(DBG_Buffer,"[ATResponse_Handler] %s\r\n", (u8*)line);

    if (Ql_RIL_FindLine(line, len, "OK"))
    {
        return  RIL_ATRSP_SUCCESS;
    }
    else if (Ql_RIL_FindLine(line, len, "ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CME ERROR"))
    {
        return  RIL_ATRSP_FAILED;
    }
    else if (Ql_RIL_FindString(line, len, "+CMS ERROR:"))
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_MQTT_QMTCFG_Ali( Enum_ConnectID connectID,u8* product_key,u8* device_name,u8* device_secret)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTCFG=\"ALIAUTH\",%d,\"%s\",\"%s\",\"%s\"\r\n",connectID,product_key,device_name,device_secret);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

s32 RIL_MQTT_QMTCFG_Showrecvlen( Enum_ConnectID connectID,Enum_ShowFlag show_flag)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[64];

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTCFG=\"SHOWRECVLEN\",%d,%d\r\n",connectID,show_flag);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

s32 RIL_MQTT_QMTCFG_Version_Select( Enum_ConnectID connectID,Enum_VersionNum version_num)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTCFG=\"VERSION\",%d,%d\r\n",connectID,version_num);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

s32 RIL_MQTT_QMTOPEN(Enum_ConnectID connectID, u8* hostName, u32 port)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[200];
    
	Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QMTOPEN=%d,\"%s\",%d\r\n", connectID,hostName,port);	
    
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

s32 RIL_MQTT_QMTCONN(Enum_ConnectID connectID, u8* clientID, u8* username, u8* password)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
	if((NULL != username) && (NULL !=password))
	{
        Ql_sprintf(strAT, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r\n", connectID,clientID,username,password);
	}
	else
	{
		Ql_sprintf(strAT, "AT+QMTCONN=%d,\"%s\"\r\n", connectID,clientID);
	}
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

s32 RIL_MQTT_QMTSUB(Enum_ConnectID connectID, u32 msgId, ST_MQTT_topic_info_t* mqtt_topic_info_t)
{
    s32 ret = RIL_AT_SUCCESS;
    u8* strAT  = NULL;
    u8* info = NULL;
	u8  temp_buffer[RIL_MQTT_TOPIC_MAX];
	u8 i = 0;

	if((mqtt_topic_info_t->count > MQTT_MAX_TOPIC)&&(mqtt_topic_info_t->count <= 0))
	{
		return RIL_AT_INVALID_PARAM;
	}
	
	strAT = (u8*)Ql_MEM_Alloc(sizeof(u8)*RIL_MQTT_LENGTH_MAX);
	if(NULL == strAT)
	{
       return RIL_AT_INVALID_PARAM;
	}

	info = (u8*)Ql_MEM_Alloc(sizeof(u8)*RIL_MQTT_INFO_MAX);
	if(NULL == info)
	{
       return RIL_AT_INVALID_PARAM;
	}
    Ql_memset(info,0, sizeof(u8)*RIL_MQTT_INFO_MAX);
	for(i = 0;i< mqtt_topic_info_t->count;i++)
	{
		Ql_memset(temp_buffer,0, sizeof(u8)*RIL_MQTT_TOPIC_MAX);
		if(0 == i)
		{
		  Ql_sprintf(temp_buffer, "\"%s\",%d",mqtt_topic_info_t->topic[i],mqtt_topic_info_t->qos[i]);
		}
		else 
		{
          Ql_sprintf(temp_buffer, ",\"%s\",%d",mqtt_topic_info_t->topic[i],mqtt_topic_info_t->qos[i]);
		}
		Ql_strncat(info,temp_buffer,Ql_strlen(temp_buffer));
	}

    Ql_memset(strAT, 0, sizeof(u8)*RIL_MQTT_LENGTH_MAX);
    Ql_sprintf(strAT, "AT+QMTSUB=%d,%d,%s\r\n", connectID,msgId,info);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	if(NULL != strAT)
	{
       Ql_MEM_Free(strAT);
	   strAT = NULL;
	}

	if(NULL != info)
	{
       Ql_MEM_Free(info);
	   info = NULL;
	}
    return ret;
}

s32 RIL_MQTT_QMTUNS(Enum_ConnectID connectID, u32 msgId,ST_MQTT_topic_info_t* mqtt_topic_info_t)
{
    s32 ret = RIL_AT_SUCCESS;
    u8* strAT  = NULL;
    u8* info = NULL;
	u8  temp_buffer[RIL_MQTT_TOPIC_MAX];
	u8 i = 0;

	if((mqtt_topic_info_t->count > MQTT_MAX_TOPIC)&&(mqtt_topic_info_t->count <= 0))
	{
		return RIL_AT_INVALID_PARAM;
	}
	
	strAT = (u8*)Ql_MEM_Alloc(sizeof(u8)*RIL_MQTT_LENGTH_MAX);
	if(NULL == strAT)
	{
       return RIL_AT_INVALID_PARAM;
	}

	info = (u8*)Ql_MEM_Alloc(sizeof(u8)*RIL_MQTT_INFO_MAX);
	if(NULL == info)
	{
       return RIL_AT_INVALID_PARAM;
	}

    Ql_memset(info, 0, sizeof(u8)*RIL_MQTT_INFO_MAX);

	for(i = 0;i< mqtt_topic_info_t->count;i++)
	{
		Ql_memset(temp_buffer,0, sizeof(u8)*RIL_MQTT_TOPIC_MAX);
		if(0 == i)
		{
		  Ql_sprintf(temp_buffer, "\"%s\"",mqtt_topic_info_t->topic[i]);
		}
		else 
		{
          Ql_sprintf(temp_buffer, ",\"%s\"",mqtt_topic_info_t->topic[i]);
		}
		Ql_strncat(info,temp_buffer,Ql_strlen(temp_buffer));
	}

    Ql_memset(strAT, 0, sizeof(u8)*RIL_MQTT_LENGTH_MAX);
    Ql_sprintf(strAT, "AT+QMTUNS=%d,%d,%s\r\n", connectID,msgId,info);
	
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);
	
    if(NULL != strAT)
	{
       Ql_MEM_Free(strAT);
	   strAT = NULL;
	}

	if(NULL != info)
	{
       Ql_MEM_Free(info);
	   info = NULL;
	}
    return ret;

}

s32 RIL_MQTT_QMTCLOSE(Enum_ConnectID connectID)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
	Ql_sprintf(strAT, "AT+QMTCLOSE=%d\r\n", connectID);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);
 
    return ret;
}

s32 RIL_MQTT_QMTDISC(Enum_ConnectID connectID)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
	Ql_sprintf(strAT, "AT+QMTDISC=%d\r\n", connectID);

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Handler,NULL,0);
    RIL_MQTT_DEBUG(DBG_Buffer,"<--Send AT:%s, ret = %d -->\r\n",strAT, ret);

	return ret;
}

static s32 ATResponse_QMTPUB_handler(char* line, u32 len, void* userdata)
{
    u8 *phead = NULL;
    s32 i;
	MQTT_PUB_Data *mqtt_data = (MQTT_PUB_Data *)userdata;

	phead = Ql_RIL_FindString(line, len, "\r\n>");
    if(phead)
    {
		Ql_RIL_WriteDataToCore (mqtt_data->data,mqtt_data->datalength);
		return RIL_ATRSP_CONTINUE;
    }

    phead = Ql_RIL_FindString(line, len, "ERROR");
    if(phead)
    {
        return  RIL_ATRSP_FAILED;
    }
    phead = Ql_RIL_FindString(line, len, "OK");
    if(phead)
    {
        return  RIL_ATRSP_SUCCESS;
    }
    phead = Ql_RIL_FindString(line, len, "+CMS ERROR:");//fail
    if(phead)
    {
        return  RIL_ATRSP_FAILED;
    }
    return RIL_ATRSP_CONTINUE; //continue wait
}


s32 RIL_MQTT_QMTPUB(Enum_ConnectID connectID, u32 msgId, Enum_Qos qos, u8 retain, u8* topic, u32 size, u8* message)
{
    s32 ret = RIL_AT_SUCCESS;
    char strAT[200];
    MQTT_PUB_Data pub_data;
    
    pub_data.datalength = size;
    
    pub_data.data = (u8*)Ql_MEM_Alloc(size+1);
	if(NULL == pub_data.data)
	{
       return RIL_AT_INVALID_PARAM;
	}
    //pub_data.data = message;
    Ql_memcpy(pub_data.data,message,pub_data.datalength);

    Ql_memset(strAT, 0, sizeof(strAT));
	Ql_sprintf(strAT, "AT+QMTPUB=%d,%d,%d,%d,\"%s\",%d\r\n", connectID,msgId,qos,retain,topic,size);
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_QMTPUB_handler,(void* )&pub_data,0);

    if(NULL != pub_data.data)
    {
        Ql_MEM_Free(pub_data.data);
		pub_data.data = NULL;
    }

    return ret;
}


#endif
