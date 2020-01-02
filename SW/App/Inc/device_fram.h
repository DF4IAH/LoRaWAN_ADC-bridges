/*
 * device_fram.h
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */

#ifndef INC_DEVICE_FRAM_H_
#define INC_DEVICE_FRAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "task_LoRaWAN.h"


typedef enum FRAM_MEM {

  /* BLOCK 1F */
  FRAM_MEM_1F00_CRC32               = 0x1f00U,                                                  //  4 bytes as word (LE)

  FRAM_MEM_LORAWAN_BITFIELD         = 0x1f08U,                                                  //  4 bytes as bitfield word (LE)

  FRAM_MEM_LORAWAN_RX_DELAY         = 0x1f0cU,                                                  //  1 byte
  FRAM_MEM_LORAWAN_DR_RXTX2         = 0x1f0dU,                                                  //  1 byte
  FRAM_MEM_LORAWAN_DR_RX1_DROFS     = 0x1f0eU,                                                  //  1 byte
  FRAM_MEM_LORAWAN_ADR_ENABLED      = 0x1f0fU,                                                  //  1 byte

  FRAM_MEM_LORAWAN_CF_LIST          = 0x1f10U,                                                  // 16 bytes

  FRAM_MEM_LORAWAN_APP_JOIN_EUI     = 0x1f20U,                                                  //  8 bytes as long word (LE)
  FRAM_MEM_LORAWAN_DEVEUI           = 0x1f28U,                                                  //  8 bytes as long word (LE)
  FRAM_MEM_LORAWAN_DEVADDR          = 0x1f30U,                                                  //  4 bytes as word (LE)
  FRAM_MEM_LORAWAN_DEVNONCE         = 0x1f34U,                                                  //  4 bytes as word (LE)

  FRAM_MEM_LORAWAN_NWKKEY           = 0x1f40U,                                                  // 16 bytes as octet array
  FRAM_MEM_LORAWAN_APPKEY           = 0x1f50U,                                                  // 16 bytes as octet array
  FRAM_MEM_LORAWAN_NWKSKEY          = 0x1f60U,                                                  // 16 bytes as octet array
  FRAM_MEM_LORAWAN_APPSKEY          = 0x1f70U,                                                  // 16 bytes as octet array

  FRAM_MEM_LORAWAN_NFCNT_UP         = 0x1f80U,                                                  //  4 bytes as word (LE)
  FRAM_MEM_LORAWAN_NFCNT_DN         = 0x1f84U,                                                  //  4 bytes as word (LE)
  FRAM_MEM_LORAWAN_AFCNT_UP         = 0x1f88U,                                                  //  4 bytes as word (LE)
  FRAM_MEM_LORAWAN_AFCNT_DN         = 0x1f8cU,                                                  //  4 bytes as word (LE)

  FRAM_MEM_LORAWAN_CH_DR_TX_MIN     = 0x1f90U,                                                  // 16 bytes as byte[16]
  FRAM_MEM_LORAWAN_CH_DR_TX_MAX     = 0x1fa0U,                                                  // 16 bytes as byte[16]
  FRAM_MEM_LORAWAN_CH_DR_TX_SEL     = 0x1fb0U,                                                  // 16 bytes as byte[16]

  FRAM_MEM_LORAWAN_ADR_PWRRED       = 0x1fc0U,                                                  //  1 byte
  FRAM_MEM_LORAWAN_ADR_DR_TX1       = 0x1fc1U,                                                  //  1 byte
  FRAM_MEM_LORAWAN_ADR_CH_MSK       = 0x1fc2U,                                                  //  2 bytes
  FRAM_MEM_LORAWAN_ADR_CH_MSK_CNTL  = 0x1fc4U,                                                  //  1 byte
  FRAM_MEM_LORAWAN_ADR_NB_TRANS     = 0x1fc5U,                                                  //  1 byte

  FRAM_MEM_LORAWAN_MHDR_MTYPE       = 0x1fc6U,                                                  //  1 byte
  FRAM_MEM_LORAWAN_MHDR_MAJOR       = 0x1fc7U,                                                  //  1 byte
  FRAM_MEM_LORAWAN_APP_NONCE        = 0x1fc8U,                                                  //  3 bytes
  FRAM_MEM_LORAWAN_NET_ID           = 0x1fcbU,                                                  //  3 bytes

  FRAM_MEM_SX_XTAL_DRIFT            = 0x1fd0U,                                                  //  4 bytes as float (LE)
  FRAM_MEM_SX_XTA_C                 = 0x1fd4U,                                                  //  1 byte
  FRAM_MEM_SX_XTB_C                 = 0x1fd5U,                                                  //  1 byte

} FRAM_MEM_t;

