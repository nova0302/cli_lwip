/* json_prog_handler.c --- 
 * 
 * Filename: json_prog_handler.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Mar  5 20:18:57 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Fri Mar  6 19:55:13 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 23
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "cJSON.h"
#include "json_prog_handler.h"
#include "tc2.h"

extern tc2_t g_Tc2;

/* --- tiny Base64 (MIT-0) --- */
int b64_encode(const void *src, size_t src_len, char *dst, size_t dst_cap, size_t *out_len);
int b64_decode(const char *src, size_t src_len, void *dst, size_t dst_cap, size_t *out_len);
size_t b64_max_decoded_size(size_t src_len);

/* --- defaults and limits for APGM/DPGM --- */
#define DEF_NXADDR     0
#define DEF_NVCG_HEX  "0xDEF" //4.32[V]
#define DEF_REPEAT_STR "1"
#define DEF_NVCG   0xDEFu

#define NXADDR_MIN 0u
#define NXADDR_MAX 1280u
#define NVCG_MIN   0x50Fu
#define NVCG_MAX   0xFFFu
#define REPEAT_MIN 1u
#define REPEAT_MAX 64u

/* --- reply helpers -------------------------------------------------------- */

static void write_json_error(const char *msg, char *out, size_t cap)
{
    int n = snprintf(out, cap, "{\"ok\":false,\"error\":\"%s\"}\n", msg ? msg : "error");
    (void)n;
}

static void write_json_ok_apgm(const char *cmd,
                               const char *nxAddr,
                               const char *nVcg,
                               const char *nNumRepeat,
                               int len,
                               char *out, size_t cap)
{
    int n = snprintf(out, cap,
        "{\"ok\":true,\"cmd\":\"%s\",\"nxAddr\":\"%s\",\"nVcg\":\"%s\",\"nNumRepeat\":\"%s\",\"len\":%d}\n",
        cmd ? cmd : "", nxAddr ? nxAddr : "", nVcg ? nVcg : "", nNumRepeat ? nNumRepeat : "", len);
    (void)n;
}

/* Trim ASCII spaces from both ends (for defensive parsing of strings). */
static void trim_ascii(const char **ps, const char **pe)
{
    const char *s = *ps, *e = *pe;
    while (s < e && isspace((unsigned char)*s)) s++;
    while (e > s && isspace((unsigned char)e[-1])) e--;
    *ps = s; *pe = e;
}

/* parse number from cJSON string or number, base 0 (accepts "0x...") */
static bool parse_u32(const cJSON *item, unsigned *out)
{
    if (!item || !out) return false;
    if (cJSON_IsNumber(item)) {
        double d = item->valuedouble;
        if (d < 0.0 || d > 4294967295.0) return false;
        *out = (unsigned)d;
        return true;
    }
    if (cJSON_IsString(item) && item->valuestring) {
        char *end = NULL;
        unsigned long v = strtoul(item->valuestring, &end, 0);
        if (end == item->valuestring || *end != '\0') return false;
        *out = (unsigned)v;
        return true;
    }
    return false;
}

/* return true if v is in [min,max] */
static bool in_range(unsigned v, unsigned minv, unsigned maxv) {
    return (v >= minv) && (v <= maxv);
}

/* Normalize nVcg into canonical "0x..." uppercase string. */
static void to_hex_upper(unsigned v, char *buf, size_t bufsz) {
    snprintf(buf, bufsz, "0x%X", v);
}

/* --- core handlers -------------------------------------------------------- */

/* New schema:
 * { "cmd":"apgm",
 *   "args":[ nxAddr?, nVcg?, nNumRepeat? ],  // 0..3 items, optional
 *   "pgmData":"<base64 of 256 bytes>",
 *   "payloadLen":"256"   // optional sanity
 * }
 *
 * Note: we keep all args as STRINGS for your contract.
 */
