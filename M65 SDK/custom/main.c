#ifdef __CUSTOMER_CODE__
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
#include "ql_time.h"
#include "ql_timer.h"
#include "ql_fs.h"

#if (defined(__OCPU_RIL_SUPPORT__) && defined(__OCPU_RIL_SMS_SUPPORT__))

#define DEBUG_ENABLE 1
#if DEBUG_ENABLE > 0
#define DEBUG_PORT UART_PORT1
#define DBG_BUF_LEN 512
static char DBG_BUFFER[DBG_BUF_LEN];
#define APP_DEBUG(FORMAT, ...)                                                                                       \
    {                                                                                                                \
        Ql_memset(DBG_BUFFER, 0, DBG_BUF_LEN);                                                                       \
        Ql_sprintf(DBG_BUFFER, FORMAT, ##__VA_ARGS__);                                                               \
        if (UART_PORT2 == (DEBUG_PORT))                                                                              \
        {                                                                                                            \
            Ql_Debug_Trace(DBG_BUFFER);                                                                              \
        }                                                                                                            \
        else                                                                                                         \
        {                                                                                                            \
            Ql_UART_Write((Enum_SerialPort)(DEBUG_PORT), (u8 *)(DBG_BUFFER), Ql_strlen((const char *)(DBG_BUFFER))); \
        }                                                                                                            \
    }
#else
#define APP_DEBUG(FORMAT, ...)
#endif

#define SERIAL_RX_BUFFER_LEN 2048
// Define the UART port and the receive data buffer
static Enum_SerialPort m_myUartPort = UART_PORT1;
static u8 m_RxBuf_Uart1[SERIAL_RX_BUFFER_LEN];
static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara);

#define CLOCK PINNAME_PCM_OUT
#define LATCH PINNAME_PCM_IN
#define DATA PINNAME_PCM_SYNC

#define OUT_1 PINNAME_DTR
#define OUT_2 PINNAME_CTS
#define OUT_3 PINNAME_RTS

#define LSBFIRST 0
#define MSBFIRST 1

#define TIMER_ID_OUTPUT1 1001
#define TIMER_ID_OUTPUT2 1002
#define TIMER_ID_OUTPUT3 1003
#define TIMER_ID_OUTPUT4 1004
#define TIMER_ID_OUTPUT5 1005
#define TIMER_ID_OUTPUT6 1006

static ST_Time currentTime;

static u32 targetTimeForOutput = 0;
static u32 targetTimestamp = 0;
static u32 currentTimestamp = 0;
static u32 timerDuration1 = 0;
static u32 timerDuration2 = 0;
static u32 timerDuration3 = 0;
static u32 timerDuration4 = 0;
static u32 timerDuration5 = 0;
static u32 timerDuration6 = 0;

union {
    struct {
        u8 out_1 : 1;
        u8 out_2 : 1;
        u8 out_3 : 1;
        u8 out_4 : 1;
        u8 out_5 : 1;
        u8 out_6 : 1;
    } set;
    u8 all;
} Data;

#define IN_1 PINNAME_RXD_AUX

char admin[22];
char users[4][15];

int mode = 15;

enum {
    NORMAL = 10,
    SET_ADMIN = 15,
};

#define DATA_FILE_PATH "data.txt"
#define LENGTH 60
s32 handle = -1;
u8 file_content[LENGTH] = {0};
s32 readenLen = 0;

#define CON_SMS_BUF_MAX_CNT (1)
#define CON_SMS_SEG_MAX_CHAR (160)
#define CON_SMS_SEG_MAX_BYTE (4 * CON_SMS_SEG_MAX_CHAR)
#define CON_SMS_MAX_SEG (7)

typedef struct {
    u8 aData[CON_SMS_SEG_MAX_BYTE];
    u16 uLen;
} ConSMSSegStruct;

typedef struct {
    u16 uMsgRef;
    u8 uMsgTot;

    ConSMSSegStruct asSeg[CON_SMS_MAX_SEG];
    bool abSegValid[CON_SMS_MAX_SEG];
} ConSMSStruct;

