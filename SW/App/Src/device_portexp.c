/*
 * device_portexp.c
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include "stm32l4xx_hal.h"

#include "cmsis_os.h"
#include "FreeRTOS.h"

#include "i2c.h"
#include "device_sx1261.h"
#include "bus_i2c.h"

#include "device_portexp.h"


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t           extiEventGroupHandle;
extern osSemaphoreId                i2c1_BSemHandle;

extern I2C_HandleTypeDef            hi2c1;
extern volatile uint8_t             i2c1RxBuffer[I2C_RXBUFSIZE];


void portexp_Init(void)
{
  /* Powering up I2C1 still needs some time here */
  osDelay(10UL);

  /* Port Expander A                  P0     P1    */
  /* Pullup / Pulldown selection */
  const uint8_t bufAPuPdSelect[2] = { 0x00U, 0x00U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x48U, sizeof(bufAPuPdSelect), bufAPuPdSelect);

  /* Pullup / Pulldown enable */
  const uint8_t bufAPuPdEnable[2] = { 0x00U, 0x00U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x46U, sizeof(bufAPuPdEnable), bufAPuPdEnable);

  /* Out port Set/Reset state 1=SET, 0=RESET */
  const uint8_t bufAOut[2]        = { 0x12U, 0x02U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x02U, sizeof(bufAOut), bufAOut);

  const uint8_t bufAType[1]       = { 0x00U };                                                  // PushPull for P1 and P0
  const uint8_t bufADir[2]        = {(uint8_t)~0x07U, (uint8_t)~0x03U };
  /* PushPull / OpenDrain port configuration 1=OD, 0=PP */
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x4fU, sizeof(bufAType), bufAType);
  /* Output port enable / tri-state configuration 1=input(TS), 0=enable */
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x06U, sizeof(bufADir),  bufADir);

  /* Interrupt enable mask 1=disabled, 0=enabled */
  const uint8_t bufAIE[2]         = {(uint8_t)~0x00U, (uint8_t)~0x00U };                        // IRQ enabled: none
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_A_ADDR, 0x4aU, sizeof(bufAIE), bufAIE);


  /* Port Expander B                  P0     P1    */
  /* Pullup / Pulldown selection */
  const uint8_t bufBPuPdSelect[2] = { 0x00U, 0x00U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x48U, sizeof(bufBPuPdSelect), bufBPuPdSelect);

  /* Pullup / Pulldown enable */
  const uint8_t bufBPuPdEnable[2] = { 0x00U, 0x00U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x46U, sizeof(bufBPuPdEnable), bufBPuPdEnable);

  /* Out port Set/Reset state 1=SET, 0=RESET */
  const uint8_t bufBOut[2]        = { 0x00U, 0x00U };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x02U, sizeof(bufBOut), bufBOut);

  const uint8_t bufBType[1]       = { 0x00U };                                                  // PushPull for P1 and P0
  const uint8_t bufBDir[2]        = {(uint8_t)~0x46U, (uint8_t)~0x50U };
  /* PushPull / OpenDrain port configuration 1=OD, 0=PP */
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x4fU, sizeof(bufBType), bufBType);
  /* Output port enable / tri-state configuration 1=input(TS), 0=enable */
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x06U, sizeof(bufBDir), bufBDir);

  /* Interrupt enable mask 1=disabled, 0=enabled */
  const uint8_t bufBIE[2]         = {(uint8_t)~0x00U, (uint8_t)~0x00U };                        // IRQ enabled: none
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_EXP_B_ADDR, 0x4aU, sizeof(bufBIE), bufBIE);


  /* Keeping SX1261 in reset state */
  sxReset(true);
}

void portexp_setDirs(uint8_t isPortExp_B, uint16_t dirs)
{
  uint8_t i2cReg = 0x06U;
  const uint8_t device = isPortExp_B == 1U ?  I2C1_PORT_EXP_B_ADDR : I2C1_PORT_EXP_A_ADDR;
  uint8_t buf[2] = { 0U };

  /* Sanity check */
  if (isPortExp_B >= 2U) {
    /* Error condition */
    return;
  }

  /* 1: output; 0: input */
  dirs ^= 0xffffU;

  /* Write direction */
  buf[0] = (uint8_t) ( dirs        & 0xffUL);
  buf[1] = (uint8_t) ((dirs >> 8U) & 0xffUL);
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, device, i2cReg, sizeof(buf), buf);
}

