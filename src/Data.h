#ifndef __DATA_H__
#define __DATA_H__

//#define TESTING  // 2018/12/10 by kf

#ifdef TESTING
#define EEPROM_TEST
#define ALPU_TEST
#endif

#define MUX_ENABLE
#ifdef MUX_ENABLE
#define I2C1_MUX
#define I2C2_MUX
#endif

#define TIMER_ENABLE
#define RS485_ENABLE
#define EXTI_ENABLE
#ifdef TIMER_ENABLE
#define TIMER0_ENABLE
#define TIMER1_ENABLE
#define TIMER2_ENABLE
#define TIMER3_ENABLE
#endif

//**************************************************************************
//
//                            Include File
//
//**************************************************************************
// #include "provide.h"
#include "misc.h"
#include "stm32f4xx.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_syscfg.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "provide.h"
//**************************************************************************
//
//                             Define
//
//**************************************************************************
#define ISR_Timer0 TIM2_IRQHandler  // 2015/03/09 by kf
#define ISR_Timer1 TIM3_IRQHandler  // 2015/03/09 by kf
#define ISR_Timer2 TIM4_IRQHandler  // 2015/03/09 by kf
#define ISR_Timer3 TIM5_IRQHandler  // 2015/03/09 by kf
#define BURN_DETECTED EXTI15_10_IRQHandler  // 2015/03/09 by kf
#define ISR_RS485 USART1_IRQHandler  // 2015/03/09 by kf
#define BURNOUT_CNT 500
#define CONV_PER_BURNOUT 3
#define VBIAS_TIME 2  // in fact, 5 ms(TIMER1_BASE) * VBIAS_TIME  // 2012/03/15
#define BURNOUT_TIME 2  // in fact, 5 ms(TIMER1_BASE) * BURNOUT_TIME  // 2012/03/15
#define AuWeekCnt			7
#define TempTm5MinUint  	(5*60*1000)/TIMER1_TRIG_MS
#define TempTm30MinUint  	(30*60*1000)/TIMER1_TRIG_MS
#define TpMaxLp		12
#define OFF			0
#define ON			1
#define Y_MAX		128
#define M_MAX		8192
#define X_MAX		128
#define KEY_MAX		128
#define T_MAX		512
#define C_MAX		256
#define F_MAX		8192
#define SYS_M_MAX	32
#define TR_MAX		32
#define STS_RES_SIZE	((0x1000*8)-(Y_MAX+M_MAX+X_MAX+KEY_MAX+T_MAX+C_MAX+F_MAX+SYS_M_MAX+TR_MAX))
#define REG_RES_SIZE	((0x3000/2)-(T_MAX*3+C_MAX*4)-3)	//�џl(�O�d)
//
#define Y_offset	0
#define M_offset	Y_MAX/16 + Y_offset
#define X_offset	M_MAX/16 + M_offset
#define K_offset	X_MAX/16 + X_offset
#define T_offset	KEY_MAX/16 + K_offset
#define C_offset	T_MAX/16 + T_offset
#define F_offset	C_MAX/16 + C_offset
//
#define TmCv_offset		0x800				//unsigned short offset	0x800
#define CtCv_offset		TmCv_offset + T_MAX	//unsigned short offset	0xA00
#define TmPv_offset		CtCv_offset + C_MAX	//unsigned short offset	0xB00
#define CtPv_offset		TmPv_offset + T_MAX	//unsigned short offset	0xD00
#define TmMmi_offset	CtPv_offset + C_MAX	//unsigned short offset	0xE00
#define CtMmi_offset	TmMmi_offset + T_MAX//unsigned short offset	0x1000
//
#define MCSJ_MAX		1024			// WMLAD�i�s��ϥ�MCSJ���̀j��
#define OSH_TMP_SIZE		(Y_MAX+M_MAX)	// >(Y+M)
#define STATE_TMP_SIZE		(M_MAX)			// >(M)
#define MCSJ_TMP_SIZE		(MCSJ_MAX)		// > MCSJ (��WMLAD����̀j�ϥΌ�)
#define LAD_STACK_SIZE		256				// unsigned short,�@����|��
#define MCS_STACK_SIZE		256				// unsigned short,MCSJ���|��
#define ADS1248_MAX_ZERO_COUNT 10  // 2015/05/29 by kf

