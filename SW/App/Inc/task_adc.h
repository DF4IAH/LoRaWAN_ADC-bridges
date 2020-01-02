/*
 * task_adc.h
 *
 *  Created on: 13.06.2019
 *      Author: DF4IAH
 */

#ifndef INC_TASK_ADC_H_
#define INC_TASK_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif


/* TODO: activate to do internal offset calibration before metering */
#define ADC_DO_CALIBRATION

/* TODO: activate when using 8 bridges */
//#define I2CADC_USE_8_BRIDGES



/* TESTING only - check ADC noise & optimizations */
//#define ADC_TEST_DEVIATION


#define ADC1_DMA_CHANNELS               5

#define ADC_V_OFFS_VREF_mV          -135.4f
#define ADC_V_OFFS_BAT_mV             56.0f
#define ADC_V_OFFS_EXTBAT_mV          56.0f
#define ADC_V_OFFS_PEXP_mV            56.0f

#define ADC_MUL_BAT                    3.0000f
#define ADC_MUL_EXTBAT                 3.2435f
#define ADC_MUL_PEXP                   1.0200f
#define ADC_MUL_TEMP                   1.1825f


typedef enum ADC_ENUM {

  ADC_ADC1_REFINT_VAL               = 0x0000U,
  ADC_ADC1_VREF_MV,
  ADC_ADC1_BAT_MV,
  ADC_ADC1_EXTBAT_MV,
  ADC_ADC1_PEXP_MV,
  ADC_ADC1_TEMP_C,

  ADC_I2CADC_BRIDGE0_RAW            = 0x0010U,
  ADC_I2CADC_BRIDGE1_RAW,
  ADC_I2CADC_BRIDGE2_RAW,
  ADC_I2CADC_BRIDGE3_RAW,
  ADC_I2CADC_BRIDGE4_RAW,
  ADC_I2CADC_BRIDGE5_RAW,
  ADC_I2CADC_BRIDGE6_RAW,
  ADC_I2CADC_BRIDGE7_RAW,

#if 0
  ADC1__NOT_RUNNING                 = 0x0100U,
  ADC1__RUNNING_VREFINT,
  ADC1__RUNNING_VBAT,
  ADC1__RUNNING_TEMP,
#endif

} ADC_ENUM_t;


/* Event Group ADC */
typedef enum EG_ADC_ENUM {

  EG_ADC1__CONV_RUNNING             = 0x000001UL,
  EG_ADC1__CONV_AVAIL_REFINT        = 0x000002UL,
  EG_ADC1__CONV_AVAIL_VREF          = 0x000004UL,
  EG_ADC1__CONV_AVAIL_BAT           = 0x000008UL,
  EG_ADC1__CONV_AVAIL_EXTBAT        = 0x000010UL,
  EG_ADC1__CONV_AVAIL_PEXP          = 0x000020UL,
  EG_ADC1__CONV_AVAIL_TEMP          = 0x000040UL,

  EG_I2CADC__DRDY                   = 0x000080UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE0     = 0x000100UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE1     = 0x000200UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE2     = 0x000400UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE3     = 0x000800UL,

  EG_I2CADC__CONV_AVAIL_BRIDGE4     = 0x001000UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE5     = 0x002000UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE6     = 0x004000UL,
  EG_I2CADC__CONV_AVAIL_BRIDGE7     = 0x008000UL,

#ifdef GNSS_EXTRA
  EG_GNSS__AVAIL_POS                = 0x010000UL,
  EG_GNSS__1PPS                     = 0x020000UL,
  EG_GNSS__TX_DMA_RUN               = 0x100000UL,
  EG_GNSS__TX_DMA_END               = 0x200000UL,
  EG_GNSS__RX_DMA_RUN               = 0x400000UL,
  EG_GNSS__RX_DMA_END               = 0x800000UL,
#endif

} EG_ADC_ENUM_t;


void adc_i2c1_mux(ADC_ENUM_t channel);
uint8_t adc_i2c1_getPair(volatile int32_t* ch_a_raw, volatile int32_t* ch_b_raw);

void adcStartConv(ADC_ENUM_t adc);
void adcStopConv(ADC_ENUM_t adc);

void adc_EXTI_Callback(void);

void adcTaskInit(void);
void adcTaskLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_TASK_ADC_H_ */
