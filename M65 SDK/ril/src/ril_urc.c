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
 *   ril_urc.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module handles URC in RIL.
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
#include "ql_stdlib.h"
#include "ril.h "
#include "ril_util.h"
#include "ril_sms.h"
#include "ril_telephony.h "
#include "ql_power.h"
#include "ql_system.h"
#include "ril_audio.h"
#include "ril_ftp.h"
#include "ril_http.h"
#include "ril_system.h"

#ifdef __OCPU_RIL_SUPPORT__

/************************************************************************/
/* Definition for URC receive task id.                                  */
/************************************************************************/
#define URC_RCV_TASK_ID  main_task_id
MQTT_Urc_Param_t   mqtt_urc_param =  {0,0,255,255,255,255,{255}}; //<for mqtt
/************************************************************************/
/* Declarations for URC handler.                                        */
/************************************************************************/
static void OnURCHandler_Call(const char* strURC, void* reserved);
static void OnURCHandler_SMS(const char* strURC, void* reserved);
static void OnURCHandler_Network(const char* strURC, void* reserved);
static void OnURCHandler_SIM(const char* strURC, void* reserved);
static void OnURCHandler_CFUN(const char* strURC, void* reserved);
static void OnURCHandler_Voltage(const char* strURC, void* reserved);
static void OnURCHandler_InitStat(const char* strURC, void* reserved);
static void OnURCHandler_HTTP(const char* strURC, void* reserved);
static void OnURCHandler_FTP_Download(const char* strURC, void* reserved);
static void OnURCHandler_FTP_Upload(const char* strURC, void* reserved);
static void OnURCHandler_AlarmRing(const char* strURC, void* reserved);
static void OnURCHandler_AudPlayInd(const char* strURC, void* reserved);
extern void OnURCHandler_QCELLLocation(const char* strURC,void* reserved);
// DTMF URC callback
extern void OnURCHandler_QToneDet( const char* strURC, void* reserved );
extern void OnURCHandler_QWDTMF( const char* strURC, void* reserved );
// NTP URC callback
extern void OnURCHandler_NTPCMD(const char* strURC, void* reserved);
#ifdef __OCPU_RIL_QLBS_SUPPORT__
extern void OnURCHandler_QLBS(const char* strURC,void* reserved);
#endif
/*************** ***MQTT********************************************/
static void OnURCHandler_MQTT_OPEN(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_CONN(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_SUB(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_PUB(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_TUNS(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_STATE(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_CLOSE(const char* strURC, void* reserved);
static void OnURCHandler_MQTT_DISC(const char* strURC, void* reserved);

/************************************************************************/
/* Customer ATC URC callback                                          */
/************************************************************************/
CallBack_Ftp_Upload FtpPut_IND_CB = NULL;
CallBack_Ftp_Download FtpGet_IND_CB = NULL;

/******************************************************************************
* Definitions for URCs and the handler.
* -------------------------------------
* -------------------------------------
* In OpenCPU RIL, URC contains two types: system URC and AT URC.
*   - System URCs indicate the various status of module.
*   - AT URC serves some specific AT command. 
*     For example, some AT command responses as below:
*         AT+QABC     (send at command)
*
*         OK          (response1)
*         +QABC:xxx   (response2) --> this is the final result which is reported by URC.
*     When calling Ql_RIL_SendATCmd() to send such AT command, the return value of 
*     Ql_RIL_SendATCmd indicates the response1, and the response2 may be reported
*     via the callback function. Especially for some AT commands that the time span
*     between response1 and response2 is very long, such as AT+QHTTPDL, AT+QFTPGET.
******************************************************************************/
/****************************************************/
/* Definitions for system URCs and the handler      */
/****************************************************/
const static ST_URC_HDLENTRY m_SysURCHdlEntry[] = {

    //Telephony unsolicited response
    {"\r\n+CRING: VOICE\r\n",                     OnURCHandler_Call},
    {"\r\nRING\r\n",                              OnURCHandler_Call},
    {"\r\nBUSY\r\n",                              OnURCHandler_Call},
    {"\r\nNO ANSWER\r\n",                         OnURCHandler_Call},
    {"\r\nNO CARRIER\r\n",                        OnURCHandler_Call},
    {"\r\nNO DIALTONE\r\n",                       OnURCHandler_Call},
    {"\r\n+CLIP:",                                OnURCHandler_Call},

    //SMS unsolicited response
    {"\r\n+CMTI:",                                OnURCHandler_SMS},

    //Network status unsolicited response
    {"\r\n+CREG:",                                OnURCHandler_Network},
    {"\r\n+CGREG:",                               OnURCHandler_Network},

    //SIM card unsolicited response
    {"\r\n+CPIN:",                                OnURCHandler_SIM},                       

    //CFUN unsolicited response
    {"\r\n+CFUN:",                                OnURCHandler_CFUN},

    //Voltage indication
    {"\r\nUNDER_VOLTAGE WARNING \r\n",            OnURCHandler_Voltage},
    {"\r\nUNDER_VOLTAGE POWER DOWN \r\n",         OnURCHandler_Voltage},
    {"\r\nOVER_VOLTAGE WARNING \r\n",             OnURCHandler_Voltage},
    {"\r\nOVER_VOLTAGE POWER DOWN \r\n",          OnURCHandler_Voltage},

    //Init status unsolicited response
    {"\r\nCall Ready\r\n",                        OnURCHandler_InitStat},
    {"\r\nSMS Ready\r\n",                         OnURCHandler_InitStat},

	// Clock alarm ring indication
	{"\r\nALARM RING\r\n",                        OnURCHandler_AlarmRing},
	{"\r\nALARM MODE\r\n",                        OnURCHandler_AlarmRing},
		
	// Location indication
	{"\r\n+QCELLLOC:",                        OnURCHandler_QCELLLocation},
};

/****************************************************/
/* Definitions for AT URCs and the handler          */
/****************************************************/
const static ST_URC_HDLENTRY m_AtURCHdlEntry[] = {
    //HTTP unsolicited response
    {"\r\n+QHTTPDL:",                             OnURCHandler_HTTP},

    //FTP unsolicited response
    {"\r\n+QFTPGET:",                             OnURCHandler_FTP_Download},
    {"\r\n+QFTPPUT:",                             OnURCHandler_FTP_Upload},

    //Audio (file or resource) playing indication
    {"\r\n+QAUDPIND:",                            OnURCHandler_AudPlayInd},
    {"\r\n+QPLAYRES:",                            OnURCHandler_AudPlayInd},
    {"\r\n+QPRESBG:",                             OnURCHandler_AudPlayInd},

    // DTMF unsolicited response
    {"\r\n+QTONEDET:",                            OnURCHandler_QToneDet},
    {"\r\n+QWDTMF:",                              OnURCHandler_QWDTMF},

	//NTP unsolicited response
	{"\r\n+QNTP:",							  	  OnURCHandler_NTPCMD},
#ifdef __OCPU_RIL_QLBS_SUPPORT__
    {"\r\n+QLBS:",							  	  OnURCHandler_QLBS},
#endif
    {"\r\n+QMTOPEN:",                             OnURCHandler_MQTT_OPEN},
	{"\r\n+QMTCONN:",                             OnURCHandler_MQTT_CONN},
	{"\r\n+QMTSUB:",                              OnURCHandler_MQTT_SUB},
    {"\r\n+QMTPUB:",                              OnURCHandler_MQTT_PUB},
	{"\r\n+QMTUNS:",                              OnURCHandler_MQTT_TUNS},
	{"\r\n+QMTSTAT:",                             OnURCHandler_MQTT_STATE},
	{"\r\n+QMTCLOSE:",                            OnURCHandler_MQTT_CLOSE},
	{"\r\n+QMTDISC:",                             OnURCHandler_MQTT_DISC},
};

static void OnURCHandler_SIM(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char strTmp[20];
    s32 len;
    extern s32 RIL_SIM_GetSimStateByName(char* simStat, u32 len);

    Ql_memset(strTmp, 0x0, sizeof(strTmp));
    len = Ql_sprintf(strTmp, "\r\n+CPIN: ");
    if (Ql_StrPrefixMatch(strURC, strTmp))
    {
        p1 = Ql_strstr(strURC, "\r\n+CPIN: ");
        p1 += len;
        p2 = Ql_strstr(p1, "\r\n");
        if (p1 && p2)
        {
            u32 cpinStat;
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            Ql_memcpy(strTmp, p1, p2 - p1);
            cpinStat = (u32)RIL_SIM_GetSimStateByName(strTmp, p2 - p1);
            Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_SIM_CARD_STATE_IND, cpinStat);
        }
    }
}
static void OnURCHandler_Network(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char strTmp[10];
    
    if (Ql_StrPrefixMatch(strURC, "\r\n+CREG: "))
    {
        u32 nwStat;
        p1 = Ql_strstr(strURC, "\r\n+CREG: ");
        p1 += Ql_strlen("\r\n+CREG: ");
		if(*(p1+1) == 0x2C)          //Active query network status without reporting URCS
		{
		   return;
		}	
        p2 = Ql_strstr(p1, "\r\n");
        if (p1 && p2)
        {
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            Ql_memcpy(strTmp, p1, p2 - p1);
            nwStat = Ql_atoi(strTmp);
            Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_GSM_NW_STATE_IND, nwStat);
        }
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\n+CGREG: "))
    {
        u32 nwStat;
        p1 = Ql_strstr(strURC, "\r\n+CGREG: ");
        p1 += Ql_strlen("\r\n+CGREG: ");
		if(*(p1+1) == 0x2C)          //Active query network status without reporting URCS
		{
		   return;
		}
        p2 = Ql_strstr(p1, "\r\n");
        if (p1 && p2)
        {
            Ql_memset(strTmp, 0x0, sizeof(strTmp));
            Ql_memcpy(strTmp, p1, p2 - p1);
            nwStat = Ql_atoi(strTmp);
            Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_GPRS_NW_STATE_IND, nwStat);
        }
    }
}
static void OnURCHandler_Call(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char strTmp[10];
    if (Ql_StrPrefixMatch(strURC, "\r\nRING\r\n") || 
        Ql_StrPrefixMatch(strURC, "\r\n+CLIP:") ||
        Ql_StrPrefixMatch(strURC, "\r\n+CRING: VOICE\r\n"))
    {// Coming call
        extern ST_ComingCallInfo  g_comingCall;
        u16 len;

        p1 = Ql_strstr(strURC, "\r\n+CLIP:");
        if (!p1)
        {
            return;
        }

        g_comingCall.ringCnt++;
        if ((g_comingCall.ringCnt / 6) > 0)
        {
            g_comingCall.ringCnt %= 6;
        }

        // Retrieve phone number
        p1 += Ql_strlen("\r\n+CLIP:");
        p2 = Ql_strstr(p1 + 1, ",");
        len = p2 - (p1 + 2) - 1;
        Ql_memcpy(g_comingCall.comingCall[g_comingCall.ringCnt].phoneNumber, p1 + 2, len);
        g_comingCall.comingCall[g_comingCall.ringCnt].phoneNumber[len] = '\0';

        // Retrieve number type
        p1 = p2;
        p2 = Ql_strstr(p1 + 1, ",");
        Ql_memset(strTmp, 0x0, sizeof(strTmp));
        Ql_memcpy(strTmp, p1 + 1, p2 - p1 -1);
        g_comingCall.comingCall[g_comingCall.ringCnt].type = Ql_atoi(strTmp);
        Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_COMING_CALL_IND, (u32)(&(g_comingCall.comingCall[g_comingCall.ringCnt])));
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\nBUSY\r\n")         ||
             Ql_StrPrefixMatch(strURC, "\r\nNO ANSWER\r\n")    ||
             Ql_StrPrefixMatch(strURC, "\r\nNO CARRIER\r\n")   ||
             Ql_StrPrefixMatch(strURC, "\r\nNO DIALTONE\r\n"))
    {
        u32 callStat;

        if (Ql_StrPrefixMatch(strURC, "\r\nBUSY\r\n"))
        {
            callStat = CALL_STATE_BUSY;
        }
        else if (Ql_StrPrefixMatch(strURC, "\r\nNO ANSWER\r\n"))
        {
            callStat = CALL_STATE_NO_ANSWER;
        }
        else if (Ql_StrPrefixMatch(strURC, "\r\nNO CARRIER\r\n"))
        {
            callStat = CALL_STATE_NO_CARRIER;
        }
        else if (Ql_StrPrefixMatch(strURC, "\r\nNO DIALTONE\r\n"))
        {
            callStat = CALL_STATE_NO_DIALTONE;
        }else{
            return;
        }
        Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_CALL_STATE_IND, callStat);
    }
}

static void OnURCHandler_SMS(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;

    //TODO: Something wrong with long SMS
    if (Ql_StrPrefixMatch(strURC, "\r\n+CMTI:")) 
    {
        u32 smsIndex;
        char mem[SMS_MEM_CHAR_LEN];

        // Get 'mem'
        p1 = Ql_strstr(strURC, ":");
        p1 += 3;
        p2 = Ql_strstr(p1, ",");
        if (p1 && p2)
        {
            Ql_memset(mem, 0x0, sizeof(mem));
            Ql_strncpy(mem, p1, (p2 - p1 - 1));
        }

        // Get index
        p1 = p2;
        p2 = Ql_strstr(p1, "\r\n");
        if (p1 && p2)
        {
            char strIndex[10];
            Ql_memset(strIndex, 0x0, sizeof(strIndex));
            Ql_strncpy(strIndex, p1 + 1, p2 - p1 - 1);
            smsIndex = Ql_atoi(strIndex);
            Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_NEW_SMS_IND, smsIndex);
        }
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\n+CMT:"))
    {
    }
}

static void OnURCHandler_Voltage(const char* strURC, void* reserved)
{
    u32 volState = VBATT_UNDER_WRN;
    
    if (Ql_StrPrefixMatch(strURC, "\r\nUNDER_VOLTAGE WARNING \r\n"))
    {
        volState = VBATT_UNDER_WRN;
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\nUNDER_VOLTAGE POWER DOWN \r\n"))
    {
        volState = VBATT_UNDER_PDN;
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\nOVER_VOLTAGE WARNING \r\n"))
    {
        volState = VBATT_OVER_WRN;
    }
    else if (Ql_StrPrefixMatch(strURC, "\r\nOVER_VOLTAGE POWER DOWN \r\n"))
    {
        volState = VBATT_OVER_PDN;
    }else{
        return;
    }
    Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MODULE_VOLTAGE_IND, volState);
}

static void OnURCHandler_InitStat(const char* strURC, void* reserved)
{
    u32 sysInitStat = SYS_STATE_START;
    
    if (Ql_strstr(strURC, "\r\nCall Ready\r\n") != NULL)
    {
        sysInitStat = SYS_STATE_PHBOK;
    }
    else if(Ql_strstr(strURC, "\r\nSMS Ready\r\n") != NULL)
    {
        sysInitStat = SYS_STATE_SMSOK;
    }
    Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_SYS_INIT_STATE_IND, sysInitStat);
}

static void OnURCHandler_CFUN(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    char strTmp[10];
    s32 len;
    u32 cfun;

    len = Ql_strlen("\r\n+CFUN: ");
    p1 = Ql_strstr(strURC, "\r\n+CFUN: ");
    p1 += len;
    p2 = Ql_strstr(p1, "\r\n");
    if (p1 && p2)
    {
        Ql_memset(strTmp, 0x0, sizeof(strTmp));
        Ql_memcpy(strTmp, p1, 1);
        cfun = Ql_atoi(strTmp);
        Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_CFUN_STATE_IND, cfun);
    }
}

static void OnURCHandler_HTTP(const char* strURC, void* reserved)
{
    u32 dwnLoadedSize = 0;
    u32 contentLen = 0;
    s32 errCode = 0;
    extern CB_HTTP_DwnldFile callback_http_dwnld;

    //+QHTTPDL: 23772,23772,0
    Ql_sscanf(strURC, "%*[^: ]: %d,%d,%d[^\r\n]", &dwnLoadedSize, &contentLen, &errCode);
    if (callback_http_dwnld)
    {
        callback_http_dwnld(dwnLoadedSize, contentLen, errCode);
        callback_http_dwnld = NULL;
    }
}

static void OnURCHandler_FTP_Upload(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    s32 nFtpDlLen = 0;
    char strTmp[10];
   
  	p1 = Ql_strstr(strURC, "\r\n+QFTPPUT:");
    p1 += Ql_strlen("\r\n+QFTPPUT:");
    p2 = Ql_strstr(p1, "\r\n");
	
    if (p1 && p2)
    {
        Ql_memset(strTmp, 0x0, sizeof(strTmp));
        Ql_memcpy(strTmp, p1, p2 - p1);
        nFtpDlLen = Ql_atoi(strTmp);
        //TODO:
        
        if(NULL != FtpPut_IND_CB)
        {
            if(nFtpDlLen < 0)
            {
                FtpPut_IND_CB(0,nFtpDlLen);
            }
            else
            {
                FtpPut_IND_CB(1,nFtpDlLen);
            }
            FtpPut_IND_CB = NULL;
            return;
        }
    }
}

static void OnURCHandler_FTP_Download(const char* strURC, void* reserved)
{
    char* p1 = NULL;
    char* p2 = NULL;
    s32 nFtpDlLen = 0;
    char strTmp[10];
	
   p1 = Ql_strstr(strURC, "\r\n+QFTPGET:");
   p1 += Ql_strlen("\r\n+QFTPGET:");
   p2 = Ql_strstr(p1, "\r\n");
   
  
    if (p1 && p2)
    {
       
        Ql_memset(strTmp, 0x0, sizeof(strTmp));
        Ql_memcpy(strTmp, p1, p2 - p1);
        nFtpDlLen = Ql_atoi(strTmp);
        //TODO:
        if(NULL != FtpGet_IND_CB)
        {
            if(nFtpDlLen < 0)
            {
                FtpGet_IND_CB(0,nFtpDlLen);
            }
            else
            {
                FtpGet_IND_CB(1,nFtpDlLen);
            }
            FtpGet_IND_CB = NULL;
            return;
        }
    }

}

static void OnURCHandler_AlarmRing(const char* strURC, void* reserved)
{
	Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_ALARM_RING_IND, 0);
}

