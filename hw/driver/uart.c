/*
 * uart.c
 *
 *  Created on: 2023. 2. 26.
 *      Author: baram
 */


#include "uart.h"
#include "xuartps.h"


#ifdef _USE_HW_UART





typedef struct
{
  bool is_open;

  uint32_t      baud;
  XUartPs       uart_ps;
} uart_tbl_t;


static uart_tbl_t uart_tbl[UART_MAX_CH];





bool uartInit(void)
{
  XUartPs_Config *config;
  int status;

  config = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
  if (NULL == config)
  {
    return false;
  }


  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;

    status = XUartPs_CfgInitialize(&uart_tbl[i].uart_ps, config, config->BaseAddress);
    if (status != XST_SUCCESS)
    {
      return false;
    }
  }

  return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;


  if (uartIsOpen(ch) == true && uartGetBaud(ch) == baud)
  {
    return true;
  }

  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].baud = baud;

      XUartPs_SetOperMode(&uart_tbl[ch].uart_ps, XUARTPS_OPER_MODE_NORMAL);
      XUartPs_SetBaudRate(&uart_tbl[ch].uart_ps, baud);

      uart_tbl[ch].is_open = true;
      ret = true;
      break;
  }

  return ret;
}

bool uartIsOpen(uint8_t ch)
{
  return uart_tbl[ch].is_open;
}

bool uartClose(uint8_t ch)
{
  uart_tbl[ch].is_open = false;
  return true;
}

uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;


  if (uart_tbl[ch].is_open != true) return 0;

  switch(ch)
  {
    case _DEF_UART1:
      ret = XUartPs_IsReceiveData(uart_tbl[ch].uart_ps.Config.BaseAddress);
      break;
  }

  return ret;
}

bool uartFlush(uint8_t ch)
{
  uint32_t pre_time;

  pre_time = millis();
  while(uartAvailable(ch))
  {
    if (millis()-pre_time >= 10)
    {
      break;
    }
    uartRead(ch);
  }

  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;

  switch(ch)
  {
    case _DEF_UART1:
      XUartPs_Recv(&uart_tbl[ch].uart_ps, &ret, 1);
      break;
  }

  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;

  if (uart_tbl[ch].is_open != true) return 0;


  switch(ch)
  {
    case _DEF_UART1:
      ret = XUartPs_Send(&uart_tbl[ch].uart_ps, p_data, length);
      while (XUartPs_IsSending(&uart_tbl[ch].uart_ps)) {};
      break;
  }

  return ret;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args);


  return ret;
}

uint32_t uartGetBaud(uint8_t ch)
{
  return uart_tbl[ch].baud;
}


#endif
