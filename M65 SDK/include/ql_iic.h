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
 *   ql_ii.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   IIC interface APIs defines.
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
#ifndef __QL_IIC_H__
#define __QL_IIC_H__


/*****************************************************************
* Function:     Ql_IIC_Init 
* 
* Description:
*               This function initialize the configurations for an IIC channel.
*               including the specified pins for IIC, IIC type, and IIC channel No.
*
* Parameters:
*               chnnlNo:
*                   IIC channel No, the range is 0~254
*               pinSCL:
*                   IIC SCL pin.
*               pinSDA:
*                   IIC SDA pin.
*               IICtype:
*                   IIC type, '0' means simulate IIC , '1' means hardware IIC(not supports).
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the channel No is be used.
*               QL_RET_ERR_FULL, IIC bus is full.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*               QL_RET_ERR_USED,the pin is used already. For example this pin has been using as EINT or gpio.
*****************************************************************/
s32 Ql_IIC_Init(u32 chnnlNo,Enum_PinName pinSCL,Enum_PinName pinSDA,bool IICtype);


/*****************************************************************
* Function:     Ql_IIC_Config 
* 
* Description:
*               This function configuration the IIC interface.
*         
* Parameters:
*               chnnlNo:
*                   IIC channel No, the No is specified by Ql_IIC_Init function
*               isHost:
*                   must be ture, just support host mode.
*               slaveAddr:
*                   slave address.An 8-bit slave address with read-write bits
*		   		IicSpeed:
*					surpport 100khz or 400khz.
*					It can only be set to 100 or 400.
*					just for hardware IIC ,and the parameter can be ingore if you use simulate IIC
*
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the channel No is be used.
*               QL_RET_ERR_FULL, IIC bus is full.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*               QL_RET_ERR_USED,the pin is used already. For example this pin has been using as EINT or gpio.
*               QL_RET_ERR_CHANNEL_NOT_FOUND,can't found the IIC channel, make sure it is initialized already.
*****************************************************************/
s32  Ql_IIC_Config(u32 chnnlNo, bool isHost, u8 slaveAddr, u32 IicSpeed);


/*****************************************************************
* Function:     Ql_IIC_Write 
* 
* Description:
*               This function  write bytes to specified slave through IIC interface.
*               
* Parameters:
*               chnnlNo:
*                   IIC channel No, the No is specified by Ql_IIC_Init function.
*               slaveAddr:
*                   slave address.An 8-bit slave address with read-write bits
*               pData:
*                   Setting value to slave
*               len:
*                   Number of bytes to write. 
*                  
* Return:        
*               if no error return, the length of the write data.
*               QL_RET_ERR_PARAM, patameter error.
*               QL_RET_ERR_CHANNEL_NOT_FOUND, can't found the IIC channel, make sure it is initialized already.
*               QL_RET_ERR_I2CHWFAILED, Maybe the hardware have something wrong.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*****************************************************************/
s32 Ql_IIC_Write(u32 chnnlNo,u8 slaveAddr,u8 *pData,u32 len);


/*****************************************************************
* Function:     Ql_IIC_Read 
* 
* Description:
*               This function read bytes from specified slave through IIC interface.
*               
* Parameters:
*               chnnlNo:
*                   IIC channel No, the No is specified by Ql_IIC_Init function.
*               slaveAddr:
*                   slave address.An 8-bit slave address with read-write bits
*               pBuffer:
*                   read buffer of reading the specified register from slave.
*               len:
*                   Number of bytes to read.
*                 
* Return:        
*               if no error, return the length of the read data.
*               QL_RET_ERR_PARAM, patameter error.
*               QL_RET_ERR_CHANNEL_NOT_FOUND, can't found the IIC channel, make sure it is initialized already.
*               QL_RET_ERR_I2CHWFAILED, Maybe the hardware have something wrong.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*****************************************************************/
s32 Ql_IIC_Read(u32 chnnlNo,u8 slaveAddr,u8 *pBuffer,u32 len);


/*****************************************************************
* Function:     Ql_IIC_Write_Read 
* 
* Description:
*               This function read data form the specified register(or address) of slave.
*               
* Parameters:
*               chnnlNo:
*                   IIC channel No, the No is specified by Ql_IIC_Init function.
*               slaveAddr:
*                   slave address.An 8-bit slave address with read-write bits
*               pData:
*                   Setting value of the specified register of slave.
*               wrtLen:
*                   Number of bytes to write.
*               pBuffer:
*                   read buffer of reading the specified register from slave.
*               rdLen:
*                   Number of bytes to read.
*                  
* Return:        
*               if no error return the length of the read data.
*               QL_RET_ERR_PARAM, patameter error.
*               QL_RET_ERR_CHANNEL_NOT_FOUND, can't found the IIC channel, make sure it is initialized already.
*               QL_RET_ERR_I2CHWFAILED, Maybe the hardware have something wrong.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*****************************************************************/
s32 Ql_IIC_Write_Read(u32 chnnlNo,u8 slaveAddr,u8 * pData,u32 wrtLen,u8 * pBuffer,u32 rdLen);


/*****************************************************************
* Function:     Ql_IIC_Uninit 
* 
* Description:
*               This function releases the pins.
*               
* Parameters:
*               chnnlNo:
*                   IIC channel No, the No is specified by Ql_IIC_Init function.

* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the channel No is be used.
*               QL_RET_ERR_CHANNEL_NOT_FOUND, can't found the IIC channel, make sure it is initialized already.
*               QL_RET_ERR_INVALID_PARAM, the input pin is invalid.
*****************************************************************/
s32 Ql_IIC_Uninit(u32 chnnlNo);


#endif  //__QL_IIC_H__