static void OnURCHandler_Undefined(const char* strURC, void* reserved)
{
    Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_END, 0);
}

static void OnURCHandler_AudPlayInd(const char* strURC, void* reserved)
{
    s32 errCode1 = 0;
    s32 errCode2 = 0;
    extern RIL_AUD_PLAY_IND cb_aud_play;

    //"+QAUDPIND: 0,<errCode>"
    //"+QPLAYRES: 0,%d"
    //"+QPRESBG: 0,%d"
    Ql_sscanf(strURC, "%*[^: ]: %d,%d[^\r\n]", &errCode1, &errCode2);
    if (cb_aud_play)
    {
        cb_aud_play(errCode2);
    }
}

/*****************************************************************
* Function:     OnURCHandler 
* 
* Description:
*               This function is the entrance for Unsolicited Result Code (URC) Handler.
*
* Parameters:
*               strURC:      
*                   [IN] a URC string terminated by '\0'.
*
*               reserved:       
*                   reserved, can be NULL.
* Return:        
*               The function returns "ptrUrc".
*****************************************************************/
void OnURCHandler(const char* strURC, void* reserved)
{
    s32 i;
    
    if (NULL == strURC)
    {
        return;
    }

    // For system URCs
    for (i = 0; i < NUM_ELEMS(m_SysURCHdlEntry); i++)
    {
        if (Ql_strstr(strURC, m_SysURCHdlEntry[i].keyword))
        {
            m_SysURCHdlEntry[i].handler(strURC, reserved);
            return;
        }
    }

    // For AT URCs
    for (i = 0; i < NUM_ELEMS(m_AtURCHdlEntry); i++)
    {
        if (Ql_strstr(strURC, m_AtURCHdlEntry[i].keyword))
        {
            m_AtURCHdlEntry[i].handler(strURC, reserved);
            return;
        }
    }

    // For undefined URCs
    OnURCHandler_Undefined(strURC, reserved);
}