static bool ConSMSBuf_IsIntact(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx, ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_AddSeg(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx, ST_RIL_SMS_Con *pCon, u8 *pData, u16 uLen);
static s8 ConSMSBuf_GetIndex(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, ST_RIL_SMS_Con *pCon);
static bool ConSMSBuf_ResetCtx(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx);

ConSMSStruct g_asConSMSBuf[CON_SMS_BUF_MAX_CNT];

u32 TimeStringToSeconds(const char* timeStr) {
    int hh, mm, ss;
    Ql_sscanf(timeStr, "%d:%d:%d", &hh, &mm, &ss);
    return hh * 3600 + mm * 60 + ss;
}

u32 ConvertToTimestamp(const ST_Time *time) {
    static const int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    long days = time->year * 365L + (time->year / 4) - (time->year / 100) + (time->year / 400);
    for (int i = 0; i < time->month - 1; i++) {
        days += monthDays[i];
    }
    if (time->month > 2 && (time->year % 4 == 0 && (time->year % 100 != 0 || time->year % 400 == 0))) {
        days += 1;
    }
    days += time->day - 1;
    long seconds = days * 86400L + time->hour * 3600 + time->minute * 60 + time->second;

    return (u32)seconds;
}

u32 TimeStringToTimestamp(const char* timeStr) {
    int hh, mm, ss;
    Ql_GetLocalTime(&currentTime);

    Ql_sscanf(timeStr, "%d:%d:%d", &hh, &mm, &ss);
    ST_Time targetTime = currentTime;
    targetTime.hour = hh;
    targetTime.minute = mm;
    targetTime.second = ss;

    return ConvertToTimestamp(&targetTime);
}

static bool ConSMSBuf_ResetCtx(ConSMSStruct *pCSBuf, u8 uCSMaxCnt, u8 uIdx) {
    if ((NULL == pCSBuf) || (0 == uCSMaxCnt) || (uIdx >= uCSMaxCnt)) {
        APP_DEBUG("Enter ConSMSBuf_ResetCtx,FAIL! Parameter is INVALID. pCSBuf:%x,uCSMaxCnt:%d,uIdx:%d\r\n", pCSBuf, uCSMaxCnt, uIdx);
        return FALSE;
    }

    // Default reset
    Ql_memset(&pCSBuf[uIdx], 0x00, sizeof(ConSMSStruct));

    // TODO: Add special reset here

    return TRUE;
}

static bool SMS_Initialize(void) {
    s32 iResult = 0;
    u8 nCurrStorage = 0;
    u32 nUsed = 0;
    u32 nTotal = 0;

    // Delete all existed short messages (if needed)
    // iResult = RIL_SMS_DeleteSMS(0, RIL_SMS_DEL_ALL_MSG);
    iResult = RIL_SMS_DeleteSMS(1, RIL_SMS_DEL_ALL_MSG);
    if (iResult != RIL_AT_SUCCESS) {
        APP_DEBUG("Fail to delete all messages, iResult=%d,cause:%d\r\n", iResult, Ql_RIL_AT_GetErrCode());
        return FALSE;
    }
    APP_DEBUG("Delete all existed messages\r\n");

    return TRUE;
}

void SMS_TextMode_Send(char *phone, char *msg) {
    s32 iResult;
    u32 nMsgRef;

    ST_RIL_SMS_SendExt sExt;

    // Initialize
    Ql_memset(&sExt, 0x00, sizeof(sExt));

    APP_DEBUG("< Send Normal Text SMS begin... >\r\n");

    iResult = RIL_SMS_SendSMS_Text(phone, Ql_strlen(phone), LIB_SMS_CHARSET_GSM, msg, Ql_strlen(msg), &nMsgRef);
    if (iResult != RIL_AT_SUCCESS) {
        APP_DEBUG("< Fail to send Text SMS, iResult=%d, cause:%d >\r\n", iResult, Ql_RIL_AT_GetErrCode());
        return;
    }
    APP_DEBUG("< Send Text SMS successfully, MsgRef:%u >\r\n", nMsgRef);
}

void TimerCallback1(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT1) {
        Data.set.out_1 = 0;  // Turn off output 1
        update_IO();
        APP_DEBUG("Output 1 turned off due to timer\r\n");
    }
}

void TimerCallback2(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT2) {
        Data.set.out_2 = 0;  // Turn off output 2
        update_IO();
        APP_DEBUG("Output 2 turned off due to timer\r\n");
    }
}

void TimerCallback3(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT3) {
        Data.set.out_3 = 0;  // Turn off output 3
        update_IO();
        APP_DEBUG("Output 3 turned off due to timer\r\n");
    }
}

void TimerCallback4(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT4) {
        Data.set.out_4 = 0;  // Turn off output 4
        update_IO();
        APP_DEBUG("Output 4 turned off due to timer\r\n");
    }
}

