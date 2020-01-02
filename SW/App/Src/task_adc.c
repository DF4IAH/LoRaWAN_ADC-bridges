/*
 * task_adc.c
 *
 *  Created on: 13.06.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stddef.h>
#include <sys/_stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "bus_i2c.h"
#include "task_LoRaWAN.h"

#include "task_adc.h"


/* Variables -----------------------------------------------------------------*/

#ifndef I2CADC_USE_8_BRIDGES
# define ADC_I2CADC_CH              4
#else
# define ADC_I2CADC_CH              8
#endif

#define ADC_MEAN_COUNT              1


extern EventGroupHandle_t           globalEventGroupHandle;
extern EventGroupHandle_t           adcEventGroupHandle;
extern EventGroupHandle_t           loraEventGroupHandle;

extern osMutexId                    iot4BeesApplUpDataMutexHandle;

extern IoT4BeesCtrlApp_up_t         g_Iot4BeesApp_up;
extern IoT4BeesCtrlApp_down_t       g_Iot4BeesApp_down;

extern ADC_HandleTypeDef            hadc1;
extern DMA_HandleTypeDef            hdma_adc1;

extern osSemaphoreId                i2c1_BSemHandle;
extern I2C_HandleTypeDef            hi2c1;
extern volatile uint8_t             i2c1RxBuffer[I2C_RXBUFSIZE];

static bool                         s_adcInitRdy                            = false;

static uint16_t                     s_adc1_dma_buf[ADC1_DMA_CHANNELS]       = { 0U };

static float                        s_adc1_val_mean_refint[ADC_MEAN_COUNT]  = { 0.0f };
static float                        s_adc1_val_mean_vbat[ADC_MEAN_COUNT]    = { 0.0f };
static float                        s_adc1_val_mean_vextbat[ADC_MEAN_COUNT] = { 0.0f };
static float                        s_adc1_val_mean_vpexp[ADC_MEAN_COUNT]   = { 0.0f };
static float                        s_adc1_val_mean_temp[ADC_MEAN_COUNT]    = { 0.0f };

volatile float                      g_adc_refint_val                        = 0.0f;
volatile float                      g_adc_vref_mv                           = 0.0f;
volatile float                      g_adc_bat_mv                            = 0.0f;
volatile float                      g_adc_extbat_mv                         = 0.0f;
volatile float                      g_adc_pexp_mv                           = 0.0f;
volatile float                      g_adc_temp_c                            = 0.0f;

volatile int32_t                    g_adci2c_ch[ADC_I2CADC_CH]              = { 0U };


#ifdef GNSS_EXTRA
volatile float                      g_gnss_lat                              = 0.0f;
volatile float                      g_gnss_lon                              = 0.0f;
volatile float                      g_gnss_alt_m                            = 0.0f;
volatile float                      g_gnss_acc                              = 1262.0f;
# ifdef GNSS_EXTRA_SPEED_COURSE
volatile float                      g_gnss_spd_kmh                          = 0.0f;
volatile float                      g_gnss_crs                              = 0.0f;
# endif

uint8_t                             g_gnss_reset                            = 0U;
#endif


static float adcCalcMean(float* fa, uint32_t cnt, float newVal)
{
  float sum = 0.0f;

  for (int i = 0; i < cnt; i++) {
    sum += fa[i];
  }

  if (!sum) {
    /* Init array for quick mean value */
    for (int j = 0; j < cnt; j++) {
      fa[j] = newVal;
    }
    sum = newVal * cnt;
  }

  /* Correct sum for old and ne value */
  sum -= fa[0];
  sum += newVal;

  /* Move array */
  for (int k = 1; k < cnt; k++) {
    fa[k - 1] = fa[k];
  }

  /* Enter new value at the end */
  fa[cnt - 1] = newVal;

  return sum / cnt;
}


