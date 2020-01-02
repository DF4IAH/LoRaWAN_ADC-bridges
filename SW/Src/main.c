/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "rng.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32l4xx_hal_pwr_ex.h"
#include "device_portexp.h"
#include "device_fram.h"
#include "device_sx1261.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern TIM_HandleTypeDef              htim2;

uint32_t                              g_RTC_WUT_value         = 2048UL * 5UL;  // Wakeup every 5 secs

ENABLE_MASK_t                         g_enableMsk             = ENABLE_MASK__LORAWAN_DEVICE;
MON_MASK_t                            g_monMsk                = MON_MASK__LORA;

static uint64_t                       s_timerLast_us          = 0ULL;
static uint64_t                       s_timerStart_us         = 0ULL;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

inline
uint8_t GET_BYTE_OF_WORD(uint32_t word, uint8_t pos)
{
  return (uint8_t) ((word >> (8 * pos)) & 0x000000ffUL);
}

void mainCalc_Float2Int(float in, uint32_t* out_i, uint16_t* out_p1000)
{
  if (in < 0) {
    in = -in;
  }

  *out_i      = (uint32_t) in;
  *out_p1000  = (uint16_t) (((uint32_t) (in * 1000.f)) % 1000);
}


uint32_t mainGetRand(void)
{
  uint32_t r;

  __HAL_RCC_RNG_CLK_ENABLE();
  osDelay(2UL);
  HAL_RNG_GenerateRandomNumber(&hrng, &r);
  __HAL_RCC_RNG_CLK_DISABLE();

  return r;
}

/* Used by the run-time stats */
void configureTimerForRunTimeStats(void)
{
  getRunTimeCounterValue();

  /* Interrupt disabled block */
  {
    __disable_irq();

    s_timerStart_us = s_timerLast_us;

    __enable_irq();
  }
}

/* Used by the run-time stats */
unsigned long getRunTimeCounterValue(void)
{
  uint64_t l_timerStart_us = 0ULL;
  uint64_t l_timer_us = HAL_GetTick() & 0x003fffffUL;                                                   // avoid overflows

  /* Add microseconds */
  l_timer_us *= 1000ULL;
  l_timer_us += TIM2->CNT % 1000UL;                                                                     // TIM2 counts microseconds

  /* Interrupt disabled block */
  {
    __disable_irq();

    s_timerLast_us  = l_timer_us;
    l_timerStart_us = s_timerStart_us;

    __enable_irq();
  }

  uint64_t l_timerDiff64 = (l_timer_us >= l_timerStart_us) ?  (l_timer_us - l_timerStart_us) : l_timer_us;
  uint32_t l_timerDiff32 = (uint32_t) (l_timerDiff64 & 0xffffffffULL);
  return l_timerDiff32;
}

void mainInitMxMinimal(void)
{
  MX_GPIO_Init();
  MX_GPIO_Init_WKUP_REED();
  MX_DMA_Init();

  #ifdef MAIN_INIT_MX_RTC
  MX_RTC_Init();
  #endif

  #ifdef MAIN_INIT_MX_TIM3
  MX_TIM3_Init();
  #endif

  #ifdef MAIN_INIT_MX_TIM16
  MX_TIM16_Init();
  #endif


  #ifdef MAIN_INIT_MX_LPUART1
  MX_LPUART1_UART_Init();
  #endif

  #ifdef MAIN_INIT_MX_USART1
  MX_USART1_UART_Init();
  #endif


  #ifdef MAIN_INIT_MX_I2C1
  MX_I2C1_Init();
  #endif

  #ifdef MAIN_INIT_MX_I2C2
  MX_I2C2_Init();
  #endif

  #ifdef MAIN_INIT_MX_SPI3
  MX_SPI3_Init();
  #endif

  #ifdef MAIN_INIT_MX_ADC1
  MX_ADC1_Init();
  #endif

  #ifdef MAIN_INIT_MX_CRC
  MX_CRC_Init();
  __HAL_RCC_CRC_CLK_DISABLE();
  #endif

  #ifdef MAIN_INIT_MX_RNG
  MX_RNG_Init();
  __HAL_RCC_RNG_CLK_DISABLE();
  #endif
}

void mainDeInitMxMinimal(void)
{
  /* Keeping SX1261 in reset state */
  sxReset(true);

  /* Shutdown everything that is possible for the deep sleep taking minutes */
  #ifdef MAIN_DEINIT_MX_RNG
  HAL_RNG_DeInit(&hrng);
  #endif

  #ifdef MAIN_DEINIT_MX_CRC
  HAL_CRC_DeInit(&hcrc);
  #endif

  #ifdef MAIN_DEINIT_MX_ADC1
  HAL_ADC_DeInit(&hadc1);
  #endif

  #ifdef MAIN_DEINIT_MX_I2C1
  HAL_I2C_MspDeInit(&hi2c1);
  #endif

  #ifdef MAIN_DEINIT_MX_I2C2
  HAL_I2C_MspDeInit(&hi2c2);
  #endif

  #ifdef MAIN_DEINIT_MX_SPI3
  HAL_SPI_MspDeInit(&hspi3);
  #endif


  #ifdef MAIN_DEINIT_MX_LPUART1
  HAL_UART_MspDeInit(&hlpuart1);
  #endif

  #ifdef MAIN_DEINIT_MX_USART1
  HAL_UART_MspDeInit(&huart1);
  #endif


  #ifdef MAIN_DEINIT_MX_TIM3
  HAL_TIM_Base_MspDeInit(&htim3);
  #endif

  #ifdef MAIN_DEINIT_MX_TIM16
  HAL_TIM_Base_MspDeInit(&htim16);
  #endif


  #ifdef MAIN_DEINIT_MX_RTC
  HAL_RTC_MspDeInit(&hrtc);
  #endif


  MX_DMA_DeInit();

  MX_GPIO_DeInit();
}