/******************************************************************************
* Function:     Ql_RIL_IsURCStr
*  
* Description:
*               This function is used to check whether a string is URC information
*               you defined.
.
* Parameters:    
*               strRsp: 
*                     [in]a string for the response of the AT command.
* Return:  
*               0 : not URC information
*               1 : URC information
******************************************************************************/
s32 Ql_RIL_IsURCStr(const char* strRsp)
{
    s32 i;
    for (i = 0; i < NUM_ELEMS(m_SysURCHdlEntry); i++) 
    {
        if (Ql_strstr(strRsp, m_SysURCHdlEntry[i].keyword)) 
        {
            return 1;
        }
    }
    for (i = 0; i < NUM_ELEMS(m_AtURCHdlEntry); i++) 
    {
        if (Ql_strstr(strRsp, m_AtURCHdlEntry[i].keyword)) 
        {
            return 1;
        }
    }
    return 0;
}

static void OnURCHandler_MQTT_OPEN(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	
	p1 = Ql_strstr(strURC, "+QMTOPEN:");
	p1 += Ql_strlen("+QMTOPEN:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.result= Ql_atoi(strTmp);
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_OPEN, &mqtt_urc_param);	
	}

}
static void OnURCHandler_MQTT_CONN(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	
	p1 = Ql_strstr(strURC, "+QMTCONN:");
	p1 += Ql_strlen("+QMTCONN:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.result= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		if(TRUE == QSDK_Get_Str(p1,strTmp,2))
		{
		  mqtt_urc_param.connect_code= Ql_atoi(strTmp);
		}
		else
		{
			mqtt_urc_param.connect_code= 255;
		}
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_CONN, &mqtt_urc_param);
	}
}
static void OnURCHandler_MQTT_SUB(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	char i;
	
	p1 = Ql_strstr(strURC, "+QMTSUB:");
	p1 += Ql_strlen("+QMTSUB:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.msgid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,2);
		mqtt_urc_param.result= Ql_atoi(strTmp);

		for(i=0;i<MQTT_MAX_TOPIC;i++)
        {
			Ql_memset(strTmp, 0x0,	sizeof(strTmp));
     		if(TRUE == QSDK_Get_Str(p1,strTmp,(3+i)))
     		{
     		  mqtt_urc_param.sub_value[i]= Ql_atoi(strTmp);
     		}
     		else
     		{
     			mqtt_urc_param.sub_value[i]= 255;
				break;
     		}
		}

		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_SUB, &mqtt_urc_param);	
	}

}
static void OnURCHandler_MQTT_PUB(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];

	p1 = Ql_strstr(strURC, "+QMTPUB:");
	p1 += Ql_strlen("+QMTPUB:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';
	

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.msgid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,2);
		mqtt_urc_param.result= Ql_atoi(strTmp);

  		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		if(TRUE == QSDK_Get_Str(p1,strTmp,3))
		{
		   mqtt_urc_param.pub_value= Ql_atoi(strTmp);
		}
		else
		{
			mqtt_urc_param.pub_value= 255;
		}
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_PUB, &mqtt_urc_param);	
	}
}
static void OnURCHandler_MQTT_TUNS(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	
	p1 = Ql_strstr(strURC, "+QMTUNS:");
	p1 += Ql_strlen("+QMTUNS:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.msgid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,2);
		mqtt_urc_param.result= Ql_atoi(strTmp);
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_TUNS, &mqtt_urc_param);	
	}
}

