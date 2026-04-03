#include "IntTm1.h"

//extern unsigned short Nthermal_couple[3][4];
extern unsigned short DnApSysFg;	// Fu 107/07/05;
extern unsigned short DelayApToBlTime;	// Fu 107/07/05
//extern unsigned short l_temperature, r_temperature, l_r_temperature;  // 2018/12/10 by kf
extern short l_temperature, r_temperature, l_r_temperature;  // 2018/12/10 by kf
extern unsigned char BURN_DETECT;
extern unsigned short TimerBase4;
extern unsigned short TimerBase5;
extern unsigned short DateCnt;
extern unsigned short StartCountFlag;
// Fu 107/01/29
//extern unsigned long Tm1Cnt;
//extern unsigned long Tm2Cnt;
//extern unsigned long Tm3Cnt;
//extern unsigned long MainCnt;
//extern unsigned short MainErrCnt;
//////////////////
//
// 250ms
//
////////////////////
void ISR_Timer1(void)
{
	if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		//
		/* Start for test program */
		//		   
		// Fu 107/08/24
		if((DnApSysFg == 0x2479u) || (*GetPtrCDM2(20559u) == 0x1368u) || (*GetPtrCDM2(20559u) == 0x2479u))
		{
				DelayApToBlTime += 1u;
				if(DelayApToBlTime >= 60000u)	// 250ms * 8  = 2 Sec
				{
					DelayApToBlTime = 60000u;
				}
		}
		else
		{
			DelayApToBlTime = 0u;
		}
		//
		TimerBase2++; // 250ms
		TimerBase4++;
		TimerBase5++; // 250ms
		// Fu 107/01/29
//		Tm1Cnt = 0;
		//
		if(TimerBase4 >= Sec11_TRIG_MS) // 2016/01/05 11sec
		{
			TimerBase4 = Sec11_TRIG_MS;
		}
		//
		if(StartCountFlag)
		{
			DateCnt++; // 2015/04/20 Perpetual Calendar Timer
		}
		//
		TempSubScan();	// ·Å«×-PID¹Bºâ
			/* End for test program */
		}
}
