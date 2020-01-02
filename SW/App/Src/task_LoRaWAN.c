/*
 * LoRaWAN.c
 *
 *  Created on: 12.05.2018
 *      Author: DF4IAH
 */

#define __STDC_WANT_LIB_EXT1__ 1

/* Includes ------------------------------------------------------------------*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#include "stm32l452xx.h"
#include "stm32l4xx_hal.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "rtc.h"
#include "rng.h"
#include "crc.h"

#include "lib_crypto.h"
#include "device_fram.h"
#include "device_sx1261.h"
#include "bus_spi.h"

#include "task_LoRaWAN.h"


/* Variables -----------------------------------------------------------------*/

extern EventGroupHandle_t   globalEventGroupHandle;
extern EventGroupHandle_t   controllerEventGroupHandle;
extern EventGroupHandle_t   loraEventGroupHandle;
extern osMessageQId         loraInQueueHandle;
extern osMessageQId         loraOutQueueHandle;
extern osMessageQId         loraMacQueueHandle;
extern osMutexId            iot4BeesApplUpDataMutexHandle;
extern osMutexId            iot4BeesApplDnDataMutexHandle;
extern osMutexId            trackMeApplUpDataMutexHandle;
extern osMutexId            trackMeApplDnDataMutexHandle;


extern ENABLE_MASK_t        g_enableMsk;
extern MON_MASK_t           g_monMsk;

extern Fram1F_Template_t    g_framShadow1F;


/* SPI communication buffers */
extern uint8_t              spi3TxBuffer[SPI3_BUFFERSIZE];
extern uint8_t              spi3RxBuffer[SPI3_BUFFERSIZE];

const uint32_t              loRaWANWait_EGW_MaxWaitTicks      = 1000UL / portTICK_PERIOD_MS;           // One second
const uint32_t              LoRaWAN_MaxWaitMs                 = 100UL;


/* USE_OTAA */

/* TheThingsNetwork - assigned codes to this device - R1.03 */
#if   defined IOT_DEVICE_0001
const uint8_t               DevEUI_LE[8]                      = { 0x31, 0x30, 0x30, 0x30, 0x73, 0x42, 0x34, 0x49 };                                                 // "I4Bs0001"
const uint8_t               AppEUI_LE[8]                      = { 0xFA, 0xD3, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };                                                 // "iot4bees-ctrl"

//const uint8_t             JoinEUI_LE[8]                     = { 0 };                                                                                              // V1.1: former AppEUI
//const uint8_t             NwkKey_BE[16]                     = { 0 };                                                                                              // Since LoRaWAN V1.1
const uint8_t               AppKey_BE[16]                     = { 0xCC, 0xE6, 0xA7, 0x12, 0xEE, 0x0D, 0x2C, 0x76, 0x19, 0x49, 0x32, 0x0B, 0x6D, 0x88, 0xF3, 0xE8 }; // AppKey for device "I4Bs0001"

#elif defined IOT_DEVICE_0002
const uint8_t               DevEUI_LE[8]                      = { 0x32, 0x30, 0x30, 0x30, 0x73, 0x42, 0x34, 0x49 };                                                 // "I4Bs0002"
const uint8_t               AppEUI_LE[8]                      = { 0xFA, 0xD3, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };                                                 // "iot4bees-ctrl"

//const uint8_t             JoinEUI_LE[8]                     = { 0 };                                                                                              // V1.1: former AppEUI
//const uint8_t             NwkKey_BE[16]                     = { 0 };                                                                                              // Since LoRaWAN V1.1
const uint8_t               AppKey_BE[16]                     = { 0x65, 0xDB, 0x1C, 0x64, 0x1C, 0x9F, 0xA0, 0x28, 0x2F, 0xE7, 0x4B, 0x2E, 0x20, 0x17, 0x52, 0x55 }; // AppKey for device "I4Bs0002"

#elif defined IOT_DEVICE_0003
const uint8_t               DevEUI_LE[8]                      = { 0x33, 0x30, 0x30, 0x30, 0x73, 0x42, 0x34, 0x49 };                                                 // "I4Bs0003"
const uint8_t               AppEUI_LE[8]                      = { 0xFA, 0xD3, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };                                                 // "iot4bees-ctrl"

//const uint8_t             JoinEUI_LE[8]                     = { 0 };                                                                                              // V1.1: former AppEUI
//const uint8_t             NwkKey_BE[16]                     = { 0 };                                                                                              // Since LoRaWAN V1.1
const uint8_t               AppKey_BE[16]                     = { 0xA6, 0xE1, 0x14, 0xDE, 0x1C, 0x47, 0x54, 0x96, 0xB3, 0xAF, 0x7D, 0x6D, 0xC4, 0xDE, 0x0E, 0xDE }; // AppKey for device "I4Bs0003"

#elif defined IOT_DEVICE_0004
const uint8_t               DevEUI_LE[8]                      = { 0x34, 0x30, 0x30, 0x30, 0x73, 0x42, 0x34, 0x49 };                                                 // "I4Bs0004"
const uint8_t               AppEUI_LE[8]                      = { 0xFA, 0xD3, 0x01, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };                                                 // "iot4bees-ctrl"

//const uint8_t             JoinEUI_LE[8]                     = { 0 };                                                                                              // V1.1: former AppEUI
//const uint8_t             NwkKey_BE[16]                     = { 0 };                                                                                              // Since LoRaWAN V1.1
const uint8_t               AppKey_BE[16]                     = { 0x0E, 0xED, 0x22, 0x83, 0xD3, 0xBB, 0x50, 0x41, 0x8A, 0x04, 0x46, 0x31, 0xC5, 0x77, 0x40, 0xC4 }; // AppKey for device "I4Bs0004"
#endif


/* Network context of LoRaWAN */
LoRaWANctx_t                loRaWANctx                        = { 0 };

/* Message buffers */
LoRaWAN_RX_Message_t        loRaWanRxMsg                      = { 0 };
LoRaWAN_TX_Message_t        loRaWanTxMsg                      = { 0 };


/* Application data for IoT4Bees-Ctrl */
IoT4BeesCtrlApp_up_t          g_Iot4BeesApp_up                = { 0 };
static IoT4BeesCtrlApp_up_t   s_Iot4BeesApp_up                = { 0 };

IoT4BeesCtrlApp_down_t        g_Iot4BeesApp_down              = {   };
static IoT4BeesCtrlApp_down_t s_Iot4BeesApp_down              = {   };


/* Forward declarations */

//static uint32_t LoRaWAN_calc_FrqMHz_2_CFListEntry(float f);
static void LoRaWAN_TX_msg(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg);


/* Implementations */

uint8_t LoRaWAN_DRtoSF(DataRates_t dr)
{
  if (DR0_SF12_125kHz_LoRa <= dr && dr <= DR5_SF7_125kHz_LoRa) {
    /* 125 kHz */
    return (12 - (uint8_t)dr);

  } else if (DR6_SF7_250kHz_LoRa) {
    /* 250 kHz */
    return 7;
  }

  /* No LoRa mode */
  return 0;
}


#if 0
static void LoRaWAN_PushToAnyInQueue(osMessageQId msgInH, const uint8_t* cmdAry, uint8_t cmdLen)
{
  for (uint8_t idx = 0; idx < cmdLen; ++idx) {
    xQueueSendToBack(msgInH, cmdAry + idx, 1);
  }
}

static void LoRaWAN_PushToLoraInQueue(const uint8_t* cmdAry, uint8_t cmdLen)
{
  LoRaWAN_PushToAnyInQueue(loraInQueueHandle, cmdAry, cmdLen);
  xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__QUEUE_IN);
}
#endif


#ifdef IOT4BEES_PAYLOAD_DECODER
// TTN Mapper enabled
function Decoder(bytes, port) {
  // Decode an uplink message from a buffer
  // (array) of bytes to an object of fields.
  var decoded = {};

  // Variables in use
  decoded.mote_flags  = 0x0000;
  decoded.ad_ch0_raw  = 0x0000;
  decoded.ad_ch1_raw  = 0x0000;
  decoded.ad_ch2_raw  = 0x0000;
  decoded.ad_ch3_raw  = 0x0000;
  decoded.ad_ch4_raw  = 0x0000;
  decoded.ad_ch5_raw  = 0x0000;
  decoded.ad_ch6_raw  = 0x0000;
  decoded.ad_ch7_raw  = 0x0000;
  decoded.v_bat       = 0.0;
  decoded.v_aux1      = 0.0;
  decoded.v_aux2      = 0.0;
  decoded.v_aux3      = 0.0;
  decoded.wx_temp     = 0.0;
  decoded.wx_rh       = 0.0;
  decoded.wx_baro_a   = 0.0;
  decoded.latitude    = 0.0;
  decoded.longitude   = 0.0;
  decoded.altitude    = 0;
  decoded.accuracy    = 1262.0;
  decoded.mote_ts     = 0;
  decoded.mote_cm     = 0;

  if (port === 1) {
    var idx = 0;

    if (bytes.length >= (idx + 2)) {
      decoded.mote_flags = (((bytes[0] & 0xff) << 8) | ((bytes[1] & 0xff) <<  0));
      idx += 2;

      if ((decoded.mote_flags & 0x0001) !== 0) {
        // AD_channels 0..3
        if (bytes.length >= (idx + 8)) {
          decoded.ad_ch0_raw = (((bytes[idx + 0] & 0xff) << 8) | (bytes[idx + 1] & 0xff));
          decoded.ad_ch1_raw = (((bytes[idx + 2] & 0xff) << 8) | (bytes[idx + 3] & 0xff));
          decoded.ad_ch2_raw = (((bytes[idx + 4] & 0xff) << 8) | (bytes[idx + 5] & 0xff));
          decoded.ad_ch3_raw = (((bytes[idx + 6] & 0xff) << 8) | (bytes[idx + 7] & 0xff));
          idx += 8;
        }
      }

      if ((decoded.mote_flags & 0x0002) !== 0) {
        // AD_channels 4..7
        if (bytes.length >= (idx + 8)) {
          decoded.ad_ch4_raw = (((bytes[idx + 0] & 0xff) << 8) | (bytes[idx + 1] & 0xff));
          decoded.ad_ch5_raw = (((bytes[idx + 2] & 0xff) << 8) | (bytes[idx + 3] & 0xff));
          decoded.ad_ch6_raw = (((bytes[idx + 4] & 0xff) << 8) | (bytes[idx + 5] & 0xff));
          decoded.ad_ch7_raw = (((bytes[idx + 6] & 0xff) << 8) | (bytes[idx + 7] & 0xff));
          idx += 8;
        }
      }

      if ((decoded.mote_flags & 0x0004) !== 0) {
        if (bytes.length >= (idx + 8)) {
          // Battery voltage and AUX voltages are ranging between  0.0 .. 25.6 volts
          decoded.v_bat   = (((bytes[idx + 0] & 0xff) << 8) | (bytes[idx + 1] & 0xff)) / 100.0;
          decoded.v_aux1  = (((bytes[idx + 2] & 0xff) << 8) | (bytes[idx + 3] & 0xff)) / 100.0;
          decoded.v_aux2  = (((bytes[idx + 4] & 0xff) << 8) | (bytes[idx + 5] & 0xff)) / 100.0;
          decoded.v_aux3  = (((bytes[idx + 6] & 0xff) << 8) | (bytes[idx + 7] & 0xff)) / 100.0;
          idx += 8;
        }
      }

      if ((decoded.mote_flags & 0x0008) !== 0) {
        if (bytes.length >= (idx + 4)) {
          // Temperature ranging between -327.68 and 327.67 degree Celsius
          decoded.wx_temp   = (((bytes[idx + 0] & 0xff) << 8) | (bytes[idx + 1] & 0xff)) / 10.0;

          // Relative humidity ranging between 0.0 and 100.0 %
          decoded.wx_rh     = (bytes[idx + 2] & 0xff);

          // Absolute pressure not height corrected ranging between 822 and 1077 hPa
          decoded.wx_baro_a = (bytes[idx + 3] & 0xff) + 950.0;
          idx += 4;

          // Clear empty field
          if (decoded.wx_baro_a == 950) {
            decoded.wx_baro_a = 0.0;
          }
        }
      }

      if ((decoded.mote_flags & 0x1000) !== 0) {
        if (bytes.length >= (idx + 8)) {
          // Latitude 21 bits
          decoded.latitude  =   -90.0 + ((((bytes[idx + 0] & 0xff) << 13) | ((bytes[idx + 1] & 0xff) <<  5) | ((bytes[idx + 2] & 0xf8) >> 3)) / 10000.0);

          // Longitude 22 bits
          decoded.longitude =  -180.0 + ((((bytes[idx + 2] & 0x07) << 19) | ((bytes[idx + 3] & 0xff) << 11) | ((bytes[idx + 4] & 0xff) << 3) | ((bytes[idx + 5] & 0xe0) >> 5)) / 10000.0);

          // Altitude in meters 16 bits
          decoded.altitude  = -8192.0 +  (((bytes[idx + 5] & 0x1f) << 11) | ((bytes[idx + 6] & 0xff) <<  3) | ((bytes[idx + 7] & 0xe0) >> 5));

          // Accuracy in meters 5 bits
          decoded.accuracy  = Math.pow(1.25, (bytes[idx + 7] & 0x1f)) / 10.0;

          idx += 8;
        }
      }
    }

    // Thin security
    decoded.mote_ts = Date.now();
    decoded.mote_cm = ((0x0392ea84 + (decoded.mote_ts & 0x7fffffff)) % 0x17ef23ab) ^ ((0x339dc627 + (decoded.mote_ts & 0x07ffffff) * 7) % 0x2a29e2cc);
  }

  return decoded;
}
#endif

static uint8_t LoRaWAN_marshalling_PayloadCompress_IoT4BeesAppUp(uint8_t outBuf[], const IoT4BeesCtrlApp_up_t* iot4BeesApp_UL)
{
  uint8_t   outLen  = 0;

  /* PAYLOAD configuration, flags are using 2 bytes: MSB + LSB */
  uint16_t  flags   = 0x0000U;
  outLen += 2U;

  /* ADC raw channel data 0..3 */
  if (iot4BeesApp_UL->ad_ch_raw[0] || iot4BeesApp_UL->ad_ch_raw[1] || iot4BeesApp_UL->ad_ch_raw[2] || iot4BeesApp_UL->ad_ch_raw[3]) {
    flags |= 0x0001;

    for (uint8_t idx = 0U; idx < 4U; ++idx) {
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->ad_ch_raw[idx], 1);
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->ad_ch_raw[idx], 0);
    }
  }

  /* ADC raw channel data 4..7 */
  if (iot4BeesApp_UL->ad_ch_raw[4] || iot4BeesApp_UL->ad_ch_raw[5] || iot4BeesApp_UL->ad_ch_raw[6] || iot4BeesApp_UL->ad_ch_raw[7]) {
    flags |= 0x0002;

    for (uint8_t idx = 4U; idx < 8U; ++idx) {
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->ad_ch_raw[idx], 1);
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->ad_ch_raw[idx], 0);
    }
  }

  /* Voltages: battery and 3 AUX channels */
  if (iot4BeesApp_UL->v_bat_100th_volt || iot4BeesApp_UL->v_aux_100th_volt[0] || iot4BeesApp_UL->v_aux_100th_volt[1] || iot4BeesApp_UL->v_aux_100th_volt[2]) {
    flags |= 0x0004;

    outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->v_bat_100th_volt, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->v_bat_100th_volt, 0);

    for (uint8_t idx = 0U; idx < 3U; ++idx) {
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->v_aux_100th_volt[idx], 1);
      outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->v_aux_100th_volt[idx], 0);
    }
  }

  /* Weather data */
  if (iot4BeesApp_UL->wx_temp_10th_degC || iot4BeesApp_UL->wx_rh || iot4BeesApp_UL->wx_baro_m950_Pa) {
    flags |= 0x0008;

    outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->wx_temp_10th_degC, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(iot4BeesApp_UL->wx_temp_10th_degC, 0);

    outBuf[outLen++]  = iot4BeesApp_UL->wx_rh;

    outBuf[outLen++]  = iot4BeesApp_UL->wx_baro_m950_Pa;
  }

