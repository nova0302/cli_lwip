/*
 * button.c
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */


#include "button.h"
#include "cli.h"
#include "xgpiops.h"


#ifdef _USE_HW_BUTTON

typedef struct
{
  uint32_t      pin;
  uint8_t       on_state;
  uint8_t       off_state;
} button_tbl_t;




button_tbl_t button_port_tbl[BUTTON_MAX_CH] =
{
  {0, _DEF_LOW, _DEF_HIGH}, // 0. BTN_PS_0
};

static const char *button_name[BUTTON_MAX_CH] =
{
  "_BTN_PS_0",
};

typedef struct
{
  bool        pressed;
  bool        pressed_event;
  uint16_t    pressed_cnt;
  uint32_t    pressed_start_time;
  uint32_t    pressed_end_time;

  bool        released;
  bool        released_event;
  uint32_t    released_start_time;
  uint32_t    released_end_time;

  bool        repeat_update;
  uint32_t    repeat_cnt;
  uint32_t    repeat_time_detect;
  uint32_t    repeat_time_delay;
  uint32_t    repeat_time;
} button_t;


#ifdef _USE_HW_CLI
static void cliButton(cli_args_t *args);
#endif

static bool is_enable = true;
static button_t button_tbl[BUTTON_MAX_CH];
static XGpioPs gpio;





bool buttonInit(void)
{
  uint32_t i;
  XGpioPs_Config *config_ptr;
  int status;


  config_ptr = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
  status = XGpioPs_CfgInitialize(&gpio, config_ptr, config_ptr->BaseAddr);
  if (status != XST_SUCCESS)
  {
    return false;
  }

  for (i=0; i<BUTTON_MAX_CH; i++)
  {
    XGpioPs_SetDirectionPin(&gpio, button_port_tbl[i].pin, 0);
  }

  for (i=0; i<BUTTON_MAX_CH; i++)
  {
    button_tbl[i].pressed_cnt    = 0;
    button_tbl[i].pressed        = 0;
    button_tbl[i].released       = 0;
    button_tbl[i].released_event = 0;

    button_tbl[i].repeat_cnt     = 0;
    button_tbl[i].repeat_time_detect = 60;
    button_tbl[i].repeat_time_delay  = 250;
    button_tbl[i].repeat_time        = 200;

    button_tbl[i].repeat_update = false;
  }



#ifdef _USE_HW_CLI
  cliAdd("button", cliButton);
#endif

  return true;
}


const char *buttonGetName(uint8_t ch)
{
  ch = constrain(ch, 0, BUTTON_MAX_CH-1);

  return button_name[ch];
}

bool buttonGetPin(uint8_t ch)
{
  if (ch >= BUTTON_MAX_CH)
  {
    return false;
  }

  if (XGpioPs_ReadPin(&gpio, button_port_tbl[ch].pin) == button_port_tbl[ch].on_state)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool buttonGetPressed(uint8_t ch)
{
  if (ch >= BUTTON_MAX_CH || is_enable == false)
  {
    return false;
  }

  return buttonGetPin(ch);
}

uint8_t  buttonGetPressedCount(void)
{
  uint32_t i;
  uint8_t ret = 0;

  for (i=0; i<BUTTON_MAX_CH; i++)
  {
    if (buttonGetPressed(i) == true)
    {
      ret++;
    }
  }

  return ret;
}



#ifdef _USE_HW_CLI
void cliButton(cli_args_t *args)
{
  bool ret = false;
  uint32_t i;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (i=0; i<BUTTON_MAX_CH; i++)
    {
      cliPrintf("%-12s pin %d\n", buttonGetName(i), button_port_tbl[i].pin);
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "show"))
  {
    while(cliKeepLoop())
    {
      for (i=0; i<BUTTON_MAX_CH; i++)
      {
        cliPrintf("%d", buttonGetPressed(i));
      }
      delay(50);
      cliPrintf("\r");
    }
    ret = true;
  }



  if (ret == false)
  {
    cliPrintf("button info\n");
    cliPrintf("button show\n");
  }
}
#endif

#endif