//#define TIMER0_TRIG_MS 		5	// 5ms
//#define TIMER0_TRIG_MS 		2.5	// 5ms
//#define TIMER1_TRIG_MS 		250	// 250ms
//#define TIMER2_TRIG_MS		0.2 // 0.2ms
#define TIMER0_TRIG_MS 		0.2	
#define TIMER1_TRIG_MS 		250	
#define TIMER2_TRIG_MS		5 
#define TIMER3_TRIG_MS		0.5
#define Sec1_TRIG_MS 		1000/TIMER0_TRIG_MS	// 1 sec
#define Sec10_TRIG_MS 		10000/TIMER0_TRIG_MS	// 1 sec
// #define Sec10_TRIG_MS 		10000/TIMER2_TRIG_MS	// 1 sec
#define Sec025_TRIG_MS 		250/TIMER0_TRIG_MS	// 0.25 sec
#define Sec2p1_TRIG_MS 		2100/TIMER0_TRIG_MS	// 2.1 sec
#define Sec0005_TRIG_MS 	5/TIMER0_TRIG_MS	// 5ms
#define Sec0010_TRIG_MS 	10/TIMER0_TRIG_MS	// 10ms
#define Sec0030_TRIG_MS 	30/TIMER2_TRIG_MS	// 30ms
#define TIMEOUT_RS485 		10000
#define RX_BUFFER_LEN 		512
#define TX_BUFFER_LEN 		512
#define AddressBase 		20000
#define AddressBaseOffset 	20001
#define AddressBase0 		9999
#define Tm1Sec1_TRIG_MS 	1000/TIMER1_TRIG_MS	// 1 sec
#define Sec11_TRIG_MS     11000/TIMER1_TRIG_MS // 11 sec
#define Sec0100_TRIG_MS 	100/TIMER0_TRIG_MS	// 100ms
//
// Macro Defined by David
#define GetPtrCDM2(x)		(pA20001+(x-20001))
#define GetPtrCDM3(x)		(pA30001+(x-30001))
//#define GetIRBit(x)			((((SplcSts.M[(x-2001)/16] & (1<<(x-2001)%16)))>>((x-2001)%16)) & 0x01)
//#define GetFunBit(x)		((((SplcSts.F[(x-1)/16] & (1<<(x-1)%16)))>>((x-1)%16)) & 0x01)
//#define SetIRBit(x)			(SplcSts.M[(x-2001)/16] |= (1<<(x-2001)%16))
//#define ClrIRBit(x)			(SplcSts.M[(x-2001)/16] &= ~(1<<(x-2001)%16))
//#define SetTmBit(x)			(SplcSts.T[(x-1)/16] |= (1<<(x-1)%16))
//#define ClrTmBit(x)			(SplcSts.T[(x-1)/16] &= ~(1<<(x-1)%16))
//#define SetCtBit(x)			(SplcSts.C[(x-1)/16] |= (1<<(x-1)%16))
//#define ClrCtBit(x)			(SplcSts.C[(x-1)/16] &= ~(1<<(x-1)%16))
//#define GetTmPv(x)			(SplcSts.TmPv[x-1])
//#define GetTmCv(x)			(SplcSts.TmCv[x-1])
//#define GetPtrTm(x)			(pA20001+(x-40001))
//#define GetCtPv(x)			(SplcSts.CtPv[x-1])
//#define GetCtCv(x)			(SplcSts.CtCv[x-1])
//#define GetPrtCv(x)			(pA20001+(x-40001))
//													    
#include "delay.h"  // DELAY_CYCLES macro

// for Thermocouple
#define T_SCLK_H GPIO_SetBits(GPIOA, GPIO_Pin_5)
#define T_SCLK_L GPIO_ResetBits(GPIOA, GPIO_Pin_5)
#define T_DIN_H GPIO_SetBits(GPIOA, GPIO_Pin_7)
#define T_DIN_L GPIO_ResetBits(GPIOA, GPIO_Pin_7)
#define T_START_H GPIO_SetBits(GPIOD, GPIO_Pin_9)
#define T_START_L GPIO_ResetBits(GPIOD, GPIO_Pin_9)
#define T_CS1_H GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define T_CS1_L GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define T_CS2_H GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define T_CS2_L GPIO_ResetBits(GPIOB, GPIO_Pin_12)
#define T_CS3_H GPIO_SetBits(GPIOA, GPIO_Pin_15)
#define T_CS3_L GPIO_ResetBits(GPIOA, GPIO_Pin_15)
#define T_CS13_H GPIO_SetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_15)
#define T_CS13_L GPIO_ResetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_15)

// for room temperature
#define ROOM_TEMPERATURE_UPPER_LIMIT	1000  // 2018/12/10 by kf
#define ROOM_TEMPERATURE_LOWER_LIMIT	-400  // 2018/12/10 by kf

// for Heat
#define HT_CS_H GPIO_SetBits(GPIOC, GPIO_Pin_5)
#define HT_CS_L GPIO_ResetBits(GPIOC, GPIO_Pin_5)
#define WD_TRIG_H GPIO_SetBits(GPIOD, GPIO_Pin_13)
#define WD_TRIG_L GPIO_ResetBits(GPIOD, GPIO_Pin_13)
#define BURN_RST_H GPIO_SetBits(GPIOD, GPIO_Pin_11)
#define BURN_RST_L GPIO_ResetBits(GPIOD, GPIO_Pin_11)

