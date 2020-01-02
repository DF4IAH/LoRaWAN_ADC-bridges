/*
 * device_sx1261.c
 *
 *  Created on: 06.05.2019
 *      Author: DF4IAH
 */


/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32l4xx_hal.h"
#include "stm32l4xx_it.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"

#include "main.h"
#include "bus_spi.h"
#include "device_fram.h"
#include "device_portexp.h"
#include "device_sx1261.h"


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t           extiEventGroupHandle;

extern Fram1F_Template_t            g_framShadow1F;


extern volatile uint8_t             spi3TxBuffer[SPI3_BUFFERSIZE];
extern volatile uint8_t             spi3RxBuffer[SPI3_BUFFERSIZE];

uint32_t                            tmrsBuf[16] = { 0UL };
uint8_t                             tmrsIdx = 0U;


#if 0
static void sxStoreTimestamp(uint32_t ts)
{
  static uint32_t endOfTx = 0UL;

  if (!endOfTx) {
    tmrsBuf[tmrsIdx++ & 0x0f] = ts;
    endOfTx = ts;

  } else {
    tmrsBuf[tmrsIdx++ & 0x0f] = ts - endOfTx;
  }
}
#endif


void sxReset(bool keep)
{
  /* Do a RESET of the SX1261 */
  portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_SX_NRESET, GPIO_PIN_RESET);

  /* Keep or start */
  if (!keep) {
    osDelay(2UL);
    portexp_write(PORTEXP_A, PORTEXP_A_BIT_OUT_SX_NRESET, GPIO_PIN_SET);
    osDelay(2UL);
    spiSxWaitUntilReady();
  }
}

void sxDIOxINTenable(bool on)
{
  /* Preparing SX DIO1/DIO3 interrupt handling */

  if (on) {
    /* Disable PortExpander INT */
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO1_TXRXDONE, false);
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO3_TIMEOUT,  false);


    /* 9 SetDioIrqParams()                              IRQ enable           DIO1           DIO2           DIO3 */
    const uint8_t bufSxSetDioIrqParams[9] = { 0x08U,  0x03U, 0xffU,  0x00U, 0x03U,  0x00U, 0x00U,  0x02U, 0x00U };
    spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetDioIrqParams), bufSxSetDioIrqParams);
    spiSxWaitUntilReady();

    /* 9a SetDio2AsRfSwitchCtrl() */
    const uint8_t bufSxSetDio2AsRfSwitchCtrl[2] = { 0x9dU, 0x01U };
    spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetDio2AsRfSwitchCtrl), bufSxSetDio2AsRfSwitchCtrl);
    spiSxWaitUntilReady();

    /* ClearIrqStatus(all) */
    const uint8_t bufSxClearIrqStatusAll[3] = { 0x02U, 0x03U, 0xffU };
    spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxClearIrqStatusAll), bufSxClearIrqStatusAll);
    spiSxWaitUntilReady();

    /* Clear interrupt reference by reading the ports */
    portexp_read(PORTEXP_A, 0);

    /* Clear EXTI_PORTEXP_A from EXTI event group */
    xEventGroupClearBits(extiEventGroupHandle, EXTI_PORTEXP_A);

    /* Enable the PortExpander interrupt chain */
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO1_TXRXDONE, true);
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO3_TIMEOUT,  true);

  } else {
    /* Disable DIO1/DIO3 PORTEXP_A INT */

    /* Disable PortExpander INT */
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO1_TXRXDONE, false);
    portexp_setIrq(PORTEXP_A, PORTEXP_A_BIT_IN_SX_DIO3_TIMEOUT,  false);

    /* Reset all IRQ flags */
    sxRegisterIRQclearAll();

    /* Clear EXTI_PORTEXP_A from EXTI event group */
    xEventGroupClearBits(extiEventGroupHandle, EXTI_PORTEXP_A);
  }
}