static void adc_i2c1_init(void)
{
  /* RESET register file */
  {
    const uint8_t resetBuf[1] = { 0x01U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(resetBuf), resetBuf);
  }

  /* Switch digital and analog part ON */
  {
    const uint8_t pwrOnBuf[1] = { 0x06U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(pwrOnBuf), pwrOnBuf);
  }

  /* Wait until ADC is powered up */
  uint8_t data = 0U;
  do {
    const uint8_t pwrReg[1] = { 0x00 };

    /* Read status register */
    if (HAL_I2C_ERROR_NONE == i2cSequenceRead(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, sizeof(pwrReg), (uint8_t*) pwrReg, sizeof(data))) {
      data = i2c1RxBuffer[0];
    } else {
      data = 0U;
    }
  } while (!(data & (1U << 3)));


  /* Disable any I2C pullup */
  {
    const uint8_t noI2cPuBuf[1] = { 0x10U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x11, sizeof(noI2cPuBuf), noI2cPuBuf);
  }

  /* Set ADC mode */
  {
    const uint8_t pgaAdcModeBuf[1] = { 0x32U };  // No common mode, chopper ADC clock delay = 2
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x15, sizeof(pgaAdcModeBuf), pgaAdcModeBuf);
  }

  /* Set PGA */
  {
  //const uint8_t pgaMulBuf[1] = { 0x00U };  // PGA gain =  1
  //const uint8_t pgaMulBuf[1] = { 0x04U };  // PGA gain = 16
    const uint8_t pgaMulBuf[1] = { 0x05U };  // PGA gain = 32
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x01, sizeof(pgaMulBuf), pgaMulBuf);

    const uint8_t pgaBpBuf[1] = { 0x20U };  // PGA not bypassed, buffer enabled
  //const uint8_t pgaBpBuf[1] = { 0x10U };  // PGA bypassed, buffer disabled
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x1b, sizeof(pgaBpBuf), pgaBpBuf);
  }

#define ADC_DO_CALIBRATION
#ifdef ADC_DO_CALIBRATION
  /* Calibration */
  do {
    /* Start calibration */
    {
        const uint8_t doCalBuf[1] = { 0x00U | (1U << 2) };  const uint32_t dly = 100UL;    //   10 SPS
      //const uint8_t doCalBuf[1] = { 0x10U | (1U << 2) };  const uint32_t dly =  50UL;    //   20 SPS
      //const uint8_t doCalBuf[1] = { 0x20U | (1U << 2) };  const uint32_t dly =  20UL;    //   40 SPS
      //const uint8_t doCalBuf[1] = { 0x30U | (1U << 2) };  const uint32_t dly =  13UL;    //   80 SPS
      //const uint8_t doCalBuf[1] = { 0x70U | (1U << 2) };  const uint32_t dly =   4UL;    //  320 SPS
      i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x02, sizeof(doCalBuf), doCalBuf);
      osDelay(dly);
    }

    /* Wait until calibration is done */
    uint8_t data = 0U;
    do {
      const uint8_t calReg[1] = { 0x02 };

      osDelay(2UL);

      /* Read status register */
      if (HAL_I2C_ERROR_NONE == i2cSequenceRead(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, sizeof(calReg), (uint8_t*) calReg, sizeof(data))) {
        data = i2c1RxBuffer[0];
      } else {
        data = 0xffU;
      }
    } while ((data & (1U << 2)));

    /* Check for errors during calibration (CAL_ERR) */
    if (!(data & 0x08)) {
      /* Calibration okay */
      break;
    }
  } while (true);
#endif
}

static void adc_i2c1_deInit(void)
{
  const uint8_t pwrOffBuf[1] = { 0x00U };

  /* Switch digital and analog part OFF */
  i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(pwrOffBuf), pwrOffBuf);
}

