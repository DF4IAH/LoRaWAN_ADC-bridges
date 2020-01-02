/*
 * bus_spi.c
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include "bus_spi.h"

#include <string.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_it.h"

#include "device_portexp.h"

#include "spi.h"


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t           globalEventGroupHandle;
extern EventGroupHandle_t           spiEventGroupHandle;
extern EventGroupHandle_t           extiEventGroupHandle;
extern osSemaphoreId                spi3_BSemHandle;


static uint8_t                      s_spix_UseCtr                 = 0U;

/* Buffer used for transmission */
volatile uint8_t                    spi3TxBuffer[SPI3_BUFFERSIZE] = { 0U };

/* Buffer used for reception */
volatile uint8_t                    spi3RxBuffer[SPI3_BUFFERSIZE] = { 0U };

/* Flags for the select lines */
volatile uint8_t                    g_spi3SelBno                  = 0U;
volatile uint8_t                    g_spi3SelSx                   = 0U;

extern SPI_HandleTypeDef            hspi3;
extern DMA_HandleTypeDef            hdma_spi3_tx;
extern DMA_HandleTypeDef            hdma_spi3_rx;


/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
  BaseType_t taskWoken = 0L;

  if (&hspi3 == hspi) {
    uint8_t spi3BusInUse = 0U;

    #if 0
    /* Enable clocks of GPIOs */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    #endif

    if (g_spi3SelBno) {
      xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3_BNO__BUS_DONE, &taskWoken);

    } else if (g_spi3SelSx) {
      xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3_SX__BUS_DONE, &taskWoken);

    } else {
      spi3BusInUse = 1U;
    }

    #if 0
    /* Disable clocks again */
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOE_CLK_DISABLE();
    #endif

    if (!spi3BusInUse) {
      xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3__BUS_FREE, &taskWoken);
    }
  }
}

/**
  * @brief  SPI error callbacks.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
  BaseType_t taskWoken = 0;

  if (&hspi3 == hspi) {
    if (g_spi3SelBno) {
      xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3_BNO__BUS_DONE, &taskWoken);

    } else if (g_spi3SelSx) {
      xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3_SX__BUS_DONE, &taskWoken);
    }

    xEventGroupSetBitsFromISR(spiEventGroupHandle, EG_SPI3__BUS_ERROR, &taskWoken);
  }
}


void spiSxWaitUntilReady(void)
{
  /* Wait until SX1261 is not busy any more */
  while (1) {
    if (GPIO_PIN_RESET == portexp_read(PORTEXP_A, PORTEXP_A_BIT_IN_SX_BUSY)) {
      break;
    }

    osDelay(1UL);
  }
}

uint32_t spiSxWaitUntilDio1(uint32_t stopTimeAbs)
{
  const uint32_t  tsIn  = xTaskGetTickCount();
  uint32_t        ticks = 1UL;

  /* Wait until interrupt SX DIO1: TX_DONE comes in */
  if (tsIn < stopTimeAbs) {
    ticks = (stopTimeAbs - tsIn) / portTICK_PERIOD_MS;

    EventBits_t eb = xEventGroupWaitBits(extiEventGroupHandle,
      EXTI_PORTEXP_A,
      EXTI_PORTEXP_A,
      0,
      ticks);

    /* Valid only when SX1261 did inform about TX/RX end */
    if (eb & EXTI_PORTEXP_A) {
      return xTaskGetTickCount();
    }
  }

  /* No IRQ catch */
  return 0UL;
}


const uint16_t spiWait_EGW_MaxWaitTicks = 500;
uint8_t spiProcessSpi3ReturnWait(void)
{
  EventBits_t eb = xEventGroupWaitBits(spiEventGroupHandle,
      EG_SPI3_BNO__BUS_DONE | EG_SPI3_SX__BUS_DONE | EG_SPI3__BUS_FREE | EG_SPI3__BUS_ERROR,
      0,
      pdFALSE, spiWait_EGW_MaxWaitTicks);
  if (eb & (EG_SPI3_BNO__BUS_DONE | EG_SPI3_SX__BUS_DONE | EG_SPI3__BUS_FREE | EG_SPI3__BUS_ERROR)) {
    if (g_spi3SelBno) {
      /* Deactivate the NSS/SEL pin */
      portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_SPI3_BNO_SEL, GPIO_PIN_SET);
      g_spi3SelBno = 0U;

    } else if (g_spi3SelSx) {
      /* Deactivate the NSS/SEL pin */
      portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_SPI3_SX_SEL, GPIO_PIN_SET);
      g_spi3SelSx = 0U;
    }
    return HAL_OK;
  }

  if (eb & EG_SPI3__BUS_ERROR) {
    Error_Handler();
  }
  return HAL_ERROR;
}

