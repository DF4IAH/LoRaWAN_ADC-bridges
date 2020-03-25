/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

typedef enum ENABLE_MASK {

  ENABLE_MASK__LORAWAN_DEVICE                                 = 0x0020UL,                       // LoRaWAN as Device role
  ENABLE_MASK__GPS_DATA                                       = 0x0100UL,                       // GPS data interpreter
  ENABLE_MASK__GPS_1PPS                                       = 0x0200UL,                       // GPS 1PPS controlled MCU clock

} ENABLE_MASK_t;


typedef enum MON_MASK {

  MON_MASK__LORA                                              = 0x01UL,
  MON_MASK__GPS_RX                                            = 0x04UL,
  MON_MASK__GPS_TIMESYNC                                      = 0x08UL,

} MON_MASK_t;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define VERSION 20190710


/* TODO: select for which device the keys should be taken */
  #define IOT_DEVICE_0001
//#define IOT_DEVICE_0002
//#define IOT_DEVICE_0003
//#define IOT_DEVICE_0004


/* TODO: enable if device should be sent to sleep after last packet transmission */
  #define POWEROFF_AFTER_TXRX


/* TODO: enable if GNSS tracking information should be added */
//#define GNSS_EXTRA
//#define GNSS_EXTRA_SPEED_COURSE


/* TODO: enable if device has BNO085 9-axis chip on board */
//#define BNO085_EXTRA



/* Init that MX peripheries at start-up */
#define MAIN_INIT_MX_RTC
//#define MAIN_INIT_MX_TIM3
#define MAIN_INIT_MX_TIM16
//#define MAIN_INIT_MX_LPUART1
//#define MAIN_INIT_MX_USART1
#define MAIN_INIT_MX_I2C1
//#define MAIN_INIT_MX_I2C2
#define MAIN_INIT_MX_SPI3
#define MAIN_INIT_MX_ADC1
#define MAIN_INIT_MX_CRC
#define MAIN_INIT_MX_RNG

/* DeInit all possible peripheries before deep-sleep */
#define MAIN_DEINIT_MX_RNG
#define MAIN_DEINIT_MX_CRC
#define MAIN_DEINIT_MX_ADC1
#define MAIN_DEINIT_MX_SPI3
//#define MAIN_DEINIT_MX_I2C2
#define MAIN_DEINIT_MX_I2C1
#define MAIN_DEINIT_MX_USART1
#define MAIN_DEINIT_MX_LPUART1
#define MAIN_DEINIT_MX_TIM16
//#define MAIN_DEINIT_MX_TIM3
//#define MAIN_DEINIT_MX_RTC

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint8_t GET_BYTE_OF_WORD(uint32_t word, uint8_t pos);
void mainCalc_Float2Int(float in, uint32_t* out_i, uint16_t* out_p1000);
uint32_t mainGetRand(void);

void mainInitMxMinimal(void);
void mainDeInitMxMinimal(void);
void mainGotoDeepSleep(void);

