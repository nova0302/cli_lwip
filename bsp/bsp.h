/*
 * bsp.h
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */

#ifndef SRC_BSP_BSP_H_
#define SRC_BSP_BSP_H_

#include "def.h"

#include "xparameters.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xtime_l.h"
#include "xstatus.h"


#define STDOUT_IS_PS7_UART
#define UART_DEVICE_ID 0



void bspInit(void);

void delay(uint32_t time_ms);
uint32_t millis(void);


#endif /* SRC_BSP_BSP_H_ */
