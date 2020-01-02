/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include <string.h>

#include "i2c.h"
#include "spi.h"
#include "bus_i2c.h"
#include "bus_spi.h"
#include "device_fram.h"
#include "device_portexp.h"
#include "device_sx1261.h"
#include "task_esp.h"
#include "task_adc.h"
#include "task_sound.h"
#include "task_LoRaWAN.h"

#ifdef GNSS_EXTRA
# include "task_gnss.h"
#endif

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
/* USER CODE BEGIN Variables */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern DMA_HandleTypeDef hdma_i2c1_tx;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c2_tx;
extern DMA_HandleTypeDef hdma_i2c2_rx;
extern SPI_HandleTypeDef hspi3;
extern DMA_HandleTypeDef hdma_spi3_tx;
extern DMA_HandleTypeDef hdma_spi3_rx;
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_lpuart_tx;
extern DMA_HandleTypeDef hdma_lpuart_rx;

extern volatile uint8_t  spi3RxBuffer[SPI3_BUFFERSIZE];

extern volatile uint8_t  g_spi3SelBno;
extern volatile uint8_t  g_spi3SelSx;

extern IoT4BeesCtrlApp_up_t         g_Iot4BeesApp_up;
extern IoT4BeesCtrlApp_down_t       g_Iot4BeesApp_down;


EventGroupHandle_t globalEventGroupHandle;
EventGroupHandle_t controllerEventGroupHandle;
EventGroupHandle_t extiEventGroupHandle;
EventGroupHandle_t loraEventGroupHandle;
EventGroupHandle_t spiEventGroupHandle;
EventGroupHandle_t adcEventGroupHandle;

#ifdef GNSS_EXTRA
osThreadId gnssTaskHandle;
osMutexId gnssCtxMutexHandle;
osMessageQId gnssRxQueueHandle;

extern uint8_t                      g_gnss_reset;
#endif

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId usart1TxTaskHandle;
osThreadId usart1RxTaskHandle;
osThreadId lorawanTaskHandle;
osThreadId espTaskHandle;
osThreadId adcTaskHandle;
osThreadId soundTaskHandle;
osMessageQId usart1TxQueueHandle;
osMessageQId usart1RxQueueHandle;
osMessageQId loraInQueueHandle;
osMessageQId loraOutQueueHandle;
osMessageQId loraMacQueueHandle;
osMessageQId soundInQueueHandle;
osMutexId trackMeApplUpDataMutexHandle;
osMutexId trackMeApplDnDataMutexHandle;
osMutexId iot4BeesApplUpDataMutexHandle;
osMutexId iot4BeesApplDnDataMutexHandle;
osSemaphoreId i2c1_BSemHandle;
osSemaphoreId i2c2_BSemHandle;
osSemaphoreId spi3_BSemHandle;
osSemaphoreId usart1Tx_BSemHandle;
osSemaphoreId usart1Rx_BSemHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartUsart1TxTask(void const * argument);
void StartUsart1RxTask(void const * argument);
void StartLorawanTask(void const * argument);
void StartEspTask(void const * argument);
void StartAdcTask(void const * argument);
void StartSoundTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
return 0;
}
/* USER CODE END 1 */

