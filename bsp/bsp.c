/*
 * bsp.c
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */


#include "bsp.h"





void bspInit(void)
{

}

void delay(uint32_t time_ms)
{
  XTime tEnd, tCur;

  XTime_GetTime(&tCur);
  tEnd  = tCur + (((XTime)time_ms) * (COUNTS_PER_SECOND/1000));
  do
  {
    XTime_GetTime(&tCur);
  } while (tCur < tEnd);
}

uint32_t millis(void)
{
  XTime tCur;
  XTime tMillis;

  XTime_GetTime(&tCur);



  tMillis  = tCur / (COUNTS_PER_SECOND/1000);

  return (uint32_t)tMillis;
}
