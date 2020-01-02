/*
 * device_bno085.h
 *
 *  Created on: 10.07.2019
 *      Author: DF4IAH
 */

#ifndef INC_DEVICE_BNO085_H_
#define INC_DEVICE_BNO085_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BNO085_EXTRA

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>


uint32_t bno085WaitUntilInt(uint32_t stopTimeAbs);
void     bno085WaitUntilReady(void);

void     bno085Init(void);


#endif  /* BNO085_EXTRA */

#ifdef __cplusplus
}
#endif

#endif /* INC_DEVICE_BNO085_H_ */
