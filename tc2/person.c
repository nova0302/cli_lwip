/* person.c --- 
 * 
 * Filename: person.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 금  2월 20 16:52:19 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 금  2월 20 17:32:03 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 7
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
#include "person.h"

static StPerson stPerson={"Peter", 23};

//bool getPerson(StPerson **ppStPerson){
//
//  bool ret=false; 
//  if(&stPerson){
//    *ppStPerson = &stPerson;
//    ret = true;
//  }
//  return ret;
//}

StPerson* getPerson(void){
  return &stPerson;
}

//void setPerson(StPerson *pStPerson, char *pName, uint32_t nAge);
void setPerson(StPerson *pStPerson, char *pcName, uint32_t nAge){
  strncpy(pStPerson->pcName, pcName, strlen(pcName));
  pStPerson->nAge = nAge;
}

/* person.c ends here */
