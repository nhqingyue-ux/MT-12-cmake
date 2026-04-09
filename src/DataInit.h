#include "Data.h"
///////////
//
//
//
///////////
void init(void);
void CDMInit(void);
unsigned short DataInitSet(unsigned short SetNumber);
void TpTypeMdSet(void);
//
extern unsigned char
write_FRAM(unsigned long addr, unsigned short *ptr_data, unsigned short cnt);
extern unsigned char
read_FRAM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt);
extern void WREG(unsigned char s, unsigned char cs, unsigned char din);
extern void HT_OUT(unsigned short data);
extern unsigned char I2C1_MUX_lock(void);
extern void I2C1_MUX_unlock(void);
extern unsigned char I2C2_MUX_lock(void);
extern void I2C2_MUX_unlock(void);
/////////////////////				 
//
//	data
//
////////////////////
//extern unsigned short FirstRS485RunFg;
extern unsigned short OldKAndJSel;
extern unsigned short DataSet500W;
extern unsigned short heat;
extern volatile unsigned short TimerBase1;
extern volatile unsigned short TimerBase2;
extern volatile unsigned short StepCnt;	  
//
extern unsigned short TestCnt;
extern unsigned short EnetServerOK;
extern unsigned short NormalTp;
extern unsigned short TempLen;
extern unsigned short TempBegLen;
extern unsigned short SynFg;
extern unsigned short TwoTpHeatFg;
extern unsigned short InitTpHt;
extern unsigned short TwoTpFg;
extern unsigned short TimeBk1;			// for 0.1 sec clock
extern unsigned short TimeBk2;			// for 1.0 sec clock
extern unsigned short TimeBk3;			// for ¶}¾÷®É¶¡­pºâ
extern unsigned short TempHiFg;
extern unsigned short TempLowFg;
extern unsigned short TpOkFg;
extern unsigned short PID_CAN_CTRL_FG;
extern unsigned short Heat_Central;
extern unsigned short TpClLpEn;
extern unsigned short HWRDlen;
extern unsigned short TpSetUint;
extern unsigned short RealTpOutPwm;
//
extern unsigned short TpErrTm;
extern unsigned short TempTmOKFg;
extern unsigned short TpClLpFg4;
extern unsigned short HeatRunFg;
extern unsigned short TpClLpFg2;
extern unsigned short TpClLpFg3;
extern unsigned short HiAlmSts;
extern unsigned short LowAlmSts;
//
extern int fifoin;
//
extern unsigned short ch1[12][35];
extern unsigned short ch1_sec_num[12];
extern unsigned short BSS_RAM_CDM_START[1024];
extern unsigned short BSS_RAM_CDM_START3[1024];
extern unsigned short TempAuTpUnitUp[12];
extern unsigned short TempAuTpUnit[12];
extern unsigned int TempTm5Min[12];
extern unsigned int TempTm5Min2[12];
extern unsigned int TempTmBuff[12];
extern unsigned short OldTpcurr2[12];
//
extern float Upmax, Upmin;
extern float dX; 					// sampling time
//
extern unsigned short *pSplcY;
extern unsigned short *pSplcM;
extern unsigned short *pSplcX;
extern unsigned short *pSplcK;
extern unsigned short *pSplcT;
extern unsigned short *pSplcC;
extern unsigned short *pSplcF;
extern unsigned short *pCDM;
extern unsigned short *pCDM3;
extern unsigned short *pA00001;
extern unsigned short *pA02001;
extern unsigned short *pA10001;
extern unsigned short *pA10129;
extern unsigned short *pA11001;
extern unsigned short *pA11501;
extern unsigned short *pA12001;
extern unsigned short *pA20001;
extern unsigned short *pA30001;
//
extern unsigned short OldTempSetSum[12];
extern unsigned short AlternTm[12];
extern unsigned short AlternTmBk[12];
extern unsigned short OldTpcurr[12];
extern unsigned short OrgTpOutPwm[12];
extern unsigned short OrgTpOutPwmBk[12];
extern unsigned short OrgTpOutPwmBk2[12];
//
extern struct NEW_MLAD_STS SplcSts;
extern struct TpPID_DATA TpPIDdata[12], tempData[12];
const unsigned short EEPROMInitialValue[499] = {
0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x0000, 0x0000, 0x0000, // 20001 ~ 20015
0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0708, 0x0000, 0x0000, 0x0000, // 20016 ~ 20030
0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x0000, 0x0000, 0x0000, // 20031 ~ 20045
0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x0000, 0x0000, 0x0000, // 20046 ~ 20060
0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x012C, 0x0000, 0x0000, 0x0000, // 20061 ~ 20075
0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x01F4, 0x0000, 0x0000, 0x0000, // 20076 ~ 20090
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20091 ~ 20105
0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0258, 0x0000, 0x0000, 0x0000, // 20105 ~ 20120
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20121 ~ 20135
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20136 ~ 20150
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20151 ~ 20165
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20166 ~ 20180
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20181 ~ 20195
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, 0x015E, // 20196 ~ 20210
0x015E, 0x015E, 0x0000, 0x0000, 0x0000, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, 0x0019, // 20211 ~ 20225
0x0019, 0x0019, 0x0000, 0x0000, 0x0000, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, 0x0064, // 20226 ~ 20240
0x0064, 0x0064, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20241 ~ 20255
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20256 ~ 20270
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, // 20271 ~ 20285
0x07D0, 0x07D0, 0x0000, 0x0000, 0x0000, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, // 20286 ~ 20300
0x0032, 0x0032, 0x0000, 0x0000, 0x0000, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, // 20301 ~ 20315
0x00C8, 0x00C8, 0x0000, 0x0000, 0x0000, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, 0x00C8, // 20316 ~ 20330
0x00C8, 0x00C8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20331 ~ 20345
0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x2710, 0x0000, 0x0000, 0x0000, // 20346 ~ 20360
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20361 ~ 20375
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20376 ~ 20390
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x007F, 0x007F, 0x0190, // 20391 ~ 20405
0x0258, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20406 ~ 20420
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20421 ~ 20435
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20436 ~ 20450
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20451 ~ 20465
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20466 ~ 20480
0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, // 20481 ~ 20495
0x0000, 0x0000, 0x0000, 0x0000 // 20496 ~ 20499
};
//**************************************************************************
//
//                             Global Variable
//
//**************************************************************************
//  data storage for transmission between CU_3505(A8) and CU_ADA(M3)

