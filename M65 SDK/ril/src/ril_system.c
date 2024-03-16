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
 *   ril_system.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to RIL.
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
#include "custom_feature_def.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_system.h"
#include "ql_stdlib.h"
#include "ql_trace.h"
#include "ql_error.h"
#include "ql_system.h"

#ifdef __OCPU_RIL_SUPPORT__ 
static s32 SYS_ATResponse_Hanlder(char* line, u32 len, void* userdata)
{
    s32* pSysInitStatus = (s32* )userdata; 
    char *head = Ql_RIL_FindString(line, len, "+QINISTAT:"); //continue wait
    if(head)
    {
        char strTmp[10];
        char* p1 = NULL;
        char* p2 = NULL;

        Ql_memset(strTmp, 0x0, sizeof(strTmp));
        p1 = Ql_strstr(head, ":");
        p2 = Ql_strstr(p1 + 1, "\r\n");
        if (p1 && p2)
        {
            Ql_memcpy(strTmp, p1 + 2, p2 - p1 - 2);
            *pSysInitStatus = Ql_atoi(strTmp);
        }
        return  RIL_ATRSP_CONTINUE;
    }

    head = Ql_RIL_FindLine(line, len, "OK");
    if(head)
    {  
        return  RIL_ATRSP_SUCCESS;
    }

    head = Ql_RIL_FindLine(line, len, "ERROR");
    if(head)
    {  
        return  RIL_ATRSP_FAILED;
    } 

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait
}


s32 RIL_QuerySysInitStatus( s32* SysInitStatus)
{
    return Ql_RIL_SendATCmd("AT+QINISTAT", 11, SYS_ATResponse_Hanlder, (void *)SysInitStatus, 0); 
}

static s32 Power_ATResponse_Hanlder(char* line, u32 len, void* userdata)
{
    ST_SysPower *PowerSupply;

    PowerSupply = (ST_SysPower *)userdata;
    char *head = Ql_RIL_FindString(line, len, "+CBC:"); //continue wait
    if(head)
    {
        char strTmp[10];
        char *p1,*p2;
        p1 = Ql_strstr(head, ":");
        p2 = Ql_strstr(p1 + 1, ",");
        if (p1 && p2)
        {
            p1 = p2;
            p2 = Ql_strstr(p1 + 1, ",");
            if (p1 && p2)
            {
                Ql_memset(strTmp, 0x0, sizeof(strTmp));
                Ql_memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                PowerSupply->capacity = Ql_atoi(strTmp);
                p1 = p2;
                p2 = Ql_strstr(p1 + 1, "\r\n");
                if (p1 && p2)
                {
                    Ql_memset(strTmp, 0x0, sizeof(strTmp));
                    Ql_memcpy(strTmp, p1 + 1, p2 - p1 - 1);
                    PowerSupply->voltage = Ql_atoi(strTmp);
                }
            } 
        }             
        //    Ql_sscanf(head,"%*[^ ]%d,%d,%[^\r\n]",&PowerSupply->capacity,&PowerSupply->voltage);
        return  RIL_ATRSP_CONTINUE;
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

    head = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if(head)
    {
        return  RIL_ATRSP_FAILED;
    }

    return RIL_ATRSP_CONTINUE; //continue wait

}


/*****************************************************************
* Function:     RIL_GetPowerSupply 
* 
* Description:
*               This function queries the battery balance, and the battery voltage.
*
* Parameters:
*               capacity:      
*                   [out] battery balance, a percent, ranges from 1 to 100.
*
*               voltage:       
*                   [out] battery voltage, unit in mV
* Return:        
*               QL_RET_OK, indicates this function successes.
*		   -1, fail.
*****************************************************************/
s32 RIL_GetPowerSupply(u32* capacity, u32* voltage)
{
    s32 ret;
    ST_SysPower PowerSupply;

    ret = Ql_RIL_SendATCmd("AT+CBC", 6, Power_ATResponse_Hanlder, (void *)&PowerSupply, 0);
    if (RIL_AT_SUCCESS == ret)
    {
        *capacity = PowerSupply.capacity;
        *voltage  = PowerSupply.voltage;
    }
    return ret;
}

static s32 ATRsp_IMEI_Handler(char* line, u32 len, void* param)
{
    char* pHead = NULL;
    pHead = Ql_RIL_FindLine(line, len, "OK"); // find <CR><LF>OK<CR><LF>, <CR>OK<CR>£¬<LF>OK<LF>
    if (pHead)
    {  
        return RIL_ATRSP_SUCCESS;
    }

    pHead = Ql_RIL_FindLine(line, len, "ERROR");// find <CR><LF>ERROR<CR><LF>, <CR>ERROR<CR>£¬<LF>ERROR<LF>
    if (pHead)
    {  
        return RIL_ATRSP_FAILED;
    } 

    pHead = Ql_RIL_FindString(line, len, "+CME ERROR:");//fail
    if (pHead)
    {
        return RIL_ATRSP_FAILED;
    }
    Ql_memcpy((char*)param, line, len - 2); // <imsi number>\r\n
    return RIL_ATRSP_CONTINUE; //continue wait
}

s32 RIL_GetIMEI(char* imei)
{
    char strAT[] = "AT+GSN\0";
    if (NULL == imei)
    {
        return RIL_AT_INVALID_PARAM;
    }
    return Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), ATRsp_IMEI_Handler,(void*)imei, 0);
}

bool QSDK_Get_Str(char *src_string,  char *dest_string, unsigned char index)
{
    u32 SentenceCnt = 0;
    u32 ItemSum = 0;
    u32 ItemLen = 0, Idx = 0;
    u32 len = 0;
    unsigned int i = 0;
    
    if (src_string ==NULL)
    {
        return FALSE;
    }
    len = Ql_strlen(src_string);
	for ( i = 0; i < len; i++)
	{
		if (*(src_string + i) == ',')
		{
			ItemLen = i - ItemSum - SentenceCnt;
			ItemSum  += ItemLen;
            if (index == SentenceCnt)
            {
                if (ItemLen == 0)
                {
                    return FALSE;
                }
		        else
                {
                    Ql_memcpy(dest_string, src_string + Idx, ItemLen);
                    *(dest_string + ItemLen) = '\0';
                    return TRUE;
                }
            }
			SentenceCnt++; 	 
			Idx = i + 1;
		}		
	}
    if (index == SentenceCnt && (len - Idx) != 0)
    {
        Ql_memcpy(dest_string, src_string + Idx, len - Idx);
        *(dest_string + len) = '\0';
        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}
#endif  //__OCPU_RIL_SUPPORT__

