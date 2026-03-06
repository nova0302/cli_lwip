/* tc2_util.c --- 
 * 
 * Filename: tc2_util.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Feb 12 09:08:00 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Thu Feb 12 09:37:51 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 4
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
#include "tc2_util.h"

uint64_t bitInterleave(uint64_t a, uint64_t b){

  uint64_t c = 0;

  for(int i=0; i<32; i++){
	c |= ((a >> i) & 1) << (2*i    );
	c |= ((b >> i) & 1) << (2*i + 1);
  }

  return c;
}

void bitInterleaveWord(StVfyo_t *pSrc, StVfyo_t *pDes){

  for(int j=0; j<32; j++){
	
	uint64_t a = pSrc->nVfyo[j+ 0];
	uint64_t b = pSrc->nVfyo[j+32];

	uint64_t c = bitInterleave(a, b);
	pDes->nVfyo[2*j]    = (uint32_t)c;
	pDes->nVfyo[2*j +1] = (uint32_t)(c >> 32);
  }
}



/* tc2_util.c ends here */