void sxSetFrequencySetMHz(float frq)
{
  const uint32_t frqWord   = (uint32_t) (0.5f + (frq * ((1UL << 25) / 32.0f)));
  const uint8_t frqQuad[4] = {
      GET_BYTE_OF_WORD(frqWord, 3),
      GET_BYTE_OF_WORD(frqWord, 2),
      GET_BYTE_OF_WORD(frqWord, 1),
      GET_BYTE_OF_WORD(frqWord, 0)
  };

  /* 3 SetRfFrequency() */
  const uint8_t bufSxSetRfFrequency[5] = { 0x86U, frqQuad[0], frqQuad[1], frqQuad[2], frqQuad[3] };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetRfFrequency), bufSxSetRfFrequency);
  spiSxWaitUntilReady();
}

void sxSetModulationParams(uint8_t BW, uint8_t SF, uint8_t LDRO)
{
  /* 7 SetModulationParams()                             SFxx  xxxkHz   CR4_8  LDROxx                               */    // Use LDRO when TX time >= 16.38 ms
  const uint8_t bufSxSetModulationParams[9] = { 0x8bU,     SF,     BW,  0x04U,   LDRO,   0x00U, 0x00U, 0x00U, 0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetModulationParams), bufSxSetModulationParams);
  spiSxWaitUntilReady();
}

void sxCalibrateImage(void)
{
  /* CalibrateImage(868 MHz) */
  const uint8_t bufSxCalibrateImage868MHz[3] = { 0x98U, 0xd7U, 0xdbU };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxCalibrateImage868MHz), bufSxCalibrateImage868MHz);
  spiSxWaitUntilReady();
  osDelay(25UL);
}


void sxRegisterIRQclearAll(void)
{
  /* ClearIrqStatus(all) */
  const uint8_t bufSxClearIrqStatusAll[3] = { 0x02U, 0x03U, 0xffU };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxClearIrqStatusAll), bufSxClearIrqStatusAll);
  spiSxWaitUntilReady();
}

void sxPrepareInitial(uint8_t XTA, uint8_t XTB)
{
  /* 1 SetStandby(STDBY_RC)                       RC */
  const uint8_t bufSxSetStdbyRc[2] = { 0x80U,  0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetStdbyRc), bufSxSetStdbyRc);
  spiSxWaitUntilReady();

  /* XTA, XTB trim                                         XTA  XTB */
  const uint8_t bufSxSetXtaXtb[5] = { 0x0dU, 0x09U, 0x11U, XTA, XTB };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetXtaXtb), bufSxSetXtaXtb);
  spiSxWaitUntilReady();

  /* SetRegulatorMode(DC_DC+LDO)                         DCDC */
  const uint8_t bufSxSetRegulatorModeDcdc[2] = { 0x96U, 0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetRegulatorModeDcdc), bufSxSetRegulatorModeDcdc);
  spiSxWaitUntilReady();

  /* 2 SetPacketType(LoRa)                             LoRa */
  const uint8_t bufSxSetPacketTypeLora[2] = { 0x8aU,  0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetPacketTypeLora), bufSxSetPacketTypeLora);
  spiSxWaitUntilReady();

  /* SetRxTxFallbackModeXosc()                              XOSC */
  const uint8_t bufSxSetRxTxFallbackModeXosc[2] = { 0x93U, 0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetRxTxFallbackModeXosc), bufSxSetRxTxFallbackModeXosc);
  spiSxWaitUntilReady();

  /* SetStandby(STDBY_XOSC)                       XOSC */
  const uint8_t bufSxSetStdbyXosc[2] = { 0x80U,  0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetStdbyXosc), bufSxSetStdbyXosc);
  spiSxWaitUntilReady();
  osDelay(10UL);

  /* 5 SetBufferBaseAddress()                               TX      RX */
  const uint8_t bufSxSetBufferBaseAddress[3] = { 0x8fU,  0x80U,  0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetBufferBaseAddress), bufSxSetBufferBaseAddress);
  spiSxWaitUntilReady();

  /* Set DIO2 to HI-level = TX */
  const uint8_t bufSxSetDio2AsRfSwitchCtrl[2] = { 0x9dU,  0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetDio2AsRfSwitchCtrl), bufSxSetDio2AsRfSwitchCtrl);
  spiSxWaitUntilReady();
}

