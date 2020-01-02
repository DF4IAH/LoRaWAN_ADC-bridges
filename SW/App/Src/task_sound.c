/*
 * task_sound.c
 *
 *  Created on: 14.06.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stddef.h>
#include <sys/_stdint.h>
#include <stdio.h>

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"

#include "main.h"

#include "task_sound.h"


/* Variables -----------------------------------------------------------------*/

extern osMessageQId                   soundInQueueHandle;
extern TIM_HandleTypeDef              htim16;


void soundTaskInit(void)
{
  /* Stop any sound */
  uint32_t pitch = 0UL;
  xQueueSend(soundInQueueHandle, &pitch, 1UL);
}

void soundTaskLoop(void)
{
  /* Pitch in mHz */
  uint32_t pitch = 0UL;

  if (xQueueReceive(soundInQueueHandle, &pitch, 0) == pdPASS) {
    if (!pitch) {
      /* Turn of sound */
      HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);

    } else {
      /* Turn on sound at pitch */
      const uint32_t tmrVal = (uint32_t) (0.5f + 16e9 / pitch);

      /* Compute the prescaler value to have TIM1 counter clock equal to 16000000 Hz */
      uint32_t uhPrescalerValue = (uint32_t)(SystemCoreClock / 16000000) - 1;

      /* Turn of sound */
      HAL_TIM_PWM_Stop(&htim16, TIM_CHANNEL_1);


      TIM_HandleTypeDef TimHandle;
      TimHandle.Instance                = TIM16;
      TimHandle.Init.Prescaler          = uhPrescalerValue;
      TimHandle.Init.Period             = tmrVal;
      TimHandle.Init.ClockDivision      = 0;
      TimHandle.Init.CounterMode        = TIM_COUNTERMODE_UP;
      TimHandle.Init.RepetitionCounter  = 0;
      if (HAL_TIM_PWM_Init(&TimHandle) != HAL_OK)
      {
        /* Initialization Error */
        Error_Handler();
      }

      TIM_OC_InitTypeDef sConfig;
      sConfig.OCMode                    = TIM_OCMODE_PWM1;
      sConfig.OCPolarity                = TIM_OCPOLARITY_HIGH;
      sConfig.OCFastMode                = TIM_OCFAST_DISABLE;
      sConfig.OCNPolarity               = TIM_OCNPOLARITY_HIGH;
      sConfig.OCNIdleState              = TIM_OCNIDLESTATE_RESET;
      sConfig.OCIdleState               = TIM_OCIDLESTATE_RESET;
      sConfig.Pulse                     = tmrVal / 2;

      if (HAL_TIM_PWM_ConfigChannel(&htim16, &sConfig, TIM_CHANNEL_1) != HAL_OK)
      {
        /* Configuration Error */
        Error_Handler();
      }

      HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
    }
  }

  /* Speed of melody */
  osDelay(300UL);
}
