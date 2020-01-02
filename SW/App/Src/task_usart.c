/*
 * task_UART.c
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */


#ifdef DO_NOT_USE_YET

/* Includes ------------------------------------------------------------------*/

#include <stddef.h>
#include <sys/_stdint.h>
#include <stdio.h>
#include <string.h>

#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "usart.h"

#include "task_Controller.h"

#include <task_usart.h>


/* Variables -----------------------------------------------------------------*/

extern osMessageQId         uartTxQueueHandle;
extern osMessageQId         uartRxQueueHandle;

extern osSemaphoreId        uart_BSemHandle;
extern osSemaphoreId        c2uartTx_BSemHandle;
extern osSemaphoreId        c2uartRx_BSemHandle;

extern EventGroupHandle_t   globalEventGroupHandle;
extern EventGroupHandle_t   uartEventGroupHandle;


volatile uint8_t            g_uartTxDmaBuf[256]                     = { 0U };

volatile uint8_t            g_uartRxDmaBuf[16]                      = { 0U };
volatile uint32_t           g_uartRxDmaBufLast                      = 0UL;
volatile uint32_t           g_uartRxDmaBufIdx                       = 0UL;

static uint8_t              s_uartTx_enable                         = 0U;
static uint32_t             s_uartTxStartTime                       = 0UL;
static osThreadId           s_uartTxPutterTaskHandle                = 0;

static uint8_t              s_uartRx_enable                         = 0U;
static uint32_t             s_uartRxStartTime                       = 0UL;
static osThreadId           s_uartRxGetterTaskHandle                = 0;



/* HAL callbacks for the UART */

/**
  * @brief  Tx Transfer completed callback
  * @param  UartHandle: UART handle.
  * @note   This example shows a simple way to report end of DMA Tx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (UartHandle == &huart4) {
    HAL_CAT_TxCpltCallback(UartHandle);
    return;
  }

  /* Set flags: DMA complete */
  xEventGroupSetBitsFromISR(  uartEventGroupHandle, UART_EG__DMA_TX_RDY, &xHigherPriorityTaskWoken);
  __ISB();

  /* Set flag: buffer empty */
  xEventGroupSetBitsFromISR(  uartEventGroupHandle, UART_EG__TX_BUF_EMPTY, &xHigherPriorityTaskWoken);
}

/**
  * @brief  Rx Transfer completed callback
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report end of DMA Rx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  if (UartHandle == &huart4) {
    HAL_CAT_RxCpltCallback(UartHandle);
    return;
  }

  /* Set flags: DMA complete */
  xEventGroupClearBitsFromISR(uartEventGroupHandle, UART_EG__DMA_RX_RUN);
  xEventGroupSetBitsFromISR(  uartEventGroupHandle, UART_EG__DMA_RX_END, &xHigherPriorityTaskWoken);
}

/**
  * @brief  UART error callbacks
  * @param  UartHandle: UART handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *UartHandle)
{
#if 0
  if (UartHandle == &huart4) {
    HAL_CAT_ErrorCallback(UartHandle);
    return;
  }

  Error_Handler();
#endif
}


/* UART HAL Init */

static void uartHalInit(void)
{
  _Bool already = false;

  /* Block when already called */
  {
    taskENTER_CRITICAL();
    if (s_uartTx_enable || s_uartRx_enable) {
      already = true;
    }
    taskEXIT_CRITICAL();

    if (already) {
      return;
    }
  }

  /* Init UART */
  {
    hlpuart1.Init.BaudRate                = 9600UL;
    hlpuart1.Init.WordLength              = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits                = UART_STOPBITS_1;
    hlpuart1.Init.Parity                  = UART_PARITY_NONE;
    hlpuart1.Init.HwFlowCtl               = UART_HWCONTROL_NONE;
    hlpuart1.Init.Mode                    = UART_MODE_TX_RX;
    hlpuart1.AdvancedInit.AdvFeatureInit  = UART_ADVFEATURE_NO_INIT;

    if(HAL_UART_DeInit(&hlpuart1) != HAL_OK)
    {
      Error_Handler();
    }
    if(HAL_UART_Init(&hlpuart1) != HAL_OK)
    {
      Error_Handler();
    }
  }

}


/* Messaging */

