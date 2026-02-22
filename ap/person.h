/* person.h --- 
 * 
 * Filename: person.h
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 금  2월 20 16:51:36 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 금  2월 20 17:31:06 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 11
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

#include <stdint.h>
#include <stdbool.h>

#define NULL (0)


typedef struct Person{
  char pcName[16];
  uint32_t nAge;
}StPerson;

//bool getPerson(StPerson **ppStPerson);
StPerson* getPerson(void);
void setPerson(StPerson *pStPerson, char *pcName, uint32_t nAge);

/* person.h ends here */
