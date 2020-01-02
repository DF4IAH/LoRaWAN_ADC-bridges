/*
 * task_gnss.c
 *
 *  Created on: 17.06.2019
 *      Author: DF4IAH
 */



/* Includes ------------------------------------------------------------------*/

#include <stddef.h>
#include <sys/_stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "stm32l4xx_hal.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"

#include "main.h"
#include "task_LoRaWAN.h"
#include "task_adc.h"

#include "task_gnss.h"


#ifdef GNSS_EXTRA

#define GNSS_RX_BUF_SIZE            256
#define GNSS_TX_BUF_SIZE            64


/* Variables -----------------------------------------------------------------*/

osThreadId                          gnssRxTaskHandle;
extern EventGroupHandle_t           adcEventGroupHandle;
extern osMutexId                    gnssCtxMutexHandle;
extern osMessageQId                 gnssRxQueueHandle;

extern uint8_t                      g_gnss_reset;
extern volatile float               g_gnss_lat;
extern volatile float               g_gnss_lon;
extern volatile float               g_gnss_alt_m;
extern volatile float               g_gnss_acc;
# ifdef GNSS_EXTRA_SPEED_COURSE
extern volatile float               g_gnss_spd_kmh;
extern volatile float               g_gnss_crs;
# endif


volatile GnssCtx_t                  gnssCtx                                 = { 0 };

volatile uint8_t                    gnss_rx_buf[GNSS_RX_BUF_SIZE]           = { 0 };
volatile uint8_t                    gnss_tx_buf[GNSS_TX_BUF_SIZE]           = { 0 };

uint8_t                             gnss_rx_LineBuf[GNSS_RX_BUF_SIZE >> 1]  = { 0 };
uint8_t                             gnss_rx_LineLen                         = 0U;


/* Local functions ----------------------------------------------------------*/

static void gnssCheckComplete(void)
{
  if (g_gnss_lat     &&
      g_gnss_lon     &&
      g_gnss_alt_m   &&
#ifdef GNSS_EXTRA_SPEED_COURSE
      g_gnss_spd_kmh &&
#endif
      (g_gnss_acc <= 2.5f))
  {
    __DSB();
    __ISB();

    /* All fields are set */
    xEventGroupSetBits(adcEventGroupHandle, EG_GNSS__AVAIL_POS);
  }
}

static float gnssCalcNmeaLonLat_float(float dddmm, uint8_t indicator)
{
  float latLon_deg  = floor(dddmm / 100.);
  float latLon_min  = dddmm - (latLon_deg * 100.);

  latLon_deg += latLon_min / 60.;

  /* Sign reversal */
  if ((indicator == 'W') || (indicator == 'S')) {
    latLon_deg = -latLon_deg;
  }

  return latLon_deg;
}

uint8_t gnssNmeaChecksum(char* outBuf, const uint8_t* inBuf, uint16_t len)
{
  char l_crcPad[3] = { 0 };

  if ((len > 10) && inBuf) {
    const uint16_t starIdx = len - 5;

    if (*(inBuf + starIdx) == '*') {
      uint8_t sum = 0;

      for (uint16_t idx = 1; idx < starIdx; idx++) {
        sum ^= *(inBuf + idx);
      }
      sprintf(l_crcPad, "%02X", sum);

      if (outBuf) {
        memcpy(outBuf, l_crcPad, 2);
      }
      return ((l_crcPad[0] == inBuf[starIdx + 1]) && (l_crcPad[1] == inBuf[starIdx + 2])) ?  1 : 0;
    }
  }
  return 0;
}

static uint16_t gnssNmeaStringParser__forwardColon(const char* inBuf, uint16_t inLen, uint16_t inBufIdx)
{
  char c = *(inBuf + inBufIdx++);

  /* Forward to next colon */
  while (c != ',') {
    if (inBufIdx >= inLen) {
      return min(inBufIdx, inLen);
    }

    c = *(inBuf + inBufIdx++);
  }

  /* Return one position after the colon */
  return inBufIdx;
}