uint32_t uartRxPullFromQueue(uint8_t* msgAry, uint32_t size, uint32_t waitMs)
{
  uint32_t len = 0UL;

  /* Get semaphore to queue out */
  if (osOK != osSemaphoreWait(uart_BSemHandle, 0UL)) {
    return 0UL;
  }

  while (len < (size - 1UL)) {
    osEvent ev = osMessageGet(uartRxQueueHandle, waitMs);
    if (ev.status == osEventMessage) {
      msgAry[len++] = ev.value.v;

    } else {
      break;
    }
  }
  msgAry[len] = 0U;

  /* Return semaphore */
  osSemaphoreRelease(uart_BSemHandle);

  return len;
}


/* UART TX */

static void uartTxPush(const uint8_t* buf, uint32_t len)
{
  xEventGroupClearBits(uartEventGroupHandle, UART_EG__TX_BUF_EMPTY);

  while (len--) {
    /* Do not block on TX queue */
    osMessagePut(uartTxQueueHandle, *(buf++), 1UL);
  }
  osMessagePut(uartTxQueueHandle, 0UL, 1UL);
}

static void uartTxPushWait(const uint8_t* buf, uint32_t len)
{
  /* Buffer is going to be filled */
  EventBits_t eb = xEventGroupWaitBits(uartEventGroupHandle,
      UART_EG__TX_BUF_EMPTY,
      UART_EG__TX_BUF_EMPTY,
      pdFALSE,
      860UL / portTICK_PERIOD_MS);  // One buffer w/ 4096 bytes @ 38.400 baud

  if (eb & UART_EG__TX_BUF_EMPTY) {
    uartTxPush(buf, len);
  }
}

void uartLogLen(const char* str, int len, _Bool doWait)
{
  /* Sanity checks */
  if (!str || !len) {
    return;
  }

  if (s_uartTx_enable) {
    if (osOK == osSemaphoreWait(uart_BSemHandle, 0UL)) {
      if (doWait) {
        uartTxPushWait((uint8_t*)str, len);

      } else {
        uartTxPush((uint8_t*)str, len);
      }

      osSemaphoreRelease(uart_BSemHandle);
    }
  }
}


static void uartTxStartDma(const uint8_t* buf, uint32_t bufLen)
{
  /* Sanity checks */
  if (!buf || !bufLen || bufLen >= sizeof(g_uartTxDmaBuf)) {
    return;
  }

  /* Block until end of transmission */
  xEventGroupWaitBits(uartEventGroupHandle,
      UART_EG__DMA_TX_RDY,
      UART_EG__DMA_TX_RDY,
      pdFALSE,
      portMAX_DELAY);

  /* Be sure that DMA engine is really stopped */
  HAL_UART_AbortTransmit(&hlpuart1);

  /* Copy to DMA TX buffer */
  memcpy((uint8_t*)g_uartTxDmaBuf, buf, bufLen);
  memset((uint8_t*)g_uartTxDmaBuf + bufLen, 0U, sizeof(g_uartTxDmaBuf) - bufLen);

  /* Start transmission */
  if (HAL_OK != HAL_UART_Transmit_DMA(&hlpuart1, (uint8_t*)g_uartTxDmaBuf, bufLen))
  {
    Error_Handler();
  }
}

void uartTxPutterTask(void const * argument)
{
  const uint32_t  maxWaitMs   = RELAY_STILL_TIME;
  uint8_t         buf[ sizeof(g_uartTxDmaBuf) ];

  const uint8_t   lastBuf     = (uint8_t) (sizeof(buf) - 1);

  /* Clear queue */
  while (osMessageGet(uartTxQueueHandle, 1UL).status == osEventMessage) {
  }
  xEventGroupSetBits(uartEventGroupHandle, UART_EG__DMA_TX_RDY);
  xEventGroupSetBits(uartEventGroupHandle, UART_EG__TX_BUF_EMPTY);


  /* TaskLoop */
  for (;;) {
    uint32_t  len     = 0UL;
    uint8_t*  bufPtr  = buf;
    memset(buf, 0U, sizeof(buf));

    for (uint8_t idx = 0U; idx < lastBuf; idx++) {
      osEvent ev = osMessageGet(uartTxQueueHandle, maxWaitMs);
      if (ev.status == osEventMessage) {
        if (ev.value.v) {
          *(bufPtr++) = (uint8_t) ev.value.v;
        }

      } else {
        break;
      }
    }
    len = bufPtr - buf;
    buf[len] = 0U;

    /* Start DMA TX engine */
    if (len) {
      uartTxStartDma(buf, len);
    }
  }
}