void TimerCallback5(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT5) {
        Data.set.out_5 = 0;  // Turn off output 5
        update_IO();
        APP_DEBUG("Output 5 turned off due to timer\r\n");
    }
}

void TimerCallback6(u32 timerId, void* param) {
    if (timerId == TIMER_ID_OUTPUT6) {
        Data.set.out_6 = 0;  // Turn off output 6
        update_IO();
        APP_DEBUG("Output 6 turned off due to timer\r\n");
    }
}

void MillisecondsToTimeStr(u32 ms, char* timeStr) {
    u32 seconds = ms / 1000;
    u32 hours = seconds / 3600;
    seconds %= 3600;
    u32 minutes = seconds / 60;
    seconds %= 60;

    Ql_sprintf(timeStr, "%02u:%02u:%02u", hours, minutes, seconds);
}

static void Hdlr_RecvNewSMS(u32 nIndex, bool bAutoReply) {
    s32 iResult = 0;
    u32 uMsgRef = 0;
    ST_RIL_SMS_TextInfo *pTextInfo = NULL;
    ST_RIL_SMS_DeliverParam *pDeliverTextInfo = NULL;
    char aPhNum[RIL_SMS_PHONE_NUMBER_MAX_LEN] = {
        0,
    };
    const char aReplyCon[] = {"Module has received SMS."};
    bool bResult = FALSE;

    pTextInfo = Ql_MEM_Alloc(sizeof(ST_RIL_SMS_TextInfo));
    if (NULL == pTextInfo) {
        APP_DEBUG("%s/%d:Ql_MEM_Alloc FAIL! size:%u\r\n", sizeof(ST_RIL_SMS_TextInfo), __func__, __LINE__);
        return;
    }
    Ql_memset(pTextInfo, 0x00, sizeof(ST_RIL_SMS_TextInfo));
    iResult = RIL_SMS_ReadSMS_Text(nIndex, LIB_SMS_CHARSET_GSM, pTextInfo);
    if (iResult != RIL_AT_SUCCESS) {
        Ql_MEM_Free(pTextInfo);
        APP_DEBUG("Fail to read text SMS[%d], cause:%d\r\n", nIndex, iResult);
        return;
    }

    if ((LIB_SMS_PDU_TYPE_DELIVER != (pTextInfo->type)) || (RIL_SMS_STATUS_TYPE_INVALID == (pTextInfo->status))) {
        Ql_MEM_Free(pTextInfo);
        APP_DEBUG("WARNING: NOT a new received SMS.\r\n");
        return;
    }

    pDeliverTextInfo = &((pTextInfo->param).deliverParam);

    APP_DEBUG("<-- RIL_SMS_ReadSMS_Text OK. eCharSet:LIB_SMS_CHARSET_GSM,nIndex:%u -->\r\n", nIndex);
    APP_DEBUG("status:%u,type:%u,alpha:%u,sca:%s,oa:%s,scts:%s,data length:%u\r\n",
              pTextInfo->status,
              pTextInfo->type,
              pDeliverTextInfo->alpha,
              pTextInfo->sca,
              pDeliverTextInfo->oa,
              pDeliverTextInfo->scts,
              pDeliverTextInfo->length);
    APP_DEBUG("data = %s\r\n", (pDeliverTextInfo->data));

    if (mode == SET_ADMIN) {
        if (Ql_strcmp((pDeliverTextInfo->data), "Manager") == 0) {
            APP_DEBUG("Manager = %s\r\n", pDeliverTextInfo->oa);
            Ql_strcpy(users[0], pDeliverTextInfo->oa);
            SMS_TextMode_Send(users[0], "You are manager now!");

            handle = Ql_FS_Open(DATA_FILE_PATH, QL_FS_CREATE_ALWAYS);
            if (handle > 0) {
                Ql_FS_Truncate(handle);
                Ql_FS_Flush(handle);
                Ql_FS_Seek(handle, 0, QL_FS_FILE_BEGIN);
                Ql_FS_Write(handle, users[0], Ql_strlen(users[0]), &readenLen);
                Ql_FS_Flush(handle);
                Ql_FS_Close(handle);
            } else {
                APP_DEBUG("ERROR WRITING File \r\n");
            }
        } else {
            APP_DEBUG("not equal\r\n");
        }
    }

    {
        APP_DEBUG("sender = %s\r\n", pDeliverTextInfo->oa);
        if (Ql_strncmp((pDeliverTextInfo->oa), users[0], 7) == 0 || Ql_strncmp((pDeliverTextInfo->oa), users[1], 7) == 0 || Ql_strncmp((pDeliverTextInfo->oa), users[2], 7) == 0 || Ql_strncmp((pDeliverTextInfo->oa), users[3], 7) == 0) {
            if (Ql_strncmp((pDeliverTextInfo->data), "Add user ", 9) == 0) {
                if (Ql_strncmp((pDeliverTextInfo->oa), users[0], 13) == 0) {
                    if (Ql_strlen(pDeliverTextInfo->data) == 22) {
                        handle = Ql_FS_Open(DATA_FILE_PATH, QL_FS_CREATE);
                        if (handle > 0) {
                            Ql_FS_Seek(handle, 0, QL_FS_FILE_BEGIN);
                            Ql_FS_Read(handle, file_content, LENGTH - 1, &readenLen);
                            APP_DEBUG("file = %s; len :%d\r\n", file_content, readenLen);
                            if ((readenLen / 13) < 4) {
                                APP_DEBUG("start write\r\n");
                                Ql_strcpy(users[(readenLen / 13)], (pDeliverTextInfo->data) + 9);
                                Ql_FS_Seek(handle, 0, QL_FS_FILE_END);
                                Ql_FS_Write(handle, (pDeliverTextInfo->data) + 9, Ql_strlen((pDeliverTextInfo->data) + 9), &readenLen);
                                Ql_FS_Flush(handle);
                                APP_DEBUG("end write\r\n");
                            } else {
                                APP_DEBUG("max users\r\n");
                                SMS_TextMode_Send(pDeliverTextInfo->oa, "max users");
                            }
                            Ql_FS_Close(handle);
                        }
                    } else {
                        APP_DEBUG("Incorrect phone number\r\n");
                        SMS_TextMode_Send(pDeliverTextInfo->oa, "Incorrect phone number");
                    }
                } else {
                    APP_DEBUG("You are not manager\r\n");
                    SMS_TextMode_Send(pDeliverTextInfo->oa, "You are not manager");
                }
            }

            if (Ql_strcmp((pDeliverTextInfo->data), "Get All Users") == 0) {
                char temp[100];

                Ql_sprintf(temp, "Manager: %s\r\nFirst User: %s\r\nSecond User: %s\r\nThird User: %s\r\n\0", users[0], users[1], users[2], users[3]);
                APP_DEBUG("Manager: %s\r\nFirst User: %s\r\nSecond User: %s\r\nThird User: %s\r\n", users[0], users[1], users[2], users[3]);
                SMS_TextMode_Send(users[0], temp);
            }

            if (Ql_strcmp((pDeliverTextInfo->data), "Get Outputs") == 0) {
                char temp[100];

                Ql_sprintf(temp, "OUT1: %d\r\nOUT2: %d\r\nOUT3: %d\r\nOUT4: %d\r\nOUT5: %d\r\nOUT6: %d\r\n\0", Data.set.out_1, Data.set.out_2, Data.set.out_3, Data.set.out_4, Data.set.out_5, Data.set.out_6);
                APP_DEBUG("OUT1: %d\r\nOUT2: %d\r\nOUT3: %d\r\nOUT4: %d\r\nOUT5: %d\r\nOUT6: %d\r\n\0", Data.set.out_1, Data.set.out_2, Data.set.out_3, Data.set.out_4, Data.set.out_5, Data.set.out_6);
                SMS_TextMode_Send(users[0], temp);
            }

            if (Ql_strcmp((pDeliverTextInfo->data), "ALL OFF") == 0) {
                APP_DEBUG("Turning all outputs to off state\r\n");
                Data.set.out_1 = 0;
                Data.set.out_2 = 0;
                Data.set.out_3 = 0;
                Data.set.out_4 = 0;
                Data.set.out_5 = 0;
                Data.set.out_6 = 0;
                update_IO();
                APP_DEBUG("OUT1 = %d\r\n", Data.set.out_1)
                APP_DEBUG("OUT2 = %d\r\n", Data.set.out_2)
                APP_DEBUG("OUT3 = %d\r\n", Data.set.out_3)
                APP_DEBUG("OUT4 = %d\r\n", Data.set.out_4)
                APP_DEBUG("OUT5 = %d\r\n", Data.set.out_5)
                APP_DEBUG("OUT6 = %d\r\n", Data.set.out_6)
            }

            if (Ql_strcmp((pDeliverTextInfo->data), "ALL ON") == 0) {
                APP_DEBUG("Turning all outputs to on state\r\n");
                Data.set.out_1 = 1;
                Data.set.out_2 = 1;
                Data.set.out_3 = 1;
                Data.set.out_4 = 1;
                Data.set.out_5 = 1;
                Data.set.out_6 = 1;
                update_IO();
                APP_DEBUG("OUT1 = %d\r\n", Data.set.out_1)
                APP_DEBUG("OUT2 = %d\r\n", Data.set.out_2)
                APP_DEBUG("OUT3 = %d\r\n", Data.set.out_3)
                APP_DEBUG("OUT4 = %d\r\n", Data.set.out_4)
                APP_DEBUG("OUT5 = %d\r\n", Data.set.out_5)
                APP_DEBUG("OUT6 = %d\r\n", Data.set.out_6)
            }

            if (Ql_strcmp(pDeliverTextInfo->data, "Current Time") == 0) {
                char timeStr[20];

                Ql_GetLocalTime(&currentTime); // Get the current time
                Ql_sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d", 
                        currentTime.year, currentTime.month, currentTime.day,
                        currentTime.hour, currentTime.minute, currentTime.second);

                SMS_TextMode_Send(pDeliverTextInfo->oa, timeStr); // Send current time as SMS
                APP_DEBUG("Current Time sent: %s\r\n", timeStr);
            }

            if (Ql_strncmp((pDeliverTextInfo->data), "ON OUT ", 7) == 0) {
                APP_DEBUG("ON = %d\r\n", pDeliverTextInfo->data[7]);
                if (pDeliverTextInfo->data[7] > 47 && pDeliverTextInfo->data[7] < 58) {
                    switch (pDeliverTextInfo->data[7] - 48) {
                    case 1:
                        Data.set.out_1 = 1;
                        break;
                    case 2:
                        Data.set.out_2 = 1;
                        break;
                    case 3:
                        Data.set.out_3 = 1;
                        break;
                    case 4:
                        Data.set.out_4 = 1;
                        break;
                    case 5:
                        Data.set.out_5 = 1;
                        break;
                    case 6:
                        Data.set.out_6 = 1;
                        break;

                    default:
                        break;
                    }
                    update_IO();
                }
                APP_DEBUG("OUT1 = %d\r\n", Data.set.out_1)
                APP_DEBUG("OUT2 = %d\r\n", Data.set.out_2)
                APP_DEBUG("OUT3 = %d\r\n", Data.set.out_3)
                APP_DEBUG("OUT4 = %d\r\n", Data.set.out_4)
                APP_DEBUG("OUT5 = %d\r\n", Data.set.out_5)
                APP_DEBUG("OUT6 = %d\r\n", Data.set.out_6)
            }

            if (Ql_strncmp(pDeliverTextInfo->data, "UNTIL ", 6) == 0) {
                char timeStr[10] = {0};
                Ql_strncpy(timeStr, pDeliverTextInfo->data + 6, 8); // Extract time string
                targetTimestamp = TimeStringToTimestamp(timeStr);
                currentTimestamp = ConvertToTimestamp(Ql_GetLocalTime(&currentTime));
                APP_DEBUG("ON UNTIL = %d\r\n", pDeliverTextInfo->data[22]);
                if (pDeliverTextInfo->data[22] > 47 && pDeliverTextInfo->data[22] < 58) {
                    APP_DEBUG("Target time: %d\r\n", targetTimestamp);
                    APP_DEBUG("Current time: %d\r\n", currentTimestamp);
                    if (targetTimestamp > currentTimestamp) {
                        switch (pDeliverTextInfo->data[22] - 48) {
                        case 1:
                            timerDuration1 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration1);
                            Data.set.out_1 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT1, timerDuration1 * 1000, FALSE); // Start timer
                            break;
                        case 2:
                            timerDuration2 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration2);
                            Data.set.out_2 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT2, timerDuration2 * 1000, FALSE); // Start timer
                            break;
                        case 3:
                            timerDuration3 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration3);
                            Data.set.out_3 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT3, timerDuration3 * 1000, FALSE); // Start timer
                            break;
                        case 4:
                            timerDuration4 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration4);
                            Data.set.out_4 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT4, timerDuration4 * 1000, FALSE); // Start timer
                            break;
                        case 5:
                            timerDuration5 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration5);
                            Data.set.out_5 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT5, timerDuration5 * 1000, FALSE); // Start timer
                            break;
                        case 6:
                            timerDuration6 = targetTimestamp - currentTimestamp;
                            APP_DEBUG("Time duration: %d\r\n", timerDuration6);
                            Data.set.out_6 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT6, timerDuration6 * 1000, FALSE); // Start timer
                            break;

                        default:
                            break;
                        }
                        update_IO();
                    }
                }
                update_IO();
                APP_DEBUG("Output %d turned on until %s\r\n", pDeliverTextInfo->data[22] - 48, timeStr);
            }

            if (Ql_strncmp(pDeliverTextInfo->data, "FOR ", 4) == 0) {
                char timeStr[10] = {0};
                Ql_strncpy(timeStr, pDeliverTextInfo->data + 4, 3); // Extract time string
                timeStr[3] = '\0';
                APP_DEBUG("ON FOR = %d\r\n", pDeliverTextInfo->data[15]);
                if (pDeliverTextInfo->data[15] > 47 && pDeliverTextInfo->data[15] < 58) {
                        switch (pDeliverTextInfo->data[15] - 48) {
                        case 1:
                            timerDuration1 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration1);
                            Data.set.out_1 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT1, timerDuration1 * 1000, FALSE); // Start timer
                            break;
                        case 2:
                            timerDuration2 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration2);
                            Data.set.out_2 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT2, timerDuration2 * 1000, FALSE); // Start timer
                            break;
                        case 3:
                            timerDuration3 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration3);
                            Data.set.out_3 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT3, timerDuration3 * 1000, FALSE); // Start timer
                            break;
                        case 4:
                            timerDuration4 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration4);
                            Data.set.out_4 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT4, timerDuration4 * 1000, FALSE); // Start timer
                            break;
                        case 5:
                            timerDuration5 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration5);
                            Data.set.out_5 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT5, timerDuration5 * 1000, FALSE); // Start timer
                            break;
                        case 6:
                            timerDuration6 = (u32)Ql_atoi(timeStr);
                            APP_DEBUG("Time duration: %d\r\n", timerDuration6);
                            Data.set.out_6 = 1;
                            update_IO();
                            Ql_Timer_Start(TIMER_ID_OUTPUT6, timerDuration6 * 1000, FALSE); // Start timer
                            break;

                        default:
                            break;
                        }
                        update_IO();
                }
                update_IO();
                APP_DEBUG("Output %d turned on for %s\r seconds \n", pDeliverTextInfo->data[15] - 48, timeStr);
            }

            if (Ql_strncmp((pDeliverTextInfo->data), "OFF OUT ", 8) == 0) {
                APP_DEBUG("OFF = %d\r\n", pDeliverTextInfo->data[8]);
                if (pDeliverTextInfo->data[8] > 47 && pDeliverTextInfo->data[8] < 58) {
                    switch (pDeliverTextInfo->data[8] - 48) {
                    case 1:
                        Data.set.out_1 = 0;
                        break;
                    case 2:
                        Data.set.out_2 = 0;
                        break;
                    case 3:
                        Data.set.out_3 = 0;
                        break;
                    case 4:
                        Data.set.out_4 = 0;
                        break;
                    case 5:
                        Data.set.out_5 = 0;
                        break;
                    case 6:
                        Data.set.out_6 = 0;
                        break;

                    default:
                        break;
                    }
                    update_IO();
                }
                APP_DEBUG("OUT1 = %d\r\n", Data.set.out_1)
                APP_DEBUG("OUT2 = %d\r\n", Data.set.out_2)
                APP_DEBUG("OUT3 = %d\r\n", Data.set.out_3)
                APP_DEBUG("OUT4 = %d\r\n", Data.set.out_4)
                APP_DEBUG("OUT5 = %d\r\n", Data.set.out_5)
                APP_DEBUG("OUT6 = %d\r\n", Data.set.out_6)
            }
        }
    }

    Ql_strcpy(aPhNum, pDeliverTextInfo->oa);
    Ql_MEM_Free(pTextInfo);

    return;
}

