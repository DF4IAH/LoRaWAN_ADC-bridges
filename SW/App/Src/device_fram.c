/*
 * device_fram.c
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32l4xx_hal.h"

#include "cmsis_os.h"
#include "FreeRTOS.h"

#include "crc.h"
#include "i2c.h"
#include "bus_i2c.h"

#include "device_fram.h"


/* Variables -----------------------------------------------------------------*/

extern osSemaphoreId        i2c1_BSemHandle;
extern I2C_HandleTypeDef    hi2c1;
extern volatile uint8_t     i2c1RxBuffer[I2C_RXBUFSIZE];

volatile uint8_t check[256] = { 0 };


volatile Fram1F_Template_t  g_framShadow1F = { 0 };


uint8_t fram_check1Fcrc(uint8_t isLoadIntoShadow)
{
  uint32_t crc1Fcalc;
  uint32_t crc1Ffram;

  /* Load block 1F from FRAM into shadow */
  if (isLoadIntoShadow) {
    /* Read CRC together w/ CRC secured data */
    memset((void*) &g_framShadow1F, 0, sizeof(g_framShadow1F));
    fram_readBuf(&g_framShadow1F, FRAM_MEM_1F00_BORDER, sizeof(g_framShadow1F));
  }

  /* Calculate CRC */
  crc1Fcalc = crcCalc(((void*)&g_framShadow1F) + (FRAM_MEM_1F00_CRC_BEGIN - FRAM_MEM_1F00_BORDER), 1UL + (FRAM_MEM_1F00_CRC_END - FRAM_MEM_1F00_CRC_BEGIN));
  crc1Ffram = g_framShadow1F.crc32;

  return crc1Fcalc == crc1Ffram ?  0U : 1U;
}

void fram_updateCrc(void)
{
  /* Remove masked flags */
  g_framShadow1F.loraEventGroup &= ~(Lora_EGW__TRANSPORT_READY | Lora_EGW__ADR_DOWNACTIVATE | Lora_EGW__DO_INIT | Lora_EGW__DATA_UP_DONE | Lora_EGW__DATA_UP_REQ | Lora_EGW__QUEUE_OUT | Lora_EGW__QUEUE_IN);

  /* Update shadow file crc32 */
  g_framShadow1F.crc32 = crcCalc(((void*)&g_framShadow1F) + (FRAM_MEM_1F00_CRC_BEGIN - FRAM_MEM_1F00_BORDER), 1UL + (FRAM_MEM_1F00_CRC_END - FRAM_MEM_1F00_CRC_BEGIN));

  /* Write volatile crc32 to FRAM cache */
  __DSB();
  __ISB();

  /* Write the total cache file into the FRAM */
  fram_writeBuf(&g_framShadow1F, FRAM_MEM_1F00_BORDER, sizeof(g_framShadow1F));
}


uint32_t fram_readWord(uint32_t addr)
{
  uint32_t word = 0UL;
  volatile uint8_t buf[4] = { 0 };

  fram_readBuf(&buf, addr, sizeof(uint32_t));

  for (uint8_t idx = 0U; idx < sizeof(uint32_t); idx++) {
    word >>= 8U;
    word |= ((uint32_t)buf[idx] << 24U);
  }
  return word;
}

void fram_readBuf(volatile void* bufOut, uint32_t addr, uint16_t len)
{
  /* Sanity check */
  if (!bufOut || !len || (addr >= 8192U) || (addr + len) > 8192UL) {
    return;
  }

  volatile uint8_t *bufPtr = bufOut;

  for (uint16_t idx = 0U; idx < len; idx++) {
    uint8_t tryCnt;

    for (tryCnt = 16U; tryCnt; --tryCnt) {
      const uint8_t val1 = fram_read_nonSecured(addr);
      const uint8_t val2 = fram_read_nonSecured(addr);

      /* Being paranoid about data validity */
      if (val1 != val2) {
        continue;
      }

      if (*bufPtr != val1) {
        /* Update cache */
        *bufPtr = val1;

        __DSB();
        __ISB();

      } else {
        /* Cache equals to FRAM content */
        addr++;
        bufPtr++;
        break;
      }
    }

    if (!tryCnt) {
      /* Problem to read FRAM */
    }
  }
}