#ifdef GNSS_EXTRA
  /* GNSS data */
  if (iot4BeesApp_UL->gnss_lat || iot4BeesApp_UL->gnss_lon || iot4BeesApp_UL->gnss_alt_m || iot4BeesApp_UL->gnss_acc_m) {
    flags |= 0x1000;

    /* Calculate data */
    const uint32_t lat =      (iot4BeesApp_UL->gnss_lat           +   90.0f)  * 10000.0f  + 0.5f;
    const uint32_t lon =      (iot4BeesApp_UL->gnss_lon           +  180.0f)  * 10000.0f  + 0.5f;
    const uint32_t alt =      (iot4BeesApp_UL->gnss_alt_m         + 8192.0f)              + 0.5f;
    const uint32_t acc = logf( iot4BeesApp_UL->gnss_acc_m                     *    10.0f) / logf(1.25f);

    /* Format data */
    const uint32_t lat_shifted  = 0x00fffff8UL & (lat << 3);    // 21 bits
    const uint32_t lon_shifted  = 0x07ffffe0UL & (lon << 5);    // 22 bits
    const uint32_t alt_shifted  = 0x001fffe0UL & (alt << 5);    // 16 bits
    const uint32_t acc_shifted  = 0x0000001fUL & (acc     );    //  5 bits  --> 64 bits = 8 bytes

    /* Fill in PAYLOAD */
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lat_shifted, 2);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lat_shifted, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lat_shifted, 0) | GET_BYTE_OF_WORD(lon_shifted, 3);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lon_shifted, 2);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lon_shifted, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(lon_shifted, 0) | GET_BYTE_OF_WORD(alt_shifted, 2);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(alt_shifted, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(alt_shifted, 0) | GET_BYTE_OF_WORD(acc_shifted, 0);
  }

# ifdef GNSS_EXTRA_SPEED_COURSE
  /* Speed and course data */
  if (iot4BeesApp_UL->gnss_spd_kmh > 1.0f) {
    flags |= 0x2000;

    /* Calculate data */
    const uint32_t crs =       iot4BeesApp_UL->gnss_crs;
    const uint32_t spd =       iot4BeesApp_UL->gnss_spd_kmh                   *    10.0f  + 0.5f;

    /* Format data */
    const uint32_t crs_shifted  = 0x0000ff80UL & (crs << 7);    //  9 bits
    const uint32_t spd_shifted  = 0x00007fffUL & (spd << 0);    // 15 bits

    /* Fill in PAYLOAD */
    outBuf[outLen++]  = GET_BYTE_OF_WORD(crs_shifted, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(crs_shifted, 0) | GET_BYTE_OF_WORD(spd_shifted, 1);
    outBuf[outLen++]  = GET_BYTE_OF_WORD(spd_shifted, 0);
  }
# endif
#endif

  /* Enter flags bit field */
  outBuf[0] = GET_BYTE_OF_WORD(flags, 1);
  outBuf[1] = GET_BYTE_OF_WORD(flags, 0);

  return outLen;
}

static uint8_t LoRaWAN_marshalling_PayloadExpand_IoT4BeesAppDown(IoT4BeesCtrlApp_down_t* ioT4BeesApp_DL, const uint8_t* compressedMsg, uint8_t compressedMsgLen)
{
  return 0;
}


static void LoRaWAN_BeamUpProcess__Fsm_TX(void)
{
  /* Prepare to transmit data buffer */
  if (loRaWANctx.FsmState == Fsm_NOP) {
    /* Adjust the context */
    loRaWANctx.Ch_Selected  = LoRaWAN_calc_randomChannel(&loRaWANctx);                // Randomized RX1 frequency

    /* Requesting for confirmed data up-transport */
    g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
    g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;
    fram_updateCrc();

    /* Start preparations for TX */
    loRaWANctx.FsmState = Fsm_TX;

  } else {
    /* Drop data */
  }
}

static void LoRaWAN_BeamUpProcess(void)
{
  /* Sensor data available for transmission */
  if (pdPASS == xSemaphoreTake(iot4BeesApplUpDataMutexHandle, 2UL / portTICK_PERIOD_MS)) {
    __DMB();
    __ISB();

    memcpy(&s_Iot4BeesApp_up, &g_Iot4BeesApp_up, sizeof(g_Iot4BeesApp_up));
    xSemaphoreGive(iot4BeesApplUpDataMutexHandle);

    loRaWanTxMsg.msg_prep_FRMPayload_Len = LoRaWAN_marshalling_PayloadCompress_IoT4BeesAppUp(loRaWanTxMsg.msg_prep_FRMPayload_Buf, &s_Iot4BeesApp_up);
    loRaWanTxMsg.msg_encoded_EncDone = 0U;

    LoRaWAN_BeamUpProcess__Fsm_TX();
  }
}


#if 0
static void LoRaWAN_QueueIn_Process__Fsm_TX(void)
{
  /* Prepare to transmit data buffer */
  if (loRaWANctx.FsmState == Fsm_NOP) {
    /* USB: info */
    //usbLogLora("\r\nLoRaWAN: Going to send sensors data.\r\n");

    /* Adjust the context */
    loRaWANctx.Ch_Selected  = LoRaWAN_calc_randomChannel(&loRaWANctx);                // Randomized RX1 frequency

    /* Requesting for confirmed data up-transport */
    loRaWANctx.MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
    loRaWANctx.MHDR_Major = LoRaWAN_R1;

    /* Request to encode TX message (again) */
    loRaWanTxMsg.msg_encoded_EncDone = 0;

    /* Start preparations for TX */
    loRaWANctx.FsmState = Fsm_TX;

  } else {
    /* Drop data when (again?) Join-Request is just exchanged */
  }
}
#endif

static void LoRaWAN_QueueIn_Process(void)
{
  static uint8_t  buf[32]   = { 0 };
  static uint8_t  bufCtr    = 0;
  static uint8_t  bufMsgLen = 0;
  BaseType_t      xStatus;
  uint8_t         inChr;

  do {
    do {
      /* Take next character from the queue, if any */
      inChr = 0;
      xStatus = xQueueReceive(loraInQueueHandle, &inChr, 100UL / portTICK_PERIOD_MS); // Wait max. 100 ms for completion
      if (pdPASS == xStatus) {
        if (!bufMsgLen) {
          bufMsgLen = inChr;

        } else {
          /* Process incoming message */
          buf[bufCtr++] = inChr;

          if (bufCtr == bufMsgLen) {
            /* Message complete */
            break;
          }
        }

      } else {
        /* Reset the state of the queue */
        goto loRaWAN_Error_clrInBuf;
      }
    } while (true);

    /* Process the message */
    switch (buf[0]) {
    case LoraInQueueCmds__Init:
      {
        /* Set event mask bit for INIT */
        xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_INIT);

        /* Write back to NVM */
        g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
        fram_updateCrc();
      }
      break;

    case LoraInQueueCmds__IoT4BeesApplUp:
    {
      /* Process LoRaWAN if enabled */
      if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
        /* Prepare data to upload */
        {
          /* Take mutex to access LoRaWAN TrackMeApp */
          if (pdTRUE == xSemaphoreTake(iot4BeesApplUpDataMutexHandle, 500UL / portTICK_PERIOD_MS)) {
            /* Marshal data for upload */
            loRaWanTxMsg.msg_prep_FRMPayload_Len = LoRaWAN_marshalling_PayloadCompress_IoT4BeesAppUp(loRaWanTxMsg.msg_prep_FRMPayload_Buf, &s_Iot4BeesApp_up);
            loRaWanTxMsg.msg_encoded_EncDone = 0U;

            /* Give back mutex */
            xSemaphoreGive(iot4BeesApplUpDataMutexHandle);

            /* Prepare to transmit data buffer */
            LoRaWAN_BeamUpProcess__Fsm_TX();

          } else {
            /* No luck, abort plan to transmit */
            loRaWANctx.FsmState = Fsm_NOP;
          }
        }
      }
    }
    break;

    case LoraInQueueCmds__LinkCheckReq:
      {
        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_LINKCHECKREQ);

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();
        }
      }
      break;

    case LoraInQueueCmds__DeviceTimeReq:
      {
        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          //xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_DEVICETIMEREQ);
        }
      }
      break;

    case LoraInQueueCmds__ConfirmedPackets:
      {
        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          const uint8_t confSet               = buf[1];
          loRaWANctx.ConfirmedPackets_enabled = confSet ?  1U : 0U;
        }
      }
      break;

    case LoraInQueueCmds__ADRset:
      {
        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          const uint8_t adrSet    = buf[1];
          g_framShadow1F.LinkADR_ADR_enabled = adrSet ?  1U : 0U;

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();
        }
      }
      break;

    case LoraInQueueCmds__DRset:
      {
        DataRates_t drSet = buf[1];
        if (drSet > DR5_SF7_125kHz_LoRa) {
          drSet   = DR5_SF7_125kHz_LoRa;
        }

        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          g_framShadow1F.LinkADR_ADR_enabled = 0U;

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();

          /* Set all RX1 channels with manual DataRate */
          for (uint8_t idx = 0U; idx < 15U; idx++) {
            g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx] = drSet;
          }
        }
      }
      break;

    case LoraInQueueCmds__PwrRedDb:
      {
        uint8_t pwrRed = buf[1];
        if (pwrRed > 20) {
          pwrRed = 20;
        }

        /* Process LoRaWAN if enabled */
        if (ENABLE_MASK__LORAWAN_DEVICE  & g_enableMsk) {
          g_framShadow1F.LinkADR_ADR_enabled = 0U;
          g_framShadow1F.LinkADR_PwrRed = pwrRed;

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();

          /* Do a link check after current power reduction setting */
          if (loRaWANctx.FsmState == Fsm_NOP) {
            loRaWANctx.FsmState = Fsm_MAC_LinkCheckReq;
          }
        }
      }
      break;

    case LoraInQueueCmds__NOP:
    default:
      /* Nothing to do */
      { }
    }  // switch (buf[0])
  } while (!xQueueIsQueueEmptyFromISR(loraInQueueHandle));  // do {} while();

loRaWAN_Error_clrInBuf:
  {
    /* Clear the buffer to sync */
    bufCtr = bufMsgLen = 0;
    memset(buf, 0, sizeof(buf));
  }
}

#if 0
static void LoRaWAN_QueueOut_Process(uint8_t cmd)
{
  switch (cmd) {
    case LoraOutQueueCmds__Connected:
      {
        const uint8_t c_qM[2] = { 1, LoraOutQueueCmds__Connected };

        for (uint8_t idx = 0; idx < sizeof(c_qM); idx++) {
          xQueueSendToBack(loraOutQueueHandle, c_qM + idx, LoRaWAN_MaxWaitMs / portTICK_PERIOD_MS);
        }

        /* Set QUEUE_OUT bit */
        xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__QUEUE_OUT);  // TODO: controllerEventGroupHandle
      }
      break;
  }
}
#endif


#if 0
static uint32_t LoRaWAN_calc_FrqMHz_2_CFListEntry(float f)
{
  const uint32_t ui32 = f / 1e-4f;
  return ui32 & 0x00ffffffUL;
}
#endif

static float LoRaWAN_calc_CFListEntryWord_2_FrqMHz(uint32_t word)
{
  return 1e-4 * word;
}

static float LoRaWAN_calc_CFListEntry_2_FrqMHz(uint8_t packed[3])
{
  const uint32_t ui32 = (((uint32_t) (packed[2])) << 16) | (((uint32_t) (packed[1])) << 8) | packed[0];
  return LoRaWAN_calc_CFListEntryWord_2_FrqMHz(ui32);
}

static void LoRaWAN_calc_Decode_CFList(LoRaWANctx_t* ctx)
{
  /* Channels 3..7, each take 3 bytes  */
  for (uint8_t idx = 0; idx < 5U; idx++) {
    ctx->Ch_FrequenciesDownlink_MHz[idx + 3] = ctx->Ch_FrequenciesUplink_MHz[idx + 3] = LoRaWAN_calc_CFListEntry_2_FrqMHz((uint8_t*)(&g_framShadow1F.LoRaWAN_CfList[3 * idx]));
  }
}


uint8_t LoRaWAN_calc_randomChannel(LoRaWANctx_t* ctx)
{
  static uint8_t s_channel = 255U;                                                              // Init to use any channel within the channel mask
  uint8_t channel;

  do {
    channel = mainGetRand() % 16;

    if (!((1UL << channel) & g_framShadow1F.LinkADR_ChanMsk)) {
      /* Channel disabled, try again */
      channel = s_channel;
    }
  } while (channel == s_channel);
  s_channel = channel;

  return channel + 1;
}

float LoRaWAN_calc_Channel_to_MHz(LoRaWANctx_t* ctx, uint8_t channel, LoRaWANctxDir_t dir, uint8_t dflt)
{
  /* Sanity check */
  if (channel > 16) {
    return 0.f;
  }

  /* EU863-870*/
  float mhz = 0.f;

  switch (channel) {
  case 1:
    mhz = 868.100f;                                                                             // SF7BW125 to SF12BW125 - default value which never changes
    break;

  case 2:
    mhz = 868.300f;                                                                             // SF7BW125 to SF12BW125  and  SF7BW250 - default value which never changes
    break;

  case 3:
    mhz = 868.500f;                                                                             // SF7BW125 to SF12BW125 - default value which never changes
    break;

  case 4:
    mhz = 867.100f;                                                                             // SF7BW125 to SF12BW125
    break;

  case 5:
    mhz = 867.300f;                                                                             // SF7BW125 to SF12BW125
    break;

  case 6:
    mhz = 867.500f;                                                                             // SF7BW125 to SF12BW125
    break;

  case 7:
    mhz = 867.700f;                                                                             // SF7BW125 to SF12BW125
    break;

  case 8:
    mhz = 867.900f;                                                                             // SF7BW125 to SF12BW125
    break;

  case 9:
    mhz = 868.800f;                                                                             // FSK
    break;

  case  0:
  case 16:
    mhz = 869.525f;                                                                             // RX2 channel
    break;

  default:
    Error_Handler();
  }

  /* Current channel list */
  if (!dflt) {
    if ((1UL << (channel - 1)) & g_framShadow1F.LinkADR_ChanMsk) {
      /* Memorized value returned */
      mhz = (dir == Dn) ?
          ctx->Ch_FrequenciesDownlink_MHz[channel - 1] :
          ctx->Ch_FrequenciesUplink_MHz[channel - 1];
    }
  }

  /* Default value returned */
  return mhz;
}


#ifdef LORAWAN_1V1
static void LoRaWAN_calc_FOpts_Encrypt(LoRaWANctx_t* ctx,
    uint8_t* msg_FOpts_Encoded,
    const uint8_t* msg_FOpts_Buf, uint8_t msg_FOpts_Len)
{
  /* Short way out when nothing to do */
  if (!msg_prep_FOpts_Len) {
    return;
  }

  /* Create crypto xor matrix */
  uint8_t* key  = ctx->NwkSEncKey;

  FRMPayloadBlockA_Up_t a_i  = {
    0x01U,
    { 0x00U, 0x00U, 0x00U, 0x00U },
    (uint8_t) ctx->Dir,
    { ctx->DevAddr_LE[0],
      ctx->DevAddr_LE[1],
      ctx->DevAddr_LE[2],
      ctx->DevAddr_LE[3] },
    { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 2),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 3) },
    0x00,
    0x00
  };

  /* Create crypto modulator */
  uint8_t ecbPad[16] = { 0 };
  memcpy((void*)ecbPad, ((const void*) &a_i), msg_prep_FOpts_Len);
  cryptoAesEcb(key, ecbPad);

  /* Encode FOpts */
  for (uint8_t idx = 0; idx < msg_prep_FOpts_Len; idx++) {
    msg_prep_FOpts_Encoded[idx] = ecbPad[idx] ^ msg_prep_FOpts_Buf[idx];
  }
}
#endif

static uint8_t LoRaWAN_calc_FRMPayload_Encrypt(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg)
{
  /* Calculate the number of blocks needed */
  const uint8_t maxLen  = sizeof(msg->msg_prep_FRMPayload_Buf);
  const uint8_t len     = msg->msg_prep_FRMPayload_Len;
  const uint8_t blocks  = (len + 15U) >> 4;

  /* Leave if no FRMPayload exists */
  if(!len) {
    return 0;
  }

  /* Leave when target buffer is too small */
  if ((blocks << 4) > maxLen) {
    return 0;
  }

  /* Encrypt application data - "Table 3: FPort list" */
#ifdef LORAWAN_1V02
  const uint8_t* key = (ctx->FPort == 0) ?  (const uint8_t*) &g_framShadow1F.LoRaWAN_NWKSKEY : (const uint8_t*) &g_framShadow1F.LoRaWAN_APPSKEY;
#elif LORAWAN_1V1
  const uint8_t* key = (ctx->FPort == 0) ?  (const uint8_t*) &g_framShadow1F.LoRaWAN_NwkSEncKey : (const uint8_t*) &g_framShadow1F.LoRaWAN_APPSKEY;
#endif

  FRMPayloadBlockA_Up_t a_i  = {
    0x01U,
    { 0x00U, 0x00U, 0x00U, 0x00U },
    (uint8_t) ctx->Dir,                                                                         // "The direction field (Dir) is 0 for uplink frames and 1 for downlink frames"
    { g_framShadow1F.LoRaWAN_DevAddr[0],
      g_framShadow1F.LoRaWAN_DevAddr[1],
      g_framShadow1F.LoRaWAN_DevAddr[2],
      g_framShadow1F.LoRaWAN_DevAddr[3] },
    { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 2),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 3)},
    0x00,
    0x00                                                                                        // i: value of block index
  };

  /* Process all blocks */
  {
    for (uint8_t i = 1, blockPos = 0U; i <= blocks; i++, blockPos += 16U) {
      uint8_t ecbPad[16]  = { 0U };

      /* Update index to generate another crypto modulator for the next block */
      a_i.idx = i;

      /* Create crypto modulator
       * Si = aes128_encrypt(K, Ai) for i = 1..k
       * S  = S1 | S2 | .. | Sk                   */
      memcpy((void*)ecbPad, ((const void*)&a_i), 16);
      cryptoAesEcb_Encrypt(key, ecbPad);

      /* Encrypt data by XOR mask - "(pld | pad16) xor S" */
      for (uint8_t idx = 0; idx < 16; idx++) {
        msg->msg_prep_FRMPayload_Encoded[blockPos + idx] = msg->msg_prep_FRMPayload_Buf[blockPos + idx] ^ ecbPad[idx];

        /* Cut block on message size - "to the first len(pld) octets" */
        if ((blockPos + idx) >= (len - 1)) {
          return len;
        }
      }
    }
  }

  /* Should not happen */
  return 0;
}