void proc_subtask1(s32 TaskId) {
    int status = 0;
    while (TRUE) {
        if (mode == SET_ADMIN) {
            Ql_GPIO_SetLevel(OUT_1, PINLEVEL_HIGH);
            Ql_GPIO_SetLevel(OUT_2, PINLEVEL_HIGH);
            Ql_GPIO_SetLevel(OUT_3, PINLEVEL_HIGH);
            Ql_Sleep(200);
            Ql_GPIO_SetLevel(OUT_1, PINLEVEL_LOW);
            Ql_GPIO_SetLevel(OUT_2, PINLEVEL_LOW);
            Ql_GPIO_SetLevel(OUT_3, PINLEVEL_LOW);
            Ql_Sleep(200);
        }
        Ql_Sleep(500);
    }
}

static s32 ReadSerialPort(Enum_SerialPort port, /*[out]*/ u8 *pBuffer, /*[in]*/ u32 bufLen) {
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen) {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1) {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0) { // All data is read out, or Serial Port Error!
            break;
        }
        rdTotalLen += rdLen;
        // Continue to read...
    }
    if (rdLen < 0) { // Serial Port Error!    
        APP_DEBUG("Fail to read from port[%d]\r\n", port);
        return -99;
    }
    return rdTotalLen;
}

static void CallBack_UART_Hdlr(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara) {
    switch (msg) {
    case EVENT_UART_READY_TO_READ: {
        char *p = NULL;
        s32 totalBytes = ReadSerialPort(port, m_RxBuf_Uart1, sizeof(m_RxBuf_Uart1));
        if (totalBytes <= 0) {
            break;
        }

        APP_DEBUG("received %s\r\n", m_RxBuf_Uart1);

        switch (m_RxBuf_Uart1[0]) {
        case '1':
            Data.set.out_1 = !Data.set.out_1;
            break;
        case '2':
            Data.set.out_2 = !Data.set.out_2;
            break;
        case '3':
            Data.set.out_3 = !Data.set.out_3;
            break;
        case '4':
            Data.set.out_4 = !Data.set.out_4;
            break;
        case '5':
            Data.set.out_5 = !Data.set.out_5;
            break;
        case '6':
            Data.set.out_6 = !Data.set.out_6;
            break;
        default:
            Data.all = 0;
            break;
        }
        update_IO();

        break;
      }
    }
}