void sxPrepareAddTX(const volatile uint8_t* payloadBuf, uint8_t payloadLen, float frq, uint8_t BW, uint8_t SF, uint8_t LDRO, int8_t txPwr)
{
  /* 3 SetRfFrequency() */
  sxSetFrequencySetMHz(frq);

  /* 4 SetTxParams()                                dBm  RT80us */
  const uint8_t bufSxSetTxParams[3]   = { 0x8eU,  txPwr,  0x05U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetTxParams), bufSxSetTxParams);
  spiSxWaitUntilReady();

  /* SetPaConfig */
//const uint8_t bufSxSetPaConfig1261[5] = { 0x95U, 0x04U, 0x00U, 0x01U, 0x01U };                // Max. +14dBm  --> +0.00 dB (12.75dBm)            (0002: 11.90 dBm)
//const uint8_t bufSxSetPaConfig1261[5] = { 0x95U, 0x05U, 0x00U, 0x01U, 0x01U };                //              --> +0.42 dB (13.17dBm)            (0002: 12.34 dBm)
  const uint8_t bufSxSetPaConfig1261[5] = { 0x95U, 0x06U, 0x00U, 0x01U, 0x01U };                // Max. +15dBm  --> +0.72 dB (13.48dBm), 27mA @ 6V (0002: 12.67 dBm)
//const uint8_t bufSxSetPaConfig1261[5] = { 0x95U, 0x07U, 0x00U, 0x01U, 0x01U };                //              --> +0.10 dB (12.85dBm)            (0002: 12.94 dBm)
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetPaConfig1261), bufSxSetPaConfig1261);
  spiSxWaitUntilReady();

  #if 0
  /* Set OCP Configuration (RegAddr: 0x08e7) */
  #endif

  /* 7 SetModulationParams() */
  sxSetModulationParams(BW, SF, LDRO);

  /* 8 SetPacketParams()                             Preamble Len  Explic  Message Len   CRCon    IQno                        */
  const uint8_t bufSxSetPacketParams[10] = { 0x8cU,  0x00U, 0x0cU,  0x00U,  payloadLen,  0x01U,  0x00U,   0x00U, 0x00U, 0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetPacketParams), bufSxSetPacketParams);
  spiSxWaitUntilReady();

  /* 10 SyncWord: WriteReg(                                0x740,  0x34,  0x44) */                                        // LoRaWAN uses 0x34 0x44
  const uint8_t bufSxLoraSyncword3444[5] = { 0x0dU, 0x07U, 0x40U,  0x34U, 0x44U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxLoraSyncword3444), bufSxLoraSyncword3444);
  spiSxWaitUntilReady();

  /* 6 WriteBuffer()                       Start */
  uint8_t bufSxWriteBuffer[258] = { 0x0eU, 0x80U };
  memcpy((void*) &(bufSxWriteBuffer[2]), (void*) payloadBuf, payloadLen);
  spiProcessSpi3MsgTemplate(SPI3_SX1261, (2U + payloadLen), bufSxWriteBuffer);
  spiSxWaitUntilReady();

  /* CalibrateImage(868 MHz) */
  sxCalibrateImage();
}