static uint8_t LoRaWAN_calc_FRMPayload_Decrypt(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg)
{
  /* Calculate the number of blocks needed */
  const uint8_t maxLen  = sizeof(msg->msg_parted_FRMPayload_Buf);
  const uint8_t len     = msg->msg_parted_FRMPayload_Len;
  const uint8_t blocks  = (len + 15U) >> 4;

  /* Leave if no FRMPayload exists */
  if(!len) {
    return len;
  }

  /* Leave when target buffer is too small */
  if ((blocks << 4) > maxLen) {
    return 0;
  }

  /* Decrypt application data */
#ifdef LORAWAN_1V02
  const uint8_t* key = (ctx->FPort == 0) ?  (const uint8_t*) &g_framShadow1F.LoRaWAN_NWKSKEY : (const uint8_t*) &g_framShadow1F.LoRaWAN_APPSKEY;
#elif LORAWAN_1V1
  const uint8_t* key = (ctx->FPort == 0) ?  (const uint8_t*) &g_framShadow1F.LoRaWAN_NwkSEncKey : (const uint8_t*) &g_framShadow1F.LoRaWAN_APPSKEY;
#endif

  FRMPayloadBlockA_Dn_t a_i  = {
    0x01U,
    { 0x00U, 0x00U, 0x00U, 0x00U },
    (uint8_t) ctx->Dir,                                                                         // "The direction field (Dir) is 0 for uplink frames and 1 for downlink frames"
    { g_framShadow1F.LoRaWAN_DevAddr[0],
      g_framShadow1F.LoRaWAN_DevAddr[1],
      g_framShadow1F.LoRaWAN_DevAddr[2],
      g_framShadow1F.LoRaWAN_DevAddr[3] },
    { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Dn, 0),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Dn, 1),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Dn, 2),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Dn, 3)},
    0x00,
    0x00                                                                                        // i: value of block index
  };

  /* Process all blocks */
  {
    for (uint8_t i = 1, blockPos = 0U; i <= blocks; i++, blockPos += 16U) {
      uint8_t ecbPad[16]  = { 0U };

      /* Update index to generate another crypto modulator for the next block */
      a_i.idx = i;

      /* Create crypto modulator
       * Si = aes128_encrypt(K, Ai) for i = 1..k
       * S  = S1 | S2 | .. | Sk                   */
      memcpy((void*)ecbPad, ((const void*)&a_i), 16);
      cryptoAesEcb_Encrypt(key, ecbPad);

      /* Encrypt data by XOR mask - "(pld | pad16) xor S" */
      for (uint8_t idx = 0; idx < 16; idx++) {
        msg->msg_parted_FRMPayload_Buf[blockPos + idx] = msg->msg_parted_FRMPayload_Encoded[blockPos + idx] ^ ecbPad[idx];

        /* Cut block on message size - "to the first len(pld) octets" */
        if ((blockPos + idx) >= (len - 1)) {
          return len;
        }
      }
    }
  }

  /* Should not happen */
  return 0;
}

static void LoRaWAN_calc_TX_MIC(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg, uint8_t micOutPad[4], LoRaWAN_CalcMIC_VARIANT_t variant)
{
  switch (variant) {
  case MIC_JOINREQUEST:
    {
      uint8_t cmac[16];

      /*  V1.02: cmac = aes128_cmac(AppKey, MHDR | AppEUI  | DevEUI | DevNonce)
      /   V1.1:  cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
      /   MIC = cmac[0..3]                                                       */

      /* "msg" contains data for CMAC hashing already */
#ifdef LORAWAN_1V02
      cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_APPKEY, msg->msg_encoded_Buf,
          MHDR_SIZE + sizeof(g_framShadow1F.LoRaWAN_APP_JOIN_EUI) + sizeof(g_framShadow1F.LoRaWAN_DEV_EUI) + sizeof(g_framShadow1F.LoRaWAN_DevNonce),
          cmac);
#elif LORAWAN_1V1
      cryptoAesCmac(g_framShadow1F.LoRaWAN_NwkKey, msg->msg_encoded_Buf,
          sizeof(msg->msg_prep_MHDR) + sizeof(g_framShadow1F.LoRaWAN_APP_JOIN_EUI) + sizeof(g_framShadow1F.LoRaWAN_DEV_EUI) + sizeof(g_framShadow1F.LoRaWAN_DevNonce),
          cmac);
#endif

      /* MIC to buffer */
      for (uint8_t i = 0; i < 4; i++) {
        micOutPad[i] = cmac[i];
      }

      return;
    }
    break;

  case MIC_DATAMESSAGE:
    {
      uint8_t   cmacLen;
      uint8_t   cmacBuf[64] = { 0 };

      if (ctx->Dir == Up) {
        /* Uplink */

        MICBlockB0_Up_t b0 = {
          0x49,
          { 0x00, 0x00, 0x00, 0x00 },
          (uint8_t) Up,
          { g_framShadow1F.LoRaWAN_DevAddr[0],
            g_framShadow1F.LoRaWAN_DevAddr[1],
            g_framShadow1F.LoRaWAN_DevAddr[2],
            g_framShadow1F.LoRaWAN_DevAddr[3] },
          { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0),
            GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1),
            GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 2),
            GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 3)},
          0x00,
          msg->msg_encoded_Len
        };

        /* Concatenate b0 and msg */
        cmacLen = sizeof(MICBlockB0_Up_t);
        memcpy((void*)cmacBuf,            (const void*)&b0,                     cmacLen);
        memcpy((void*)cmacBuf + cmacLen,  (const void*)(msg->msg_encoded_Buf),  msg->msg_encoded_Len);  // "msg = MHDR | FHDR | FPort | FRMPayload"
        cmacLen += msg->msg_encoded_Len;

        uint8_t cmacF[16] = { 0 };
#ifdef LORAWAN_1V02
        /* "cmac = aes128_cmac(NwkSKey, B0 | msg)" */
        cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_NWKSKEY, cmacBuf, cmacLen, cmacF);
#elif LORAWAN_1V1
        if ((!(msg->msg_prep_FPort)) || (msg->msg_prep_FPort_absent)) {
          cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_NwkSKey, cmacBuf, cmacLen, cmacF);
        } else {
          cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_AppSKey, cmacBuf, cmacLen, cmacF);
        }
        cryptoAesCmac(g_framShadow1F.LoRaWAN_FNwkSIntKey, cmacBuf, cmacLen, cmacF);
#endif

        if (ctx->LoRaWAN_ver == LoRaWANVersion_10) {
          /* MIC to buffer */
          for (uint8_t i = 0; i < 4; i++) {
            micOutPad[i] = cmacF[i];
          }

          return;

#ifdef LORAWAN_1V1
        } else if (ctx->LoRaWAN_ver == LoRaWANVersion_11) {
          MICBlockB1_Up_t b1 = {
            0x49U,
            { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_FCntUp, 0),
              GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_FCntUp, 1) },
            ctx->TxDr,
            ctx->TxCh,
            (uint8_t) Up,
            { g_framShadow1F.LoRaWAN_DevAddr[0],
              g_framShadow1F.LoRaWAN_DevAddr[1],
              g_framShadow1F.LoRaWAN_DevAddr[2],
              g_framShadow1F.LoRaWAN_DevAddr[3] },
            { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0),
              GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1),
              GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 2),
              GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 3)},
            0x00,
            msg->msg_encoded_Len
          };

          /* Concatenate b1 and message */
          cmacLen = sizeof(MICBlockB1_Up_t);
          memcpy((void*)cmacBuf,            (const void*)&b1,                     cmacLen);
          memcpy((void*)cmacBuf + cmacLen,  (const void*)(msg->msg_encoded_Buf),  msg->msg_encoded_Len);
          cmacLen += msg->msg_encoded_Len;

          uint8_t cmacS[16];
          cryptoAesCmac(g_framShadow1F.LoRaWAN_SNwkSIntKey, cmacBuf, cmacLen, cmacS);

          /* MIC to buffer */
          micOutPad[0] = cmacS[0];
          micOutPad[1] = cmacS[1];
          micOutPad[2] = cmacF[0];
          micOutPad[3] = cmacF[1];

          return;
#endif
        }
      }

      /* Should not happen */
      Error_Handler();
    }
    break;

    default:
    {
      /* Nothing to do */
    }
  }  // switch ()
}

static void LoRaWAN_calc_RX_MIC(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg, uint8_t micOutPad[4])
{
  const uint8_t micLen      = sizeof(msg->msg_parted_MIC_Buf);
  uint8_t       cmacLen;
  uint8_t       cmacBuf[64] = { 0 };

  /* Downlink */

  uint32_t FCntDwn = g_framShadow1F.LoRaWAN_NFCNT_Dn;
  if ((!(msg->msg_parted_FCntDwn[0]) && (!(msg->msg_parted_FCntDwn[1]))) && ((FCntDwn & 0x0000ffffUL) == 0x0000ffffUL)) {
    FCntDwn += 0x00010000UL;
  }
  FCntDwn &= 0xffff0000UL;
  FCntDwn |= (int32_t)(msg->msg_parted_FCntDwn[0]);
  FCntDwn |= (int32_t)(msg->msg_parted_FCntDwn[1]) << 8;

  /* Write back to NVM */
  g_framShadow1F.LoRaWAN_NFCNT_Dn = FCntDwn;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

#ifdef LORAWAN_1V02
  MICBlockB0_Dn_t b0  = {
    0x49U,
    { 0x00U, 0x00U, 0x00U, 0x00U },
    (uint8_t) Dn,
    { msg->msg_parted_DevAddr[0],
      msg->msg_parted_DevAddr[1],
      msg->msg_parted_DevAddr[2],
      msg->msg_parted_DevAddr[3]},
    { GET_BYTE_OF_WORD(FCntDwn, 0),
      GET_BYTE_OF_WORD(FCntDwn, 1),
      GET_BYTE_OF_WORD(FCntDwn, 2),
      GET_BYTE_OF_WORD(FCntDwn, 3)},
    0x00,
    (msg->msg_encoded_Len - micLen)
  };
#elif LORAWAN_1V1
  MICBlockB0_Dn_t b0  = {
    0x49U,
    { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1)},
    { 0x00U, 0x00U },
    (uint8_t) Dn,
    { g_framShadow1F.LoRaWAN_DevAddr[0],
      g_framShadow1F.LoRaWAN_DevAddr[1],
      g_framShadow1F.LoRaWAN_DevAddr[2],
      g_framShadow1F.LoRaWAN_DevAddr[3] },
    { GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_AFCNT_Dn, 0),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_AFCNT_Dn, 1),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_AFCNT_Dn, 2),
      GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_AFCNT_Dn, 3)},
    0x00,
    msg->msg_encoded_Len
  };
#endif

  /* Concatenate b0 and message */
  cmacLen = sizeof(MICBlockB0_Dn_t);
  memcpy((void*)cmacBuf,            (const void*)&b0,                     cmacLen);
  memcpy((void*)cmacBuf + cmacLen,  (const void*)(msg->msg_encoded_Buf),  msg->msg_encoded_Len);
  cmacLen += (msg->msg_encoded_Len - micLen);

  uint8_t cmac[16] = { 0 };
#ifdef LORAWAN_1V02
  cryptoAesCmac((const uint8_t*)&g_framShadow1F.LoRaWAN_NWKSKEY, cmacBuf, cmacLen, cmac);
#elif LORAWAN_1V1
  if ((!(msg->msg_parted_FPort)) || (msg->msg_parted_FPort_absent)) {
    cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_SNwkSIntKey, cmacBuf, cmacLen, cmac);
  } else {
    cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_AppSKey,   cmacBuf, cmacLen, cmac);
#endif

  /* MIC to buffer */
  for (uint8_t i = 0; i < 4; i++) {
    micOutPad[i] = cmac[i];
  }
}

static void LoRaWAN_calc_TxMsg_Reset(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg)
{
  /* Reset timestamp of last transmission */
  ctx->TsEndOfTx = 0UL;

  /* Clear TX message buffer */
  memset(msg, 0, sizeof(LoRaWAN_TX_Message_t));

  /* Pre-sets for a new TX message */
  ctx->FCtrl_ADR              = g_framShadow1F.LinkADR_ADR_enabled;
  ctx->FCtrl_ADRACKReq        = 0;
  ctx->FCtrl_ACK              = 0;
  ctx->FCtrl_ClassB           = 0;
  ctx->FPort_absent           = 1;
  ctx->FPort                  = 0;
}

static void LoRaWAN_calc_TxMsg_Compiler_MHDR_JOINREQUEST(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg)
{
  /* JoinRequest does not have encryption and thus no packet compilation follows, instead write directly to msg_encoded[] */

  /* Start new message and write directly to msg_encoded[] - no packet compilation used here */
  memset(msg, 0, sizeof(LoRaWAN_TX_Message_t));

  /* MHDR */
  g_framShadow1F.LoRaWAN_MHDR_MType = JoinRequest;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  uint8_t l_MHDR = (g_framShadow1F.LoRaWAN_MHDR_MType << LoRaWAN_MHDR_MType_SHIFT) |
                   (g_framShadow1F.LoRaWAN_MHDR_Major << LoRaWAN_MHDR_Major_SHIFT);
  msg->msg_encoded_Buf[msg->msg_encoded_Len++]   = l_MHDR;

  for (uint8_t i = 0; i < 8; i++) {
#ifdef LORAWAN_1V02
    msg->msg_encoded_Buf[msg->msg_encoded_Len++] = g_framShadow1F.LoRaWAN_APP_JOIN_EUI[i];
#elif LORAWAN_1V1
    msg->msg_encoded_Buf[msg->msg_encoded_Len++] = JoinEUI_LE[i];
#endif
  }

  /* DevEUI[0:7] */
  for (uint8_t i = 0; i < 8; i++) {
    msg->msg_encoded_Buf[msg->msg_encoded_Len++] = g_framShadow1F.LoRaWAN_DEV_EUI[i];
  }

  /* DevNonce */
  {
#ifdef LORAWAN_1V02
    g_framShadow1F.LoRaWAN_DevNonce[0] = mainGetRand() & 0xffU;
    g_framShadow1F.LoRaWAN_DevNonce[1] = mainGetRand() & 0xffU;
#elif LORAWAN_1V1
    g_framShadow1F.LoRaWAN_DevNonce[0] = 0;
    g_framShadow1F.LoRaWAN_DevNonce[1] = 0;
#endif

    /* Write back to NVM */
    g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
    fram_updateCrc();

    msg->msg_encoded_Buf[msg->msg_encoded_Len++] = g_framShadow1F.LoRaWAN_DevNonce[0];
    msg->msg_encoded_Buf[msg->msg_encoded_Len++] = g_framShadow1F.LoRaWAN_DevNonce[1];
  }

  /* MIC */
  LoRaWAN_calc_TX_MIC(ctx, msg, (uint8_t*)(msg->msg_encoded_Buf + msg->msg_encoded_Len), MIC_JOINREQUEST);
  msg->msg_encoded_Len += 4;

  /* Packet now ready for TX */
}

