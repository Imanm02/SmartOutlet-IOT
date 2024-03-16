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
 *   ril_wifi.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module implements WIFI related APIs.
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
#include "ril_location2.h"
#include "custom_feature_def.h"
#include "ril.h"
#include "ril_util.h"
#include "ql_stdlib.h"
#include "ql_error.h"
#include "ql_trace.h"
#include "ql_uart.h"
#include "ql_common.h"
#include "ql_uart.h"
#ifdef __OCPU_RIL_QLBS_SUPPORT__
static CB_LocInfo callback_qlbsloc = NULL;
bool update_time_flag = 0;

s32 RIL_QLBS_Cfg(Enum_Qlbs_Cfg_Flag cfg_flag,ST_Qlbs_Cfg_Para* cfg_para)
{
    s32 ret = RIL_AT_FAILED;
    char strAT[200];

    Ql_memset(strAT, 0, sizeof(strAT));
    switch(cfg_flag)
    {
        case ASYNCH_MODE:
            Ql_sprintf(strAT, "AT+QLBSCFG=\"asynch\",%d\r\n",cfg_para->asynch_mode);
            break;
        case TIMEOUT:
            Ql_sprintf(strAT, "AT+QLBSCFG=\"timeout\",%d\r\n",cfg_para->timeout);
            break;
        case SERVER_ADDRESS:
            Ql_sprintf(strAT, "AT+QLBSCFG=\"server\",\"%s\"\r\n",cfg_para->server_name);
            break;
        case TOKEN:
            Ql_sprintf(strAT, "AT+QLBSCFG=\"token\",\"%s\"\r\n",cfg_para->token_value);
            break;
        case TIMEUPDATE:
            Ql_sprintf(strAT, "AT+QLBSCFG=\"timeUpdate\",%d\r\n",cfg_para->update_mode);
            break;
        case WITHTIME:
            {
                if(1 == cfg_para->time_mode)
                {
                    update_time_flag = 1;
                }
                else
                {
                    update_time_flag = 0;
                }
            }
            Ql_sprintf(strAT, "AT+QLBSCFG=\"withTime\",%d\r\n",cfg_para->time_mode);
            break;
        default:
            break;
    }

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);
    return ret;
}

s32 RIL_QLBS_Loc(CB_LocInfo cb_qlbsloc)
{
    s32 ret = RIL_AT_FAILED;
	char strAT[200]; 
    callback_qlbsloc = cb_qlbsloc;

    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QLBS\r\n");                

    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),NULL,NULL,0);

    return ret;
}
ST_Lbs_LocInfo loclnfo;
void OnURCHandler_QLBS(const char* strURC,void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char* p3 = NULL;
    char* p4 = NULL;
	u8 buff[50];
    u8 time[TIME_STRING_LENGTH] = {0}; 
    s32 result = 0;
	
 	if (NULL != callback_qlbsloc)
 	{
		Ql_strcpy(buff,"\r\n+QLBS:\0");
		if(Ql_StrPrefixMatch(strURC,buff))
		{
            Ql_memset(buff , 0, 50);
            p3 = Ql_strstr(strURC,":");
            p4 = Ql_strstr(p3+1,",");
            if(p4 != NULL)
            {
                Ql_memcpy(buff,p3+2,p4-p3-2);
    			result = Ql_atoi(buff);
            }
            else
            {
                Ql_memcpy(buff,p3+2,4);
                result = Ql_atoi(buff);
            }

            if(result == 0)
            {
                Ql_memset(buff , 0, 50);
                p1 = Ql_strstr(strURC,",");
                p2 = Ql_strstr(p1+1,",");
                Ql_memcpy(buff,p1+1,p2-p1-1);
    			loclnfo.latitude = Ql_atof(buff);
    			
                Ql_memset(buff , 0, 50);
                p3 = Ql_strstr(p2,",");
                p4 = Ql_strstr(p3+1,",");
                Ql_memcpy(buff,p3+1,p4-p3-1);
    			loclnfo.longitude = Ql_atof(buff);
                
                if(1 == update_time_flag)
                {
                    p3 = Ql_strstr(p2,"\"");
                    p4 = Ql_strstr(p3+1,"\"");
                    Ql_memcpy(time,p3+1,p4-p3-1);
                    time[20] = '\0';
                    Ql_memcpy(loclnfo.time,time,TIME_STRING_LENGTH);
                }
                callback_qlbsloc(result,&loclnfo);
                return;
            }
            else
            {
                loclnfo.longitude = 0;
                loclnfo.latitude = 0;
                callback_qlbsloc(result,&loclnfo);
            }
		}
	}
}

static s32 ATResponse_Qlbs_Ex_Handler(char* line, u32 len, void* userdata)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char* p3 = NULL;
    char* p4 = NULL;
    char buff[30];
    u8 time[TIME_STRING_LENGTH] = {0}; 

    ST_Lbs_LocInfo *loclnfo = (ST_Lbs_LocInfo *)userdata ;
    
    Ql_memset(buff , 0, 30);
    char* head = Ql_RIL_FindString(line, len, "+QLBS:"); //continue wait
    if(head)
    {
		p1 = Ql_strstr(head,",");
        if(p1 == NULL)
        {
            return  RIL_ATRSP_FAILED;
        }
        p2 = Ql_strstr(p1+1,",");
        Ql_memcpy(buff,p1+1,p2-p1-1);
		loclnfo->latitude = Ql_atof(buff); 
		
		Ql_memset(buff , 0, 30);
        p3 = Ql_strstr(p2,",");
        p4 = Ql_strstr(p3+1,",");
        Ql_memcpy(buff,p3+1,p4-p3-1);
		loclnfo->longitude = Ql_atof(buff);

        if(1 == update_time_flag)
        {
            p3 = Ql_strstr(p2,"\"");
            p4 = Ql_strstr(p3+1,"\"");
            Ql_memcpy(time,p3+1,p4-p3-1);
            time[20] = '\0';
            Ql_memcpy(loclnfo->time,time,TIME_STRING_LENGTH);
        }
        
        return  RIL_ATRSP_CONTINUE;
    }

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    }

    head = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if(head)
    {
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_QLBS_Loc_Ex(ST_Lbs_LocInfo* locinfo)
{
	s32 ret = RIL_AT_FAILED;
	char strAT[200]; 
    ST_Lbs_LocInfo Locinfo = {0};


    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+QLBS\r\n");                  
    ret = Ql_RIL_SendATCmd(strAT,Ql_strlen(strAT),ATResponse_Qlbs_Ex_Handler,(void*)&Locinfo,0);
    if(RIL_ATRSP_SUCCESS == ret)
    {
        locinfo->latitude = Locinfo.latitude;
        locinfo->longitude = Locinfo.longitude;
        Ql_strcpy(locinfo->time,Locinfo.time);
       
    }
    else
    {
        locinfo->latitude = 0;
        locinfo->longitude = 0;
    }

    return ret;
}
#endif