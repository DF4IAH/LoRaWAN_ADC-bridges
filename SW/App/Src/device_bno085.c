/*
 * device_bno085.c
 *
 *  Created on: 10.07.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"
#include "stm32l4xx_it.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"

#include "main.h"
#include "bus_spi.h"
#include "device_portexp.h"
#include "device_bno085.h"


#ifdef BNO085_EXTRA


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t           extiEventGroupHandle;

extern volatile uint8_t             spi3TxBuffer[SPI3_BUFFERSIZE];
extern volatile uint8_t             spi3RxBuffer[SPI3_BUFFERSIZE];


uint32_t bno085WaitUntilInt(uint32_t stopTimeAbs)
{
  return portexp_WaitUntilInt(PORTEXP_A, PORTEXP_A_BIT_IN_BNO_NINT, GPIO_PIN_RESET, stopTimeAbs);
}

void bno085WaitUntilReady(void)
{
  const uint32_t waitMaxUntil = xTaskGetTickCount() + 50UL;

  bno085WaitUntilInt(waitMaxUntil);
}


void bno085Init(void)
{
  /* Set signals first */
  {
    /* Activate reset line */
    portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_BNO_NRST, GPIO_PIN_RESET);

    osDelay(2UL);

    /* Set BOOTN high for normal operation */
    portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_BNO_BOOTN, GPIO_PIN_SET);

    /* Set PS0 (with PS1 fixed at GPIO_PIN_SET) to use the SPI interface */
    //HAL_GPIO_WritePin(BOARD_WKUP_GPIO_Port, BOARD_WKUP_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIO_BNO_PS0WAKE_GPIO_Port, GPIO_BNO_PS0WAKE_Pin, GPIO_PIN_SET);

    osDelay(2UL);

    /* Release reset line */
    portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_BNO_NRST, GPIO_PIN_SET);

    /* Give time for waking up */
    osDelay(100UL);
  }

  /* Send init SPI commands */
  {
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_BNO_NINT, true);

    /* Activate PS0 wake for SPI communication */
    //HAL_GPIO_WritePin(BOARD_WKUP_GPIO_Port, BOARD_WKUP_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIO_BNO_PS0WAKE_GPIO_Port, GPIO_BNO_PS0WAKE_Pin, GPIO_PIN_RESET);

    /* Wait until the interrupt line is pulled */
    bno085WaitUntilReady();

    /* Read header */
    uint8_t bufBnoHdr[4] = { 0xa5, 0x5a, 0xaa, 0x55 };
    spiProcessSpi3MsgTemplate(SPI3_BNO085, sizeof(bufBnoHdr), bufBnoHdr);

    HAL_GPIO_WritePin(GPIO_BNO_PS0WAKE_GPIO_Port, GPIO_BNO_PS0WAKE_Pin, GPIO_PIN_SET);
    __NOP();
  }
}


#endif