static void LoRaWAN_calc_TxMsg_Compiler_Standard(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg)
{
  /* Start new encoded message */
  {
    msg->msg_encoded_Len = 0;
    memset((uint8_t*)msg->msg_encoded_Buf, 0, sizeof(msg->msg_encoded_Buf));
  }

  /* PHYPayload */
  {
    /* MHDR */
    {
      uint8_t l_MHDR = (g_framShadow1F.LoRaWAN_MHDR_MType << LoRaWAN_MHDR_MType_SHIFT) |
                       (g_framShadow1F.LoRaWAN_MHDR_Major << LoRaWAN_MHDR_Major_SHIFT);
      msg->msg_encoded_Buf[msg->msg_encoded_Len++] = l_MHDR;
    }

    /* MACPayload */
    {
      /* FHDR */
      {
        /* DevAddr */
        for (uint8_t i = 0; i < 4U; i++) {
          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = g_framShadow1F.LoRaWAN_DevAddr[i];
        }

        /* FCtrl */
        {
          /* Take global ADR setting */
          ctx->FCtrl_ADR = g_framShadow1F.LinkADR_ADR_enabled;

          uint8_t l_FCtrl = (ctx->FCtrl_ADR       << LoRaWAN_FCtl_ADR_SHIFT)       |
                            (ctx->FCtrl_ADRACKReq << LoRaWAN_FCtl_ADRACKReq_SHIFT) |
                            (ctx->FCtrl_ACK       << LoRaWAN_FCtl_ACK_SHIFT)       |
                            (ctx->TX_MAC_Len      << LoRaWAN_FCtl_FOptsLen_SHIFT );

          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = l_FCtrl;

          /* Reset acknowledges */
          ctx->FCtrl_ACK        = 0U;
          ctx->FCtrl_ADRACKReq  = 0U;
        }

        /* FCnt */
        {
          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 0);
          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = GET_BYTE_OF_WORD(g_framShadow1F.LoRaWAN_NFCNT_Up, 1);
        }

        /* FOpts (V1.1: encoded) - not emitted when msg_FOpts_Len == 0 */
        {
#ifdef LORAWAN_1V02
          for (uint8_t l_FOpts_Idx = 0; l_FOpts_Idx < ctx->TX_MAC_Len; l_FOpts_Idx++) {
            msg->msg_encoded_Buf[msg->msg_encoded_Len++] = ctx->TX_MAC_Buf[l_FOpts_Idx];
          }
#elif LORAWAN_1V1
          if (msg->msg_prep_FOpts_Len) {
            LoRaWAN_calc_FOpts_Encrypt(ctx, msg->msg_prep_FOpts_Encoded, msg->msg_prep_FOpts_Buf, msg->msg_prep_FOpts_Len);

            for (uint8_t FOpts_Idx = 0; FOpts_Idx < msg->msg_prep_FOpts_Len; FOpts_Idx++) {
              msg->msg_encoded_Buf[msg->msg_encoded_Len++]  = msg->msg_prep_FOpts_Encoded[FOpts_Idx];
            }
          }
#endif

          /* Reset for next entries to be filled in */
          ctx->TX_MAC_Len = 0U;
          memset((uint8_t*) ctx->TX_MAC_Buf, 0, sizeof(ctx->TX_MAC_Buf));
        }  // FOpts
      }  // FHDR

      /* FPort - not emitted when FPort_absent is set */
      {
        if (msg->msg_prep_FRMPayload_Len) {
          ctx->FPort_absent = 0U;
          ctx->FPort        = FPort_Appl_Default;
        } else {
          ctx->FPort_absent = 1U;
        }

        if (!(ctx->FPort_absent)) {
          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = ctx->FPort;
        }
      }

      /* FRMPayload - not emitted when msg_prep_FRMPayload_Len == 0 */
      if (msg->msg_prep_FRMPayload_Len) {
        /* Encode FRMPayload*/
        LoRaWAN_calc_FRMPayload_Encrypt(ctx, msg);

        /* Copy into target msg_encoded_Buf[] */
        for (uint8_t l_FRMPayload_Idx = 0; l_FRMPayload_Idx < msg->msg_prep_FRMPayload_Len; l_FRMPayload_Idx++) {
          msg->msg_encoded_Buf[msg->msg_encoded_Len++] = msg->msg_prep_FRMPayload_Encoded[l_FRMPayload_Idx];
        }
      }
    }

    /* MIC */
    {
      LoRaWAN_calc_TX_MIC(ctx, msg, (uint8_t*)(msg->msg_encoded_Buf + msg->msg_encoded_Len), MIC_DATAMESSAGE);
      msg->msg_encoded_Len += 4U;
    }
  }

  /* Set flag for Encoding Done */
  msg->msg_encoded_EncDone = 1U;

  /* Packet now ready for TX */
}


static void LoRaWAN_calc_RxMsg_Reset(LoRaWAN_RX_Message_t* msg)
{
  /* Clear RX message buffer */
  memset(msg, 0, sizeof(LoRaWAN_RX_Message_t));
}

static uint8_t LoRaWAN_calc_RxMsg_Decoder_MHDR_JOINACCEPT(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg)
{
  /* Process the message */
  if (msg->msg_encoded_Len) {
    uint8_t decPad[64] = { 0 };
    uint8_t micPad[16] = { 0 };

    memcpy((void*)decPad, (const void*)msg->msg_encoded_Buf, msg->msg_encoded_Len);

    uint8_t ecbCnt = (msg->msg_encoded_Len - 1 + 15) / 16;
    for (uint8_t i = 0, idx = 1; i < ecbCnt; i++, idx += 16) {
      cryptoAesEcb_Encrypt((const uint8_t*)g_framShadow1F.LoRaWAN_APPKEY, (uint8_t*)&decPad + idx);                   // Reversed operation as explained in 6.2.5
    }
    memset(decPad + msg->msg_encoded_Len, 0, sizeof(decPad) - msg->msg_encoded_Len);

    /* Check MIC */
    cryptoAesCmac((const uint8_t*)g_framShadow1F.LoRaWAN_APPKEY, decPad, msg->msg_encoded_Len - 4, micPad);
    uint8_t micMatch = 1;
    for (uint8_t idx = 0; idx < 4; idx++) {
      if (micPad[idx] != decPad[idx + 29]) {
        micMatch = 0;
        break;
      }
    }

    /* Process the data */
    if (micMatch) {
      uint8_t encPad[16] = { 0 };
      uint8_t keyPad[16] = { 0 };

      /* Increment uplink FCnt as long as link is not established */
      g_framShadow1F.LoRaWAN_NFCNT_Up++;

      /* Copy values into own device context */
      uint8_t decPadIdx = 0;
      g_framShadow1F.LoRaWAN_MHDR_MType             = (decPad[decPadIdx  ] & 0xe0) >> LoRaWAN_MHDR_MType_SHIFT;
      g_framShadow1F.LoRaWAN_MHDR_Major             = (decPad[decPadIdx++] & 0x03) >> LoRaWAN_MHDR_Major_SHIFT;
      //
      g_framShadow1F.LoRaWAN_AppNonce[0]            = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_AppNonce[1]            = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_AppNonce[2]            = decPad[decPadIdx++];
      //
      g_framShadow1F.LoRaWAN_NetID[0]               = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_NetID[1]               = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_NetID[2]               = decPad[decPadIdx++];
      //
      g_framShadow1F.LoRaWAN_DevAddr[0]             = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_DevAddr[1]             = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_DevAddr[2]             = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_DevAddr[3]             = decPad[decPadIdx++];
      //
      uint8_t dlSettings 									          = decPad[decPadIdx++];
      g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[16 - 1]   = g_framShadow1F.LinkADR_DR_RXTX2 = (DataRates_t) (dlSettings & 0x0f);
      g_framShadow1F.LinkADR_DR_RX1_DRofs                                             =               (dlSettings & 0x70) >> 4;
      //
      g_framShadow1F.LoRaWAN_rxDelay = decPad[decPadIdx++];
      //
      for (uint8_t cfIdx = 0; cfIdx < sizeof(g_framShadow1F.LoRaWAN_CfList); cfIdx++) {
        g_framShadow1F.LoRaWAN_CfList[cfIdx] = decPad[decPadIdx++];
      }
      LoRaWAN_calc_Decode_CFList(ctx);

      /* Write back to NVM */
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
      fram_updateCrc();


      /* RX2: DataRate setting */

      /* NwkSKey = aes128_encrypt(AppKey, 0x01 | AppNonce | NetID | DevNonce | pad16) */
      encPad[0] = 0x01;
      memcpy(&(encPad[1]), (uint8_t*)g_framShadow1F.LoRaWAN_AppNonce,  sizeof(g_framShadow1F.LoRaWAN_AppNonce));
      memcpy(&(encPad[4]), (uint8_t*)g_framShadow1F.LoRaWAN_NetID,     sizeof(g_framShadow1F.LoRaWAN_NetID));
      memcpy(&(encPad[7]), (uint8_t*)g_framShadow1F.LoRaWAN_DevNonce,  sizeof(g_framShadow1F.LoRaWAN_DevNonce));
      //
      memcpy(keyPad, encPad, sizeof(keyPad));
      cryptoAesEcb_Encrypt((const uint8_t*)g_framShadow1F.LoRaWAN_APPKEY, keyPad);
      memcpy((uint8_t*)g_framShadow1F.LoRaWAN_NWKSKEY, (uint8_t*)keyPad, sizeof(g_framShadow1F.LoRaWAN_NWKSKEY));

      /* AppSKey = aes128_encrypt(AppKey, 0x02 | AppNonce | NetID | DevNonce | pad16) */
      encPad[0] = 0x02;
      //
      memcpy(keyPad, encPad, sizeof(keyPad));
      cryptoAesEcb_Encrypt((const uint8_t*)g_framShadow1F.LoRaWAN_APPKEY, keyPad);
      memcpy((uint8_t*)g_framShadow1F.LoRaWAN_APPSKEY, (uint8_t*)keyPad, sizeof(g_framShadow1F.LoRaWAN_APPSKEY));

      return HAL_OK;
    }
  }
  return HAL_ERROR;
}

static uint8_t LoRaWAN_calc_RxMsg_Decoder_Standard(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg)
{
  const uint8_t micLen = sizeof(msg->msg_parted_MIC_Buf);

  /* PHYPayload access */
  uint8_t idx       = 0;
  uint8_t micValid  = 1;

  /* Initial FRMPayload length calculation */
  msg->msg_parted_FRMPayload_Len = msg->msg_encoded_Len;

  /* PHYPayload */
  {
    /* MHDR */
    {
      msg->msg_parted_MHDR = msg->msg_encoded_Buf[idx++];
      msg->msg_parted_FRMPayload_Len--;

      /* MType */
      msg->msg_parted_MType = msg->msg_parted_MHDR >> LoRaWAN_MHDR_MType_SHIFT;

      /* Major */
      msg->msg_parted_Major = msg->msg_parted_MHDR & 0b11;

      if ((msg->msg_parted_MType == Proprietary) ||
          (msg->msg_parted_Major != LoRaWAN_R1)
      ) {
        return 0;
      }
    }

    /* MACPayload */
    {
      /* FHDR */
      {
        /* DevAddr */
        {
          memcpy(msg->msg_parted_DevAddr, (uint8_t*)(msg->msg_encoded_Buf + idx), sizeof(msg->msg_parted_DevAddr));

          const uint8_t DevAddrSize        = sizeof(msg->msg_parted_DevAddr);
          idx                             += DevAddrSize;
          msg->msg_parted_FRMPayload_Len  -= DevAddrSize;
        }

        /* FCtrl */
        {
          msg->msg_parted_FCtrl = msg->msg_encoded_Buf[idx++];
          msg->msg_parted_FRMPayload_Len--;

          msg->msg_parted_FCtrl_ADR      = 0x01 & (msg->msg_parted_FCtrl >> LoRaWAN_FCtl_ADR_SHIFT);
          msg->msg_parted_FCtrl_ACK      = 0x01 & (msg->msg_parted_FCtrl >> LoRaWAN_FCtl_ACK_SHIFT);
          msg->msg_parted_FCtrl_FPending = 0x01 & (msg->msg_parted_FCtrl >> LoRaWAN_FCtl_FPending_SHIFT);
          msg->msg_parted_FCtrl_FOptsLen = 0x0f & (msg->msg_parted_FCtrl >> LoRaWAN_FCtl_FOptsLen_SHIFT);
        }

        /* FCnt */
        {
          msg->msg_parted_FCntDwn[0]       = msg->msg_encoded_Buf[idx++];
          msg->msg_parted_FCntDwn[1]       = msg->msg_encoded_Buf[idx++];
          msg->msg_parted_FRMPayload_Len  -= sizeof(msg->msg_parted_FCntDwn);
        }

        /* FOpts */
        {
#ifdef LORAWAN_1V02
          for (uint8_t FOptsIdx = 0; FOptsIdx < msg->msg_parted_FCtrl_FOptsLen; FOptsIdx++) {
            msg->msg_parted_FOpts_Buf[FOptsIdx] = msg->msg_encoded_Buf[idx++];
          }
          msg->msg_parted_FRMPayload_Len -= msg->msg_parted_FCtrl_FOptsLen;
#elif LORAWAN_1V1
          /* Decrypt FOpts */

          msg->msg_parted_FRMPayload_Len -= msg->msg_parted_FCtrl_FOptsLen;
#endif
        }
      }

      /* Subtract MIC length to finally get the FRMPayload length */
      msg->msg_parted_FRMPayload_Len -= micLen;

      /* Are optional parts absent? */
      if (2 > msg->msg_parted_FRMPayload_Len) {
        msg->msg_parted_FPort_absent    = 1;
        msg->msg_parted_FPort           = 0;
        msg->msg_parted_FRMPayload_Len  = 0;

      } else {
        /* optional FPort */
        {
          msg->msg_parted_FPort_absent  = 0;
          msg->msg_parted_FPort         = msg->msg_encoded_Buf[idx++];
          msg->msg_parted_FRMPayload_Len--;
        }

        /* Copy optional encoded FRMPayload to its section */
        {
          memcpy(msg->msg_parted_FRMPayload_Encoded, (uint8_t*)(msg->msg_encoded_Buf + idx), msg->msg_parted_FRMPayload_Len);
          idx += msg->msg_parted_FRMPayload_Len;
        }
      }
    }

    /* MIC */
    {
      uint8_t micComparePad[4] = { 0 };

      memcpy(msg->msg_parted_MIC_Buf, (uint8_t*)(msg->msg_encoded_Buf + idx), micLen);
      LoRaWAN_calc_RX_MIC(ctx, msg, micComparePad);

      for (uint8_t micIdx = 0; micIdx < micLen; micIdx++) {
        if (micComparePad[micIdx] != msg->msg_parted_MIC_Buf[micIdx]) {
          micValid = 0;
          break;
        }
      }
      msg->msg_parted_MIC_Valid = micValid;
    }
  }

  /* Data packet valid */
  if (!micValid) {
    return 0;
  }

  /* Confirmed frame does update frame counter */
  {
    if (ctx->ConfirmedPackets_enabled) {
      if (loRaWanRxMsg.msg_parted_FCtrl_ACK) {
        /* FCntUp */
        g_framShadow1F.LoRaWAN_NFCNT_Up++;
        // NVM update follows

#ifdef LORAWAN_1V1
        if (g_framShadow1F.LoRaWAN_MHDR_MType == JoinAccept)
        if (!(++(g_framShadow1F.LoRaWAN_DevNonce[0]))) {
          ++(g_framShadow1F.LoRaWAN_DevNonce[1]);
        }
#endif
      }

    } else {  // if (ctx->ConfirmedPackets_enabled) else
      /* FCntUp */
      g_framShadow1F.LoRaWAN_NFCNT_Up++;
      // NVM update follows
    }

    /* FCntDwn */
    {
      const uint32_t FCntDwnLast  = g_framShadow1F.LoRaWAN_NFCNT_Dn;
      uint32_t FCntDwnNew         = FCntDwnLast;
      uint32_t FCntDwn16          = (loRaWanRxMsg.msg_parted_FCntDwn[0]) | ((uint32_t)loRaWanRxMsg.msg_parted_FCntDwn[1] << 8);

      /* 16-bit roll-over handling */
      if (((FCntDwnLast & 0x0000ffffUL) == 0x0000ffffUL) && !FCntDwn16) {
        FCntDwnNew += 0x00010000UL;
      }

      FCntDwnNew &= 0xffff0000UL;
      FCntDwnNew |= 0x0000ffffUL & FCntDwn16;

#if LORAWAN_1V1
      if (FCntDwnNew != (1 + FCntDwnLast)) {
        /* Unexpected FCntDwn - alert */
        return 0;
      }
#endif

      /* Write to backup register */
      g_framShadow1F.LoRaWAN_NFCNT_Dn = FCntDwnNew;
    }

    /* Write back to NVM */
    fram_updateCrc();
  }

  /* Decrypt the FRMPayload */
  return LoRaWAN_calc_FRMPayload_Decrypt(ctx, msg);
}


void LoRaWAN_MAC_Queue_Push(const uint8_t* macBuf, uint8_t cnt)
{
  /* Send MAC commands with their options */
  for (uint8_t idx = 0; idx < cnt; ++idx) {
    BaseType_t xStatus = xQueueSendToBack(loraMacQueueHandle, macBuf + idx, LoRaWAN_MaxWaitMs / portTICK_PERIOD_MS);
    if (pdTRUE != xStatus) {
      Error_Handler();
    }
  }
}

void LoRaWAN_MAC_Queue_Pull(uint8_t* macBuf, uint8_t cnt)
{
  /* Receive MAC commands with their options */
  for (uint8_t idx = 0; idx < cnt; ++idx) {
    BaseType_t xStatus = xQueueReceive(loraMacQueueHandle, macBuf + idx, LoRaWAN_MaxWaitMs / portTICK_PERIOD_MS);
    if (pdTRUE != xStatus) {
      Error_Handler();
    }
  }
}

