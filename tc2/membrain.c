/* membrain.c --- 
 * 
 * Filename: membrain.c
 * Description: 
 * Author: Sanglae Kim
 * Maintainer: 
 * Created: Wed Nov 19 14:17:28 2025 (+0900)
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

//#include "tc2.h"
#include "membrain.h"

/****************************************************************************/
/**
*
* This function initializes membrain
*
* @param	None
*
* @return	XST_SUCCESS if everything sets up well, XST_FAILURE otherwise.
*
* @note		None
*
*****************************************************************************/
void initMembrain(MbCtrl_t *pMbCtrl, MbBuff_t *pMbBuff){

  uint32_t nArr[]={
	0x00000001 ,0x00000021 ,0x00000000 ,0x00000002
	,0x00000008 ,0x000000C1 ,0x000000C2 ,0x000000C3
	,0x000000C4 ,0x000000C5 ,0x000000C6 ,0x000000A0
	,0x000000A1 ,0x000000A2 ,0x000000A3 ,0x000000A4};

  pMbCtrl->SOF                   = 0xAAAA5555;  
  pMbCtrl->Membrain_IP_CMD       = 0;
  pMbCtrl->cmd_max_Word_Num      = 8;
  pMbCtrl->cmd_max_Page_Num      = 8;
  pMbCtrl->cmd_Cfg_Data_Number   = 16;
  pMbCtrl->cmd_Erase_Sector_Addr = 0;
  pMbCtrl->cmd_Program_XA        = 0;
  pMbCtrl->cmd_Program_YA        = 0;
  pMbCtrl->cmd_Output_Signal     = 0;
  pMbCtrl->cmd_Redundancy_EN     = 0;
  pMbCtrl->cmd_Read_Word_Number  = 64;
  pMbCtrl->cmd_CE_Control        = 0;
  pMbCtrl->cmd_Program_READE_Type= 2;
  pMbCtrl->cmd_membrain_ip_sel   = TC2b;
  pMbCtrl->cmd_sw_RESET          = 0;
  pMbCtrl->cmd_IP_TEST           = 0;
  pMbCtrl->cmd_IP_BIAS           = 0;
  pMbCtrl->tRDNS_margin_time     = 0;
  pMbCtrl->tACCN1_margin_time    = 0;
  pMbCtrl->cmd_Program_time      = 900;
  pMbCtrl->cmd_Erase_time        = 1500;
  pMbCtrl->cmd_efuse_set         = 0;

  pMbBuff->pSet_Cfg_Addr         = (uint32_t*)SCFG_ADDR_BASE; 
  pMbBuff->pSet_Cfg_Data         = (uint32_t*)SCFG_DATA_BASE; 
  pMbBuff->pRead_Cfg_Data        = (uint32_t*)RCFG_DATA_BASE; 
  pMbBuff->pProgram_WD_Page      = (uint32_t*)PGM_WR_PAGE_BASE; 
  pMbBuff->pProgram_RD_Page      = (uint32_t*)PGM_RD_PAGE_BASE; 
  pMbBuff->pInputTagRegBase      = (uint32_t*)INPUT_TAG_REG_BASE; 
  pMbBuff->pInputRowRegBase      = (uint32_t*)INPUT_ROW_REG_BASE; 
  pMbBuff->pInputVfyRegBase      = (uint32_t*)INPUT_VFY_REG_BASE; 
  pMbBuff->pArrcolBase      	 = (uint32_t*)ARRCOL_BASE;
  pMbBuff->pRdlo64Base           = (uint32_t*)RDLO64_BASE;
  pMbBuff->pCpuCmdDone           = (uint32_t*)CPU_CMD_DONE; 


  for(int i=0; i<16; i++) pMbBuff->pSet_Cfg_Addr[ i] = nArr[i];
  for(int i=0; i<16; i++) pMbBuff->pSet_Cfg_Data[i]  = 0;
  for(int i=0; i<16; i++) pMbBuff->pRead_Cfg_Data[i] = 0;
  for(int i=0; i<64; i++) pMbBuff->pArrcolBase[i] = 0;

//  for(int i=0; i<512; i++){
////	pMbBuff->pProgram_WD_Page[i] = 0;
//	pMbBuff->pProgram_RD_Page[i] = 0;
//  }
}

//void CE_BIAS_CE_on(int mb_number){
void CE_BIAS_CE_on(MbCtrl_t *pMbCtrl){
  //CE & BIAS_CE on
  pMbCtrl->cmd_CE_Control  = 1;	// CE[1:0] : 1-On, 6-Off
  pMbCtrl->cmd_IP_BIAS	   = 3;	// bit[0]: BIASMB Chip-Enable, bit[1] : BIASSL Chip-Enable)
  pMbCtrl->Membrain_IP_CMD = 0	;  
  //send_cmd(mb_number);
  //delay(5000);	// 5000 clk x 8ns = 40 us for tCS (CE setup time to READ MODE)
}

/* membrain.c ends here */