uint8_t adc_i2c1_getPair(volatile int32_t* ch_a_raw, volatile int32_t* ch_b_raw)
{
  uint32_t valA = 0UL;
  uint32_t valB = 0UL;

  /* Channel A */

  /* Enable power for bridge sensors */
  HAL_GPIO_WritePin(GPIO_SW_BRIDGE_EN_GPIO_Port, GPIO_SW_BRIDGE_EN_Pin, GPIO_PIN_SET);

  /* Set to channel A */
  {
    const uint8_t ctrl2Buf[1] = { 0x00U };  // Channel A, 10SPS, no_CALS
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x02, sizeof(ctrl2Buf), ctrl2Buf);
  }
  osDelay(10UL);

  /* Enable CS for conversion start */
  {
    const uint8_t csBuf[1] = { 0x16U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(csBuf), csBuf);
  }

  /* Wait until sampled data is valid */
  xEventGroupWaitBits(adcEventGroupHandle, EG_I2CADC__DRDY, EG_I2CADC__DRDY, pdFALSE, portMAX_DELAY);

  /* Read 24 bit value */
  {
    const uint8_t dataReg[1]  = { 0x12 };

    /* Read data register */
    if (HAL_I2C_ERROR_NONE == i2cSequenceRead(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, sizeof(dataReg), (uint8_t*) dataReg, 3U)) {
      valA  = (uint32_t)i2c1RxBuffer[0] << 16;
      valA |= (uint32_t)i2c1RxBuffer[1] <<  8;
      valA |= (uint32_t)i2c1RxBuffer[2];

    } else {
      *ch_a_raw = 0U;
      *ch_b_raw = 0U;
      return HAL_ERROR;
    }
  }

  /* Disable power for bridge sensors */
  HAL_GPIO_WritePin(GPIO_SW_BRIDGE_EN_GPIO_Port, GPIO_SW_BRIDGE_EN_Pin, GPIO_PIN_RESET);

  /* Disable CS for conversion stop */
  {
    const uint8_t csOffBuf[1] = { 0x06U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(csOffBuf), csOffBuf);
  }


  /* Channel B */

  /* Enable power for bridge sensors */
  HAL_GPIO_WritePin(GPIO_SW_BRIDGE_EN_GPIO_Port, GPIO_SW_BRIDGE_EN_Pin, GPIO_PIN_SET);

  /* Set to channel B */
  {
    const uint8_t ctrl2Buf[1] = { 0x80U };  // Channel B, 10SPS, no_CALS
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x02, sizeof(ctrl2Buf), ctrl2Buf);
  }
  osDelay(10UL);

  /* Enable CS for conversion start */
  {
    const uint8_t csBuf[1] = { 0x16U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(csBuf), csBuf);
  }

  /* Wait until sampled data is valid */
  xEventGroupWaitBits(adcEventGroupHandle, EG_I2CADC__DRDY, EG_I2CADC__DRDY, pdFALSE, portMAX_DELAY);

  /* Read 24 bit value */
  {
    const uint8_t dataReg[1]  = { 0x12 };

    /* Read data register */
    if (HAL_I2C_ERROR_NONE == i2cSequenceRead(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, sizeof(dataReg), (uint8_t*) dataReg, 3U)) {
      valB  = (uint32_t)i2c1RxBuffer[0] << 16;
      valB |= (uint32_t)i2c1RxBuffer[1] <<  8;
      valB |= (uint32_t)i2c1RxBuffer[2];

    } else {
      *ch_b_raw = 0U;
      return HAL_ERROR;
    }
  }

  /* Disable power for bridge sensors */
  HAL_GPIO_WritePin(GPIO_SW_BRIDGE_EN_GPIO_Port, GPIO_SW_BRIDGE_EN_Pin, GPIO_PIN_RESET);

  /* Disable CS for conversion stop */
  {
    const uint8_t csOffBuf[1] = { 0x06U };
    i2cSequenceWriteLong(&hi2c1, i2c1_BSemHandle, I2C1_PORT_ADC, 0x00, sizeof(csOffBuf), csOffBuf);
  }

  *ch_a_raw = valA;
  *ch_b_raw = valB;

  return HAL_OK;
}