void LoRaWAN_MAC_Queue_Reset(void)
{
  xQueueReset(loraMacQueueHandle);
}

#if 0
uint8_t LoRaWAN_MAC_Queue_isAvail(uint8_t* snoop)
{
  uint8_t* fill = snoop;
  uint8_t pad;

  if (!snoop) {
    fill = &pad;
  }

  return pdTRUE == xQueuePeek(loraMacQueueHandle, fill, 1);
}
#endif


static void LoRaWAN_InitNVMtemplate(void)
{
  memset(&g_framShadow1F, 0, sizeof(g_framShadow1F));

  /* Three numbers to be configured by the TTN console */
  memcpy((void*) &(g_framShadow1F.LoRaWAN_APP_JOIN_EUI),  AppEUI_LE, sizeof(g_framShadow1F.LoRaWAN_APP_JOIN_EUI));
  memcpy((void*) &(g_framShadow1F.LoRaWAN_DEV_EUI),       DevEUI_LE, sizeof(g_framShadow1F.LoRaWAN_DEV_EUI));
  memcpy((void*) &(g_framShadow1F.LoRaWAN_APPKEY),        AppKey_BE, sizeof(g_framShadow1F.LoRaWAN_APPKEY));

  /* Default DataRates */
  {
    for (uint8_t ch = 1; ch <= 8; ch++) {
      const uint8_t idx = ch - 1;
      g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx]    = g_framShadow1F.LoRaWAN_Ch_DR_TX_min[idx]      = DR0_SF12_125kHz_LoRa;
      g_framShadow1F.LoRaWAN_Ch_DR_TX_max[idx]                                                    = DR5_SF7_125kHz_LoRa;
    }

    /* RX2 - Default channel */
    g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[16 - 1]   = g_framShadow1F.LoRaWAN_Ch_DR_TX_min[16 - 1]   = DR0_SF12_125kHz_LoRa;
    g_framShadow1F.LoRaWAN_Ch_DR_TX_max[16 - 1]                                                   = DR5_SF7_125kHz_LoRa;
  }

  g_framShadow1F.LoRaWAN_MHDR_MType       = ConfDataUp;                                       // Confirmed data transport in use
  g_framShadow1F.LoRaWAN_MHDR_Major       = LoRaWAN_R1;                                       // Major release in use

  g_framShadow1F.LinkADR_ADR_enabled      = LORAWAN_ADR_ENABLED_DEFAULT;                      // Global setting for ADR
  g_framShadow1F.LinkADR_PwrRed           = 0;                                                // No power reduction
  g_framShadow1F.LinkADR_DR_TX1           = g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[ 1 - 1];      // RX1 - Channel 1 as an example
  g_framShadow1F.LinkADR_DR_RX1_DRofs     = 0;                                                // Default (SF12)
  g_framShadow1F.LinkADR_DR_RXTX2         = g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[16 - 1];      // RX2
  g_framShadow1F.LinkADR_ChanMsk          = 0x0007U;                                          // Enable default channels (1..3) only
  g_framShadow1F.LinkADR_NbTrans          = 1;                                                // Number of repetitions for unconfirmed packets
  g_framShadow1F.LinkADR_ChanMsk_Cntl     = ChMaskCntl__appliesTo_1to16;                      // Mask settings 1..16 are valid

  g_framShadow1F.SX_XTAL_Drift            = 0.0f;                                             // Real value of this device
//g_framShadow1F.SX_XTA_Cap               = 0x1bU;
//g_framShadow1F.SX_XTB_Cap               = 0x1bU;
  g_framShadow1F.SX_XTA_Cap               = 0x10U;
  g_framShadow1F.SX_XTB_Cap               = 0x10U;

  g_framShadow1F.loraEventGroup           = (Lora_EGW_BM_t) Lora_EGW__DATAPRESET_OK;

  /* Write back to NVM */
  fram_updateCrc();
}

static void LoRaWANctx_readNVM(void)
{
  /* Drift of the Gateway PPM */
//loRaWANctx.GatewayPpm                           = -0.70f;                                     // Gateway drift
  loRaWANctx.GatewayPpm                           =  0.00f;                                     // Gateway drift

#ifdef LORAWAN_1V02
  loRaWANctx.LoRaWAN_ver                          = LoRaWANVersion_10;
#elif defined LORAWAN_1V1
  loRaWANctx.LoRaWAN_ver                          = LoRaWANVersion_11;
#endif

  /* Current transmission state */
  {
    loRaWANctx.Dir                                = Up;
    loRaWANctx.Current_RXTX_Window                = CurWin_none;                                // out of any window
    loRaWANctx.FPort_absent                       = 1U;                                         // Without FRMPayload this field is disabled
    loRaWANctx.FPort                              = 1U;                                         // Default application port
    loRaWANctx.ConfirmedPackets_enabled           = LORAWAN_CONFPACKS_DEFAULT;                  // ConfDataUp to be used
  }

  /* Import loraEventGroup */
  g_framShadow1F.loraEventGroup &= ~Lora_EGW__TRANSPORT_READY;
  xEventGroupSetBits(loraEventGroupHandle, g_framShadow1F.loraEventGroup);
}

static void generateFrequencyTable(void)
{
  /* RX1 - Copy default channel settings */
  for (uint8_t ch = 1; ch <= 8; ch++) {
    const uint8_t idx = ch - 1;
    if (idx < 3U) {
      loRaWANctx.Ch_FrequenciesDownlink_MHz[idx]  = loRaWANctx.Ch_FrequenciesUplink_MHz[idx]  = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, ch, Up, 1);  // Default values / default channels

    } else {
      uint8_t val[4] = { 0 };
      val[0] = g_framShadow1F.LoRaWAN_CfList[3U * (idx - 3U) + 0U];
      val[1] = g_framShadow1F.LoRaWAN_CfList[3U * (idx - 3U) + 1U];
      val[2] = g_framShadow1F.LoRaWAN_CfList[3U * (idx - 3U) + 2U];

      const float memFrq = LoRaWAN_calc_CFListEntryWord_2_FrqMHz( *((uint32_t*) val) );
      loRaWANctx.Ch_FrequenciesDownlink_MHz[idx] = loRaWANctx.Ch_FrequenciesUplink_MHz[idx] = (memFrq ?  memFrq : LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, ch, Up, 1));
    }
  }
}


//#define DEBUG_TX_PWR

/* Push the complete message to the FIFO and go to transmission mode */
static void LoRaWAN_TX_msg(LoRaWANctx_t* ctx, LoRaWAN_TX_Message_t* msg)
{
  const float     frq     = ctx->Ch_FrequenciesUplink_MHz[ctx->Ch_Selected - 1U];
#ifdef DEBUG_TX_PWR
  const  int8_t   pwr     = 14;
#else
  const  int8_t   pwr     = 14 - g_framShadow1F.LinkADR_PwrRed;
#endif
  const uint8_t   SF      = LoRaWAN_DRtoSF(g_framShadow1F.LinkADR_DR_TX1);
  const uint8_t   LDRO    = (SF >= 11U) ?  1U : 0U;

  sxTxRxPreps(ctx, SX_Mode_TX,
    msg->msg_encoded_Buf, msg->msg_encoded_Len,
    frq,
    pwr,
    4U,
    SF, LDRO,
    g_framShadow1F.SX_XTA_Cap, g_framShadow1F.SX_XTB_Cap);

  /* Transmitter on */
  {
    /* Turn on transmitter */
    const uint32_t tsStartOfTx = sxStartTXFromBuf(LORAWAN_EU868_MAX_TX_DURATION_MS);

    /* Wait until TX has finished - the transceiver changes to STANDBY by itself - give 10ms extra for external abortion */
    ctx->TsEndOfTx = sxWaitUntilTxDone(tsStartOfTx + LORAWAN_EU868_MAX_TX_DURATION_MS + 10UL);
  }
}


static void LoRaWAN_RX_msg(LoRaWANctx_t* ctx, LoRaWAN_RX_Message_t* msg, uint32_t diffToTxStartMs, uint32_t diffToTxStopMs)
{
  /* Sanity checks - no receiver operation w/o transmission before */
  if (!ctx || !ctx->TsEndOfTx || !msg) {
    return;
  }

  /* Clear receiving message buffer */
  LoRaWAN_calc_RxMsg_Reset(msg);

  const uint32_t    endOfTx       = ctx->TsEndOfTx;
  TickType_t        lastWakeupAbs = endOfTx;
  const TickType_t  nextWakeupRel = diffToTxStartMs - LORAWAN_RX1_PREPARE_MS;
  const TickType_t  nextWakeupAbs = endOfTx + nextWakeupRel;
  uint32_t          tsNow         = xTaskGetTickCount();

  /* Receive window */
  if (tsNow < nextWakeupAbs) {
    /* Sleep until Receiver is needed */
      vTaskDelayUntil(&lastWakeupAbs, nextWakeupRel / portTICK_PERIOD_MS);
  }

  /* Prepare RX */
  const float     frq     = ctx->FrequencyMHz     = 0.0f;                                       // Force recalculation
  const uint8_t   SF      = ctx->SpreadingFactor  = 0U;                                         // Force recalculation

  sxTxRxPreps(ctx, (ctx->Current_RXTX_Window == CurWin_RXTX2 ?  SX_Mode_RX2 : SX_Mode_RX),
      NULL, 0U,
      frq,
      0U,
      4U,
      SF, 0U,
      g_framShadow1F.SX_XTA_Cap, g_framShadow1F.SX_XTB_Cap);

  /* Receiver on */
  {
    tsNow = xTaskGetTickCount();

    const uint32_t txStopAbs  = (endOfTx + diffToTxStopMs);
    const uint32_t rxLenMs    = (txStopAbs - tsNow) - 2UL;

    /* Turn on receiver and wait for the message in this time span */
    sxStartRXToBuf(rxLenMs);
    sxWaitUntilRxDone(ctx, msg, txStopAbs);
  }
}


//#define RF_TEST_PREAMBLE
//#define RF_TEST_PREAMBLE_SWEEP
//#define TEST_FRAM_ACCESS
void loRaWANLoraTaskInit(void)
{
#ifdef RF_TEST_PREAMBLE
  {
    uint8_t buf[4] = { 0 };

    for (uint8_t cnt = 2U; cnt; --cnt) {
      sxTxRxPreps(&loRaWANctx, SX_Mode_TX,
        buf, 4,                                                                                   // not used
        868.500f,                                                                                 // QRG
        +14,                                                                                      // dBm
        4U,                                                                                       // 125 kHz
        7, false,                                                                                 // SF, LDRO
        0x1fU, 0x1fU);                                                                            // XTA / XTB capacitors for the XTAL
    }

    /* Transmitter on */
#if 0
    sxTestTXInfinitePreamble();
#else
    sxTestTXContinuesWave();
#endif

#ifdef RF_TEST_PREAMBLE_SWEEP
    for (float f = 866.000f; f <= 870.000f; f += 0.010f) {
      sxSetFrequencySetMHz(f);
      osDelay(25UL);
    }
#endif

    /* SetStandby(STDBY_XOSC) */
    const uint8_t bufSxSetStdbyXosc[2] = { 0x80U, 0x01U };
    spiProcessSpi3MsgTemplate(SPI3_SX1261, sizeof(bufSxSetStdbyXosc), bufSxSetStdbyXosc);
    spiSxWaitUntilReady();

    while (1) {
      osDelay(1000UL);
    }
  }
#endif

#ifdef TEST_FRAM_ACCESS
  /* FRAM check */
  volatile uint8_t* cp = (uint8_t*) &g_framShadow1F;
  for (int idx = 0; idx < sizeof(g_framShadow1F); idx++, cp++) {
    *cp = (uint8_t) (255 - idx);
  }

  //g_framShadow1F.crc32 = 0xe8acb9c2UL;
  //fram_writeBuf(&g_framShadow1F, FRAM_MEM_1F00_BORDER, sizeof(g_framShadow1F));
  fram_updateCrc();

  __DSB();
  __ISB();

  memset(&g_framShadow1F, 0, sizeof(g_framShadow1F));
  fram_readBuf(&g_framShadow1F, FRAM_MEM_1F00_BORDER, sizeof(g_framShadow1F));

  int ctr = 0;
  while (1) {
    ++ctr;

    if (fram_check1Fcrc(true)) {
      __NOP();
    }
  }

  while (1)
    ;
#endif

  /* Clear queue */
  uint8_t inChr = 0;
  while (xQueueReceive(loraInQueueHandle, &inChr, 0) == pdPASS) {
  }

  /* Init LoRaWAN */
  {
    /* Prepare LoRaWAN context */
    if (fram_check1Fcrc(true)) {
      /* Init FRAM */
      LoRaWAN_InitNVMtemplate();
    }

    /* Setup data from FRAM NVM */
    LoRaWANctx_readNVM();

    /* Set-up LoRaWAN context */
    {
      /* NVM data to expanded frequency table */
      generateFrequencyTable();

      /* Default FSM state when everything is ready for LoRaWAN data transfer */
      loRaWANctx.FsmState = Fsm_NOP;


      /* Check connection state */
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);

      if (!(Lora_EGW__JOINACCEPT_OK & g_framShadow1F.loraEventGroup)) {
        /* Start with JOIN-REQUEST */
        loRaWANctx.FsmState = Fsm_MAC_JoinRequest;

      } else {
        if (!(Lora_EGW__LINKCHECK_OK & g_framShadow1F.loraEventGroup)) {
          /* Request LoRaWAN link check and network time */
          xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_LINKCHECKREQ);

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();

          /* Start with LINK_CHECK_REQUEST */
          loRaWANctx.FsmState = Fsm_MAC_LinkCheckReq;
        }

        #if 0
        else if (!(Lora_EGW__DEVICETIME_OK & g_framShadow1F.loraEventGroup)) {
          /* Request LoRaWAN network time */
          xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_DEVICETIMEREQ);

          /* Write back to NVM */
          g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
          fram_updateCrc();

          /* Start with LINK_CHECK_REQUEST */
          loRaWANctx.FsmState = Fsm_MAC_DeviceTimeReq;
        }
        #endif

        else if (Lora_EGW__LINK_ESTABLISHED & g_framShadow1F.loraEventGroup) {
          /* Ready to start LoRaWAN transport */
          xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__TRANSPORT_READY);
        }
      }
    }
  }
}


static void loRaWANLoRaWANTaskLoop__Fsm_RX1(void)
{
  if (loRaWANctx.TsEndOfTx) {
    /* TX response after DELAY1 at RX1 - switch on receiver */

    /* USB: info */
    //usbLogLora("\r\nLoRaWAN: RX1 (1 s).\r\n");

    /* Adjust the context */
    loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
    loRaWANctx.Dir                  = Dn;
//  loRaWANctx.Ch_Selected          = ;                                                         // Keep same channel of TX before

    /* Gateway response after DELAY1 at RX1 - switch on receiver */
    LoRaWAN_RX_msg(&loRaWANctx, &loRaWanRxMsg,
        LORAWAN_EU868_DELAY1_MS,
        LORAWAN_EU868_DELAY1_MS + LORAWAN_EU868_MAX_RX1_DURATION_MS);                           // Same frequency and SF as during transmission

    if (loRaWanRxMsg.msg_encoded_Len == 0) {
      /* Listen to next window */
      loRaWANctx.Current_RXTX_Window  = CurWin_RXTX2;
      loRaWANctx.FsmState             = Fsm_RX2;

    } else {
      /* USB: info */
      //usbLogLora("LoRaWAN: Received packet within RX1 window.\r\n");
      //usbLogLora("LoRaWAN: (TRX off)\r\n");

      /* Process message */
      loRaWANctx.FsmState = Fsm_MAC_Decoder;
    }
  }
}

static void loRaWANLoRaWANTaskLoop__Fsm_RX2(void)
{
  if (loRaWANctx.TsEndOfTx) {
    /* TX response after DELAY2 at RX2 */

    /* USB: info */
    //usbLogLora("\r\nLoRaWAN: RX2 (2 s).\r\n");

    /* Adjust the context */
    loRaWANctx.Current_RXTX_Window  = CurWin_RXTX2;
    loRaWANctx.Dir                  = Dn;
    loRaWANctx.Ch_Selected          = 16;                                                       // Jump to RX2 channel

    /* Gateway response after DELAY2 at RX2 */
    LoRaWAN_RX_msg(&loRaWANctx, &loRaWanRxMsg,
        LORAWAN_EU868_DELAY2_MS,
        LORAWAN_EU868_DELAY2_MS + LORAWAN_EU868_MAX_TX_DURATION_MS);

    /* Reset SX1261 and keep in that state */
    sxReset(true);

    if (loRaWanRxMsg.msg_encoded_Len == 0) {
      /* No message received - try again if packet is of confirmed type */
      if (ConfDataUp == g_framShadow1F.LoRaWAN_MHDR_MType) {
        /* USB: info */
        //usbLogLora("LoRaWAN: Failed to RX.\r\n\r\n");

        /* Work on MAC queue to clean it up */
        loRaWANctx.FsmState = Fsm_MAC_Proc;

      } else {
        /* Increment uplink FCnt */
        g_framShadow1F.LoRaWAN_NFCNT_Up++;

        /* Write back to NVM */
        g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
        fram_updateCrc();

        /* USB: info */
        //usbLogLora("\r\n\r\n");

        /* Work on MAC queue to clean it up */
        loRaWANctx.FsmState = Fsm_MAC_Proc;
      }

    } else {
      /* USB: info */
      //usbLogLora("LoRaWAN: Received packet within RX2 window.\r\n\r\n");

      /* Process message */
      loRaWANctx.FsmState = Fsm_MAC_Decoder;
    }
  }
}

