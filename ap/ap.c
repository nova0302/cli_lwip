/*
 * ap.c
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */

#include "ap.h"
#include "tc2_lwip.h"

void apInit(void) {
  cliOpen(_DEF_UART1, 115200);
  tc2_lwip_init();
}

void apMain(void) {
  uint32_t pre_time;

  while (1) {

    tc2_lwip_main();

    cliMain();

    if (millis() - pre_time >= 500) {
      pre_time = millis();
//      ledToggle(_DEF_CH1);
    }


  }
}