#define FRAM_MEM_1F00_BORDER        0x1f00UL
#define FRAM_MEM_1F00_CRC_BEGIN     0x1f08UL
#define FRAM_MEM_1F00_CRC_END       0x1fffUL


typedef struct Fram1F_Template {

  volatile uint32_t                 crc32;                                                      // 0x1f00
  uint32_t                          _free_04;                                                   // 0x1f04

  /* BEGIN CRC SECURED DATA */
  volatile Lora_EGW_BM_t            loraEventGroup;                                             // 0x1f08

  volatile uint8_t                  LoRaWAN_rxDelay;                                            // 0x1f0c

  volatile uint8_t                  LinkADR_DR_RXTX2;                                           // 0x1f0d
  volatile uint8_t                  LinkADR_DR_RX1_DRofs;                                       // 0x1f0e
  volatile uint8_t                  LinkADR_ADR_enabled;                                        // 0x1f0f

  volatile uint8_t                  LoRaWAN_CfList[16];                                         // 0x1f10     Ch3 Ch3 Ch3  Ch4 Ch4 Ch4  Ch5 Ch5 Ch5  Ch6 Ch6 Ch6  Ch7 Ch7 Ch7  RFU

  volatile uint8_t                  LoRaWAN_APP_JOIN_EUI[8];                                    // 0x1f20
  volatile uint8_t                  LoRaWAN_DEV_EUI[8];                                         // 0x1f28
  volatile uint8_t                  LoRaWAN_DevAddr[4];                                         // 0x1f30
  volatile uint8_t                  LoRaWAN_DevNonce[2];                                        // 0x1f34
  volatile uint8_t                  _free_36[0x0a];                                             // 0x1f36

  volatile uint8_t                  LoRaWAN_NWKKEY[16];                                         // 0x1f40
  volatile uint8_t                  LoRaWAN_APPKEY[16];                                         // 0x1f50
  volatile uint8_t                  LoRaWAN_NWKSKEY[16];                                        // 0x1f60
  volatile uint8_t                  LoRaWAN_APPSKEY[16];                                        // 0x1f70

  volatile uint32_t                 LoRaWAN_NFCNT_Up;                                           // 0x1f80
  volatile uint32_t                 LoRaWAN_NFCNT_Dn;                                           // 0x1f84
  volatile uint32_t                 LoRaWAN_AFCNT_Up;                                           // 0x1f88
  volatile uint32_t                 LoRaWAN_AFCNT_Dn;                                           // 0x1f8c

  volatile uint8_t                  LoRaWAN_Ch_DR_TX_min[16];                                   // 0x1f90
  volatile uint8_t                  LoRaWAN_Ch_DR_TX_max[16];                                   // 0x1fa0
  volatile uint8_t                  LoRaWAN_Ch_DR_TX_sel[16];                                   // 0x1fb0

  volatile uint8_t                  LinkADR_PwrRed;                                             // 0x1fc0
  volatile uint8_t                  LinkADR_DR_TX1;                                             // 0x1fc1
  volatile uint16_t                 LinkADR_ChanMsk;                                            // 0x1fc2
  volatile uint8_t                  LinkADR_ChanMsk_Cntl;                                       // 0x1fc4
  volatile uint8_t                  LinkADR_NbTrans;                                            // 0x1fc5

  volatile uint8_t                  LoRaWAN_MHDR_MType;                                         // 0x1fc6
  volatile uint8_t                  LoRaWAN_MHDR_Major;                                         // 0x1fc7
  volatile uint8_t                  LoRaWAN_AppNonce[3];                                        // 0x1fc8
  volatile uint8_t                  LoRaWAN_NetID[3];                                           // 0x1fcb
  volatile uint8_t                  _free_ce[0x02];                                             // 0x1fce

  volatile float                    SX_XTAL_Drift;                                              // 0x1fd0
  volatile uint8_t                  SX_XTA_Cap;                                                 // 0x1fd4
  volatile uint8_t                  SX_XTB_Cap;                                                 // 0x1fd5

  volatile uint8_t                  _free_d6[0x0a];                                             // 0x1fd6

  uint8_t                           _free_e0[0x20];                                             // 0x1fe0

  /* END CRC SECURED DATA */

} Fram1F_Template_t;


uint8_t   fram_check1Fcrc(uint8_t isLoadIntoShadow);
void      fram_updateCrc(void);

uint32_t  fram_readWord(uint32_t addr);
void      fram_readBuf(volatile void* buf, uint32_t addr, uint16_t len);
uint8_t   fram_read_nonSecured(uint32_t addr);

void      fram_writeWord(uint32_t addr, uint32_t data);
void      fram_writeBuf(volatile void* buf, uint32_t addr, uint16_t len);
void      fram_write_nonSecured(uint32_t addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_FRAM_H_ */
