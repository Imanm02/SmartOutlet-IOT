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
 *   ql_secure_data.h 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   SECURE_DATA APIs definition.
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
#ifndef __QL_SECURE_DATA_H__
#define __QL_SECURE_DATA_H__

/*****************************************************************
* Function:     Ql_SecureData_Store 
* 
* Description:
*              This function can be used to store some critical user data 
*              to prevent them from losing.
*
*              Note:
*              1.
*              OpenCPU has designed 16 blocks of system storage space to 
*              backup critical user data. Developer may specify the first
*              parameter index [1-16] to specify different storage block. 
*              Among the storage blocks, 1~8 blocks can store 52 bytes for 
*              each block, 9~12 blocks can store 100 bytes for each block, 
*              13~14 blocks can store 500 bytes,and the 15~16 blocks 
*              can store 1000 bytes.
*
*              2.
*              User should not call this API function frequently, which is not
*              good for life cycle of flash.
*
* Parameters:
*              index:
*               [in]  the index of the secure data block. The range is: 1~16.
*              
*               pData: 
*                   [in] The data to be backed up. In 1~8 groups, every group can 
*                   save 52 bytes at most. In 9~12 groups, every group can save 
*                   100 bytes at most. If index is 13~14, the user data can save 500 bytes at most.
*                   If index is 15~16, the user data can save 1000 bytes at most.
*
*               len:
*                   [in] The length of the user data. When the index is (1~8), 
*                   then len<=52. When the index is (9~12), then len<=100. 
*                   When the index is (13~14), then len<=500.When the index is (15~16), 
*                   then len<=1000.
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, invalid paramter.
*               QL_RET_ERR_GET_MEM, the heap memory is no enough.
*               ......
*****************************************************************/
s32 Ql_SecureData_Store(u8 index, u8 *pdata, u32 len);


/*****************************************************************
* Function:     Ql_SecureData_Read
* 
* Description:
*              This functin reads secure data which is previously 
*              stored by Ql_SecureData_Store.
* Parameters:
*               index:
*                   [in] The index of the secure data block. The range is: 1~16.
*
*               len:
*                   [in] The length of the user data. When the index is (1~8), 
*                   then len<=52. When the index is (9~12), then len<=100. 
*                   When the index is (13~14), then len<=500.When the index is (15~16), 
*                   then len<=1000.
* Return:        
*               If this function succeeds, the real read length is returned.
*               QL_RET_ERR_PARAM, invalid paramter.
*               QL_RET_ERR_GET_MEM, the heap memory is no enough.
*               Ql_RET_ERR_UNKOWN, unknown error.
*****************************************************************/
s32 Ql_SecureData_Read(u8 index, u8* pBuffer, u32 len);


#endif  //__QL_SECURE_DATA_H__

