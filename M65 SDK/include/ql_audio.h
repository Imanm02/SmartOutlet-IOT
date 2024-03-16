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
 *   ql_audio.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The module defines the information, and APIs related to the ADC function.
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
#ifndef __QL_AUDIO_H__
#define __QL_AUDIO_H__

typedef enum
{
	QlPlayBuf_Format_AMR=12,
	QlPlayBuf_Format_MP3=13,
	QlPlayBuf_Format_WAV=15,	  

} QlPlayBuf_Format;

typedef enum
{
    AUDIO_ERR_NO = 0,
    AUDIO_ERR_BUSY = 2,
    AUDIO_ERR_INVALID_PARAMETER = 3,
    AUDIO_ERR_CANNOT_OPEN_FILE = 6,
    AUDIO_ERR_BAD_FORMAT = 9,
} AUDIO_ERR_T;


/*****************************************************************
* Function:     Ql_AUD_PlayBuf 
* 
* Description:
*               Used to play audio in buffer.
*
* Parameters:
*               databuf:
*                   point of audio data
*               len:
*                   audio data len
*				format:
*					audio format
*				loop:
*					loop
*				pVolume:
*					audio volume
*				channel:
*					channel of sound output
*                 
* Return:        
*               AUDIO_ERR_NO, this function succeeds.
*               AUDIO_ERR_BUSY, audio is in playing.
*               AUDIO_ERR_INVALID_PARAMETER, input a invalid parameter.
*				AUDIO_ERR_BAD_FORMAT, format error
*
*****************************************************************/
s32 Ql_AUD_PlayBuf(u8 *databuf,u32 len,QlPlayBuf_Format format,u8 loop,u8 pVolume,u8 channel );

/*****************************************************************
* Function:     Ql_AUD_StopPlayBuf 
* 
* Description:
*               Stop playing audio.
*
* Parameters:
*               NA.
*                 
* Return:        
*               NA. 
*****************************************************************/
void Ql_AUD_StopPlayBuf(void);

/*****************************************************************
* Function:     Ql_AUD_GetStatusPlaybuf 
* 
* Description:
*               Get status of playing audio.
*
* Parameters:
*               NA.
*                 
* Return:        
*               0 STOP.
*               1 PLAYING.
*****************************************************************/
u8 Ql_AUD_GetStatusPlaybuf(void);


#endif  //__QL_ADC_H__