uint8_t spiProcessSpi3MsgLocked(SPI3_CHIPS_t chip, uint8_t msgLen, uint8_t waitComplete)
{
  uint8_t           PORTEXPx;
  uint16_t          PORTEXP_Pin;
  volatile uint8_t* PORTEXP_var   = 0UL;
  HAL_StatusTypeDef status        = HAL_OK;
  uint8_t           errCnt        = 0U;

  switch (chip) {
  case SPI3_BNO085:
    PORTEXPx     = PORTEXP_A;
    PORTEXP_Pin  = PORTEXP_A_BIT_OUT_SPI3_BNO_SEL;
    PORTEXP_var  = &g_spi3SelBno;
    break;

  case SPI3_SX1261:
    PORTEXPx     = PORTEXP_A;
    PORTEXP_Pin  = PORTEXP_A_BIT_OUT_SPI3_SX_SEL;
    PORTEXP_var  = &g_spi3SelSx;
    break;

  default:
    PORTEXPx     = 255U;
    PORTEXP_Pin  = 0U;
    PORTEXP_var  = 0UL;
  }

  /* Sanity check */
  if (PORTEXPx > PORTEXP_B) {
    return 0U;
  }

  /* Wait for free select line */
  do {
    if (GPIO_PIN_RESET != portexp_read(PORTEXPx, PORTEXP_Pin)) {
      break;
    }

    /* Wait some time for free bus */
    osDelay(1UL);
  } while (1);

  /* Set SEL flag */
  if (PORTEXP_var) {
    *PORTEXP_var = 1U;

    /* Activate low active NSS/SEL transaction */
    portexp_write(PORTEXPx, PORTEXP_Pin, GPIO_PIN_RESET);
  }

  do {
    __DSB();
    __ISB();

  status = HAL_SPI_TransmitReceive_DMA(&hspi3, (uint8_t*) spi3TxBuffer, (uint8_t *) spi3RxBuffer, msgLen);
	if (status == HAL_BUSY)
    {
      osThreadYield();

      if (++errCnt >= 100U) {
        /* Transfer error in transmission process */
        portexp_write(PORTEXPx, PORTEXP_Pin, GPIO_PIN_SET);
        Error_Handler();
      }
    }
  } while (status == HAL_BUSY);

  if ((status == HAL_OK) && waitComplete) {
    /* Wait until the data is transfered */
    status = spiProcessSpi3ReturnWait();
  }

  __DSB();
  __ISB();

  #if 0
  /* Disable GPIO clocks again */
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOD_CLK_DISABLE();
  __HAL_RCC_GPIOE_CLK_DISABLE();
  #endif

  return status;
}

uint8_t spiProcessSpi3MsgTemplateLocked(SPI3_CHIPS_t chip, uint16_t templateLen, const uint8_t* templateBuf, uint8_t waitComplete)
{
  /* Wait for SPI3 mutex */
  if (osOK != osSemaphoreWait(spi3_BSemHandle, 1000UL)) {
    return 0U;
  }

  /* Copy from Template */
  memcpy((void*) spi3TxBuffer, (void*) templateBuf, templateLen);

  /* Execute SPI1 communication and leave with locked SPI3 mutex for read purpose */
  return spiProcessSpi3MsgLocked(chip, templateLen, waitComplete);
}

uint8_t spiProcessSpi3MsgTemplate(SPI3_CHIPS_t chip, uint16_t templateLen, const uint8_t* templateBuf)
{
  const uint8_t ret = spiProcessSpi3MsgTemplateLocked(chip, templateLen, templateBuf, 1U);

  /* Release SPI3 mutex */
  osSemaphoreRelease(spi3_BSemHandle);

  return ret;
}


static uint8_t spix_getDevIdx(SPI_HandleTypeDef* dev)
{
  if (&hspi3 == dev) {
    return 0U;

  #if 0
  } else if (&hspi3 == dev) {
    return 1U;
  #endif
  }

  Error_Handler();
  return 0U;
}


void spix_Init(SPI_HandleTypeDef* dev, osSemaphoreId semaphoreHandle)
{
  const uint8_t devIdx = spix_getDevIdx(dev);

  osSemaphoreWait(semaphoreHandle, osWaitForever);

  if (!s_spix_UseCtr++) {
    switch (devIdx) {
    case 0:
      #if 0
      __HAL_RCC_GPIOA_CLK_ENABLE();                                                                   // SPI1: MCU_SPI1_SCK
      #endif
      //MX_SPI3_Init();                                                                               // Already done in main()
      break;

    #if 0
    case 1:
      __HAL_RCC_GPIOC_CLK_ENABLE();                                                                   // SPI3: MCU_SPI3_SCK, MCU_SPI3_MISO, MCU_SPI3_MOSI, MCU_OUT_AUDIO_DAC_SEL
      __HAL_RCC_GPIOD_CLK_ENABLE();                                                                   // SPI3: MCU_OUT_AUDIO_ADC_SEL
      __HAL_RCC_GPIOE_CLK_ENABLE();                                                                   // SPI3: MCU_OUT_AX_SEL, MCU_OUT_SX_SEL
      //MX_SPI3_Init();                                                                               // Already done in main()
      break;
    #endif

    default: { }
    }
  }

  osSemaphoreRelease(semaphoreHandle);

  xEventGroupSetBits(globalEventGroupHandle, Global_EGW__SPI_RDY);
}

void spix_DeInit(SPI_HandleTypeDef* dev, osSemaphoreId semaphoreHandle)
{
  //const uint8_t devIdx = spix_getDevIdx(dev);

  osSemaphoreWait(semaphoreHandle, osWaitForever);

  if (!--s_spix_UseCtr) {
    HAL_SPI_MspDeInit(dev);

  } else if (s_spix_UseCtr == 255U) {
    /* Underflow */
    s_spix_UseCtr = 0U;
  }

  osSemaphoreRelease(semaphoreHandle);
}