void adc_i2c1_mux(ADC_ENUM_t channel)
{
  switch (channel) {
  case ADC_I2CADC_BRIDGE0_RAW:
  case ADC_I2CADC_BRIDGE1_RAW:
  {
    HAL_GPIO_WritePin(GPIO_MUXG0_EN_GPIO_Port, GPIO_MUXG0_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIO_MUXG1_EN_GPIO_Port, GPIO_MUXG1_EN_Pin, GPIO_PIN_RESET);
  }
    break;

  case ADC_I2CADC_BRIDGE2_RAW:
  case ADC_I2CADC_BRIDGE3_RAW:
  {
    HAL_GPIO_WritePin(GPIO_MUXG0_EN_GPIO_Port, GPIO_MUXG0_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIO_MUXG1_EN_GPIO_Port, GPIO_MUXG1_EN_Pin, GPIO_PIN_SET);
  }
    break;

#ifdef I2CADC_USE_8_BRIDGES
  case ADC_I2CADC_BRIDGE4_RAW:
  case ADC_I2CADC_BRIDGE5_RAW:
  {
  }
    break;

  case ADC_I2CADC_BRIDGE6_RAW:
  case ADC_I2CADC_BRIDGE7_RAW:
  {
  }
    break;
#endif

  default:
  {
    /* Turn every MUX away of the external ADC */
    HAL_GPIO_WritePin(GPIO_MUXG0_EN_GPIO_Port, GPIO_MUXG0_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIO_MUXG1_EN_GPIO_Port, GPIO_MUXG1_EN_Pin, GPIO_PIN_RESET);
  }
  }  // switch (channel)
}

void adcStartConv(ADC_ENUM_t adc)
{
  switch (adc) {
  case ADC_ADC1_REFINT_VAL:
  case ADC_ADC1_VREF_MV:
  case ADC_ADC1_BAT_MV:
  case ADC_ADC1_EXTBAT_MV:
  case ADC_ADC1_PEXP_MV:
  case ADC_ADC1_TEMP_C:
  {
    ADC_ChannelConfTypeDef sConfig;

    /* In case when it is needed */
    HAL_ADC_Stop_DMA(&hadc1);

    /* Remove old flag state */
    xEventGroupClearBits(adcEventGroupHandle, EG_ADC1__CONV_RUNNING | EG_ADC1__CONV_AVAIL_REFINT | EG_ADC1__CONV_AVAIL_VREF | EG_ADC1__CONV_AVAIL_BAT | EG_ADC1__CONV_AVAIL_EXTBAT | EG_ADC1__CONV_AVAIL_PEXP);

    /* Enable power for battery voltage divider */
    HAL_GPIO_WritePin(GPIO_SW_VOLTAGE_EN_GPIO_Port, GPIO_SW_VOLTAGE_EN_Pin, GPIO_PIN_SET);
    osDelay(25UL);

    sConfig.Channel       = ADC_CHANNEL_VREFINT;
    sConfig.Rank          = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime  = ADC_SAMPLETIME_92CYCLES_5;
    sConfig.SingleDiff    = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber  = ADC_OFFSET_NONE;
    sConfig.Offset        = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
      Error_Handler();
    }

    xEventGroupSetBits(adcEventGroupHandle, EG_ADC1__CONV_RUNNING);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*) &(s_adc1_dma_buf[0]), ADC1_DMA_CHANNELS);
  }
    break;

  case ADC_I2CADC_BRIDGE0_RAW:
  case ADC_I2CADC_BRIDGE1_RAW:
  {
    adc_i2c1_mux(ADC_I2CADC_BRIDGE0_RAW);
    osDelay(10UL);
    adc_i2c1_getPair(&g_adci2c_ch[0], &g_adci2c_ch[1]);
    xEventGroupSetBits(adcEventGroupHandle, (EG_I2CADC__CONV_AVAIL_BRIDGE0 | EG_I2CADC__CONV_AVAIL_BRIDGE1));
  }
    break;

  case ADC_I2CADC_BRIDGE2_RAW:
  case ADC_I2CADC_BRIDGE3_RAW:
  {
    adc_i2c1_mux(ADC_I2CADC_BRIDGE2_RAW);
    osDelay(10UL);
    adc_i2c1_getPair(&g_adci2c_ch[2], &g_adci2c_ch[3]);
    xEventGroupSetBits(adcEventGroupHandle, (EG_I2CADC__CONV_AVAIL_BRIDGE2 | EG_I2CADC__CONV_AVAIL_BRIDGE3));
  }
    break;

