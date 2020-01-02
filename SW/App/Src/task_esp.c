/*
 * task_esp.c
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

extern EventGroupHandle_t           globalEventGroupHandle;


void espTaskInit(void)
{

}

void espTaskLoop(void)
{
  osDelay(1000UL);
}
