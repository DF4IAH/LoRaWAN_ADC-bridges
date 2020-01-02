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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define REED_WKUP_Pin GPIO_PIN_0
#define REED_WKUP_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define SMPS_EN_Pin GPIO_PIN_4
#define SMPS_EN_GPIO_Port GPIOA
#define SMPS_V1_Pin GPIO_PIN_5
#define SMPS_V1_GPIO_Port GPIOA
#define SMPS_PG_Pin GPIO_PIN_6
#define SMPS_PG_GPIO_Port GPIOA
#define SMPS_SW_Pin GPIO_PIN_7
#define SMPS_SW_GPIO_Port GPIOA
#define GPIO_SW_BRIDGE_EN_Pin GPIO_PIN_4
#define GPIO_SW_BRIDGE_EN_GPIO_Port GPIOC
#define GPIO_SW_VOLTAGE_EN_Pin GPIO_PIN_0
#define GPIO_SW_VOLTAGE_EN_GPIO_Port GPIOB
#define GPIO_SW_USART1_EN_Pin GPIO_PIN_12
#define GPIO_SW_USART1_EN_GPIO_Port GPIOB
#define LD4_Pin GPIO_PIN_13
#define LD4_GPIO_Port GPIOB
#define EXTI7_I2C1_PEXPB_Pin GPIO_PIN_7
#define EXTI7_I2C1_PEXPB_GPIO_Port GPIOC
#define EXTI8_I2C1_PEXPA_Pin GPIO_PIN_8
#define EXTI8_I2C1_PEXPA_GPIO_Port GPIOC
#define EXTI9_I2C1_ADC_DRDY_Pin GPIO_PIN_9
#define EXTI9_I2C1_ADC_DRDY_GPIO_Port GPIOC
#define USART1_ESP_CTS_Pin GPIO_PIN_11
#define USART1_ESP_CTS_GPIO_Port GPIOA
#define USART1_ESP_RTS_Pin GPIO_PIN_12
#define USART1_ESP_RTS_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define GPIO_SW_SPI3_EN_Pin GPIO_PIN_5
#define GPIO_SW_SPI3_EN_GPIO_Port GPIOB
#define USART1_ESP_TX_Pin GPIO_PIN_6
#define USART1_ESP_TX_GPIO_Port GPIOB
#define USART1_ESP_RX_Pin GPIO_PIN_7
#define USART1_ESP_RX_GPIO_Port GPIOB
#define TIM16_CH1_BUZZER_Pin GPIO_PIN_8
#define TIM16_CH1_BUZZER_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