static void OnURCHandler_MQTT_STATE(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	
	p1 = Ql_strstr(strURC, "+QMTSTAT:");
	p1 += Ql_strlen("+QMTSTAT:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.mqtt_state= Ql_atoi(strTmp);
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_STATE, &mqtt_urc_param);	
	}
}
static void OnURCHandler_MQTT_CLOSE(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	p1 = Ql_strstr(strURC, "+QMTCLOSE:");
	p1 += Ql_strlen("+QMTCLOSE:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.msgid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,2);
		mqtt_urc_param.result= Ql_atoi(strTmp);
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_CLOSE, &mqtt_urc_param);	
	}
}

static void OnURCHandler_MQTT_DISC(const char* strURC, void* reserved)
{
	char* p1 = NULL;
	char* p2 = NULL;
	char strTmp[10];
	
	p1 = Ql_strstr(strURC, "+QMTDISC:");
	p1 += Ql_strlen("+QMTDISC:");
	p1++;
	p2 = Ql_strstr(p1, "\r\n");
	*p2 = '\0';

	if (p1)
	{
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,0);
		mqtt_urc_param.connectid= Ql_atoi(strTmp);
		
		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,1);
		mqtt_urc_param.msgid= Ql_atoi(strTmp);

		Ql_memset(strTmp, 0x0,	sizeof(strTmp));
		QSDK_Get_Str(p1,strTmp,2);
		mqtt_urc_param.result= Ql_atoi(strTmp);
		
		Ql_OS_SendMessage(URC_RCV_TASK_ID, MSG_ID_URC_INDICATION, URC_MQTT_DISC, &mqtt_urc_param);	
	}
}
#endif  // __OCPU_RIL_SUPPORT__
