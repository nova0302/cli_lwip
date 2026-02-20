/*
 * led.h
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */

#ifndef SRC_COMMON_HW_INCLUDE_LED_H_
#define SRC_COMMON_HW_INCLUDE_LED_H_


#include "hw_def.h"


#define LED_MAX_CH      HW_LED_MAX_CH


bool ledInit(void);
void ledOn(uint8_t ch);
void ledOff(uint8_t ch);
void ledToggle(uint8_t ch);


#endif /* SRC_COMMON_HW_INCLUDE_LED_H_ */
