/* tc2.c --- 
 * 
 * Filename: tc2.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 토  1월 24 23:18:01 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 토  2월 28 20:50:57 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 36
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
#include <string.h>

#include "lwip/tcp.h"

#include "cli.h"
#include "person.h"
#include "tc2.h"

void send_message(const char *msg);

extern struct tcp_pcb *global_pcb; 

void tc2 (cli_args_t *args){

  (void)args;
  static uint32_t nTxCnt = 0;

  StPerson *pStPerson = getPerson();
  //getPerson(&pStPerson);
  if(pStPerson){
    cliPrintf("Name: %s, Age: %d\r\n",
              pStPerson->pcName,
              pStPerson->nAge);
  }
  setPerson(pStPerson, "Danny", 21);

  if(pStPerson){
    cliPrintf("Name: %s, Age: %d\r\n",
              pStPerson->pcName,
              pStPerson->nAge);
  }
  char msg[16];
  sprintf(msg,"%d - HelloWorld\n", nTxCnt++);
  send_message(msg);

}

void send_message(const char *msg) {

  if (global_pcb == NULL) {
    xil_printf("no space in tcp_sndbuf\n\r");
    return;
  }

  if (tcp_sndbuf(global_pcb) > 0) {
    tcp_write(global_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
  }

//err_t err = tcp_write(global_pcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
//if (err == ERR_OK) {
//  tcp_output(global_pcb); // Flush immediately
//}
}

/* tc2.c ends here */
