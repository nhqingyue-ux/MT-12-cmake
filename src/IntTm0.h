#include "Data.h"
//////////////////////
//
//	Function
//
/////////////////////////
void ISR_Timer0(void);
//
extern void HT_OUT(unsigned short data);
void RealTpOutMd(unsigned short x, unsigned short *OrgOutPwm, unsigned short *RealOutPwm);
//
extern void TempHWSave(void);
//////////////////////////
//
//	DATA				    
//
////////////////////////// 
extern unsigned char ch_index;				  
//extern unsigned short FirstRS485RunFg;
//extern unsigned short RS485ConCnt;
extern unsigned short DataTest, DataTest1, DataTest2;
extern unsigned short Int1Sts;
extern unsigned short heat;
extern unsigned short TimerBase1;  
extern unsigned short TimerBase2;
extern unsigned short TimerBase3;
extern unsigned short TimerBase4;
extern unsigned short TimerBaseled;
extern unsigned short StepCnt;
extern unsigned short RsTxRxSts;
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
extern unsigned short TpClLpFg5;
extern unsigned short TpClLpFg4;
extern unsigned short HeatRunFg;
extern unsigned short TpClLpFg2;
extern unsigned short TpClLpFg3;
extern unsigned short HiAlmSts;
extern unsigned short LowAlmSts;
//
extern int fifoin;
//
extern unsigned short ledtab[15];
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
extern unsigned short *pA00001;
extern unsigned short *pA02001;
extern unsigned short *pA10001;
extern unsigned short *pA10129;
extern unsigned short *pA11001;
extern unsigned short *pA11501;
extern unsigned short *pA12001;
extern unsigned short *pA20001;
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