#ifdef I2CADC_USE_8_BRIDGES
  case ADC_I2CADC_BRIDGE4_RAW:
  case ADC_I2CADC_BRIDGE5_RAW:
  {
    adc_i2c1_mux(ADC_I2CADC_BRIDGE4_RAW);
    osDelay(10UL);
    adc_i2c1_getPair(&g_adci2c_ch[4], &g_adci2c_ch[5]);
    xEventGroupSetBits(adcEventGroupHandle, (EG_I2CADC__CONV_AVAIL_BRIDGE4 | EG_I2CADC__CONV_AVAIL_BRIDGE5));
  }
    break;

  case ADC_I2CADC_BRIDGE6_RAW:
  case ADC_I2CADC_BRIDGE7_RAW:
  {
    adc_i2c1_mux(ADC_I2CADC_BRIDGE6_RAW);
    osDelay(10UL);
    adc_i2c1_getPair(&g_adci2c_ch[6], &g_adci2c_ch[7]);
    xEventGroupSetBits(adcEventGroupHandle, (EG_I2CADC__CONV_AVAIL_BRIDGE6 | EG_I2CADC__CONV_AVAIL_BRIDGE7));
  }
    break;
#endif

  default: { }
  }  // switch (adc)
}

void adcStopConv(ADC_ENUM_t adc)
{
  switch (adc) {
  case ADC_ADC1_REFINT_VAL:
  case ADC_ADC1_VREF_MV:
  case ADC_ADC1_BAT_MV:
  case ADC_ADC1_EXTBAT_MV:
  case ADC_ADC1_PEXP_MV:
  case ADC_ADC1_TEMP_C:
  {
    /* Disable power for battery voltage divider */
    HAL_GPIO_WritePin(GPIO_SW_VOLTAGE_EN_GPIO_Port, GPIO_SW_VOLTAGE_EN_Pin, GPIO_PIN_RESET);

    /* Stop ADC convert process */
    HAL_ADC_Stop_DMA(&hadc1);

    /* Inform about state */
    xEventGroupClearBits(adcEventGroupHandle, EG_ADC1__CONV_RUNNING);
  }
    break;

  case ADC_I2CADC_BRIDGE0_RAW:
  case ADC_I2CADC_BRIDGE1_RAW:
  {
    /* TBD */
  }
    break;

  case ADC_I2CADC_BRIDGE2_RAW:
  case ADC_I2CADC_BRIDGE3_RAW:
  {
    /* TBD */
  }
    break;

#ifdef I2CADC_USE_8_BRIDGES
  case ADC_I2CADC_BRIDGE4_RAW:
  case ADC_I2CADC_BRIDGE5_RAW:
  {
    /* TBD */
  }
    break;

  case ADC_I2CADC_BRIDGE6_RAW:
  case ADC_I2CADC_BRIDGE7_RAW:
  {
    /* TBD */
  }
    break;
#endif

  default:
    { }
  }
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* ISR context */
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;

  if (hadc == &hadc1) {
    uint32_t groupBits = 0UL;

    /* Disable power for battery voltage divider */
    HAL_GPIO_WritePin(GPIO_SW_VOLTAGE_EN_GPIO_Port, GPIO_SW_VOLTAGE_EN_Pin, GPIO_PIN_RESET);

    /* In case when it is needed */
    HAL_ADC_Stop_DMA(&hadc1);

    /* Inform about state */
    xEventGroupClearBitsFromISR(adcEventGroupHandle, EG_ADC1__CONV_RUNNING);

    /* VREFINT */
    {
      const float vref_cal_value = (float) *VREFINT_CAL_ADDR;

      const float mean  = adcCalcMean(s_adc1_val_mean_refint, ADC_MEAN_COUNT, s_adc1_dma_buf[0] / 16.0f);
      g_adc_refint_val  = mean;
      g_adc_vref_mv     = (((float)VREFINT_CAL_VREF * vref_cal_value) / g_adc_refint_val) + ADC_V_OFFS_VREF_mV;
      groupBits |= (EG_ADC1__CONV_AVAIL_REFINT | EG_ADC1__CONV_AVAIL_VREF);
    }

    /* VBAT */
    {
      const float mean  = adcCalcMean(s_adc1_val_mean_vbat, ADC_MEAN_COUNT, s_adc1_dma_buf[1] / 16.0f);
      g_adc_bat_mv      = ((ADC_MUL_BAT * mean * g_adc_vref_mv) / 4095.0f) + ADC_V_OFFS_BAT_mV;
      groupBits |= EG_ADC1__CONV_AVAIL_BAT;
    }

    /* VEXTBAT */
    {
      const float mean  = adcCalcMean(s_adc1_val_mean_vextbat, ADC_MEAN_COUNT, s_adc1_dma_buf[2] / 16.0f);
      g_adc_extbat_mv   = ((ADC_MUL_EXTBAT * mean * g_adc_vref_mv) / 4095.0f) + ADC_V_OFFS_EXTBAT_mV;
      groupBits |= EG_ADC1__CONV_AVAIL_EXTBAT;
    }

    /* VPEXP */
    {
      const float mean  = adcCalcMean(s_adc1_val_mean_vpexp, ADC_MEAN_COUNT, s_adc1_dma_buf[3] / 16.0f);
      g_adc_pexp_mv     = ((ADC_MUL_PEXP * mean * g_adc_vref_mv) / 4095.0f) + ADC_V_OFFS_PEXP_mV;
      groupBits |= EG_ADC1__CONV_AVAIL_PEXP;
    }

    /* TEMP */
    {
      const uint16_t* TS_CAL1_ADDR = (const uint16_t*) 0x1FFF75A8;
      const uint16_t* TS_CAL2_ADDR = (const uint16_t*) 0x1FFF75CA;
      const float ts_cal1 = (float) *TS_CAL1_ADDR;
      const float ts_cal2 = (float) *TS_CAL2_ADDR;

      const float mean  = adcCalcMean(s_adc1_val_mean_temp, ADC_MEAN_COUNT, s_adc1_dma_buf[4] / 16.0f);
      g_adc_temp_c      = ((float)(110 - 30) / (ts_cal2 - ts_cal1)) * ((mean * ADC_MUL_TEMP) - ts_cal1) + 30.f;
      groupBits |= EG_ADC1__CONV_AVAIL_TEMP;
    }

    /* Set all ready ADC channel flags */
    xEventGroupSetBitsFromISR(adcEventGroupHandle, groupBits, &pxHigherPriorityTaskWoken);
  }
}

