/* tc2_enet.c --- 
 * 
 * Filename: tc2_enet.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Wed Mar  4 11:14:57 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Fri Mar  6 19:42:57 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 61
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
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <tc2_lib/cJSON.h>

#include "xil_printf.h"
#include "tc2.h"
#include "cli.h"
#include "json_prog_handler.h"

void tc2_enet_update_0(char *input);

/* Safely copy a cJSON string item into a fixed buffer. Returns 0 on success. */
static int copy_json_string(const cJSON *item, char *dst, size_t dst_sz) {
  if (!item || !cJSON_IsString(item) || !item->valuestring || !dst || dst_sz == 0) {
	return -1;
  }
  /* Ensure null-termination */
  snprintf(dst, dst_sz, "%s", item->valuestring);
  return 0;
}

/*
 * Parse JSON of the form:
 * { "cmd": "<string>", "args": ["<string>", "<string>", ...] }
 *
 * - Extracts "cmd" as a string
 * - Extracts up to max_args strings from "args"
 * - Returns number of args copied on success, or -1 on error
 */
int parse_cmd_and_args_as_strings(const char *json_text,
                                  char *cmd_out, size_t cmd_out_sz,
                                  char **args_out, size_t max_args) {
  if (!json_text || !cmd_out || cmd_out_sz == 0 || !args_out) {
	return -1;
  }

  int ret = -1;
  cJSON *root = cJSON_Parse(json_text);
  if (!root) {
	fprintf(stderr, "JSON parse error near: %s\n",
			cJSON_GetErrorPtr() ? cJSON_GetErrorPtr() : "(unknown)");
	return -1;
  }

  const cJSON *cmd  = cJSON_GetObjectItemCaseSensitive(root, "cmd");
  const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");

  /* Require "cmd" as string and "args" as array */
  if (!cJSON_IsString(cmd) || !cmd->valuestring) {
	fprintf(stderr, "`cmd` must be a JSON string\n");
	goto cleanup;
  }
  if (!cJSON_IsArray(args)) {
	fprintf(stderr, "`args` must be a JSON array\n");
	goto cleanup;
  }

  /* Copy cmd into caller buffer */
  if (copy_json_string(cmd, cmd_out, cmd_out_sz) != 0) {
	fprintf(stderr, "Failed to copy `cmd` string\n");
	goto cleanup;
  }

  /* Copy each args[i] strictly as string */
  int n = (int)cJSON_GetArraySize(args);
  if ((size_t)n > max_args) n = (int)max_args;

  for (int i = 0; i < n; ++i) {
	const cJSON *ai = cJSON_GetArrayItem(args, i);
	if (!cJSON_IsString(ai) || !ai->valuestring) {
	  fprintf(stderr, "`args[%d]` is not a JSON string\n", i);
	  goto cleanup;
	}
	/* Duplicate into caller-provided storage: args_out[i] must be preallocated */
	snprintf(args_out[i], strlen(ai->valuestring) + 1, "%s", ai->valuestring);
  }

  ret = n;

 cleanup:
  cJSON_Delete(root);
  return ret;
}

/* Convert a JSON value that might be a string like "32" or a number 32 into uint32_t */
static int json_to_u32(const cJSON *v, uint32_t *out) {
  if (!v || !out) return -1;

  if (cJSON_IsNumber(v)) {
	if (v->valuedouble < 0 || v->valuedouble > 4294967295.0) return -1;
	*out = (uint32_t) v->valuedouble;
	return 0;
  }

  if (cJSON_IsString(v) && v->valuestring) {
	char *end = NULL;
	errno = 0;

	// Detect base: 0x... => hex, 0... => octal (we'll treat "0" as zero), else decimal
	unsigned long val = strtoul(v->valuestring, &end, 0);
	if (errno != 0 || end == v->valuestring || *end != '\0') return -1;
	if (val > 0xFFFFFFFFUL) return -1;
	*out = (uint32_t)val;
	return 0;
  }

  return -1;
}

/* Convert a JSON value that should be a hex string like "0xa3000000" to uint32_t */
static int json_hex_string_to_u32(const cJSON *v, uint32_t *out) {
  if (!v || !out || !cJSON_IsString(v) || !v->valuestring) return -1;

  const char *s = v->valuestring;

  // Optional: allow either "0x...." or plain hex digits
  int base = 16;
  if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;

  char *end = NULL;
  errno = 0;
  unsigned long val = strtoul(s, &end, base);
  if (errno != 0 || end == s || *end != '\0') return -1;
  if (val > 0xFFFFFFFFUL) return -1;

  *out = (uint32_t)val;
  return 0;
}