static void uartTxInit(void)
{
  /* Init UART HAL */
  uartHalInit();

  /* Start putter thread */
  osThreadDef(uartTxPutterTask, uartTxPutterTask, osPriorityNormal, 0, 256);
  s_uartTxPutterTaskHandle = osThreadCreate(osThread(uartTxPutterTask), NULL);
}

static void uartTxMsgProcess(uint32_t msgLen, const uint32_t* msgAry)
{
  uint32_t            msgIdx  = 0UL;
  const uint32_t      hdr     = msgAry[msgIdx++];
  const UartTxCmds_t  cmd     = (UartTxCmds_t) (0xffUL & hdr);

  switch (cmd) {
  case MsgUartTx__InitDo:
    {
      /* Start at defined point of time */
      const uint32_t delayMs = msgAry[msgIdx++];
      if (delayMs) {
        uint32_t  previousWakeTime = s_uartTxStartTime;
        osDelayUntil(&previousWakeTime, delayMs);
      }

      /* Init module */
      uartTxInit();

      /* Activation flag */
      s_uartTx_enable = 1U;

      /* Return Init confirmation */
      uint32_t cmdBack[1];
      cmdBack[0] = controllerCalcMsgHdr(Destinations__Controller, Destinations__Network_UartTx, 0U, MsgUartTx__InitDone);
      controllerMsgPushToInQueue(sizeof(cmdBack) / sizeof(int32_t), cmdBack, 10UL);
    }
    break;

  default: { }
  }  // switch (cmd)
}

void uartTxTaskInit(void)
{
  /* Wait until controller is up */
  xEventGroupWaitBits(globalEventGroupHandle,
      EG_GLOBAL__Controller_CTRL_IS_RUNNING,
      0UL,
      0, portMAX_DELAY);

  /* Store start time */
  s_uartTxStartTime = osKernelSysTick();

  /* Give other tasks time to do the same */
  osDelay(10UL);
}

void uartTxTaskLoop(void)
{
  uint32_t  msgLen                        = 0UL;
  uint32_t  msgAry[CONTROLLER_MSG_Q_LEN];

  /* Wait for door bell and hand-over controller out queue */
  {
    osSemaphoreWait(c2uartTx_BSemHandle, osWaitForever);
    msgLen = controllerMsgPullFromOutQueue(msgAry, Destinations__Network_UartTx, 1UL);                  // Special case of callbacks need to limit blocking time
  }

  /* Decode and execute the commands when a message exists
   * (in case of callbacks the loop catches its wakeup semaphore
   * before ctrlQout is released results to request on an empty queue) */
  if (msgLen) {
    uartTxMsgProcess(msgLen, msgAry);
  }
}


/* UART RX */

static void uartRxStartDma(void)
{
  const uint16_t dmaBufSize = sizeof(g_uartRxDmaBuf);

  /* Clear DMA buffer */
  memset((char*) g_uartRxDmaBuf, 0, sizeof(g_uartRxDmaBuf));

  /* Reset working indexes */
  g_uartRxDmaBufLast = g_uartRxDmaBufIdx = 0UL;

  xEventGroupClearBits(uartEventGroupHandle, UART_EG__DMA_RX_END);

  /* Start RX DMA after aborting the previous one */
  if (HAL_UART_Receive_DMA(&hlpuart1, (uint8_t*)g_uartRxDmaBuf, dmaBufSize) != HAL_OK)
  {
    //Error_Handler();
  }

  /* Set RX running flag */
  xEventGroupSetBits(uartEventGroupHandle, UART_EG__DMA_RX_RUN);
}


