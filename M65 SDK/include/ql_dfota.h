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
 *   ql_dfota.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   DFota API  defines.
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


#ifndef __QL_DFOTA_H__
#define __QL_DFOTA_H__

#include "ql_type.h"

typedef struct
{
    s16 Q_gpio_pin1;        //Watchdog GPIO pin 1, If only use one GPIO,you can set other to -1,it means invalid.
    s16 Q_feed_interval1;   //gpio1 time interval for feed dog.
    s16 Q_gpio_pin2;        //Watchdog GPIO pin 2, If only use one GPIO,you can set other to -1,it means invalid.
    s16 Q_feed_interval2;   //gpio2 time interval for feed dog.
    s32 reserved1;          //reserve 1
    s32 reserved2;          //reserve 2
}ST_FotaConfig;


/*****************************************************************
* Function:     Ql_DFOTA_Init 
* 
* Description:  Initialize DFOTA related functions.
*               It a simple API.Programer only need to pass the
*               simple parameters to this API.
*
* Parameters:
*               pFotaCfg: Initialize dfota config include watch dog. 
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               QL_ERR_DFOTA_INIT_FAIL indicates init fota parameter failed.
*****************************************************************/
s32 Ql_DFOTA_Init(ST_FotaConfig * pFotaCfg);


/*****************************************************************
* Function:     Ql_DFOTA_WriteData 
* 
* Description:
*               DFOTA write data API.
*                1. This function is used to write data to spare image pool
*                2. This API only allow sequentially writing mechanism
*                3. Authentication mechanism is executed during writing
* Parameters:
*               buffer: point to the start address of buffer
*               length: the length of writing (Unit: Bytes).recommend 512 bytes
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_RET_ERR_PARAM indicates parameter error.
*               QL_ERR_DFOTA_FAT_FAIL indicates written flash failure.
*****************************************************************/
s32  Ql_DFOTA_WriteData(u8 * buffer,s32 length);

/*****************************************************************
* Function:     Ql_DFOTA_Finish 
* 
* Description:
*               DFOTA finalization API.
*                1. compare calculated checksum with image checksum in the header after
*                   whole image is written
*                2. mark the status to UPDATE_NEEDED 
* Parameters:
*               None
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_ERR_DFOTA_CHECK_FAIL indicates Upgrade package unavailable.
*****************************************************************/
s32 Ql_DFOTA_Finish(void);

/*****************************************************************
* Function:     Ql_DFota_Update 
* 
* Description:
*               Starts FOTA Update.
* Parameters:
*               None.
* Return:        
*               QL_RET_OK indicates this function successes.
*               QL_ERR_DFOTA_UPGRADE_FAIL indicates upgrade failed. 
*****************************************************************/
s32 Ql_DFOTA_Update(void);

#endif  // End-of __QL_DFOTA_H__