void sxPrepareAddRX(float frq, uint8_t BW, uint8_t SF, uint8_t LDRO)
{
  /* 3 SetRfFrequency() */
  sxSetFrequencySetMHz(frq);

  /* 7 SetModulationParams() */
  sxSetModulationParams(BW, SF, LDRO);

  /* 8 SetPacketParams()                             Preamble Len  Explic  Message Len   CRCon   IQyes                        */    // Max. payload length 0x60 bytes
  const uint8_t bufSxSetPacketParams[10] = { 0x8cU,  0x00U, 0x0cU,  0x00U,       0x60U,  0x01U,  0x01U,   0x00U, 0x00U, 0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetPacketParams), bufSxSetPacketParams);
  spiSxWaitUntilReady();

  /* 10 SyncWord: WriteReg(                                0x740,  0x34,  0x44) */                                                  // LoRaWAN uses 0x34 0x44
  const uint8_t bufSxLoraSyncword3444[5] = { 0x0dU, 0x07U, 0x40U,  0x34U, 0x44U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxLoraSyncword3444), bufSxLoraSyncword3444);
  spiSxWaitUntilReady();

  #if 1
  /* Set RX gain high                                             Hi */
  const uint8_t bufSxSetRxGainHigh[5] = { 0x0dU, 0x08U, 0xacU, 0x96U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetRxGainHigh), bufSxSetRxGainHigh);
  spiSxWaitUntilReady();
  #endif

  #if 0
  /* StopTimerOnPreamble()                                on */
  const uint8_t bufSxStopTimerOnPreamble[2] = { 0x9fU, 0x01U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxStopTimerOnPreamble), bufSxStopTimerOnPreamble);
  spiSxWaitUntilReady();
  #endif

  #if 0
  /* SetLoRaSymbNumTimeout() */
  const uint8_t bufSxSetLoRaSymbNumTimeout[2] = { 0xa0U, 0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetLoRaSymbNumTimeout), bufSxSetLoRaSymbNumTimeout);
  spiSxWaitUntilReady();
  #endif

  #if 0
  /* CalibrateImage(868 MHz) */
  sxCalibrateImage();
  #endif
}

void sxTxRxPreps(LoRaWANctx_t* ctx, SX_Mode_t mode, const volatile uint8_t* payloadBuf, uint8_t payloadLen, float frq, int8_t txPwr, uint8_t BW, uint8_t SF, uint8_t LDRO, uint8_t XTA, uint8_t XTB)
{
  /* Determine the frequency to use */
  if (!frq) {
    if (CurWin_RXTX1 == ctx->Current_RXTX_Window) {
      /* Set TX/RX1 frequency */
      frq = ctx->FrequencyMHz = LoRaWAN_calc_Channel_to_MHz(
                                            ctx,
                                            ctx->Ch_Selected,
                                            ctx->Dir,
                                            0);

    } else if (CurWin_RXTX2 == ctx->Current_RXTX_Window) {
      /* Jump to RX2 frequency (default frequency) */
      frq = ctx->FrequencyMHz = LoRaWAN_calc_Channel_to_MHz(
                                            ctx,
                                            0,                                                  // The default RX2 channel
                                            ctx->Dir,
                                            1);                                                 // Use default settings
    }
  }

  /* Determine the data rate to use */
  if (!SF) {
    if (CurWin_RXTX1 == ctx->Current_RXTX_Window) {
      /* TX/RX1 - UpLink and DownLink */
      SF = ctx->SpreadingFactor = LoRaWAN_DRtoSF(g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[ctx->Ch_Selected - 1] + g_framShadow1F.LinkADR_DR_RX1_DRofs);

    } else if (CurWin_RXTX2 == ctx->Current_RXTX_Window) {
      /* RX2 - DownLink */
      SF = ctx->SpreadingFactor = LoRaWAN_DRtoSF(g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[ctx->Ch_Selected - 1]);
    }

    LDRO = (SF >= 11U ?  1U : 0U);
  }

  switch (mode) {
  case SX_Mode_TX :
    {
      /* Reset SX1261 */
      sxReset(false);
      osDelay(5UL);

      /* Initial TX/RX preparations */
      sxPrepareInitial(XTA, XTB);

      /* Add additional TX specific settings */
      const float l_frq = frq * (1 + 1e-6 * g_framShadow1F.SX_XTAL_Drift);
      sxPrepareAddTX(payloadBuf, payloadLen, l_frq, BW, SF, LDRO, txPwr);
    }
    break;

  case SX_Mode_RX  :
    {
      /* Reset SX1261 */
      sxReset(false);
      osDelay(5UL);

      /* Initial TX/RX preparations */
      sxPrepareInitial(XTA, XTB);

      /* Add additional RX specific settings */
      const float l_frq = frq * (1 + (1e-6 * g_framShadow1F.SX_XTAL_Drift) - (1e-6 * ctx->GatewayPpm));
      sxPrepareAddRX(l_frq, BW, SF, LDRO);
    }
    break;

  case SX_Mode_RX2 :
    {
      /* Changing the frequency and SF on the fly - image calibration needed to be done */
      const float l_frq = frq * (1 + (1e-6 * g_framShadow1F.SX_XTAL_Drift) - (1e-6 * ctx->GatewayPpm));

      /* 3 SetRfFrequency() */
      sxSetFrequencySetMHz(l_frq);

      /* 7 SetModulationParams() */
      sxSetModulationParams(BW, SF, LDRO);
    }
    break;

  default:
    return;
  }
}

