/* tc2_lwip.c --- 
 * 
 * Filename: tc2_lwip.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 토  1월 31 16:22:24 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 일  2월 22 22:10:52 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 12
 * URL: 
 * Doc URL: 
 * Keywords: 
 * Compatibility: 
 * 
 */

/* Commentary: 
 * 
 * 
 * 
 */

/* Change Log:
 * 
 * 
 */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.
 */

/* Code: */

#include "netif/xadapter.h"

#include "platform.h"
#include "platform_config.h"
#if defined (__arm__) || defined(__aarch64__)
#include "xil_printf.h"
#endif

#include "lwip/tcp.h"
#include "xil_cache.h"
#include "tc2_lwip.h"

#define PLATFORM_EMAC_BASEADDR XPAR_XEMACPS_0_BASEADDR

/* defined by each RAW mode application */
void print_app_header();
int start_application();
int transfer_data();
void tcp_fasttmr(void);
void tcp_slowtmr(void);

/* missing declaration in lwIP */
//void lwip_init(void);

extern volatile int TcpFastTmrFlag;
extern volatile int TcpSlowTmrFlag;
static struct netif server_netif;
struct netif *echo_netif;

void print_ip(char *msg, ip_addr_t *ip) {
  print(msg);
  xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
             ip4_addr3(ip), ip4_addr4(ip));
}

void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw) {
  print_ip("Board IP: ", ip);
  print_ip("Netmask : ", mask);
  print_ip("Gateway : ", gw);
}

void tc2_lwip_init(void)
{
  ip_addr_t ipaddr, netmask, gw;

  /* the mac address of the board. this should be unique per board */
  unsigned char mac_ethernet_address[] =
    { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

  echo_netif = &server_netif;

  init_platform();

  /* initliaze IP addresses to be used */
  IP4_ADDR(&ipaddr,  192, 168,   0, 10);
  IP4_ADDR(&netmask, 255, 255, 255,  0);
  IP4_ADDR(&gw,      192, 168,   0,  1);

  print_app_header();

  lwip_init();

  /* Add network interface to the netif_list, and set it as default */
  if (!xemac_add(echo_netif, &ipaddr, &netmask,
                 &gw, mac_ethernet_address,
                 PLATFORM_EMAC_BASEADDR)) {
    xil_printf("Error adding N/W interface\n\r");
//    return -1;
  }

  netif_set_default(echo_netif);

  /* now enable interrupts */
  platform_enable_interrupts();

  /* specify that the network if is up */
  netif_set_up(echo_netif);

  print_ip_settings(&ipaddr, &netmask, &gw);

  /* start the application (web server, rxtest, txtest, etc..) */
  start_application();
  //start_tcp_client();


}

void tc2_lwip_main()
{
  if (TcpFastTmrFlag) {
    tcp_fasttmr();
    TcpFastTmrFlag = 0;
  }
  if (TcpSlowTmrFlag) {
    tcp_slowtmr();
    TcpSlowTmrFlag = 0;
  }
  xemacif_input(echo_netif);
  transfer_data();

}


/* tc2_lwip.c ends here */
