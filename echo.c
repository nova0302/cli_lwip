/*
 * Copyright (C) 2009 - 2018 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/tcp.h"
#if defined (__arm__) || defined (__aarch64__)
#include "xil_printf.h"
#endif

static char cMsge[] = "Hello World";
struct tcp_pcb *global_pcb = NULL;

err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

int transfer_data() {
  return 0;
}

void print_app_header()
{
  xil_printf("\n\r\n\r-----lwIP TCP echo server ------\n\r");
  xil_printf("TCP packets sent to port 6001 will be echoed back\n\r");
}

#define ECHO_SERVER
#ifdef ECHO_SERVER
err_t recv_callback(void *arg, struct tcp_pcb *tpcb,
                    struct pbuf *p, err_t err)
{
  /* do not read the packet if we are not in ESTABLISHED state */
  if (!p) {
    tcp_close(tpcb);
    tcp_recv(tpcb, NULL);
    return ERR_OK;
  }

  /* indicate that the packet has been received */
  tcp_recved(tpcb, p->len);
  xil_printf("Msg from host: %s\n\r", (char*)p->payload);

  /* echo back the payload */
  /* in this case, we assume that the payload is < TCP_SND_BUF */
  if (tcp_sndbuf(tpcb) > p->len) {
    err = tcp_write(tpcb, (void*)&cMsge[0], strlen(cMsge), 0);
    //err = tcp_write(tpcb, p->payload, p->len, 0);
//		err = tcp_write(tpcb, p->payload, p->len, 1);
  } else
    xil_printf("no space in tcp_sndbuf\n\r");

  /* free the received pbuf */
  pbuf_free(p);

  return ERR_OK;
}
#endif

err_t accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  static int connection = 1;
  global_pcb = newpcb;

  /* set the receive callback for this connection */
  tcp_recv(newpcb, recv_callback);

  /* just use an integer number indicating the connection id as the
     callback argument */
  tcp_arg(newpcb, (void*)(UINTPTR)connection);

  /* increment for subsequent accepted connections */
  connection++;


  return ERR_OK;
}

int start_application()
{
  struct tcp_pcb *pcb;
  err_t err;
  unsigned port = 7;

  /* create new TCP PCB structure */
  pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
  if (!pcb) {
    xil_printf("Error creating PCB. Out of Memory\n\r");
    return -1;
  }

  /* bind to specified @port */
  err = tcp_bind(pcb, IP_ANY_TYPE, port);
  if (err != ERR_OK) {
    xil_printf("Unable to bind to port %d: err = %d\n\r", port, err);
    return -2;
  }

  /* we do not need any arguments to callback functions */
  tcp_arg(pcb, NULL);

  /* listen for connections */
  pcb = tcp_listen(pcb);
  if (!pcb) {
    xil_printf("Out of memory while tcp_listen\n\r");
    return -3;
  }

  /* specify callback to use for incoming connections */
  tcp_accept(pcb, accept_callback);

  xil_printf("TCP echo server started @ port %d\n\r", port);

  return 0;
}


//
//#include "lwip/tcp.h"
//#include "xil_printf.h"
//
//// Define Server IP and Port
//#define SERVER_IP "192.168.0.23"
//#define SERVER_PORT 65432 // Example echo port
//
//static struct tcp_pcb *client_pcb;
//#ifdef ECHO_SERVER
//#else
//// Callback when data is received from the server
//err_t recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
//  if (!p) {
//    tcp_close(tpcb); // Server closed connection
//    return ERR_OK;
//  }
//  xil_printf("Received: %s\n", (char *)p->payload);
//  tcp_recved(tpcb, p->len); // Acknowledge receipt
//  pbuf_free(p);
//  return ERR_OK;
//}
//#endif
//
//// Callback when the connection is successfully established
//err_t connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
//  xil_printf("Connected to server!\n");
//
//  // Send data to server
//  char *msg = "Hello from Zynq!";
//  tcp_write(tpcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
//  tcp_output(tpcb); // Force push to wire
//
//  // Set receive callback for this connection
//  tcp_recv(tpcb, recv_callback);
//  return ERR_OK;
//}
//
//void start_tcp_client() {
//  ip_addr_t server_addr;
//  ipaddr_aton(SERVER_IP, &server_addr);
//
//  // 1. Create a new TCP Control Block (PCB)
//  client_pcb = tcp_new();
//  if (!client_pcb) {
//    xil_printf("Error creating PCB\n");
//    return;
//  }
//
//  // 2. Connect to the server
//  err_t err = tcp_connect(client_pcb, &server_addr, SERVER_PORT, connected_callback);
//  if (err != ERR_OK) {
//    xil_printf("Connect error: %d\n", err);
//  }
//}

