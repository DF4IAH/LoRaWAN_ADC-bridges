/*
 * task_gnss.h
 *
 *  Created on: 17.06.2019
 *      Author: DF4IAH
 */

#ifndef TASK_GNSS_H_
#define TASK_GNSS_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GNSS_EXTRA

/* Bit-mask for the gpscomEventGroup */
typedef enum Gpscom_EGW_BM {
 Gpscom_EGW__QUEUE_IN                = 0x00000001UL,
 Gpscom_EGW__DMA_TX_END              = 0x00000010UL,
 Gpscom_EGW__DMA_RX50                = 0x00000020UL,
 Gpscom_EGW__DMA_RX100_END           = 0x00000040UL,
 Gpscom_EGW__DMA_TXRX_ERROR          = 0x00000080UL,
 Gpscom_EGW__DMA_TX_RUN              = 0x00000100UL,
 Gpscom_EGW__DMA_RX_RUN              = 0x00000200UL,
 Gpscom_EGW__TIMER_SERVICE_RX        = 0x00002000UL,
} Gpscom_EGW_BM_t;

/* Command types for the gpscomInQueue */
typedef enum gpscomInQueueCmds {
 gpscomInQueueCmds__NOP              = 0,
 gpscomInQueueCmds__NmeaSendToGPS,
} gpscomInQueueCmds_t;

/* Command types for the gpscomOutQueue */
typedef enum gpscomOutQueueCmds {
 gpscomOutQueueCmds__NOP             = 0,
 gpscomOutQueueCmds__EndOfParse,
} gpscomOutQueueCmds_t;


#define Gps_Rcvr_Channels             12
#define Gps_Channels                  24

typedef enum GpsPosIndicatorValues {
GpsPosInd__0_FixNotAvailable         = 0,
GpsPosInd__1_GPS_SPS_Mode_FixValid,
GpsPosInd__2_DGPS_SPS_Mode_FixValid,
GpsPosInd__3_NotSupported,
GpsPosInd__4_NotSupported,
GpsPosInd__5_NotSupported,
GpsPosInd__6_DeadReck_FixValid
} GpsPosIndicatorValues_t;

typedef enum GpsStatus {
 GpsStatus__notValid                 = 0,
 GpsStatus__valid
} GpsStatus_t;

typedef enum GpsMode {
 GpsMode__A_Autonomous               = 0,
 GpsMode__D_DGPS,
 GpsMode__N_DatNotValid,
 GpsMode__R_CoarsePosition,
 GpsMode__S_Simulator
} GpsMode_t;

typedef enum GpsMode1 {
 GpsMode1__M_Manual                  = 0,
 GpsMode1__A_Automatic
} GpsMode1_t;

typedef enum GpsMode2 {
 GpsMode2__unset                     = 0,
 GpsMode2__1_NoFix,
 GpsMode2__2_2D,
 GpsMode2__3_3D,
} GpsMode2_t;


typedef struct GnssCtx {

 /* Last timestamp */
 float                               time;
 int32_t                             date;

 /* Status and modes */
 GpsStatus_t                         status;
 GpsPosIndicatorValues_t             piv;
 GpsMode_t                           mode;
 GpsMode1_t                          mode1;
 GpsMode2_t                          mode2;
 int32_t                             bootMsg;

 /* 3D position */
 float                               lat_deg;
 float                               lon_deg;
 float                               alt_m;

 /* Motion */
 float                               speed_kts;                                                // 1 kts = 1 nm / h    &   1 nm = 1,852 m
 float                               course_deg;

 /* Dilution of precision */
 float                               pdop;
 float                               hdop;
 float                               vdop;

 /* Sats count */
 uint8_t                             satsUse;
 uint8_t                             satsView;

 /* Channel array - GPS (12ch), GLONASS (12ch) */
 int32_t                             sv[   Gps_Channels << 1];
 int32_t                             sElev[Gps_Channels << 1];
 int32_t                             sAzim[Gps_Channels << 1];
 int32_t                             sSNR[ Gps_Channels << 1];

} GnssCtx_t;



void StartGnssRxTask(void const * argument);

void gnssTaskInit(void);
void gnssTaskLoop(void);


#endif  // GNSS_EXTRA

#ifdef __cplusplus
}
#endif

#endif /* TASK_GNSS_H_ */
