/* tc2.h --- 
 * 
 * Filename: tc2.h
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 일  1월 25 21:58:24 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 금  2월 20 17:01:03 2026 (+0900)
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
#ifndef TC2_H
#define TC2_H

#include <stdint.h>
#include <stdbool.h>

#define NUM_BIT_LINE (2048)

typedef struct {
  uint8_t x[5][NUM_BIT_LINE];
  double X[NUM_BIT_LINE];
}levels_t;

void tc2 (cli_args_t *args);

#endif /* TC2_H */

/* tc2.h ends here */