void portexp_setPullMode(uint8_t isPortExp_B, uint8_t bit, PullMode_t pullMode)
{
  const uint8_t device = isPortExp_B == 1U ?  I2C1_PORT_EXP_B_ADDR : I2C1_PORT_EXP_A_ADDR;
  uint8_t buf[2] = { 0U };

  /* Sanity check */
  if (isPortExp_B >= 2U) {
    /* Error condition */
    return;
  }

  /* Read pull mode enable registers */
  uint8_t enableRegs = 0x46U;
  uint16_t pupdEnable;
  i2cSequenceRead(&hi2c1, i2c1_BSemHandle, device, sizeof(enableRegs), &enableRegs, sizeof(pupdEnable));
  pupdEnable  = i2c1RxBuffer[0];                                                                     // Port 0
  pupdEnable |= (uint16_t)i2c1RxBuffer[1] << 8;                                                      // Port 1

  /* Read pull selection registers */
  uint8_t variantRegs = 0x48U;
  uint16_t pupdVariant;
  i2cSequenceRead(&hi2c1, i2c1_BSemHandle, device, sizeof(variantRegs), &variantRegs, sizeof(pupdVariant));
  pupdVariant  = i2c1RxBuffer[0];                                                                   // Port 0
  pupdVariant |= (uint16_t)i2c1RxBuffer[1] << 8;                                                    // Port 1

  switch (pullMode)
  {
  case PupdNone:
  default:
    pupdEnable  &= ~(1UL << bit);
    break;

  case PupdPU:
    pupdEnable  |=   1UL << bit;
    pupdVariant |=   1UL << bit;
    break;

  case PupdPD:
    pupdEnable  |=   1UL << bit;
    pupdVariant &= ~(1UL << bit);
    break;
  }

  /* Write pull selection registers */
  buf[0] = (uint8_t) ( pupdVariant        & 0xffUL);
  buf[1] = (uint8_t) ((pupdVariant >> 8U) & 0xffUL);
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, device, variantRegs, sizeof(buf), buf);

  /* Write pull enable registers */
  buf[0] = (uint8_t) ( pupdEnable         & 0xffUL);
  buf[1] = (uint8_t) ((pupdEnable >> 8U)  & 0xffUL);
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, device, enableRegs,  sizeof(buf), buf);
}

void portexp_setIrq(uint8_t isPortExp_B, uint8_t bit, uint8_t irqEnable)
{
  const uint8_t   device  = isPortExp_B == 1U ?  I2C1_PORT_EXP_B_ADDR : I2C1_PORT_EXP_A_ADDR;
  static uint16_t maskA   = 0xffffU;
  static uint16_t maskB   = 0xffffU;
  uint16_t*       mP      = isPortExp_B == 1U ?  &maskB : &maskA;

  /* Sanity check */
  if (isPortExp_B >= 2U) {
    /* Error condition */
    return;
  }

  if (irqEnable) {
    /* Enable IRQ - disable mask */
    *mP &= (uint16_t) ~(1UL << bit);

  } else {
    /* Disable IRQ - enable mask */
    *mP |= (uint16_t)  (1UL << bit);
  }

  /* Interrupt enable mask 1=disabled, 0=enabled */
  const uint8_t bufIE[2] = {(uint8_t)GET_BYTE_OF_WORD(*mP, 0), (uint8_t)GET_BYTE_OF_WORD(*mP, 1) };
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, device, 0x4aU, sizeof(bufIE), bufIE);
}


uint8_t portexp_read(uint8_t isPortExp_B, uint8_t bit)
{
  const uint8_t device  = isPortExp_B == 1U ?  I2C1_PORT_EXP_B_ADDR : I2C1_PORT_EXP_A_ADDR;
  uint8_t i2cReg[1]     = { 0U };
  uint16_t data         = 0U;

  /* Sanity check */
  if ((isPortExp_B >= 2U) || (bit >= 16U)) {
    /* Error condition */
    return 0U;
  }

  i2cReg[0] = 0x00U;

  /* Read from port0 and port1 */
  i2cSequenceRead(&hi2c1, i2c1_BSemHandle, device, sizeof(i2cReg), i2cReg, sizeof(data));
  data  = i2c1RxBuffer[0];                                                                            // Port 0
  data |= (uint16_t)i2c1RxBuffer[1] << 8;                                                             // Port 1

  return (data & (1UL << bit)) ?  1U : 0U;
}

void portexp_write(uint8_t isPortExp_B, uint8_t bit, uint8_t doSet)
{
  uint8_t i2cReg = 0x00U;
  const uint8_t device = isPortExp_B == 1U ?  I2C1_PORT_EXP_B_ADDR : I2C1_PORT_EXP_A_ADDR;
  uint16_t data     = 0U;
  uint8_t buf[2] = { 0U };

  /* Sanity check */
  if ((isPortExp_B >= 2U) || (bit >= 16U)) {
    /* Error condition */
    return;
  }

  /* Read from port0 and port1 */
  i2cSequenceRead(&hi2c1, i2c1_BSemHandle, device, sizeof(i2cReg), &i2cReg, sizeof(data));
  data  = i2c1RxBuffer[0];                                                                            // Port 0
  data |= (uint16_t)i2c1RxBuffer[1] << 8;                                                             // Port 1

  if (doSet) {
    data |= 1UL << bit;
  } else {
    data &= ~(1UL << bit);
  }

  /* Write back modified data */
  i2cReg = 0x02U;
  buf[0] = (uint8_t) ( data        & 0xffUL);
  buf[1] = (uint8_t) ((data >> 8U) & 0xffUL);
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, device, i2cReg, sizeof(buf), buf);
}


/* EXTI Callback */
void portexp_EXTI_Callback(uint8_t isPortExp_B)
{
  BaseType_t taskWoken = 0;

  switch (isPortExp_B) {
  case 0:
  {
    /* Port A signal */
    xEventGroupSetBitsFromISR(extiEventGroupHandle, EXTI_PORTEXP_A, &taskWoken);
  }
  break;

  case 1:
  {
    /* Port B signal */
    xEventGroupSetBitsFromISR(extiEventGroupHandle, EXTI_PORTEXP_B, &taskWoken);
  }
  break;

  default: { }
  }
}
