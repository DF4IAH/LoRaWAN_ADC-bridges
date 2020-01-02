/*
 * bus_i2c.c
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_i2c.h"
#include "stm32l4xx_it.h"

#include "cmsis_os.h"
#include "FreeRTOS.h"

#include "i2c.h"
#include "bus_i2c.h"
#include "device_portexp.h"


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t   globalEventGroupHandle;
extern osSemaphoreId        i2c1_BSemHandle;
extern I2C_HandleTypeDef    hi2c1;


#ifdef MAIN_INIT_MX_I2C1
static uint8_t              s_i2cx_UseCtr                     = 1U;
#else
static uint8_t              s_i2cx_UseCtr                     = 0U;
#endif

volatile uint8_t            i2c1TxBuffer[I2C_TXBUFSIZE];
volatile uint8_t            i2c1RxBuffer[I2C_RXBUFSIZE];


void i2cBusAddrScan(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle) {
  /* DEBUG I2C1 Bus */
  const char msgStart[] = "\r\n\r\nI2C1 bus: scanning ...\r\n";
  const char msgEnd[]   = "... done.\r\n\r\n> ";
  (void) msgStart;
  (void) msgEnd;

  osSemaphoreWait(semaphoreHandle, osWaitForever);

  //printf(msgStart);

  i2c1TxBuffer[0] = 0x00;
  for (uint8_t addr = 0x01U; addr <= 0x7FU; addr++) {
    __DSB();
    __ISB();

    if (HAL_OK != HAL_I2C_Master_Sequential_Transmit_DMA(dev, ((uint16_t)addr << 1U), (uint8_t*)i2c1TxBuffer, 0U, I2C_FIRST_AND_LAST_FRAME)) {
      /* Error_Handler() function is called when error occurs. */
      Error_Handler();
    }
    while (HAL_I2C_GetState(dev) != HAL_I2C_STATE_READY) {
      taskYIELD();
    }

    __DSB();
    __ISB();

    #if 0
    if (HAL_I2C_GetError(dev) != HAL_I2C_ERROR_AF) {
      char dbgBuf[64];
      snprintf(dbgBuf, sizeof(dbgBuf) - 1, "GOOD:  Addr=0x%02X  got response\r\n", addr);
      //printf(dbgBuf);
      osDelay(25UL);
    }
    #endif
  }
  //printf(msgEnd);

  osSemaphoreRelease(semaphoreHandle);
}

uint32_t i2cSequenceWriteLong(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle, uint8_t addr, uint8_t i2cReg, uint16_t count, const uint8_t i2cWriteAryLong[]) {
  if (count && ((count - 1) >= I2C_TXBUFSIZE)) {
    return HAL_I2C_ERROR_SIZE;
  }

  /* Wait for the I2Cx bus to be free */
  osSemaphoreWait(semaphoreHandle, osWaitForever);

  i2c1TxBuffer[0] = i2cReg;

  for (uint8_t idx = 0; idx < count; idx++) {
    i2c1TxBuffer[idx + 1] = i2cWriteAryLong[idx];
  }

  __DSB();
  __ISB();

  if (HAL_OK != HAL_I2C_Master_Sequential_Transmit_DMA(dev, ((uint16_t)addr << 1U), (uint8_t*) i2c1TxBuffer, min((1U + count), I2C_TXBUFSIZE), I2C_FIRST_AND_LAST_FRAME)) {
    /* Error_Handler() function is called when error occurs. */
    Error_Handler();
  }
  while (HAL_I2C_GetState(dev) != HAL_I2C_STATE_READY) {
    taskYIELD();
  }
  uint32_t i2cErr = HAL_I2C_GetError(dev);

  __DSB();
  __ISB();

  /* Return mutex */
  osSemaphoreRelease(semaphoreHandle);

  if (i2cErr == HAL_I2C_ERROR_AF) {
    /* Chip not responding */
    //const char errMsg[] = "i2cSequenceWriteLong: ERROR chip does not respond\r\n";
    //(void) errMsg;
    //printf(errMsg);
    return HAL_I2C_ERROR_AF;
  }

  return HAL_I2C_ERROR_NONE;
}