static void loRaWANLoRaWANTaskLoop__JoinRequest_NextTry(void)
{
  if ((JoinRequest  == g_framShadow1F.LoRaWAN_MHDR_MType) ||
      (JoinAccept   == g_framShadow1F.LoRaWAN_MHDR_MType)) {
    /* Try again with new JOINREQUEST - clears loRaWanTxMsg by itself */
    loRaWANctx.FsmState = Fsm_MAC_JoinRequest;

  } else {
    loRaWANctx.FsmState = Fsm_MAC_Proc;
  }

  /* Sequence has ended */
  LoRaWAN_calc_TxMsg_Reset(&loRaWANctx, &loRaWanTxMsg);

  /* Remove RX message */
  LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);
}


static void loRaWANLoRaWANTaskLoop__JoinRequestRX1(void)
{
  if (loRaWANctx.TsEndOfTx) {
    /* JOIN-ACCEPT response after JOIN_ACCEPT_DELAY1 at RX1 - switch on receiver */

    /* USB: info */
    //usbLogLora("\r\nLoRaWAN: JOIN-ACCEPT RX1 (5 s).\r\n");

    /* Adjust the context */
    loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
    loRaWANctx.Dir                  = Dn;
//  loRaWANctx.Ch_Selected          = ;                                                         // Keep same channel of TX before

    /* Receive on RX1 frequency */                                                              // Same frequency and SF as during transmission
    LoRaWAN_RX_msg(&loRaWANctx, &loRaWanRxMsg,
        LORAWAN_EU868_JOIN_ACCEPT_DELAY1_MS,
        LORAWAN_EU868_JOIN_ACCEPT_DELAY1_MS + LORAWAN_EU868_MAX_RX1_DURATION_MS);

    if (loRaWanRxMsg.msg_encoded_Len == 0) {
      /* Receive response at JOINREQUEST_RX2 */
      loRaWANctx.FsmState = Fsm_JoinRequestRX2;

    } else {
      /* USB: info */
      //usbLogLora("LoRaWAN: JOIN-RESPONSE received within window JR-Delay_RX1.\r\n");

      /* Process message */
      loRaWANctx.FsmState = Fsm_MAC_JoinAccept;
    }

  } else {  // if (tsEndOfTx)
    /* Reset FSM */
    loRaWANctx.FsmState = Fsm_NOP;
  }
}

static void loRaWANLoRaWANTaskLoop__JoinRequestRX2(void)
{
  if (loRaWANctx.TsEndOfTx) {
    /* JOIN-ACCEPT response after JOIN_ACCEPT_DELAY2 at RX2 */

    /* USB: info */
    //usbLogLora("\r\nLoRaWAN: JOIN-ACCEPT RX2 (6 s).\r\n");

    /* Adjust the context */
    loRaWANctx.Current_RXTX_Window  = CurWin_RXTX2;
    loRaWANctx.Dir                  = Dn;
    loRaWANctx.Ch_Selected          = 16;                                                       // Jump to RX2 channel

    /* Listen on RX2 frequency */
    LoRaWAN_RX_msg(&loRaWANctx, &loRaWanRxMsg,
        LORAWAN_EU868_JOIN_ACCEPT_DELAY2_MS,
        LORAWAN_EU868_JOIN_ACCEPT_DELAY2_MS + LORAWAN_EU868_MAX_TX_DURATION_MS);

    /* Reset SX1261 and keep in that state */
    sxReset(true);

    if (loRaWanRxMsg.msg_encoded_Len == 0) {
      //usbLogLora("LoRaWAN: Failed to RX.\r\n\r\n");

#if 0
      if (loRaWANctx.SpreadingFactor < SF12_DR0_VAL) {
        loRaWANctx.SpreadingFactor++;
      }
#endif

      /* Try again with new JOINREQUEST */
      loRaWANLoRaWANTaskLoop__JoinRequest_NextTry();

    } else {  // if (loRaWanRxMsg.msg_encoded_Len == 0) {} else
      /* USB: info */
      //usbLogLora("LoRaWAN: JOIN-RESPONSE received within window JR-Delay_RX2.\r\n");

      /* Continue with the JoinAccept handling */
      loRaWANctx.FsmState = Fsm_MAC_JoinAccept;
    }

  } else {  // if (tsEndOfTx)
    /* Sequence has ended */
    LoRaWAN_calc_TxMsg_Reset(&loRaWANctx, &loRaWanTxMsg);

    /* Remove RX message */
    LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);

    /* Reset FSM */
    LoRaWAN_MAC_Queue_Reset();
    loRaWANctx.FsmState = Fsm_NOP;
  }
}