/* USER CODE BEGIN 2 */
__weak void vApplicationIdleHook( void )
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
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* Create the mutex(es) */
  /* definition and creation of trackMeApplUpDataMutex */
  osMutexDef(trackMeApplUpDataMutex);
  trackMeApplUpDataMutexHandle = osMutexCreate(osMutex(trackMeApplUpDataMutex));

  /* definition and creation of trackMeApplDnDataMutex */
  osMutexDef(trackMeApplDnDataMutex);
  trackMeApplDnDataMutexHandle = osMutexCreate(osMutex(trackMeApplDnDataMutex));

  /* definition and creation of iot4BeesApplUpDataMutex */
  osMutexDef(iot4BeesApplUpDataMutex);
  iot4BeesApplUpDataMutexHandle = osMutexCreate(osMutex(iot4BeesApplUpDataMutex));

  /* definition and creation of iot4BeesApplDnDataMutex */
  osMutexDef(iot4BeesApplDnDataMutex);
  iot4BeesApplDnDataMutexHandle = osMutexCreate(osMutex(iot4BeesApplDnDataMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
#ifdef GNSS_EXTRA
  /* definition and creation of gnssCtxMutex */
  osMutexDef(gnssCtxMutex);
  gnssCtxMutexHandle = osMutexCreate(osMutex(gnssCtxMutex));
#endif
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of i2c1_BSem */
  osSemaphoreDef(i2c1_BSem);
  i2c1_BSemHandle = osSemaphoreCreate(osSemaphore(i2c1_BSem), 1);

  /* definition and creation of i2c2_BSem */
  osSemaphoreDef(i2c2_BSem);
  i2c2_BSemHandle = osSemaphoreCreate(osSemaphore(i2c2_BSem), 1);

  /* definition and creation of spi3_BSem */
  osSemaphoreDef(spi3_BSem);
  spi3_BSemHandle = osSemaphoreCreate(osSemaphore(spi3_BSem), 1);

  /* definition and creation of usart1Tx_BSem */
  osSemaphoreDef(usart1Tx_BSem);
  usart1Tx_BSemHandle = osSemaphoreCreate(osSemaphore(usart1Tx_BSem), 1);

  /* definition and creation of usart1Rx_BSem */
  osSemaphoreDef(usart1Rx_BSem);
  usart1Rx_BSemHandle = osSemaphoreCreate(osSemaphore(usart1Rx_BSem), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  globalEventGroupHandle = xEventGroupCreate();
  controllerEventGroupHandle = xEventGroupCreate();
  extiEventGroupHandle = xEventGroupCreate();
  spiEventGroupHandle = xEventGroupCreate();
  adcEventGroupHandle = xEventGroupCreate();
  loraEventGroupHandle = xEventGroupCreate();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* definition and creation of usart1TxQueue */
  osMessageQDef(usart1TxQueue, 256, uint8_t);
  usart1TxQueueHandle = osMessageCreate(osMessageQ(usart1TxQueue), NULL);

  /* definition and creation of usart1RxQueue */
  osMessageQDef(usart1RxQueue, 32, uint8_t);
  usart1RxQueueHandle = osMessageCreate(osMessageQ(usart1RxQueue), NULL);

  /* definition and creation of loraInQueue */
  osMessageQDef(loraInQueue, 32, uint8_t);
  loraInQueueHandle = osMessageCreate(osMessageQ(loraInQueue), NULL);

  /* definition and creation of loraOutQueue */
  osMessageQDef(loraOutQueue, 32, uint8_t);
  loraOutQueueHandle = osMessageCreate(osMessageQ(loraOutQueue), NULL);

  /* definition and creation of loraMacQueue */
  osMessageQDef(loraMacQueue, 8, uint8_t);
  loraMacQueueHandle = osMessageCreate(osMessageQ(loraMacQueue), NULL);

  /* definition and creation of soundInQueue */
  osMessageQDef(soundInQueue, 8, uint32_t);
  soundInQueueHandle = osMessageCreate(osMessageQ(soundInQueue), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
#ifdef GNSS_EXTRA
  /* definition and creation of soundInQueue */
  osMessageQDef(gnssRxQueue, 128, uint8_t);
  gnssRxQueueHandle = osMessageCreate(osMessageQ(gnssRxQueue), NULL);
#endif
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityBelowNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of usart1TxTask */
  osThreadDef(usart1TxTask, StartUsart1TxTask, osPriorityNormal, 0, 256);
  usart1TxTaskHandle = osThreadCreate(osThread(usart1TxTask), NULL);

  /* definition and creation of usart1RxTask */
  osThreadDef(usart1RxTask, StartUsart1RxTask, osPriorityAboveNormal, 0, 256);
  usart1RxTaskHandle = osThreadCreate(osThread(usart1RxTask), NULL);

  /* definition and creation of lorawanTask */
  osThreadDef(lorawanTask, StartLorawanTask, osPriorityNormal, 0, 512);
  lorawanTaskHandle = osThreadCreate(osThread(lorawanTask), NULL);

  /* definition and creation of espTask */
  osThreadDef(espTask, StartEspTask, osPriorityNormal, 0, 512);
  espTaskHandle = osThreadCreate(osThread(espTask), NULL);

  /* definition and creation of adcTask */
  osThreadDef(adcTask, StartAdcTask, osPriorityNormal, 0, 512);
  adcTaskHandle = osThreadCreate(osThread(adcTask), NULL);

  /* definition and creation of soundTask */
  osThreadDef(soundTask, StartSoundTask, osPriorityLow, 0, 256);
  soundTaskHandle = osThreadCreate(osThread(soundTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
#ifdef GNSS_EXTRA
  osThreadDef(gnssTask, StartGnssTask, osPriorityNormal, 0, 512);
  gnssTaskHandle = osThreadCreate(osThread(gnssTask), NULL);
#endif

  /* add to registry */
  vQueueAddToRegistry(usart1TxQueueHandle,          "Q usarTx");
  vQueueAddToRegistry(usart1RxQueueHandle,          "Q usarRx");
  vQueueAddToRegistry(loraInQueueHandle,            "Q LoRaIn");
  vQueueAddToRegistry(loraOutQueueHandle,           "Q LoRaOut");
  vQueueAddToRegistry(loraMacQueueHandle,           "Q LoRaMac");
  vQueueAddToRegistry(soundInQueueHandle,           "Q soundIn");
//vQueueAddToRegistry(lpuart1TxQueueHandle,         "Q lpuarTx");
//vQueueAddToRegistry(lpuart1RxQueueHandle,         "Q lpuarRx");
//vQueueAddToRegistry(controllerInQueueHandle,      "Q ctrlIn");
//vQueueAddToRegistry(controllerOutQueueHandle,     "Q ctrlOut");
#ifdef GNSS_EXTRA
  vQueueAddToRegistry(gnssRxQueueHandle,            "Q gnssRx");
#endif

  vQueueAddToRegistry(usart1Tx_BSemHandle,          "Rs usartT");
  vQueueAddToRegistry(usart1Rx_BSemHandle,          "Rs usartR");
  vQueueAddToRegistry(i2c1_BSemHandle,              "Rs I2C1");
  vQueueAddToRegistry(i2c2_BSemHandle,              "Rs I2C2");
  vQueueAddToRegistry(spi3_BSemHandle,              "Rs SPI3");
//vQueueAddToRegistry(lpuart1Tx_BSemHandle,         "Rs lpuarT");
//vQueueAddToRegistry(lpuart1Rx_BSemHandle,         "Rs lpuarR");

//vQueueAddToRegistry(cQin_BSemHandle,              "Rs cQin");
//vQueueAddToRegistry(cQout_BSemHandle,             "Rs cQout");

//vQueueAddToRegistry(c2default_BSemHandle,         "Wk c2dflt");
//vQueueAddToRegistry(c2interpreter_BSemHandle,     "Wk c2intr");
//vQueueAddToRegistry(c2usart1Tx_BSemHandle,        "Wk c2usaT");
//vQueueAddToRegistry(c2usart1Rx_BSemHandle,        "Wk c2usaR");
//vQueueAddToRegistry(c2lpuart1Tx_BSemHandle,       "Wk c2lpuT");
//vQueueAddToRegistry(c2lpuart1Rx_BSemHandle,       "Wk c2lpuR");

  vQueueAddToRegistry(trackMeApplUpDataMutexHandle, "Mx LoRaUp");
  vQueueAddToRegistry(trackMeApplDnDataMutexHandle, "Mx LoRaDn");
#ifdef GNSS_EXTRA
  vQueueAddToRegistry(gnssCtxMutexHandle,           "Mx gnss");
#endif

  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used 
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN StartDefaultTask */
  /* Resonancy of piezoelectric speaker @ 600 Hz +/- 300 Hz */
  const uint32_t pitch          = 635000UL;
  const uint32_t silence        = 0UL;
  TickType_t previousWakeTime   = 0UL;

  /* Check if REED_WKUP is active @ 0sec */
  if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REED_WKUP_GPIO_Port, REED_WKUP_Pin)) {
    /* 1x Beep */
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);

    /* Inform */
    xEventGroupSetBits(globalEventGroupHandle, Global_EGW__REEDWKUP_00SEC);
  }

  /* I2C1 */
  i2cx_Init(&hi2c1, i2c1_BSemHandle);

//#define TEST_I2C1_ADDR_SCAN
#ifdef  TEST_I2C1_ADDR_SCAN
  /* Wait until I2C is ready */
  xEventGroupWaitBits(globalEventGroupHandle, Global_EGW__I2C_RDY, 0UL, pdFALSE, portMAX_DELAY);

  i2cBusAddrScan(&hi2c1, i2c1_BSemHandle);

  while (1) {
    osDelay(1000UL);
  }
#endif

  /* Check if REED_WKUP is active @ 5sec */
  osDelayUntil(&previousWakeTime,  5000UL);
  if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REED_WKUP_GPIO_Port, REED_WKUP_Pin)) {
    /* 2x Beep */
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);

    /* Inform */
    xEventGroupSetBits(globalEventGroupHandle, Global_EGW__REEDWKUP_05SEC);
  }

  /* Check if REED_WKUP is active @ 10sec */
  osDelayUntil(&previousWakeTime, 5000UL);
  if (GPIO_PIN_SET == HAL_GPIO_ReadPin(REED_WKUP_GPIO_Port, REED_WKUP_Pin)) {
    /* 3x Beep */
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);
    xQueueSend(soundInQueueHandle, &pitch, 1UL);
    xQueueSend(soundInQueueHandle, &silence, 1UL);

    /* Inform */
    xEventGroupSetBits(globalEventGroupHandle, Global_EGW__REEDWKUP_10SEC);
  }