static size_t handle_apgm_like(const char *cmd,
                               const cJSON *args,
                               const cJSON *pgmData,
                               const cJSON *payloadLen,
                               char *tx_buf, size_t tx_cap)
{
    /* ------------- parse arguments with defaults ------------- */
    const char *nxAddr_str = NULL, *nVcg_str = NULL, *nNumRepeat_str = NULL;
    char nxAddr_buf[12] = {0};     /* decimal string */
    char nVcg_buf[12]   = {0};     /* "0x..." */
    char repeat_buf[12] = {0};     /* decimal string */

    unsigned nxAddr_val = DEF_NXADDR;
    unsigned nVcg_val   = DEF_NVCG;
    unsigned repeat_val = 1;

    /* Defaults (strings) */
    snprintf(nxAddr_buf, sizeof nxAddr_buf, "%u", DEF_NXADDR);
    strncpy(nVcg_buf, DEF_NVCG_HEX, sizeof nVcg_buf - 1);
    strncpy(repeat_buf, DEF_REPEAT_STR, sizeof repeat_buf - 1);

    nxAddr_str     = nxAddr_buf;
    nVcg_str       = nVcg_buf;
    nNumRepeat_str = repeat_buf;

    /* If args array exists, consume up to 3 items */
    if (args) {
        if (!cJSON_IsArray(args)) {
            write_json_error("args must be array", tx_buf, tx_cap);
            return strnlen(tx_buf, tx_cap);
        }
        int n = cJSON_GetArraySize(args);
        if (n > 3) n = 3;

        /* nxAddr */
        if (n >= 1) {
            if (!parse_u32(cJSON_GetArrayItem(args, 0), &nxAddr_val) ||
                !in_range(nxAddr_val, NXADDR_MIN, NXADDR_MAX)) {
                write_json_error("nxAddr out of range (0..1280)", tx_buf, tx_cap);
                return strnlen(tx_buf, tx_cap);
            }
            snprintf(nxAddr_buf, sizeof nxAddr_buf, "%u", nxAddr_val);
        }

        /* nVcg */
        if (n >= 2) {
            if (!parse_u32(cJSON_GetArrayItem(args, 1), &nVcg_val) ||
                !in_range(nVcg_val, NVCG_MIN, NVCG_MAX)) {
                write_json_error("nVcg out of range (0x50F..0xFFF)", tx_buf, tx_cap);
                return strnlen(tx_buf, tx_cap);
            }
            to_hex_upper(nVcg_val, nVcg_buf, sizeof nVcg_buf);
        } else {
            /* default nVcg = 0x50F */
            nVcg_val = DEF_NVCG;
        }

        /* nNumRepeat */
        if (n >= 3) {
            if (!parse_u32(cJSON_GetArrayItem(args, 2), &repeat_val) ||
                !in_range(repeat_val, REPEAT_MIN, REPEAT_MAX)) {
                write_json_error("nNumRepeat out of range (1..64)", tx_buf, tx_cap);
                return strnlen(tx_buf, tx_cap);
            }
            snprintf(repeat_buf, sizeof repeat_buf, "%u", repeat_val);
        } else {
            repeat_val = REPEAT_MIN;
        }
    } else {
        /* args omitted entirely -> use defaults already set above */
        nVcg_val   = DEF_NVCG;
        repeat_val = REPEAT_MIN;
    }

    /* ------------- payload (Base64) ------------- */
    if (!pgmData || !cJSON_IsString(pgmData) || !pgmData->valuestring) {
        write_json_error("missing pgmData (base64)", tx_buf, tx_cap);
        return strnlen(tx_buf, tx_cap);
    }

    const char *b64 = pgmData->valuestring;

    /* Optional sanity: payloadLen == PROG_PAYLOAD_SIZE */
    if (payloadLen) {
        unsigned plen = 0;
        if (!parse_u32(payloadLen, &plen) || plen != PROG_PAYLOAD_SIZE) {
            write_json_error("payloadLen mismatch", tx_buf, tx_cap);
            return strnlen(tx_buf, tx_cap);
        }
    }

    /* Decode */
    static uint8_t decode_buf[PROG_PAYLOAD_SIZE];  /* static on bare-metal (no heap) */
    size_t out_len = 0;
    int rc = b64_decode(b64, strlen(b64), decode_buf, sizeof(decode_buf), &out_len);
    if (rc != 0 || out_len != PROG_PAYLOAD_SIZE) {
	  write_json_error("base64 decode failed or wrong size", tx_buf, tx_cap);
	  return strnlen(tx_buf, tx_cap);
    }

    /* ------------- call application hook ------------- */
	memcpy(&g_Tc2.nPageData[0], &decode_buf[0], sizeof(decode_buf));

	cli_args_t cli_args;
	cli_args.argc = 3;
	cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

	if (cli_args.argv == NULL) {print("malloc failed!!\n"); return -1;}

	cli_args.argv[0] = nxAddr_buf;
	cli_args.argv[1] = nVcg_buf;
	cli_args.argv[2] = repeat_buf;

	on_ana_page_program(&cli_args);

	free(cli_args.argv); // cleanup

	//rc = do_program_ex(nxAddr_str, nVcg_str, nNumRepeat_str, decode_buf, out_len);
	if (rc != 0) {
	  write_json_error("programming failed", tx_buf, tx_cap);
	  return strnlen(tx_buf, tx_cap);
	}

	/* ------------- reply ------------- */
	write_json_ok_apgm(cmd, nxAddr_str, nVcg_str, nNumRepeat_str, (int)out_len, tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
}

/* Legacy schema compatibility:
 * {"cmd":"apgm","args":["0xa3000000","256"],"data_b64":"<...>"}
 */
static size_t handle_legacy_apgm(const char *cmd,
                                 const cJSON *args,
                                 const cJSON *data_b64,
                                 char *tx_buf, size_t tx_cap)
{
  if (!cJSON_IsArray(args) || cJSON_GetArraySize(args) < 2) {
	write_json_error("args must be [addr,len]", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }
  const cJSON *arg0 = cJSON_GetArrayItem(args, 0);
  const cJSON *arg1 = cJSON_GetArrayItem(args, 1);
  if (!cJSON_IsString(arg0) || !arg0->valuestring) {
	write_json_error("addr must be string", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }
  unsigned len = 0;
  if (!parse_u32(arg1, &len) || len != PROG_PAYLOAD_SIZE) {
	write_json_error("len must be 256 in legacy schema", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }
  if (!data_b64 || !cJSON_IsString(data_b64) || !data_b64->valuestring) {
	write_json_error("missing data_b64", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }

  static uint8_t buf[PROG_PAYLOAD_SIZE];
  size_t out_len = 0;
  int rc = b64_decode(data_b64->valuestring, strlen(data_b64->valuestring),
					  buf, sizeof(buf), &out_len);
  if (rc != 0 || out_len != PROG_PAYLOAD_SIZE) {
	write_json_error("base64 decode failed", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }

  /* If you keep legacy path, forward to a legacy app hook (or map to ex). */
  if (do_program_legacy) {
	//rc = do_program_legacy(arg0->valuestring, buf, out_len);
  } else {
	/* Map to new hook with defaults */
	//rc = do_program_ex("0", DEF_NVCG_HEX, "1", buf, out_len);
  }

  if (rc != 0) {
	write_json_error("programming failed", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }

  write_json_ok_apgm(cmd, "0", DEF_NVCG_HEX, "1", (int)out_len, tx_buf, tx_cap);
  return strnlen(tx_buf, tx_cap);
}

size_t process_json_line(const char *line, char *tx_buf, size_t tx_cap)
{
  if (!line || !tx_buf || tx_cap == 0) return 0;

  cJSON *root = cJSON_Parse(line);
  if (!root) {
	write_json_error("bad json", tx_buf, tx_cap);
	return strnlen(tx_buf, tx_cap);
  }

  const cJSON *cmd  = cJSON_GetObjectItemCaseSensitive(root, "cmd");
  const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");

  if (!cJSON_IsString(cmd) || !cmd->valuestring) {
	write_json_error("missing cmd", tx_buf, tx_cap);
	cJSON_Delete(root);
	return strnlen(tx_buf, tx_cap);
  }

  const char *cmd_str = cmd->valuestring;

  size_t wrote = 0;

  if (strcmp(cmd_str, "apgm") == 0 || strcmp(cmd_str, "dpgm") == 0) {
	/* Prefer new schema: pgmData (+ optional payloadLen) */
	const cJSON *pgmData    = cJSON_GetObjectItemCaseSensitive(root, "pgmData");
	const cJSON *payloadLen = cJSON_GetObjectItemCaseSensitive(root, "payloadLen");

	if (pgmData && cJSON_IsString(pgmData)) {
	  wrote = handle_apgm_like(cmd_str, args, pgmData, payloadLen, tx_buf, tx_cap);
	} else {
	  /* Fallback: legacy field "data_b64" */
	  const cJSON *data_b64 = cJSON_GetObjectItemCaseSensitive(root, "data_b64");
	  wrote = handle_legacy_apgm(cmd_str, args, data_b64, tx_buf, tx_cap);
	}
  } else {
	/* TODO: implement other commands as needed */
	write_json_error("unsupported cmd", tx_buf, tx_cap);
	wrote = strnlen(tx_buf, tx_cap);
  }

  cJSON_Delete(root);
  return wrote;
}


/* json_prog_handler.c ends here */
