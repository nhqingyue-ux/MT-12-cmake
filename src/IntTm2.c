#include "Data.h"
unsigned short timer2_cnt = 0;

extern unsigned char rx_buffer[RX_BUFFER_LEN];
extern unsigned char tx_buffer[TX_BUFFER_LEN];
extern unsigned short crc_temp;
extern unsigned short id;
extern unsigned short SendRs485Fg;
extern unsigned short *pA20001;
extern unsigned short TimerBase1;
extern unsigned short TimerBase2;
extern unsigned short  TimerBase3;
extern unsigned short TimerBase22;
extern unsigned short RealTpOutPwm;
extern unsigned char BURN_DETECT;
// Fu 107/01/29
//extern unsigned long Tm1Cnt;
//extern unsigned long Tm2Cnt;
//extern unsigned long Tm3Cnt;
//extern unsigned long MainCnt;
//extern unsigned short MainErrCnt;

extern void TempHWSave(void);
//	Fu 106/07/24
extern unsigned long TEMP_BASE_0p1S;
extern struct TpPID_DATA TpPIDdata[12], tempData[12];
//
//void HT_OUT(unsigned short);

void ISR_Timer2(void)
{
	static unsigned short TimeBase0p1Sec=0;
	//
	if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		//
// Fu 107/01/29
//		Tm2Cnt = 0;	// !!!!!!!
		//	Fu 106/07/24
		if((*(tempData[0].HeatCtrlMd) & 0x000b) != 0)
		{
			TimeBase0p1Sec += 1;
			if(TimeBase0p1Sec >= 20)
			{
				TEMP_BASE_0p1S += 1;
				TimeBase0p1Sec = 0;
			}
		}
		else
		{
			TimeBase0p1Sec = 0;
			TEMP_BASE_0p1S = 0;
		}
		//
		TempHWSave();  	// µwÅé±½´y
	}
}
/////////////////			    
//
//////////////////
void HT_OUT(unsigned short data)
{
	unsigned short p_b, p_e, p_f, p_g;
	unsigned short i;
	p_b = data & 0x07;
	p_e = (data & 0x0C00) >> 3;
	p_f = (data & 0x00F8) << 8;
	p_g = (data & 0x0300) >> 8;
	HT_CS_L;
	p_b = (GPIO_ReadOutputData(GPIOB) & 0xFFF8) | p_b;
	p_e = (GPIO_ReadOutputData(GPIOE) & 0xFE7F) | p_e;
	p_f = (GPIO_ReadOutputData(GPIOF) & 0x07FF) | p_f;
	p_g = (GPIO_ReadOutputData(GPIOG) & 0xFFFC) | p_g;
	GPIO_Write(GPIOB, p_b);
	GPIO_Write(GPIOE, p_e);
	GPIO_Write(GPIOF, p_f);
	GPIO_Write(GPIOG, p_g);
	for(i = 0; i < 10; i++);
	HT_CS_H;
}