#ifdef POWEROFF_AFTER_TXRX

#ifdef GNSS_EXTRA
  /* Reset GNSS chip @ 3 minutes */
  osDelayUntil(&previousWakeTime, 180UL * 1000UL);
  g_gnss_reset = 1U;

  /* Hard switch-off @ 3 minutes + 10 Sec */
  osDelayUntil(&previousWakeTime, 190UL * 1000UL);

#else
  /* Hard switch-off after 30 seconds */
  osDelayUntil(&previousWakeTime, 30000UL);
#endif
  mainGotoDeepSleep();

#else

  for (;;) {
    osDelay(8 * 60000UL);
  }

#endif

  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartUsart1TxTask */
/**
* @brief Function implementing the usart1TxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUsart1TxTask */
void StartUsart1TxTask(void const * argument)
{
  /* USER CODE BEGIN StartUsart1TxTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(300000UL);
  }
  /* USER CODE END StartUsart1TxTask */
}

/* USER CODE BEGIN Header_StartUsart1RxTask */
/**
* @brief Function implementing the usart1RxTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUsart1RxTask */
void StartUsart1RxTask(void const * argument)
{
  /* USER CODE BEGIN StartUsart1RxTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(300000UL);
  }
  /* USER CODE END StartUsart1RxTask */
}

