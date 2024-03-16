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
 *   ql_error.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   error code  defines.
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


#ifndef __QL_ERROR_H__
#define __QL_ERROR_H__

/****************************************************************************
 * Error Code Definition
 ***************************************************************************/
enum {
	QL_RET_OK		  = 0,
	QL_RET_ERR_PARAM  = -1,   /*Peripheral*/
	QL_RET_ERR_ERROR  = -2, 
	QL_RET_ERR_FULL   =  -3,
	QL_RET_ERR_INVALID_PARAM = -4,
	QL_RET_ERR_FATAL	  = -5,
	QL_RET_ERR_INVALID_OP = -6,
	QL_RET_ERR_BUSY 	  =-7,
	QL_RET_ERR_USED 	  = -8,
	QL_RET_ERR_NOT_INIT   = -9,
	QL_RET_ERR_NOSUPPORTEINT  = -10,
	QL_RET_ERR_ALREADY_REGISTERED  = -11,
	QL_RET_ERR_CHANNEL_NOT_FOUND  = -12,
	QL_RET_ERR_IIC_SAME_SLAVE_ADDRESS = -13,
	QL_RET_ERR_I2CHWFAILED	  = -14,
	QL_RET_ERR_IIC_SLAVE_TOO_MANY	  = -15,
	QL_RET_ERR_INVALID_TASK_ID = -16,	
	QL_RET_ERR_UNKNOWN		   = -17,

	QL_RET_ERR_NOEXISTOBJEXT	  = -20,/*flash operate*/
	QL_RET_ERR_OPERATEOBJEXTFAILED		= -21,
	QL_RET_ERR_OPENOBJEXTFAILED 	 = -22,
	QL_RET_ERR_WRITEOBJEXTFAILED	  = -23,
	QL_RET_ERR_READOBJEXTFAILED 	 = -24,
	QL_RET_ERR_FLASHFULLOVER	 = -25,    /*flash full over*/
	QL_RET_ERR_FLASHSPACE			   = -26,
	QL_RET_ERR_DRIVE				 = -27,
	QL_RET_ERR_DRIVEFULLOVER		= -28,
	QL_RET_ERR_INVALIDFLASHID	   = -29,
	QL_RET_ERR_FILEFAILED		  = -30,
	QL_RET_ERR_FILEOPENFAILED	   = -31,
	QL_RET_ERR_FILENAMETOOLENGTH = -32,
	QL_RET_ERR_FILEREADFAILED = -33,
	QL_RET_ERR_FILEWRITEFAILED	= -34,
	QL_RET_ERR_FILESEEKFAILED  = -35,
	QL_RET_ERR_FILENOTFOUND  = -36,
	QL_RET_ERR_FILENOMORE  = -37,
	QL_RET_ERR_FILEDISKFULL = -38,
	QL_RET_ERR_FILE_NO_CARD = -39,
	QL_RET_ERR_API_NO_RESPONSE = -40,
	QL_RET_ERR_API_INVALID_RESPONSE = -41,
	QL_RET_ERR_SMS_EXCEED_LENGTH =-42,
	QL_RET_ERR_SMS_NOT_INIT = -43,
	QL_RET_ERR_INVALID_PARAMETER = -51,
	QL_RET_ERR_PATHNOTFOUND = -52,
	QL_RET_ERR_GET_MEM = -53,
	QL_RET_ERR_GENERAL_FAILURE = -54,
	QL_RET_ERR_FILE_EXISTS = -55,
	QL_RET_ERR_SMS_INVALID_FORMAT = -56,	
	QL_RET_ERR_SMS_GET_FORMAT = -57,		
	QL_RET_ERR_SMS_INVALID_STORAGE = -58,		 
	QL_RET_ERR_SMS_SET_STORAGE = -59,			 
	QL_RET_ERR_SMS_SEND_AT_CMD = -60,
	QL_RET_ERR_API_CMD_BUSY = -61,	
	Ql_RET_ERR_SIM_NOT_INSERTED 			   =-62,
	Ql_RET_ERR_SIM_TYPE_ERROR				   =-63,