static HAL_StatusTypeDef gnssNmeaStringParser(const char* inBuf, uint16_t inLen, const char* fmtAry, float* fAry, int32_t* iAry, uint8_t* bAry)
{
  const uint8_t fmtLen  = strlen(fmtAry);
  uint8_t       fmtIdx  = 0;
  uint16_t      inIdx   = 0U;
  uint8_t       fAryIdx = 0U;
  uint8_t       iAryIdx = 0U;
  uint8_t       bAryIdx = 0U;

  while (fmtIdx < fmtLen) {
    char fmt = *(fmtAry + fmtIdx++);

    switch (fmt) {
    case 'f':
      {
        /* Floating point */
        if (*(inBuf + inIdx) != ',') {
          float f = atoff(inBuf + inIdx);
          fAry[fAryIdx++] = f;
          inIdx = gnssNmeaStringParser__forwardColon(inBuf, inLen, inIdx);

        } else {
          fAry[fAryIdx++] = 0.f;
        }

      }
      break;

    case 'i':
      {
        /* integer */
        if (*(inBuf + inIdx) != ',') {
          int i = atoi(inBuf + inIdx);
          iAry[iAryIdx++] = i;

        } else {
          iAry[iAryIdx++] = 0;
        }

        inIdx = gnssNmeaStringParser__forwardColon(inBuf, inLen, inIdx);
      }
      break;

    case 'b':
      {
        /* byte */
        if (inIdx < inLen) {
          uint8_t b = *(((uint8_t*) inBuf) + inIdx++);
          if (b != ',') {
            bAry[bAryIdx++] = b;
            inIdx++;

          } else {
            bAry[bAryIdx++] = 0U;
          }
        }
      }
      break;

    default:
      /* Bad format identifier */
      return HAL_ERROR;
    }
  }
  return HAL_OK;
}

