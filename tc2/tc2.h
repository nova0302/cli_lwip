/* tc2.h --- 
 * 
 * Filename: tc2.h
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Nov 27 18:09:30 2025 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Thu Mar  5 13:50:47 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 10
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

#include <stdbool.h>
#include "cli.h"

#define NUM_BLINE (2048)
#define NUM_WLINE (1280)

#define NOE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define IP_Mode_TC2b  (0)
#define IP_Mode_TC2   (1)
#define ROW_EVEN      (0)
#define ROW_ODD       (2)

#define PGM_TYPE_ABP  (0)
#define PGM_TYPE_OOTP (1)
#define PGM_TYPE      PGM_TYPE_OOTP

#define RDE_TYPE_ORIG (0 << 1)
#define RDE_TYPE_BINT (1 << 1)
#define RDE_TYPE      RDE_TYPE_BINT

#define READE_ADDR_TYPE_WRAP      (0 << 2)
#define READE_ADDR_TYPE_LINEAR    (1 << 2)
#define READE_ADDR_TYPE   READE_ADDR_TYPE_WRAP

#define RDDNCY_ARRDN    (1 << 0)
#define RDDNCY_ARRCOL   (1 << 1)
#define RDDNCY_NVR      (1 << 3)
#define RDDNCY_NONE     (0)

typedef enum{ePGM_TYPE_ABP, ePGM_TYPE_OOTP}pgm_type_selection_e;
typedef enum{eRDE_TYPE_ORIGIN, eRDE_TYPE_BIRD=2}rde_type_selection_e;

typedef struct{
  uint32_t n;
  uint8_t ucx[NUM_BLINE];
  double dX[NUM_BLINE];
  //uint8_t ucX[NUM_BLINE];
}StLvl_t;

typedef enum {eVCG_4v ,eVCG_4v5 ,eVCG_5v ,eVCG_5v5 ,eVCG_6v}VCG_e;
typedef enum{eIDLE, eINIT_ERASE, eDPGM, eAPGM, eBPGM, eREADE, eREADN, eREADV, eREADVA, eREADVB, eREADB}tc2_modte_e;
typedef enum{eWriteThrouh, eWriteBack}cfg_write_policy_e;

typedef enum {eVEG_2v ,eVEG_2v5 ,eVEG_3v ,eVEG_3v5 ,eVEG_4v ,eVEG_4v5 ,eVEG_5v ,eVEG_MAX}VEG_e;

typedef enum{
  NVR_LOW  = 0x0,
  NVR_HIGH = 0x8
}NVR_CNTL_e;

typedef enum{
  AutoBitPart,
  OnlyOneTime // << for bliso
}pgm_type_sel_e;

typedef enum{
  OriginalData, // << for bliso
  BitInterleavedData
}reade_type_sel_e;

typedef enum{
  WrappAddr,
  LinearAddr // << for bliso
}cmd_output_signal_e;

typedef struct _cfg_addr_data_pair{
  uint32_t addr;
  uint32_t data;
}cfg_addr_data_pair_t;

//typedef struct _cfg_data{
//  uint16_t sNBits;
//  uint16_t sNShift;
//  uint16_t sCfgData;
//}cfg_data_t;

typedef struct __attribute__((packed)) {
  uint32_t timestamp_ms;
  float    temp_c;
  int16_t  accel_x;
  int16_t  accel_y;
  int16_t  accel_z;
} sample_t;

typedef struct __attribute__((packed)) {
  uint32_t nRef;
  uint32_t nReserved[3];
  uint32_t nVfyo[2048/32]; // 2048/32 = 64
} StVfyo_t;

typedef struct __attribute__((packed)) {
  uint32_t nVcg;
  uint32_t nReserved[3];
  StVfyo_t StVfyo[32];
} StReadv_t;

typedef struct{
  VCG_e eVCG;
  tc2_modte_e eMode;
  //bool bSetCfgInit;
  bool bProgramType;
  bool bApgmBitPattern;
  bool bCE_BIAS_CE;
  bool bRecallInitialized;
  bool bSetCfgInit;
  bool bSetCfgApgm;
  bool bSetCfgReade;
  bool bSetCfgReadn;
  bool bSetCfgReadv;
  bool bReadeAddrType;
  bool bReadeType;
  bool bVerbose;
  bool bReadeUseTM;
  uint32_t nDbgLevel;
  uint32_t nReadvFileCnt;
  uint32_t nApgmFileCnt;
  uint32_t nVcg;
  uint16_t sRecallArr[5][16];
  StReadv_t StReadv;
  uint32_t nPageData[64];
}tc2_t;

/* ====== enum ====== */
typedef enum {
    ST_Ready            = 0,
    ST_Set_Config       = 1,
    ST_Read_Config      = 2,
    ST_Sector_Erase     = 3,
    ST_Chip_Erase       = 4,
    ST_Page_Program     = 5,
    ST_Block_Program    = 6,
    ST_INPUT_ROW_TAG    = 7,
    ST_INPUT_ROW_Reg    = 8,
    ST_READN_Mode       = 9,
    ST_VFYO_Verify_All  = 10,
    ST_VFYO_Verify_One  = 11,
    ST_VFYO_READE       = 12,
    ST_READE            = 13,
    ST_Recall           = 14,
    ST_IO_MUX_Write     = 15
} amac_cmd_t;

