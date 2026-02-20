/*
 * def.h
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */

#ifndef SRC_COMMON_DEF_H_
#define SRC_COMMON_DEF_H_


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>



#define _DEF_CH1              0
#define _DEF_CH2              1
#define _DEF_CH3              2
#define _DEF_CH4              3

#define _DEF_UART1            0
#define _DEF_UART2            1
#define _DEF_UART3            2
#define _DEF_UART4            3

#define _DEF_LOW              0
#define _DEF_HIGH             1



#define constrain(amt,low,high) ((amt)<=(low)?(low):((amt)>(high)?(high):(amt)))

#ifndef cmax
#define cmax(a,b) (((a) > (b)) ? (a) : (b))
#define cmin(a,b) (((a) < (b)) ? (a) : (b))
#endif


#ifndef cmap
#define cmap(value, in_min, in_max, out_min, out_max) ((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)
#endif


#endif /* SRC_COMMON_DEF_H_ */
