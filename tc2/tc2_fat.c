/* tc2_fat.c --- 
 * 
 * Filename: tc2_fat.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Mon Jan 19 14:54:36 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 
 *           By: 
 *     Update #: 0
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

#include "cli.h"         // FatFs API
#include "ff.h"         // FatFs API
#include "tc2.h"         // FatFs API
#include "tc2_fat.h"         // FatFs API

extern tc2_t g_Tc2;

static void print_tchar(const TCHAR *ts);

int save_ucx_to_csv(const StLvl_t *lvl, const char *path)
{
  if (!lvl || !path) return -1;

  FRESULT fr;
  UINT bw = 0;
  FIL fil;

  // Mount (you can do this once at startup instead)
  static FATFS fs;
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
	// You can log with xil_printf/printf as preferred
	printf("f_mount failed: %d\n", fr);
	return -2;
  }

  fr = f_open(&fil, path, FA_WRITE | FA_OPEN_ALWAYS);
  if (fr == FR_OK) {
	//fr = f_lseek(&fil, f_size(&fil));
	fr = f_lseek(&fil, fil.fsize);
  }
  else{
	fr = f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS);
	cliPrintf("file open: %s", path);
  }
  if (fr != FR_OK) {
	printf("f_open('%s') failed: %d\n", path, fr);
	return -3;
  }

  // Small stack buffer for chunked writing
  char buf[256];
  size_t pos = 0;

  for (size_t i = 0; i < NUM_BLINE; ++i) {
	// Format each value, prepend comma except for first element
	int needed;
	if (i == 0) {
	  needed = snprintf(buf + pos, sizeof(buf) - pos, "%u", (unsigned)lvl->ucx[i]);
	} else {
	  needed = snprintf(buf + pos, sizeof(buf) - pos, ",%u", (unsigned)lvl->ucx[i]);
	}

	if (needed < 0) {
	  f_close(&fil);
	  return -4; // snprintf error
	}

	pos += (size_t)needed;

	// If buffer is near full, flush to file
	if (pos > sizeof(buf) - 16 || i == NUM_BLINE - 1) {
	  // Append newline at the end of the last chunk
	  if (i == NUM_BLINE - 1) {
		if (pos < sizeof(buf) - 2) {
		  buf[pos++] = '\n';
		  buf[pos] = '\0';
		} else {
		  // flush current buffer, then write "\n"
		  fr = f_write(&fil, buf, (UINT)pos, &bw);
		  if (fr != FR_OK || bw != pos) { f_close(&fil); return -5; }
		  pos = 0;
		  buf[pos++] = '\n';
		}
	  }

	  fr = f_write(&fil, buf, (UINT)pos, &bw);
	  if (fr != FR_OK || bw != pos) {
		printf("f_write failed: fr=%d bw=%u pos=%zu\n", fr, bw, pos);
		f_close(&fil);
		return -6;
	  }
	  pos = 0;
	}
  }

  fr = f_sync(&fil); // ensure data committed
  if (fr != FR_OK) {
	printf("f_sync failed: %d\n", fr);
	f_close(&fil);
	return -7;
  }

  fr = f_close(&fil);
  if (fr != FR_OK) {
	printf("f_close failed: %d\n", fr);
	return -8;
  }

  // 5) Unmount SD card
  fr = f_mount(NULL, "0:", 0);
  if (fr != FR_OK) {
	xil_printf("Unmount failed: %d\r\n", fr);
  } else {
	xil_printf("SD unmounted.\r\n");
  }

  return 0;
}

// Stream ucX[] to CSV without large allocations.
// Returns 0 on success, negative on error. Prints FatFs error codes for debugging.
int save_ucX_to_csv(const StLvl_t *lvl, const char *path)
{
  if (!lvl || !path) return -1;

  FRESULT fr;
  UINT bw = 0;
  FIL fil;

  // Mount (you can do this once at startup instead)
  static FATFS fs;
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
	// You can log with xil_printf/printf as preferred
	printf("f_mount failed: %d\n", fr);
	return -2;
  }

  fr = f_open(&fil, path, FA_WRITE | FA_OPEN_ALWAYS);
  if (fr == FR_OK) {
	//fr = f_lseek(&fil, f_size(&fil));
	fr = f_lseek(&fil, fil.fsize);
  }
  else{
	fr = f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS);
	cliPrintf("file open: %s", path);
  }
  if (fr != FR_OK) {
	printf("f_open('%s') failed: %d\n", path, fr);
	return -3;
  }

  // Small stack buffer for chunked writing
  char buf[256];
  size_t pos = 0;

  for (size_t i = 0; i < NUM_BLINE; ++i) {
	// Format each value, prepend comma except for first element
	int needed;
	if (i == 0) {
	  needed = snprintf(buf + pos, sizeof(buf) - pos, "%u", (unsigned)lvl->dX[i]);
	} else {
	  needed = snprintf(buf + pos, sizeof(buf) - pos, ",%u", (unsigned)lvl->dX[i]);
	}

	if (needed < 0) {
	  f_close(&fil);
	  return -4; // snprintf error
	}

	pos += (size_t)needed;

	// If buffer is near full, flush to file
	if (pos > sizeof(buf) - 16 || i == NUM_BLINE - 1) {
	  // Append newline at the end of the last chunk
	  if (i == NUM_BLINE - 1) {
		if (pos < sizeof(buf) - 2) {
		  buf[pos++] = '\n';
		  buf[pos] = '\0';
		} else {
		  // flush current buffer, then write "\n"
		  fr = f_write(&fil, buf, (UINT)pos, &bw);
		  if (fr != FR_OK || bw != pos) { f_close(&fil); return -5; }
		  pos = 0;
		  buf[pos++] = '\n';
		}
	  }

	  fr = f_write(&fil, buf, (UINT)pos, &bw);
	  if (fr != FR_OK || bw != pos) {
		printf("f_write failed: fr=%d bw=%u pos=%zu\n", fr, bw, pos);
		f_close(&fil);
		return -6;
	  }
	  pos = 0;
	}
  }

  fr = f_sync(&fil); // ensure data committed
  if (fr != FR_OK) {
	printf("f_sync failed: %d\n", fr);
	f_close(&fil);
	return -7;
  }

  fr = f_close(&fil);
  if (fr != FR_OK) {
	printf("f_close failed: %d\n", fr);
	return -8;
  }

  // 5) Unmount SD card
  fr = f_mount(NULL, "0:", 0);
  if (fr != FR_OK) {
	xil_printf("Unmount failed: %d\r\n", fr);
  } else {
	xil_printf("SD unmounted.\r\n");
  }

  return 0;
}

FRESULT read_streadv_file(const char *path, StReadv_t *out)
{

  static FATFS fs; // Volume work area

  FRESULT fr;
  //  FIL file;
  UINT br  = 0;
  //char cFileName[16];
  //StReadv_t StReadv;
  //StLvl_t StLvl;

  xil_printf("Mounting SD...\r\n");

  // 1) Mount the volume (drive "0:" is SD in xilffs)
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) { xil_printf("f_mount failed: %d\r\n", fr); return 0;}

  //FRESULT fr;
  FIL fil;
  //UINT br = 0;
  FILINFO finfo;

  if (out == NULL) return FR_INVALID_OBJECT;

  // (Optional) Check file size first for a precise match
  fr = f_stat(path, &finfo);
  if (fr != FR_OK) {
	return fr; // file not found or stat error
  }
  if (finfo.fsize != sizeof(StReadv_t)) {
	// Size mismatch means file doesn't match expected struct size
	return FR_INVALID_OBJECT;
  }

  fr = f_open(&fil, path, FA_READ);
  if (fr != FR_OK) {
	return fr;
  }

  // Read all bytes
  br = 0;
  fr = f_read(&fil, (void*)out, sizeof(StReadv_t), &br);

  // Always close the file
  FRESULT fr_close = f_close(&fil);
  if (fr == FR_OK && fr_close != FR_OK) fr = fr_close;

  if (fr != FR_OK) {
	return fr;
  }
  if (br != sizeof(StReadv_t)) {
	return FR_INT_ERR; // short read
  }
  // 5) Unmount SD card
  fr = f_mount(NULL, "0:", 0);
  if (fr != FR_OK) {
	xil_printf("Unmount failed: %d\r\n", fr);
  } else {
	xil_printf("SD unmounted.\r\n");
  }

  return FR_OK;
}

void saveReadv(StReadv_t *pStReadv)
{

  static FATFS fs; // Volume work area

  FRESULT fr;
  FIL file;
  UINT bw;
  char cFileName[32];

  xil_printf("Mounting SD...\r\n");

  // 1) Mount the volume (drive "0:" is SD in xilffs)
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) { xil_printf("f_mount failed: %d\r\n", fr); return ;}

  //sprintf(cFileName, "0:/readv_%d_%d.bin" ,g_Tc2.nApgmFileCnt ,g_Tc2.nReadvFileCnt++);
  sprintf(cFileName, "0:/readv_%d.bin" ,g_Tc2.nReadvFileCnt++);

  // 2) Open for write; create/overwrite
  fr = f_open(&file, cFileName, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) { xil_printf("f_open failed: %d\r\n", fr); return ;}

  // 3) Write a packed struct (binary record)
  fr = f_write(&file, pStReadv, sizeof(StReadv_t), &bw);
  if (fr != FR_OK || bw != sizeof(StReadv_t)) {
	xil_printf("f_write struct failed: fr=%d bw=%u\r\n", fr, bw);
	f_close(&file);
	return;
  }

  // 4) Flush metadata to the card
  fr = f_sync(&file);
  if (fr != FR_OK) { xil_printf("f_sync failed: %d\r\n", fr); }
  f_close(&file);

  xil_printf("Wrote %u bytes to %s\r\n", (unsigned)sizeof(StReadv_t), cFileName);

  // 5) Unmount SD card
  fr = f_mount(NULL, "0:", 0);
  if (fr != FR_OK) {
	xil_printf("Unmount failed: %d\r\n", fr);
  } else {
	xil_printf("SD unmounted.\r\n");
  }
}

void ls(cli_args_t *args)
{
    (void)args;

    static FATFS fs; // Volume work area
    FRESULT fr;

    xil_printf("Mounting SD...\r\n");

    // 1) Mount the volume (drive "0:" is SD in xilffs)
    fr = f_mount(&fs, "0:", 1);
    if (fr != FR_OK) {
        xil_printf("f_mount failed: %d\r\n", fr);
        return;
    }

    // 2) List directory
    DIR dir;
    FILINFO fno;

    // If LFN is enabled, set up lfname buffer (type must be TCHAR!)
	//#if _USE_LFN
#if 1
    static TCHAR lfn[255 + 1];   // persistent buffer (static to avoid small stack)
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn) / sizeof(TCHAR);
#else
    fno.lfname = NULL;
    fno.lfsize = 0;
#endif

    fr = f_opendir(&dir, "0:/");
    if (fr == FR_OK) {
        for (;;) {
            fr = f_readdir(&dir, &fno); // Read next
            if (fr != FR_OK) {
                xil_printf("f_readdir failed: %d\r\n", fr);
                break;
            }
            if (fno.fname[0] == 0) break; // End of dir

            const TCHAR *name =
#if FF_USE_LFN
                (fno.lfname && fno.lfname[0]) ? fno.lfname :
#endif
                fno.fname;

            xil_printf("%c %10lu  ", (fno.fattrib & AM_DIR) ? 'D' : 'F', (unsigned long)fno.fsize);
            print_tchar(name);
            xil_printf("\r\n");
        }
        f_closedir(&dir);
    } else {
        xil_printf("f_opendir failed: %d\r\n", fr);
    }

    // 3) Unmount SD card
    fr = f_mount(NULL, "0:", 0);
    if (fr != FR_OK) {
        xil_printf("Unmount failed: %d\r\n", fr);
    } else {
        xil_printf("SD unmounted.\r\n");
    }

    xil_printf("Done.\r\n");
}

void rm(cli_args_t *args)
{

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc != 1){
	cliPrintf("Usage:del fileName\r\n");
	return;
  }

  char file_path[16];
  static FATFS fs;
  FRESULT fr;

  // Mount SD card
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
	xil_printf("Mount failed: %d\r\n", fr);
	return;
  }

  // Delete file
  //const char *file_path = "0:/data.csv";
  sprintf(file_path, "0:/%s", argv[0]);
  cliPrintf("argv[0]: %s\r\n", argv[0]);
  

  fr = f_unlink(file_path);
  if (fr == FR_OK) {
	xil_printf("File deleted: %s\r\n", file_path);
  } else {
	xil_printf("Delete failed: %d\r\n", fr);
  }

  // Unmount SD card
  f_mount(NULL, "0:", 0);
  xil_printf("Unmounted.\r\n");
}

// Helper: print TCHAR string with xil_printf
static void print_tchar(const TCHAR *ts)
{
#if FF_LFN_UNICODE
    // Unicode: TCHAR is typically WCHAR (UTF-16). Print basic ASCII subset.
    // Convert simple range 0x20..0x7E; replace others with '?'
    for (size_t i = 0; ts[i] != 0; ++i) {
        uint16_t wc = (uint16_t)ts[i];
        char c = (wc >= 0x20 && wc <= 0x7E) ? (char)wc : '?';
        xil_printf("%c", c);
    }
#else
    // ANSI/OEM: TCHAR is char
    xil_printf("%s", ts);
#endif
}

void fat_test(cli_args_t *args) {
  (void)args;

  static FATFS fs; // Volume work area

  FRESULT fr;
  FIL file;
  //UINT bw, br;
  UINT bw;

  xil_printf("Mounting SD...\r\n");

  // 1) Mount the volume (drive "0:" is SD in xilffs)
  fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) { xil_printf("f_mount failed: %d\r\n", fr);
	//return fr;
	return ;
  }

  // 2) Open for write; create/overwrite
  fr = f_open(&file, "0:/data.bin", FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) { xil_printf("f_open failed: %d\r\n", fr);
	//return fr;
	return ;
  }

  // 3) Write an array of uint32_t (binary)
  uint32_t values[4] = {0xDEADBEEF, 0x12345678, 42, 777};
  fr = f_write(&file, values, sizeof(values), &bw);
  if (fr != FR_OK || bw != sizeof(values)) {
	xil_printf("f_write values failed: fr=%d bw=%u\r\n", fr, bw);
	f_close(&file);
	//return fr ? fr : -1;
	return;
  }

  // 4) Write a packed struct (binary record)
  sample_t s = {.timestamp_ms=12345, .temp_c=23.5f, .accel_x=100, .accel_y=-50, .accel_z=25};
  fr = f_write(&file, &s, sizeof(s), &bw);
  if (fr != FR_OK || bw != sizeof(s)) {
	xil_printf("f_write struct failed: fr=%d bw=%u\r\n", fr, bw);
	f_close(&file);
	//return fr ? fr : -1;
	return;
  }

  // 5) Flush metadata to the card
  fr = f_sync(&file);
  if (fr != FR_OK) { xil_printf("f_sync failed: %d\r\n", fr); }

  f_close(&file);
  xil_printf("Wrote %u + %u bytes to data.bin\r\n", (unsigned)sizeof(values), (unsigned)sizeof(s));

#ifdef AABBCC
  // Create/write a file
  fr = f_open(&file, "hello.txt", FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) {
	xil_printf("f_open write failed: %d\r\n", fr);
	//return fr;
	return;
  }
  const char *msg = "Hello from Zynq FatFs!\r\n";
  fr = f_write(&file, msg, (UINT)strlen(msg), &bw);
  f_close(&file);
  if (fr != FR_OK || bw != strlen(msg)) {
	xil_printf("f_write failed: %d bw=%u\r\n", fr, bw);
	//return fr ? fr : -1;
	return;
  }
  xil_printf("Wrote hello.txt (%u bytes)\r\n", bw);
#endif
  

  cliPrintf("fat_test\r\n");
}

int load_readv_0(StReadv_t *dst)
{
  // Mount the drive once somewhere in your init code; do it here for completeness.
  static FATFS fs;
  FRESULT fr = f_mount(&fs, "0:", 1);
  if (fr != FR_OK) {
	printf("f_mount failed: %d\n", fr);
	return -1;
  }

  fr = read_streadv_file("0:/READV_0.BIN", dst);
  if (fr != FR_OK) {
	printf("read_streadv_file failed: %d\n", fr);
	return -2;
  }

  // If needed: invalidate DCache for the struct (Standalone BSP)
  // #include "xil_cache.h"
  // Xil_DCacheInvalidateRange((UINTPTR)dst, sizeof(StReadv_t));

  return 0;
}

/* tc2_fat.c ends here */
