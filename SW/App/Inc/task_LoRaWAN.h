/*
 * LoRaWAN.h
 *
 *  Created on: 12.05.2018
 *      Author: DF4IAH
 */

#ifndef LORAWAN_H_
#define LORAWAN_H_

#ifdef __cplusplus
 extern "C" {
#endif


#include "stm32l4xx_hal.h"


#define LoRaWAN_MsgLenMax             51
#define LoRaWAN_FRMPayloadMax         48


/* UTC --> GPS  leap seconds adjustment: +18 s as of 2016-12-31 */
#define GPS_UTC_LEAP_SECS             (+18)


/* Default settings */
#define  LORAWAN_CONFPACKS_DEFAULT    0
#define  LORAWAN_ADR_ENABLED_DEFAULT  1



/* Bit-mask for the loRaWANEventGroup */
typedef enum Lora_EGW_BM {

  Lora_EGW__QUEUE_IN                  = 0x00000001UL,
  Lora_EGW__QUEUE_OUT                 = 0x00000002UL,
  Lora_EGW__DATA_UP_REQ               = 0x00000004UL,
  Lora_EGW__DATA_UP_DONE              = 0x00000008UL,

  Lora_EGW__DO_INIT                   = 0x00000010UL,
  Lora_EGW__DO_LINKCHECKREQ           = 0x00000020UL,
//Lora_EGW__DO_DEVICETIMEREQ          = 0x00000040UL,

  Lora_EGW__DATAPRESET_OK             = 0x00000100UL,
  Lora_EGW__JOINACCEPT_OK             = 0x00000200UL,
  Lora_EGW__LINKCHECK_OK              = 0x00000400UL,
//Lora_EGW__DEVICETIME_OK             = 0x00000800UL,
  Lora_EGW__LINK_ESTABLISHED          = 0x00001000UL,
  Lora_EGW__ADR_DOWNACTIVATE          = 0x00002000UL,
  Lora_EGW__TRANSPORT_READY           = 0x00008000UL,

} Lora_EGW_BM_t;

/* Command types for the loraInQueue */
typedef enum LoraInQueueCmds {

  LoraInQueueCmds__NOP                = 0,
  LoraInQueueCmds__Init,
  LoraInQueueCmds__LinkCheckReq,
  LoraInQueueCmds__DeviceTimeReq,
  LoraInQueueCmds__ConfirmedPackets,
  LoraInQueueCmds__ADRset,
  LoraInQueueCmds__PwrRedDb,
  LoraInQueueCmds__DRset,
  LoraInQueueCmds__IoT4BeesApplUp,
  LoraInQueueCmds__TrackMeApplUp,

} LoraInQueueCmds_t;

/* Command types for the loraOutQueue */
typedef enum LoraOutQueueCmds {

  LoraOutQueueCmds__NOP                = 0,
  LoraOutQueueCmds__Connected,

} LoraOutQueueCmds_t;


typedef enum DataRates {

  DR0_SF12_125kHz_LoRa                = 0,
  DR1_SF11_125kHz_LoRa,
  DR2_SF10_125kHz_LoRa,
  DR3_SF9_125kHz_LoRa,
  DR4_SF8_125kHz_LoRa,
  DR5_SF7_125kHz_LoRa,
  DR6_SF7_250kHz_LoRa,
  DR7_50kHz_FSK,                                                                                // nRF905 "OGN Tracking Protocol" compatible?

} DataRates_t;


/* FSM states of the loRaWANLoRaWANTaskLoop */
typedef enum Fsm_States {

  Fsm_NOP                             = 0,

  Fsm_RX1                             = 0x01,
  Fsm_RX2,
  Fsm_JoinRequestRX1,
  Fsm_JoinRequestRX2,

  Fsm_TX                              = 0x09,

  Fsm_MAC_Decoder                     = 0x10,

  Fsm_MAC_Proc,

  Fsm_MAC_JoinRequest,
  Fsm_MAC_JoinAccept,

  Fsm_MAC_LinkCheckReq,
  Fsm_MAC_LinkCheckAns,

  Fsm_MAC_LinkADRReq,
  Fsm_MAC_LinkADRAns,

  Fsm_MAC_DutyCycleReq,
  Fsm_MAC_DutyCycleAns,

  Fsm_MAC_RXParamSetupReq,
  Fsm_MAC_RXParamSetupAns,

  Fsm_MAC_DevStatusReq,
  Fsm_MAC_DevStatusAns,

  Fsm_MAC_NewChannelReq,
  Fsm_MAC_NewChannelAns,

  Fsm_MAC_RXTimingSetupReq,
  Fsm_MAC_RXTimingSetupAns,

  Fsm_MAC_TxParamSetupReq,
  Fsm_MAC_TxParamSetupAns,

  Fsm_MAC_DlChannelReq,
  Fsm_MAC_DlChannelAns,

  Fsm_MAC_DeviceTimeReq,
  Fsm_MAC_DeviceTimeAns,

} Fsm_States_t;



/* --- LoRaWAN --- */

typedef enum LoRaWAN_CalcMIC_JOINREQUEST {

  MIC_JOINREQUEST                     = 0x01,
  MIC_DATAMESSAGE                     = 0x11,

} LoRaWAN_CalcMIC_VARIANT_t;


typedef enum CurrentWindow {

  CurWin_none                         = 0,
  CurWin_RXTX1,
  CurWin_RXTX2

} CurrentWindow_t;

/* LoRaWAN RX windows */
typedef enum LoRaWAN_RX_windows {

  LORAWAN_RX1_PREPARE_MS              =  150,                                                   // Minimum 146 ms

  LORAWAN_EU868_DELAY1_MS             = 1000,
  LORAWAN_EU868_DELAY2_MS             = 2000,

  LORAWAN_EU868_JOIN_ACCEPT_DELAY1_MS = 5000,
  LORAWAN_EU868_JOIN_ACCEPT_DELAY2_MS = 6000,

  LORAWAN_EU868_MAX_RX1_DURATION_MS   =  825,                                                   // Maximum 825 ms
  LORAWAN_EU868_MAX_TX_DURATION_MS    = 2800,                                                   // max 2793.5 ms @ 59 byte MAC length

} LoRaWAN_RX_windows_t;


/* MHDR */
typedef enum LoRaWAN_MTypeBF {

  JoinRequest                         = 0,
  JoinAccept                          = 1,
  UnconfDataUp                        = 2,
  UnconfDataDn                        = 3,
  ConfDataUp                          = 4,
  ConfDataDn                          = 5,
  RejoinReq                           = 6,
  Proprietary                         = 7

} LoRaWAN_MTypeBF_t;

typedef enum LoRaWAN_MajorBF {

  LoRaWAN_R1                          = 0,

} LoRaWAN_MajorBF_t;

#define LoRaWAN_MHDR_MType_SHIFT      5
#define LoRaWAN_MHDR_Major_SHIFT      0
#define MHDR_SIZE                     (sizeof(uint8_t))

typedef enum LoRaWANVersion {

  LoRaWANVersion_10                   = 0,
  LoRaWANVersion_11                   = 1,

} LoRaWANVersion_t;


/* FCtl */
#define LoRaWAN_FCtl_ADR_SHIFT        7
#define LoRaWAN_FCtl_ADRACKReq_SHIFT  6
#define LoRaWAN_FCtl_ACK_SHIFT        5
#define LoRaWAN_FCtl_ClassB_SHIFT     4
#define LoRaWAN_FCtl_FPending_SHIFT   4
#define LoRaWAN_FCtl_FOptsLen_SHIFT   0


/* FPort */
#define FPort_Appl_Default            1


/* LoRaWAN direction of transmission - Up: device --> gateway / Dn: device <-- gateway */
typedef enum LoRaWANctxDir {

  Up                                  = 0,
  Dn                                  = 1

} LoRaWANctxDir_t;


typedef enum LoRaWANMAC_CID {

  ResetInd_UP                         = 0x01,                                                   // ABP devices only, LW 1.1
  ResetConf_DN                        = 0x01,                                                   // ABP devices only, LW 1.1

  LinkCheckReq_UP                     = 0x02,
  LinkCheckAns_DN                     = 0x02,

  LinkADRReq_DN                       = 0x03,
  LinkADRAns_UP                       = 0x03,

  DutyCycleReq_DN                     = 0x04,
  DutyCycleAns_UP                     = 0x04,

  RXParamSetupReq_DN                  = 0x05,
  RXParamSetupAns_UP                  = 0x05,

  DevStatusReq_DN                     = 0x06,
  DevStatusAns_UP                     = 0x06,

  NewChannelReq_DN                    = 0x07,
  NewChannelAns_UP                    = 0x07,

  RXTimingSetupReq_DN                 = 0x08,
  RXTimingSetupAns_UP                 = 0x08,

  TxParamSetupReq_DN                  = 0x09,
  TxParamSetupAns_UP                  = 0x09,

  DlChannelReq_DN                     = 0x0A,
  DlChannelAns_UP                     = 0x0A,

  RekeyInd_UP                         = 0x0B,
  RekeyConf_DN                        = 0x0B,

  ADRParamSetupReq_DN                 = 0x0C,
  ADRParamSetupAns_UP                 = 0x0C,

  DeviceTimeReq_UP                    = 0x0D,
  DeviceTimeAns_DN                    = 0x0D,

  ForceRejoinReq_DN                   = 0x0E,

  RejoinParamSetupReq_DN              = 0x0F,
  RejoinParamSetupAns_UP              = 0x0F,

} LoRaWANMAC_CID_t;

typedef enum ChMaskCntl {

  ChMaskCntl__appliesTo_1to16         = 0,
  ChMaskCntl__allChannelsON           = 6,

} ChMaskCntl_t;


/* MIC calculation structures */
typedef struct FRMPayloadBlockA_Up {

  uint8_t                             variant;
  uint8_t                             _noUse[4];
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntUp[4];
  uint8_t                             _pad;
  uint8_t                             idx;

} FRMPayloadBlockA_Up_t;

#ifdef LORAWAN_1V02
typedef struct FRMPayloadBlockA_Dn {

  uint8_t                             variant;
  uint8_t                             _noUse[4];
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntDn[4];
  uint8_t                             _pad;
  uint8_t                             idx;

} FRMPayloadBlockA_Dn_t;
#endif

typedef struct MICBlockB0_Up {

  uint8_t                             variant;
  uint8_t                             _noUse[4];
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntUp[4];
  uint8_t                             _pad;
  uint8_t                             len;

} MICBlockB0_Up_t;

#ifdef LORAWAN_1V1
typedef struct MICBlockB1_Up {

  uint8_t                             variant;
  uint8_t                             ConfFCnt[2];
  uint8_t                             TxDr;
  uint8_t                             TxCh;
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntUp[4];
  uint8_t                             _pad;
  uint8_t                             len;

} MICBlockB1_Up_t;
#endif

#ifdef LORAWAN_1V02
typedef struct MICBlockB0_Dn {

  uint8_t                             variant;
  uint8_t                             _noUse[4];
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntDn[4];
  uint8_t                             _pad;
  uint8_t                             len;

} MICBlockB0_Dn_t;
#elif LORAWAN_1V1
typedef struct MICBlockB0_Dn {

  uint8_t                             variant;
  uint8_t                             ConfFCnt[2];
  uint8_t                             _noUse[2];
  uint8_t                             Dir;
  uint8_t                             DevAddr[4];
  uint8_t                             FCntDn[4];
  uint8_t                             _pad;
  uint8_t                             len;

} MICBlockB0_Dn_t;
#endif


/* Context information */
typedef struct LoRaWANctx {

  /* FSM */
  volatile Fsm_States_t               FsmState;

  /* Network / MAC specific */
  LoRaWANVersion_t                    LoRaWAN_ver;
#ifdef LORAWAN_1V1
  uint8_t                             __NwkKey_1V1[16];                                           // Network root key (for OTA devices)
  uint8_t                             FNwkSKey_1V1[16];                                         // JOIN-ACCEPT
  uint8_t                             FNwkSIntKey_1V1[16];                                      // JOIN-ACCEPT (derived from: NwkKey)
  uint8_t                             SNwkSIntKey_1V1[16];                                      // JOIN-ACCEPT (derived from: NwkKey)
  uint8_t                             NwkSEncKey_1V1[16];                                       // JOIN-ACCEPT (derived from: NwkKey)
  uint8_t                             JSIntKey_1V1[16];                                         // (for OTA devices)
  uint8_t                             JSEncKey_1V1[16];                                         // (for OTA devices)
#endif
  volatile uint8_t                    Ch_Selected;                                              // Fixed or randomly assigned channel
  volatile float                      Ch_FrequenciesUplink_MHz[16];                             // Default / MAC:NewChannelReq
  volatile float                      Ch_FrequenciesDownlink_MHz[16];                           // Default / MAC:NewChannelReq / MAC:DlChannelReq
  volatile float                      GatewayPpm;                                               // PPM value to adjust RX for remote gateway crystal drift

  /* MAC communicated data */
  volatile uint8_t                    ConfirmedPackets_enabled;                                 // Setting: global MType
  volatile uint8_t                    TX_MAC_Len;                                               // MAC list to be sent at next transmission
  volatile uint8_t                    TX_MAC_Buf[16];                                           // MAC list to be sent at next transmission
  volatile uint8_t                    LinkCheck_Ppm_SNR;                                        // MAC: LinkCheckAns
  volatile uint8_t                    LinkCheck_GW_cnt;                                         // MAC: LinkCheckAns
  volatile uint8_t                    LinkADR_ChannelMask_OK;                                   // Last channel mask setting whether valid
  volatile uint8_t                    DutyCycle_MaxDutyCycle;                                   // MAC: DutyCycleReq
  volatile uint8_t                    TxParamSetup_MaxEIRP_dBm;                                 // MAC: TxParamSetupReq
  volatile uint16_t                   TxParamSetup_UplinkDwellTime_ms;                          // MAC: TxParamSetupReq
  volatile uint16_t                   TxParamSetup_DownlinkDwellTime_ms;                        // MAC: TxParamSetupReq
  volatile uint8_t                    Channel_DataRateValid;                                    // MAC: NewChannelReq
  volatile uint8_t                    Channel_FrequencyValid;                                   // MAC: NewChannelReq, DlChannelReq
  volatile uint8_t                    Channel_FrequencyExists;                                  // MAC: DlChannelReq

  /* Join Server specific */
#ifdef LORAWAN_1V1
  uint8_t                             JoinEUI_LE_1V1[8];                                        // JOIN-REQUEST, REJOIN-REQUEST
  volatile uint8_t                    JoinNonce_LE_1V1[4];                                      // JOIN-ACCEPT
  volatile uint8_t                    ServerNonce_LE_1V1[3];
#endif

  volatile uint64_t                   BootTime_UTC_ms;                                          // DEVICE_TIME_ANS: UTC time of xTaskGetTickCount() == 0 (init of the Free-RTOS) in ms

  /* Current transmission */
  volatile uint32_t                   TsEndOfTx;                                                // Timestamp of last TX end
  volatile CurrentWindow_t            Current_RXTX_Window;                                      // RX/TX1, RX/TX2 or none
  volatile LoRaWANctxDir_t            Dir;                                                      // 0:up, 1:down
  volatile uint8_t                    FCtrl_ADR;
  volatile uint8_t                    FCtrl_ADRACKReq;
  volatile uint8_t                    FCtrl_ACK;
  volatile uint8_t                    FCtrl_ClassB;
  volatile uint8_t                    FCtrl_FPending;
  volatile uint8_t                    FPort_absent;                                             // 1: FPort field absent
  volatile uint8_t                    FPort;
  volatile float                      FrequencyMHz;                                             // This value is taken for the TRX when turned on
  volatile uint8_t                    SpreadingFactor;                                          // This value is taken for the TRX when turned on

  /* Last radio measurements */
  volatile uint32_t                   LastIqBalTimeUs;                                          // Last point of system time when procedure was done
  volatile uint32_t                   LastIqBalDurationUs;                                      // Duration of balancing procedure
  volatile int8_t                     LastIqBalTemp;                                            // Temperature degC when last I/Q balancing took place
  volatile int16_t                    LastRSSIDbm;
  volatile int16_t                    LastPacketStrengthDBm;
  volatile int8_t                     LastPacketSnrDb;
  volatile float                      LastFeiHz;
  volatile float                      LastFeiPpm;

} LoRaWANctx_t;


typedef struct LoRaWAN_TX_Message {

  /* Ready for transport section */
  volatile uint8_t                    msg_encoded_EncDone;
  volatile uint8_t                    msg_encoded_Len;
  volatile uint8_t                    msg_encoded_Buf[LoRaWAN_MsgLenMax];

  /* Prepare section */
#ifdef LORAWAN_1V1
  uint8_t                             msg_prep_FOpts_Encoded[16];
  //
#endif
  uint8_t                             msg_prep_FRMPayload_Len;
  uint8_t                             msg_prep_FRMPayload_Buf[LoRaWAN_FRMPayloadMax];
  uint8_t                             msg_prep_FRMPayload_Encoded[LoRaWAN_FRMPayloadMax];

} LoRaWAN_TX_Message_t;

typedef struct LoRaWAN_RX_Message {

  /* Received data section */
  volatile uint8_t                    msg_encoded_Len;
  volatile uint8_t                    msg_encoded_Buf[LoRaWAN_MsgLenMax];

  /* parted section */
  uint8_t                             msg_parted_MHDR;
  uint8_t                             msg_parted_MType;
  uint8_t                             msg_parted_Major;
  //
  uint8_t                             msg_parted_DevAddr[4];
  //
  uint8_t                             msg_parted_FCtrl;
  uint8_t                             msg_parted_FCtrl_ADR;
  uint8_t                             msg_parted_FCtrl_ACK;
  uint8_t                             msg_parted_FCtrl_FPending;
  uint8_t                             msg_parted_FCtrl_FOptsLen;
  //
  uint8_t                             msg_parted_FCntDwn[2];
  //
  uint8_t                             msg_parted_FOpts_Buf[16];
  //
  uint8_t                             msg_parted_FPort_absent;
  uint8_t                             msg_parted_FPort;
  //
  uint8_t                             msg_parted_FRMPayload_Len;
  uint8_t                             msg_parted_FRMPayload_Buf[LoRaWAN_FRMPayloadMax];
  uint8_t                             msg_parted_FRMPayload_Encoded[LoRaWAN_FRMPayloadMax];
  //
  uint8_t                             msg_parted_MIC_Buf[4];
  uint8_t                             msg_parted_MIC_Valid;

} LoRaWAN_RX_Message_t;



/* PAYLOAD data  definition */

/* IoT4BeesCtrl_App */
typedef struct {

  /* Configuration */
  uint16_t                            flags;

  /* Bridges: weight and/or temperature raw values */
  uint16_t                            ad_ch_raw[8];

  /* Voltage measurements */
  int16_t                             v_bat_100th_volt;
  int16_t                             v_aux_100th_volt[3];

  /* Weather entities */
  int16_t                             wx_temp_10th_degC;
  int8_t                              wx_rh;
  int8_t                              wx_baro_m950_Pa;

#ifdef GNSS_EXTRA
  /* GNSS position */
  float                               gnss_lat;
  float                               gnss_lon;
  float                               gnss_alt_m;
  float                               gnss_acc_m;

# ifdef GNSS_EXTRA_SPEED_COURSE
  float                               gnss_spd_kmh;
  float                               gnss_crs;
# endif
#endif

} IoT4BeesCtrlApp_up_t;

typedef struct {

  /* TBD */

} IoT4BeesCtrlApp_down_t;


#ifdef PAYLOAD_FORMAT_IOT4BEES

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
  decoded.speed       = 0.0;
  decoded.course      = 0.0;
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

      if ((decoded.mote_flags & 0x2000) !== 0) {
        if (bytes.length >= (idx + 3)) {
          // Course 9 bits
          decoded.course  = ((bytes[idx + 0] & 0xff) << 1) | ((bytes[idx + 1] & 0x80) >>  7);

          // Speed 15 bits
          decoded.speed   = (((bytes[idx + 1] & 0x7f) << 8) | ((bytes[idx + 2] & 0xff) << 0)) / 10.0;

          idx += 3;
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



/* Functions */

uint8_t LoRaWAN_DRtoSF(DataRates_t dr);

uint8_t LoRaWAN_calc_randomChannel(LoRaWANctx_t* ctx);
float LoRaWAN_calc_Channel_to_MHz(LoRaWANctx_t* ctx, uint8_t channel, LoRaWANctxDir_t dir, uint8_t dflt);

void LoRaWAN_MAC_Queue_Push(const uint8_t* macAry, uint8_t cnt);
void LoRaWAN_MAC_Queue_Pull(uint8_t* macAry, uint8_t cnt);
void LoRaWAN_MAC_Queue_Reset(void);

void loRaWANLoraTaskInit(void);
void loRaWANLoraTaskLoop(void);


#ifdef __cplusplus
}
#endif

#endif /* LORAWAN_H_ */