/* USER CODE BEGIN Header_StartLorawanTask */
/**
* @brief Function implementing the lorawanTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLorawanTask */
void StartLorawanTask(void const * argument)
{
  /* USER CODE BEGIN StartLorawanTask */
  loRaWANLoraTaskInit();

  /* Infinite loop */
  for(;;)
  {
    loRaWANLoraTaskLoop();
  }
  /* USER CODE END StartLorawanTask */
}

/* USER CODE BEGIN Header_StartEspTask */
/**
* @brief Function implementing the espTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartEspTask */
void StartEspTask(void const * argument)
{
  /* USER CODE BEGIN StartEspTask */
  espTaskInit();

  /* Infinite loop */
  for(;;)
  {
    espTaskLoop();
  }
  /* USER CODE END StartEspTask */
}

/* USER CODE BEGIN Header_StartAdcTask */
/**
* @brief Function implementing the adcTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAdcTask */
void StartAdcTask(void const * argument)
{
  /* USER CODE BEGIN StartAdcTask */
  adcTaskInit();

  /* Infinite loop */
  for(;;)
  {
    adcTaskLoop();
  }
  /* USER CODE END StartAdcTask */
}

/* USER CODE BEGIN Header_StartSoundTask */
/**
* @brief Function implementing the soundTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSoundTask */
void StartSoundTask(void const * argument)
{
  /* USER CODE BEGIN StartSoundTask */
  soundTaskInit();

  /* Infinite loop */
  for(;;)
  {
    soundTaskLoop();
  }
  /* USER CODE END StartSoundTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
#ifdef GNSS_EXTRA
void StartGnssTask(void const * argument)
{
  gnssTaskInit();

  /* Infinite loop */
  for(;;)
  {
    gnssTaskLoop();
  }
}
#endif
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
