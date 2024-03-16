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
 *   ql_adc.h 
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
#ifndef __QL_ADC_H__
#define __QL_ADC_H__

typedef enum
{
    PIN_ADC0,
    PIN_ADC_MAX = 2
}Enum_ADCPin;

typedef enum
{
    ADC_PERIOD_122US = 0,
    ADC_PERIOD_1MS,
    ADC_PERIOD_10MS,
    ADC_PERIOD_100MS,
    ADC_PERIOD_250MS,
    ADC_PERIOD_500MS,
    ADC_PERIOD_1S,
    ADC_PERIOD_2S,
} QL_ADC_Period;

/*****************************************************************
* Function:     Ql_ADC_Open 
* 
* Description:
*               Used to open an ADC channel.
*
* Parameters:
*               adcPin:
*                   adc pin name, one value of Enum_ADCPin
*               adcPeriod:
*                   adc period,one value of QL_ADC_Period
*                 
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the input pin is invalid.
*                QL_RET_ERR_ERROR,open failed.
*****************************************************************/
s32 Ql_ADC_Open(Enum_ADCPin adcPin,QL_ADC_Period adcPeriod);
/*****************************************************************
* Function:     Ql_ADC_Read 
* 
* Description:
*               Read the value of the ADC from the current pin.
*
* Parameters:
*               adcPin:
*                   adc pin name, one value of Enum_ADCPin
*               adcValue(out):
*                   the value of the ADC
*                 
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the input pin is invalid.
*               QL_RET_ERR_NOT_INIT,can't operate,Maybe the ADC not open. 
*****************************************************************/
s32 Ql_ADC_Read(Enum_ADCPin adcPin,u16 *adcValue);
/*****************************************************************
* Function:     Ql_ADC_Close 
* 
* Description:
*               Close the ADC channel corresponding to the current pin.
*
* Parameters:
*               adcPin:
*                   adc pin name, one value of Enum_ADCPin
*                 
* Return:        
*               QL_RET_OK, this function succeeds.
*               QL_RET_ERR_PARAM, the input pin is invalid.
*               QL_RET_ERR_ERROR,closed failed.
*****************************************************************/
s32 Ql_ADC_Close(Enum_ADCPin adcPin);

#endif  //__QL_ADC_H__