	/* AUD -70 ~ -90*/
	QL_RET_ERR_MED_BAD_FORMAT					= -70,
	QL_RET_ERR_MED_BUSY 						= -71,
	QL_RET_ERR_MED_DISC_FULL					= -72,
	QL_RET_ERR_MED_OPEN_FILE_FAIL				= -73,
	QL_RET_ERR_MED_BAD_FILE_EXTENSION			= -74,
	QL_RET_ERR_MED_WRITE_PROTECTION 			= -75,
	QL_RET_ERR_MED_FILE_EXIST					= -76,
	QL_RET_ERR_MED_UNSUPPORT_FMT_IN_CALLING 	= -77,
	Ql_RET_ERR_AUD_REC_STOP_FAIL				= -78,
	QL_RET_ERR_MED_DRIVE_NOT_FOUND				= -79,
	QL_RET_ERR_MED_NO_CARD						= -80,
	Ql_RET_ERR_MEM_FULL 						= -81, 
	QL_ERR_DTMFSTRING_TOO_LONG					= -82, 
	QL_ERR_WDTMF_PS_BUSY						= -83, 
	QL_ERR_DTMF_BUSY							= -84,
	QL_ERR_DTMF_NO_CALLING						= -85,
	/* Dfota to -91 ~ -94*/
	QL_ERR_DFOTA_INIT_FAIL						= -91,
	QL_ERR_DFOTA_FAT_FAIL						= -92,
	QL_ERR_DFOTA_CHECK_FAIL 					= -93,
	QL_ERR_DFOTA_UPGRADE_FAIL					= -94,
	
	Ql_RET_ERR_SYS_NOT_READY					= -98,
	Ql_RET_ERR_UNKOWN							= -99,
	Ql_RET_NOT_SUPPORT							= -100,

	/*  RIL FTP -101   */
    QL_RET_ERR_RIL_FTP_OPENFAIL = -101,   
    QL_RET_ERR_RIL_FTP_CLOSEFAIL = -102,
    QL_RET_ERR_RIL_FTP_SETPATHFAIL = -103,
    QL_RET_ERR_RIL_FTP_SETCFGFAIL = -104,
    QL_RET_ERR_RIL_FTP_RENAMEFAIL = -105,
    QL_RET_ERR_RIL_FTP_SIZEFAIL = -106,    
    QL_RET_ERR_RIL_FTP_DELETEFAIL = -107,
    QL_RET_ERR_RIL_FTP_MKDIRFAIL = -108,



	/* SMS -201 ~ -214 ,Not used*/ 
	QL_RET_ERR_SMS_NOT_INITIAL = -201,
	Ql_RET_ERR_SMS_NOT_READY = -202,
	QL_RET_ERR_SMS_INVALID_PARAM = -203,
	QL_RET_ERR_SMS_OUT_OF_MEMORY = -204,
	QL_RET_ERR_SMS_INCORRECT_DATA_LENGTH = -205,
	QL_RET_ERR_SMS_PDU_SYNTEX_ERROR = -206,
	QL_RET_ERR_SMS_INVALID_MEM_INDEX = -207,
	QL_RET_ERR_SMS_CMD_CONFLICT = -208,
	QL_RET_ERR_SMS_MSG_EMPTY = -209,
	QL_RET_ERR_SMS_INVALID_NUMBER_STRING = -210,
	QL_RET_ERR_SMS_INVALID_TEXT_CONTENT = -211,
	QL_RET_ERR_SMS_NOT_SUPPORTED = -212,
	QL_RET_ERR_SMS_INCORRECT_FORMAT = -213,
	QL_RET_ERR_SMS_STORAGE_FULL = -214,

};

#endif // End-of QL_ERROR_H 

