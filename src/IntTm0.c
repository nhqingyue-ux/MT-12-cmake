#include "IntTm0.h"
//
extern unsigned short DnApSysFg;	// Fu 107/07/05
extern unsigned short TimerBase22;
extern unsigned char rx_buffer[RX_BUFFER_LEN];  // 2012/10/11
extern unsigned char tx_buffer[TX_BUFFER_LEN];  // 2012/10/11
extern unsigned short crc_temp;  // 2012/10/11
extern unsigned short id;  // 2012/10/11
extern volatile unsigned short SendRs485Fg;  // 2012/10/11
extern unsigned short *pA20001;  // 2012/10/11
extern volatile unsigned short TimerBase3;  // 2012/10/11
// Fu 107/01/29
//extern unsigned long Tm1Cnt;
//extern unsigned long Tm2Cnt;
//extern unsigned long Tm3Cnt;
//extern unsigned long MainCnt;
//extern unsigned short MainErrCnt;
unsigned short ErrorFlag = 0;
unsigned short ErrorLed = 0;
// Fu 107/01/29
extern void LenAlarm(void);  // 2012/10/11
extern void DataAlarm(void);  // 2012/10/11
extern void CrcAlarm(void);  // 2012/10/11
extern unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);  // 2012/10/11

extern float PwmCounterUnit;	// Fu 106/07/19
extern unsigned short RelayTm[12];
extern unsigned short ThermostatTm[12];
extern unsigned short ActThermostatTm[12];
//extern float OutputPwmMaxUnit;
extern float OutputPwmMaxUnit[12];	// Fu 108/01/03 : 由單段變更為多段
extern unsigned short buffer_index;  // 2012/10/11
extern unsigned short HeatSw, HwSw2, PIDLastOut;
extern unsigned short FirstToPAreaFg;	// Fu 106/05/18
unsigned short Output100Fg2=0;
extern unsigned short Output100Fg;
extern unsigned short Output100Pr[12];
//extern unsigned short thermal_couple[12];
extern volatile short thermal_couple[12];
extern unsigned short Bk_thermal_couple[12];
extern unsigned char BURN_DETECT;
extern unsigned short TemperatureOutput[12];
unsigned short FirstRsTxRxStsOKFlag = 0;
extern unsigned short ThermostatFun;
extern unsigned short ADErrorFlag;
extern volatile unsigned char adc_sampling_active;  /* B-1: freeze heater during ADC */
unsigned short TpPwmOutputSub(unsigned short Len, unsigned short *SourPwmOutPoint, unsigned short DestPwmOutPointAddress[]);
//void Package_extract(void); // 2012/10/11
////////////////////////
//
//
//
//////////////////////////
void ISR_Timer0(void)
{
	unsigned short delay;		   
	unsigned short i;	
	//	
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		// Fu 107/07/05
		if((DnApSysFg != 0x2479u) && (*GetPtrCDM2(20559u) != 0x1368u))
		{
			TimerBase22++;
			TimerBase3++; // 0.2ms
			//
			for(i=0 ;i<12; i++)
			{
				if(*(tempData[0].AuTnFun) & (0x01<<i) && (TpClLpFg3 & (0x01<<i)) && (RsTxRxSts))
				{
					if(*(tempData[0].AuTnFun) & (0x01<<i))
					{
						if(RealTpOutPwm & (0x01<<i))
						{
							*(tempData[i].PwmOutput) = 1000;	//	100.0%
						}
						else
						{
							*(tempData[i].PwmOutput) = 0;	//	0.0%
						}
					}
				}
				//
				else if((((ThermostatFun & (0x01<<i)) && (RsTxRxSts)) || ((TpClLpFg3 & (0x01<<i)) && (tempData[0].HeatErrIR == OFF) && (RsTxRxSts))) && (ADErrorFlag == OFF)) // 2015/06/02 ¥[¤J·Å«×·P´úIC²§±`§YÃö³¬¥[¼ö.
				{
						if((AlternTm[i] == 0) && (TpClLpFg3 & (0x01<<i)))
								RealTpOutMd(i, &OrgTpOutPwm[0], &RealTpOutPwm);
				}
				else 
				{
							RealTpOutPwm = RealTpOutPwm & ~(0x01<<i);
				}
			}
			//
			if((FirstRsTxRxStsOKFlag == OFF) && (TimerBase4 >= Sec11_TRIG_MS) && RsTxRxSts) // 2015/01/23
			{
				FirstRsTxRxStsOKFlag = ON;
			}
			else if(FirstRsTxRxStsOKFlag && ((RsTxRxSts == OFF) || (ADErrorFlag != OFF))) // 2015/06/02 ¥[¤J·Å«×·P´úIC²§±`§YÃö³¬¥[¼ö.
			{
				FirstRsTxRxStsOKFlag = OFF;
				*(tempData[0].HeatCtrlMd) = OFF;
			}
			//
			*(tempData[0].HeatFlashBit) = RealTpOutPwm;          // 32073 (å°„å˜´åŠæ®µæ•¸show  flash) TEMP DISPLAY FLASH
			RealTpOutPwm = TpPwmOutputSub(TpMaxLp, &RealTpOutPwm, TemperatureOutput);
			//
			if(adc_sampling_active)
				;  /* B-1: freeze heater output while ADC is sampling — skip HT_OUT */
			else if(!BURN_DETECT)
				HT_OUT(~RealTpOutPwm);  // Modify heat out
			else
				HT_OUT(0xFFFF);
		}
		else
		{
			HT_OUT(0xFFFF);
		}
		//
		// For WatchDog Count
		WD_TRIG_L;
		DELAY_CYCLES(10);
		WD_TRIG_H;
	}
}
//
//
//
void RealTpOutMd(unsigned short x, unsigned short *OrgOutPwm, unsigned short *RealOutPwm)
{
	static unsigned short RelayTmBuff[12];
	static unsigned short Bk20616Data[12];	// Fu 108/06/24
	//
	if(ThermostatFun & (0x01<<x))		        // æ†æº«æŽ§åˆ¶
	{
		*(tempData[x].PwmOutputCyc) += 1;                                      // temp run range : 0.2ms // Fu 103/04/23
		//
		if(*(tempData[x].PwmOutputCyc) <=  ThermostatTm[x])	    // 1ms --> 100ms
		{
			if(*(tempData[x].PwmOutputCyc) <= ActThermostatTm[x])	// 10ms
				*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On(PWM switch on)
			else
				*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off(PWM switch off)
		}
		else
			*(tempData[x].PwmOutputCyc) = 0;
		//
		*(tempData[x].PwmOutput) = *(tempData[x].Proportion) * 10;
	}
	else if(*(tempData[0].ThermostatFun) & 0x2000)	// 13 Bit	// Fu 105/11/02
	{
		RelayTmBuff[x] += 1;	// Fu 103/04/23
		//
		if(RelayTmBuff[x] >= 5)	// 0.2 * 5 = 1ms	// Fu 103/04/23
		{
			RelayTmBuff[x] = 0;
			//
			*(tempData[x].PwmOutputCyc) += 1;                                      // temp run range : 0.2ms	// Fu 103/04/23
			//
			if(*(tempData[x].PwmOutputCyc) <= 2000)    //  
			{
				if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])        // ThermostatTmBuf[x])
					*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
				else
					*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
					}
			else
			{
				HeatRunFg &= ~(0x01<<x);
				*(tempData[x].PwmOutputCyc) = 0;
				Bk20616Data[x] = (*(OrgOutPwm+x) * 2000)/ PwmCounterUnit;	// Save Real Output Time : 2.0 sec
			}
		}
		//	Fu 105/12/08
		*(tempData[x].PwmOutput) = (*(OrgOutPwm+x) * 1000) / PwmCounterUnit;
	}
	else if(*(tempData[0].ContrlFun) & (0x01<<x))                // 55548  RELAYæ¨¡å¼   æŽ¥è§¸å™¨æº«åº¦æŽ§åˆ¶åŠŸèƒ½é¸æ“‡(SSR  OR RELAY)
	{
		RelayTmBuff[x] += 1;	// Fu 103/04/23
		//
		if(RelayTmBuff[x] >= 5)	// 0.2 * 5 = 1ms	// Fu 103/04/23
		{
			RelayTmBuff[x] = 0;
			//
			*(tempData[x].PwmOutputCyc) += 1;                                      // temp run range : 0.2ms	// Fu 103/04/23
			//
			if(*(tempData[x].PwmOutputCyc) <= RelayTm[x])    // 55550
			{
				if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])        // ThermostatTmBuf[x])
					*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
				else
					*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
					}
			else
			{
				HeatRunFg &= ~(0x01<<x);
				*(tempData[x].PwmOutputCyc) = 0;
				Bk20616Data[x] = (*(OrgOutPwm+x) * RelayTm[x])/PwmCounterUnit;	// Save Real Output Time
			}
		}
		//	Fu 105/12/08
		*(tempData[x].PwmOutput) = (*(OrgOutPwm+x) * 1000) / PwmCounterUnit;
	}
	else
	{
		*(tempData[x].PwmOutputCyc) += 1;                                      // temp run range : 0.2ms	// Fu 103/04/23
		// Fu 108/01/03 : 可以每段獨立選擇MT12或MJ86模式
		if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<x)) == 0))	// 15 Bit : MJ86 - 12¬íPID¼Ò¦¡
		{
			if((Output100Fg2 & (0x01<<x)) != 0)	// Fu 106/05/18
			{
				if(thermal_couple[x] < TpPIDdata[x].Set)
				{
						*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
				}
				else
				{
					if(*(tempData[x].PwmOutputCyc) <= Sec2p1_TRIG_MS)			       // SSR  0.25 sec
					{
						if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])
							*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
						else
							*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
					}
					else
					{
						HeatRunFg &= ~(0x01<<x);
						*(tempData[x].PwmOutputCyc) = 0;			                    // reture
						Bk20616Data[x] = (*(OrgOutPwm+x)*Sec2p1_TRIG_MS) / 4095;	// Save Real Output Time(1 sec)
					}
				}
			}
			else
			{
				if(*(tempData[x].PwmOutputCyc) <= Sec2p1_TRIG_MS)			       // SSR  0.25 sec
				{
					if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])
						*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
					else
						*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
				}
				else
				{
					HeatRunFg &= ~(0x01<<x);
					*(tempData[x].PwmOutputCyc) = 0;			                    // reture
					Bk20616Data[x] = (*(OrgOutPwm+x)*Sec2p1_TRIG_MS) / 4095;	// Save Real Output Time(1 sec)
				}
				//
				*(tempData[x].PwmOutput) = (*(OrgOutPwm+x) * 1000) / 4095;			
			}
		}
		else
		{
			if((Output100Fg2 & (0x01<<x)) != 0)	// Fu 106/05/18
			{
				if(thermal_couple[x] < TpPIDdata[x].Set)
				{
						*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
				}
				else
				{
					if(*(tempData[x].PwmOutputCyc) <= Sec025_TRIG_MS)			       // SSR  0.25 sec
					{
						if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])
						{
							*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
						}
						else
						{
							*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
						}
					}
					else
					{
						HeatRunFg &= ~(0x01<<x);
						*(tempData[x].PwmOutputCyc) = 0;			                    // reture
						Bk20616Data[x] = (*(OrgOutPwm+x)*Sec025_TRIG_MS) / PwmCounterUnit;	// Save Real Output Time(1 sec)
					}
				}
			}
			else
			{
				if(*(tempData[x].PwmOutputCyc) <= Sec025_TRIG_MS)			       // SSR  0.25 sec
				{
					if(*(tempData[x].PwmOutputCyc) <= Bk20616Data[x])
					{
						*(RealOutPwm) = (*(RealOutPwm) | (0x01<<x));	    // Power On
					}
					else
					{
						*(RealOutPwm) = (*(RealOutPwm) & ~(0x01<<x));	    // Power Off
					}
				}
				else
				{
					HeatRunFg &= ~(0x01<<x);
					*(tempData[x].PwmOutputCyc) = 0;			                    // reture
					Bk20616Data[x] = (*(OrgOutPwm+x)*Sec025_TRIG_MS) / PwmCounterUnit;	// Save Real Output Time(1 sec)
				}
				//
				*(tempData[x].PwmOutput) = (*(OrgOutPwm+x) * 1000) / PwmCounterUnit;			
			}
		}
	}
}
/////////////////////////
//
//
unsigned short TpPwmOutputSub(unsigned short Len, unsigned short *SourPwmOutPoint, unsigned short DestPwmOutPointAddress[])
{
	unsigned short BkSourPwmOutPoint;
	unsigned short i;
	//
	BkSourPwmOutPoint = *SourPwmOutPoint;	// ³Æ¦sPWM¿é¥Xª¬ºA
	// 
	for(i = 0; i < Len; i++)
	{
		if(BkSourPwmOutPoint & (0x0001<<i))
			*SourPwmOutPoint |= (0x0001<<DestPwmOutPointAddress[i]);
		else
			*SourPwmOutPoint &= ~(0x0001<<DestPwmOutPointAddress[i]);
	}
	//
	return *SourPwmOutPoint;
}