void adc_EXTI_Callback(void)
{
  BaseType_t pxHigherPriorityTaskWoken = 0L;

  if (s_adcInitRdy) {
    xEventGroupSetBitsFromISR(adcEventGroupHandle, EG_I2CADC__DRDY, &pxHigherPriorityTaskWoken);
  }
}


void adcTaskInit(void)
{
  /* Wait until I2C is ready */
  xEventGroupWaitBits(globalEventGroupHandle, Global_EGW__I2C_RDY, 0UL, pdFALSE, portMAX_DELAY);

  /* External ADC Init */
  adc_i2c1_init();

  /* Allow ADC DRDY callbacks */
  s_adcInitRdy = true;

  /* Wait until LoRaWAN Link is established */
  xEventGroupWaitBits(loraEventGroupHandle, Lora_EGW__TRANSPORT_READY, Lora_EGW__TRANSPORT_READY, pdFALSE, portMAX_DELAY);

#if 0
  // TODO: not used (yet)

  /* Clear queue */
  uint8_t inChr = 0;
  while (xQueueReceive(adcInQueueHandle, &inChr, 0) == pdPASS) {
  }
#endif

#ifdef ADC_TEST_DEVIATION
  float mean_ch0 = 0.0f;
  float mean_ch1 = 0.0f;
  float dev_ch0  = 0.0f;
  float dev_ch1  = 0.0f;
  bool isInitial = true;

  for (uint32_t cnt = 10UL; cnt; --cnt) {
    adcStartConv(ADC_I2CADC_BRIDGE0_RAW);

    EventBits_t eb = xEventGroupWaitBits(adcEventGroupHandle,
        (EG_I2CADC__CONV_AVAIL_BRIDGE0 | EG_I2CADC__CONV_AVAIL_BRIDGE1),
        (EG_I2CADC__CONV_AVAIL_BRIDGE0 | EG_I2CADC__CONV_AVAIL_BRIDGE1),
        pdTRUE, portMAX_DELAY);
    (void) eb;

    /* Get the data */
    const int32_t this_ch0 = g_adci2c_ch[0];
    const int32_t this_ch1 = g_adci2c_ch[1];

    /* Calc new mean value as low pass filter */
    if (isInitial) {
      mean_ch0 = this_ch0;
      mean_ch1 = this_ch1;
      isInitial = false;

    } else {
      mean_ch0 = 0.95f * mean_ch0 + 0.05f * this_ch0;
      mean_ch1 = 0.95f * mean_ch1 + 0.05f * this_ch1;
    }

    /* Calc deviations */
    const float this_dev0 = fabs(this_ch0 - mean_ch0);
    const float this_dev1 = fabs(this_ch1 - mean_ch1);
    dev_ch0 = 0.95f * dev_ch0 + 0.05f * this_dev0;
    dev_ch1 = 0.95f * dev_ch1 + 0.05f * this_dev1;
  }

  (void) mean_ch0;
  (void) mean_ch1;
  (void) dev_ch0;
  (void) dev_ch1;

  while (true) {
    osDelay(1000UL);
  }
#endif

  osDelay(100UL);
}