static void InitSerialPort(void) {
    s32 iResult = 0;

    // Register & Open UART port
    iResult = Ql_UART_Register(UART_PORT1, CallBack_UART_Hdlr, NULL);
    if (iResult != QL_RET_OK) {
        Ql_Debug_Trace("Fail to register UART port[%d]:%d\r\n", UART_PORT1);
    }

    iResult = Ql_UART_Open(UART_PORT1, 115200, FC_NONE);
    if (iResult != QL_RET_OK) {
        Ql_Debug_Trace("Fail to open UART port[%d], baud rate:115200, FC_NONE\r\n", UART_PORT1);
    }
}

void check_file() {
    handle = Ql_FS_Open(DATA_FILE_PATH, QL_FS_READ_ONLY);
    if (handle > 0) {
        Ql_FS_Seek(handle, 0, QL_FS_FILE_BEGIN);
        Ql_FS_Read(handle, file_content, LENGTH - 1, &readenLen);
        Ql_FS_Close(handle);
        Ql_strncpy(users[0], file_content, 13);
        Ql_strncpy(users[1], file_content + 13, 13);
        Ql_strncpy(users[2], file_content + 26, 13);
        Ql_strncpy(users[3], file_content + 39, 13);
        APP_DEBUG("Manager: %s\r\n First User: %s\r\n Second User : %s\r\n Third User: %s\r\n", users[0], users[1], users[2], users[3]);
        APP_DEBUG("The content of the file:%s\r\n", file_content);
    } else {
        APP_DEBUG("Couldn't find the file \r\n");
    }
}