// for other GPIO pins
#define USART1_RTS_H GPIO_SetBits(GPIOA, GPIO_Pin_12)
#define USART1_RTS_L GPIO_ResetBits(GPIOA, GPIO_Pin_12)

// for LED_RED and LED_GRN
#define LED_RED_H GPIO_SetBits(GPIOG, GPIO_Pin_5)
#define LED_RED_L GPIO_ResetBits(GPIOG, GPIO_Pin_5)
#define LED_GRN_H GPIO_SetBits(GPIOG, GPIO_Pin_2)
#define LED_GRN_L GPIO_ResetBits(GPIOG, GPIO_Pin_2)
//
//
//                Address define for FPGA functions
//
// Only one of follow two items can be choice, the other must be mask
//#define J_TYPE
#define K_TYPE

//  number of elements for j-type and k-type record
#define j_element 26
#define k_element 35
//  define the first element of j- or k-table in temperature
#define j_start (-2000L)
#define k_start (-1600L)
//  define the scale between each temperature record
#define temp_scale (400UL)
//
/*struct NEW_MLAD_STS
{										
	//0x0000 unsigned shorts, bytes
	unsigned short Y[Y_MAX/16];						// 0   ~7    unsigned shorts (0x0000 ~ 0x0007)
	unsigned short M[M_MAX/16];						// 8   ~519  unsigned shorts (0x0008 ~ 0x0207)
	unsigned short X[X_MAX/16];						// 520 ~527  unsigned shorts (0x0208 ~ 0x020F)
	unsigned short K[KEY_MAX/16];					// 528 ~535  unsigned shorts (0x0210 ~ 0x0217)
	unsigned short T[T_MAX/16];						// 536 ~567  unsigned shorts (0x0218 ~ 0x0237)
	unsigned short C[C_MAX/16];						// 568 ~583  unsigned shorts (0x0238 ~ 0x0247)
	unsigned short F[F_MAX/16];						// 584 ~1095 unsigned shorts (0x0248 ~ 0x0447)
	unsigned short F_SM[SYS_M_MAX/16];              // 1096~1097 unsigned shorts (0x0448 ~ 0x0449)
	unsigned short F_TR[TR_MAX/16];					// 1098~1099 unsigned shorts (0x044A ~ 0x044B)
	unsigned short StsRes[STS_RES_SIZE/16];			// ���ABIT�O�d��, 1100~2047 unsigned shorts (0x044C ~ 0x07FF)

	// *** Ladder�ь�Register�� 0x1000~0x3fff
	//0x1000 bytes
	unsigned short TmCv[T_MAX];						// Timer Current Value 			(0x0800 ~ 0x09FF)
	unsigned short CtCv[C_MAX];						// Counter Current Value 		(0x0A00 ~ 0x0AFF)
	unsigned short TmPv[T_MAX];						// Timer Preset Value			(0x0B00 ~ 0x0CFF)
	unsigned short CtPv[C_MAX];						// Counter Preset Value			(0x0D00 ~ 0x0DFF)
	unsigned short TmMmi[T_MAX];					// Timer Preset Value from MMI	(0x0E00 ~ 0x0FFF)
	unsigned short CtMmi[C_MAX];					// Counter Preset Value from MMI(0x1000 ~ 0x10FF)
	unsigned short CtPfl[C_MAX];					// Counter ???Power Flow???
	unsigned short RegRes[REG_RES_SIZE-1];			// ���REG�O�d��
	unsigned short T0001;							// TimeBase 0.001S
	unsigned short T001;							// TimeBase 0.01S
	unsigned short T01;								// TimeBase 0.1S
	unsigned short T1;								// TimeBase 1S
	//0x3FFF ----End
	//
	// *** Ladder OneShot&���|�� 0x4000~, total
	//0x4000
	unsigned short OshTmp[OSH_TMP_SIZE/16];			// ��mCoil(Y,M)���A�܀�
	unsigned short StateTmp[STATE_TMP_SIZE/16];		// ��mState���A�܀�
	unsigned short McsjTmp[MCSJ_TMP_SIZE/16];		// ��mMCSJ���A�܀�
	unsigned short LadStack[LAD_STACK_SIZE];		// Ladder ���A���|��
	unsigned short McsStack[MCS_STACK_SIZE];		// MCSJ�PSTATE��D1���|��
	// �s�W�[���ъ��إ�
};*/
//
//
//
struct TpPID_DATA
{
	unsigned short *HeatSet;						// 1
	unsigned short *PerHeatSet;					// 2
	unsigned short *PIDp;								// 3
	unsigned short *PIDi;								// 4
	unsigned short *PIDd;								// 5
	unsigned short *HiAlarm;						// 6
	unsigned short *LoAlarm;						// 7
	unsigned short *TpControl;					// 8
	unsigned short *TpDisplay;					// 9
	unsigned short *PwmOffSet;					// 10
	unsigned short *ThermostatFun;			// 11
	unsigned short *Thermostat;					// 12
	unsigned short *Proportion;					// 13
	unsigned short *TpConAddHi;					// 13
	unsigned short *Synchron;						// 15
  unsigned short *SynchronFun;				// 16
  unsigned short *TwoUpOffset;				// 17
  unsigned short *ContrlFun;					// 18
  unsigned short *CycleTm;						// 19
  unsigned short *AuTnFun;						// 20
  unsigned short *HeatCtrlMd;					// 21
  unsigned short *HeatTm;							// 22
  unsigned short *HeatPerTm;					// 23
  unsigned short *HeatHour;						// 24
  unsigned short *HeatMin;						// 25
  unsigned short *CoolSet;						// 26
  unsigned short *OilHeat;						// 27
  unsigned short *HeatWeek;						// 28
  unsigned short  HeatOKSts;					// 29
  unsigned short  HeatHiSts;					// 30
  unsigned short  HeatLowSts;					// 31
  unsigned short  PlcHeatOn;					// 32
  unsigned short  SysHeatOn;					// 33
  unsigned short  PlcHeatOff;					// 34
  unsigned short  SysHeatOff;					// 35
  unsigned short  HoldSts;						// 36
  unsigned short  HeatSW;							// 37
  unsigned short  HeatPerSW;					// 38
  unsigned short  TpCoolIR;						// 39
  unsigned short  OilHiSts;						// 40
  unsigned short  OilHeatIR;					// 41
  unsigned short  *HiOilSet;					// 42
  unsigned short  TempCurIR;					// 43
  unsigned short  *KAndJSel;      		// 44
  unsigned short  *HeatFlashBit;			// 45
  unsigned short  HeatErrIR;					// 46
  unsigned short  *TwoUpFun;					// 47
  unsigned short  AllTpRdyIR;					// 48
  unsigned short  *HeatErrSts;				// 49
  unsigned short  *HiAlmSts;					// 50
  unsigned short  *LowAlmSts;					// 51
  unsigned short  *PwmOutput;					// 52
	unsigned short  *TempLinearErr;			// 53
	unsigned short  *AutoTingSts;				// 54
	unsigned short  *TpVersion;					// 55
	unsigned short  *TwoHeatMd;					// 56
	unsigned short  *TwoHeatPercentMd;
	unsigned short  *TwoHeatTm;					// 57
	unsigned short  *TwoHeatTp;					// 58
	unsigned short  *AllTmAuTpSw;				// 59
	unsigned short  *SlowTpUpSw;				// 60
	unsigned short  *TpSensorSour;			// 61
	unsigned short  *TpSensorDest;			// 62
	unsigned short  *TpPointSour;				// 63
	unsigned short  *TpPointDest;				// 64
	unsigned short  *TpChgError;				// 65
	unsigned short  *VariableBaud;
	unsigned short	*Week;
	unsigned short	*Sec;
	unsigned short	*Min;
	unsigned short	*Hour;
	unsigned short	*Day;
	unsigned short	*Month;
	unsigned short	*Year;
	unsigned short	*AutoHeatMainSwitch;
	unsigned short	*AutoHeatBranchSwitch;
	unsigned short	*AutoPerHeatBranchSwitch;	
	unsigned short	*HeatWattUnit;	// Fu 108/12/30 : �[�J�ƥ��C�q�ūת��[���˯S��
	//
	unsigned short *PwmOutputCyc;
	unsigned short DirFg;
	float P;
	float I;
	float D;
	float Pturn;	// P????
	float Isum;		// I?????�
	float Isum1;	// I???
	float Iturn;	// I????
	float Dturn;	// D????
	float Err;		// ????
	unsigned short  OldCurr;	// ????
	float Out;		// PID??
	unsigned short  Set;		// ????
	float SetBk;	// ??????
	unsigned short  Curr;		// ????
	float Limit;	// ????
	unsigned short  OldControl;	// ????
};
//
struct TempSpMathTab
{
	unsigned short 	DeltaCv_head;
	unsigned short  	DeltaCv_tail;
	int	    DeltaCvTotal;
	unsigned short     Lpm_BK;
	int 	DeltaCv[64];
	int    DctMb_ch[64];
	unsigned short UpMax;
	unsigned short DnMin;
};
#endif