void uartRxGetterTask(void const * argument)
{
  const uint8_t nulBuf[1]   = { 0U };
  const uint32_t maxWaitMs  = 25UL;

  /* TaskLoop */
  for (;;) {
    /* Find last written byte */
    g_uartRxDmaBufIdx = g_uartRxDmaBufLast + strnlen((char*)g_uartRxDmaBuf + g_uartRxDmaBufLast, sizeof(g_uartRxDmaBuf) - g_uartRxDmaBufLast);

    /* Send new character in RX buffer to the queue */
    if (g_uartRxDmaBufIdx > g_uartRxDmaBufLast) {
      /* From UART data into the buffer */
      const uint32_t    l_uartRxDmaBufIdx = g_uartRxDmaBufIdx;
      volatile uint8_t* bufPtr            = g_uartRxDmaBuf + g_uartRxDmaBufLast;

      for (int32_t idx = g_uartRxDmaBufLast + 1U; idx <= l_uartRxDmaBufIdx; ++idx, ++bufPtr) {
        xQueueSendToBack(uartRxQueueHandle, (uint8_t*)bufPtr, maxWaitMs);
      }
      xQueueSendToBack(uartRxQueueHandle, nulBuf, maxWaitMs);

      g_uartRxDmaBufLast = l_uartRxDmaBufIdx;

      /* Restart DMA if transfer has finished */
      if (UART_EG__DMA_RX_END & xEventGroupGetBits(uartEventGroupHandle)) {
        /* Reactivate UART RX DMA transfer */
        uartRxStartDma();
      }

    } else {
      /* Restart DMA if transfer has finished */
      if (UART_EG__DMA_RX_END & xEventGroupGetBits(uartEventGroupHandle)) {
        /* Reactivate UART RX DMA transfer */
        uartRxStartDma();
      }

      /* Delay for the next attempt */
      osDelay(25UL);
    }
  }
}

static void uartRxInit(void)
{
  /* Init UART HAL */
  uartHalInit();

  /* Start getter thread */
  osThreadDef(uartRxGetterTask, uartRxGetterTask, osPriorityHigh, 0, 128);
  s_uartRxGetterTaskHandle = osThreadCreate(osThread(uartRxGetterTask), NULL);

  /* Activate UART RX DMA transfer */
  if (HAL_UART_Receive_DMA(&hlpuart1, (uint8_t*) g_uartRxDmaBuf, sizeof(g_uartRxDmaBuf) - 1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void uartRxMsgProcess(uint32_t msgLen, const uint32_t* msgAry)
{
  uint32_t            msgIdx  = 0UL;
  const uint32_t      hdr     = msgAry[msgIdx++];
  const UartRxCmds_t  cmd     = (UartRxCmds_t) (0xffUL & hdr);

  switch (cmd) {
  case MsgUartRx__InitDo:
    {
      /* Start at defined point of time */
      const uint32_t delayMs = msgAry[msgIdx++];
      if (delayMs) {
        uint32_t  previousWakeTime = s_uartRxStartTime;
        osDelayUntil(&previousWakeTime, delayMs);
      }

      /* Init module */
      uartRxInit();

      /* Activation flag */
      s_uartRx_enable = 1U;

      /* Return Init confirmation */
      uint32_t cmdBack[1];
      cmdBack[0] = controllerCalcMsgHdr(Destinations__Controller, Destinations__Network_UartRx, 0U, MsgUartRx__InitDone);
      controllerMsgPushToInQueue(sizeof(cmdBack) / sizeof(int32_t), cmdBack, 10UL);
    }
    break;

  default: { }
  }  // switch (cmd)
}

void uartRxTaskInit(void)
{
  /* Wait until controller is up */
  xEventGroupWaitBits(globalEventGroupHandle,
      EG_GLOBAL__Controller_CTRL_IS_RUNNING,
      0UL,
      0, portMAX_DELAY);

  /* Store start time */
  s_uartRxStartTime = osKernelSysTick();

  /* Give other tasks time to do the same */
  osDelay(10UL);
}

void uartRxTaskLoop(void)
{
  uint32_t  msgLen                        = 0UL;
  uint32_t  msgAry[CONTROLLER_MSG_Q_LEN];

  /* Wait for door bell and hand-over controller out queue */
  {
    osSemaphoreWait(c2uartRx_BSemHandle, osWaitForever);
    msgLen = controllerMsgPullFromOutQueue(msgAry, Destinations__Network_UartRx, 1UL);                // Special case of callbacks need to limit blocking time
  }

  /* Decode and execute the commands when a message exists
   * (in case of callbacks the loop catches its wakeup semaphore
   * before ctrlQout is released results to request on an empty queue) */
  if (msgLen) {
    uartRxMsgProcess(msgLen, msgAry);
  }
}

#endif
