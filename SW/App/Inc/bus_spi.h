/*
 * bus_spi.h
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */

#ifndef BUS_SPI_H_
#define BUS_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "stm32l4xx_hal.h"
#include "main.h"

//#include "LoRaWAN.h"


#define SPI3_BUFFERSIZE 256

#define SPI_WR_FLAG   (1 << 7)
#define SPI_RD_FLAG   (0 << 7)


typedef enum EG_SPI3_ENUM {

  EG_SPI3_BNO__BUS_DONE                                       = 0x0001U,
  EG_SPI3_SX__BUS_DONE                                        = 0x0002U,

  EG_SPI3__BUS_FREE                                           = 0x0010U,
  EG_SPI3__BUS_ERROR                                          = 0x0080U,

} EG_SPI3_t;


typedef enum SPI3_CHIPS_ENUM {

  SPI3_BNO085                                                 = 1,
  SPI3_SX1261,

} SPI3_CHIPS_t;


extern void _Error_Handler(char *, int);


void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);

void     spiSxWaitUntilReady(void);
uint32_t spiSxWaitUntilDio1(uint32_t stopTimeAbs);

uint8_t spiProcessSpi3ReturnWait(void);
uint8_t spiProcessSpi3MsgLocked(SPI3_CHIPS_t chip, uint8_t msgLen, uint8_t waitComplete);
uint8_t spiProcessSpi3MsgTemplateLocked(SPI3_CHIPS_t chip, uint16_t templateLen, const uint8_t* templateBuf, uint8_t waitComplete);
uint8_t spiProcessSpi3MsgTemplate(SPI3_CHIPS_t chip, uint16_t templateLen, const uint8_t* templateBuf);

void spix_Init(SPI_HandleTypeDef* dev, osSemaphoreId semaphoreHandle);
void spix_DeInit(SPI_HandleTypeDef* dev, osSemaphoreId semaphoreHandle);

#ifdef __cplusplus
}
#endif

#endif /* BUS_SPI_H_ */
