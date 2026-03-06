/*
 * membrain.h
 *
 *  Created on: 2025. 11. 19.
 *      Author: User
 */

#ifndef SRC_MEMBRAIN_H_
#define SRC_MEMBRAIN_H_

#include <stdint.h>

#define __I volatile const /* 'read only' permissions      */
#define __O volatile       /* 'write only' permissions     */
#define __IO volatile      /* 'read / write' permissions   */

#define APBPERIPH_BASE        (0xA3000000U)
#define TC2_BASE              (APBPERIPH_BASE + 0x00000000U)
#define SCFG_ADDR_BASE        (TC2_BASE + 0x00000058U)
#define SCFG_DATA_BASE        (TC2_BASE + 0x00000098U)
#define RCFG_DATA_BASE        (TC2_BASE + 0x000000D8U)
#define PGM_WR_PAGE_BASE      (TC2_BASE + 0x00000118U)
#define INPUT_TAG_REG_BASE    (TC2_BASE + 0x00000118U)
#define INPUT_ROW_REG_BASE    (TC2_BASE + 0x00000218U)
#define INPUT_VFY_REG_BASE    (TC2_BASE + 0x00000818U)
#define PGM_RD_PAGE_BASE      (TC2_BASE + 0x00000918U)
#define CPU_CMD_DONE          (TC2_BASE + 0x00001334U)

#define ARRCOL_BASE           (TC2_BASE + 0x00001118U)
#define RDLO64_BASE           (TC2_BASE + 0x00001218U)

typedef struct{
   __IO  uint32_t SOF;                      //0 
   __IO  uint32_t Membrain_IP_CMD;			//1
   __IO  uint32_t cmd_max_Word_Num;			//2
   __IO  uint32_t cmd_max_Page_Num;			//3
   __IO  uint32_t cmd_Cfg_Data_Number;		//4
   __IO  uint32_t cmd_Erase_Sector_Addr;	//5
   __IO  uint32_t cmd_Program_XA;			//6
   __IO  uint32_t cmd_Program_YA;			//7
   __IO  uint32_t cmd_Output_Signal;		//8
   __IO  uint32_t cmd_Redundancy_EN;		//9
   __IO  uint32_t cmd_Read_Word_Number;		//10
   __IO  uint32_t cmd_CE_Control;			//11
   __IO  uint32_t cmd_Program_READE_Type;	//12
   __IO  uint32_t cmd_membrain_ip_sel;		//13
   __IO  uint32_t cmd_sw_RESET;				//14
   __IO  uint32_t cmd_IP_TEST;				//15
   __IO  uint32_t cmd_IP_BIAS;				//16
   __IO  uint32_t tRDNS_margin_time;		//17
   __IO  uint32_t tACCN1_margin_time;		//18
   __IO  uint32_t cmd_Program_time;			//19
   __IO  uint32_t cmd_Erase_time;			//20
   __IO  uint32_t cmd_efuse_set;		    //21
   __IO  uint32_t Set_Cfg_Addr[16];		    //22
   __IO  uint32_t Set_Cfg_Data[16];		    //38
   __IO  uint32_t Read_Cfg_Data[16];		//54

}MbCtrl_t;

typedef struct{
  __IO  uint32_t *pSet_Cfg_Addr;  //37
  __IO  uint32_t *pSet_Cfg_Data;  //53
  __IO  uint32_t *pRead_Cfg_Data; //69 Read_Cfg_data placeholders
  __IO  uint32_t *pProgram_WD_Page;  //70~581, 581-70+1=512=8*8*8
  __IO  uint32_t *pProgram_RD_Page;  //582 ~
  __IO  uint32_t *pInputTagRegBase;  //582 ~
  __IO  uint32_t *pInputRowRegBase;  //582 ~
  __IO  uint32_t *pInputVfyRegBase;  //582 ~
  __IO  uint32_t *pArrcolBase;  //582 ~
  __IO  uint32_t *pRdlo64Base;  //582 ~
  __IO  uint32_t *pCpuCmdDone;  //1229(0x001334) ~
}MbBuff_t;

typedef enum{
  TC2b, TC2
}tc2_version_e;


#define MB0 ((MbCtrl_t*)TC2_BASE)

void initMembrain(MbCtrl_t *pMbCtrl, MbBuff_t *pMbBuff);

#endif /* SRC_MEMBRAIN_H_ */