void adcTaskLoop(void)
{
#ifndef POWEROFF_AFTER_TXRX
  static TickType_t previousWakeTime = 0UL;
#endif

  /* Start internal ADC channels working w/ DMA */
  adcStartConv(ADC_ADC1_REFINT_VAL);

  /* External ADC Init (again) */
  adc_i2c1_init();

  adcStartConv(ADC_I2CADC_BRIDGE0_RAW);
  adcStartConv(ADC_I2CADC_BRIDGE2_RAW);

#ifdef I2CADC_USE_8_BRIDGES
  adcStartConv(ADC_I2CADC_BRIDGE4_RAW);
  adcStartConv(ADC_I2CADC_BRIDGE6_RAW);
#endif

  /* Wait for the internal ADC channels */
  /* EventBits_t eb = */  xEventGroupWaitBits(adcEventGroupHandle,
      (EG_ADC1__CONV_AVAIL_VREF | EG_ADC1__CONV_AVAIL_BAT | EG_ADC1__CONV_AVAIL_EXTBAT | EG_ADC1__CONV_AVAIL_PEXP),
      0UL,
      pdTRUE, portMAX_DELAY);

  /* Wait for the external ADC channels 0..3 */
  xEventGroupWaitBits(adcEventGroupHandle,
      (EG_I2CADC__CONV_AVAIL_BRIDGE0 | EG_I2CADC__CONV_AVAIL_BRIDGE1 | EG_I2CADC__CONV_AVAIL_BRIDGE2 | EG_I2CADC__CONV_AVAIL_BRIDGE3),
      0UL,
      pdTRUE, portMAX_DELAY);

#ifdef I2CADC_USE_8_BRIDGES
  xEventGroupWaitBits(adcEventGroupHandle,
      (EG_I2CADC__CONV_AVAIL_BRIDGE4 | EG_I2CADC__CONV_AVAIL_BRIDGE5 | EG_I2CADC__CONV_AVAIL_BRIDGE6 | EG_I2CADC__CONV_AVAIL_BRIDGE7),
      0UL,
      pdTRUE, portMAX_DELAY);
#endif

  /* Shutdown external ADC device */
  adc_i2c1_deInit();

  /* Fill in the data */

  /* Get Mutex for pushing data */
  if (pdPASS == xSemaphoreTake(iot4BeesApplUpDataMutexHandle, 1000UL / portTICK_PERIOD_MS)) {
    g_Iot4BeesApp_up.flags = 0U;

    g_Iot4BeesApp_up.ad_ch_raw[0]         = (uint16_t) ((g_adci2c_ch[0] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[1]         = (uint16_t) ((g_adci2c_ch[1] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[2]         = (uint16_t) ((g_adci2c_ch[2] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[3]         = (uint16_t) ((g_adci2c_ch[3] + 0x7f) >> 8);

#ifdef I2CADC_USE_8_BRIDGES
    g_Iot4BeesApp_up.ad_ch_raw[4]         = (uint16_t) ((g_adci2c_ch[4] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[5]         = (uint16_t) ((g_adci2c_ch[5] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[6]         = (uint16_t) ((g_adci2c_ch[6] + 0x7f) >> 8);
    g_Iot4BeesApp_up.ad_ch_raw[7]         = (uint16_t) ((g_adci2c_ch[7] + 0x7f) >> 8);
#endif

    g_Iot4BeesApp_up.v_bat_100th_volt     = (int16_t) (g_adc_extbat_mv  / 10.0f + 0.5f);
    g_Iot4BeesApp_up.v_aux_100th_volt[0]  = (int16_t) (g_adc_bat_mv     / 10.0f + 0.5f);
    g_Iot4BeesApp_up.v_aux_100th_volt[1]  = (int16_t) (g_adc_vref_mv    / 10.0f + 0.5f);
    g_Iot4BeesApp_up.v_aux_100th_volt[2]  = (int16_t) (g_adc_pexp_mv    / 10.0f + 0.5f);

    g_Iot4BeesApp_up.wx_temp_10th_degC    = (int16_t) (g_adc_temp_c     * 10.0f + 0.5f);
    g_Iot4BeesApp_up.wx_rh                = (int8_t)  (0.0f                     + 0.5f);
    g_Iot4BeesApp_up.wx_baro_m950_Pa      = (int8_t)  ((950.00f - 950.0f)       + 0.5f);

#ifdef GNSS_EXTRA
    /* Do not delay by GNSS when REED_Switch is still being shorted */
    if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(REED_WKUP_GPIO_Port, REED_WKUP_Pin)) {
        /* Real GNSS data - max. 7 minutes to wait for initial GPS data */
      const EventBits_t eb = xEventGroupWaitBits(adcEventGroupHandle, EG_GNSS__AVAIL_POS, EG_GNSS__AVAIL_POS, pdFALSE, 7 * 60000UL / portTICK_PERIOD_MS);
        if (eb & EG_GNSS__AVAIL_POS) {
          /* Take over current position */
          __DSB();
          __ISB();
          g_Iot4BeesApp_up.gnss_lat     = g_gnss_lat;
          g_Iot4BeesApp_up.gnss_lon     = g_gnss_lon;
          g_Iot4BeesApp_up.gnss_alt_m   = g_gnss_alt_m;
          g_Iot4BeesApp_up.gnss_acc_m   = g_gnss_acc;

#ifdef GNSS_EXTRA_SPEED_COURSE
          g_Iot4BeesApp_up.gnss_spd_kmh = g_gnss_spd_kmh;
          g_Iot4BeesApp_up.gnss_crs     = g_gnss_crs;
#endif
        }
    }
#endif

    xSemaphoreGive(iot4BeesApplUpDataMutexHandle);
    xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DATA_UP_REQ);
  }


#ifdef POWEROFF_AFTER_TXRX

  const EventBits_t eb = xEventGroupGetBits(globalEventGroupHandle);
  if (!(eb & Global_EGW__REEDWKUP_05SEC) &&
      !(eb & Global_EGW__REEDWKUP_10SEC)) {
    /* Fire and forget: wait until TX has completed */
    xEventGroupWaitBits(loraEventGroupHandle, Lora_EGW__DATA_UP_DONE, Lora_EGW__DATA_UP_DONE, pdFALSE, portMAX_DELAY);

  } else {
    osDelay(20000UL);
  }

  // TODO: The ESP module is responsible to go to DeepSleep, not here
  mainGotoDeepSleep();

#else

  /* Repeat test message every 30 sec */
  osDelayUntil(&previousWakeTime, 30000UL);

#endif
}