uint32_t sxStartTXFromBuf(uint32_t timeout_ms)
{
  const uint32_t timeOutWord = 64UL * timeout_ms;

  /* SetTx() */
  const uint8_t bufSxSetTx[4] = { 0x83U, GET_BYTE_OF_WORD(timeOutWord, 2), GET_BYTE_OF_WORD(timeOutWord, 1), GET_BYTE_OF_WORD(timeOutWord, 0) };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetTx), bufSxSetTx);
  spiSxWaitUntilReady();
  const uint32_t tsTxStart = xTaskGetTickCount();

  /* Preparing SX DIO1/DIO3 interrupt handling */
  sxDIOxINTenable(true);

  return tsTxStart;
}

void sxTestTXContinuesWave(void)
{
  /* RfSwitch */

  /* SetTxContinuousWave() */
  const uint8_t bufSxSetTxContinuousWave[1] = { 0xd1U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetTxContinuousWave), bufSxSetTxContinuousWave);
  spiSxWaitUntilReady();
}

void sxTestTXInfinitePreamble(void)
{
  /* SetTxInfinitePreamble() */
  const uint8_t bufSxSetTxInfinitePreamble[1] = { 0xd2U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetTxInfinitePreamble), bufSxSetTxInfinitePreamble);
  spiSxWaitUntilReady();
}


void sxStartRXToBuf(uint32_t timeout_ms)
{
  const uint32_t timeOutWord = 64UL * timeout_ms + 1UL;

  /* SetRX() */
  const uint8_t bufSxSetRx[4] = { 0x82U, GET_BYTE_OF_WORD(timeOutWord, 2), GET_BYTE_OF_WORD(timeOutWord, 1), GET_BYTE_OF_WORD(timeOutWord, 0) };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetRx), bufSxSetRx);
  spiSxWaitUntilReady();

  /* Preparing SX DIO1/DIO3 interrupt handling */
  sxDIOxINTenable(true);

  /* Debugging */
  //sxStoreTimestamp(xTaskGetTickCount());
}

uint32_t sxWaitUntilTxDone(uint32_t stopTimeAbs)
{
  /* Use TxDone and RxDone - mask out all other IRQs */
  /* ClearIrqStatus() */
  const uint8_t bufSxClearIrqStatusAll[3] = { 0x02U, 0xffU, 0xffU };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxClearIrqStatusAll), bufSxClearIrqStatusAll);
  spiSxWaitUntilReady();

  /* Wait for IRQ */
  const uint32_t tsEndOfTx = spiSxWaitUntilDio1(stopTimeAbs);

  /* Disable DIO1/DIO3 PORTEXP_A INT */
  sxDIOxINTenable(false);

  /* Debugging */
  //sxStoreTimestamp(tsEndOfTx);

  /* Time of TX completed */
  return tsEndOfTx;
}

