/*
 * bus_i2c.h
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */

#ifndef BUS_I2C_H_
#define BUS_I2C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_i2c.h"
#include "stm32l4xx_it.h"

#include "cmsis_os.h"
#include "FreeRTOS.h"


/* Module */

#define I2C_TXBUFSIZE                                         32U
#define I2C_RXBUFSIZE                                         32U

#define I2C1_PORT_EXP_A_ADDR                                  0x20                                  // Port Expander A: 9D chip and SX1261
#define I2C1_PORT_EXP_B_ADDR                                  0x21                                  // Port Expander B: WLAN and power switches
#define I2C1_PORT_ADC                                         0x2a
#define I2C1_PORT_FRAM                                        0x50
#define I2C1_PORT_9D                                          0x76


void i2cBusAddrScan(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle);
uint32_t i2cSequenceWriteLong(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle, uint8_t addr, uint8_t i2cReg, uint16_t count, const uint8_t i2cWriteAryLong[]);
uint32_t i2cSequenceRead(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle, uint8_t addr, uint8_t i2cRegLen, uint8_t i2cReg[], uint16_t readlen);

void i2cx_Init(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle);
void i2cx_DeInit(I2C_HandleTypeDef* dev, osSemaphoreId semaphoreHandle);

#ifdef __cplusplus
}
#endif

#endif /* BUS_I2C_H_ */
