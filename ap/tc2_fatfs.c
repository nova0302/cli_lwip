/* tc2_fatfs.c --- 
 * 
 * Filename: tc2_fatfs.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: 토  1월 24 21:27:16 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: 일  1월 25 23:40:15 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 70
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

#include "ff.h"
#include "cli.h"
#include "tc2.h"

FRESULT cat_file(const char* filename);
FRESULT save_levels_to_csv(const char* filename, levels_t* data);

//FATFS fatfs;      // File system object
//FIL fil;          // File object
//FRESULT res;      // FatFs return code
//char *filename = "test.txt";
//char write_buf[] = "Hello from Xilffs!";
//char read_buf[32];
//UINT bw, br;      // Bytes written/read

// 1. Declare this GLOBALLY or as STATIC
static FATFS fatfs; 
static int is_mounted = 0;

void mount_sd() {
    FRESULT res = f_mount(&fatfs, "0:", 1);
    if (res == FR_OK) {
        is_mounted = 1;
        xil_printf("SD Mounted\r\n");
    }
}

void unmount_sd() {

    if (!is_mounted) return;
    // Passing NULL for the structure and 1 for 'immediate'
    FRESULT res = f_mount(NULL, "0:", 1); 
    if (res == FR_OK) {
        is_mounted = 0;
        xil_printf("SD Unmounted\r\n");
    } else {
        // If you get 12 here, the pointer in the FatFs table was already cleared
        xil_printf("Unmount failed: %d\r\n", res);
    }
}

void test (cli_args_t *args)
{
  // 1. Mount the SD Card
  //mount_sd();
    FRESULT res = f_mount(&fatfs, "0:", 1);
    if (res == FR_OK) {
        is_mounted = 1;
        xil_printf("SD Mounted\r\n");
    }



  FIL fil;          // File object
  //FRESULT res;      // FatFs return code
  UINT bw, br;      // Bytes written/read

  const char *pFilename = "HelloWorld.csv";

  char write_buf[] = "Hello from Xilffs!";
  char read_buf[32];

  // 2. Open file for writing (Create new if it doesn't exist)
  res = f_open(&fil, pFilename, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
  if (res != FR_OK) {
    xil_printf("Failed to open file (Error: %d)\r\n", res);
  }

  // 3. Write data to the file
  res = f_write(&fil, write_buf, sizeof(write_buf), &bw);
  if (res != FR_OK) {
    xil_printf("Failed to write to file (Error: %d)\r\n", res);
    f_close(&fil);
  }

  // 4. Reset pointer and read data back to verify
  f_lseek(&fil, 0); 
  res = f_read(&fil, read_buf, sizeof(read_buf), &br);
  if (res == FR_OK) {
    xil_printf("Read from SD: %s\r\n", read_buf);
  }

  // 5. Close the file
  f_close(&fil);

  if (!is_mounted) return;
  // Passing NULL for the structure and 1 for 'immediate'
  res = f_mount(NULL, "0:", 1); 
  if (res == FR_OK) {
    is_mounted = 0;
    xil_printf("SD Unmounted\r\n");
  } else {
    // If you get 12 here, the pointer in the FatFs table was already cleared
    xil_printf("Unmount failed: %d\r\n", res);
  }

//  unmount_sd();

}


//void test1 (cli_args_t *args)
//{
//  // 1. Mount the SD Card
//  // "0:" is the logical drive number for the first SD controller
//  res = f_mount(&fatfs, "0:", 1);
//  if (res != FR_OK) {
//    xil_printf("Failed to mount SD card (Error: %d)\r\n", res);
//    //return -1;
//  }
//
//  // 2. Open file for writing (Create new if it doesn't exist)
//  res = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
//  if (res != FR_OK) {
//    xil_printf("Failed to open file (Error: %d)\r\n", res);
//    //return -1;
//  }
//
//  // 3. Write data to the file
//  res = f_write(&fil, write_buf, sizeof(write_buf), &bw);
//  if (res != FR_OK) {
//    xil_printf("Failed to write to file (Error: %d)\r\n", res);
//    f_close(&fil);
//    //return -1;
//  }
//
//  // 4. Reset pointer and read data back to verify
//  f_lseek(&fil, 0);
//  res = f_read(&fil, read_buf, sizeof(read_buf), &br);
//  if (res == FR_OK) {
//    xil_printf("Read from SD: %s\r\n", read_buf);
//  }
//
//  // 5. Close the file
//  f_close(&fil);
//
//  // 2. Unmount the drive
//  // Passing NULL detaches the FATFS object from the library
//  res = f_mount(NULL, "0:", 1);
//
//  if (res == FR_OK) {
//    xil_printf("SD card unmounted successfully.\r\n");
//  } else {
//    xil_printf("Unmount failed with code: %d\r\n", res);
//  }
//}

//FRESULT ls(const char* path)
void ls (cli_args_t *args)
{
  FRESULT res;
  DIR dir;
  static FILINFO fno; // FILINFO structure holds file details
  const char *pcPath = "0:/";

  // 1. Mount the SD Card
  // "0:" is the logical drive number for the first SD controller
  res = f_mount(&fatfs, "0:", 1); 
  if (res != FR_OK) {
    xil_printf("Failed to mount SD card (Error: %d)\r\n", res);
    //return -1;
  }

  res = f_opendir(&dir, pcPath);                       // Open the directory
  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &fno);               // Read a directory item
      if (res != FR_OK || fno.fname[0] == 0) break; // Break on error or end of dir

      if (fno.fattrib & AM_DIR) {                // Check if it's a directory
        xil_printf(" <DIR>   %s\r\n", fno.fname);
      } else {                                   // It's a file
        xil_printf("%10u  %s\r\n", (UINT)fno.fsize, fno.fname);
      }
    }
    f_closedir(&dir);
  }

  // 2. Unmount the drive
  // Passing NULL detaches the FATFS object from the library
  res = f_mount(NULL, "0:", 1);

  if (res == FR_OK) {
    xil_printf("SD card unmounted successfully.\r\n");
  } else {
    xil_printf("Unmount failed with code: %d\r\n", res);
  }

  //return res;
}

//void rm(const char* path)
void rm (cli_args_t *args)
{
  const char *target_file = NULL;

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0) {
    target_file = argv[0];
    cliPrintf("got: %s \r\n", argv[0]);
    //size = (int)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  } else{
    cliPrintf("usage: rm fileName\r\n");
    return;
  }

  FATFS fatfs;
  FRESULT res;
  // const char *target_file = "OLD_LOG.TXT";

  // 1. Mount the drive
  res = f_mount(&fatfs, "0:", 1);
  if (res != FR_OK) {
    xil_printf("Mount failed: %d\r\n", res);
    //return -1;
  }

  // 2. Delete the file
  // f_unlink works for both files and empty directories
  res = f_unlink(target_file);

  if (res == FR_OK) {
    xil_printf("File '%s' deleted successfully.\r\n", target_file);
  } 
  else if (res == FR_NO_FILE) {
    xil_printf("Error: File '%s' does not exist.\r\n", target_file);
  } 
  else if (res == FR_DENIED) {
    xil_printf("Error: Cannot delete. File is read-only or a non-empty directory.\r\n");
  } 
  else {
    xil_printf("Delete failed with error code: %d\r\n", res);
  }

  // 2. Unmount the drive
  // Passing NULL detaches the FATFS object from the library
  res = f_mount(NULL, "0:", 1);

  if (res == FR_OK) {
    xil_printf("SD card unmounted successfully.\r\n");
  } else {
    xil_printf("Unmount failed with code: %d\r\n", res);
  }

}

void cat (cli_args_t *args)
{

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0) {

    cliPrintf("got: %s \r\n", argv[0]);

    const  char *target_file = argv[0];
    cat_file(target_file);
    //size = (int)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  } else{
    cliPrintf("usage: rm fileName\r\n");
    return;
  }
}

#define BUFFER_SIZE 512

FRESULT cat_file(const char* filename) {
  FIL fil;            // File object
  FRESULT res;        // API return code
  char buffer[BUFFER_SIZE];
  UINT br;            // Bytes read

  // 1. Mount the drive
  res = f_mount(&fatfs, "0:", 1);
  if (res != FR_OK) {
    xil_printf("Mount failed: %d\r\n", res);
    //return -1;
  }

  // 1. Open the file for reading
  res = f_open(&fil, filename, FA_READ);
  if (res != FR_OK) {
    xil_printf("Error: Could not open %s (Code: %d)\r\n", filename, res);
    return res;
  }

  xil_printf("--- Content of %s ---\r\n", filename);

  // 2. Read and print in chunks until the end of the file
  while (1) {
    res = f_read(&fil, buffer, sizeof(buffer) - 1, &br); // Leave space for null terminator
    if (res != FR_OK || br == 0) break;                 // Error or End of File

    buffer[br] = '\0';  // Null terminate the string chunk
    xil_printf("%s", buffer);
  }

  xil_printf("\r\n--- End of File ---\r\n");

  // 3. Close the file
  f_close(&fil);


  // 2. Unmount the drive
  // Passing NULL detaches the FATFS object from the library
  res = f_mount(NULL, "0:", 1);

  if (res == FR_OK) {
    xil_printf("SD card unmounted successfully.\r\n");
  } else {
    xil_printf("Unmount failed with code: %d\r\n", res);
  }
  return res;
}

void save(cli_args_t *args)
{


  int    argc = args->argc;
  char **argv = args->argv;

  if(argc < 1) {
    cliPrintf("usage: save [filename] \r\n");
    return;
    //size = (int)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  } 

  levels_t levels;
  for(int r=0; r<NUM_BIT_LINE; r++){
    for(int c=0; c<32; c++){
      levels.x[r][c] = c;
    }
  }
  for(int c=0; c<NUM_BIT_LINE; c++){
    levels.X[c] = c % 32;
  }
  //save_levels_to_csv("aa.csv", &levels);
  //const char *target_file = argv[0];
  const char *target_file = args->argv[0];
  cliPrintf("Saving to %s...\r\n", target_file);
  save_levels_to_csv(target_file, &levels);
}

FRESULT save_levels_to_csv(const char* filename, levels_t* data) {
  FIL fil;
  FRESULT res;
  char buffer[128]; // Buffer for a single row
  UINT bw;


  // 1. Mount the SD Card
  // "0:" is the logical drive number for the first SD controller
  res = f_mount(&fatfs, "0:", 1); 
  if (res != FR_OK) {
    xil_printf("Failed to mount SD card (Error: %d)\r\n", res);
    //return -1;
  }

  // 2. Open file for writing (Create new if it doesn't exist)
  //res = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
  res = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (res != FR_OK) {
    xil_printf("Failed to open file (Error: %d)\r\n", res);
    //return -1;
  }
  // Header
  //res = f_write(&fil, write_buf, sizeof(write_buf), &bw);
  //f_puts("x0,x1,x2,x3,x4,X_Converted\n", &fil);
  //sprintf(buffer, "x0,x1,x2,x3,x4,X_Converted\n");
  //f_write(&fil, buffer, sizeof(buffer), &bw);

  char *pHeader = "x0,x1,x2,x3,x4,X_Converted\n";
  f_write(&fil, pHeader, strlen(pHeader), &bw);

  for (int i = 0; i < NUM_BIT_LINE; i++) {
    /* 
       Conversion Logic:
       (uint8_t)data->X[i]           -> Truncates (fastest)
       (uint8_t)round(data->X[i])    -> Rounds to nearest (most accurate)
    */
    //uint8_t x_conv = (uint8_t)round(data->X[i]);
    uint8_t x_conv = (uint8_t)(data->X[i]);

    int len = snprintf(buffer, sizeof(buffer), "%u,%u,%u,%u,%u,%u\n",
                       data->x[0][i], data->x[1][i], data->x[2][i],
                       data->x[3][i], data->x[4][i], x_conv);

    res = f_write(&fil, buffer, len, &bw);
    if (res != FR_OK) break;
  }

  f_close(&fil);

  // 2. Unmount the drive
  // Passing NULL detaches the FATFS object from the library
  res = f_mount(NULL, "0:", 1);

  if (res == FR_OK) {
    xil_printf("SD card unmounted successfully.\r\n");
  } else {
    xil_printf("Unmount failed with code: %d\r\n", res);
  }

  return res;
}



/* tc2_fatfs.c ends here */