void proc_main_task(s32 iTaskID) {
    s32 iResult = 0;
    ST_MSG taskMsg;

    // Register & open UART port
    InitSerialPort();

    APP_DEBUG("SMS Controller Embedding Systems\r\n");

    iResult = Ql_IIC_Init(1, PINNAME_RI, PINNAME_DCD, FALSE);
    APP_DEBUG("IIC init State %d\r\n", iResult);
    Ql_Sleep(100);
    iResult = Ql_IIC_Config(1, TRUE, 0x40, 100);
    APP_DEBUG("IIC Config State%d\r\n", iResult);

    Data.all =0;

    // enable led
    Ql_GPIO_Init(OUT_1, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_PULLUP);
    Ql_GPIO_Init(OUT_2, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_PULLUP);
    Ql_GPIO_Init(OUT_3, PINDIRECTION_OUT, PINLEVEL_LOW, PINPULLSEL_PULLUP);

    Ql_GPIO_Init(IN_1, PINDIRECTION_IN, PINLEVEL_HIGH, PINPULLSEL_PULLUP);

    Ql_GPIO_SetLevel(OUT_1, PINLEVEL_HIGH);
    Ql_GPIO_SetLevel(OUT_2, PINLEVEL_HIGH);
    Ql_GPIO_SetLevel(OUT_3, PINLEVEL_HIGH);
    Ql_Sleep(200);
    Ql_GPIO_SetLevel(OUT_1, PINLEVEL_LOW);
    Ql_GPIO_SetLevel(OUT_2, PINLEVEL_LOW);
    Ql_GPIO_SetLevel(OUT_3, PINLEVEL_LOW);

    // Set the system time
    ST_Time CTime;
    CTime.year = 2024;  // Set the correct year
    CTime.month = 1;    // Set the correct month
    CTime.day = 31;     // Set the correct day
    CTime.hour = 12;    // Set the correct hour
    CTime.minute = 0;   // Set the correct minute
    CTime.second = 0;   // Set the correct second

    Ql_SetLocalTime(&CTime);

    Ql_Timer_Register(TIMER_ID_OUTPUT1, TimerCallback1, NULL);
    Ql_Timer_Register(TIMER_ID_OUTPUT2, TimerCallback2, NULL);
    Ql_Timer_Register(TIMER_ID_OUTPUT3, TimerCallback3, NULL);
    Ql_Timer_Register(TIMER_ID_OUTPUT4, TimerCallback4, NULL);
    Ql_Timer_Register(TIMER_ID_OUTPUT5, TimerCallback5, NULL);
    Ql_Timer_Register(TIMER_ID_OUTPUT6, TimerCallback6, NULL);


    check_file();

    // START MESSAGE LOOP OF THIS TASK
    while (TRUE) {
        s32 i = 0;

        Ql_memset(&taskMsg, 0x0, sizeof(ST_MSG));
        Ql_OS_GetMessage(&taskMsg);
        switch (taskMsg.message) {
        case MSG_ID_RIL_READY: {
            APP_DEBUG("<-- RIL is ready -->\r\n");
            Ql_RIL_Initialize(); // MUST call this function

            for (i = 0; i < CON_SMS_BUF_MAX_CNT; i++) {
                ConSMSBuf_ResetCtx(g_asConSMSBuf, CON_SMS_BUF_MAX_CNT, i);
            }
            break;
        }
        case MSG_ID_URC_INDICATION:
            switch (taskMsg.param1) {
            case URC_SYS_INIT_STATE_IND: {
                APP_DEBUG("<-- Sys Init Status %d -->\r\n", taskMsg.param2);
                if (SYS_STATE_SMSOK == taskMsg.param2) {
                    APP_DEBUG("\r\n<-- SMS module is ready -->\r\n");
                    APP_DEBUG("\r\n<-- Initialize SMS-related options -->\r\n");
                    iResult = SMS_Initialize();
                    if (!iResult) {
                        APP_DEBUG("Fail to initialize SMS\r\n");
                    }

                    // SMS_TextMode_Send();
                }
                break;
            }

            case URC_SIM_CARD_STATE_IND: {
                APP_DEBUG("\r\n<-- SIM Card Status:%d -->\r\n", taskMsg.param2);
                break;
            }

            case URC_GSM_NW_STATE_IND: {
                APP_DEBUG("\r\n<-- GSM Network Status:%d -->\r\n", taskMsg.param2);
                break;
            }

            case URC_GPRS_NW_STATE_IND: {
                APP_DEBUG("\r\n<-- GPRS Network Status:%d -->\r\n", taskMsg.param2);
                break;
            }

            case URC_CFUN_STATE_IND: {
                APP_DEBUG("\r\n<-- CFUN Status:%d -->\r\n", taskMsg.param2);
                break;
            }

            case URC_COMING_CALL_IND: {
                ST_ComingCall *pComingCall = (ST_ComingCall *)(taskMsg.param2);
                APP_DEBUG("\r\n<-- Coming call, number:%s, type:%d -->\r\n", pComingCall->phoneNumber, pComingCall->type);
                break;
            }

            case URC_NEW_SMS_IND: {
                APP_DEBUG("\r\n<-- New SMS Arrives: index=%d\r\n", taskMsg.param2);
                Hdlr_RecvNewSMS((taskMsg.param2), FALSE);
                break;
            }

            case URC_MODULE_VOLTAGE_IND: {
                APP_DEBUG("\r\n<-- VBatt Voltage Ind: type=%d\r\n", taskMsg.param2);
                break;
            }

            default:
                break;
            }
            break;

        default:
            break;
        }
    }
}

void update_IO() {
    pcf8574_setoutput(0, ~Data.all);
}

#endif

#endif //  __CUSTOMER_CODE__