static int parse_cmd_args(const char *json_text,
				   char *cmd_buf, size_t cmd_buf_sz,
				   uint32_t *addr_out, uint32_t *len_out) {
  if (!json_text || !cmd_buf || cmd_buf_sz == 0 || !addr_out || !len_out) return -1;

  int ret = -1;
  cJSON *root = cJSON_Parse(json_text);
  if (!root) {
	const char *errptr = cJSON_GetErrorPtr();
	fprintf(stderr, "JSON parse error near: %s\n", errptr ? errptr : "(unknown)");
	return -1;
  }

  // root must be an object
  if (!cJSON_IsObject(root)) {
	fprintf(stderr, "Top-level JSON is not an object.\n");
	goto cleanup;
  }

  const cJSON *cmd  = cJSON_GetObjectItemCaseSensitive(root, "cmd");
  const cJSON *args = cJSON_GetObjectItemCaseSensitive(root, "args");

  if (!cJSON_IsString(cmd) || !cmd->valuestring) {
	fprintf(stderr, "`cmd` must be a string.\n");
	goto cleanup;
  }
  if (!cJSON_IsArray(args)) {
	fprintf(stderr, "`args` must be an array.\n");
	goto cleanup;
  }
  if (cJSON_GetArraySize(args) < 2) {
	fprintf(stderr, "`args` must have at least 2 elements.\n");
	goto cleanup;
  }

  // Copy cmd safely into caller's buffer
  snprintf(cmd_buf, cmd_buf_sz, "%s", cmd->valuestring);

  // args[0] is expected to be a hex address string
  const cJSON *arg0 = cJSON_GetArrayItem(args, 0);
  if (json_hex_string_to_u32(arg0, addr_out) != 0) {
	fprintf(stderr, "Failed to parse args[0] as hex address string.\n");
	goto cleanup;
  }

  // args[1] may be a number or numeric string
  const cJSON *arg1 = cJSON_GetArrayItem(args, 1);
  if (json_to_u32(arg1, len_out) != 0) {
	fprintf(stderr, "Failed to parse args[1] as length (number or numeric string).\n");
	goto cleanup;
  }

  // Success
  ret = 0;

 cleanup:
  cJSON_Delete(root);
  return ret;
}

void tc2_enet_update(char *input){
  char buf[PROG_PAYLOAD_SIZE];
  //tc2_enet_update_0(input);
  (void)process_json_line(input, buf, PROG_PAYLOAD_SIZE);
}

void tc2_enet_update_0(char *input){

//  bool ret = false;
  /* Prepare output buffers */
  char cmd[32] = {0};

  /* Suppose we expect up to 4 args; allocate buffers for each string */
  enum { MAX_ARGS = 4 };
  char arg0[64] = {0}, arg1[64] = {0}, arg2[64] = {0}, arg3[64] = {0};
  char *args_bufs[MAX_ARGS] = { arg0, arg1, arg2, arg3 };

  int argc = parse_cmd_and_args_as_strings(input, cmd, sizeof(cmd),
										   args_bufs, MAX_ARGS);
  if (argc < 0) {cliPrintf("Failed to parse JSON command.\n");}

  cliPrintf("cmd = \"%s\"\r\n", cmd);
  for (int i = 0; i < argc; ++i) {cliPrintf("args[%d] = \"%s\"\r\n", i, args_bufs[i]);}

  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {print("malloc failed!!\n"); return;}

  /* Example usage: treat them purely as strings */
       if (strcmp(cmd, "reade" ) == 0 && argc >= 2) {
	char *addr_str = args_bufs[0];  // "0xa3000000"
	const char *len_str  = args_bufs[1];  // "64"
	cliPrintf("Will perform %s: addr=\"%s\", len=\"%s\"\r\n", cmd, addr_str, len_str);

	cli_args.argc     =  1;
	cli_args.argv[0] = addr_str;
	reade(&cli_args);
  }
  else if (strcmp(cmd, "readva") == 0 && argc >= 2) {
	char *addr_str = args_bufs[0];  // "0xa3000000"
	char *len_str  = args_bufs[1];  // "64"
	cliPrintf("Will perform %s: addr=\"%s\", len=\"%s\"\r\n", cmd, addr_str, len_str);

	cli_args.argc     =  2;
	cli_args.argv[0] = addr_str;
	cli_args.argv[1] = len_str;
	readva(&cli_args);
  }
  else if (strcmp(cmd, "i"     ) == 0) {
	cli_args.argc     =  0     ;
	tc2_init(&cli_args);
  }
  else if (strcmp(cmd, "h"     ) == 0) {
	cli_args.argc     =  0;
	cliShowList(&cli_args);
  }
  else if (strcmp(cmd, "por"   ) == 0) {
	cli_args.argc     =  0;
	porb(&cli_args);
  }
  else if (strcmp(cmd, "ce"    ) == 0 && argc >= 1) {
	char *addr_str = args_bufs[0];  
	cli_args.argc     =  1;
	cli_args.argv[0] = addr_str;
	onCE_BIAS_CE(&cli_args);
  }
	   //  else if (strcmp(cmd, "apgm"    ) == 0 && argc >= 1) {
	   //	char *addr_str = args_bufs[0];  
	   //	cli_args.argc     =  1;
	   //	cli_args.argv[0] = addr_str;
	   //	on_ana_page_program(&cli_args);
	   //  }


  free(cli_args.argv); // cleanup
}

int __main(void) {
  const char *input = "{\"cmd\": \"reade\", \"args\": [\"0xa3000000\", \"32\"]}";

  char cmd[32] = {0};
  uint32_t addr = 0, length = 0;

  if (parse_cmd_args(input, cmd, sizeof(cmd), &addr, &length) == 0) {
	printf("cmd   : %s\n", cmd);
	printf("addr  : 0x%08" PRIx32 "\n", addr);
	printf("length: %" PRIu32 "\n", length);

	// Example: act on the command
	if (strcmp(cmd, "reade") == 0) {
	  // TODO: perform read from 'addr' of 'length' bytes
	  // For illustration:
	  printf("Would read %u bytes from 0x%08" PRIx32 "\n", length, addr);
	} else {
	  fprintf(stderr, "Unknown cmd: %s\n", cmd);
	  return 2;
	}
	return 0;
  } else {
	fprintf(stderr, "Failed to parse the JSON command.\n");
	return 1;
  }
}



/* tc2_enet.c ends here */
