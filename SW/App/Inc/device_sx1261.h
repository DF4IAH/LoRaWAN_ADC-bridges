/*
 * device_sx1261.h
 *
 *  Created on: 06.05.2019
 *      Author: DF4IAH
 */

#ifndef INC_DEVICE_SX1261_H_
#define INC_DEVICE_SX1261_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "task_LoRaWAN.h"


typedef enum SX_Mode {

  SX_Mode_NOP = 0,

  SX_Mode_TX,

  SX_Mode_RX,
  SX_Mode_RX2,

} SX_Mode_t;


typedef enum sxIRQbits {

  TxDone                  = 0,
  RxDone,
  PreambleDetected,
  SyncWordValid,
  HeaderValid,
  HeaderErr,
  CrcErr,
  CadDone,
  CadDetected,
  Timeout,

} sxIRQbits_t;


void      sxReset(bool keep);
void      sxDIOxINTenable(bool on);
void      sxSetFrequencySetMHz(float frq);
void      sxSetModulationParams(uint8_t BW, uint8_t SF, uint8_t LDRO);
void      sxCalibrateImage(void);

void      sxRegisterIRQclearAll(void);

void      sxPrepareInitial(uint8_t XTA, uint8_t XTB);
void      sxPrepareAddTX(const volatile uint8_t* payloadBuf, uint8_t payloadLen, float frq, uint8_t BW, uint8_t SF, uint8_t LDRO, int8_t txPwr);
void      sxPrepareAddRX(float frq, uint8_t BW, uint8_t SF, uint8_t LDRO);

void      sxTxRxPreps(LoRaWANctx_t* ctx, SX_Mode_t mode, const volatile uint8_t* payloadBuf, uint8_t payloadLen, float frq, int8_t txPwr, uint8_t BW, uint8_t SF, uint8_t LDRO, uint8_t XTA, uint8_t XTB);

uint32_t  sxStartTXFromBuf(uint32_t timeout_ms);
void      sxTestTXContinuesWave(void);
void      sxTestTXInfinitePreamble(void);
void      sxStartRXToBuf(uint32_t timeout_ms);

uint32_t  sxWaitUntilTxDone(uint32_t stopTimeAbs);
void      sxWaitUntilRxDone(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg, uint32_t stopTimeAbs);
void      sxProcessRxDone(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg);

#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_SX1261_H_ */
