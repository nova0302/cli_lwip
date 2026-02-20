/*
 * led.c
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */

#include "xparameters.h"
#include "led.h"

//#include "xgpiops.h"
#include "xgpio.h"


typedef struct
{
  uint32_t pin;
  uint32_t on_state;
  uint32_t off_state;
} led_tbl_t;


static const led_tbl_t led_tbl[LED_MAX_CH] =
    {
//        {9, 1, 0},
		 {1, 1, 0},
    };

static bool is_init = false;
//static XGpioPs gpio;

XGpio Gpio; /* The Instance of the GPIO Driver */

bool ledInit(void)
{
	int Status;
//  XGpioPs_Config *config_ptr;
//  int status;
//
//
//  config_ptr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
//  status = XGpioPs_CfgInitialize(&gpio, config_ptr, config_ptr->BaseAddr);
//  if (status != XST_SUCCESS)
//  {
//    return false;
//  }

	/* Initialize the GPIO driver */
//	Status = XGpio_Initialize(&Gpio, XPAR_AXI_GPIO_0_DEVICE_ID);
	Status = XGpio_Initialize(&Gpio, 0);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}

  for (int i=0; i<LED_MAX_CH; i++)
  {
	  XGpio_SetDataDirection(&Gpio, led_tbl[i].pin, 1);
//    XGpio_SetOutputEnablePin(&Gpio, led_tbl[i].pin, 1);
//    XGpio_WritePin(&Gpio, led_tbl[i].pin, led_tbl[i].off_state);
  }

  is_init = true;

  return true;
}

void ledOn(uint8_t ch)
{
  if (ch >= LED_MAX_CH)
    return;

//  XGpioPs_WritePin(&gpio, led_tbl[ch].pin, led_tbl[ch].on_state);
  XGpio_DiscreteWrite(&Gpio, led_tbl[ch].pin, led_tbl[ch].on_state);
}

void ledOff(uint8_t ch)
{
  if (ch >= LED_MAX_CH)
    return;

//  XGpioPs_WritePin(&gpio, led_tbl[ch].pin, led_tbl[ch].off_state);
  XGpio_DiscreteClear(&Gpio, led_tbl[ch].pin, led_tbl[ch].on_state);
}

void ledToggle(uint8_t ch)
{
  if (ch >= LED_MAX_CH)
    return;

//  XGpioPs_WritePin(&gpio, led_tbl[ch].pin, !XGpioPs_ReadPin(&gpio, led_tbl[ch].pin));
}