void sxWaitUntilRxDone(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg, uint32_t stopTimeAbs)
{
  const uint32_t tsEndTx = ctx->TsEndOfTx;
  (void) tsEndTx;

  /* Wait for IRQ */
  /* uint32_t ts = */  spiSxWaitUntilDio1(stopTimeAbs);

  /* Debugging */
  //sxStoreTimestamp(ts);

  /* GetIrqStatus() */
  const uint8_t bufSxGetIrqStatusTEST[4] = { 0x12U, 0x00U, 0x00U, 0x00U };
  spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxGetIrqStatusTEST), bufSxGetIrqStatusTEST);
  spiSxWaitUntilReady();
  uint16_t  irq  = (uint16_t)spi3RxBuffer[2] << 8;
            irq |=           spi3RxBuffer[3]     ;

  /* Disable DIO1/DIO3 PORTEXP_A INT */
  sxDIOxINTenable(false);

  if (irq & (1U << Timeout)) {
    return;

  } else if (irq & (1U << RxDone)) {
    /* Check for CRC */
    if (irq & (1U << CrcErr)) {
      return;
    }

    /* Valid Payload to decode */

    /* GetRxBufferStatus() */
    const uint8_t bufSxGetRxBufferStatus[4]   = { 0x13U,  0x00U, 0x00U, 0x00U };
    spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxGetRxBufferStatus), bufSxGetRxBufferStatus);
    spiSxWaitUntilReady();
    const uint8_t rxNbBytes = spi3RxBuffer[2];
    const uint8_t rxBufPtr  = spi3RxBuffer[3];

    /* FIFO readout */
    if (rxNbBytes) {
      /* FIFO read out */
      if (rxNbBytes < sizeof(spi3RxBuffer)) {
        /* ReadBuffer() */
        const uint8_t bufSxReadBuffer[sizeof(spi3RxBuffer)]   = { 0x1eU,  rxBufPtr };
        spiProcessSpi3MsgTemplate(SPI3_SX1261, (rxNbBytes + 3U), bufSxReadBuffer);
        spiSxWaitUntilReady();

        /* Copy SPI receive buffer content to msg object */
        memcpy((void*)msg->msg_encoded_Buf, (const void*)spi3RxBuffer + 3, rxNbBytes);
        msg->msg_encoded_Len = rxNbBytes;

      } else {
        /* Buffer to small */
        Error_Handler();
      }
    }
  }
}

void sxProcessRxDone(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg)
{
  uint8_t irq;

  /* Get the current IRQ flags */
#if 0
  spi3TxBuffer[0] = SPI_RD_FLAG | 0x12;                                                         // LoRa: RegIrqFlags
  spiProcessSpi3Msg(2);
#endif
  irq = spi3RxBuffer[1];

  /* Reset all IRQ flags */
  sxRegisterIRQclearAll();

  if (irq & (1U << RxDone)) {
    /* Check for CRC */
    if (irq & (1U << 0 /*PayloadCrcError*/)) {
      return;
    }

    //spi3TxBuffer[0] = SPI_RD_FLAG | 0x13;                                                       // RegRxNbBytes
    //spiProcessSpiMsg(2);
    uint8_t rxNbBytes = 0 /*spi3RxBuffer[1]*/;

    /* FIFO readout */
    if (rxNbBytes) {
      /* Positioning of the FIFO addr ptr */
      {
        spi3TxBuffer[0] = SPI_RD_FLAG | 0x10;                                                   // RegFifoRxCurrentAddr
        //spiProcessSpiMsg(2);
        uint8_t fifoRxCurrentAddr = spi3RxBuffer[1];

        spi3TxBuffer[0] = SPI_WR_FLAG | 0x0d;                                                   // RegFifoAddrPtr
        spi3TxBuffer[1] = fifoRxCurrentAddr;
        //spiProcessSpiMsg(2);
      }

      /* FIFO read out */
      if (rxNbBytes < sizeof(spi3RxBuffer)) {
        spi3TxBuffer[0] = SPI_RD_FLAG | 0x00;    // RegFifo
        //spiProcessSpiMsg(1 + rxNbBytes);

        /* Copy SPI receive buffer content to msg object */
        memcpy((void*)msg->msg_encoded_Buf, (const void*)spi3RxBuffer + 1, rxNbBytes);
        msg->msg_encoded_Len = rxNbBytes;

      } else {
        /* Buffer to small */
        Error_Handler();
      }
    }
  }
}