uint8_t fram_read_nonSecured(uint32_t addr)
{
  uint8_t i2cReg[2] = { 0U };
  uint8_t data      = 0U;

  /* Sanity check */
  if (addr >= 8192UL) {
    /* Error condition */
    return 0U;
  }

  __DSB();
  __ISB();

  i2cReg[0] = (uint8_t) ((addr >> 8) & 0x1fUL);
  i2cReg[1] = (uint8_t) (addr & 0xffUL);

  /* Read data from FRAM */
  i2cSequenceRead(&hi2c1, i2c1_BSemHandle, I2C1_PORT_FRAM, sizeof(i2cReg), i2cReg, sizeof(data));
  data = i2c1RxBuffer[0];
  return data;
}


void fram_writeWord(uint32_t addr, uint32_t data)
{
  volatile uint8_t buf[4] = { 0 };

  for (uint8_t idx = 0U; idx < sizeof(uint32_t); idx++) {
    buf[idx] = (uint8_t) (data & 0xffUL);
    data >>= 8U;
  }

  fram_writeBuf(buf, addr, sizeof(uint32_t));
}

void fram_writeBuf(volatile void* bufIn, uint32_t addr, uint16_t len)
{
  /* Sanity check */
  if (!bufIn || !len || (addr >= 8192U) || (addr + len) > 8192UL) {
    return;
  }

  __DSB();
  __ISB();

  volatile uint8_t *bufInPtr  = (volatile uint8_t*) bufIn;

#if 0
  uint32_t addrOut            = addr;
  uint16_t lenOut             = len;
  uint8_t buf[32];

  /* Chunk of 31 bytes max */
  while (lenOut) {
    const uint8_t lenPart = (uint8_t) (lenOut <= 31U ?  lenOut : 31U);
    const uint8_t i2cReg  = (uint8_t) ((addrOut >> 8) & 0x1fUL);

    buf[0] = (uint8_t) (addrOut & 0xffUL);
    volatile uint8_t *bufPtr = &(buf[1]);

    for (uint8_t idx = 0U; idx < lenPart; idx++) {
      *(bufPtr++) = *(bufInPtr++);
    }
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_FRAM, i2cReg, 1U + lenPart, buf);

    /* Advance */
    lenOut  -= lenPart;
    addrOut += lenPart;
  }
#endif

  /* Verify and re-write differences */
  {
    uint32_t addrIn = addr;
    uint16_t lenIn  = len;
    bufInPtr = (volatile uint8_t*) bufIn;

    for (; lenIn; --lenIn) {
      uint8_t tryCnt;

      for (tryCnt = 16U; tryCnt; --tryCnt) {
        /* Read byte from FRAM */
        const uint8_t val1 = fram_read_nonSecured(addrIn);
        const uint8_t val2 = fram_read_nonSecured(addrIn);

        /* Being paranoid about data validity */
        if (val1 != val2) {
          continue;
        }

        if (*bufInPtr != val1) {
          /* Write out this data again */
          fram_write_nonSecured(addrIn, *bufInPtr);

        } else {
          /* FRAM content now equals to cache content */
          addrIn++;
          bufInPtr++;
          break;
        }
      }

      if (!tryCnt) {
        /* FRAM has not taken the data for multiple times */
      }
    }
  }
}

void fram_write_nonSecured(uint32_t addr, uint8_t data)
{
  const uint8_t i2cReg = (uint8_t) ((addr >> 8) & 0x1fUL);
  uint8_t buf[2] = { 0U };

  /* Sanity check */
  if (addr >= 8192UL) {
    return;
  }

  __DSB();
  __ISB();

  buf[0] = (uint8_t) (addr & 0xffUL);
  buf[1] = data;

  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_FRAM, i2cReg, sizeof(buf), buf);
}