void SystemClock_Config(void);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_WKUP_Pin GPIO_PIN_13
#define BOARD_WKUP_GPIO_Port GPIOC
#define GPIO_MUXG0_EN_Pin GPIO_PIN_0
#define GPIO_MUXG0_EN_GPIO_Port GPIOH
#define GPIO_MUXG1_EN_Pin GPIO_PIN_1
#define GPIO_MUXG1_EN_GPIO_Port GPIOH
#define LPUART1_SIM_RX_Pin GPIO_PIN_0
#define LPUART1_SIM_RX_GPIO_Port GPIOC
#define LPUART1_SIM_TX_Pin GPIO_PIN_1
#define LPUART1_SIM_TX_GPIO_Port GPIOC
#define GPIO_EXTI2_EXPCON_Pin GPIO_PIN_2
#define GPIO_EXTI2_EXPCON_GPIO_Port GPIOC
#define GPIO_EXTI2_EXPCON_EXTI_IRQn EXTI2_IRQn
#define GPIO_EXTI3_EXPCON_Pin GPIO_PIN_3
#define GPIO_EXTI3_EXPCON_GPIO_Port GPIOC
#define GPIO_EXTI3_EXPCON_EXTI_IRQn EXTI3_IRQn
#define REED_WKUP_Pin GPIO_PIN_0
#define REED_WKUP_GPIO_Port GPIOA
#define ADC1_IN6_PEXP_Pin GPIO_PIN_1
#define ADC1_IN6_PEXP_GPIO_Port GPIOA
#define ADC1_IN9_BAT_Pin GPIO_PIN_4
#define ADC1_IN9_BAT_GPIO_Port GPIOA
#define TIM3_CH1_CAPT_PEXP_Pin GPIO_PIN_6
#define TIM3_CH1_CAPT_PEXP_GPIO_Port GPIOA
#define GPIO_SW_BRIDGE_EN_Pin GPIO_PIN_4
#define GPIO_SW_BRIDGE_EN_GPIO_Port GPIOC
#define GPIO_SW_VOLTAGE_EN_Pin GPIO_PIN_0
#define GPIO_SW_VOLTAGE_EN_GPIO_Port GPIOB
#define LPUART1_SIM_RTS_Pin GPIO_PIN_1
#define LPUART1_SIM_RTS_GPIO_Port GPIOB
#define GPIO_SW_USART1_EN_Pin GPIO_PIN_12
#define GPIO_SW_USART1_EN_GPIO_Port GPIOB
#define LPUART1_SIM_CTS_Pin GPIO_PIN_13
#define LPUART1_SIM_CTS_GPIO_Port GPIOB
#define GPIO_SMPS_VDD12_EN_Pin GPIO_PIN_14
#define GPIO_SMPS_VDD12_EN_GPIO_Port GPIOB
#define GPIO_SW_VDD12_EN_Pin GPIO_PIN_15
#define GPIO_SW_VDD12_EN_GPIO_Port GPIOB
#define EXTI7_I2C1_PEXPB_Pin GPIO_PIN_7
#define EXTI7_I2C1_PEXPB_GPIO_Port GPIOC
#define EXTI7_I2C1_PEXPB_EXTI_IRQn EXTI9_5_IRQn
#define EXTI8_I2C1_PEXPA_Pin GPIO_PIN_8
#define EXTI8_I2C1_PEXPA_GPIO_Port GPIOC
#define EXTI8_I2C1_PEXPA_EXTI_IRQn EXTI9_5_IRQn
#define EXTI9_I2C1_ADC_DRDY_Pin GPIO_PIN_9
#define EXTI9_I2C1_ADC_DRDY_GPIO_Port GPIOC
#define EXTI9_I2C1_ADC_DRDY_EXTI_IRQn EXTI9_5_IRQn
#define GPIO_SW_I2C1_EN_Pin GPIO_PIN_8
#define GPIO_SW_I2C1_EN_GPIO_Port GPIOA
#define USART1_ESP_CTS_Pin GPIO_PIN_11
#define USART1_ESP_CTS_GPIO_Port GPIOA
#define USART1_ESP_RTS_Pin GPIO_PIN_12
#define USART1_ESP_RTS_GPIO_Port GPIOA
#define GPIO_SW_SPI3_EN_Pin GPIO_PIN_5
#define GPIO_SW_SPI3_EN_GPIO_Port GPIOB
#define USART1_ESP_TX_Pin GPIO_PIN_6
#define USART1_ESP_TX_GPIO_Port GPIOB
#define USART1_ESP_RX_Pin GPIO_PIN_7
#define USART1_ESP_RX_GPIO_Port GPIOB
#define TIM16_CH1_BUZZER_Pin GPIO_PIN_8
#define TIM16_CH1_BUZZER_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#define min(a,b) (a) < (b) ?  (a) : (b)
#define max(a,b) (a) > (b) ?  (a) : (b)

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