static void gnssNmeaParser__GGA(const char* buf, uint16_t len)
{
  float   fAry[6] = { 0. };
  int32_t iAry[4] = { 0L };
  uint8_t bAry[4] = { 0U };
  uint8_t fIdx = 0, iIdx = 0, bIdx = 0;

  gnssNmeaStringParser(buf + 7, len - 7, "ffbfbiiffbfbii", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    gnssCtx.time     = fAry[fIdx++];
    gnssCtx.lat_deg  = g_gnss_lat = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);
    gnssCtx.lon_deg  = g_gnss_lon = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);
    gnssCtx.piv      = (GpsPosIndicatorValues_t) iAry[iIdx++];
    gnssCtx.satsUse  = iAry[iIdx++];
    gnssCtx.hdop     = fAry[fIdx++];
    gnssCtx.alt_m    = g_gnss_alt_m = fAry[fIdx++];

    /* Hack to calculate the accuracy in meters from the HDOP */
    g_gnss_acc = gnssCtx.hdop * 3.0f;

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__GLL(const char* buf, uint16_t len)
{
  float   fAry[3] = { 0. };
  int32_t iAry[0] = {    };
  uint8_t bAry[4] = { 0U };
  uint8_t fIdx = 0, /* iIdx = 0, */ bIdx = 0;

  gnssNmeaStringParser(buf + 7, len - 7, "fbfbfbb", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    gnssCtx.lat_deg  = g_gnss_lat = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);
    gnssCtx.lon_deg  = g_gnss_lon = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);
    gnssCtx.time     = fAry[fIdx++];

    const char status     = bAry[bIdx++];
    gnssCtx.status   = (status == 'A') ?  GpsStatus__valid : (status == 'V') ?  GpsStatus__notValid : gnssCtx.status;

    const char mode       = bAry[bIdx++];
    switch (mode) {
    case 'A':
      gnssCtx.mode   = GpsMode__A_Autonomous;
      break;

    case 'D':
      gnssCtx.mode   = GpsMode__D_DGPS;
      break;

    case 'N':
      gnssCtx.mode   = GpsMode__N_DatNotValid;
      break;

    case 'R':
      gnssCtx.mode   = GpsMode__R_CoarsePosition;
      break;

    case 'S':
      gnssCtx.mode   = GpsMode__S_Simulator;
      break;

    default:
      { /* Skip setting */ }
    }

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__GSA(const char* buf, uint16_t len)
{
  float   fAry[ 3] = { 0. };
  int32_t iAry[13] = { 0L };
  uint8_t bAry[ 1] = { 0U };
  uint8_t fIdx = 0, iIdx = 0, bIdx = 0;

  gnssNmeaStringParser(buf + 7, len - 7, "biiiiiiiiiiiiifff", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    /* Status and modes */
    const char mode1      = bAry[bIdx++];
    gnssCtx.mode1    = (mode1 == 'A') ?  GpsMode1__A_Automatic : (mode1 == 'M') ?  GpsMode1__M_Manual : gnssCtx.mode1;
    gnssCtx.mode2    = (GpsMode2_t) iAry[iIdx++];

#if 0  // Done by GSV
    /* Satellite receiver channels */
    for (uint8_t ch = 1; ch <= Gps_Rcvr_Channels; ch++) {
      gpscomGpsCtx.sv[ch - 1] = iAry[iIdx++];
    }
#endif

    /* Dilution of precision */
    gnssCtx.pdop     = fAry[fIdx++];
    gnssCtx.hdop     = fAry[fIdx++];
    gnssCtx.vdop     = fAry[fIdx++];

    /* Hack to calculate the accuracy in meters from the HDOP */
    g_gnss_acc = gnssCtx.hdop * 2.0f;

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__GSV(const char* buf, uint16_t len)
{
  float   fAry[ 0] = {    };
  int32_t iAry[19] = { 0L };
  uint8_t bAry[ 0] = {    };
  uint8_t /* fIdx = 0, */ iIdx = 0 /*, bIdx = 0 */ ;

  gnssNmeaStringParser(buf + 7, len - 7, "iiiiiiiiiiiiiiiiiii", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    /* Pages */
    const int32_t pages     = iAry[iIdx++];
    const int32_t thisPage  = iAry[iIdx++];
    gnssCtx.satsView        = iAry[iIdx++];

    /* Sanity check */
    if (thisPage < 1 || thisPage > pages) {
      return;
    }

    /* Satellite channels (GPS / GLONASS) */
    {
      uint8_t chStart = 0;
      uint8_t chEnd   = 0;
      if (buf[2] == 'P') {
        /* GPS entries */
        chStart =  1 + ((thisPage - 1) << 2);
        chEnd   =  min((thisPage << 2), gnssCtx.satsView);

      } else if (buf[2] == 'N' ||
                 buf[2] == 'D') {
        /* GLONASS / BEIDOU entries */
        chStart = Gps_Channels + 1 + ((thisPage - 1) << 2);
        chEnd   = Gps_Channels + min((thisPage << 2), gnssCtx.satsView);
      }

      if (chStart && chEnd) {
        /* All channels in use */
        for (uint8_t ch = chStart; ch <= chEnd; ch++) {
          gnssCtx.sv   [ch - 1]  = iAry[iIdx++];
          gnssCtx.sElev[ch - 1]  = iAry[iIdx++];
          gnssCtx.sAzim[ch - 1]  = iAry[iIdx++];
          gnssCtx.sSNR [ch - 1]  = iAry[iIdx++];
        }
      }
    }

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__RMC(const char* buf, uint16_t len)
{
  float   fAry[5] = { 0. };
  int32_t iAry[2] = { 0L };
  uint8_t bAry[5] = { 0U };
  uint8_t fIdx = 0, iIdx = 0, bIdx = 0;

  gnssNmeaStringParser(buf + 7, len - 7, "fbfbfbffiibb", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    gnssCtx.time            = fAry[fIdx++];

    const char status       = bAry[bIdx++];
    gnssCtx.status          = (status == 'A') ?  GpsStatus__valid : (status == 'V') ?  GpsStatus__notValid : gnssCtx.status;

    gnssCtx.lat_deg         = g_gnss_lat = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);
    gnssCtx.lon_deg         = g_gnss_lon = gnssCalcNmeaLonLat_float(fAry[fIdx++], bAry[bIdx++]);

    gnssCtx.speed_kts       = g_gnss_spd_kmh  = fAry[fIdx++];
    g_gnss_spd_kmh         *= 1.852f;
    gnssCtx.course_deg      = g_gnss_crs      = fAry[fIdx++];

    gnssCtx.date            = iAry[iIdx++];

    const int32_t magVar    = iAry[iIdx++];
    (void) magVar;

    const uint8_t magSen    = bAry[bIdx++];
    (void) magSen;

    const char mode         = bAry[bIdx++];
    switch (mode) {
    case 'A':
      gnssCtx.mode          = GpsMode__A_Autonomous;
      break;

    case 'D':
      gnssCtx.mode          = GpsMode__D_DGPS;
      break;

    case 'N':
      gnssCtx.mode          = GpsMode__N_DatNotValid;
      break;

    case 'R':
      gnssCtx.mode          = GpsMode__R_CoarsePosition;
      break;

    case 'S':
      gnssCtx.mode          = GpsMode__S_Simulator;
      break;

    default:
      { /* Skip setting */ }
    }

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__VTG(const char* buf, uint16_t len)
{
  float   fAry[4] = { 0. };
  int32_t iAry[0] = {    };
  uint8_t bAry[5] = { 0U };
  uint8_t fIdx = 0, /* iIdx = 0, */ bIdx = 0;

  gnssNmeaStringParser(buf + 7, len - 7, "fbfbfbfbb", fAry, iAry, bAry);

  /* Try to get the mutex within that short time or drop the message */
  if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
  {
    gnssCtx.course_deg      = g_gnss_crs = fAry[fIdx++];
    const uint8_t cRef1     = bAry[bIdx++];
    (void) cRef1;

    const float c2          = fAry[fIdx++];
    const uint8_t cRef2     = bAry[bIdx++];
    (void) c2;
    (void) cRef2;

    gnssCtx.speed_kts       = g_gnss_spd_kmh = fAry[fIdx++];
    g_gnss_spd_kmh         *= 1.852f;
    const uint8_t spdU      = bAry[bIdx++];
    (void) spdU;

    const float   spd2      = fAry[fIdx++];
    const uint8_t spdU2     = bAry[bIdx++];
    (void) spd2;
    (void) spdU2;

    const char mode         = bAry[bIdx++];
    switch (mode) {
    case 'A':
      gnssCtx.mode          = GpsMode__A_Autonomous;
      break;

    case 'D':
      gnssCtx.mode          = GpsMode__D_DGPS;
      break;

    case 'N':
      gnssCtx.mode          = GpsMode__N_DatNotValid;
      break;

    case 'R':
      gnssCtx.mode          = GpsMode__R_CoarsePosition;
      break;

    case 'S':
      gnssCtx.mode          = GpsMode__S_Simulator;
      break;

    default:
      { /* Skip setting */ }
    }

    /* Give mutex back */
    xSemaphoreGive(gnssCtxMutexHandle);
  }
}

static void gnssNmeaParser__PMT(const char* buf, uint16_t len)
{
  float   fAry[0] = {    };
  int32_t iAry[1] = { 0  };
  uint8_t bAry[0] = {    };
  uint8_t /* fIdx = 0, */ iIdx = 0 /*,  bIdx = 0 */ ;

  if (!strncmp(buf + 1, "PMTK010", 7)) {
    gnssNmeaStringParser(buf + 7, len - 7, "i", fAry, iAry, bAry);

    /* Try to get the mutex within that short time or drop the message */
    if (pdTRUE == xSemaphoreTake(gnssCtxMutexHandle, 1000UL / portTICK_PERIOD_MS))
    {
      const int32_t msg     = iAry[iIdx++];
      gnssCtx.bootMsg       = msg;

      /* Give mutex back */
      xSemaphoreGive(gnssCtxMutexHandle);
    }
  }
}

static void gnssNmeaParser(const char* buf, uint16_t len)
{
  if        ((!strncmp(buf + 1, "GPGGA", 5)) ||
             (!strncmp(buf + 1, "GNGGA", 5))) {
    /* Global Positioning System Fix Data */
    gnssNmeaParser__GGA(buf, len);

  } else if ((!strncmp(buf + 1, "GPGLL", 5)) ||
             (!strncmp(buf + 1, "GNGLL", 5))) {
    /* GLL - Geographic Position - Latitude / Longitude */
    gnssNmeaParser__GLL(buf, len);

  } else if ((!strncmp(buf + 1, "GPGSA", 5)) ||
             (!strncmp(buf + 1, "GNGSA", 5)) ||
             (!strncmp(buf + 1, "BDGSA", 5))) {
    /* GSA - GNSS DOP and Active Satellites */
    gnssNmeaParser__GSA(buf, len);

  } else if ((!strncmp(buf + 1, "GPGSV", 5)) ||
             (!strncmp(buf + 1, "GLGSV", 5)) ||
             (!strncmp(buf + 1, "BDGSV", 5))) {
    /* GSV - GNSS Satellites in View */
    gnssNmeaParser__GSV(buf, len);

  } else if ((!strncmp(buf + 1, "GPRMC", 5)) ||
             (!strncmp(buf + 1, "GNRMC", 5))) {
    /* RMC - Recommended Minimum Specific GNSS Data */
    gnssNmeaParser__RMC(buf, len);

  } else if ( !strncmp(buf + 1, "GPVTG", 5)) {
    /* VTG - Course Over Ground and Ground Speed */
    gnssNmeaParser__VTG(buf, len);

  } else if (!strncmp(buf + 1, "PMT", 3)) {
    /* System response messages */
    gnssNmeaParser__PMT(buf, len);
  }
}


// #define NMEA_DEBUG
static void gnssNmeaInterpreter(const uint8_t* buf, uint16_t len)
{
  const uint8_t chkVld = gnssNmeaChecksum(NULL, buf, len);
  if (chkVld) {
    gnssNmeaParser((const char*) buf, len);
  }
}


/* Calculate Date and Time to un*x time */
uint32_t gnssCalcDataTime_to_unx_s(float* out_s_frac, uint32_t gnss_date, float gnss_time)
{
  time_t l_unx_s;

  {
    struct tm timp  = { 0 };

    uint16_t year   = gnss_date % 100;
    uint8_t  month  = (gnss_date / 100) % 100;
    uint8_t  mday   = (gnss_date / 10000);
    uint8_t  hour   = (uint8_t) (((uint32_t)  gnss_time) / 10000UL);
    uint8_t  minute = (uint8_t) ((((uint32_t) gnss_time) / 100) % 100);
    uint8_t  second = (uint8_t) (((uint32_t)  gnss_time) % 100);

    /* Sanity checks */
    if (month < 1) {
      return 0.f;
    }

    timp.tm_year    = year + 100;
    timp.tm_mon     = month - 1;
    timp.tm_mday    = mday;
    timp.tm_hour    = hour;
    timp.tm_min     = minute;
    timp.tm_sec     = second;
    l_unx_s         = mktime(&timp);
  }

  if (out_s_frac){
    float l_s_frac = gnss_time - floor(gnss_time);
    *out_s_frac = l_s_frac;
  }

  return l_unx_s;
}


/* Callback functions */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  BaseType_t taskWoken = 0UL;
  xEventGroupSetBitsFromISR(adcEventGroupHandle, EG_GNSS__RX_DMA_END, &taskWoken);
  xEventGroupClearBitsFromISR(adcEventGroupHandle, EG_GNSS__RX_DMA_RUN);

  /* Restart DMA from the starting point */
  xEventGroupSetBitsFromISR(adcEventGroupHandle, EG_GNSS__RX_DMA_RUN, &taskWoken);
  xEventGroupClearBitsFromISR(adcEventGroupHandle, EG_GNSS__RX_DMA_END);
  HAL_StatusTypeDef status = HAL_UART_Receive_DMA(huart, (uint8_t*)gnss_rx_buf, GNSS_RX_BUF_SIZE);
  if (status != HAL_OK) {
    Error_Handler();
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  HAL_UART_AbortTransmit(huart);

  BaseType_t taskWoken = 0UL;
  xEventGroupSetBitsFromISR(adcEventGroupHandle, EG_GNSS__TX_DMA_END, &taskWoken);
  xEventGroupClearBitsFromISR(adcEventGroupHandle, EG_GNSS__TX_DMA_RUN);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  /* Do drop this messages - data can come in before module is being set-up */
}


/* GNSS RX line task slicing lines from the DMA buffer */

static void gnssRxPushLine(uint16_t idxPullDone, uint16_t idxPullAdvance)
{
  if (idxPullDone == idxPullAdvance) {
    return;
  }

  /* Correct for loop-buffer */
  if (idxPullAdvance < idxPullDone) {
    idxPullAdvance += GNSS_RX_BUF_SIZE;
  }

  /* Push length of line in RX queue */
  const uint8_t rxLen = 1U + idxPullAdvance - idxPullDone;
  xQueueSendToBack(gnssRxQueueHandle, &rxLen, 5UL / portTICK_PERIOD_MS);

  for (uint16_t idx = idxPullDone; idx <= idxPullAdvance; idx++) {
    /* Get character from loop-buffer */
    const uint8_t c = gnss_rx_buf[idx % GNSS_RX_BUF_SIZE];

    /* Push in RX queue */
    xQueueSendToBack(gnssRxQueueHandle, &c, 5UL / portTICK_PERIOD_MS);
  }

  /* Clear ring-buffer */
  memset((uint8_t*) &(gnss_rx_buf[idxPullDone % GNSS_RX_BUF_SIZE]), 0, rxLen);
}

void StartGnssRxTask(void const * argument)
{
  /* Pull running data from the RX DMA ring-buffer */

  uint16_t idxPullDone    = 0U;

  /* Wait until RX RMA is running */
  xEventGroupWaitBits(adcEventGroupHandle, EG_GNSS__RX_DMA_RUN, 0UL, pdFALSE, portMAX_DELAY);

  while (true) {
    /* Advance until last character */
    for (uint16_t idxPullAdvance = idxPullDone; idxPullAdvance < (idxPullDone + GNSS_RX_BUF_SIZE); idxPullAdvance++) {
      /* Get ringbuffer entry */
      const char c = gnss_rx_buf[idxPullAdvance % GNSS_RX_BUF_SIZE];

      /* Check character */
      if (!c) {
        /* no more characters found */
        break;

      } else if (c == '\n') {
        /* Push out line */
        gnssRxPushLine(idxPullDone, idxPullAdvance);

        /* Adjust left side of string */
        idxPullDone = idxPullAdvance;
        idxPullDone++;
      }
    }

    /* Correct loop buffer idx */
    idxPullDone %= GNSS_RX_BUF_SIZE;

    /* Step time */
    osDelay(100UL);
  }
}

void gnssTaskInit(void)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Init  LPUART1_SIM_TX (PC1)  and  LPUART1_SIM_RX (PC0) */
  MX_LPUART1_UART_Init();

  if(HAL_UART_DeInit(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if(HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Init TX and RX buffers */
  // already 0

  osThreadDef(gnssRxTask, StartGnssRxTask, osPriorityAboveNormal, 0, 256);
  gnssRxTaskHandle = osThreadCreate(osThread(gnssRxTask), NULL);


  xEventGroupSetBits(adcEventGroupHandle, EG_GNSS__RX_DMA_RUN);
  xEventGroupClearBits(adcEventGroupHandle, EG_GNSS__RX_DMA_END);
  status = HAL_UART_Receive_DMA(&hlpuart1, (uint8_t*)gnss_rx_buf, GNSS_RX_BUF_SIZE);
  if (status != HAL_OK) {
    Error_Handler();
  }


  /* TX: GNSS setup */
  const uint8_t l_gnss_tx_initBuf[] = "\r\n\r\n";
  const int     l_gnss_tx_initLen   = strlen((char*)l_gnss_tx_initBuf);
  memcpy((uint8_t*)gnss_tx_buf, l_gnss_tx_initBuf, l_gnss_tx_initLen);
  __DSB();
  __ISB();

  xEventGroupSetBits(adcEventGroupHandle, EG_GNSS__TX_DMA_RUN);
  status = HAL_UART_Transmit_DMA(&hlpuart1, (uint8_t* const) gnss_tx_buf, l_gnss_tx_initLen);
  if (status != HAL_OK) {
    Error_Handler();
  }
}

void gnssTaskLoop(void)
{
  uint8_t cnt = 0U;

  gnss_rx_LineLen = 0U;

  BaseType_t status = xQueueReceive(gnssRxQueueHandle, &cnt, 1000UL / portTICK_PERIOD_MS);
  if (status == pdPASS) {
    for (uint8_t idx = 0U; idx < cnt; idx++) {
      status = xQueueReceive(gnssRxQueueHandle, &(gnss_rx_LineBuf[idx]), 1000UL / portTICK_PERIOD_MS);
      if (status != pdPASS) {
        Error_Handler();
      }
    }
    gnss_rx_LineLen = cnt;

    /* Process NMEA message */
    gnssNmeaInterpreter(gnss_rx_LineBuf, gnss_rx_LineLen);

    /* Check if all fields are set */
    gnssCheckComplete();

    /* Invalidate line */
    gnss_rx_LineLen = 0U;
    memset(gnss_rx_LineBuf, 0, sizeof(gnss_rx_LineBuf));

    /* Check if reset flag is set */
    if (g_gnss_reset) {
#if 1
      /* Warm reset NMEA message for the MediaTek MT3333 chipset */
      const uint8_t resetBuf[] = "\r\n\r\n$PMTK102*31\r\n";
#else
      /* Cold reset NMEA message for the MediaTek MT3333 chipset */
      const uint8_t resetBuf[] = "\r\n\r\n$PMTK103*30\r\n";
#endif
      HAL_UART_Transmit_DMA(&hlpuart1, (uint8_t*)resetBuf, strlen((char*)resetBuf));

      /* Reset flag */
      g_gnss_reset = 0U;
    }

  } else {
    osDelay(100UL);
  }
}

#endif
