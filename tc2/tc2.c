/* tc2.c --- 
 * 
 * Filename: tc2.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Thu Nov 27 18:11:48 2025 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Thu Mar  5 15:33:30 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 181
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

//#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include "ff.h"         // FatFs API
#include "sleep.h"
#include "cli.h"
#include "tc2.h"
#include "tc2_util.h"
#include "tc2_fat.h"
#include "membrain.h"

MbBuff_t g_MbBuff = {0,};

void acntInit();
static void showInfo(char *pcBase, uint32_t numWords);
static uint16_t getBitMask(uint16_t sNumBits, uint16_t sNumShift );
static cfg_addr_data_pair_t getVcg(uint32_t nVCG);
static void ft_cfg_reade(cfg_write_policy_e cfg_write_policy, uint32_t nFT_IDAC);

static char* getCurrnetMode();
static void ft_cfg_dpgm(cfg_write_policy_e cfg_write_policy);
static void ft_cfg_apgm(cfg_write_policy_e cfg_write_policy);
static void ft_cfg_bpgm(cfg_write_policy_e cfg_write_policy);
static void ft_cfg_erase(cfg_write_policy_e cfg_write_policy);

static void tm_cfg_init();
static void tm_cfg_reade(bool bInit);
static void ft_set_cfg(cfg_addr_data_pair_t cfg_addr_data_pair_arr[], uint32_t nNumElements, bool bChkReadback);
static void ft_read_cfg(cfg_addr_data_pair_t cfg_addr_data_pair_arr[], uint32_t nNumElements);

static void ft_cfg_readva_entry(uint32_t nRmpUp);
static void ft_cfg_readva_exit(void);

static void getLvlAvg(StLvl_t *pStLvl);

void evalReadvas(uint32_t nVcg);
void evalReadvas100(uint32_t nVcg);

uint32_t getSel(uint32_t num);

unsigned int Input_Row_TAG_Data [44];
unsigned int Input_Row_Reg_Data [324];	// XA[10:4] ; 0x00 ~ 0x4F (128bit), 0x50 {64bit)
unsigned int Input_Verify_Reg_Data [36];

tc2_t g_Tc2 = {
  .eVCG          = eVCG_6v
  ,.eMode        = eIDLE
  ,.bProgramType        = false
  ,.bApgmBitPattern     = false
  ,.bCE_BIAS_CE         = false
  ,.bRecallInitialized  = true 
  ,.bSetCfgInit         = false
  ,.bSetCfgApgm         = false
  ,.bSetCfgReade        = false
  ,.bSetCfgReadn        = false
  ,.bSetCfgReadv        = false
  ,.bVerbose            = false
  ,.nDbgLevel           = false
  ,.bReadeAddrType      = true
  ,.bReadeUseTM           = false
  ,.nApgmFileCnt        = 0
  ,.nVcg                = 0xDF0 // 4.32[v]
  ,.sRecallArr    = {
	{0x0014 ,0x0008 ,0x0012 ,0x000C ,0x000F ,0x0003 ,0x0006 ,0x0035 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0}
	,{0x0390 ,0x0B2D ,0x0031 ,0x0CBF ,0x0D89 ,1 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0}
	,{0x000A ,0x001F ,0x009C ,0 ,0 ,0 ,1 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0}
	,{0x000A ,0x001F ,0x000C ,0x0038, 0x000F, 0x0003  ,0x0017 ,0x0003 ,0x0000 ,0x001C ,1 ,0 ,0 ,0 ,0 ,0}
	,{0x000E ,0x000E ,0x009C ,0x003F,0x0029,0x000C ,0x0038 ,0x0003 ,0x000F ,0x0017 ,0x0003 ,0x0000 ,0x001C ,1 ,1 ,0}}
  ,.nPageData = {0,}
};

char *pcVcg[]={"vcg_3v2", "vcg_3v4", "vcg_3v6", "vcg_3v8",
			   "vcg_4v0", "vcg_4v2", "vcg_4v4", "vcg_4v6",
			   "vcg_4v8", "vcg_5v0"};

void tgl_rde_usetm(cli_args_t *args) {
  if(g_Tc2.bReadeUseTM)
	g_Tc2.bReadeUseTM = false;
  else
	g_Tc2.bReadeUseTM = true;

  cliPrintf("bReadeUseTM: %s\r\n", g_Tc2.bReadeUseTM ? "true" : "false");
}

void getRdn(cli_args_t *args) {
  (void)args;
  for(int i=0; i<64/2; i++){
	if(i%4 == 0) cliPrintf("\r\n");
	uint32_t temp = g_MbBuff.pProgram_RD_Page[i];
	uint8_t *pcTemp = (uint8_t*)&temp;
	for(int j=0; j<4; j++)
	  cliPrintf("%d ", pcTemp[j]);
  } 
}

void readn(cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nrTag  = 0; // default: no row selected!!

  if(argc > 0 && strcmp("h", argv[0]) == 0){cliPrintf("Usage: readv [nrTag:0]"); return;}
  if(argc > 0) nrTag   = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  cliPrintf("nrTag: 0x%08X\r\n", nrTag);

  // >>> 0. FT for READN <<<
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	 {0x00C0, 0xCB7F  } //FT_CLK_READV_SEL[2:0]
	,{0x0149, 0xFFF7  } //FT_TRIM_BLRD[5:0]        
  };
  ft_set_cfg(cfg_addr_data_pair_arr, NOE(cfg_addr_data_pair_arr), true);

  acntInit();

  // >>> 1. Input TAG-Register <<<
  // 1.0 deselect all rows
  for (int i = 0; i < 44; i++) Input_Row_TAG_Data[i] = 0; 

  // 1.1 set row tag
  Input_Row_TAG_Data[0] = nrTag;

  // 1.2 rtag commmand
  MB0->Membrain_IP_CMD  = ST_INPUT_ROW_TAG; // <<<<<< Row TAG Command

  // 1.3 CPU Command Done!!
  for(int i=0; i<44; i++) g_MbBuff.pInputTagRegBase[i] =  Input_Row_TAG_Data[i];

  usleep(8);   // subject to be optimization

  // >>> 2. Copy ROW-Register(DAC) Data to rreg buffer only <<<
  for(int i=0; i<324; i++) g_MbBuff.pInputRowRegBase[i] =  0xFFFFFFFF; // << for test

  // >>> 3. READN <<<
  MB0->cmd_Program_XA   = 0;
  MB0->cmd_Program_YA   = 0;
  MB0->Membrain_IP_CMD  = ST_READN_Mode;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(100);   //8ns  x 1000 = 8us

  volatile  uint32_t *pErrStatus = (volatile uint32_t*)0xA3001338;
  uint32_t nRdyCnt = pErrStatus[0];
  uint32_t nOutRegErrorNV = pErrStatus[2];
  cliPrintf("RdyCnt:%d RegErrNV(%d):%s\r\n",
			nRdyCnt, nOutRegErrorNV ,(nOutRegErrorNV ? "Fail" : "Pass"));
  if(g_Tc2.bVerbose){
	cliPrintf("-- Control               --"); showInfo("0xa3000000", 8);

	cliPrintf("-- Output for READN --"); showInfo("0xa3000918", 64/2);
  }

  // >>> 4. FT Init to 0xFFFF <<< why? not sure yet!!
  for(int i=0; i<NOE(cfg_addr_data_pair_arr); i++)
	cfg_addr_data_pair_arr[i].data = 0xFFFF;

  ft_set_cfg(cfg_addr_data_pair_arr, NOE(cfg_addr_data_pair_arr), true);

  getRdn(args);

}

void bit_interleave(cli_args_t *args) {
  (void)args;
  StVfyo_t StVfyo;
  for(int i=0; i<2048/32; i++){
	StVfyo.nVfyo[i] = g_MbBuff.pProgram_RD_Page[i];
  }

  for(int j=0; j<32; j++){
	
	if(j%2 == 0)cliPrintf("\r\n");

	uint64_t a = StVfyo.nVfyo[j+ 0];
	uint64_t b = StVfyo.nVfyo[j+32];

	uint64_t c = bitInterleave(a, b);

	cliPrintf("0x%08X ",  (uint32_t) c);
	cliPrintf("0x%08X ", (uint32_t)(c >> 32));
  
  }
}

void apgm_test(cli_args_t *args) {

  cliPrintf("=====  Anallog Program / READV Test ==== \r\n");

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc == 0){
	cliPrintf("Usage:vcg_test [#Test:0] >>> vcg6v0x6 -> vcg5v0x10\r\n");
	cliPrintf("Usage:vcg_test [#Test:1] >>> vcg6v0x6 -> vcg4v5x76\r\n");
	return;
  }

  // === for internal usage ===
  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }
  cli_args.argc = 0;

  // 1. Erase
  cliPrintf(">>> 1. Sector Erase \r\n"); on_sector_erase(&cli_args); delay(1500);

  // 2. bpgm
  cliPrintf(">>> 2. Bliso Pgm\r\n"); bliso_pgm(&cli_args); delay(1500);

  // 3. readb
  cliPrintf(">>> 3. Bliso Read\r\n"); bliso_read(&cli_args); delay(1500);

  // 4. reade
  cliPrintf(">>> 4. READE\r\n"); reade(&cli_args); delay(1500);

  // 5.0 Ananlog Program
  uint32_t selTest = 0; // default: 6v0x6, 5v0x10
  if(argc > 0) selTest = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  cliPrintf("TestCase: %d\r\n", selTest);

  if(selTest ==0 || selTest == 1){
	
	cliPrintf(">>> 5.0 Analog Program 1st\r\n"); 
	// 5.0 6v0x6 common case
	char* pcVcg       = "3";       // default : 6[v]
	char* pcNumRepeat = "6"; // x6
	char* pcxAddr     = "2"; // x6
	cli_args.argc     = 3;
	cli_args.argv[0]  = pcVcg;
	cli_args.argv[1]  = pcNumRepeat;
	cli_args.argv[1]  = pcxAddr;
	on_ana_page_program(&cli_args);
	delay(1500);
	// 5.0 5v0x10, 4v5x76

	cliPrintf(">>> 5.1 Analog Program 2nd\r\n"); 
	if(selTest == 0){
	  pcVcg       = "1";  // default : 5[v]
	  pcNumRepeat = "10"; // x6
	}else{
	  pcVcg       = "0";  // default : 4.5[v]
	  pcNumRepeat = "76"; // x6
	}
	cli_args.argv[0] = pcVcg;
	cli_args.argv[1] = pcNumRepeat;
	on_ana_page_program(&cli_args);
	delay(1500);

	cliPrintf(">>> 6 READVA Sweep\r\n"); 
	cli_args.argc = 0;
	readvas(&cli_args);
  }
  else{
	char* pcVcg       = "1"; // default : 5[v]
	//char* pcNumRepeat = "5"; // x5
	//uint32_t nNumRepeat = 5;
	//if(argc > 1) selTest = (uint16_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);

	cliPrintf(">>> 5.0 Analog Program \r\n"); 
	cli_args.argc = 3;
	cli_args.argv[0] = pcVcg;
	cli_args.argv[1] = argv[1];
	on_ana_page_program(&cli_args);
	delay(1500);

	cliPrintf(">>> 6.0 READVA Sweep\r\n"); 
	cli_args.argc = 3;
	readvas(&cli_args);
	  
  }

  free(cli_args.argv); // cleanup

}

void porb(cli_args_t *args) {
  (void)args;

  MB0->Membrain_IP_CMD   = ST_Ready;

  MB0->cmd_sw_RESET = 0;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(1000);

  MB0->cmd_sw_RESET = 1;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(1000);

  MB0->cmd_sw_RESET = 0;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(1000);

  cliPrintf("PORb\r\n");
}

void setVcg(cli_args_t *args) {

  uint32_t nVcg = 0; // default 4.5[v]
  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0) {
	if(strcmp("h", argv[0])==0){
	  cliPrintf("setVcg [0~3], default: 0 - 4.5[v]\r\n");
	  return;
	}
	else{
	  nVcg  = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	}
  }else{
	cliPrintf("using default: %d - 4.5[v]\r\n", nVcg);
  }

  g_Tc2.nVcg = nVcg % 4;
  const char *cpVCG[] = {"4.5[v]", "5[v]", "5.5[v]", "6[v]"};
  cliPrintf("VCG:%s\r\n", cpVCG[g_Tc2.nVcg]);
}

void toggleAutoBitPart(cli_args_t *args) {
  g_Tc2.bProgramType = g_Tc2.bProgramType ? false : true;
  char* pcBitPattern = g_Tc2.bProgramType ? "Auto" : "Manual";
  cliPrintf("Bit Part Mode: %s\r\n", pcBitPattern);
}

void toggleReadeType(cli_args_t *args) {
  g_Tc2.bReadeType = g_Tc2.bReadeType ? false : true;
  char* pcReadeType = g_Tc2.bReadeType ? "Bitinterleaved" : "Origin";
  cliPrintf("READE_Type: %s\r\n", pcReadeType);
}

void toggleApgmBitPattern(cli_args_t *args) {
  g_Tc2.bApgmBitPattern = g_Tc2.bApgmBitPattern ? false : true;
  char* pcBitPattern = g_Tc2.bApgmBitPattern ? "0x55555555" : "0xAAAAAAAA";
  cliPrintf("APGM BitPattern: %s\r\n", pcBitPattern);
}

void toggleRdeAddrType(cli_args_t *args) {
  g_Tc2.bReadeAddrType = g_Tc2.bReadeAddrType ? false : true;
  char* pcRdeAddrType = g_Tc2.bReadeAddrType ? "WrappAddr" : "LinearAddr";
  cliPrintf("%s\r\n", pcRdeAddrType);

  char* pcRdeType = g_Tc2.bReadeAddrType ? "BitInterleaved" : "Originaldata";
  cliPrintf("%s\r\n", pcRdeType);
}

void clear_all_flag(cli_args_t *args) {
  g_Tc2.bCE_BIAS_CE        = false;
  g_Tc2.bRecallInitialized = false;
  g_Tc2.bSetCfgInit        = false;
  g_Tc2.bSetCfgApgm        = false;
  g_Tc2.bSetCfgReade       = false;
  g_Tc2.bSetCfgReadn       = false;
  g_Tc2.bSetCfgReadv       = false;
  g_Tc2.bVerbose           = false;
}

void recall(cli_args_t *args) {

  //if(g_Tc2.bRecallInitialized) return;

  int    argc = args->argc;
  char **argv = args->argv;

  cliPrintf("Recall!!\r\n");

  // 1. initialize X0Y0-~X0Y7
  for(int i=0; i<64; i++)g_MbBuff.pProgram_RD_Page[i] = 0;

  uint32_t nNumWords = 64;
  if(argc > 0){
	nNumWords = strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	if(nNumWords < 0 || nNumWords > 256){
	  cliPrintf("nNumWords: %d\r\n", nNumWords);
	  cliPrintf("[USAGE] >recall [0 < nNumWords < 128]\r\n");
	  return;
	}
  }

  MB0->Membrain_IP_CMD   = ST_Recall;
  MB0->cmd_Program_YA    = 0;
  MB0->cmd_Redundancy_EN = NVR_HIGH; // NVR ON
  *g_MbBuff.pCpuCmdDone  = 0;

  // according to KDH, 200us for 8page reade/recall
  usleep(200); 

  if(g_Tc2.bVerbose){
	cliPrintf("--   Control   --");
	showInfo("0xa3000000", 8);
  } 

  cliPrintf("--   Output Register for READ[E/N/V]   --");
  showInfo("0xA3000918", nNumWords);

  MB0->cmd_Redundancy_EN = NVR_LOW; // NVR OFF
  // initialize 0x98 ~0xB4, why? not sure
  for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
  MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
  *g_MbBuff.pCpuCmdDone  = 0;
  usleep(2); // why? not sure yet!

  volatile uint32_t *pRecallBases[]={
	(volatile uint32_t *)0xA3000958,
	(volatile uint32_t *)0xA3000978,
	(volatile uint32_t *)0xA3000998,
	(volatile uint32_t *)0xA30009B8,
	(volatile uint32_t *)0xA30009D8,
  };
  uint32_t nCount = NOE(pRecallBases);

  for(int i=0; i<nCount; i++){
	volatile uint32_t *pnPtr = pRecallBases[i];
	for(int j=0; j<4; j++){
	  g_Tc2.sRecallArr[i][2*j+0] = pnPtr[j] >> 0;
	  g_Tc2.sRecallArr[i][2*j+1] = pnPtr[j] >> 16;
	}
  }

  if(g_Tc2.bVerbose){
	for(int i=0; i<nCount; i++){
	  for(int j=0; j<8; j++){
		uint16_t temp = g_Tc2.sRecallArr[i][j];
		cliPrintf("[%d][%d] - 0x%04X(%02d)\r\n", i,j, temp, temp);
	  }
	}
  }
  g_Tc2.bRecallInitialized = true;
}

void tc2_init(cli_args_t *args) {

  // 1. initialize X0Y-~X0Y7
  for(int i=0; i<64; i++)g_MbBuff.pProgram_RD_Page[i] = 0;

  initMembrain(MB0, &g_MbBuff);

  cliPrintf("\r\n0 ---- Init TC2 ---- \r\n");
  cliPrintf("date: %s\t time: %s\r\n", __DATE__, __TIME__);
  cliPrintf("file: %s\t function: %s\r\n", __FILE__, __func__);

  g_Tc2.eVCG               = eVCG_6v;
  //g_Tc2.eMode              = eINIT_ERASE;
  g_Tc2.bCE_BIAS_CE        = false;
  g_Tc2.bRecallInitialized = true;
  g_Tc2.bSetCfgInit        = false;
  g_Tc2.bSetCfgApgm        = false;
  g_Tc2.bSetCfgReade       = false;
  g_Tc2.bSetCfgReadn       = false;
  g_Tc2.bSetCfgReadv       = false;
  g_Tc2.bVerbose           = false;
  g_Tc2.bReadeAddrType     = true;
  g_Tc2.bReadeType         = true;
  g_Tc2.bProgramType       = true; // autobitpart

  cli_args_t cli_args;

  // 1. ce_bce
  cliPrintf("\r\n1_ce_bce\r\n");
  cli_args.argv = malloc(sizeof(char *) * 3); // 3 pointers for now!
  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }
  cli_args.argc = 1;  // row 0 
  cli_args.argv[0] = "on";
  onCE_BIAS_CE(&cli_args);
  delay(5);

  // 2. FT Cfg
  cliPrintf("\r\n2_set config for [NVR]\r\n");
  ft_cfg_init(args);
  delay(5);

  g_Tc2.bVerbose    = false;
  free(cli_args.argv); // cleanup
}

void show_status(cli_args_t *args) {

  const char *pCfgStr0 = g_Tc2.bVerbose           ? "ON" : "OFF";
  const char *pCfgStr1 = g_Tc2.bCE_BIAS_CE        ? "ON" : "OFF";
  const char *pCfgStr2 = g_Tc2.bRecallInitialized ? "ON" : "OFF";
  const char *pCfgStr3 = g_Tc2.bSetCfgInit        ? "ON" : "OFF";
  const char *pCfgStr4 = g_Tc2.bSetCfgApgm        ? "ON" : "OFF";
  const char *pCfgStr5 = g_Tc2.bSetCfgReade       ? "ON" : "OFF";
  const char *pCfgStr6 = g_Tc2.bSetCfgReadn       ? "ON" : "OFF";
  const char *pCfgStr7 = g_Tc2.bSetCfgReadv       ? "ON" : "OFF";
  const char *pcReadeType = g_Tc2.bReadeType      ? "BitInterleaving" : "Origin";
  const char *pcProgramType = g_Tc2.bProgramType  ? "AutoBitPart" : "OnlyOnetimeProgram";
  const char *pcLinearAddr = g_Tc2.bReadeAddrType ? "WrappAddr" : "LinearAddr";
  const char* pcApgmBitPattern = g_Tc2.bApgmBitPattern ? "0x55555555" : "0xAAAAAAAA";

  char *pcMode = getCurrnetMode();

  cliPrintf("MODE:\t %s\r\n",             pcMode);
  cliPrintf("nReadvFileCnt:\t %u\r\n",    g_Tc2.nReadvFileCnt);
  cliPrintf("Verbose:\t %s\r\n",          pCfgStr0);
  cliPrintf("LinearAddr:\t %s\r\n",       pcLinearAddr   );
  cliPrintf("CE_BIAS_CE:\t %s\r\n",       pCfgStr1);
  cliPrintf("RecalInitialized:\t %s\r\n", pCfgStr2);
  cliPrintf("SetCfgInit:\t %s\r\n",       pCfgStr3);
  cliPrintf("SetCfgApgm:\t %s\r\n",       pCfgStr4);
  cliPrintf("SetCfgReade:\t %s\r\n",      pCfgStr5);
  cliPrintf("SetCfgReadn:\t %s\r\n",      pCfgStr6);
  cliPrintf("SetCfgReadv:\t %s\r\n",      pCfgStr7);
  cliPrintf("ApgmBitPattern:\t %s\r\n",   pcApgmBitPattern);
  cliPrintf("ProgramType:\t %s\r\n",      pcProgramType);
  cliPrintf("ReadeType:\t %s\r\n",        pcReadeType);
  cliPrintf("Vcg:\t %d(%0.3f)\r\n"
			,g_Tc2.nVcg
			,(-0.0025*(double)g_Tc2.nVcg + 13.2375));
  cliPrintf("dbgLevel:\t %d\r\n",         g_Tc2.nDbgLevel);

  free(pcMode); // free allocated memory
}

void verbose(cli_args_t *args) {
  if(!g_Tc2.bVerbose){
	g_Tc2.bVerbose = true;
	cliPrintf("Verbose: ON\r\n");
  }else{
	g_Tc2.bVerbose = false;
	cliPrintf("Verbose: OFF\r\n");
  }
}

//readv sweep 
void readvs (cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0 && strcmp("h", argv[0])==0){cliPrintf("readvs [xAddr:0]\r\n"); return;}

  cli_args_t cli_args;

  cli_args.argc = 2;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }
  // 1. choose xAddr
  if(argc > 0){
	cli_args.argv[0] = argv[0]; //xAddr
  } else{
	cli_args.argv[0] = "0"; //default: xAddr
	cliPrintf("default: xAddr: %s\r\n", cli_args.argv[0]);
  }

  int32_t refData = 31;

  while(refData > 0){

	char cRefData[16];
	sprintf(cRefData, "%d", refData);

	cli_args.argv[1] = cRefData; // verify value
	readvb(&cli_args);
	refData -= 1;
  }
 
  free(cli_args.argv); // cleanup
}

void regres_test_all (cli_args_t *args) {

  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  cli_args.argv[0]= "0"; // xAddr
  char *pcVcg[]      = {"0", "1", "2", "3", "4", "5", "6","7", "8","9"}; // Vcg
  cli_args.argv[2]= "500"; // numRepeat

  for(int i=0; i<10; i++){
	cli_args.argv[1]= pcVcg[i];
	regression_test(&cli_args);
  }
}

void regres_test_100 (cli_args_t *args) {

  (void)args;

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nVcg       = 5;  //4.2[v] - default: (0~3) 4[v], 4.5[v], 5[v], 6[v]
  uint32_t nNumRepeat = 1; //10times - default: for 6[v], 1000 for 5[v], 4.5[v], 4[v]
  uint32_t nDly       = 2; 
  //  uint32_t nxAddr     = 0;

  if(argc == 1 && strcmp(argv[0], "h") == 0){
	cliPrintf("rtest [xAddr:0] [vcg:9(5v)] [numRepeat:10]\r\n");
	return;
  }

  //  if(argc > 0) nxAddr     = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nVcg       = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nNumRepeat = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);

  nVcg = nVcg % 10;

  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  cli_args.argc     =  1;

  // if xAddr is given with CLI command
  if(argc > 0) 
	cli_args.argv[0]  =  argv[0]; 
  else
	cli_args.argv[0]  =  "0"; //default;

  // 1. Erase
  cliPrintf(">>> 1. Sector Erase \r\n"); on_sector_erase(&cli_args); delay(nDly*1000);

  // 2. bpgm
  cliPrintf(">>> 2. Bliso Pgm\r\n"); bliso_pgm(&cli_args); delay(nDly*1000);

  // 3. readb
  cliPrintf(">>> 3. Bliso Read\r\n"); bliso_read(&cli_args); delay(nDly*1000);

  // 4. reade
  cliPrintf(">>> 4. READE\r\n"); reade(&cli_args); delay(nDly*1000);

  // 5. analog program and readv sweep
  g_Tc2.nApgmFileCnt =0;

  // for 6[v], 10 times, for 5[v],4.5[v], 4[v] 1000times
  for(int i=0; i<nNumRepeat; i++){
	char cVcg[16];
	sprintf(cVcg, "%d", nVcg);

	cli_args.argc     =  3;
	//cli_args.argv[0]  =  "3"; // 0~3 4[v], 4.5[v], 5[v], 6[v]
	cli_args.argv[0]  =  "0"; // xAddr
	cli_args.argv[1]  =  cVcg; // 0~3 4[v], 4.5[v], 5[v], 6[v]
	cli_args.argv[2]  =  "50"; // numRepeat for *ANALOG PROGRAM*

	// 1. analog program with given VCG
	on_ana_page_program(&cli_args);
	g_Tc2.nReadvFileCnt = 0;

	// 2. readv sweep 100 times 
	for(int j=0; j<100; j++){
	  cli_args.argc     =  2;
	  cli_args.argv[0]  =  "0"; // xAddr
	  cli_args.argv[1]  =  "1"; // nStride

	  // for every readv sweep, create a file, 
	  readvas(&cli_args);
	}
	// 3. at this point we have 100 files
	// now, we open the files and get an average over 5 readv sweep data
	// and save the result to a file, nVcg is used for file name
	evalReadvas100(nVcg);
	g_Tc2.nApgmFileCnt++;
  }
}

void regression_test (cli_args_t *args) {

  (void)args;

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nVcg       = 9;  //6[v] - default: (0~3) 4[v], 4.5[v], 5[v], 6[v]
  uint32_t nNumRepeat = 10; //10times - default: for 6[v], 1000 for 5[v], 4.5[v], 4[v]
  uint32_t nDly       = 2; 
  //  uint32_t nxAddr     = 0;

  if(argc == 1 && strcmp(argv[0], "h") == 0){
	cliPrintf("rtest [xAddr:0] [vcg:9(5v)] [numRepeat:10]\r\n");
	return;
  }

  //  if(argc > 0) nxAddr     = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nVcg       = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nNumRepeat = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);

  nVcg = nVcg % 10;

  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  cli_args.argc     =  1;

  // if xAddr is given with CLI command
  if(argc > 0) 
	cli_args.argv[0]  =  argv[0]; 
  else
	cli_args.argv[0]  =  "0"; //default;

  // 1. Erase
  cliPrintf(">>> 1. Sector Erase \r\n"); on_sector_erase(&cli_args); delay(nDly*1000);

  // 2. bpgm
  cliPrintf(">>> 2. Bliso Pgm\r\n"); bliso_pgm(&cli_args); delay(nDly*1000);

  // 3. readb
  cliPrintf(">>> 3. Bliso Read\r\n"); bliso_read(&cli_args); delay(nDly*1000);

  // 4. reade
  cliPrintf(">>> 4. READE\r\n"); reade(&cli_args); delay(nDly*1000);

  // 5. analog program and readv sweep
  g_Tc2.nApgmFileCnt =0;

  // for 6[v], 10 times, for 5[v],4.5[v], 4[v] 1000times
  for(int i=0; i<nNumRepeat; i++){
	char cVcg[16];
	sprintf(cVcg, "%d", nVcg);

	cli_args.argc     =  3;
	//cli_args.argv[0]  =  "3"; // 0~3 4[v], 4.5[v], 5[v], 6[v]
	cli_args.argv[0]  =  "0"; // xAddr
	cli_args.argv[1]  =  cVcg; // 0~3 4[v], 4.5[v], 5[v], 6[v]
	cli_args.argv[2]  =  "1"; // numRepeat for analog program

	// 1. analog program with given VCG
	on_ana_page_program(&cli_args);
	g_Tc2.nReadvFileCnt = 0;

	// 2. readv sweep 5 times for every analog program
	for(int j=0; j<5; j++){
	  cli_args.argc     =  2;
	  cli_args.argv[0]  =  "0"; // xAddr
	  cli_args.argv[1]  =  "1"; // nStride

	  // for every readv sweep, create a file, 
	  readvas(&cli_args);
	}
	// 3. at this point we have 5 files
	// now, we open the files and get an average over 5 readv sweep data
	// and save the result to a file, nVcg is used for file name
	evalReadvas(nVcg);
	g_Tc2.nApgmFileCnt++;
  }
}

void evalReadvas(uint32_t nVcg){

  // 1. for File Read from SD
  StReadv_t StReadv[5];
  StLvl_t StLvl = {1, {0,}, {0.,}};
  char cFilename[32];

  // 2. for File Save to SD
  //char *pFilename[]={"vcg_4v" ,"vcg_4v5" ,"vcg_5v" ,"vcg_6v"};
  char cPath[32];
  sprintf(cPath, "0:/%s.csv", pcVcg[nVcg]);

  for(int i=0; i<5; i++){

	//sprintf(cFilename, "0:/readv_%d_%d.bin", g_Tc2.nApgmFileCnt, i);
	sprintf(cFilename, "0:/readv_%d.bin", i);
	// 1. fill in the struct StReadv from file READV_0.BIN 32x2048 bits
	read_streadv_file(cFilename, &StReadv[i]); // <<  file read

	// 2. get the levels of cells 1x2048 bytes
	getLvls(&StReadv[i], StLvl.ucx, NOE(StLvl.ucx));

	// 3. average 1x2048 bytes
	getLvlAvg(&StLvl);
  }
  // 4.1 append average data to the file
  for(int i=0; i<5; i++){
	StLvl_t StLvl0 = {1, {0,}, {0.0,}};
	getLvls(&StReadv[i], StLvl0.ucx, NOE(StLvl0.ucx));

	save_ucx_to_csv(&StLvl0, cPath); // file write
  }
  
  // 4.2 append average data to the file
  save_ucX_to_csv(&StLvl, cPath); // file write

}

void evalReadvas100(uint32_t nVcg){

  // 1. for File Read from SD
  StReadv_t StReadv;
  //StReadv_t StReadv[5];
  StLvl_t StLvl = {1, {0,}, {0.,}};
  char cFilename[32];

  // 2. for File Save to SD
  //char *pFilename[]={"vcg_4v" ,"vcg_4v5" ,"vcg_5v" ,"vcg_6v"};
  char cPath[32];
  sprintf(cPath, "0:/%s.csv", pcVcg[nVcg]);

  for(int i=0; i<100; i++){

	//sprintf(cFilename, "0:/readv_%d_%d.bin", g_Tc2.nApgmFileCnt, i);
	sprintf(cFilename, "0:/readv_%d.bin", i);
	// 1. fill in the struct StReadv from file READV_0.BIN 32x2048 bits
	read_streadv_file(cFilename, &StReadv); // <<  file read

	// 2. get the levels of cells 1x2048 bytes
	getLvls(&StReadv, StLvl.ucx, NOE(StLvl.ucx));

	// 3. average 1x2048 bytes
	save_ucx_to_csv(&StLvl, cPath); // file write
  }
}

#define MB_NUM_COLS (2048U)

void getLevels (cli_args_t *args) {

  StReadv_t StReadv;
  uint8_t ucLvlArr[MB_NUM_COLS] = {0,};
  
  readvas(args);

  memcpy(&StReadv, &g_Tc2.StReadv, sizeof(StReadv_t));

  getLvls(&StReadv, &ucLvlArr[0], MB_NUM_COLS);

  for(int i=0; i<2048/32; i++){
	for(int j=0; j<32; j++){
	  bool bIsLast = ((i == (2048/3-1)) && (j == (32-1)));
	  if(!bIsLast)
		cliPrintf("%d, ", ucLvlArr[32*i+j]);
	  else
		cliPrintf("%d"  , ucLvlArr[32*i+j]);
	}
  }
}

// READV_A Sweep
void readvas (cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nRmpUp  = 0xFE1F;
  uint32_t nStride = 1;

  if(argc > 0 && (strcmp("h", argv[0])==0)){
	cliPrintf("readvas [xAddr:0] [nStride:1~4] [rmpup:0xFE1F(0xFE0F, x0FE4F, 0xFE8F,0xFE1F)]\r\n");
	return;
  }
  if(argc > 1) nStride = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nRmpUp  = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);

  nStride = nStride % 5;

  cli_args_t cli_args;

  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  cli_args.argv[0] = "0"; //default: xAddr
  if(argc > 0) cli_args.argv[0] = argv[0];
  char pcRmpUp[16];
  sprintf(pcRmpUp, "%d", nRmpUp);
  cli_args.argv[2] = pcRmpUp;

  int32_t refData = 39;
  int32_t nIndex  =  0;

  StReadv_t StReadv;
  initStReadv(&StReadv);

  cliPrintf("xAddr:%s, nStride: %d\r\n", cli_args.argv[0], nStride);

  //while(refData >= 0){
  while(refData > (8-1)){

	char cRefData[16];// verify value
	sprintf(cRefData, "%d", refData);

	// <<<<<<<<<  1. readv Operation >>>>>>>>>>//
	cli_args.argv[1] = cRefData; 
	readva(&cli_args);

	// <<<<<<<<<  2. fetch Vfyo from DPRAM for File Operation >>>>>>>>>>//
	StReadv.StVfyo[nIndex % 32].nRef = refData;
	for(int i=0; i<2048/32; i++) StReadv.StVfyo[nIndex % 32].nVfyo[i] = g_MbBuff.pProgram_RD_Page[i];

	refData -= nStride;
	nIndex++;
  }

  // <<<<<<<<<  3.0 save rdvs data to a global variable/sd card >>>>>>>>>>//
  memcpy(&g_Tc2.StReadv, &StReadv, sizeof(StReadv_t));

  // <<<<<<<<<  3.1 save data to SD card >>>>>>>>>>//
  //if(nStride == 1) saveReadv(&StReadv);
 
  free(cli_args.argv); // cleanup
}

void onCE_BIAS_CE(cli_args_t *args) {

  initMembrain(MB0, &g_MbBuff);

  int    argc = args->argc;
  char **argv = args->argv;

  // in case, no argument given!!
  if(argc < 1 || strcmp("help", argv[0]) == 0){
	cliPrintf("usage: ce [on:enable, off:disable]\r\n");
	return;
  }
  else if(strcmp("on", argv[0]) == 0){
	//	if(g_Tc2.bCE_BIAS_CE){
	//	  cliPrintf("TC2 is already up\r\n");
	//	  return;
	//	}else{
	MB0->cmd_CE_Control  = 1;
	MB0->cmd_IP_BIAS     = 3;
	MB0->Membrain_IP_CMD = ST_IO_MUX_Write;
	*g_MbBuff.pCpuCmdDone = 0;

	g_Tc2.bCE_BIAS_CE = true;
	g_Tc2.eMode       = eINIT_ERASE;
	cliPrintf("TC2 is now up\r\n");
	//}
  }
  else if(strcmp("off", argv[0]) == 0){
	//	if(!g_Tc2.bCE_BIAS_CE){
	//	  cliPrintf("TC2 is already down\r\n");
	//	  return;
	//	}
	//	else{
	MB0->cmd_CE_Control   = 6;
	MB0->cmd_IP_BIAS      = 0;
	MB0->Membrain_IP_CMD  = ST_IO_MUX_Write;
	*g_MbBuff.pCpuCmdDone = 0;

	g_Tc2.bCE_BIAS_CE = false;
	cliPrintf("TC2 is now down\r\n");
	cliPrintf("%s\r\n", argv[0]);
	//}
  }
  usleep(40);// 5000 clk x 8ns = 40 us for tCS (CE setup time to READ MODE)

  if(g_Tc2.bVerbose){cliPrintf("--   Control   --"); showInfo("0xa3000000", 8);}
}

void ft_default(cli_args_t *args){

  // 1. FT cfg addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000, 0xFFFF  } //FT_ERS_HV[4:0]
	,{0x0001, 0xFFFF  } //FT_VSL[4:0], FT_ENBIAS_PRGDIG
	,{0x0002, 0xFFFF  } //FT_UARANGE_EN
	,{0x0006, 0xFFFF  } //FT_PRGDIG[3:0]

	,{0x0008, 0xFFFF  } //FT_VEGTRIM[10:0]
	,{0x0009, 0xFFFF  } //FT_IDP[5:0]
	,{0x0021, 0xFFFF  } //FT_ENIP_PRGDIG
	,{0x008B, 0xFFFF  } //FT_OSC_200MHZ[4:0]

	,{0x00C0, 0xFFFF  } //FT_CLK_READV_SEL FT_I2VREFADC[8:0]
	,{0x00C7, 0xFFFF  } //FT_CGRH_SHIFT[5:0] FT_DLL_BYPASS, FT_SEL_CLK_CITV
	,{0x014A, 0xFFFF  } //FT_VREFH_1P4V_MV

	,{0x014B, 0xFFFF  } //FT_DC_RANGE[3:0]+FT_TRIM_DC[5:0]
	,{0x0170, 0xFFFF  } //FT_DAC_DLL_BYPASS
	,{0x0172, 0xFFFF  } //FT_IREFTRIM[3:0] FT_STEP[0] + FT_RANGE[2:0]
	,{0x0173, 0xFFFF  } //FT_STEP[2:1]
  };
  ft_set_cfg(cfg_addr_data_pair_arr, NOE(cfg_addr_data_pair_arr), true);
}

void ft_dump(cli_args_t *args) {

  // 1. FT cfg addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={

	{0x0000, 0xFFFF  } //FT_ERS_HV[4:0]
	,{0x0001, 0xFFFF  } //FT_VSL[4:0], FT_ENBIAS_PRGDIG
	,{0x0002, 0xFFFF  } //FT_UARANGE_EN
	,{0x0003, 0xFFFF} //FT_UARANGE <<<<<<<<<<<<<<<<<<<<<<<<

	,{0x0006, 0xFFFF  } //FT_PRGDIG[3:0]
	,{0x0007, 0xFFFF  } //FT_PRGDIG[3:0]
	,{0x0008, 0xFFFF  } //FT_VEGTRIM[10:0]
	,{0x0009, 0xFFFF  } //FT_IDP[5:0]

	,{0x0011, 0xFFFF  } //FT_ENIP_PRGDIG
	,{0x0021, 0xFFFF  } //FT_ENIP_PRGDIG
	,{0x008B, 0xFFFF  } //FT_OSC_200MHZ[4:0]
	,{0x00C0, 0xFFFF  } //FT_CLK_READV_SEL FT_I2VREFADC[8:0]

	,{0x00C3, 0xFFFF} // FT_USE_IDAC <<<<<<<<<<<<<<<<<<<<<<
	,{0x00C7, 0xFFFF  } //FT_CGRH_SHIFT[5:0] FT_DLL_BYPASS, FT_SEL_CLK_CITV
	,{0x0149, 0xFFFF  } //FT_VREFH_1P4V_MV
	,{0x014A, 0xFFFF  } //FT_VREFH_1P4V_MV
  };

  printf("----------------------------\r\n");
  printf("Addr\t Data\r\n");
  ft_read_cfg(cfg_addr_data_pair_arr, NOE(cfg_addr_data_pair_arr));
  cfg_addr_data_pair_t cfg_addr_data_pair_arr1[]={
	{0x014B, 0xFFFF  } //FT_DC_RANGE[3:0]+FT_TRIM_DC[5:0]
	,{0x0170, 0xFFFF  } //FT_DAC_DLL_BYPASS
	,{0x0172, 0xFFFF  } //FT_IREFTRIM[3:0] FT_STEP[0] + FT_RANGE[2:0]
	,{0x0173, 0xFFFF  } //FT_STEP[2:1]
  };
  ft_read_cfg(cfg_addr_data_pair_arr1, NOE(cfg_addr_data_pair_arr1));
  printf("----------------------------\r\n");
}

static void ft_read_cfg(cfg_addr_data_pair_t cfg_addr_data_pair_arr[], uint32_t nNumElements){

  uint32_t nCfg_Data_Number = nNumElements;

  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
  }

  //3. run ft set cfg command
  MB0->Membrain_IP_CMD		= ST_Read_Config;
  MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;
  *g_MbBuff.pCpuCmdDone     = 0; 
  //  delay(1); //must, but maybe, need some optimizations?
  usleep(10);

  for(int i=0; i<nCfg_Data_Number; i++){
	uint32_t temp = MB0->Read_Cfg_Data[i];
	printf("0x%08x\t 0x%08x\r\n", cfg_addr_data_pair_arr[i].addr, temp);
  }
}

static void ft_set_cfg(cfg_addr_data_pair_t cfg_addr_data_pair_arr[], uint32_t nNumElements, bool bChkReadback){

  uint32_t nCfg_Data_Number = nNumElements;

  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
  }

  //3. run ft set cfg command
  MB0->Membrain_IP_CMD		= ST_Set_Config;
  MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;
  *g_MbBuff.pCpuCmdDone     = 0; 
  usleep(10);

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_init\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }

  if(bChkReadback){
	
	//4. run ft set cfg command
	MB0->Membrain_IP_CMD		= ST_Read_Config;
	MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;
	*g_MbBuff.pCpuCmdDone     = 0; 

	usleep(10);
	bool bCfgCheck = true;
	for(int i=0; i<nCfg_Data_Number; i++ ){
	  uint32_t cfgWd = g_MbBuff.pSet_Cfg_Data[i];
	  uint32_t cfgRd = g_MbBuff.pRead_Cfg_Data[i];
	  if(cfgWd != cfgRd){
		bCfgCheck = false;
		break;
	  }
	}
	cliPrintf("%d Registers: \tFT Cfg Data Check: %s\r\n", nNumElements, (bCfgCheck) ? "PASS" : "FAIL");
  }
}

void ft_cfg_init(cli_args_t *args){

  g_Tc2.bSetCfgInit = true;

  // 1. FT cfg init addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000, 0xD3FF } //FT_ERS_HV[4:0]
	,{0x0001, 0xEF97 } //FT_VSL[4:0]
	,{0x0006, 0xFFE3 } //FT_PRGDIG_HV[3:0]
	,{0x0009, 0xFFD7 } //FT_IDP[5:0]

	,{0x0011, 0xFF97 } //FT_VSL_2[4:0]
	,{0x0021, 0xFEFF } //FT_ENIP_PRGDIG
	,{0x00C0, 0xFB7F } //FT_I2VREFADC[3:0]
	,{0x00C4, 0xF87F } //FT_YATD[5:0]

	,{0x00C5, 0xFFEF } //FT_HALF_CAP
	,{0x00C6, 0xFFF9 } //FT_IRANGE[2:0]
	,{0x00C7, 0x7FFD } //FT_DLL_BYPASS&FT_SEL_CLK_CITV&FT_DIS_BLREG
	,{0x014B, 0xABF6 } //FT_DC_RANGE[3:0]+FT_TRIM_DC[5:0]

	,{0x0170, 0xFDFF } //FT_DLL_BYPASS
	,{0x0172, 0xECFF } //FT_STEP[0] + FT_RANGE[2:0]
	,{0x0173, 0xFFFC } //FT_STEP[2:1]
  };

  //2. Write cfg data to dual port memory
  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  ft_set_cfg(cfg_addr_data_pair_arr, nCfg_Data_Number, true);

  usleep(10);

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_init\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }
}

static void ft_cfg_readva_entry(uint32_t nRmpUp){

  // 1. FT cfg init addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x008B,  0xFFC8} //FT_OSC_200MHz[4:0] FT_OSC_200MHZ_EN
	,{0x00C6,  0xFE09} //FT_RMP_UP[4:0]-EVEN FT_RMP_UP[4:0]-ODD
	,{0x0149,  0xFFF7} //FT_TRIM_BLRD[5:0]
  };

  cfg_addr_data_pair_arr[1].data = nRmpUp;

  //2. Write cfg data to dual port memory
  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  ft_set_cfg(cfg_addr_data_pair_arr, nCfg_Data_Number, true);

  usleep(10);

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_init\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }
}

static void ft_cfg_readva_exit(){

  // 1. FT cfg init addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x008B,  0xFFFF} //FT_OSC_200MHz[4:0] FT_OSC_200MHZ_EN
	,{0x00C6,  0xFFFF} //FT_RMP_UP[4:0]-EVEN FT_RMP_UP[4:0]-ODD
	,{0x0149,  0xFFFF} //FT_TRIM_BLRD[5:0]
  };

  //2. Write cfg data to dual port memory
  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  ft_set_cfg(cfg_addr_data_pair_arr, nCfg_Data_Number, true);

  usleep(10);

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_init\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }
}

void ft_cfg_readva(cli_args_t *args){

  // 1. FT cfg init addr/data array
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x008B,  0xFFC4} //FT_OSC_200MHz[4:0] FT_OSC_200MHZ_EN
	,{0x00C6,  0xFE0F} //FT_RMP_UP[4:0]-EVEN FT_RMP_UP[4:0]-ODD
	,{0x0149,  0xFFF5} //FT_TRIM_BLRD[5:0]
  };

  //2. Write cfg data to dual port memory
  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  ft_set_cfg(cfg_addr_data_pair_arr, nCfg_Data_Number, true);

  usleep(10);

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_init\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }
}

// temporary, let's give it a try!!
void ft_cfg_bliso(){

  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x007,   0xFFFA  } //FT_TRIM1UA[3:0], FT_TRIM1NA[4:0]
	,{0x006,   0x9CE3  } //FT_IDAC[7:0] from Test mode and array pdf, pg: 181, Hmmm~~
	//,{0x0C3,   0xFFDF  } // FT_USE_IDAC
	,{0x0C3,   0xFFFF  } // FT_USE_IDAC
	,{0x003,   0xFFBF  } // FT_UARANGE_EN
	,{0x0C7,   0xFFFD  } // FT_DLL_BYPASS
	,{0x170,   0xFFFF  } // FT_DAC_DLL_BYPASS
  };

  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
  }

  MB0->Membrain_IP_CMD		= ST_Set_Config;
  MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;
  *g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
  usleep(10);
}

void erase_all_sectors(cli_args_t *args) {

  cli_args_t cli_args;

  cli_args.argc = 1;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  for(int xAddr = 0; xAddr<1280/2; xAddr++){
	char cxAddr[16];
	sprintf(cxAddr, "%d", 2*xAddr);

	cli_args.argv[0] = cxAddr; // verify value
	on_sector_erase(&cli_args);
  }
 
  free(cli_args.argv); // cleanup

}

void on_sector_erase(cli_args_t *args) {

  if(g_Tc2.eMode != eINIT_ERASE){
	cliPrintf("[SERASE] eINIT_ERAS\r\n");
	g_Tc2.eMode = eINIT_ERASE;
  }
  ft_cfg_erase(eWriteThrouh);

  unsigned int nSectAddr   = 0;    //default
  unsigned int nNumToErase = 1; //default
  const unsigned int nEraseTime = 1500; //default

  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0 && (strcmp(argv[0], "h") == 0)){
	cliPrintf("uage: erase [SecNumber:0] [NumToErase:1] [RddncyAddCode:0(main array)]\r\n");
	return;
  }

  char *pcRddncy[3] = {"ARRDN", "ARRCOL", "NVR"};
  uint32_t nRddncyCode = 0;

  bool bOnRddncyAccess = false;
  if(argc > 2){
	if(strcmp(argv[2], "ARRDN" ) == 0 ) {nRddncyCode = 0;MB0->cmd_Redundancy_EN = RDDNCY_ARRDN; bOnRddncyAccess = true;}
	if(strcmp(argv[2], "ARRCOL") == 0 ) {nRddncyCode = 1;MB0->cmd_Redundancy_EN = RDDNCY_ARRCOL;bOnRddncyAccess = true;}
	if(strcmp(argv[2], "NVR"   ) == 0 ) {nRddncyCode = 2;MB0->cmd_Redundancy_EN = RDDNCY_NVR;   bOnRddncyAccess = true;}
	if(!bOnRddncyAccess) {cliPrintf("[ERROR] RdncyAcc Code Error: %s\r\n", argv[2]); return;}

	if(bOnRddncyAccess){
	  cliPrintf("Sector Erase Access: %s\r\n", pcRddncy[nRddncyCode]);
	  // initialize 0x98 ~0xB4, why? not sure
	  for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	  MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	  *g_MbBuff.pCpuCmdDone  = 0;
	  usleep(2); // why? not sure yet!
	}
  }

  if(argc > 1) {
	nNumToErase  = strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
	nSectAddr    = strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	printf("SectAddr: %d, NumToErase: %d\r\n", nSectAddr, nNumToErase);
  }else if(argc > 0) {
	nSectAddr  = strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	printf("SecNumber: %d, NumToErase: %d\r\n", nSectAddr, nNumToErase);
  }else{
	cliPrintf("Using Default: SecNumber:%d NumToErase:%d]\r\n", nSectAddr, nNumToErase);
  }


  if((nSectAddr % 2) != 0){
	printf("EraseSecNumber must be a multiple of 2\r\n");
	printf("nSectaddr: %d, NumToErase: %d\r\n", nSectAddr, nNumToErase);
	return;
  }

  for(int i=0; i<nNumToErase; i++){
	MB0->Membrain_IP_CMD       = ST_Sector_Erase;
	MB0->cmd_Erase_Sector_Addr = nSectAddr + 2*i;
	MB0->cmd_Erase_time        = nEraseTime;
	*g_MbBuff.pCpuCmdDone      = 0;
	delay(4); // 1ms, why? not sure yet!!
  }

  if(g_Tc2.bVerbose){cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);} 

  if(bOnRddncyAccess){
	MB0->cmd_Redundancy_EN = RDDNCY_NONE; // Reset to 0
	// initialize 0x98 ~0xB4, why? not sure
	for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	*g_MbBuff.pCpuCmdDone  = 0;
	usleep(2); // why? not sure yet!
  }
}

void bliso_pgm(cli_args_t *args) {

  (void)args;
  if(g_Tc2.eMode != eBPGM){
	cliPrintf("BPGM Entry\r\n");
	g_Tc2.eMode = eBPGM;
  }
  //ft_cfg_dpgm(eWriteThrouh);
  ft_cfg_bpgm(eWriteThrouh);

  // 1.1 TM Entry
  MB0->cmd_IP_TEST      = 1;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  // 1.2 TM cfg
  cfg_addr_data_pair_t cfg_addr_data_pair= {0x00E, 0x0200};// tm_sel_bliso
  MB0->cmd_Cfg_Data_Number  = 1;
  g_MbBuff.pSet_Cfg_Addr[0] = cfg_addr_data_pair.addr;
  g_MbBuff.pSet_Cfg_Data[0] = cfg_addr_data_pair.data;
  MB0->Membrain_IP_CMD      = ST_Set_Config;
  *g_MbBuff.pCpuCmdDone     = 0;
  usleep(50);

  // 2. BLISO program
  // 2.1 data && parameters backup previous state
  uint32_t cmd_Output_Signal      = MB0->cmd_Output_Signal;
  uint32_t cmd_Program_READE_Type = MB0->cmd_Program_READE_Type;

#if 0
  MB0->cmd_Output_Signal      = LinearAddr << 2;
  MB0->cmd_Program_READE_Type = OnlyOneTime | OriginalData << 1;
#else
#endif

  MB0->cmd_max_Word_Num = 8;
  MB0->cmd_max_Page_Num = 8;
  MB0->cmd_Program_YA   = 0;
  MB0->cmd_Program_time = 900; // 9uSec[10ns]

  // bliso data: all 0's
  MB0->Membrain_IP_CMD  = ST_Ready;// we use cpuCmdDone
  for(int i=0; i<64; i++) g_MbBuff.pProgram_WD_Page[i] =  0;

  // 2.2  page_program
  for(int i=0; i<1280/8; i++){
	// BLISO PGM
	// 1 Block = 8 Pgaes
	for(int j=0; j<8; j++){
	  MB0->cmd_Program_XA   = 8*i + j;
	  MB0->Membrain_IP_CMD  = ST_Page_Program;
	  *g_MbBuff.pCpuCmdDone = 0; 
	  usleep(400); // 80 us x 5 = 400 us
	}
  }

  // 1.2 TM cfg initialze as 0 according to Testmodes and Array~~.pdf Page.27
  cfg_addr_data_pair.data= 0x0;// tm_sel_bliso
  MB0->cmd_Cfg_Data_Number  = 1;
  g_MbBuff.pSet_Cfg_Addr[0] = cfg_addr_data_pair.addr;
  g_MbBuff.pSet_Cfg_Data[0] = cfg_addr_data_pair.data;
  MB0->Membrain_IP_CMD      = ST_Set_Config;
  *g_MbBuff.pCpuCmdDone     = 0;
  usleep(50);
 
  // 5. TM Exit
  MB0->cmd_IP_TEST      = 0;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  // recover previous state
  MB0->cmd_Output_Signal      = cmd_Output_Signal;
  MB0->cmd_Program_READE_Type = cmd_Program_READE_Type;
}

void bliso_read(cli_args_t *args) {

  if(g_Tc2.eMode != eREADB){
	cliPrintf("READB Entry\r\n");
	g_Tc2.eMode = eREADB;
	// check the following later!!
  }
  ft_cfg_bliso();

  uint32_t nxAddr = 0;
  int    argc     = args->argc;
  char **argv     = args->argv;
  if(argc > 0) {
	nxAddr = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	cliPrintf("Got nxAddr: %d\r\n", nxAddr);
  }else{
	cliPrintf("Using Default nxAddr: %d\r\n", nxAddr);
  }

  // 1.1 TM Entry
  MB0->cmd_IP_TEST      = 1;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(10);

  // 1.2 TM cfg
  cfg_addr_data_pair_t cfg_addr_data_pair= {0x00E, 0x0200};// tm_sel_bliso
  MB0->cmd_Cfg_Data_Number = 1;
  g_MbBuff.pSet_Cfg_Addr[0] = cfg_addr_data_pair.addr;
  g_MbBuff.pSet_Cfg_Data[0] = cfg_addr_data_pair.data;

  MB0->Membrain_IP_CMD = ST_Set_Config;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  uint32_t cmd_Output_Signal      = MB0->cmd_Output_Signal;
  uint32_t cmd_Program_READE_Type = MB0->cmd_Program_READE_Type;

  if(g_Tc2.bReadeAddrType){
	//	MB0->cmd_Output_Signal      = LinearAddr << 2;
	MB0->cmd_Output_Signal      = READE_ADDR_TYPE_WRAP;
	MB0->cmd_Program_READE_Type = OnlyOneTime |RDE_TYPE_BINT ;
	
  }else{
	MB0->cmd_Output_Signal      = READE_ADDR_TYPE_LINEAR;
	//	MB0->cmd_Program_READE_Type = 2;
	MB0->cmd_Program_READE_Type = RDE_TYPE_ORIG;
  }

  MB0->cmd_max_Word_Num = 8;
  MB0->cmd_max_Page_Num = 8;
  MB0->cmd_Program_YA   = 0;

  // 2. READE
  MB0->cmd_Program_XA  = nxAddr;		// Page : 0~7
  MB0->Membrain_IP_CMD = ST_READE; // READE
  *g_MbBuff.pCpuCmdDone = 0; //consider to use later!! Hmmm!!
  usleep(80);		//8ns x 10000 = 80 us
  cliPrintf("-- Output Reg  for READE --"); showInfo("0xa3000918", 64);

  // 1.2 TM cfg initialze as 0 according to Testmodes and Array~~.pdf Page.27
  cfg_addr_data_pair.data = 0x0;// tm_sel_bliso
  MB0->cmd_Cfg_Data_Number = 1;
  g_MbBuff.pSet_Cfg_Addr[0] = cfg_addr_data_pair.addr;
  g_MbBuff.pSet_Cfg_Data[0] = cfg_addr_data_pair.data;

  MB0->Membrain_IP_CMD = ST_Set_Config;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  // 3. TM Exit
  MB0->cmd_IP_TEST      = 0;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  // recover previous state
  MB0->cmd_Output_Signal      = cmd_Output_Signal;
  MB0->cmd_Program_READE_Type = cmd_Program_READE_Type;
}

void reade(cli_args_t *args) {
  char *pcFT_IDAC[]={
	"0xE0(3.2[uA])"
	,"0xC0(6.4[uA])"
	,"0xA0(9.6[uA])"
	,"0x90(11.2[uA])"
	,"0x80(12.8[uA])"
  };
  if(g_Tc2.eMode != eREADE){cliPrintf("eREADE Entry\r\n"); g_Tc2.eMode = eREADE;}

  uint32_t nxAddr   = 0;
  uint32_t nFT_IDAC = 0xE0; // 3.2uA
  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("Usage: reade [xAddr:0] [FT_IDAC:0x%02X] [ARRDN/ARRCOL/NVR]\r\n", nFT_IDAC);
	for(int i=0; i<5; i++) cliPrintf("%s ", pcFT_IDAC[i]);
	return;
  }
  char *pcRddncy[3] = {"ARRDN", "ARRCOL", "NVR"};
  uint32_t nRddncyCode = 0;

  bool bOnRddncyAccess = false;
  if(argc > 2){
	if(strcmp(argv[2], "ARRDN" ) == 0 ) {nRddncyCode = 0;MB0->cmd_Redundancy_EN = RDDNCY_ARRDN; bOnRddncyAccess = true;}
	else if(strcmp(argv[2], "ARRCOL") == 0 ) {nRddncyCode = 1;MB0->cmd_Redundancy_EN = RDDNCY_ARRCOL;bOnRddncyAccess = true;}
	else if(strcmp(argv[2], "NVR"   ) == 0 ) {nRddncyCode = 2;MB0->cmd_Redundancy_EN = RDDNCY_NVR;   bOnRddncyAccess = true;}
	if(!bOnRddncyAccess) {cliPrintf("[ERROR] RdncyAcc Code Error: %s\r\n", argv[2]); return;}
	if(bOnRddncyAccess){
	  // initialize 0x98 ~0xB4, why? not sure
	  for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	  MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	  *g_MbBuff.pCpuCmdDone  = 0;
	  usleep(2); // why? not sure yet!
	}
  }

  if(argc > 1) {
	nFT_IDAC = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
	switch(nFT_IDAC){
	case 0xE0:
	case 0xC0:
	case 0xA0:
	case 0x90:
	case 0x80: cliPrintf("Got nFT_IDAC: 0x%X\r\n", nFT_IDAC);        break;
	default:   cliPrintf("[ERROR]Got nFT_IDAC: 0x%X\r\n", nFT_IDAC); return;
	}
  }

  if(argc > 0) {
	nxAddr = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	cliPrintf("Got nxAddr: %d\r\n", nxAddr);
  }else{
	cliPrintf("Using Default nxAddr: %d, nFT_IDAC:0x00%02X\r\n", nxAddr, nFT_IDAC);
  }

  acntInit();

  ft_cfg_reade(eWriteThrouh, nFT_IDAC);

  if(g_Tc2.bReadeUseTM) tm_cfg_reade(true); // TM Entry

  MB0->cmd_Program_XA         = nxAddr;	// default : 0
  MB0->cmd_Program_YA         = 0;		// Page : 0~7
  if(g_Tc2.bReadeAddrType){
	MB0->cmd_Output_Signal      = READE_ADDR_TYPE_WRAP;	
	MB0->cmd_Program_READE_Type = RDE_TYPE_BINT;
  }else{
	MB0->cmd_Output_Signal      = READE_ADDR_TYPE_LINEAR;	
	MB0->cmd_Program_READE_Type = RDE_TYPE_ORIG;
  }

  MB0->Membrain_IP_CMD        = ST_READE; 
  *g_MbBuff.pCpuCmdDone       = 0;      
  usleep(80);		//8ns x 10000 = 80 us

  volatile  uint32_t *pErrStatus = (volatile uint32_t*)0xA3001338;
  uint32_t nRdyCnt = pErrStatus[0];
  uint32_t nOutRegErrorNV = pErrStatus[2];
  cliPrintf("RdyCnt:%d RegErrNV(%d):%s\r\n",
			nRdyCnt, nOutRegErrorNV ,(nOutRegErrorNV ? "Fail" : "Pass"));
  cliPrintf("-- Control               --"); showInfo("0xa3000000", 8);

  if(bOnRddncyAccess && nRddncyCode == 1){
	cliPrintf("-- Output for RDDNCY:%s --", pcRddncy[nRddncyCode%3]); showInfo("0xa3001218", 64);
  } else if(bOnRddncyAccess){
	cliPrintf("-- Output for READE:%s --", pcRddncy[nRddncyCode%3]); showInfo("0xa3000918", 64);
  }else{
	cliPrintf("-- Output for READE --"); showInfo("0xa3000918", 64);
  }
  if(g_Tc2.bReadeUseTM) tm_cfg_reade(false); // TM Exit

  if(bOnRddncyAccess){
	MB0->cmd_Redundancy_EN = RDDNCY_NONE; // Reset to 0
	// initialize 0x98 ~0xB4, why? not sure
	for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	*g_MbBuff.pCpuCmdDone  = 0;
	usleep(2); // why? not sure yet!
  }
}

void dpgm_all (cli_args_t *args) {

  cli_args_t cli_args;

  cli_args.argc = 1;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }

  for(int xAddr = 0; xAddr<1280; xAddr++){
	char cxAddr[16];
	sprintf(cxAddr, "%d", xAddr);

	cli_args.argv[0] = cxAddr; // verify value
	on_dig_page_program(&cli_args);
  }
 
  free(cli_args.argv); // cleanup
}

void on_dig_page_program (cli_args_t *args) {

  if(g_Tc2.eMode != eDPGM){
	cliPrintf("DPGM INIT Entry\r\n");
	g_Tc2.eMode = eDPGM;
  }

  uint32_t nxAddr = 0;
  int    argc = args->argc;
  char **argv = args->argv;

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("dpgm [xAddr:0] [ARRDN/ARRCOL/NVR]");
	return;
  }

  if(argc == 1) {
	nxAddr = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
	cliPrintf("Got nxAddr: %d\r\n", nxAddr);
  }else{
	cliPrintf("Using Default nxAddr: %d\r\n", nxAddr);
  }
  char *pcRddncy[3] = {"ARRDN", "ARRCOL", "NVR"};
  uint32_t nRddncyCode = 0;

  bool bOnRddncyAccess = false;
  if(argc > 1){
	if(strcmp(argv[1], "ARRDN" ) == 0 ) {nRddncyCode = 0;MB0->cmd_Redundancy_EN = RDDNCY_ARRDN; bOnRddncyAccess = true;}
	if(strcmp(argv[1], "ARRCOL") == 0 ) {nRddncyCode = 1;MB0->cmd_Redundancy_EN = RDDNCY_ARRCOL;bOnRddncyAccess = true;}
	if(strcmp(argv[1], "NVR"   ) == 0 ) {nRddncyCode = 2;MB0->cmd_Redundancy_EN = RDDNCY_NVR; bOnRddncyAccess = true;}
	if(!bOnRddncyAccess) {cliPrintf("[ERROR] RdncyAcc Code Error: %s\r\n", argv[1]); return;}
	if(bOnRddncyAccess){
	  // initialize 0x98 ~0xB4, why? not sure
	  for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	  MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	  *g_MbBuff.pCpuCmdDone  = 0;
	  usleep(2); // why? not sure yet!
	}
  }

  // 0. ft cfg
  ft_cfg_dpgm(eWriteBack);

  // 1. prepare data to write
  uint32_t pageData[64] = {0, }; //32 x 64 = 2^5 * 2^6 = 2^11 = 2048BL

  // 2. set parameters
  MB0->cmd_max_Word_Num = 8;
  MB0->cmd_max_Page_Num = 8;
  MB0->cmd_Program_XA   = nxAddr;
  MB0->cmd_Program_YA   = 0;

  //MB0->cmd_Program_READE_Type = 2;
  // bit[1] : 1 (interleaving) bit[0] : 0 (auto partitioning[default])
  MB0->cmd_Program_READE_Type =
	g_Tc2.bProgramType ? PGM_TYPE_ABP : PGM_TYPE_OOTP;

  MB0->cmd_membrain_ip_sel    = IP_Mode_TC2b; // bit[0] : 0 New Version mode
  MB0->Membrain_IP_CMD        = ST_Page_Program;

  for(int i=0; i<64; i++) {
	if(bOnRddncyAccess && nRddncyCode == 1) // ARRCOL
	  g_MbBuff.pArrcolBase[i] = pageData[i];
	else
	  g_MbBuff.pProgram_WD_Page[i] = pageData[i]; // 3. fast mode, cpuCmdDone(NVR, MainArr)
  }

  if(bOnRddncyAccess && nRddncyCode != 2) *g_MbBuff.pCpuCmdDone  = 0;

  usleep(400); // 80 us x 5 = 400 us

  //if(g_Tc2.bVerbose){
  cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
  cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
  cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
  if(bOnRddncyAccess && nRddncyCode != 2){
	cliPrintf("-- Pgm Data:%s --", pcRddncy[nRddncyCode%3]); showInfo("0xa3001118", 16);
  } else if(bOnRddncyAccess){
	cliPrintf("-- Pgm Data:%s --", pcRddncy[nRddncyCode%3]); showInfo("0xa3000118", 16);
  }else{
	cliPrintf("-- Pgm Data --"); showInfo("0xa3000118", 16);
  }
  //}

  if(bOnRddncyAccess){
	MB0->cmd_Redundancy_EN = RDDNCY_NONE; // Reset to 0
	// initialize 0x98 ~0xB4, why? not sure
	for(int i=0; i<8; i++) g_MbBuff.pSet_Cfg_Data[i] = 0;
	MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
	*g_MbBuff.pCpuCmdDone  = 0;
	usleep(2); // why? not sure yet!
  }

}

void on_ana_page_program(cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nNumRepeat = 1;
  uint32_t nxAddr     = 0;
  uint32_t nVcg       = 0xDEF; // 4.32[v]

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("apgm [xAddr:%d] [vcg:0x%03X - 0xFFF(3[v])~0x50F(10[v])] [NumRepeat:1]",
			  nxAddr, nVcg, nNumRepeat);
	return;
  }

  if(g_Tc2.eMode != eAPGM){cliPrintf("eAPGM Entry\r\n"); g_Tc2.eMode = eAPGM;}


  if(argc > 0) nxAddr     = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nVcg       = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nNumRepeat = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);

  g_Tc2.nVcg = nVcg & 0xFFF;// for now, we use a global variable for ft_cfg_apgm

  // 1. ft cfg
  ft_cfg_apgm(eWriteBack); // KMC version
  //printf("nxAddr: %d sVCG: %s nNumRepeat: %d\r\n", nxAddr, pcVcg[g_Tc2.nVcg], nNumRepeat);
  printf("nxAddr:%d g_Tc2.nVcg:0x%04X(%0.3f) nNumRepeat:%d\r\n"
		 ,nxAddr
		 ,g_Tc2.nVcg
		 ,(-0.0025*(double)g_Tc2.nVcg + 13.2375)
		 ,nNumRepeat);

  // 2. prepare pageData
  uint32_t pageData[64] = {0,};   //select all bit line for now
  memcpy(&pageData[0], &g_Tc2.nPageData[0], sizeof(pageData));

  // 3. ana_page_program seting
  MB0->cmd_Program_XA	      = nxAddr;	// Page : 0~1279
  MB0->cmd_Program_YA	      = 0;	    // YA ignore for Page_Program, Word : page program : 256 bit (word) x 8
  MB0->cmd_max_Word_Num	      = 8;
  MB0->cmd_max_Page_Num	      = 8;      //page program : don't care 
  MB0->cmd_Program_READE_Type = 1;      // for analog program- 1 : Only one time program [Active Low Enable)
  MB0->Membrain_IP_CMD 	      = ST_Page_Program; 		//ana_page_program
  MB0->cmd_Program_time       = 900;
  do{
	for(int e=0; e<2; e++){
	  for(int i = 0; i < 64; i++){
		uint32_t nMask = ((e%2) ? 0x55555555 : 0xAAAAAAAA);
		g_MbBuff.pProgram_WD_Page[i] = pageData[i] | nMask; // cpuCmdDone!
	  }
	  delay(1); // not sure, let's just give it a try!
	}
	cliPrintf("%d._",nNumRepeat);
  } while(--nNumRepeat);
  cliPrintf("\r\n");

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 16);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 16);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 16);
	cliPrintf("-- Pgm Data --"); showInfo("0xa3000118", 16);
  }
}

void input_tag_set(uint32_t nxAddr) {

  // ### Input TAG-Register ###
  for (int i = 0; i < 44; i++) Input_Row_TAG_Data[i] = 0; // deselect all rows

  // 2. set the only row to verify
  uint32_t nwIndex = nxAddr / 32;
  Input_Row_TAG_Data[nwIndex % 44] = 1 << (nxAddr % 32);

  // 2. Note. Must be set Before writing TAG_Data to stop auto start!
  MB0->Membrain_IP_CMD     = ST_INPUT_ROW_TAG; // <<<<<< Row TAG Command

  //3. Write Tag_data to Dual port ram,  BASE + 0x00000118
  // CPU Command Done!!
  for(int i=0; i<44; i++) g_MbBuff.pInputTagRegBase[i] =  Input_Row_TAG_Data[i];

  usleep(8);   //8ns  x 1000 = 8us
}

void row_reg_set(uint32_t nxAddr) {

  // ### Input ROW-Register ###
  // 1. Initialize to 0'
  for (int i = 0; i < 324; i++) Input_Row_Reg_Data[i] = 0;	

  // 2. set the only row to verify
  uint32_t nwIndex = nxAddr / 4;
  Input_Row_Reg_Data[nwIndex] = 0xFF << (8 * (nxAddr % 4));

  // 2. write row reg data to dpram(BASE + 0x00000218) 
  for(int i=0; i<324; i++) g_MbBuff.pInputRowRegBase[i] =  Input_Row_Reg_Data[i];

  MB0->Membrain_IP_CMD = ST_INPUT_ROW_Reg;
  *g_MbBuff.pCpuCmdDone = 0;

  usleep(150);   //8ns  x 1000 = 8us
}

void readvb(cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nxAddr   = 0;
  uint32_t nyAddr   = 0;
  uint32_t nVfyData = 39;
  uint32_t nRmpUp   = 0xFE1F;

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("Usage: readvb [xAddr:%d] [yAddr:%d] [VfyData:%d] [rmpup:0x%04X(0xFE0F, x0FE4F, 0xFE8F,0xFE1F)]",
			  nxAddr, nyAddr, nVfyData, nRmpUp);
	return;
  }

  if(g_Tc2.eMode != eREADVB){cliPrintf("eREADEVB Entry\r\n"); g_Tc2.eMode = eREADVB;}

  if(argc > 0) nxAddr   = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nyAddr   = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nVfyData = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);
  if(argc > 3) nRmpUp   = (uint32_t)strtoul((const char * ) argv[3], (char **)NULL, (int) 0);
  nxAddr   = nxAddr   % 1280;
  nyAddr   = nyAddr   % 16;
  nVfyData = nVfyData % 40;

  acntInit();

  // must be here, after parsing command line parameter
  ft_cfg_readva_entry(nRmpUp);   // 1. FT Entry set cfg

  // 1. on_input_tag_set(args);
  input_tag_set(nxAddr);

  // 2. on_row_reg_set(args);
  row_reg_set(nxAddr);

  // 3. prepare vfy data(whose range is 31~0, 39~8)
  uint8_t *pcInputVfyRegData = (uint8_t*)&Input_Verify_Reg_Data[0];
  for(int i=0; i<36*4; i++) pcInputVfyRegData[i] = (i < 32*4) ? (nVfyData & 0x3F) : 0;

  // ############## Input Verify-Register (1) ####################

  // 4. write vfy data to dual port ram
	for (int j = 0; j < 36; j++) { //32 x 4 = BL 128
	  //g_MbBuff.pInputVfyRegBase[j] = (j<32) ? verify_data[i * 32 + j] : 0;	//128 BL
	  g_MbBuff.pInputVfyRegBase[j] = Input_Verify_Reg_Data[j];
	}

	MB0->cmd_Program_XA = nxAddr;		// Row
	MB0->cmd_Program_YA = nyAddr;		// Col : 0~15
	MB0->Membrain_IP_CMD = ST_VFYO_Verify_One;//VFYO for Read-Verify Mode [YA: x]

	*g_MbBuff.pCpuCmdDone = 0;
	usleep(100); // kdh@260210

	volatile  uint32_t *pErrStatus = (volatile uint32_t*)0xA3001338;
	uint32_t nRdyCnt = pErrStatus[0];
	uint32_t nOutRegErrorNV = pErrStatus[1];

	cliPrintf("RdyCnt:%d RegErrNV(%d):%s", nRdyCnt, nOutRegErrorNV ,(nOutRegErrorNV ? "Fail" : "Pass"));
	if(g_Tc2.nDbgLevel == 1){
	  showInfo("0xa3000918", 4); // 1 word(128bit)
	}
  // 5. FT initialize  to all 1's, why? not sure
  ft_cfg_readva_exit();   
}

// readv mode a(ya: 0~7)
void readva(cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nxAddr   =0;
  uint32_t nVfyData = 39;
  uint32_t nRmpUp   = 0xFE1F;

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("readva [xAddr:0] [VfyData:%d] [rmpup:0x%04X(0xFE0F, x0FE4F, 0xFE8F,0xFE1F)]",
			  nVfyData, nRmpUp);
	return;
  }

  if(g_Tc2.eMode != eREADVA){cliPrintf("eREADEVA Entry\r\n"); g_Tc2.eMode = eREADVA;}

  if(argc > 0) nxAddr   = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nVfyData = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nRmpUp   = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);
  nxAddr   = nxAddr   % 1280;
  nVfyData = nVfyData % 40;

  acntInit();

  // must be here, after parsing command line parameter
  ft_cfg_readva_entry(nRmpUp);   // 1. FT Entry set cfg

  input_tag_set(nxAddr); // 2. row tag

  // 3.1 row reg, done by rtl
  for (int i = 0; i < 324; i++) Input_Row_Reg_Data[i] = 0;	

  // 3.2 set the only row to verify, seems to me a bug!!!! modified like below@260203
  uint32_t nwIndex = nxAddr / 4;
  Input_Row_Reg_Data[nwIndex] = 0xFF << 8*(nxAddr%4); // row0

  // 3.3 copy _row reg_ data to DPRAM only
  for(int i=0; i<324; i++) g_MbBuff.pInputRowRegBase[i] = Input_Row_Reg_Data[i];

  // 5. readv
  uint32_t vData = 0;
  for(int i=0; i<4; i++) vData += ((nVfyData)& 0x3F) << 8*i;

  uint32_t verify_data[512]; //32 x 64 = 2048BL
  for(int i =0; i<512 ; i++) verify_data[i] = vData; //0x1F : 31

  // ############## Input Verify-Register (1) ####################
  // 5.1. write vfy data to dual port ram
  for (int j = 0; j < 36; j++) { //32 x 4 = BL 128
	g_MbBuff.pInputVfyRegBase[j] = (j<32) ? verify_data[j] : 0;	//128 BL
  }

  MB0->cmd_Program_XA = nxAddr;		// Row
  MB0->cmd_Program_YA = 0;		// Col : 0~15
  MB0->Membrain_IP_CMD = ST_VFYO_Verify_All;//VFYO for Read-Verify Mode [YA: x]

  // 5.2 start readv
  *g_MbBuff.pCpuCmdDone = 0;
  //usleep(150);	// <<< Readv에 ft cfg가 바뀌면 확인해서 늘려야 할 수 있음, 시뮬에이션에서는 120usec정도 인데, 현재 40 정동 여유를 줬다.
  usleep(350);	// kdh@260210

  // Set_Cfg_Data_00 ~ Set_Cfg_Data_08 : init 0
  for(int i=0; i<8; i++) MB0->Set_Cfg_Data[i] = 0;

  MB0->cmd_CE_Control  = 1;
  MB0->cmd_IP_BIAS     = 3;
  MB0->Membrain_IP_CMD = ST_IO_MUX_Write;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(2);	// note sure yet!

  volatile  uint32_t *pErrStatus = (volatile uint32_t*)0xA3001338;
  uint32_t nRdyCnt = pErrStatus[0];
  uint32_t nOutRegErrorNV = pErrStatus[1];
  cliPrintf(" nxAddr:%d, nVfyData:%d, nRmpup:0x%04X, RdyCnt:%d RegErrNV(%d):%s",
			nxAddr, nVfyData, nRmpUp, nRdyCnt, nOutRegErrorNV ,(nOutRegErrorNV ? "Fail" : "Pass"));

  if(g_Tc2.nDbgLevel == 1){
	showInfo("0xa3000918", 4*8*2); // 1 page(B0 ~ B1024)
  }
  ft_cfg_readva_exit();   // 1. FT Entry set cfg
}

void md_ctrl(cli_args_t *args) {
  (void)args;
  cliPrintf("--   Control   --"); showInfo("0xa3000000", 20);
}

void md_ft_cfg(cli_args_t *args) {

  (void)args;

  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000, 0xD3FF} //0 FT_ERS_HV[4:0]
	,{0x0001, 0xEF97} //1 FT_VSL[4:0], FT_ENBIAS_PRGDIG
	,{0x0002, 0xFFBF} //2 FT_UARANGE_EN
	,{0x0003, 0xFFE3} //3 FT_PRGDIG[3:0]

	,{0x0006, 0xFFD7} //4 FT_IDP[5:0]
	,{0x0007, 0xFEFF} //5 FT_ENIP_PRGDIG
	,{0x0008, 0xFFCD} //6 FT_OSC_200MHz[4:0]
	,{0x0009, 0xFFDF} //7 FT_USE_IDAC

	,{0x0021, 0x7FFF} //8 FT_DLL_BYPASS
	,{0x0085, 0xFFFE} //9 FT_DAC_DLL_BYPASS
	,{0x0086, 0xFCFF} //10FT_IREFTRIM[3:0], FT_STEP[0], FT_RANGE[2:0]
	,{0x008B, 0xFFFF} //11FT_STEP[2:1]

	,{0x00A3, 0xFFFF} //12FT_STEP[2:1]
	,{0x00C0, 0xFFFF} //13FT_STEP[2:1]
	,{0x00C3, 0xFFFF} //14FT_STEP[2:1]
	,{0x00C7, 0xFFFF} //15FT_STEP[2:1]
  };

  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
  }

  //3. run ft rd cfg command
  MB0->Membrain_IP_CMD		= ST_Read_Config;
  MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;
  *g_MbBuff.pCpuCmdDone     = 0; 
  usleep(10);

  cliPrintf("--   FT Config Data  --"); showInfo("0xa30000D8", 16);  

  cfg_addr_data_pair_t cfg_addr_data_pair_arr1[]={
	{0x014A, 0xEF97} //1 FT_VSL[4:0], FT_ENBIAS_PRGDIG
	,{0x0170, 0xFFBF} //2 FT_UARANGE_EN
	,{0x0172, 0xFFFF} //14FT_STEP[2:1]
	,{0x0173, 0xFFFF} //15FT_STEP[2:1]
  };

  nCfg_Data_Number = NOE(cfg_addr_data_pair_arr1);
  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr1[i].addr;
  }

  *g_MbBuff.pCpuCmdDone     = 0; 
  usleep(10);

  cliPrintf("--   FT Config Data  --"); showInfo("0xa30000D8", 8);  

}

static void showInfo(char *pcBase, uint32_t numWords){

  uint32_t nNumWords = numWords % 256;
  char cNumWords[16];

  sprintf(cNumWords, "%d", nNumWords);

  cli_args_t cli_args;
  //char tc2_base[]     = "0xa3000000";
  cli_args.argc = 2;

  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);
  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }
  cli_args.argv[0] = pcBase;
  cli_args.argv[1] = cNumWords;

  cliMemoryDump(&cli_args);
  free(cli_args.argv); // cleanup
}

static uint16_t getBitMask(uint16_t sNumBits, uint16_t sNumShift ){
  uint16_t  temp = 0xFFFF;
  temp = ~(~(temp << sNumBits) << sNumShift);
  return temp;
}

static char* getCurrnetMode(){
  char *str = (char*)malloc(sizeof(char)*32); // allocate memory
  switch(g_Tc2.eMode){
  case eIDLE:       strcpy(str, "IDLE");   break;
  case eINIT_ERASE: strcpy(str, "INIT");   break;
  case eBPGM:       strcpy(str, "BPGM");   break;
  case eDPGM:       strcpy(str, "DPGM");   break;
  case eAPGM:       strcpy(str, "APGM");   break;
  case eREADE:      strcpy(str, "READE");  break;
  case eREADN:      strcpy(str, "READN");  break;
  case eREADV:      strcpy(str, "READV");  break;
  case eREADVA:     strcpy(str, "READVA"); break;
  case eREADVB:     strcpy(str, "READVB"); break;
  case eREADB:      strcpy(str, "READB");  break;
  }
  return str; // return pointer
}

// need to be generalized for any MBs, MbBuff_t
static void ft_cfg_dpgm(cfg_write_policy_e cfg_write_policy){

  //#define USE_FT_DEFAULT_DPGM
#ifdef USE_FT_DEFAULT_DPGM  
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]= {
	{0x001,0xEF97},
	{0x021,0xFEFF}};
#else
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000,  0xD3FF} //FT_ERS_HV[4:0]
	,{0x0001,  0xEF97} //FT_VSL[4:0], FT_ENBIAS_PRGDIG
  };
#endif
  uint16_t nCfgDataNumber = NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number = nCfgDataNumber;
  for(int i = 0; i<nCfgDataNumber; i++) {
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr ;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data ;
  }

  if(cfg_write_policy == eWriteThrouh){
	MB0->Membrain_IP_CMD		= ST_Set_Config;
	*g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
	usleep(10);
  }
}

static void ft_cfg_reade(cfg_write_policy_e cfg_write_policy, uint32_t nFT_IDAC){

  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000,  0xD3FF} //FT_ERS_HV[4:0]
	,{0x0001,  0xEF97} //FT_VSL[4:0], FT_ENBIAS_PRGDIG
  };

  uint16_t nCfgDataNumber = NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number = nCfgDataNumber;
  for(int i = 0; i<nCfgDataNumber; i++) {
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr ;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data ;
  }

  if(cfg_write_policy == eWriteThrouh){
	MB0->Membrain_IP_CMD		= ST_Set_Config;
	*g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
	usleep(10);
  }
}

static void ft_cfg_erase(cfg_write_policy_e cfg_write_policy){

  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000,  0xD3FF} //FT_ERS_HV[4:0]
	,{0x0001,  0xEF97} //FT_VSL[4:0], FT_ENBIAS_PRGDIG
  };

  uint16_t nCfgDataNumber = NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number = nCfgDataNumber;
  for(int i = 0; i<nCfgDataNumber; i++) {
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr ;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data ;
  }

  if(cfg_write_policy == eWriteThrouh){
	MB0->Membrain_IP_CMD		= ST_Set_Config;
	*g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
	usleep(10);
  }
}

static void ft_cfg_bpgm(cfg_write_policy_e cfg_write_policy)
{

  //#define USE_FT_DEFAULT_BPGM  
#ifdef USE_FT_DEFAULT_BPGM  
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]= {
	{0x001,0xEFFF},
	{0x021,0xFEFF}};
#else
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x0000,  0xD3FF} //FT_ERS_HV[4:0]
	,{0x0001,  0xEF97} //FT_VSL[4:0], FT_ENBIAS_PRGDIG
	,{0x0006,  0xFFE3} //FT_PRGDIG[3:0]
	,{0x0009,  0xFFD7} //FT_IDP[5:0]

	,{0x0021,  0xFEFF} //FT_ENIP_PRGDIG
	,{0x008B,  0xFFC3} //FT_OSC_200MHz[4:0]
	,{0x00C0,  0xDFFF} //FT_USE_IDAC
	,{0x00C7,  0x7FFD} //FT_DLL_BYPASS

	,{0x014B,  0xFFF6} //FT_IREFTRIM[3:0], FT_STEP[0], FT_RANGE[2:0]
	,{0x0170,  0xFDFF} //FT_STEP[2:1]
	,{0x0172,  0xFCFF} //FT_STEP[2:1]
  };
#endif
  uint16_t nCfgDataNumber = NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number = nCfgDataNumber;
  for(int i = 0; i<nCfgDataNumber; i++) {
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr ;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data ;
  }

  if(cfg_write_policy == eWriteThrouh){
	MB0->Membrain_IP_CMD		= ST_Set_Config;
	*g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
	usleep(10);
  }
}

static cfg_addr_data_pair_t getVcg(uint32_t nVCG){

  uint16_t nVcg = nVCG % 10;
  cfg_addr_data_pair_t temp = {0x002,0xFFAF};

  switch(nVcg){
  case 0:  temp.data  = 0xFFAF; break; //FT_VCGTRIM 3.2[v]  from Testmodes and Array
  case 1:  temp.data  = 0xFF5F; break; //FT_VCGTRIM 3.4[v]  from NVR_V003 
  case 2:  temp.data  = 0xFF0F; break; //FT_VCGTRIM 3.6[v]  from NVR_V003 
  case 3:  temp.data  = 0xFEBF; break; //FT_VCGTRIM 3.8[v]  from NVR_V003 
  case 4:  temp.data  = 0xFE6F; break; //FT_VCGTRIM 4.0[v]  from NVR_V003 
  case 5:  temp.data  = 0xFE1F; break; //FT_VCGTRIM 4.2[v]  from NVR_V003 
  case 6:  temp.data  = 0xFDCF; break; //FT_VCGTRIM 4.4[v]  from NVR_V003 
  case 7:  temp.data  = 0xFD7F; break; //FT_VCGTRIM 4.6[v]  from NVR_V003 
  case 8:  temp.data  = 0xFD2F; break; //FT_VCGTRIM 4.8[v]  from NVR_V003 
  case 9:  temp.data  = 0xFCDF; break; //FT_VCGTRIM 5.0[v]  from NVR_V003 
  default: temp.data  = 0xFFAF; break;
  }
  return temp;
}

static void ft_cfg_apgm(cfg_write_policy_e cfg_write_policy){
#if 0
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x000, 0xD3B1}
	,{0x001, 0xFF97}
	,{0x002, 0xFD89}
	,{0x008, 0xFB90}
	,{0x021, 0xFFFF}
  };
#else
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x000, 0xD3B1} //FT_ERS_HV[4:0] FT_VSLTRIM[6:0]
	,{0x001, 0xFF97} //FT_VSL[4:0] FT_ENBIAS_PRGDIG
	,{0x002, 0xFD89} //FT_VCGTRIM[11:0]
	,{0x008, 0xFB90} //FT_VEGTRIM[10:0]
	,{0x021, 0xFFFF} //FT_ENIP_PRGDIG
  };
#endif

  //cfg_addr_data_pair_arr[2]= getVcg(g_Tc2.nVcg);
  cfg_addr_data_pair_arr[2].data = 0xF0000 | g_Tc2.nVcg;

  uint32_t nCfg_Data_Number = NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number	= nCfg_Data_Number;

  for(int i=0; i<nCfg_Data_Number; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
  }

  if(cfg_write_policy == eWriteThrouh){
	MB0->Membrain_IP_CMD		= ST_Set_Config;
	*g_MbBuff.pCpuCmdDone     = 0; //consider to use later!! Hmmm!!
	usleep(10);
  }

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Addr --"); showInfo("0xa3000058", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 8);
	cliPrintf("cfg_apgm\r\n");
	for(int i=0; i<nCfg_Data_Number; i++)
	  cliPrintf("0x%03X : 0x%04X\r\n", cfg_addr_data_pair_arr[i].addr, cfg_addr_data_pair_arr[i].data);
  }
  g_Tc2.bSetCfgApgm = true;
}

static void tm_cfg_init(){
  
  // 1.1 TM Entry
  MB0->cmd_IP_TEST      = 1;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(10);

  // 1.2 TM cfg
  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x001, 0x0  } //  TM_IDAC_EN
	,{0x015, 0x0  } //	TM_UARANGE_EN
	,{0x00E, 0x0  } //	TM_SEL_BLISO
	,{0x00C, 0x0  } //	TM_CGRH/M/L_VCGM+TM_GDACS_SPEED_MODE
	,{0x019, 0x0  } //	TM_CGREF_CGR
  };

  uint32_t nCfgDataNumber =  NOE(cfg_addr_data_pair_arr);
  MB0->cmd_Cfg_Data_Number =nCfgDataNumber;
  for(int i=0; i<nCfgDataNumber; i++){
	g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
  }

  MB0->Membrain_IP_CMD = ST_Set_Config;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);

  // 3. TM Exit
  MB0->cmd_IP_TEST      = 0;
  MB0->Membrain_IP_CMD  = ST_Ready;
  *g_MbBuff.pCpuCmdDone = 0;
  usleep(50);
}

static void tm_cfg_reade(bool bInit){

  cfg_addr_data_pair_t cfg_addr_data_pair_arr[]={
	{0x001, 0x0020} // TM_IDAC_EN
	,{0x015, 0x2000} //	TM_UARANGE_EN
	,{0x010, 0x0000} //	TM_CGRH/M/L_VCGM+TM_GDACS_SPEED_MODE
	,{0x019, 0x0000} //	TM_CGREF_CGR
	,{0x00C, 0x0000} //	TM_EG0V
  };

  uint32_t nCfgDataNumber =  NOE(cfg_addr_data_pair_arr);

  if(bInit){
	
	// Entry Phase : TM Entry and TM cfg
	// 1. TM Entry
	MB0->cmd_IP_TEST      = 1;
	MB0->Membrain_IP_CMD  = ST_Ready;
	*g_MbBuff.pCpuCmdDone = 0;
	usleep(10);

	// 2. TM cfg
	MB0->cmd_Cfg_Data_Number =nCfgDataNumber;
	for(int i=0; i<nCfgDataNumber; i++){
	  g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	  g_MbBuff.pSet_Cfg_Data[i] = cfg_addr_data_pair_arr[i].data;
	}

	MB0->Membrain_IP_CMD = ST_Set_Config;
	*g_MbBuff.pCpuCmdDone = 0;
	usleep(50);

  }else{

	// Exit Phase : TM cfg and TM Exit
	// 1. TM cfg initialize to 0'
	MB0->cmd_Cfg_Data_Number =nCfgDataNumber;
	for(int i=0; i<nCfgDataNumber; i++){
	  g_MbBuff.pSet_Cfg_Addr[i] = cfg_addr_data_pair_arr[i].addr;
	  g_MbBuff.pSet_Cfg_Data[i] = 0;
	}

	MB0->Membrain_IP_CMD = ST_Set_Config;
	*g_MbBuff.pCpuCmdDone = 0;
	usleep(50);

	// 2. TM Exit
	MB0->cmd_IP_TEST      = 0;
	MB0->Membrain_IP_CMD  = ST_Ready;
	*g_MbBuff.pCpuCmdDone = 0;
	usleep(50);
  }
}

void getLvls(StReadv_t *pStReadv, uint8_t ucLvlArr[], uint32_t nNumElements){

  for(uint32_t uwIndex = 0; uwIndex<2048/32; uwIndex++){
	for(uint32_t ubIndex = 0; ubIndex<32; ubIndex++){

	  ucLvlArr[32*uwIndex + ubIndex] = 0;

	  for (int lIndex=0; lIndex<32; ++lIndex) {
		uint32_t bit = ((pStReadv->StVfyo[lIndex].nVfyo[uwIndex] >> ubIndex) & 1);
		if(bit) {
		  ucLvlArr[32*uwIndex + ubIndex] = pStReadv->StVfyo[lIndex].nRef + 1;
		  break;
		}
	  }
	}
  }
}

static void getLvlAvg(StLvl_t *pStLvl) {

  double n = (double)pStLvl->n;

  for(int i=0; i<NUM_BLINE; i++){

	double   x = pStLvl->ucx[i];
	double   X = pStLvl->dX[i];
	double alpha = (n-1)/n;

	//pStLvl->X[i] = (n-1)/n*X + 1/n*x;
	//pStLvl->ucX[i] = (uint8_t)(alpha*X + (1-alpha)*x);
	pStLvl->dX[i] = (alpha*X + (1-alpha)*x);
  }
  pStLvl->n++;
}

void initStReadv(StReadv_t *pStReadv){

  pStReadv->nVcg = g_Tc2.nVcg;
  pStReadv->nReserved[0] = 0xAAAAAAAA;
  pStReadv->nReserved[1] = 0xBBBBBBBB;
  pStReadv->nReserved[2] = 0xCCCCCCCC;

  for(int i=0; i<32; i++){
	//stReadv->StVfyo[i].nRef = refData;
	pStReadv->StVfyo[i].nReserved[0] = 0xAAAAAAAA;
	pStReadv->StVfyo[i].nReserved[1] = 0xBBBBBBBB;
	pStReadv->StVfyo[i].nReserved[2] = 0xCCCCCCCC;
  }
}
  
void tglDbgLevel(cli_args_t *args) {
  g_Tc2.nDbgLevel  = g_Tc2.nDbgLevel ? 0 : 1;
  cliPrintf("DbgLevel: %d\r\n", g_Tc2.nDbgLevel);
}

uint32_t getSel(uint32_t num)
{
  return (num % 4 >= 2);   // even=0, odd=1
}

void acntInit(){

  for(int i=0; i<16; i++){
	if(i<8 ) MB0->Set_Cfg_Data[i] = 0; // DIN[127:0](8)
	else if(i<11) MB0->Set_Cfg_Data[i] = 0; // BIAS,IP0, IP1(3)
	else if(i<12) MB0->Set_Cfg_Data[i] = 0x7; // ACNT(1)
	else if(i<13) MB0->Set_Cfg_Data[i] = 0; // ARRDN(1)
	else if(i<16) MB0->Set_Cfg_Data[i] = 0; // RFU(3)
  }

  MB0->Membrain_IP_CMD   = ST_IO_MUX_Write;
  *g_MbBuff.pCpuCmdDone  = 0;
  usleep(2); // why? not sure yet!

  if(g_Tc2.bVerbose){
	cliPrintf("-- Control  --"); showInfo("0xa3000000", 8);
	cliPrintf("-- Cfg Data --"); showInfo("0xa3000098", 16);
  }
}