/* Compile message and TX */
static void loRaWANLoRaWANTaskLoop__Fsm_TX(void)
{
  /* USB: info */
  //usbLogLora("LoRaWAN: TX packet.\r\n");

  /* Clear RX msg buffer */
  {
    LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);
    LoRaWAN_MAC_Queue_Reset();
  }

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window        = CurWin_RXTX1;
  loRaWANctx.Dir                        = Up;
  loRaWANctx.Ch_Selected                = LoRaWAN_calc_randomChannel(&loRaWANctx);              // Randomized TX1 frequency

  /* Get current state of the LoRa EG */
  g_framShadow1F.loraEventGroup = xEventGroupGetBits(loraEventGroupHandle);

  /* Decide if response is needed */
  if (!(Lora_EGW__LINK_ESTABLISHED & g_framShadow1F.loraEventGroup) ||
      (Lora_EGW__ADR_DOWNACTIVATE  & g_framShadow1F.loraEventGroup)) {
    loRaWANctx.ConfirmedPackets_enabled = 1U;                                                   // Do confirm packets when re-configuring
  } else {
    loRaWANctx.ConfirmedPackets_enabled = LORAWAN_CONFPACKS_DEFAULT;                            // ConfDataUp to be used
  }
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Compile packet if not done, already */
  if (!(loRaWanTxMsg.msg_encoded_EncDone)) {
    LoRaWAN_calc_TxMsg_Compiler_Standard(&loRaWANctx, &loRaWanTxMsg);
  }

  /* Prepare transmitter and go on-air */
  {
    /* Update to current settings */
    loRaWANctx.FrequencyMHz     = 0.f;
    loRaWANctx.SpreadingFactor  = 0;

    for (uint8_t tryCnt = 4U; tryCnt; --tryCnt) {
      LoRaWAN_TX_msg(&loRaWANctx, &loRaWanTxMsg);

      /* Check if SX1261 failed to transmit */
      if (!loRaWANctx.TsEndOfTx) {
        /* TX has failed */

        /* Reset SX1261 and keep in that state */
        sxReset(false);
      } else {
        break;
      }
    }

    /* Reset SX1261 and keep in that state */
    sxReset(true);
  }

  /* Decide if down link data is expected, or not */
  if (loRaWANctx.ConfirmedPackets_enabled) {
    /* Receive response at RX1 */
    loRaWANctx.FsmState = Fsm_RX1;

  } else {
    /* Increment uplink FCnt */
    g_framShadow1F.LoRaWAN_NFCNT_Up++;

    /* Write back to NVM */
    g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
    fram_updateCrc();

    loRaWANctx.FsmState = Fsm_MAC_Proc;

    if (!loRaWANctx.ConfirmedPackets_enabled) {
      /* Fire and forget */
      xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DATA_UP_DONE);
    }
  }
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_Decoder(void)
{
  /* Decode received message */

  /* Non-valid short message */
  if (loRaWanRxMsg.msg_encoded_Len < 5) {
    /* USB: info */
    //usbLogLora("LoRaWAN: Decoder informs: FAIL - message too short.\r\n");

    /* Sequence has ended */
    LoRaWAN_calc_TxMsg_Reset(&loRaWANctx, &loRaWanTxMsg);

    /* Remove RX message */
    LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);

    if (ConfDataUp == g_framShadow1F.LoRaWAN_MHDR_MType) {
      switch (loRaWANctx.TX_MAC_Buf[0]) {
      case LinkCheckReq_UP:
        {
          /* Try again */
          loRaWANctx.FsmState = Fsm_MAC_LinkCheckReq;
        }
        break;

      default:
        {
          /* Nothing to do with that */
          loRaWANctx.FsmState = Fsm_NOP;
        }
      }  // switch (loRaWanTxMsg.msg_prep_FOpts_Buf[0])
    }
    return;
  }

  /* Decode the RX msg */
  {
    LoRaWAN_calc_RxMsg_Decoder_Standard(&loRaWANctx, &loRaWanRxMsg);

    if (loRaWanRxMsg.msg_parted_MIC_Valid) {
#if 0
      /* USB: info */
      {
        char usbDbgBuf[128];
        int  len;
        (void) len;

        len = sprintf(usbDbgBuf,
            "LoRaWAN: Decoder informs: SUCCESS - more data pending?=%u, ACK=%u\r\n",
            loRaWANctx.FCtrl_FPending, loRaWANctx.FCtrl_ACK);
        //usbLogLenLora(usbDbgBuf, len);
      }
#endif

      /* Default setting */
      loRaWANctx.FsmState = Fsm_NOP;

      /* Up data successfully sent */
      xEventGroupSetBits(globalEventGroupHandle, Lora_EGW__DATA_UP_DONE);


      /* Single MAC command can be handled within the FSM directly */
      if ((loRaWanRxMsg.msg_parted_FCtrl_FOptsLen          > 0) &&                          // FOpts MAC
         ( loRaWanRxMsg.msg_parted_FPort_absent                 ||                          // no FPort0 MAC
          (loRaWanRxMsg.msg_parted_FPort                  != 0)))
      {
        /* Push all MAC data to the MAC queue from FOpts_Buf */
        LoRaWAN_MAC_Queue_Push(loRaWanRxMsg.msg_parted_FOpts_Buf, loRaWanRxMsg.msg_parted_FCtrl_FOptsLen);

        /* MAC processing */
        loRaWANctx.FsmState = Fsm_MAC_Proc;

      } else if ((loRaWanRxMsg.msg_parted_FCtrl_FOptsLen  == 0) &&                          // no FOpts MAC
                (!loRaWanRxMsg.msg_parted_FPort_absent)         &&                          // Payload MAC
                 (loRaWanRxMsg.msg_parted_FPort           == 0) &&
                 (loRaWanRxMsg.msg_parted_FRMPayload_Len   > 0))
      {
        /* Push all MAC data to the MAC queue from FRMPayload_Buf[] */
        LoRaWAN_MAC_Queue_Push(loRaWanRxMsg.msg_parted_FRMPayload_Buf, loRaWanRxMsg.msg_parted_FRMPayload_Len);

        /* MAC processing */
        loRaWANctx.FsmState = Fsm_MAC_Proc;
      }

      /* Downlink data processing */
      if ((loRaWanRxMsg.msg_parted_FPort > 0) && (loRaWanRxMsg.msg_parted_FRMPayload_Len > 0)) {
        /* Downlink IoT4BeesApp payload data */
        LoRaWAN_marshalling_PayloadExpand_IoT4BeesAppDown(&s_Iot4BeesApp_down, loRaWanRxMsg.msg_parted_FRMPayload_Buf, loRaWanRxMsg.msg_parted_FRMPayload_Len);
      }

    } else {  // if (loRaWanRxMsg.msg_parted_MIC_Valid)
      /* USB: info */
      //usbLogLora("LoRaWAN: Decoder informs: FAIL - bad format.\r\n");

      if ((JoinRequest  == (loRaWanRxMsg.msg_parted_MHDR >> LoRaWAN_MHDR_MType_SHIFT)) ||
          (JoinAccept   == (loRaWanRxMsg.msg_parted_MHDR >> LoRaWAN_MHDR_MType_SHIFT))) {
        loRaWANLoRaWANTaskLoop__JoinRequest_NextTry();

      } else {
        /* Sequence has ended */
        LoRaWAN_calc_TxMsg_Reset(&loRaWANctx, &loRaWanTxMsg);

        /* Remove RX message */
        LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);

        /* Nothing to do with that */
        loRaWANctx.FsmState = Fsm_NOP;
      }
    }
  }
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_Proc(void)
{
  /* MAC queue processing */

  if (!xQueueIsQueueEmptyFromISR(loraMacQueueHandle)) {
    uint8_t mac = 0;
    LoRaWAN_MAC_Queue_Pull(&mac, 1);

    switch (mac) {
    case LinkCheckAns_DN:
      loRaWANctx.FsmState = Fsm_MAC_LinkCheckAns;
      break;

    case LinkADRReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_LinkADRReq;
      break;

    case DutyCycleReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_DutyCycleReq;
      break;

    case RXParamSetupReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_RXParamSetupReq;
      break;

    case DevStatusReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_DevStatusReq;
      break;

    case NewChannelReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_NewChannelReq;
      break;

    case RXTimingSetupReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_RXTimingSetupReq;
      break;

    case TxParamSetupReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_TxParamSetupReq;
      break;

    case DlChannelReq_DN:
      loRaWANctx.FsmState = Fsm_MAC_DlChannelReq;
      break;

    default:
      LoRaWAN_MAC_Queue_Reset();
      loRaWANctx.FsmState = Fsm_NOP;
    }  // switch (mac)
    return;
  }  // if (LoRaWAN_MAC_Queue_isAvail())

  /* Fall back to NOP */
  loRaWANctx.FsmState = Fsm_NOP;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_JoinRequest(void)
{
  /* MAC CID 0x01 - up:JoinRequest --> dn:JoinAccept */

  /* JoinRequest does reset frame counters */
  g_framShadow1F.LoRaWAN_NFCNT_Up = 0UL;
  g_framShadow1F.LoRaWAN_NFCNT_Dn = 0UL;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: JOIN-REQUEST going to be sent TX.\r\n");

  /* Compile packet */
  LoRaWAN_calc_TxMsg_Compiler_MHDR_JOINREQUEST(&loRaWANctx, &loRaWanTxMsg);

  /* Prepare transmitter and go on-air */
  {
    /* Update to current settings */
    loRaWANctx.FrequencyMHz     = 0.f;
    loRaWANctx.SpreadingFactor  = 0;

    LoRaWAN_TX_msg(&loRaWANctx, &loRaWanTxMsg);
  }

  /* Expect response at JOINREQUEST_RX1 first */
  loRaWANctx.FsmState = Fsm_JoinRequestRX1;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_JoinAccept(void)
{
  /* JOIN-ACCEPT process the message */
  if (HAL_OK == LoRaWAN_calc_RxMsg_Decoder_MHDR_JOINACCEPT(&loRaWANctx, &loRaWanRxMsg)) {
    /* Sequence has ended */
    LoRaWAN_calc_TxMsg_Reset(&loRaWANctx, &loRaWanTxMsg);

    /* Remove RX message */
    LoRaWAN_calc_RxMsg_Reset(&loRaWanRxMsg);

    loRaWANctx.FsmState = Fsm_MAC_Proc;

#if 0
    /* USB: info */
    {
      char usbDbgBuf[512];
      int  len;
      (void) len;

      len = sprintf(usbDbgBuf,
          "LoRaWAN: JOIN-ACCEPT message successfully decoded: NetID=%02X.%02X.%02X, Dev-Addr=%02X.%02X.%02X.%02X, DR-RXTX2=DR%u, DRoffs-RX1=+%u DRs, RXDelay=%u sec\r\n",
          loRaWANctx.NetID_LE[2], loRaWANctx.NetID_LE[1], loRaWANctx.NetID_LE[0],
          loRaWANctx.DevAddr_LE[3], loRaWANctx.DevAddr_LE[2], loRaWANctx.DevAddr_LE[1], loRaWANctx.DevAddr_LE[0],
          loRaWANctx.LinkADR_DataRate_RXTX2,
          loRaWANctx.LinkADR_DataRate_RX1_DRofs,
          loRaWANctx.RXDelay);
      //usbLogLenLora(usbDbgBuf, len);
    }
#endif

    /* Store new connection states */
    {
      /* Network Counter Up */
      g_framShadow1F.LoRaWAN_NFCNT_Up = 0UL;

      /* Network Counter Down */
      g_framShadow1F.LoRaWAN_NFCNT_Dn = 0UL;

      /* Application Counter Up */
      g_framShadow1F.LoRaWAN_AFCNT_Up = 0UL;

      /* Application Counter Down */
      g_framShadow1F.LoRaWAN_AFCNT_Dn = 0UL;


      /* Inform about JOIN_ACCEPT */
      xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__JOINACCEPT_OK);

      /* Write back the complete file to NVM */
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
      fram_updateCrc();

      if (!(Lora_EGW__LINKCHECK_OK & g_framShadow1F.loraEventGroup)) {
        /* Request LoRaWAN link check and network time */
        xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_LINKCHECKREQ);

        /* Write back to NVM */
        g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
        fram_updateCrc();
      }
    }

  } else {
    /* USB: info */
    //usbLogLora("LoRaWAN: JOIN-RESPONSE message failed to decode.\r\n\r\n");

    /* Try again with new JOINREQUEST */
    loRaWANLoRaWANTaskLoop__JoinRequest_NextTry();
  }
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkCheckReq(void)
{
  /* MAC CID 0x02 - up:LinkCheckReq --> dn:LinkCheckAns */

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = LinkCheckReq_UP;
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX LinkCheckReq.\r\n");

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkCheckAns(void)
{
  uint8_t macBuf[2] = { 0 };

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  loRaWANctx.LinkCheck_Ppm_SNR  = macBuf[0];
  loRaWANctx.LinkCheck_GW_cnt   = macBuf[1];

  loRaWANctx.FsmState           = Fsm_MAC_Proc;

#if 0
  /* USB: info */
  {
    char usbDbgBuf[128];
    int  len;
    (void) len;

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX LinkCheckAns: Signal +%02u dB above decoding level, %u gateways heard us\r\n",
        loRaWANctx.LinkCheck_Ppm_SNR, loRaWANctx.LinkCheck_GW_cnt);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  xEventGroupClearBits(loraEventGroupHandle, Lora_EGW__DO_LINKCHECKREQ);
  xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__LINKCHECK_OK | Lora_EGW__ADR_DOWNACTIVATE | Lora_EGW__LINK_ESTABLISHED | Lora_EGW__TRANSPORT_READY);

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  #if 0
  /* Check connection state */
  Lora_EGW_BM_t lora_EG = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  if (!(Lora_EGW__DEVICETIME_OK & lora_EG)) {
    /* Request LoRaWAN link check and network time */
    g_framShadow1F.loraEventGroup = xEventGroupSetBits(loraEventGroupHandle, Lora_EGW__DO_DEVICETIMEREQ);

    /* Write back to NVM */
    fram_writeBuf(&(g_framShadow1F.loraEventGroup), FRAM_MEM_LORAWAN_BITFIELD, sizeof(g_framShadow1F.loraEventGroup));
    fram_updateCrc();

    loRaWANctx.FsmState = Fsm_MAC_DeviceTimeReq;
  }
  #endif
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkADRReq(void)
{
  /* MAC CID 0x03 - dn:LinkADRReq --> up:LinkADRAns */

  uint8_t macBuf[4] = { 0 };
  uint8_t nextMac;

  /* Iterate over all LinkADRReq MACs */
  do {
    LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
    g_framShadow1F.LinkADR_PwrRed =                (macBuf[0] & 0x0f) << 1;
    g_framShadow1F.LinkADR_DR_TX1 = (DataRates_t) ((macBuf[0] & 0xf0) >> 4);          // Do not honor DR as long as ADR is sent only once ?

    /* Write back to NVM */
    g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
    fram_updateCrc();

    /* For all RX1 channels, do */
    for (uint8_t idx = 0U; idx < 15U; idx++) {
      g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx] = g_framShadow1F.LinkADR_DR_TX1;
    }

    /* Prove each channel mask bit before acceptance */
    uint16_t newChMask      =  macBuf[1] | ((uint16_t)macBuf[2] << 8);
    uint8_t  newChMaskValid = (newChMask != 0) ?  1 : 0;
    for (uint8_t chIdx = 0; chIdx < 16; chIdx++) {
      if ((1UL << chIdx) & newChMask) {
        if (!loRaWANctx.Ch_FrequenciesUplink_MHz[chIdx]) {
          newChMaskValid = 0;
          break;
        }
      }
    }
    if (newChMaskValid) {
      g_framShadow1F.LinkADR_ChanMsk        = newChMask;
      loRaWANctx.LinkADR_ChannelMask_OK     = 1;
    } else {
      loRaWANctx.LinkADR_ChannelMask_OK     = 0;
    }

    g_framShadow1F.LinkADR_NbTrans          = macBuf[3] & 0x0f;
    g_framShadow1F.LinkADR_ChanMsk_Cntl     = (ChMaskCntl_t) ((macBuf[3] & 0x70) >> 4);

    /* Write back to NVM */
    g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
    fram_updateCrc();

    /* Acknowledge this packet */
    loRaWANctx.FCtrl_ACK = 1;

#if 0
    /* USB: info */
    {
      char usbDbgBuf[256];
      int  len;
      (void) len;

      len = sprintf(usbDbgBuf,
          "LoRaWAN: Got RX LinkADRReq: Power reduction by=%02udB, DataRate TX1=DR%u (do not honor and keep DR%u), new channel mask=0x%04X, NbTrans=%u\r\n",
          loRaWANctx.LinkADR_TxPowerReduction_dB,
          ((macBuf[0] & 0xf0) >> 4),
          loRaWANctx.LinkADR_DataRate_TX1,
          loRaWANctx.LinkADR_ChannelMask,
          loRaWANctx.LinkADR_NbTrans);
      //usbLogLenLora(usbDbgBuf, len);
    }
#endif

    if (xQueueIsQueueEmptyFromISR(loraMacQueueHandle)) {
      break;
    }

    LoRaWAN_MAC_Queue_Pull(&nextMac, sizeof(nextMac));
    if (LinkADRReq_DN != nextMac) {
      break;
    }
  } while (1);

  loRaWANctx.FsmState = Fsm_MAC_LinkADRAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkADRAns(void)
{
  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency
  loRaWANctx.Dir                  = Up;

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = LinkADRAns_UP;
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = 0b110 | (0x1 & loRaWANctx.LinkADR_ChannelMask_OK);  // Power-change OK, DataRate-change OK, ChannelMask-change is context
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX LinkADRAns.\r\n");

  /* ADR message valid - no more RX windows needed */
  xEventGroupClearBitsFromISR(loraEventGroupHandle, Lora_EGW__ADR_DOWNACTIVATE);

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBits(loraEventGroupHandle);

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DutyCycleReq(void)
{
  /* MAC CID 0x04 - dn:DutyCycleReq --> up:DutyCycleAns */

  uint8_t macBuf[1] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  loRaWANctx.DutyCycle_MaxDutyCycle = macBuf[0] & 0x0f;

#if 0
  /* USB: info */
  {
    char usbDbgBuf[64];
    int  len;
    (void) len;

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX DutyCycleReq: Max duty cycle=%02u\r\n",
        loRaWANctx.DutyCycle_MaxDutyCycle);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_DutyCycleAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DutyCycleAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX DutyCycleAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency
  // loRaWANctx.LinkADR_DataRate_TX1);


  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = DutyCycleAns_UP;
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RXParamSetupReq(void)
{
  /* MAC CID 0x05 - dn:RXParamSetupReq --> up:RXParamSetupAns */

  uint8_t macBuf[4] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  uint8_t dlSettings                        = macBuf[0];
  g_framShadow1F.LinkADR_DR_RXTX2           = (DataRates_t) (dlSettings & 0x0f);
  g_framShadow1F.LinkADR_DR_RX1_DRofs       = (dlSettings & 0x70) >> 4;
  loRaWANctx.Ch_FrequenciesDownlink_MHz[15] = loRaWANctx.Ch_FrequenciesUplink_MHz[15] = LoRaWAN_calc_CFListEntry_2_FrqMHz(macBuf + 1);    // New TXRX2 frequency

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

#if 0
  /* USB: info */
  {
    char      usbDbgBuf[128];
    int       len;
    uint32_t  fmt_mhz;
    uint16_t  fmt_mhz_f1;
    (void) len;

    mainCalc_Float2Int(loRaWANctx.Ch_FrequenciesUplink_MHz[15], &fmt_mhz, &fmt_mhz_f1);

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX RXParamSetupReq: DR-RXTX2=DR%u, DRoffs-RX1=+%u DRs, RX/TX2 (Up/Down) f = %03lu.%03u MHz\r\n",
        g_framShadow1F.LinkADR_DR_RXTX2,
        g_framShadow1F.LinkADR_DR_RX1_DRofs,
        fmt_mhz, fmt_mhz_f1);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_RXParamSetupAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RXParamSetupAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX RXParamSetupAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = RXParamSetupAns_UP;
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = 0b111;                                       // RX1DRoffset ACK, RX2 Data rate ACK, Channel ACK
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DevStatusReq(void)
{
  /* MAC CID 0x06 - dn:DevStatusReq --> up:DevStatusAns */

  //uint8_t macBuf[0] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  //LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));

#if 0
  /* USB: info */
  {
    char usbDbgBuf[64];
    int  len;
    (void) len;

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX DevStatusReq: (no data)\r\n");
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_DevStatusAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DevStatusAns(void)
{
  int margin = loRaWANctx.LastPacketSnrDb;
  if (margin > 31) {
    margin = 31;
  } else if (margin < -31) {
    margin = -31;
  }

#if 0
  /* USB: info */
  {
    char usbDbgBuf[128];
    int  len;
    (void) len;

    len = sprintf(usbDbgBuf,
        "\r\nLoRaWAN: Going to TX DevStatusAns: Battery=Ext.Power(0), Margin=%3ddB\r\n",
        margin);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = DevStatusAns_UP;
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = margin & 0x3f;
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_NewChannelReq(void)
{
  /* MAC CID 0x07 - dn:NewChannelReq --> up:NewChannelAns */

  uint8_t   macBuf[5] = { 0 };
  uint8_t   l_chIdx;
  float     l_f;
  uint8_t   l_DRmin, l_DRmax;

  loRaWANctx.Channel_FrequencyValid = 0;
  loRaWANctx.Channel_DataRateValid  = 0;

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  l_chIdx = macBuf[0];
  l_f     = LoRaWAN_calc_CFListEntry_2_FrqMHz(macBuf + 1);
  l_DRmin =  macBuf[4] & 0x0f;
  l_DRmax = (macBuf[4] & 0xf0) >> 4;

  if ((l_DRmin <= l_DRmax) &&
      (DR0_SF12_125kHz_LoRa <= l_DRmin && l_DRmin <= DR5_SF7_125kHz_LoRa) &&
      (DR0_SF12_125kHz_LoRa <= l_DRmax && l_DRmax <= DR5_SF7_125kHz_LoRa)) {
    loRaWANctx.Channel_DataRateValid = 1;
  }

  if ((860e6 <= l_f) && (l_f <= 920e6)) {
    loRaWANctx.Channel_FrequencyValid   = 1;

    if ((1 <= l_chIdx) && (l_chIdx <= 16)) {
      const uint8_t idx = l_chIdx - 1;
      if (loRaWANctx.Channel_DataRateValid) {
        loRaWANctx.Ch_FrequenciesDownlink_MHz[idx]          = loRaWANctx.Ch_FrequenciesUplink_MHz[idx]  = l_f;
        g_framShadow1F.LoRaWAN_Ch_DR_TX_min[idx]            = (DataRates_t) l_DRmin;
        g_framShadow1F.LoRaWAN_Ch_DR_TX_max[idx]            = (DataRates_t) l_DRmax;

        if (g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx]        < g_framShadow1F.LoRaWAN_Ch_DR_TX_min[idx]) {
          g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx]          = g_framShadow1F.LoRaWAN_Ch_DR_TX_min[idx];

        } else if (g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx] > g_framShadow1F.LoRaWAN_Ch_DR_TX_max[idx]) {
          g_framShadow1F.LoRaWAN_Ch_DR_TX_sel[idx]          = g_framShadow1F.LoRaWAN_Ch_DR_TX_max[idx];
        }
      }
    }
  }

#if 0
  /* USB: info */
  {
    char      usbDbgBuf[256];
    int       len;
    uint32_t  fmt_mhz;
    uint16_t  fmt_mhz_f1;
    (void) len;

    mainCalc_Float2Int(l_f, &fmt_mhz, &fmt_mhz_f1);

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX NewChannelReq: ChannelIdx=%u, new = %03lu.%03u MHz, min DR=%u, max DR=%u\r\n",
        l_chIdx,
        fmt_mhz, fmt_mhz_f1,
        l_DRmin, l_DRmax);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_NewChannelAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_NewChannelAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX NewChannelAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = NewChannelAns_UP;
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] =  (loRaWANctx.Channel_DataRateValid  ?  0x02 : 0x00) |
                                                    (loRaWANctx.Channel_FrequencyValid ?  0x01 : 0x00);
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DlChannelReq(void)
{
  /* MAC CID 0x0A - dn:DlChannelReq --> up:DlChannelAns */

  uint8_t   l_chIdx;
  float     l_f;

  loRaWANctx.Channel_FrequencyValid   = 0;
  loRaWANctx.Channel_FrequencyExists  = 0;

  uint8_t macBuf[4] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  l_chIdx = macBuf[0];
  l_f     = LoRaWAN_calc_CFListEntry_2_FrqMHz(macBuf + 1);

  if ((867.09e6 <= l_f) && (l_f <= 868.91e6)) {
    loRaWANctx.Channel_FrequencyValid = 1;

    if ((1 <= l_chIdx) && (l_chIdx <= 16)) {
      const uint8_t idx = l_chIdx - 1;

      if (loRaWANctx.Ch_FrequenciesUplink_MHz[idx]) {
        loRaWANctx.Channel_FrequencyExists          = 1;
        loRaWANctx.Ch_FrequenciesDownlink_MHz[idx]  = l_f;
      }
    }
  }

#if 0
  /* USB: info */
  {
    char      usbDbgBuf[256];
    int       len;
    uint32_t  fmt_mhz;
    uint16_t  fmt_mhz_f1;
    (void) len;

    mainCalc_Float2Int(l_f, &fmt_mhz, &fmt_mhz_f1);

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX DlChannelReq: ChannelIdx=%u, new Downlink f = %03lu.%03u MHz\r\n",
        l_chIdx,
        fmt_mhz, fmt_mhz_f1);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_DlChannelAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DlChannelAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX DlChannelAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = DlChannelAns_UP;
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] =  (loRaWANctx.Channel_FrequencyExists ?  0x02 : 0x00) |
                                                    (loRaWANctx.Channel_FrequencyValid  ?  0x01 : 0x00);
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RXTimingSetupReq(void)
{
  /* MAC CID 0x08 - dn:RXTimingSetupReq --> up:RXTimingSetupAns */

  uint8_t macBuf[1] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  uint8_t l_rxDelay = macBuf[0] & 0x0f;

  /* RXDelay mapping 0 --> 1 */
  if (!l_rxDelay) {
    l_rxDelay++;
  }
  g_framShadow1F.LoRaWAN_rxDelay = l_rxDelay;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

#if 0
  /* USB: info */
  {
    char      usbDbgBuf[64];
    int       len;
    (void) len;

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX RXTimingSetupReq: RX1 delay=%u s after TX.\r\n",
        loRaWANctx.RXDelay);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_RXTimingSetupAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RXTimingSetupAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX RXTimingSetupAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = RXTimingSetupAns_UP;
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}

#ifdef AS923
const uint8_t c_MaxEIRP_dBm[16] = {  8, 10, 12, 13, 14, 16, 18, 20, 21, 24, 26, 27, 29, 30, 33, 36 };
static void loRaWANLoRaWANTaskLoop__Fsm_MAC_TxParamSetupReq(void)
{
  /* MAC CID 0x09 - dn:TxParamSetupReq --> up:TxParamSetupAns */

  uint8_t macBuf[1] = { 0 };

  /* Pull data from the MAC_Queue and store in context */
  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  loRaWANctx.TxParamSetup_DownlinkDwellTime_ms  = ((macBuf[0] & 0x20) > 0) ?  400U : 0U;
  loRaWANctx.TxParamSetup_UplinkDwellTime_ms    = ((macBuf[0] & 0x10) > 0) ?  400U : 0U;
  loRaWANctx.TxParamSetup_MaxEIRP_dBm           = c_MaxEIRP_dBm[macBuf[0] & 0x0f];

#if 0
  /* USB: info */
  {
    char      usbDbgBuf[128];
    int       len;

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX TxParamSetupReq: Max EIRP=%u dBm, Uplink-Dwell-Time = %u ms, Downlink-Dwell-Time = % ms.\r\n",
        loRaWANctx.TxParamSetup_MaxEIRP_dBm, loRaWANctx.TxParamSetup_UplinkDwellTime_ms, loRaWANctx.TxParamSetup_DownlinkDwellTime_ms);
    usbLogLenLora(usbDbgBuf, len);
  }
#endif

  loRaWANctx.FsmState = Fsm_MAC_TxParamSetupAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_TxParamSetupAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX TxParamSetupAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Dir                  = Up;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = TxParamSetupAns_UP;
  loRaWanTxMsg.msg_encoded_EncDone = 0;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;
  fram_updateCrc();

  /* Prepare for transmission */
  loRaWANctx.FsmState = Fsm_TX;
}
#endif

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeReq(void)
{
  /* MAC CID 0x0D - up:DeviceTimeReq --> dn:DeviceTimeAns */

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.Ch_Selected          = LoRaWAN_calc_randomChannel(&loRaWANctx);                    // Randomized RX1 frequency

  /* MAC to be added */
  loRaWANctx.TX_MAC_Buf[loRaWANctx.TX_MAC_Len++] = DeviceTimeReq_UP;
  loRaWanTxMsg.msg_encoded_EncDone = 0U;

  /* Requesting for confirmed data up-transport */
  g_framShadow1F.LoRaWAN_MHDR_MType = loRaWANctx.ConfirmedPackets_enabled ?  ConfDataUp : UnconfDataUp;
  g_framShadow1F.LoRaWAN_MHDR_Major = LoRaWAN_R1;

  /* Write back to NVM */
  g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
  fram_updateCrc();

  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX DeviceTimeReq.\r\n");

  /* Prepare for transmission */
 loRaWANctx.FsmState = Fsm_TX;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeAns(void)
{
  uint8_t         macBuf[5]               = { 0 };
  uint32_t        timeDiffNowTx_ms        = xTaskGetTickCount();
  const uint32_t  SystemTimeSinceBoot_ms  = timeDiffNowTx_ms;
  double          timeDeviceTimeReqNow;

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
  uint32_t l_gps_epoc_s         = ((uint32_t)macBuf[3] << 24) | ((uint32_t)macBuf[2] << 24) | ((uint32_t)macBuf[1] << 24) | ((uint32_t)macBuf[0]);
  uint32_t l_gps_epoc_frac      = macBuf[4];

  /* Point of time of TX done of DeviceTimeReq */
  l_gps_epoc_s                 -= GPS_UTC_LEAP_SECS;
  timeDeviceTimeReqNow          = l_gps_epoc_s + (l_gps_epoc_frac / 256.);

  /* Time difference between now and TX done of DeviceTimeReq */
  timeDiffNowTx_ms             -= loRaWANctx.TsEndOfTx;

  /* Transfer GW information to now time */
  timeDeviceTimeReqNow         += timeDiffNowTx_ms / 1000.;

  /* System start up time */
  loRaWANctx.BootTime_UTC_ms    = ((uint64_t) (timeDeviceTimeReqNow * 1000.)) - ((uint64_t) SystemTimeSinceBoot_ms);

  loRaWANctx.FsmState           = Fsm_MAC_Proc;

#if 0
  /* USB: info */
  {
    char        usbDbgBuf[128];
    char        timeStrBuf[64];
    int         len;
    (void) len;

    /* Get UTC time string */
    {
#if 0
      struct timeval tv;
      tv.tv_sec  = (time_t)        timeDeviceTimeReqNow;
      tv.tv_usec = ((suseconds_t)  (((uint64_t) (timeDeviceTimeReqNow * 1000.)) % 1000));
#else
      time_t l_t = (time_t) timeDeviceTimeReqNow;
#endif

      struct tm* lp_tm = gmtime(&l_t);
      strftime(timeStrBuf, sizeof(timeStrBuf), "%F  %T UTC", lp_tm);
    }

    len = sprintf(usbDbgBuf,
        "LoRaWAN: Got RX DeviceTimeAns: GPS epoch = %lu.%u s: %s\r\n",
        ((uint32_t) timeDeviceTimeReqNow), ((uint16_t) (((uint64_t) (timeDeviceTimeReqNow * 1000.)) % 1000)),
        timeStrBuf);
    //usbLogLenLora(usbDbgBuf, len);
  }
#endif
}


#ifdef LORANET_1V1
static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RekeyConf(void)
{
  /* MAC CID 0x0? - dn:RekeyConf --> up:? */

  uint8_t macBuf[1] = { 0 };

  /* USB: info */
  //usbLogLora("LoRaWAN: Got RX RekeyConf.\r\n");

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
//      loRaWANctx.xxx = macBuf[0];
  loRaWANctx.FsmState = Fsm_MAC_RekeyConf2;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RekeyConf2(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX RekeyConf2.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.FrequencyMHz         = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, LoRaWAN_calc_randomChannel(&loRaWANctx), 0);                                                                                   // Randomized RX1 frequency
  loRaWANctx.SpreadingFactor      = lorawanDR_to_SF(loRaWANctx.LinkADR_DataRate_TX1 + loRaWANctx.LinkADR_DataRate_RX1_DRofs);

  /* Prepare for transmission */
  loRaWANctx.FsmState             = Fsm_NOP;    // TODO
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_ADRParamSetupReq(void)
{
  /* MAC CID 0x0? - dn:ADRParamSetupReq --> up:ADRParamSetupAns */

  uint8_t macBuf[1] = { 0 };

  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Got RX ADRParamSetupReq.\r\n");

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
//      loRaWANctx.xxx = macBuf[0];
  loRaWANctx.FsmState = Fsm_MAC_ADRParamSetupAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_ADRParamSetupAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX ADRParamSetupAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.FrequencyMHz         = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, LoRaWAN_calc_randomChannel(&loRaWANctx), Up, 0);                                                                                   // Randomized RX1 frequency
  loRaWANctx.SpreadingFactor      = lorawanDR_to_SF(loRaWANctx.LinkADR_DataRate_TX1 + loRaWANctx.LinkADR_DataRate_RX1_DRofs);

  /* Prepare for transmission */
  loRaWANctx.FsmState             = Fsm_NOP;    // TODO
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeReq(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX DeviceTimeReq.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.FrequencyMHz         = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, LoRaWAN_calc_randomChannel(&loRaWANctx), Up, 0);                                                                                   // Randomized RX1 frequency
  loRaWANctx.SpreadingFactor      = lorawanDR_to_SF(loRaWANctx.LinkADR_DataRate_TX1 + loRaWANctx.LinkADR_DataRate_RX1_DRofs);

  /* Prepare for transmission */
  loRaWANctx.FsmState             = Fsm_NOP;    // TODO
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeAns(void)
{
  uint8_t macBuf[1] = { 0 };

  /* USB: info */
  //usbLogLora("LoRaWAN: Got RX DeviceTimeAns.\r\n");

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
//      loRaWANctx.xxx = macBuf[0];
  loRaWANctx.FsmState = Fsm_MAC_Proc;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_ForceRejoinReq(void)
{
  /* MAC CID 0x0? - dn:ForceRejoinReq --> up:ForceRejoinAns */

  uint8_t macBuf[1] = { 0 };

  /* USB: info */
  //usbLogLora("LoRaWAN: Got RX ForceRejoinReq.\r\n");

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
//      loRaWANctx.xxx = macBuf[0];
  loRaWANctx.FsmState = Fsm_MAC_ForceRejoinAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_ForceRejoinAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX ForceRejoinAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.FrequencyMHz         = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, LoRaWAN_calc_randomChannel(&loRaWANctx), Up, 0);                                                                                   // Randomized RX1 frequency
  loRaWANctx.SpreadingFactor      = lorawanDR_to_SF(loRaWANctx.LinkADR_DataRate_TX1 + loRaWANctx.LinkADR_DataRate_RX1_DRofs);

  /* Prepare for transmission */
  loRaWANctx.FsmState             = Fsm_NOP;    // TODO
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RejoinParamSetupReq(void)
{
  /* MAC CID 0x0? - dn:RejoinParamSetupReq --> up:RejoinParamSetupAns */

  uint8_t macBuf[1] = { 0 };

  /* USB: info */
  //usbLogLora("LoRaWAN: Got RX RejoinParamSetupReq.\r\n");

  LoRaWAN_MAC_Queue_Pull(macBuf, sizeof(macBuf));
//    loRaWANctx.xxx = macBuf[0];
  loRaWANctx.FsmState = Fsm_MAC_RejoinParamSetupAns;
}

static void loRaWANLoRaWANTaskLoop__Fsm_MAC_RejoinParamSetupAns(void)
{
  /* USB: info */
  //usbLogLora("\r\nLoRaWAN: Going to TX RejoinParamSetupAns.\r\n");

  /* Adjust the context */
  loRaWANctx.Current_RXTX_Window  = CurWin_RXTX1;
  loRaWANctx.FrequencyMHz         = LoRaWAN_calc_Channel_to_MHz(&loRaWANctx, LoRaWAN_calc_randomChannel(&loRaWANctx), Up, 0);                                                                                   // Randomized RX1 frequency
  loRaWANctx.SpreadingFactor      = lorawanDR_to_SF(loRaWANctx.LinkADR_DataRate_TX1 + loRaWANctx.LinkADR_DataRate_RX1_DRofs);

  /* Prepare for transmission */
  loRaWANctx.FsmState             = Fsm_NOP;    // TODO
}
#endif


void loRaWANLoraTaskLoop(void)
{
  EventBits_t eb;

  switch (loRaWANctx.FsmState) {
  case Fsm_RX1:
    loRaWANLoRaWANTaskLoop__Fsm_RX1();
    break;

  case Fsm_RX2:
    loRaWANLoRaWANTaskLoop__Fsm_RX2();
    break;

  case Fsm_JoinRequestRX1:
    loRaWANLoRaWANTaskLoop__JoinRequestRX1();
    break;

  case Fsm_JoinRequestRX2:
    loRaWANLoRaWANTaskLoop__JoinRequestRX2();
    break;


  /* Compile message and TX */
  case Fsm_TX:
    loRaWANLoRaWANTaskLoop__Fsm_TX();
    break;


  /* Decode received message */
  case Fsm_MAC_Decoder:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_Decoder();
    break;


  /* MAC queue processing */
  case Fsm_MAC_Proc:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_Proc();
    break;


  /* MAC CID 0x01 - up:JoinRequest --> dn:JoinAccept */
  case Fsm_MAC_JoinRequest:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_JoinRequest();
    break;

  case Fsm_MAC_JoinAccept:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_JoinAccept();
    break;


  /* MAC CID 0x02 - up:LinkCheckReq --> dn:LinkCheckAns */
  case Fsm_MAC_LinkCheckReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkCheckReq();
    break;

  case Fsm_MAC_LinkCheckAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkCheckAns();
    break;


  /* MAC CID 0x03 - dn:LinkADRReq --> up:LinkADRAns */
  case Fsm_MAC_LinkADRReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkADRReq();
    break;

  case Fsm_MAC_LinkADRAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_LinkADRAns();
    break;


  /* MAC CID 0x04 - dn:DutyCycleReq --> up:DutyCycleAns */
  case Fsm_MAC_DutyCycleReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DutyCycleReq();
    break;

  case Fsm_MAC_DutyCycleAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DutyCycleAns();
    break;


  /* MAC CID 0x05 - dn:RXParamSetupReq --> up:RXParamSetupAns */
  case Fsm_MAC_RXParamSetupReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RXParamSetupReq();
    break;

  case Fsm_MAC_RXParamSetupAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RXParamSetupAns();
    break;


  /* MAC CID 0x06 - dn:DevStatusReq --> up:DevStatusAns */
  case Fsm_MAC_DevStatusReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DevStatusReq();
    break;

  case Fsm_MAC_DevStatusAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DevStatusAns();
    break;


  /* MAC CID 0x07 - dn:NewChannelReq --> up:NewChannelAns */
  case Fsm_MAC_NewChannelReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_NewChannelReq();
    break;

  case Fsm_MAC_NewChannelAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_NewChannelAns();
    break;


  /* MAC CID 0x08 - dn:RXTimingSetupReq --> up:RXTimingSetupAns */
  case Fsm_MAC_RXTimingSetupReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RXTimingSetupReq();
    break;

  case Fsm_MAC_RXTimingSetupAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RXTimingSetupAns();
    break;


#ifdef AS923
  /* MAC CID 0x09 - dn:TxParamSetupReq --> up:TxParamSetupAns */
  case Fsm_MAC_TxParamSetupReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_TxParamSetupReq();
    break;

  case Fsm_MAC_TxParamSetupAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_TxParamSetupAns();
    break;
#endif


  /* MAC CID 0x0A - dn:DlChannelReq --> up:DlChannelAns */
  case Fsm_MAC_DlChannelReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DlChannelReq();
    break;

  case Fsm_MAC_DlChannelAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DlChannelAns();
    break;

  /* MAC CID 0x0D - up:DeviceTimeReq --> dn:DeviceTimeAns */
  case Fsm_MAC_DeviceTimeReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeReq();
    break;

  case Fsm_MAC_DeviceTimeAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_DeviceTimeAns();
    break;


#ifdef LORANET_1V1
  /* MAC CID 0x0? - dn:RekeyConf --> up:? */
  case Fsm_MAC_RekeyConf:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RekeyConf();
    break;

  case Fsm_MAC_RekeyConf2:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RekeyConf2();
    break;


  /* MAC CID 0x0? - dn:ADRParamSetupReq --> up:ADRParamSetupAns */
  case Fsm_MAC_ADRParamSetupReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_ADRParamSetupReq();
    break;

  case Fsm_MAC_ADRParamSetupAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_ADRParamSetupAns();
    break;


  /* MAC CID 0x0? - dn:ForceRejoinReq --> up:ForceRejoinAns */
  case Fsm_MAC_ForceRejoinReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_ForceRejoinReq();
    break;

  case Fsm_MAC_ForceRejoinAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_ForceRejoinAns();
    break;


  /* MAC CID 0x0? - dn:RejoinParamSetupReq --> up:RejoinParamSetupAns */
  case Fsm_MAC_RejoinParamSetupReq:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RejoinParamSetupReq();
    break;

  case Fsm_MAC_RejoinParamSetupAns:
    loRaWANLoRaWANTaskLoop__Fsm_MAC_RejoinParamSetupAns();
    break;
#endif

  default:
    loRaWANctx.FsmState = Fsm_NOP;
    // Fall-through.
  case Fsm_NOP:
    eb = xEventGroupWaitBits(loraEventGroupHandle,
        Lora_EGW__QUEUE_IN | Lora_EGW__DATA_UP_REQ | Lora_EGW__DO_LINKCHECKREQ /* | Lora_EGW__DO_DEVICETIMEREQ */,
        0,
        0, loRaWANWait_EGW_MaxWaitTicks);

    if (eb & Lora_EGW__QUEUE_IN) {
      /* Clear event group bit */
      xEventGroupClearBits(loraEventGroupHandle, Lora_EGW__QUEUE_IN);

      /* Read in queue and work on that commands */
      LoRaWAN_QueueIn_Process();

    } else if ((eb & Lora_EGW__DATA_UP_REQ) && (eb & Lora_EGW__LINK_ESTABLISHED)) {
      /* Clear event group bit */
      xEventGroupClearBits(loraEventGroupHandle, Lora_EGW__DATA_UP_REQ);

      /* Write back to NVM */
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
      fram_updateCrc();

      /* Read in data */
      LoRaWAN_BeamUpProcess();

    } else if (eb & Lora_EGW__DO_LINKCHECKREQ) {
      /* Clear event group bit */
      xEventGroupClearBits(loraEventGroupHandle, Lora_EGW__DO_LINKCHECKREQ);

      /* Write back to NVM */
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
      fram_updateCrc();

      /* LinkCheckReq is next on the priority table */
      loRaWANctx.FsmState = Fsm_MAC_LinkCheckReq;

#if 0
    } else if (eb & Lora_EGW__DO_DEVICETIMEREQ) {
      /* Clear event group bit */
      xEventGroupClearBits(loraEventGroupHandle, Lora_EGW__DO_DEVICETIMEREQ);
      g_framShadow1F.loraEventGroup = xEventGroupGetBitsFromISR(loraEventGroupHandle);
      fram_updateCrc();

      /* LinkCheckReq is last on the priority table */
      loRaWANctx.FsmState = Fsm_MAC_DeviceTimeReq;
#endif

    } else {
      /* No EventGroup bit but time out has triggered */

      /* In case not already joined */
      if (!(eb & Lora_EGW__JOINACCEPT_OK) &&
          loRaWANctx.TsEndOfTx &&
          ((loRaWANctx.TsEndOfTx + (30000UL / portTICK_PERIOD_MS)) <= xTaskGetTickCount())) {
        /* Retry JOIN_REQUEST */
        loRaWANctx.FsmState = Fsm_MAC_JoinRequest;

      } else if (!(eb & Lora_EGW__LINKCHECK_OK) &&
          loRaWANctx.TsEndOfTx &&
          ((loRaWANctx.TsEndOfTx + (5000UL / portTICK_PERIOD_MS)) <= xTaskGetTickCount())) {
        /* Retry LINK_CHECK_REQUEST */
        loRaWANctx.FsmState = Fsm_MAC_LinkCheckReq;
      }
    }

  }  // switch ()
}