//extern unsigned short thermal_couple[12];
//extern unsigned short l_temperature, r_temperature;  // 2018/12/10 by kf
extern volatile short l_temperature, r_temperature;  // 2018/12/10 by kf
extern unsigned char error_temperature;  // 2018/12/10 by kf
extern unsigned short thermal_hex[12];

extern unsigned short heat;

extern unsigned short id;
extern unsigned short tx_buffer[TX_BUFFER_LEN];
extern unsigned short tx_buffer_bk[TX_BUFFER_LEN];
extern unsigned short rx_buffer[RX_BUFFER_LEN];
extern unsigned short error_rs485;
extern unsigned short crc_temp;

extern unsigned short g_debug;
extern unsigned char err_msg;
extern unsigned short err_cnt;
extern unsigned short TEMPerr_cnt;
extern unsigned short CH_check;
//extern unsigned char b_test;
extern unsigned short rand_seed;

extern short difference;

extern unsigned char reg[4][3];

//  for other use
extern unsigned int T_DO[4];
extern unsigned int thermal[4];
extern unsigned short ch_test;

extern unsigned short j_ch1[26], j_ch2[26], j_ch3[26];
extern unsigned short k_ch1[35], k_ch2[35], k_ch3[35];
//extern unsigned char left, right;
//extern unsigned char mid;
extern int calc;

/*
These Variables below has been declared in the library file.
You should call this function by extern. These Variables are for ckecking the data from ecryption mode.
*/
extern unsigned char alpum_tx_data[8];
extern unsigned char alpum_rx_data[10];
extern unsigned char alpum_ex_data[8];

extern unsigned char _alpu_rand(void);
