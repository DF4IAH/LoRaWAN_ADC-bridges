/*
 * device_portexp.h
 *
 *  Created on: 04.05.2019
 *      Author: DF4IAH
 */

#ifndef INC_DEVICE_PORTEXP_H_
#define INC_DEVICE_PORTEXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PORTEXP_A                                             0U

#define PORTEXP_A_BIT_OUT_BNO_NRST                            0U
#define PORTEXP_A_BIT_OUT_SPI3_BNO_SEL                        1U
#define PORTEXP_A_BIT_OUT_BNO_NBOOT                           2U
#define PORTEXP_A_BIT_IN_BNO_NINT                             3U
#define PORTEXP_A_BIT_Z_P0_4                                  4U
#define PORTEXP_A_BIT_Z_P0_5                                  5U
#define PORTEXP_A_BIT_Z_P0_6                                  6U
#define PORTEXP_A_BIT_Z_P0_7                                  7U
#define PORTEXP_A_BIT_OUT_SX_NRESET                           8U
#define PORTEXP_A_BIT_OUT_SPI3_SX_SEL                         9U
#define PORTEXP_A_BIT_IN_SX_BUSY                              10U
#define PORTEXP_A_BIT_IN_SX_DIO1_TXRXDONE                     11U
#define PORTEXP_A_BIT_IN_SX_DIO2_TXRXSW                       12U
#define PORTEXP_A_BIT_IN_SX_DIO3_TIMEOUT                      13U
#define PORTEXP_A_BIT_Z_EXPCON_P1_6                           14U
#define PORTEXP_A_BIT_Z_EXPCON_P1_7                           15U


#define PORTEXP_B                                             1U

#define PORTEXP_B_BIT_Z_DC_P0_0                               0U
#define PORTEXP_B_BIT_OUT_ESP_RST                             1U
#define PORTEXP_B_BIT_OUT_ESP_CHIP_EN                         2U
#define PORTEXP_B_BIT_Z_ESP_GPIO16                            3U
#define PORTEXP_B_BIT_Z_ESP_GPIO14                            4U
#define PORTEXP_B_BIT_Z_ESP_GPIO12                            5U
#define PORTEXP_B_BIT_OUT_ESP_CS0                             6U
#define PORTEXP_B_BIT_Z_ESP_MISO                              7U
#define PORTEXP_B_BIT_Z_ESP_GPIO9                             8U
#define PORTEXP_B_BIT_Z_ESP_GPIO10                            9U
#define PORTEXP_B_BIT_Z_ESP_MOSI                              10U
#define PORTEXP_B_BIT_Z_ESP_SCLK                              11U
#define PORTEXP_B_BIT_OUT_ESP_GPIO2                           12U
#define PORTEXP_B_BIT_Z_ESP_GPIO0                             13U
#define PORTEXP_B_BIT_OUT_ESP_GPIO4                           14U
#define PORTEXP_B_BIT_Z_ESP_GPIO5                             15U


typedef enum PullMode {

  PupdNone    = 0,
  PupdPU,
  PupdPD

} PullMode_t;


void    portexp_Init(void);
void    portexp_setDirs(uint8_t isPortExp_B, uint16_t dirs);
void    portexp_setPullMode(uint8_t isPortExp_B, uint8_t bit, PullMode_t pullMode);
void    portexp_setIrq(uint8_t isPortExp_B, uint8_t bit, uint8_t irqEnable);
uint8_t portexp_read(uint8_t isPortExp_B, uint8_t bit);
void    portexp_write(uint8_t isPortExp_B, uint8_t bit, uint8_t doSet);

void    portexp_EXTI_Callback(uint8_t isPortExp_B);

#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_PORTEXP_H_ */
