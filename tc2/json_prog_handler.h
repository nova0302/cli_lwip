/* json_prog_handler.h --- 
 * 
 * Filename: json_prog_handler.h
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Mar  5 20:18:28 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Fri Mar  6 15:14:38 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 1
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
#ifndef JSON_PROG_HANDLER_H
#define JSON_PROG_HANDLER_H

#include <stddef.h>
#include <stdint.h>

/* Expect exactly this many bytes for pgmData */
#ifndef PROG_PAYLOAD_SIZE
#define PROG_PAYLOAD_SIZE 256
#endif

/* Application hook: program a 256-byte block
 * All values are passed back as strings (your preferred contract).
 * Return 0 on success; non-zero on error.
 */
int do_program_ex(const char *nxAddr_str,
                  const char *nVcg_str,
                  const char *nNumRepeat_str,
                  const uint8_t *data,
                  size_t len);

/* Optional legacy hook used by older client schema (can leave unimplemented) */
int do_program_legacy(const char *addr_str,
                      const uint8_t *data,
                      size_t len);

/* Process one newline-delimited JSON command from 'line'.
 * Writes a JSON reply into tx_buf (newline-terminated) and returns bytes written.
 * Never returns negative; on parse/validation errors, returns a JSON {"ok":false,...}.
 */
size_t process_json_line(const char *line, char *tx_buf, size_t tx_cap);

#endif /* JSON_PROG_HANDLER_H */


/* json_prog_handler.h ends here */
