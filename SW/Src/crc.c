/**
  ******************************************************************************
  * File Name          : CRC.c
  * Description        : This file provides code for the configuration
  *                      of the CRC instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "crc.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

CRC_HandleTypeDef hcrc;

/* CRC init function */
void MX_CRC_Init(void)
{

  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }

}

void HAL_CRC_MspInit(CRC_HandleTypeDef* crcHandle)
{

  if(crcHandle->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspInit 0 */

  /* USER CODE END CRC_MspInit 0 */
    /* CRC clock enable */
    __HAL_RCC_CRC_CLK_ENABLE();
  /* USER CODE BEGIN CRC_MspInit 1 */

  /* USER CODE END CRC_MspInit 1 */
  }
}

void HAL_CRC_MspDeInit(CRC_HandleTypeDef* crcHandle)
{

  if(crcHandle->Instance==CRC)
  {
  /* USER CODE BEGIN CRC_MspDeInit 0 */

  /* USER CODE END CRC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CRC_CLK_DISABLE();
  /* USER CODE BEGIN CRC_MspDeInit 1 */

  /* USER CODE END CRC_MspDeInit 1 */
  }
} 

/* USER CODE BEGIN 1 */
uint32_t crcCalc(volatile void* ptr, uint32_t len)
{
  /* Decrease to lower word boundary */
  ptr = (volatile void*) ((uint32_t)ptr & ~0x00000003UL);

  __DSB();
  __ISB();

  __HAL_RCC_CRC_CLK_ENABLE();
  const uint32_t crc = HAL_CRC_Calculate(&hcrc, (uint32_t*) ptr, len >> 2);
  __HAL_RCC_CRC_CLK_DISABLE();

  __DSB();
  __ISB();

  return crc;
}

/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