void mainGotoDeepSleep(void)
{
   /* Prepare for deep sleep */
  mainDeInitMxMinimal();

  /* PU / PD during Standby and Shutdown modes */
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_5);     // GPIO_SW_SPI3_EN_GPIO_Port,     GPIO_SW_SPI3_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_8);     // GPIO_SW_I2C1_EN_GPIO_Port,     GPIO_SW_I2C1_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_14);    // GPIO_SMPS_VDD12_EN_GPIO_Port,  GPIO_SMPS_VDD12_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_15);    // GPIO_SW_VDD12_EN_GPIO_Port,    GPIO_SW_VDD12_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_12);    // GPIO_SW_USART1_EN_GPIO_Port,   GPIO_SW_USART1_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_4);     // GPIO_SW_BRIDGE_EN_GPIO_Port,   GPIO_SW_BRIDGE_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_B, PWR_GPIO_BIT_0);     // GPIO_SW_VOLTAGE_EN_GPIO_Port,  GPIO_SW_VOLTAGE_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_H, PWR_GPIO_BIT_0);     // GPIO_MUXG0_EN_GPIO_Port,       GPIO_MUXG0_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_H, PWR_GPIO_BIT_1);     // GPIO_MUXG1_EN_GPIO_Port,       GPIO_MUXG1_EN_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_13);    // BOARD_WKUP_GPIO_Port,          BOARD_WKUP_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_A, PWR_GPIO_BIT_0);     // REED_WKUP_GPIO_Port,           REED_WKUP_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_2);     // GPIO_EXTI2_EXPCON_GPIO_Port,   GPIO_EXTI2_EXPCON_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_3);     // GPIO_EXTI3_EXPCON_GPIO_Port,   GPIO_EXTI3_EXPCON_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_7);     // EXTI7_I2C1_PEXPB_GPIO_Port,    EXTI7_I2C1_PEXPB_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_8);     // EXTI8_I2C1_PEXPA_GPIO_Port,    EXTI8_I2C1_PEXPA_Pin
  HAL_PWREx_EnableGPIOPullDown(PWR_GPIO_C, PWR_GPIO_BIT_9);     // EXTI9_I2C1_ADC_DRDY_GPIO_Port, EXTI9_I2C1_ADC_DRDY_Pin
  HAL_PWREx_EnablePullUpPullDownConfig();

  /* Enable WKUP pins */
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN2_HIGH);                // REED_WKUP
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_HIGH);                // BOARD_WKUP

  /* Enter deep sleep */
  HAL_PWREx_EnterSHUTDOWNMode();
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* Check for the reason to enter this main() again */
  if (!(RCC->CSR & 0xff000000UL)) {
    /* Wake-up event is triggered */

    /* Init to be able to access the RTC backup registers */
    HAL_Init();
    SystemClock_Config();
    mainInitMxMinimal();

    /* Counter in the backup-domain to be incremented */
    uint32_t counter = HAL_RTCEx_BKUPRead(&hrtc, 31UL);
    HAL_RTCEx_BKUPWrite(&hrtc, 31UL, ++counter);

    /* ARM software reset to be done */
    HAL_NVIC_SystemReset();
  }

  /* Clear the reason flags */
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /* USER CODE END 1 */
  

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  #ifdef DO_INIT_ALL
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_LPUART1_UART_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  MX_SPI3_Init();
  MX_TIM3_Init();
  MX_TIM16_Init();
  MX_CRC_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */
  #endif
  mainInitMxMinimal();

  /* Enable clock capacitor charger as soon as possible */
  HAL_PWREx_EnableBatteryCharging(PWR_BATTERY_CHARGING_RESISTOR_1_5);

  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init(); 

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability 
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_8;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_I2C2|RCC_PERIPHCLK_RNG
                              |RCC_PERIPHCLK_ADC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_HSI;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c2ClockSelection = RCC_I2C2CLKSOURCE_PCLK1;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_MSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCCEx_EnableLSCO(RCC_LSCOSOURCE_LSE);
  /** Configure the main internal regulator output voltage 
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable MSI Auto calibration 
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/* USER CODE BEGIN 4 */
void  vApplicationIdleHook(void)
{
  /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
  to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
  task. It is essential that code added to this hook function never attempts
  to block in any way (for example, call xQueueReceive() with a block time
  specified, or call vTaskDelay()). If the application makes use of the
  vTaskDelete() API function (as this demo application does) then it is also
  important that vApplicationIdleHook() is permitted to return to its calling
  function, because it is the responsibility of the idle task to clean up
  memory allocated by the kernel to any task that has since been deleted. */

  /* Enter sleep mode */
  __asm volatile( "WFI" );
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* Store error code */
  // TODO DF4IAH - coding needed here

  /* Switch off to be able to restart next interval */
  mainGotoDeepSleep();

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  __NOP();
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
