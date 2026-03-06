/* tc2_ana_algo.c --- 
 * 
 * Filename: tc2_ana_algo.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Tue Feb  3 14:05:16 2026 (+0900)
 * Version: 
 * Package-Requires: ()
 * Last-Updated: Fri Feb 27 20:18:23 2026 (+0900)
 *           By: Sanglae Kim
 *     Update #: 120
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
// clang-format off
#include <stdbool.h>
#include "tc2.h"
#include "membrain.h"
#include "tc2_util.h"
#include "tc2_ana_algo.h"

#define NUM_RDV_AVG (20)

#define MB_NUM_COLS (2048)

extern tc2_t g_Tc2;
extern MbBuff_t g_MbBuff; 
//extern cli_t   cli_node;

void readva_sm (cli_args_t *args) ;

void analog_algo_row (cli_args_t *args) {

  int    argc = args->argc;
  char **argv = args->argv;

  uint32_t nxAddr        = 0;
  uint32_t nTgtLvl       = 54;
  uint32_t nVcgStart     = 0xDF0; // 4.32[v]

  if(argc > 0 && strcmp("h", argv[0]) == 0){
	cliPrintf("algo [xAddr:%d] [nTgtlvl:%d(~96)] [nVcgStart:0x%04X - 0xFFF(3[v])~0x50F(10[v])]",
			  nxAddr, nTgtLvl, nVcgStart);
	return;
  }

  if(argc > 0) nxAddr    = (uint32_t)strtoul((const char * ) argv[0], (char **)NULL, (int) 0);
  if(argc > 1) nTgtLvl   = (uint32_t)strtoul((const char * ) argv[1], (char **)NULL, (int) 0);
  if(argc > 2) nVcgStart = (uint32_t)strtoul((const char * ) argv[2], (char **)NULL, (int) 0);

  // variables init//////////////////////////////////////////////////////////////////////////
  // constant
  const  int MB_NUM_ROWS      = 1280;
  const  int INERTIA_MAX_CNT  = 2;
  const  int hist_sz          = 3;
  const  int max_loop         = 800;
  const  int max_threshold    = 100;
  const  int rdv_margin       = 3;
  const  int backtrack_vcg_sz = 5;
  int inertia_cnt             = 2;
  const  int LOWEST_VCG_LEVEL = 0xFFF; // VCG 3V

  // variables
  int max_std_weight       = 0; // not used
  int loop_count           = 0;

  int tgt_level = 0;
  uint32_t vcg       = nVcgStart; // eflash coupling gate voltage

  int tgt_weight[MB_NUM_COLS]     = {0,};  // target weight array from AI model in a row
  int pgm_done[MB_NUM_COLS]       = {0,};  // marking pgm_done, 1=pgm_done, 0=programming
  int current_weight[MB_NUM_COLS] = {0,};  // current weight placeholder during main loop
  int data_bits[MB_NUM_COLS]      = {0,};  // data_bits to be analog programmed
  int last_data_bits[MB_NUM_COLS] = {0,};  // previous loop data_bits for pgm_done marking
  int delta_weight[MB_NUM_COLS]   = {0,};  // delta weight from weight history, VCG decision

  int weight_hist[hist_sz][MB_NUM_COLS];   // weight history [0] : fast, [2] : present

  for (int i = 0; i < hist_sz; i++) {
	for (int j = 0; j < MB_NUM_COLS; j++) {
	  weight_hist[i][j] = 0;
	}
  }
  // variables init END/////////////////

  // weight target set//////////////////
  // target 54 is only for test. target data will be a AI model from txt files
  for (int i = 0; i < MB_NUM_COLS; i++) tgt_weight[i] = nTgtLvl % 97;

  // weight target set END//////////////

  // analog algo preperation//////////////////////////////////////////////////////////////////////////
  const  uint32_t nDly   = 2; 
  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {print("malloc failed!!\n"); return;}

  char cxAddr[16];
  sprintf(cxAddr, "%d", nxAddr);

  cli_args.argc     =  2;
  cli_args.argv[0]  =  cxAddr;
  cli_args.argv[1]  =  "1"; // numToerase

  // 1. >>>> TARGET ROW ERASE <<<<<
  cliPrintf(">>> 1. Sector Erase \r\n"); on_sector_erase(&cli_args); delay(nDly*1000);

  // 2. >>>> TARGET ROW BLISO program <<<<
  cliPrintf(">>> 2. Bliso Pgm\r\n");     bliso_pgm(&cli_args);       delay(nDly*1000);

  // TODO : ##TC2B oepration##
  // 3. SOFT PROGRAM(selectable 0xE0, 0x90~~)
  // iteration : analog program all row 3.8V -> readE iDAC 10uA
  // finish condition : all bit programmed

  // TODO : ##TC2B operation##
  // 4. Digital program for zero weight
  // will be implemented

  // TODO : ##TC2B operation##
  // 5. 1row READV SWEEP before main program entry
  cli_args.argc     =  2;
  //cli_args.argv[0]  =  "0"; // xAddr done priviously at Erase Step
  cli_args.argv[1]  =  "1";   // nStride
  readvas(&cli_args); // << do readva sweep & save the results to the global variable

  // current_weight[0] = READV SWEEP
  StReadv_t StReadv;
  // 1.0 just copy all, we dont touch global data and leave it as is.
  memcpy(&StReadv, &g_Tc2.StReadv, sizeof(StReadv_t));
  //initStReadv(&StReadv);

  // 1.1 bit interleave now
  for(int i=0; i<32; i++) bitInterleaveWord(&g_Tc2.StReadv.StVfyo[i], &StReadv.StVfyo[i]);
  // 1.2 get levels
  uint8_t ucLvlArr[MB_NUM_COLS]; // temporary
  getLvls(&StReadv, &ucLvlArr[0], sizeof(ucLvlArr));

  // the elements of ucLvl should be _0xFF_ cause we have erased above.
  // broadcasting [0] to [1], [2] <<<<< ?
  for(int i=0; i<MB_NUM_COLS; i++){
	current_weight[i] = (ucLvlArr[i] - 8) * 3; //(39~8) -> (31 ~0) * 3 = 93~0
	weight_hist[0][i] = (ucLvlArr[i] - 8) * 3;
	weight_hist[1][i] = (ucLvlArr[i] - 8) * 3;
	weight_hist[2][i] = (ucLvlArr[i] - 8) * 3;
  }

  // main analog algo loop//////////////////////////////////////////////////////////////////////////
  while (1) {

	// 0. check if all cells reached target levels
	bool bPgmDone = true;
	for(int i=0; i<MB_NUM_COLS; i++){
	  if(pgm_done[i] == 1){continue;} else {bPgmDone = false; break;}
	}
	if(bPgmDone) {
	  cliPrintf(">>>>>> Apgm Algo Done!!! <<<<<<\r\n");
	  cliPrintf(">>>>>> loop_count: %d !!! <<<<<<\r\n", loop_count);
	  cliPrintf(">>>>>> Apgm Algo Done!!! <<<<<<\r\n");
	  return;
	}
	uint32_t nPgmDone = 0;
	for(int i=0; i<MB_NUM_COLS; i++) nPgmDone += pgm_done[i];
	cliPrintf("loop_count: %d NumToProgram: %d(%d)\r\n",
			  loop_count ,(MB_NUM_COLS - nPgmDone), MB_NUM_COLS);

	// 1. Extracting max_remaining_trget
	int max_remaining_target = 0;
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if (!pgm_done[i] && tgt_weight[i] > max_remaining_target) {
		max_remaining_target = tgt_weight[i];
	  }
	}

	// TODO : ##TC2B operation##
	tgt_level = max_remaining_target + rdv_margin;
	char cTgtLvl[16];
	sprintf(cTgtLvl, "%d", (tgt_level / 3 + 8 )); // (96~0, 39~8)???
	cli_args.argc     =  2;
	cli_args.argv[0]  =  cxAddr;  // xAddr
	cli_args.argv[1]  =  cTgtLvl; // target_level

	//  readV 5th and average = rdV_avg[i];
	double dRdvAvg[MB_NUM_COLS]  = {0.,};  // target readV average value during main loop
	for(int i=0; i<NUM_RDV_AVG; i++){

	  // a1. run readva
	  readva(&cli_args);

	  // a2. get readva result from dual port ram to local, bit wise
	  //uint32_t nVfyo[MB_NUM_COLS/32];
	  StVfyo_t src, des;
	  for(int i=0; i<MB_NUM_COLS/32; i++) src.nVfyo[i] = g_MbBuff.pProgram_RD_Page[i];
	  bitInterleaveWord(&src, &des);

	  // a3. expand from bit to double & accumulate(summation) 
	  for(int i=0; i<MB_NUM_COLS/32; i++){
		for(int j=0; j<32; j++){
		  dRdvAvg[32*i+j] += (des.nVfyo[i] >> j) & 0x1;
		}
	  }
	}
	// a4. get average
	for(int i=0; i<MB_NUM_COLS;i++) dRdvAvg[i] /= NUM_RDV_AVG;

	// 2. current weight setting for selecting pgm_done bits
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if(!pgm_done[i]){
		if (dRdvAvg[i] < 0.5 ) {
		  current_weight[i] = max_remaining_target;
		} else {
		  current_weight[i] = max_threshold;
		}
	  }
	}
	// 3. data bits setting for programming bit
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if(!pgm_done[i]){
		if (current_weight[i] > tgt_weight[i]) {
		  data_bits[i] = 0; // bits for program
		} else {
		  data_bits[i] = 1; // pgm_done_bit
		}
	  }
	}
	// 4. pgm_done marking
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if(!pgm_done[i]){
		
		if (data_bits[i] == 1 && last_data_bits[i] == 0) {
		  pgm_done[i] = 1; // pgm_done_bit
		} else {
		  pgm_done[i] = 0; // bits for program
		}
	  }
	}
	// 5. set last_data_bits
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  last_data_bits[i] = data_bits[i];
	}
	// 6. weight history
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  weight_hist[0][i] = weight_hist[1][i]; // pop
	  weight_hist[1][i] = weight_hist[2][i]; // pop
	  weight_hist[2][i] = current_weight[i]; // present
	}

	for (int i = 0; i < MB_NUM_COLS; i++) {
	  delta_weight[i] = weight_hist[1][i] - weight_hist[2][i]; // present
	}
	// 7. extracting max_delta_weight
	int max_delta_weight = 0;
	for (int i = 0; i < MB_NUM_ROWS; i++) {
	  if (!pgm_done[i]) {
		int d = delta_weight[i];
		if (d < 0)
		  d = -d; // delta 횇짤짹창(�첵쨈챘째짧) 쩐짼째챠 쩍횒�쨍쨍챕
		if (d > max_delta_weight) {
		  max_delta_weight = d;
		}
	  }
	}
	// 8. setting analog program condition : VCG
	// case 0 : the last jump is too high, decrease VCG
	if (max_delta_weight > 15) {
	  if (vcg >= LOWEST_VCG_LEVEL) {
		// current VCG is the LOWEST_VCG_LEVEL, holding VCG
	  } else {
		vcg = vcg + backtrack_vcg_sz; // decreasing vcg
	  }
	}
	// case 1 : stddev is too high, decrease VCG
	// not used
	else if (max_std_weight > 10) {
	  if (vcg >= LOWEST_VCG_LEVEL) {
		// current VCG is the LOWEST_VCG_LEVEL, holding VCG
	  } else {
		// TODO : ##TC2B operation : analog program_row with data_bits
	  }
	}
	// case 2 :
	else if (max_delta_weight > 10) {
	  // holding VCG
	  vcg = vcg;
	}
	// case 3 :
	// not used
	else if (max_std_weight > 5) {
	  if (inertia_cnt >= INERTIA_MAX_CNT) {
		vcg = vcg - 1; // increaseing vcg, +0.0025V
		inertia_cnt = 0;
	  } else {
		vcg = vcg; // holding vcg
		inertia_cnt++;
	  }
	}
	// case 4 :
	else {
	  if (inertia_cnt >= INERTIA_MAX_CNT) {
		vcg = vcg - 1; // increaseing vcg, +0.0025V
		inertia_cnt = 0;
	  } else {
		vcg = vcg; // holding vcg
		inertia_cnt++;
	  }
	}

	// 9. main analog program
	// TODO : ##TC2B operation : main analog program with current vcg & data bits
	// analog_program(vcg, data_bit, row);
	// int to bits
	for(int i=0; i<MB_NUM_COLS/32; i++){
	  uint32_t temp = 0;
	  for(int j=0; j<32; j++) temp |= (data_bits[32*i + j] & 0x1) << j;
	  g_Tc2.nPageData[i] = temp;
	}
	char strVcg[16];
	sprintf(strVcg, "%d", vcg);

	cli_args.argc     =  3;
	cli_args.argv[0]  =  cxAddr;
	cli_args.argv[1]  =  strVcg; // vcg
	cli_args.argv[2]  =  "1"; // numRepeat
	on_ana_page_program(&cli_args);

	loop_count++;
	if ((loop_count > max_loop) || (vcg < 0xB4F)) {
	  cliPrintf("loop_count: %d\r\n", loop_count);
	  cliPrintf("Vcg: 0x%03x(%0.3f)\r\n", vcg, (-0.0025*(double)vcg + 13.2375));
	  break;
	}

	if(uartAvailable(0) > 0) break;

  } // main analog algo loop

  free(cli_args.argv); // cleanup
} // analog_algo_row_END

void soft_program_row(cli_args_t *args) {
  int argc    = args->argc;
  char **argv = args->argv;

  uint32_t nxAddr    = 0;
  uint32_t nVcgStart = 0xDF0; // 4.32[v]
  // uint32_t iDAC = 0xE0;		// reade iDAC
  uint32_t iDAC = 41; //(22.4[uA]) readv //temporary

  if (argc > 0 && strcmp("h", argv[0]) == 0) {
	cliPrintf("algo [xAddr:%d] [nVcgStart:0x%04X] [iDAC:0x%04X]", nxAddr, nVcgStart, iDAC);
	return;
  }

  if (argc > 0) nxAddr    = (uint32_t)strtoul((const char *)argv[0], (char **)NULL, (int)0);
  if (argc > 1) nVcgStart = (uint32_t)strtoul((const char *)argv[1], (char **)NULL, (int)0);
  if (argc > 2) iDAC      = (uint32_t)strtoul((const char *)argv[2], (char **)NULL, (int)0);

  // variables init//////////////////////////////////////////////////////////////////////////
  // constant

  //  const int MB_NUM_ROWS      = 1280;
  const int max_loop         = 1000;
  //  const int LOWEST_VCG_LEVEL = 0xFFF; // VCG 3Vread

  // variables
  int loop_count = 0;
  //  int tgt_level  = 0;
  uint32_t vcg   = nVcgStart; // eflash coupling gate voltage

  int pgm_done[MB_NUM_COLS]       = {0,}; // marking pgm_done, 1=pgm_done, 0=programming
  int data_bits[MB_NUM_COLS]      = {0,}; // data_bits to be analog programmed
  int last_data_bits[MB_NUM_COLS] = {0,}; // previous loop data_bits for pgm_done marking
  // variables init END/////////////////

  // analog algo preperation//////////////////////////////////////////////////////////////////////////
  //  const uint32_t nDly = 2;
  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {
	print("malloc failed!!\n");
	return; // handle allocation failure
  }
  // main analog algo loop//////////////////////////////////////////////////////////////////////////

  // backup the state
  bool bReadAddrTypeBackup = g_Tc2.bReadeAddrType;
  g_Tc2.bReadeAddrType =  false;// fasle(->LINEAR_ADDR, ORIG_DATA) because in line with readva

  while (1) {
	// 0. check if all cells reached target levels
	bool bPgmDone = true;
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if (pgm_done[i] == 1) {
		continue;
	  } else {
		bPgmDone = false;
		break;
	  }
	}
	if (bPgmDone) {
	  cliPrintf(">>>>>> soft program Done!!! <<<<<<\r\n");
	  cliPrintf(">>>>>> loop_count: %d !!! <<<<<<\r\n", loop_count);
	  cliPrintf(">>>>>> soft program Done!!! <<<<<<\r\n");
	  return;
	}
	uint32_t nPgmDone = 0;
	for (int i = 0; i < MB_NUM_COLS; i++) nPgmDone += pgm_done[i];
	cliPrintf("loop_count: %d NumToProgram: %d(%d)\r\n",
			  loop_count, (MB_NUM_COLS - nPgmDone), MB_NUM_COLS);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//  reade on dedicated iDAC
	double dRdvAvg[MB_NUM_COLS] = {0.,};
	// a1. run reade_iDAC
	// cliPrintf("reade [xAddr:0] [FT_IDAC:0x%02X] [ARRDN/ARRCOL/NVR]", nFT_IDAC);
	char cxAddr[16];
	char ciDAC[16];
	sprintf(cxAddr, "%d", nxAddr);
	sprintf(ciDAC, "%d", iDAC);
	cli_args.argc = 2;
	cli_args.argv[0] = cxAddr;
	cli_args.argv[1] = ciDAC; // target_level

	// dummy READV for stable operation
	// hsm@260220
	//cliPrintf("dummy READV for stable operation\r\n");
	//for (int i = 0; i < 20; i++)
	//  readva_sm(&cli_args);

	for (int i = 0; i < 5; i++) {
	  // a1. run readva
	  //readva_sm(&cli_args);
	  reade(&cli_args);

	  // a2. get readva result from dual port ram to local, bit wise
	  // uint32_t nVfyo[MB_NUM_COLS/32];
	  StVfyo_t src, des;
	  for (int i = 0; i < MB_NUM_COLS / 32; i++)
		src.nVfyo[i] = g_MbBuff.pProgram_RD_Page[i];
	  bitInterleaveWord(&src, &des);

	  // a3. expand from bit to double & accumulate(summation)
	  for (int i = 0; i < MB_NUM_COLS / 32; i++) {
		for (int j = 0; j < 32; j++) {
		  dRdvAvg[32 * i + j] += (des.nVfyo[i] >> j) & 0x1;
		}
	  }
	}
	// a4. get average
	int dRdvAvg_cnt = 0;
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  dRdvAvg[i] /= 5.0;
	  if (dRdvAvg[i] < 0.5) dRdvAvg_cnt++;
	  // if (i < 10) cliPrintf("dRdvAvg[%d] =  %f\r\n", i, dRdvAvg[i]);
	}
	cliPrintf("dRdvAvg_cnt: %d \r\n", dRdvAvg_cnt);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 2. current weight setting for selecting pgm_done bits
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if (!pgm_done[i]) {
		if (dRdvAvg[i] < 0.5) {
		  data_bits[i] = 1; // pgm_done_bit
		} else {
		  data_bits[i] = 0; // bits for program
		}
	  }
	}

	// 4. pgm_done marking
	//////////////////////////////////////////////////////////////////
	for (int i = 0; i < MB_NUM_COLS; i++) {
	  if (!pgm_done[i]) {
		if (data_bits[i] == 1 && last_data_bits[i] == 0) {
		  pgm_done[i] = 1; // pgm_done_bit
		} else {
		  pgm_done[i] = 0; // bits for program
		}
	  }
	  // if (i < 10) cliPrintf("pgm_done[%d] =  %d\r\n", i, pgm_done[i]);
	}

	// 5. set last_data_bits
	for (int i = 0; i < MB_NUM_COLS; i++) last_data_bits[i] = data_bits[i];

	// 9. main analog program
	// TODO : ##TC2B operation : main analog program with current vcg & data bits
	// analog_program(vcg, data_bit, row);
	// int to bits
	for (int i = 0; i < MB_NUM_COLS / 32; i++) {
	  uint32_t temp = 0;
	  for (int j = 0; j < 32; j++)
		temp |= (data_bits[32 * i + j] & 0x1) << j;
	  g_Tc2.nPageData[i] = temp;
	}
	char strVcg[16];
	sprintf(strVcg, "%d", vcg);

	cli_args.argc = 3;
	cli_args.argv[0] = cxAddr;
	cli_args.argv[1] = strVcg; // vcg
	cli_args.argv[2] = "1";	   // numRepeatdpom
	on_ana_page_program(&cli_args);

	loop_count++;
	// if ((loop_count > max_loop) || (vcg < 0xB4F))
	// vcg max 9V
	if ((loop_count > max_loop) || (vcg < 0x69F)) {
	  cliPrintf("loop_count: %d\r\n", loop_count);
	  cliPrintf("Vcg: 0x%03x(%0.3f)\r\n", vcg, (-0.0025 * (double)vcg + 13.2375));
	  cliPrintf("break condition\r\n");
	  break;
	}

	if (uartAvailable(0) > 0) break;

  } // main analog algo loop

  // restore the orignal state
  g_Tc2.bReadeAddrType = bReadAddrTypeBackup;

  free(cli_args.argv); // cleanup
} // soft_program_row_END

void readva_sm (cli_args_t *args) {
  
};

void readvas_lvl(cli_args_t *args) {

  int result_ref[2048];
  int last_bit[2048];

  // init
  for (int j = 0; j < 2048; j++) {result_ref[j] = 8; last_bit[j] = 0;}

  int argc    = args->argc;
  char **argv = args->argv;

  uint32_t nxAddr  = 0;
  uint32_t nRmpUp  = 0xFE4F;
  uint32_t nOSC    = 0xFFC5; // #21
  uint32_t nStride = 1;

  if (argc > 0 && (strcmp("h", argv[0]) == 0)) {
	cliPrintf("Usage: readvas [xAddr:%d] [rmpup:0x%04X] [rOSC:0x%04X]\r\n",  nxAddr, nRmpUp, nOSC);
	return;
  }

  if (argc > 0) nxAddr = (uint32_t)strtoul((const char *)argv[0], (char **)NULL, (int)0);
  if (argc > 1) nRmpUp = (uint32_t)strtoul((const char *)argv[1], (char **)NULL, (int)0);
  if (argc > 2) nOSC   = (uint32_t)strtoul((const char *)argv[2], (char **)NULL, (int)0);

  cli_args_t cli_args;
  cli_args.argc = 3;
  cli_args.argv = malloc(sizeof(char *) * cli_args.argc);

  if (cli_args.argv == NULL) {print("malloc failed!!\n"); return;}
  char cxAddr[16];
  char pcRmpUp[16];
  char pcOSC[16];
  sprintf(cxAddr, "%d", nxAddr);
  sprintf(pcRmpUp, "%d", nRmpUp);
  sprintf(pcOSC, "%d", nOSC);
  cli_args.argv[0] = cxAddr;

  int32_t refData = 39;
  int32_t nIndex = 0;

  StReadv_t StReadv;
  initStReadv(&StReadv);

  // cliPrintf("xAddr:%s, nStride: %d\r\n", cli_args.argv[0], nStride);
  for (int loop = 0; loop < 3; loop++) {

    int32_t refData = 39;
    int32_t nIndex = 0;

    // init
    for (int j = 0; j < 2048; j++) {result_ref[j] = 8; last_bit[j] = 0;}

    if (loop == 0)
      nRmpUp = 0xFE4F;
    else if (loop == 1)
      nRmpUp = 0xFE4F;
    else if (loop == 2)
      nRmpUp = 0xFE4F;
    else if (loop == 3)
      nRmpUp = 0xFE8F;
    else if (loop == 4)
      nRmpUp = 0xFE4F;
    else if (loop == 5)
      nRmpUp = 0xFE0F;

    char pcRmpUp[16];
    sprintf(pcRmpUp, "%d", nRmpUp);
    cli_args.argv[2] = pcRmpUp;

    while (refData >= 8) {

      char cRefData[16]; // verify value
      sprintf(cRefData, "%d", refData);

      // <<<<<<<<<  1. readv Operation >>>>>>>>>>//
      cli_args.argv[0] = cxAddr;
      cli_args.argv[1] = cRefData;
      cli_args.argv[2] = pcRmpUp;
      // readva_sm(&cli_args);
      readva(&cli_args);

      // <<<<<<<<<  2. fetch Vfyo from DPRAM for File Operation >>>>>>>>>>//
      StReadv.StVfyo[nIndex % 32].nRef = refData;

      // volatile uint32_t *pnVfyo = (uint32_t*)0xA3000918;
      // for(int i=0; i<2048/32; i++) stReadv.StVfyo[nIndex % 32].nVfyo[i] = pnVfyo[i];
      for (int i = 0; i < 2048 / 32; i++)
        StReadv.StVfyo[nIndex % 32].nVfyo[i] = g_MbBuff.pProgram_RD_Page[i];

      // ?? ¿©±â¼­ bit transition °Ë»ç
      for (int j = 0; j < 2048; j++) {
        int word = j / 32;
        int bit  = j % 32;

        int cur_bit = (g_MbBuff.pProgram_RD_Page[word] >> bit) & 0x1;
        // ? case 1: Ã³À½ºÎÅÍ 1ÀÎ °æ¿ì
        if (nIndex == 0 && cur_bit == 1 && result_ref[j] == 8)      result_ref[j] = refData;

        if (last_bit[j] == 0 && cur_bit == 1 && result_ref[j] == 8) result_ref[j] = refData + 1; // 0¡æ1 ÃÖÃÊ ÁöÁ¡

        last_bit[j] = cur_bit;
      }

      refData -= nStride;
      nIndex++;
    }
    if (loop > 1) {
      for (int i = 0; i < 2048; i++) {
        // cliPrintf(" BL_add :%d, nVFData:%d, RMPUP:%04X\r\n", i, result_ref[i], nRmpUp);
        if (i == 2047)
          cliPrintf("%d\r\n", result_ref[i]);
        else
          cliPrintf(" %d,", result_ref[i]);
      }
    }
  }

  // <<<<<<<<<  3.0 save rdvs data to global variagle card >>>>>>>>>>//
  memcpy(&g_Tc2.StReadv, &StReadv, sizeof(StReadv_t));

  // <<<<<<<<<  3.1 save data to SD card >>>>>>>>>>//
  // if(nStride == 1) saveReadv(&stReadv);

  free(cli_args.argv); // cleanup
}
// clang-format on