// TC2 functions
void onCE_BIAS_CE(cli_args_t *args) ;
void on_sector_erase(cli_args_t *args) ;
void on_dig_page_program(cli_args_t *args) ;
void on_ana_page_program(cli_args_t *args) ;
void on_readv_1row(cli_args_t *args) ;
void on_readv_1row_verbose(cli_args_t *args);
void reade(cli_args_t *args);

void on_rd_cfga(cli_args_t *args);
void on_rd_cfgd(cli_args_t *args);
void on_rd_cfg_verbose(cli_args_t *args);

void on_set_configa(cli_args_t *args);
void on_set_configd(cli_args_t *args);

void getRdn(cli_args_t *args);
void readn(cli_args_t *args);
void readvs(cli_args_t *args) ;
void readva(cli_args_t *args) ;
void readvb(cli_args_t *args) ;

void readvas(cli_args_t *args) ;
void verbose(cli_args_t *args); 
void show_status(cli_args_t *args); 
void set_vcg(cli_args_t *args); 
void inc_vcg(cli_args_t *args); 
void tc2_init(cli_args_t *args); 
void recall(cli_args_t *args); 

void ft_cfg_init(cli_args_t *args);
//void ft_cfg_apgm(cli_args_t *args);
//void ft_cfg_reade(cli_args_t *args);
void ft_cfg_readv(cli_args_t *args);
void ft_cfg_readn(cli_args_t *args);
void clear_all_flag(cli_args_t *args);
void md_ctrl(cli_args_t *args);
void bliso_pgm(cli_args_t *args);
void bliso_read(cli_args_t *args);

void md_ft_cfg(cli_args_t *args);
void ft_default(cli_args_t *args);
void ft_dump(cli_args_t *args);

void setVcg(cli_args_t *args);
void porb(cli_args_t *args);
void erase_all_sectors(cli_args_t *args);
void dpgm_all(cli_args_t *args);

// FatFS related
void ls(cli_args_t *args);
void rm(cli_args_t *args);

void apgm_test(cli_args_t *args);
void regression_test(cli_args_t *args);
void regres_test_all (cli_args_t *args);
void regres_test_100 (cli_args_t *args);

void toggleAutoBitPart(cli_args_t *args);
void toggleRdeAddrType(cli_args_t *args);

//void togglePgmType(cli_args_t *args);

void tgl_rde_usetm(cli_args_t *args);
void toggleApgmBitPattern(cli_args_t *args);
void tglDbgLevel(cli_args_t *args);
void toggleReadeType(cli_args_t *args);

void getLevels (cli_args_t *args);
void getLvls(StReadv_t *rec, uint8_t ucLvlArr[], uint32_t nNumElements);

void bit_interleave(cli_args_t *args);
void initStReadv(StReadv_t *pStReadv);

#endif /* TC2_H */

/* tc2.h ends here */