uint32_t i2cSequenceRead(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle, uint8_t addr, uint8_t i2cRegLen, uint8_t i2cReg[], uint16_t readLen)
{
  for (uint8_t regIdx = 0; regIdx < i2cRegLen; regIdx++) {
    i2c1TxBuffer[regIdx] = i2cReg[regIdx];
  }

  /* Wait for the I2Cx bus to be free */
  osSemaphoreWait(semaphoreHandle, osWaitForever);

  __DSB();
  __ISB();

  if (HAL_I2C_Master_Seq_Transmit_DMA(dev, (uint16_t) (addr << 1U), (uint8_t*) i2c1TxBuffer, min(i2cRegLen, I2C_TXBUFSIZE), I2C_LAST_FRAME_NO_STOP) != HAL_OK) {
    /* Error_Handler() function is called when error occurs. */
    Error_Handler();
  }
  while (HAL_I2C_GetState(dev) != HAL_I2C_STATE_READY) {
    taskYIELD();
  }
  uint32_t i2cErr = HAL_I2C_GetError(dev);

  __DSB();
  __ISB();

  if (i2cErr == HAL_I2C_ERROR_AF) {
    /* Return mutex */
    osSemaphoreRelease(semaphoreHandle);

    /* Chip not responding */
    //const char errMsg[] = "i2cSequenceRead: ERROR chip does not respond\r\n";
    //(void) errMsg;
    //printf(errMsg);
    return HAL_I2C_ERROR_AF;
  }

  memset((uint8_t*) i2c1RxBuffer, 0, sizeof(i2c1RxBuffer));

  __DSB();
  __ISB();

  if (HAL_I2C_Master_Sequential_Receive_DMA(dev, (uint16_t) (addr << 1U), (uint8_t*) i2c1RxBuffer, min(readLen, I2C_RXBUFSIZE), I2C_LAST_FRAME) != HAL_OK) {
    Error_Handler();
  }
  while (HAL_I2C_GetState(dev) != HAL_I2C_STATE_READY) {
    taskYIELD();
  }

  __DSB();
  __ISB();

  /* Return mutex */
  osSemaphoreRelease(semaphoreHandle);

  return HAL_I2C_ERROR_NONE;
}


static uint8_t i2cx_getDevIdx(I2C_HandleTypeDef* dev)
{
  if (&hi2c1 == dev) {
    return 0U;

#if 0
  } else if (&hi2c2 == dev) {
    return 1U;

  } else if (&hi2c3 == dev) {
    return 2U;

  } else if (&hi2c4 == dev) {
    return 3U;
#endif
  }

  Error_Handler();
  return 0U;
}


void i2cx_Init(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle)
{
  const uint8_t devIdx = i2cx_getDevIdx(dev);

  osSemaphoreWait(semaphoreHandle, osWaitForever);

  if (!s_i2cx_UseCtr++) {
    switch (devIdx) {
    case 0:
    {
      #if 0
      __HAL_RCC_GPIOG_CLK_ENABLE();                                                                   // I2C1: SCL, SDA
      #endif
      MX_I2C1_Init();
    }
      break;

#if 0
    case 1:
      #if 0
      __HAL_RCC_GPIOF_CLK_ENABLE();                                                                   // I2C2: SCL, SDA
      #endif
      MX_I2C2_Init();
      /* Do not turn off clock of GPIOx SCL */
      break;

    case 2:
      #if 0
      __HAL_RCC_GPIOG_CLK_ENABLE();                                                                   // I2C3: SCL, SDA
      #endif
      MX_I2C3_Init();
      /* Do not turn off clock of GPIOx SCL */
      break;

    case 3:
      #if 0
      __HAL_RCC_GPIOD_CLK_ENABLE();                                                                   // I2C4: SDA
      __HAL_RCC_GPIOF_CLK_ENABLE();                                                                   // I2C4: SCL
      #endif
      MX_I2C4_Init();
      #if 0
      __HAL_RCC_GPIOD_CLK_DISABLE();                                                                  // I2C4: SDA
      #endif
      /* Do not turn off clock of GPIOx SCL */
      break;
#endif

    default: { }
    }
  }

  osSemaphoreRelease(semaphoreHandle);

  /* Set port expander defaults for device A and B */
  portexp_Init();

  /* I2C ready */
  xEventGroupSetBits(globalEventGroupHandle, Global_EGW__I2C_RDY);
}

void i2cx_DeInit(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle)
{
  //const uint8_t devIdx = i2cx_getDevIdx(dev);

  osSemaphoreWait(semaphoreHandle, osWaitForever);

  if (!--s_i2cx_UseCtr) {
    HAL_I2C_MspDeInit(dev);

  } else if (s_i2cx_UseCtr == 255U) {
    /* Underflow */
    s_i2cx_UseCtr = 0U;
  }

  osSemaphoreRelease(semaphoreHandle);
}
