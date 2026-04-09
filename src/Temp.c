#include "Temp.h"
//
void TempSpUpDnMath(unsigned short i);
//
//	Fu 106/07/24
void TempAutoDateRun(void);
void TEMP_OUTPUT_MD(unsigned short i);
void PID_COUNTER_MD(unsigned short i);
unsigned short PidMathTime[12]={0};
unsigned short AUTO_TUNNING[12]={2,2,2,2,2,2,2,2,2,2,2,2};
unsigned short  AUTO_COUNTER[12]={0};
unsigned short TP_OVER_FG2[12]={0};
unsigned short AUTO_GET_CURR2[12][12]={0};
unsigned long AUTO_TUNNING_TM[12][12]={0};
unsigned long TEMP_BASE_0p1S=0;
unsigned short POWER_ON_OFF_FG=0;
unsigned short AUTO_FG3=0;
unsigned short CHANGE_ON_OFF=0;
unsigned short AUTO_FG2=0;
//
extern float PwmCounterUnit;	// Fu 106/07/19
unsigned short Old20514=0;	// Fu 107/01/08
unsigned short Old20515=0;	// Fu 107/01/08
unsigned short AuTuFirstFg=0;
unsigned short AuTnFunSw=0;
unsigned short TpAuStopFg=0;
unsigned short Output100Fg=0;
unsigned short TowTpFg=0;
unsigned short TwoUpFun=0;
unsigned short FullTimeTuningFlag=0;
extern unsigned short TpCutFg;
unsigned short TpRangCnt=0;
unsigned short TpRangUsedFg=0;
unsigned short TpRangFg=0;
unsigned short TwoHeatMdFinishFlag=0;
unsigned short SecTempSw=0;
unsigned short TpBkBuffCnt=0;
unsigned short TpChgOverFg=0;
unsigned short CntFgSts=0;
unsigned short FirstToSetUnitFg=0;
extern unsigned short InitBellBuffFg;
unsigned short TpBkBuffData[12][32];
unsigned int TempTm5Min3[12];
extern unsigned short TpCutFgHex[TpMaxLp];
extern unsigned short PID2PTmP[12];
extern unsigned short PIDTpCurrBkP[12];
extern unsigned short PID2PTmI[12];
extern unsigned short PID2PTmD[12];
extern unsigned short PIDTpCurrBkI[12];
extern unsigned short Bk_thermal_couple[12];
unsigned short PIDTpCurrBkD[12];
unsigned short PIDTpCurrBkD2[12];
unsigned short Output100Pr[12];
float Temperature_Up_Error[12] = {0};
float Temperature_Up_Kp1[12] = {0};
float Temperature_Up_Kp2[12] = {0};
unsigned short BkTempSpRamp[12]={0};
unsigned short OldBkTempSpRamp[12]={0};
float FloatKd[12]={0};
extern  unsigned short TempSetUnit[12][50];
unsigned short Temperature_Up_Flag = 0;
unsigned short Temperature_First_Up_Limit_Flag = 0;
//extern float OutputPwmMaxUnit;
extern float OutputPwmMaxUnit[12];	// Fu 108/01/03 : �ѳ�q�ܧ󬰦h�q
extern unsigned short ThermostatFun;
extern struct TempSpMathTab TempUpDnSpMathTab[TpMaxLp];	// Fu 106/05/16
extern unsigned short BkPidPUnit[TpMaxLp];
extern unsigned short BkPidIUnit[TpMaxLp];
extern unsigned short BkPidDUnit[TpMaxLp];
/* A-1: Median-of-3 filter — immune to single-sample spikes */
static unsigned short med_buf[TpMaxLp][3];
static unsigned char  med_idx = 0;
static unsigned char  med_fill = 0;  /* counts up to 3 during init */
extern unsigned short TwoHeatSetTm[12];
extern unsigned short thermal_hex[12];
struct TempSpMathTab TempUpDnSpMathTab[TpMaxLp];	// Fu 106/05/16
//
//	�ūױ��y��
//	250ms
//
void TempSubScan(void)
{
	unsigned short i, TowTpFg=0, cnt, TperrorUnit;
	unsigned short Temp, Temp1, Temp2;
	unsigned short val, TpCurrUnit;
	unsigned long TpDisplayBk;
	//unsigned short PwmOffsetOut;
	unsigned short ErrArea;
		//
		TempAutoDateRun();
		//
	  if((*(tempData[0].HeatCtrlMd) & 0x0003) <= 2) // 2015/04/20
			Temp1 = *(tempData[0].HeatCtrlMd) & 0x03;  //  MMI 45819 / select heating status	(0 ~ 1 Bits)
	  else
			Temp1 = 0;
    Temp2 = (*(tempData[0].HeatCtrlMd) & 0x08)>>3;  //  MMI 45819 / select heating status	(2 ~ 3 Bits)
		if(Temp1)
			Temp = Temp1;
		else if(Temp2)
			Temp = Temp2;
		else
		{
			Temp = OFF;
			TpRangCnt = OFF;	// Fu 104/07/27
			TpRangUsedFg = OFF; // 2015/09/07
			TpRangFg = OFF; // 2015/09/16
			TwoHeatMdFinishFlag = OFF; // 2016/01/07
		}
		//
    SecTempSw = ~((*tempData[0].HeatCtrlMd>>4) & 0xfff);  //  MMI 45819 / select heating status ( 4 ~ 11 Bits)
    //
    TpClLpEn++;
		//
  	TpClLpEn = 0;
    TpClLpFg2 = 0;
    //
		TpBkBuffCnt++;
		if(TpBkBuffCnt >= 30)
			TpBkBuffCnt = 0;
		//
		//	Fu 106/05/16
		//
		for(i=0; i<TpMaxLp; i++)
		{
			if((Temp != 0u) && (*(tempData[i].TpDisplay) != 0) && (*(tempData[i].TpDisplay) != 0xffff))
			{
				TempSpUpDnMath(i);
			}
			else
			{
				TempUpDnSpMathTab[i].Lpm_BK =  *(tempData[i].TpDisplay);
				TempUpDnSpMathTab[i].UpMax = 0;
				TempUpDnSpMathTab[i].DnMin = 0xffff;
			}
		}
		//
    for(i=0; i<TpMaxLp; i++)
    {
      	TpCurrUnit = thermal_couple[i];
				//	Fu 106/06/14
				if((TpCurrUnit > 32768) && (TpCurrUnit != 0xffff))
				{
						TpCurrUnit = 0;
				}
				/* A-1: Slew rate limit — reject single-sample spikes */
				/* A-1: Median-of-3 filter */
				if(TpCurrUnit != 0xffff && TpCurrUnit != 12000) {
					med_buf[i][med_idx] = TpCurrUnit;
					if(med_fill >= 3) {
						/* Sort 3 values, pick median */
						unsigned short a = med_buf[i][0], b = med_buf[i][1], c = med_buf[i][2];
						if(a > b) { unsigned short t=a; a=b; b=t; }
						if(b > c) { unsigned short t=b; b=c; c=t; }
						if(a > b) { unsigned short t=a; a=b; b=t; }
						TpCurrUnit = b;  /* median */
					}
				}
				if(i == TpMaxLp - 1) {
					med_idx = (med_idx + 1) % 3;
					if(med_fill < 3) med_fill++;
				}
				//
				Bk_thermal_couple[i] = TpCurrUnit;
				// Fu 105/09/21  // KK: The temperature change over a level. NO use.
				if(((TpCurrUnit+100u) >= (TpPIDdata[i].OldCurr+150)) || ((TpCurrUnit+100u) <= (TpPIDdata[i].OldCurr+50)))
				{
					TpChgOverFg |= (0x01<<i);
				}
				else
				{
					TpChgOverFg &= ~(0x01<<i);
				}
		//
		//*(tempData[i].HeatSet)=HeatSetUnit;
		//
        if((Temp==2) || (Temp==3))  // PerHeat model
          	TpSetUint = *(tempData[i].PerHeatSet);
        else
            TpSetUint = *(tempData[i].HeatSet);
		//
		if(TpSetUint > 6000)
			TpSetUint = 0xffff;
		//
		if((Temp) && (TpSetUint != 0) && (TpSetUint != 0xffff) && (BkPidPUnit[i] != 0xffff) && (BkPidPUnit[i]) && (TpCurrUnit <= 5800) && (SecTempSw & (0x01<<i)) && (*(tempData[0].AuTnFun) == 0))
		{
			
			if(*(tempData[0].TwoHeatMd) & (0x01<<i)) 
			{
				TwoHeatSetTm[i]++;
				if(*(tempData[0].TwoHeatPercentMd) & (0x01<<i))
				{
					if(*(tempData[i].TwoHeatTp) >= 100)
					{
						*(tempData[i].TwoHeatTp) = 100;
					}
					else if(*(tempData[i].TwoHeatTp) <= 1)
					{
						*(tempData[i].TwoHeatTp) = 1;
					}
					//
					if((TwoHeatSetTm[i] < (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))) && ((TpCurrUnit+BkPidPUnit[i]) < TpSetUint))
					{
						TpSetUint = *(tempData[i].HeatSet) * (*(tempData[i].TwoHeatTp) / 100.);
					}
					else
					{
						TwoHeatSetTm[i] = (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm));
					}
				}
				else
				{
				  if((TwoHeatSetTm[i] < (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))) && ((TpCurrUnit+BkPidPUnit[i]) < TpSetUint))
					{
					  TpSetUint = *(tempData[i].TwoHeatTp);
					}
				  else
					{
					  TwoHeatSetTm[i] = (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm));
					}
				}
			}
			else if((*(tempData[0].SlowTpUpSw) & (0x01<<i)) && !(*(tempData[0].ThermostatFun) & (0x01<<i)))
			{
				TwoHeatSetTm[i]++;
				//
				if((TwoHeatSetTm[i] < (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))) && ((TpCurrUnit+BkPidPUnit[i]) < TpSetUint))
				{
					ThermostatFun |= (0x01<<i);
				}
				else
				{
					TwoHeatSetTm[i] = (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm));
				}
			}
			else
			{
				TwoHeatSetTm[i] = OFF;
			}
		}
		else
		{
			TwoHeatSetTm[i] = OFF;
		}
		//
		if(TpSetUint == 0xffff)
		{
			CntFgSts |= (0x01<<i);
			for(cnt=0; cnt<30; cnt++)
				TpBkBuffData[i][cnt] = 0xffff;
		}
		/*else if(TpSetUint == 0)
		{
			CntFgSts |= (0x01<<i);
			for(cnt=0; cnt<30; cnt++)
				TpBkBuffData[i][cnt] = TpCurrUnit; // 2015/01/23
		}
		else if(TpCurrUnit == 12000)
		{
			CntFgSts |= (0x01<<i);
			for(cnt=0; cnt<30; cnt++)
				TpBkBuffData[i][cnt] = 12000;
		}
		else if(TpChgOverFg & (0x01<<i))	// Fu 105/09/21
		{
			CntFgSts |= (0x01<<i);
			for(cnt=0; cnt<30; cnt++)
				TpBkBuffData[i][cnt] = TpCurrUnit; // 2015/01/23
			TpChgOverFg &= ~(0x01<<i);
		}
		else
		{
			if(CntFgSts & (0x01<<i))
			{
				for(cnt=0; cnt<30; cnt++)
					TpBkBuffData[i][cnt] = TpCurrUnit;
				CntFgSts &= ~(0x01<<i);
			}
			else
				TpBkBuffData[i][TpBkBuffCnt] = TpCurrUnit;
		}*/
		else	// Fu 106/02/21
		{
			for(cnt=0; cnt<30; cnt++)
				TpBkBuffData[i][cnt] = TpCurrUnit;
		}
		TpDisplayBk = 0;
		//
		for(cnt=0; cnt<30; cnt++)			// 10 * 250ms = 2.5 sec
			TpDisplayBk += TpBkBuffData[i][cnt];
		//
		*(tempData[i].TpControl) = TpSetUint;
		*(tempData[i].TpDisplay) = (unsigned short)(TpDisplayBk/30);
		*(tempData[i].TpConAddHi) = *(tempData[i].TpControl) +  *(tempData[i].HiAlarm);
		//
		if((TpSetUint == 0) || (TpSetUint == 0xffff) || (BkPidPUnit[i] == 0xffff) || (BkPidPUnit[i] ==0 ) || (Temp == 0) || (Temp2 && (!((SecTempSw>>i) & 0x01))))
		{
				HiAlmSts = HiAlmSts & ~(0x01<<i);
				LowAlmSts = LowAlmSts & ~(0x01<<i);
		}
		else
		{
				val =  *(tempData[i].TpControl) >  *(tempData[i].LoAlarm) ? *(tempData[i].TpControl) - *(tempData[i].LoAlarm) : 0;
				if(*(tempData[i].TpDisplay) < val)
						LowAlmSts = LowAlmSts | (0x01<<i);  // 32652
				else if (*(tempData[i].TpDisplay) > (*(tempData[i].TpControl) + *(tempData[i].HiAlarm)))
						HiAlmSts = HiAlmSts | (0x01<<i);    // 32650
				else
				{
						LowAlmSts = LowAlmSts & ~(0x01<<i);
						HiAlmSts = HiAlmSts & ~(0x01<<i);
				}
		}
		//
		//	Fu 101/07/21
		//
		if((*(tempData[0].ThermostatFun) & (0x01<<i)) && (Temp))
		{
			ThermostatFun |= (0x01<<i);
		}
		else
		{
			if(!(*(tempData[0].SlowTpUpSw) & (0x01<<i)))
			{
				ThermostatFun &= ~(0x01<<i);
			}
			else
			{
				if((TwoHeatSetTm[i] == OFF) || (TwoHeatSetTm[i] == (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm)))) // 2016/01/13
				{
					ThermostatFun &= ~(0x01<<i);
				}
			}
		}
		//
		if(!(TwoHeatMdFinishFlag & (0x01<<i)) && (*(tempData[0].TwoHeatMd) & (0x01<<i)) && (TwoHeatSetTm[i] == (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))) && (*(tempData[i].TpControl) == OldTpControl[i])) // 2016/02/04 �@���[��, �Y�ūצb��ұa��, �Y���M����q�ɷŤΦP�B�ɷť\�઺�Ѽ�.
		{
			TwoHeatMdFinishFlag |= (0x01<<i);
		}
		//
		if(*(tempData[i].TpControl) != OldTpControl[i])
		{
			///// 2014.02.07 �[�J���ū׳]�w�ȤW�ի�, P���H�ۻ~�t����.
			if((*(tempData[i].TpControl) > OldTpControl[i]) && (Temperature_First_Up_Limit_Flag & (0X01<<i))) // 2015/07/28
			{
				Temperature_Up_Flag |= (0x01<<i); // 2015/07/28
				Temperature_Up_Error[i] = (float)*(tempData[i].TpControl) - (float)OldTpControl[i];
			}
			/////
			if((*(tempData[i].TpControl)-OldTpControl[i]) > BkPidPUnit[i]) // 2015/11/26 �ܧ�[���Ƚd��j���ұa, �~�����q�W�ť\��.
			{
				TwoTpHeatFg &= ~(0x01<<i);		// ���s�p��G�q�ɷť\��
				TwoTpFg &= ~(0x01<<i);				// ���s�p��G�q�ɷť\��
				TwoUpTm[i] = 0;	// Fu 106/05/24
				FirstToSetUnitFg  &= ~(0x01<<i);	// Fu 106/05/24
				//FirstToPAreaFg &= ~(0x01<<i);	// Fu 106/05/18
			}
			//
			OldTpControl[i] = *(tempData[i].TpControl);
			//
			TpClLpFg4 &= ~(0x01<<i);			// ���s�p��P�B�ɷť\��
			TempTm5Min[i] = 0;
			TempTm5Min2[i] = 0;
			TempTm5Min3[i] = 0;
			InitBellBuffFg &= ~(0x01<<i);	// Fu 104/08/13
			//
			if(!(TwoHeatMdFinishFlag & (0x01<<i)) && (*(tempData[0].TwoHeatMd) & (0x01<<i)) && (TwoHeatSetTm[i] == (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm)))) // 2016/01/07
			{
				TwoHeatMdFinishFlag |= (0x01<<i);
				TwoUpTm[i] = OFF;
				//
				if(TwoHeatMdFinishFlag == OFF)
				{
					TpRangCnt = OFF;
					TpRangUsedFg = OFF;
					TpRangFg = OFF;
				}
			}
		}
		//
		TpPIDdata[i].Curr = *(tempData[i].TpDisplay);
		if((TpPIDdata[i].Curr == 12000) && ((SecTempSw>>i) & 0x01))
		{
			TpCutFg |= (0x01<<i);
			TpCutFgHex[i] = thermal_hex[i];
		}
		else
			TpCutFg &= ~(0x01<<i);
		//
		*tempData[9].TpVersion = TpCutFg;
		//
		if((Temp) && (*(tempData[i].TpControl) != 0) && (*(tempData[i].TpControl) != 0xffff) && (BkPidPUnit[i] != 0xffff) && (BkPidPUnit[i]) && (TpPIDdata[i].Curr <= 5800) && (SecTempSw & (0x01<<i)))
		{
				TpClLpFg3 |= (0x1<<i);  // Have Heat Statsu
				//
				if(*(tempData[0].SynchronFun) & (0x01<<i))
				{
					if(TpClLpFg4 & (0x01<<i))
					{
						FullTimeTuningFlag |= (0x01<<i);
					}
				}
				else
				{
					FullTimeTuningFlag |= (0x01<<i);
				}
				//
				if((!(TpClLpFg2 & (0x01<<i))) && (*(tempData[0].AuTnFun) == 0) && !(ThermostatFun & (0x01<<i))) // 2016/01/29 �[�JThermostatFun
				{
						TpClLpFg2 |= (0x01<<i);
						TpPIDdata[i].P = BkPidPUnit[i];
						TpPIDdata[i].I = BkPidIUnit[i];
						TpPIDdata[i].D = BkPidDUnit[i];
						TpPIDdata[i].Set = *(tempData[i].TpControl);
						//	FU 106/05/24
						if(Bk_thermal_couple[i] >= TpPIDdata[i].Set)
						{
							FirstToSetUnitFg  |= (0x01<<i);
						}
						else
						{
							FirstToSetUnitFg  &= ~(0x01<<i);
						}
						TwoUpFun = 0xfff;
						*tempData[i].TwoUpOffset = TpPIDdata[i].P;
						//
						// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
						if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
						{
								TwoTpHeatFg |= (0x01<<i); // 2016/01/08
								TwoTpFg |= (0x01<<i); // 2016/01/08
								TwoUpTm[i] = 20;
						}
						else
						{
							if((TwoUpFun & (0x01<<i)) && (((*(tempData[0].TwoHeatMd) & (0x01<<i)) && (TwoHeatSetTm[i] < (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))) && (*(tempData[i].TpControl) < 1500)) || !(*(tempData[0].SynchronFun) & (0x01<<i)) || (TpRangCnt >= 48))) // 2016/01/08
							{
									if(*tempData[i].TwoUpOffset <= 50)
									{
										*tempData[i].TwoUpOffset = 50;
									}
									//
									if(TwoUpTempMd(i, TpPIDdata[i].Set, thermal_couple[i], (*tempData[i].TwoUpOffset)))	// �G�q�W�ť\��
									{
											TpPIDdata[i].Set = thermal_couple[i]+3;  // �ܧ�]�w���ثe�� , �u�ѤUPD��
											TpPIDdata[i].Isum1 = TpPIDdata[i].Isum1/10;  // ��C�ū׹L��
									}
							}
							else
							{
								TwoTpHeatFg |= (0x01<<i); // 2016/01/08
								TwoTpFg |= (0x01<<i); // 2016/01/08
								TwoUpTm[i] = 20;
							}
						}
						//
						TowTpFg = 0;
						//	Fu 104/08/13
						Old20514 = *GetPtrCDM2(20514);
						Old20515 = *GetPtrCDM2(20515);
						//
						if(Old20514 != 0)
						{
							//	Fu 104/08/13
							if((*(tempData[0].SynchronFun) & (0x01<<i)) && (Old20515 <= 48) && (TpPIDdata[i].P) && (*(tempData[i].TpControl) >= 1200))		 	// �P�B�ɷť\��
							{
								if((TpCurrUnit >= (TempSetUnit[i][Old20515])) && (!((TpRangFg>>i) & 0x01)))
								{
									TpRangFg |= (0x01<<i);
									//	
									if(TpCurrUnit <= (TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])))
									{
										if(TpPIDdata[i].Isum1)
										{
											if(TpPIDdata[i].P >= ((TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])) - TpCurrUnit))
											{
												if(TpPIDdata[i].Isum1 >= ((((TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P))
												{
													TpPIDdata[i].Isum1 = ((((TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P);
												}
											}
											else
											{
												if(TpPIDdata[i].Isum1 >= ((((TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P * 2))
												{
													TpPIDdata[i].Isum1 = ((((TempSetUnit[i][Old20515] + (TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P * 2);
												}
											}
										}
									}
									else
									{
										TpPIDdata[i].Isum1 = 0;
									}
								}
								//
								if(TpRangFg & (0x01<<i))
								{
									TpPIDdata[i].Set = (TempSetUnit[i][Old20515]) + ((TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515]) / 2);
								}
								else
								{
									TpPIDdata[i].Set = (TempSetUnit[i][Old20515]) + ((TempSetUnit[i][Old20515+1] - TempSetUnit[i][Old20515]) * 1);		
								}
								//
								if(TpPIDdata[i].Set >= (*(tempData[i].TpControl) - 10))
								{
									TpPIDdata[i].Set = *(tempData[i].TpControl) - 10;
								}
								// Fu 107/01/03
								if(Old20515 >= 32)	// 70%
								{
								}
								else
								{
									TpPIDdata[i].P = TpPIDdata[i].P / 2.0F;
									//
									if(TpPIDdata[i].P == 0)
									{
										TpPIDdata[i].P = 2.0F;
									}
								}
							}
						}
						else
						{
							//	Fu 104/08/13
							if((*(tempData[0].SynchronFun) & (0x01<<i)) && (TpRangCnt <= 48) && (TpPIDdata[i].P) && (*(tempData[i].TpControl) >= 1200))		 	// �P�B�ɷť\��
							{
								TpRangUsedFg |= (0x01<<i);
								//
								if((TpCurrUnit >= (TempSetUnit[i][TpRangCnt])) && (!((TpRangFg>>i) & 0x01)))
								{
									TpRangFg |= (0x01<<i);
									//	
									if(TpCurrUnit <= (TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])))
									{
										if(TpPIDdata[i].Isum1)
										{
											if(TpPIDdata[i].P >= ((TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])) - TpCurrUnit))
											{
												if(TpPIDdata[i].Isum1 >= ((((TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P))
												{
													TpPIDdata[i].Isum1 = ((((TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P);
												}
											}
											else
											{
												if(TpPIDdata[i].Isum1 >= ((((TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P * 2))
												{
													TpPIDdata[i].Isum1 = ((((TempSetUnit[i][TpRangCnt] + (TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt])) - TpCurrUnit) * OutputPwmMaxUnit[i]) / TpPIDdata[i].P * 2);
												}
											}
										}
									}
									else
									{
										TpPIDdata[i].Isum1 = 0;
									}
								}
								//
								if(TpRangFg & (0x01<<i))
								{
									TpPIDdata[i].Set = (TempSetUnit[i][TpRangCnt]) + ((TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt]) / 2);
								}
								else
								{
									TpPIDdata[i].Set = (TempSetUnit[i][TpRangCnt]) + ((TempSetUnit[i][TpRangCnt+1] - TempSetUnit[i][TpRangCnt]) * 8);		
								}
								//
								if(TpPIDdata[i].Set >= (*(tempData[i].TpControl) - 10))
								{
									TpPIDdata[i].Set = *(tempData[i].TpControl) - 10;
								}
								//
								if(TpRangUsedFg == TpRangFg)
								{
									TpRangFg = 0;
									TpRangCnt++;
									//
									if(TpRangCnt >= 49)
									{
										TpRangCnt = 49;
									}
								}
								else
								{
								}
							}
							else
							{
									TpRangUsedFg &= ~(0x01<<i); // 2015/09/16
							}
						}
						// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
						if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
						{
							/*//	Fu 106/07/28
							if(BkTempSpRamp[i] >= 6)	// 0.6%
							{
								Output100Fg |= (0x01<<i);
								Output100Pr[i] = 4;
							}
							else if(BkTempSpRamp[i] >= 4)	// 0.4%
							{
								Output100Fg |= (0x01<<i);
								Output100Pr[i] = 1;
							}
							else
							{
								Output100Fg &= ~(0x01<<i);
								Output100Pr[i] = 0;
							}*/
							Output100Pr[i] = 0;
							//
							PidMathTime[i] += 1;
							if(PidMathTime[i] >= 46)	// 11500ms
							{
								OrgTpOutPwmBk[i] = TpCloseLoopSub(i, TowTpFg);
								TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
								PidMathTime[i] = 0;
							}
							OldBkTempSpRamp[i] = BkTempSpRamp[i];	// Fu 106/05/22
						}
						else
						{
							OrgTpOutPwmBk[i] = TpCloseLoopSub(i, TowTpFg);
							TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
							OldBkTempSpRamp[i] = BkTempSpRamp[i];	// Fu 106/05/22
						}
						//
						//if((*(tempData[0].SynchronFun) & (0x01<<i)) && (i==SynFg) && (SynFg <= 11) && !(TpClLpFg4 & (0x01<<i)))		 	// �P�B�ɷť\��
							//TpPIDdata[i].Isum1 = TpPIDdata[i].Isum1/(TpPIDdata[i].P/10.);	// ��C�ū׹L��
				}
				//
				if(!(*(tempData[0].AuTnFun) & (0x01<<i)))
				{
					if(AlternTm[i] == 0)
					{
						if((HeatRunFg & (0x01<<i)) == OFF)
						{
							HeatRunFg |= (0x01<<i);
							OrgTpOutPwm[i] = OrgTpOutPwmBk[i];      // First PWM unit
							OrgTpOutPwmBk2[i] = OrgTpOutPwmBk[i];   // First PWM unit
						}
						else
						{	    
							if(*(tempData[0].ContrlFun) & (0x01<<i))    // Relay
							{
								/*RelayTmMd[i]++;		// 250ms * 16 = 4 sec
								if(RelayTmMd[i] > 16)
								{
									RelayTmMd[i]= 0;
									LastOutUnit1[i] = (OrgTpOutPwmBk[i] * ((*(tempData[i].CycleTm)/10))) / 32767;	   	// �ثe-PID Unit
									LastOutUnit2[i] = (OrgTpOutPwmBk2[i] * ((*(tempData[i].CycleTm)/10))) / 32767;	   	// ���e-PID Unit
									if(LastOutUnit1[i] == 0)
										*GetPtrCDM2(20616+i) = 0;
									else if(LastOutUnit1[i] > LastOutUnit2[i])
									{
										if(LastOutUnit1[i] >= *(tempData[i].PwmOutputCyc))
											*GetPtrCDM2(20616+i) = LastOutUnit1[i];
										else
										{
											PwmOffsetOut = *(tempData[i].PwmOutputCyc) + (LastOutUnit1[i] - LastOutUnit2[i]);
											if(PwmOffsetOut >= (*(tempData[i].CycleTm)/10))
												*GetPtrCDM2(20616+i) = (*(tempData[i].CycleTm)/10);
											else
												*GetPtrCDM2(20616+i) = PwmOffsetOut;
										}
									}
									else if(LastOutUnit1[i] < LastOutUnit2[i])
									{
										if(LastOutUnit1[i] <= *(tempData[i].PwmOutputCyc))
											*GetPtrCDM2(20616+i) = 0;
										else
											*GetPtrCDM2(20616+i) = LastOutUnit1[i];
								}
						}*/
					}
					//else    // SSR : 500ms
					//{
						//LastOutUnit1[i] = (OrgTpOutPwmBk[i] * Sec025_TRIG_MS) / 32767;	   	// �ثe
						//LastOutUnit2[i] = (OrgTpOutPwmBk2[i] * Sec025_TRIG_MS) / 32767;	   	// ���e
						//if(LastOutUnit1[i] == 0)
							// *GetPtrCDM2(20616+i) = 0;
						//else if(LastOutUnit1[i] > LastOutUnit2[i])
						//{
							//if(LastOutUnit1[i] >= *(tempData[i].PwmOutputCyc))
								// *GetPtrCDM2(20616+i) = LastOutUnit1[i];
							//else
							//{
								//PwmOffsetOut = *(tempData[i].PwmOutputCyc) + (LastOutUnit1[i] - LastOutUnit2[i]);
								//if(PwmOffsetOut >= Sec025_TRIG_MS)
									// *GetPtrCDM2(20616+i) = Sec025_TRIG_MS;
								//else
									// *GetPtrCDM2(20616+i) = PwmOffsetOut;
							//}
						//}
						//else if(LastOutUnit1[i] < LastOutUnit2[i])
						//{
							//if(LastOutUnit1[i] <= *(tempData[i].PwmOutputCyc))
								// *GetPtrCDM2(20616+i) = 0;
							//else
								// *GetPtrCDM2(20616+i) = LastOutUnit1[i];
						//}
					//}
									}		
				//
				//	Fu 101/07/10
				//
				//if((TpClLpFg3 & (0x01<<i)) && (TwoUpTm[i] == 20) && (TwoUpTm[i] >= 65000))	// Fu 102/08/02
				if((TpClLpFg3 & (0x01<<i)) && (TwoUpTm[i] == 20) && (*tempData[0].AllTmAuTpSw & (0x01<<i)) && (FullTimeTuningFlag & (0x01<<i)) && !(Temperature_Up_Flag & (0x01<<i)))	// Fu 102/08/02  // 2015/07/31
				//if((TpClLpFg3 & (0x01<<i)) && (TwoUpTm[i] == 20))	// Fu 102/08/02
				{
					if(*(tempData[i].TpControl) >= 1200)
					{
						TpPIDdata[i].Err = (float)(*(tempData[i].TpControl)) - (float)(Bk_thermal_couple[i]);   // Err = Tset - Tcurr  // 當次誤差 = 設定壓力 - 目前壓力
						if(TpPIDdata[i].Err < 0)
							ErrArea = 20;
						else
							ErrArea = 50;
						if(((unsigned short)abs(TpPIDdata[i].Err) > 10) && ((unsigned short)abs(TpPIDdata[i].Err) <= ErrArea))// ����2.0�פ����~���B��
						{
							PID2PTmP[i]++;
							if(PID2PTmP[i] >= 120) // 250*120 = 60 sec
							{
								PID2PTmP[i] = 0; 
								if(Bk_thermal_couple[i] > *(tempData[i].TpControl))	// �ثe�ūװ���
								{
									TperrorUnit = PIDTpCurrBkP[i]/12;
									if(TperrorUnit >= 10)
										TperrorUnit = 10;
									if(BkPidPUnit[i] < 1200)
										BkPidPUnit[i] = BkPidPUnit[i] + TperrorUnit;
									else
										BkPidPUnit[i] = 1200;
								}
								else	// �ثe�ūװ��C
								{
									TperrorUnit = PIDTpCurrBkP[i]/12;
									if(TperrorUnit >= 10)
										TperrorUnit = 10;
									if((BkPidPUnit[i] > TperrorUnit) && (BkPidPUnit[i] > 30))
										BkPidPUnit[i] = BkPidPUnit[i] - TperrorUnit;
									else
										BkPidPUnit[i] = 30;
								}
								//
								PIDTpCurrBkP[i] = 0;
							}
							else
							{
								if(abs(TpPIDdata[i].Err) >= PIDTpCurrBkP[i])
									PIDTpCurrBkP[i] = abs(TpPIDdata[i].Err);
							}
						}
						else
						{
							PID2PTmP[i] = 0;
							PIDTpCurrBkP[i] = 0;
						}
						//
						//	Fu 101/07/11
						//
						TpPIDdata[i].Err = (float)(*(tempData[i].TpControl)) - (float)(Bk_thermal_couple[i]);   // Err = Tset - Tcurr  // 當次誤差 = 設定壓力 - 目前壓力
						if(((((unsigned short)abs(TpPIDdata[i].Err) <= 10)) && ((unsigned short)abs(TpPIDdata[i].Err) >= 5)) && (PIDTpCurrBkP[i] == 0))// ����0.5 ~ 2.5�פ����~���B��
						{
							PID2PTmI[i]++;
							if(PID2PTmI[i] >= 240)	// 250*240 = 120 sec
							{
								PID2PTmI[i] = 0;
								if(Bk_thermal_couple[i] > *(tempData[i].TpControl))	// �ثe�ūװ���
								{
									TperrorUnit = PIDTpCurrBkI[i]/265;
									if(TperrorUnit >= 5)
										TperrorUnit = 5;
									if(TperrorUnit == 0)
										TperrorUnit = 1;
									if((BkPidIUnit[i] >= TperrorUnit) && (BkPidIUnit[i] > TperrorUnit)) // 2015/12/14 BkPidIUnit[i] >= 1 -> BkPidIUnit[i] > TperrorUnit
										BkPidIUnit[i] = BkPidIUnit[i] - TperrorUnit;
									else
										BkPidIUnit[i] = 1;
								}
								else	// �ثe�ūװ��C
								{
									TperrorUnit = PIDTpCurrBkI[i]/265;
									if(TperrorUnit >= 5)
										TperrorUnit = 5;
									if(TperrorUnit == 0)
										TperrorUnit = 1;
									if(BkPidIUnit[i] < 1000)
										BkPidIUnit[i] = BkPidIUnit[i] + TperrorUnit;
									else
										BkPidIUnit[i] = 1000;
								}
								//
								PIDTpCurrBkI[i] = 0;
							}
							else
								PIDTpCurrBkI[i] += abs(TpPIDdata[i].Err);
						}
						else
						{
							PID2PTmI[i] = 0;
							PIDTpCurrBkI[i] = 0;
						}
						/*//
						//	Fu 101/10/19
						//
						if(((unsigned short)abs(TpPIDdata[i].Err) >= 15) && ((unsigned short)abs(TpPIDdata[i].Err) <= BkPidPUnit[i]))// ����1.5 ~ 20.0�פ����~���B��
						{
							PID2PTmD[i]++;
							if(PID2PTmD[i] >= 120)	// 250*120 = 30 sec
							{
								PID2PTmD[i] = 0;
								if(Bk_thermal_couple[i] > *(tempData[i].TpControl))	// �ثe�ūװ���
								{
									//if(PIDTpCurrBkD2[i] <= 30)	// 
									if(abs(Bk_thermal_couple[i] - PIDTpCurrBkD2[i]) <= 15)	// 
									{
										if(BkPidDUnit[i] <= 10)
											BkPidDUnit[i] = 10;
										else
											BkPidDUnit[i] -= ((unsigned short)abs(TpPIDdata[i].Err)/3.14);
									}
								}
								else	// �ثe�ūװ��C
								{
									//if(PIDTpCurrBkD2[i] <= 30)	// 
									if(abs(Bk_thermal_couple[i] - PIDTpCurrBkD2[i]) <= 15)	// 
									{
										if(BkPidDUnit[i] >= 1000)
											BkPidDUnit[i] = 1000;
										else
											BkPidDUnit[i] += ((unsigned short)abs(TpPIDdata[i].Err)/3.14);
									}
								}
								//
								//PIDTpCurrBkD2[i] = 0;
								PIDTpCurrBkD2[i] = Bk_thermal_couple[i];
							}
							//else
								//PIDTpCurrBkD2[i] += abs(Bk_thermal_couple[i] - PIDTpCurrBkD[i]);
						}
						else
						{
							PID2PTmD[i] = 0;
							//PIDTpCurrBkD2[i] = 0;
							PIDTpCurrBkD2[i] = Bk_thermal_couple[i];
						}*/
					}
				}
			}
			else
					AlternTm[i] = AlternTm[i]-1;
			}
			else
			{
					// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
					if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
					{
						PidMathTime[i] += 1;
						//if(PidMathTime[i] >= 46)	// 11500ms
						if(PidMathTime[i] >= 8)	// 250ms * 4 = 2 Sec : Fu 108/01/03 : �P�_�ū��ܧ�2����1��
						{
							PidMathTime[i] = 0;
							TpAuMdSub(i);           // Auto Tining
							TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
						}
						//
						if((AUTO_FG2 & (0x01<<i)) != 0)
						{
							*(tempData[0].AuTnFun) &= ~(0x01<<i);
							RealTpOutPwm &= ~(0x01<<i);     // Hest OFF
						}
					}
					else
					{
							TpAuMdSub(i);           // Auto Tining
							TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
					}
			}
			//
			//
			//	Fu 102/12/27 : �榸�۰ʽտӱ���A����
			//
			*GetPtrCDM2(20561) = *tempData[0].AuTnFun;
			//
			if(*tempData[0].AuTnFun)
				*tempData[0].AutoTingSts = 0;
			else		 
				*tempData[0].AutoTingSts = 1;	// �ūצ۰ʽտӧ���
     	}
			else
			{
				if(Temp)	// Fu 102/07/25
					*tempData[0].AuTnFun &= ~(0x01<<i);
				//	Fu 106/07/24
				POWER_ON_OFF_FG &= ~(0x01<<i);
				AUTO_FG3 &= ~(0x01<<i);
				CHANGE_ON_OFF &= ~(0x01<<i);
				AUTO_FG2 &= ~(0x01<<i);
				AUTO_COUNTER[i] = 0;
				TP_OVER_FG2[i] = 0;
				//	Fu 105/09/21
				AuTuFirstFg &= ~(0x01<<i);
				//
				TpAuStopFg &= ~(0x01<<i);
				// Fu 109/07/28 : �[�J�ʤ����X�Ҧ��]�n�P�_�[���l�}��
				//if(ThermostatFun & (0x01<<i))
				if((ThermostatFun & (0x01<<i)) && (SecTempSw & (0x01<<i)))
				{
					if(AlternTm[i] == 0)
							TpClLpFg3 |= (0x1<<i);  	// Have Heat Statsu
					else
							AlternTm[i] = AlternTm[i]-1;
				}
				else
				{
					TpClLpFg3 &= ~(0x1<<i);     // Not Heat Statsu
					OrgTpOutPwm[i] = 0;
					OrgTpOutPwmBk[i] = 0;
					TpPIDdata[i].Isum1 = 0;   // I累計值
					HeatRunFg &= ~(0x01<<i);
					*GetPtrCDM2(20601+i)=0;
					AlternTm[i] = AlternTmBk[i];
					TempTm5Min[i] = 0;
					TempTm5Min2[i] = 0;
					TwoUpTm[i] = 0;
					FullTimeTuningFlag &= ~(0x01<<i); // 2015/07/31
					InitBellBuffFg &= ~(0x01<<i);
				}
				// Fu 106/05/18
				Output100Fg &= ~(0x01<<i);
				Output100Pr[i] = 0;				
				FirstToSetUnitFg  &= ~(0x01<<i);	// Fu 106/05/24
				//FirstToPAreaFg &= ~(0x01<<i);	// Fu 106/05/18
				OldBkTempSpRamp[i] = BkTempSpRamp[i];	// Fu 106/05/22
			}
			//		Fu 106/07/23
			 if((*(tempData[0].AuTnFun) & (0x01<<i)))
			 {
			 }
			 else
			 {
					// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
					if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
					{
					 if((TpClLpFg3 & (0x01<<i)) == 0)
					 {
						PidMathTime[i] += 1;
						if(PidMathTime[i] >= 46)	// 11500ms
						{
							PidMathTime[i] = 0;
							TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
						}
					 }
					}
					else
					{
					 if((TpClLpFg3 & (0x01<<i)) == 0)
					 {
							TpPIDdata[i].OldCurr = thermal_couple[i];	// �B��@��PID�Ȱ��@���O�����e���ū�
					 }
					}
			 }
	}
}
////////////////////////////////////////////////
//
//
unsigned short TwoUpTempMd(unsigned short Sec, unsigned short Setting, unsigned short Current, unsigned short Offset)
{
	unsigned short Setting2,TstSts;
	//
	Setting2 = Setting;             // real setting
	//
	if(Setting >= Offset)
		Setting = Setting-Offset;   // offset setting
	else
		Setting = Setting/2;        // offset setting
	//
	InitTpHt++;
	TstSts = 0;
	//
	if((InitTpHt >= (TpMaxLp*2)) && (!(TwoTpHeatFg & (0x01<<Sec))))
	{
		InitTpHt = TpMaxLp*3;
		//
		if(Current >= Setting)		// �ثe�Ȥj��]�w��?	
		{
			if((Current >= Setting)	&& (Current <= (Setting2+Offset)))	// �ثe�Ȥj��]�w��?	
			{
				if(((TwoTpFg & (0x01<<Sec)) == 0) || ((TwoTpHeatFg & (0x01<<Sec)) == 0))
				{
					if((Current <= Setting2) && (Current >= TpPIDdata[Sec].OldCurr))
					{
						TwoTpFg |= (0x01<<Sec);
						TstSts = ON;	
					}
					else
					{
						if(TwoUpTm[Sec]++ >= 20) // 2014/08/15 �P�_���Ʊq10��אּ20.
						{
							TwoUpTm[Sec] = 20;
							TwoTpHeatFg |= (0x01<<Sec);
						}
					}
				}
			}
		}
		//
		if(Current >=  Setting2)
		{
			TwoTpHeatFg |= (0x01<<Sec);
			TwoUpTm[Sec] = 20;
		}
	}
	//
	return TstSts;
}
///////////////////////////////////////////
//
//
void TpAuMdSub(unsigned short Sec)
{
	static unsigned short i;
	unsigned int Data;
	// Fu 106/07/25
	i = Sec;
	//	Fu 106/07/27
	//if(*(tempData[0].ThermostatFun) & 0x8000)	// 15 Bit : MJ86 - 12��PID�Ҧ�
	// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
	if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
	{
		if((AUTO_FG2 & (0x01<<i)) != 0)
		{
			*(tempData[0].AuTnFun) &= ~(0x01<<i);
			RealTpOutPwm &= ~(0x01<<i);     // Hest OFF
		}
		else
		{
			if(*(tempData[i].TpControl) <= thermal_couple[i])
			{
				AUTO_FG3 |= (0x01<<i);
				TP_OVER_FG2[i] = 0;
				POWER_ON_OFF_FG &= ~(0x01<<i);
				TEMP_OUTPUT_MD(i);
				RealTpOutPwm &= ~(0x01<<i);     // Hest OFF
			}
			else	// Heat ON
			{
				if((AUTO_FG3 & (0x01<<i)) != 0)
				{
					TP_OVER_FG2[i] += 1;
					if(TP_OVER_FG2[i] <= 2)
					{
					}
					else
					{
						TP_OVER_FG2[i] = 3;
						POWER_ON_OFF_FG |= (0x01<<i);
						TEMP_OUTPUT_MD(i);
						RealTpOutPwm |= 0x01<<i;            // Hest ON
					}
				}
				else
				{
					POWER_ON_OFF_FG |= (0x01<<i);
					RealTpOutPwm |= 0x01<<i;            // Hest ON
				}
			}
		}
	}
	else
	{
		//
		TempTm5Min[Sec]++;
		//	Fu 105/09/14
		if(*(tempData[0].ThermostatFun) & 0x1000u)	// 12 Bit
		{
			if(thermal_couple[Sec] >= *(tempData[Sec].TpControl))
			{
				if((AuTuFirstFg & (0x01u<<Sec)) == 0u)
				{
					TempTm5Min2[Sec] = 0u;
					//TempTm5Min[Sec] = TempTm5MinUint;
					TpAuStopFg |= (0x01u<<Sec);
					TempAuTpUnit[Sec] = thermal_couple[Sec];
					TempAuTpUnitUp[Sec] = 0u;
					TempTmBuff[Sec] = 0;
					TempTmOKFg &= ~(0x01u<<Sec);
					TpDownCnt[Sec] = 0;
					AuTuFirstFg |= (0x01<<Sec);
				}
				else
				{
				}
			}
			else
			{
			}
			//
			if((AuTuFirstFg & (0x01u<<Sec)) != 0u)
			{								
					if((TempTmOKFg & (0x01u<<Sec)) == 0u)
					{
						TempTmBuff[Sec]++;		// 250ms time base
					}
					else
					{
					}
					//
					//if(TempTmBuff[Sec] >= 7200u)	// max 7200 * 0.25S  = 30 min time
					if(TempTmBuff[Sec] >= 14400u)	// max 14400 * 0.25S  = 60 min time	// Fu 106/06/01
					{
						TpDownCnt[Sec] = 10;
					}
					//
					//if((((thermal_couple[Sec]) < TpPIDdata[Sec].OldCurr)) || (TempTmOKFg & (0x01<<Sec)))
					if(((thermal_couple[Sec]+5u) < *(tempData[Sec].TpControl)) || (TempTmOKFg & (0x01<<Sec)))
					{
						if(TpDownCnt[Sec]++ > 2u)	// 250ms * 3 = 0.75 sec
						{
							if(TempAuTpUnitUp[Sec] < thermal_couple[Sec])
									TempAuTpUnitUp[Sec] = thermal_couple[Sec];	// �O���ثe�̤j�ū׭�
							//
							TempTmOKFg |= (0x01u<<Sec);
							//
							//TempTm5Min2[Sec]++;
							////
							//if(TempTm5Min2[Sec] >= TempTm5MinUint)
							//{
									//TempTm5Min2[Sec] = TempTm5MinUint;
									//
									Data = TempAuTpUnitUp[Sec]-TempAuTpUnit[Sec];	// �O���̤j�i�p�M�i�����t��
									Data = (Data + ((Data * 5) / 10));
									//
									if((Data >= 75u) && (Data <= 1000u))
									{
											*(tempData[Sec].PIDp) = Data;
											BkPidPUnit[Sec] = Data;
									}
									else
									{
										//if(TempTm5Min3[Sec] == TempTm30MinUint)
										//{
											// *(tempData[Sec].PIDp) = 350;
											//BkPidPUnit[Sec] = 250;
										//}
										//else if(Data <= 25)
										if(Data <= 75u)
										{
											*(tempData[Sec].PIDp) = 75u;
											BkPidPUnit[Sec] = 75u;
										}
										else
										{
											*(tempData[Sec].PIDp) = 300;
											BkPidPUnit[Sec] = 300;
										}
									}
								//
								*tempData[Sec].TwoUpOffset = (*tempData[Sec].PIDp+30);
								//
								if(TempTmBuff[Sec])
								{
										if(TpAuStopFg & (0x01<<Sec))
										{
											//Data = 6000/TempTmBuff[Sec];
											//Data = 60000/TempTmBuff[Sec];
											//Data = 150000/TempTmBuff[Sec];
											Data = 180000/TempTmBuff[Sec];
											//if(TempTm5Min3[Sec] == TempTm30MinUint)
											//{
												 // *(tempData[Sec].PIDi) = 10;
												 //BkPidIUnit[Sec] = 10;
											//}
											//else if((Data >= 1) && (Data <= 999))
											if((Data >= 25) && (Data <= 150))
											{
												 *(tempData[Sec].PIDi) = Data;
												 BkPidIUnit[Sec] = Data;
											}
											else
											{
												//	Fu 106/05/26
												if(Data >= 150)
												{
												 *(tempData[Sec].PIDi) = 150;
												 BkPidIUnit[Sec] = 150;
												}
												else
												{
												 *(tempData[Sec].PIDi) = 25;
												 BkPidIUnit[Sec] = 25;
												}
											}
											//
											Data = TempTmBuff[Sec]/60;		// 60 / 0.5 = 12
											//if(TempTm5Min3[Sec] == TempTm30MinUint)
											//{
												 // *(tempData[Sec].PIDd) = 25;
												 //BkPidDUnit[Sec] = 25;
											//}
											//else if((Data >= 1) && (Data <= 999))
											//if((Data >= 1) && (Data <= 250))
											if((Data >= 20) && (Data <= 250))
											{
												 *(tempData[Sec].PIDd) = Data;
												 BkPidDUnit[Sec] = Data;
											}
											else
											{
												//	Fu 106/05/26
												if(Data >= 250)
												{
												 *(tempData[Sec].PIDd) = 250;
												 BkPidDUnit[Sec] = 250;
												}
												else
												{
												 *(tempData[Sec].PIDd) = 20;
												 BkPidDUnit[Sec] = 20;
												}
											}
										}
										else
										{
										}
									}
									else
									{
										 *(tempData[Sec].PIDi) = 25;
										 BkPidIUnit[Sec] = 25;
										 *(tempData[Sec].PIDd) = 50;
										 BkPidDUnit[Sec] = 50;
									}
									*(tempData[0].AuTnFun) &= ~(0x01<<Sec);
									AuTnFunSw &= ~(0x01<<Sec);
									TpAuStopFg &= ~(0x01<<Sec);
								}
								//
								TpDownCnt[Sec] = 10;
								//
								RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
							//}
							////
							//RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
						}
						else
						{
							TempTm5Min3[Sec]++;	// 250 * 20 Min = 1600
							//
							RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
							if(TempAuTpUnitUp[Sec] < thermal_couple[Sec])
									TempAuTpUnitUp[Sec] = thermal_couple[Sec];	// �O���ثe�̤j�ū׭�
						}
				//
				//TempTm5Min[Sec] = TempTm5MinUint+10;
			}
			else
			{
					RealTpOutPwm |= 0x01<<Sec;            // Hest ON
			}
		}
		else
		{
			if(TempTm5Min[Sec] < TempTm5MinUint)
			{
				if(thermal_couple[Sec] >= *(tempData[Sec].TpControl))
				{
					if(TempTm5MinUint >= TempTm5Min[Sec])
						TempTm5Min2[Sec] = TempTm5MinUint -  TempTm5Min[Sec];
					TempTm5Min[Sec] = TempTm5MinUint;
					TpAuStopFg |= (0x01<<Sec);
				}
			}
			//
			if(TempTm5Min[Sec] >= TempTm5MinUint)
			{												   
					TempTmBuff[Sec]++;		// 250ms time base
					//
					if(TempTm5Min[Sec] >= (TempTm5MinUint+10))
					{
							if((((thermal_couple[Sec]) < TpPIDdata[Sec].OldCurr)) || (TempTmOKFg & (0x01<<Sec)) || (TempTm5Min3[Sec] >= TempTm30MinUint))
							{
								if(TpDownCnt[Sec]++ > 2)	// 250ms * 3 = 0.75 sec
								{
									TempTmOKFg |= (0x01<<Sec);
									//
									TempTm5Min2[Sec]++;
									//
									if(TempTm5Min2[Sec] >= TempTm5MinUint)
									{
											TempTm5Min2[Sec] = TempTm5MinUint;
											//
											Data = TempAuTpUnitUp[Sec]-TempAuTpUnit[Sec];
											if((Data >= 25) && (Data <= 1000))
											{
													*(tempData[Sec].PIDp) = Data;
													BkPidPUnit[Sec] = Data;
											}
											else
											{
												if(TempTm5Min3[Sec] == TempTm30MinUint)
												{
													*(tempData[Sec].PIDp) = 350;
													BkPidPUnit[Sec] = 250;
												}
												else if(Data <= 25)
												{
													*(tempData[Sec].PIDp) = 50;
													BkPidPUnit[Sec] = 50;
												}
												else
												{
													*(tempData[Sec].PIDp) = 200;
													BkPidPUnit[Sec] = 200;
												}
											}
										//
										*tempData[Sec].TwoUpOffset = (*tempData[Sec].PIDp+30);
										//
										if(TempTmBuff[Sec])
										{
												if(TpAuStopFg & (0x01<<Sec))
												{
													Data = 6000/TempTmBuff[Sec];
													if(TempTm5Min3[Sec] == TempTm30MinUint)
													{
														 *(tempData[Sec].PIDi) = 10;
														 BkPidIUnit[Sec] = 10;
													}
													else if((Data >= 1) && (Data <= 999))
													{
														 *(tempData[Sec].PIDi) = Data;
														 BkPidIUnit[Sec] = Data;
													}
													else
													{
														 *(tempData[Sec].PIDi) = 10;
														 BkPidIUnit[Sec] = 10;
													}
													//
													Data = TempTmBuff[Sec]/60;		// 60 / 0.5 = 12
													if(TempTm5Min3[Sec] == TempTm30MinUint)
													{
														 *(tempData[Sec].PIDd) = 25;
														 BkPidDUnit[Sec] = 25;
													}
													else if((Data >= 1) && (Data <= 999))
													{
														 *(tempData[Sec].PIDd) = Data;
														 BkPidDUnit[Sec] = Data;
													}
													else
													{
														 *(tempData[Sec].PIDd) = 25;
														 BkPidDUnit[Sec] = 25;
													}
												}
												else
												{
													Data = 60000/TempTmBuff[Sec];
													if((Data >= 1) && (Data <= 999))
													{
														 *(tempData[Sec].PIDi) = Data;
														 BkPidIUnit[Sec] = Data;
													}
													else
													{
														 *(tempData[Sec].PIDi) = 10;
														 BkPidIUnit[Sec] = 10;
													}
													//
													Data = TempTmBuff[Sec]/60;		// 60 / 0.5 = 12
													if((Data >= 1) && (Data <= 999))
													{
														 *(tempData[Sec].PIDd) = Data;
														 BkPidDUnit[Sec] = Data;
													}
													else
													{
														 *(tempData[Sec].PIDd) = 25;
														 BkPidDUnit[Sec] = 25;
													}
												}
											}
											else
											{
												 *(tempData[Sec].PIDi) = 10;
												 BkPidIUnit[Sec] = 10;
												 *(tempData[Sec].PIDd) = 25;
												 BkPidDUnit[Sec] = 25;
											}
											*(tempData[0].AuTnFun) &= ~(0x01<<Sec);
											AuTnFunSw &= ~(0x01<<Sec);
											TpAuStopFg &= ~(0x01<<Sec);
										}
										//
										TpDownCnt[Sec] = 10u;									
										//
										RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
									}
									//
									RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
							}
							else
							{
								TempTm5Min3[Sec]++;	// 250 * 20 Min = 1600
								//
								RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
								if(TempAuTpUnitUp[Sec] < thermal_couple[Sec])
										TempAuTpUnitUp[Sec] = thermal_couple[Sec];
							}
					}
					else
							RealTpOutPwm &= ~(0x01<<Sec);     // Hest OFF
					//
					if(TempTm5Min[Sec] == TempTm5MinUint)
					{
							TempAuTpUnit[Sec] = thermal_couple[Sec];
							TempAuTpUnitUp[Sec] = 0;
							TempTmBuff[Sec] = 0;
							TempTmOKFg = 0;
							TpDownCnt[Sec] = 0;
					}
				//
				TempTm5Min[Sec] = TempTm5MinUint+10;
			}
			else
					RealTpOutPwm |= 0x01<<Sec;            // Hest ON
		}
	}
}
//
// Fu 106/07/25
//
void TEMP_OUTPUT_MD(unsigned short i)
{
	if((POWER_ON_OFF_FG & (0x01<<i)) != 0)
	{
		if((CHANGE_ON_OFF & (0x01<<i)) != 0)
		{
			if(thermal_couple[i] <= TpPIDdata[i].OldCurr)
			{
			}
			else
			{
				CHANGE_ON_OFF &= ~(0x01<<i);
				AUTO_GET_CURR2[i][AUTO_COUNTER[i]] = thermal_couple[i];
				AUTO_TUNNING_TM[i][AUTO_COUNTER[i]] = TEMP_BASE_0p1S;
				AUTO_COUNTER[i] += 1;
				if(AUTO_COUNTER[i] >= ((AUTO_TUNNING[i]*2)+2+1))
				{
					AUTO_FG2 |= (0x01<<i);
					PID_COUNTER_MD(i);
				}
				else
				{
				}
			}
		}
		else
		{
		}
	}
	else
	{
		if((CHANGE_ON_OFF & (0x01<<i)) != 0)
		{
		}
		else
		{
			if(thermal_couple[i] > TpPIDdata[i].OldCurr)
			{
			}
			else
			{
				CHANGE_ON_OFF |= (0x01<<i);
				AUTO_GET_CURR2[i][AUTO_COUNTER[i]] = thermal_couple[i];
				AUTO_TUNNING_TM[i][AUTO_COUNTER[i]] = TEMP_BASE_0p1S;
				AUTO_COUNTER[i] += 1;
				if(AUTO_COUNTER[i] >= ((AUTO_TUNNING[i]*2)+2+1))
				{
					AUTO_FG2 |= (0x01<<i);
					PID_COUNTER_MD(i);
				}
				else
				{
				}
			}
		}
	}
}
//
// Fu 106/07/25
//
void PID_COUNTER_MD(unsigned short i)
{
	static unsigned short PUnit=0, IUnit=0, DUnit=0, Amp=0, j=0;
	static unsigned long TimeMax, TimeMin, T_OSC;
	//
	TimeMax = AUTO_TUNNING_TM[i][((AUTO_TUNNING[i]*2)+2)-1];
	TimeMin = AUTO_TUNNING_TM[i][1];
	T_OSC = ((TimeMax - TimeMin) / (AUTO_TUNNING[i]+1));
	//
	Amp = AUTO_GET_CURR2[i][2] - AUTO_GET_CURR2[i][1];
	//
	for(j=0; j<((AUTO_TUNNING[i]*2)-1); j++)
	{
		Amp += AUTO_GET_CURR2[i][2+2+(j*2)];
		Amp -= AUTO_GET_CURR2[i][1+2+(j*2)];
	}
	//
	PUnit = Amp / ((AUTO_TUNNING[i]*2)+2);
	//
	PUnit *= 131;
	PUnit /= 2100;
	if(PUnit >= 2)
	{
	}
	else
	{
		PUnit = 2;
	}	
	//
	if(T_OSC != 0)
	{
		IUnit = 50400 / T_OSC;
		DUnit = T_OSC / 40;
	}
	else
	{
		IUnit = 10;
		DUnit = 100;
	}
	//
	 *(tempData[i].PIDp) = PUnit;
	 BkPidPUnit[i] = PUnit;
	 *(tempData[i].PIDi) = IUnit;
	 BkPidIUnit[i] = IUnit;
	 *(tempData[i].PIDd) = DUnit;
	 BkPidDUnit[i] = DUnit;
}	
//
//----------------------------------------------------------
// 	Closed-Loop Control
unsigned short TpCloseLoopSub(unsigned short sec, unsigned short ChgPDUnitFg)
{
	static unsigned short Output=32767, j, Ki;
	static float dXi, dXd;
	//
	//	Fu 106/07/27
	// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
	if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<sec)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
	{
		/*
		Error = ((SET * 1024 /1010) - Curr));
		Kp = (((SET * 1024 /1010) - Curr)) * 100) / P
		Ki = (84 * I * ((SET * 1024 /1010) - Curr)) / (600 * P)
		Kd = ((Curr - Before) * (600 * D)) / (84 * P)
		//
		Kp - Kd + Ki
		if( > Max) to Max
		else to Real Output	
		*/
		// Fu 106/07/25
		TpPIDdata[sec].Err = (float)(TpPIDdata[sec].Set) - (float)(thermal_couple[sec]);   // Err = Tset - Tcurr  // 當次誤差 = 設定壓力 - 目前壓力
		TpPIDdata[sec].Pturn = ((TpPIDdata[sec].Err) * 100) / (float)TpPIDdata[sec].P;
		TpPIDdata[sec].Iturn = (84 * (float)TpPIDdata[sec].I * TpPIDdata[sec].Err) / (600 * (float)TpPIDdata[sec].P);
		TpPIDdata[sec].Dturn = ((float)(thermal_couple[sec] - TpPIDdata[sec].OldCurr) * 600 * (float)TpPIDdata[sec].D) / (84 * (float)TpPIDdata[sec].P);	// Fu 106/05/17
		TpPIDdata[sec].Isum1 += TpPIDdata[sec].Iturn;
		//
		TpPIDdata[sec].Out =  TpPIDdata[sec].Isum1 + TpPIDdata[sec].Pturn - TpPIDdata[sec].Dturn;
		//
		if(TpPIDdata[sec].Err >= 0.0F)	// Set P >= Back P ==> +
		{
			if(TpPIDdata[sec].Pturn >= 4095)
			{
				TpPIDdata[sec].Pturn = 4095;
				TpPIDdata[sec].Isum1 = 0.0F;
			}
			else
			{
				if(TpPIDdata[sec].Isum1 >= (4095 - TpPIDdata[sec].Pturn))
				{
					TpPIDdata[sec].Isum1 = 4095 - TpPIDdata[sec].Pturn;
				}
				else
				{
				}
			}
		}
		else	// Set P < Back P ==> -
		{
			if(TpPIDdata[sec].Pturn <= (float)-4095.0F)
			{
				TpPIDdata[sec].Pturn = (float)-4095.0F;
				TpPIDdata[sec].Isum1 = 0.0F;
			}
			else 
			{
				if(TpPIDdata[sec].Isum1 <= ((float)-4095.0F - TpPIDdata[sec].Pturn))
				{
					TpPIDdata[sec].Isum1 = (float)-4095.0F - TpPIDdata[sec].Pturn;
				}
				else
				{
				}
			}
		}
		//
		if((TpPIDdata[sec].Out > OutputPwmMaxUnit[sec]) || (TpPIDdata[sec].Out < Upmin))// OutputPwmMaxUnit = 32767, Upmin = 0
		{
			if(TpPIDdata[sec].Out > OutputPwmMaxUnit[sec])
			{
				TpPIDdata[sec].Out = OutputPwmMaxUnit[sec];
			}
			else
			{
				TpPIDdata[sec].Out = Upmin;
			}
		}
		//
		Output = (unsigned short)(TpPIDdata[sec].Out);
		//
		if(TpPIDdata[sec].Err < 0)
		{
			if(fabs(TpPIDdata[sec].Err) > 40)		// 4.0 C
			{
				Output = 0;
				TpPIDdata[sec].Isum1 = 0;
			}
		}
		else
		{
			if(Output >= 4095)
			{
				Output = 4095;
			}
			else
			{
			}
		}
		//	Fu 106/07/28
		/*if(TpPIDdata[sec].Err >= 0)
		{
			if((Output100Pr[sec] != 0) && (thermal_couple[sec] <= TpPIDdata[sec].OldCurr))	// Fu 106/06/01
			{
				Output = 4095;
			}
		}
		else
		{
			if(Output100Pr[sec] == 4)
			{
				Output = Upmin;
			}
			else if(Output100Pr[sec] == 1)
			{
				Output = Output/10;
			}
		}*/
		//
		return Output;
	}
	else
	{
		j = *(tempData[0].ContrlFun);
		//
		if(j & (0x01<<sec))
		{
			dXi = (float)(12. * 60.) * (float)0.001;		// 8�q*���y�g��(760 = 6000/8)
			dXd = (float)(12. * 60.) * (float)0.001;		// 8�q*10ms���y�g��
			Ki = TpPIDdata[sec].I * 10;
			//
			if(Ki == 0) 
			{
				TpPIDdata[sec].Isum1 = 0;
			}
			//
			FloatKd[sec] = (float)(TpPIDdata[sec].D/2); 	// Fu 106/05/17
		}
		else
		{
			dXi = (float)(12 * 265)*(float)0.001;		// 8�q*10ms���y�g��
			dXd = (float)(12 * 265)*(float)0.001;		// 8�q*10ms���y�g��
			Ki = TpPIDdata[sec].I*5;
			//
			if(Ki == 0) 
			{
				TpPIDdata[sec].Isum1 = 0;
			}
			//
			FloatKd[sec] = (float)(TpPIDdata[sec].D); 	// Fu 106/05/17
		}
		//
		TpPIDdata[sec].Err = (float)(TpPIDdata[sec].Set) - (float)(thermal_couple[sec]);   // Err = Tset - Tcurr  // 當次誤差 = 設定壓力 - 目前壓力
		//
		// 2014.02.07 �T�O��F�Ĥ@���]�w�ؼЫ�, P���H�ۻ~�t���ܥ\��~�}��.
		if((TpPIDdata[sec].Err <= 0) && ((float)(TpPIDdata[sec].Set) > 300) && (TpClLpFg4 & (0x01<<sec))) // 2015/07/31 �[�JTpClLpFg4.
		{
			Temperature_First_Up_Limit_Flag |= (0x01<<sec); // 2015/07/28
		}
		// 2014.02.07 �[�J, ���ū׳]�w�ȤW�ի�, P���H�ۻ~�t����.
		if(Temperature_Up_Flag & (0x01<<sec)) // 2015/07/28
		{
			Temperature_Up_Kp1[sec] = TpPIDdata[sec].P * TpPIDdata[sec].Err / Temperature_Up_Error[sec];
			//
			if(TpPIDdata[sec].Err > Temperature_Up_Error[sec])
			{
				Temperature_Up_Kp2[sec] = TpPIDdata[sec].P;
			}
			else
			{
				Temperature_Up_Kp2[sec] = TpPIDdata[sec].P + (TpPIDdata[sec].P - Temperature_Up_Kp1[sec]);
			}			
		}
		// 2014.02.07 ���~�t��0��, P�Ȱ������.
		if((TpPIDdata[sec].Err <= 0) && (Temperature_Up_Flag & (0x01<<sec))) // 2015/07/28
		{
			Temperature_Up_Flag &= ~(0x01<<sec); // 2015/07/28
			Temperature_Up_Error[sec] = 0;
			Temperature_Up_Kp1[sec] = 0.;
			Temperature_Up_Kp2[sec] = 0.;
		}
		//
		if(TpPIDdata[sec].P >= fabs(TpPIDdata[sec].Err))// 	
		{ //within PB
			/*// Fu 106/05/17
			if(BkTempSpRamp[sec] >= 6)	// 0.6%
			{
				Output100Fg |= (0x01<<sec);
				Output100Pr[sec] = 4;
				FloatKd[sec] = (FloatKd[sec] + (BkTempSpRamp[sec] * 6320));
			}
			else if(BkTempSpRamp[sec] >= 4)	// 0.4%
			{
				Output100Fg |= (0x01<<sec);
				Output100Pr[sec] = 1;
				FloatKd[sec] = (FloatKd[sec] + (BkTempSpRamp[sec] * 2830));
			}
			else
			{
				Output100Fg &= ~(0x01<<sec);
				Output100Pr[sec] = 0;
			}*/
			Output100Pr[sec] = 0;
			//
			if(Temperature_Up_Flag & (0x01<<sec))
			{
				TpPIDdata[sec].Iturn = (32767 * (float)Ki * TpPIDdata[sec].Err * dXi) / (6000 * Temperature_Up_Kp2[sec] * 250);
				//	Fu 106/05/22
				//if((Output100Pr[sec] != 0) && (TpPIDdata[sec].Err >= 0))
				//{
					//TpPIDdata[sec].Pturn = ((TpPIDdata[sec].Err) * 32767) / (((float)Temperature_Up_Kp2[sec]/(float)Output100Pr[sec]));
					//TpPIDdata[sec].Dturn = (32767 * 0.6 * FloatKd[sec] * ((float)((BkTempSpRamp[sec] - OldBkTempSpRamp[sec])))) / ((dXd * Temperature_Up_Kp2[sec]));	// Fu 106/05/17
				//}
				//else
				//{
					TpPIDdata[sec].Pturn = ((TpPIDdata[sec].Err) * 32767) / (Temperature_Up_Kp2[sec]);
					TpPIDdata[sec].Dturn = (32767 * 0.6 * FloatKd[sec] * ((float)((thermal_couple[sec] - TpPIDdata[sec].OldCurr)))) / ((dXd * Temperature_Up_Kp2[sec]));	// Fu 106/05/17
				//}
			}
			else
			{
				TpPIDdata[sec].Iturn = (32767 * (float)Ki * TpPIDdata[sec].Err * dXi) / (6000 * (float)(TpPIDdata[sec].P) * 250);
				//	Fu 106/05/22
				//if((Output100Pr[sec] != 0) && (TpPIDdata[sec].Err >= 0))
				//{
					//TpPIDdata[sec].Pturn = ((TpPIDdata[sec].Err) * 32767) / (float)(((float)TpPIDdata[sec].P/(float)Output100Pr[sec]));// P項計算值 = 當次誤差*32767
					//TpPIDdata[sec].Dturn = (32767 * 0.6 * FloatKd[sec] * ((float)((BkTempSpRamp[sec] - OldBkTempSpRamp[sec])))) / ((dXd * (float)(TpPIDdata[sec].P)));	// Fu 106/05/17
				//}
				//else
				//{
					TpPIDdata[sec].Pturn = ((TpPIDdata[sec].Err) * 32767) / (float)(TpPIDdata[sec].P);// P項計算值 = 當次誤差*32767
					TpPIDdata[sec].Dturn = (32767 * 0.6 * FloatKd[sec] * ((float)((thermal_couple[sec] - TpPIDdata[sec].OldCurr)))) / ((dXd * (float)(TpPIDdata[sec].P)));	// Fu 106/05/17
				//}
			}
			//
			TpPIDdata[sec].Isum1 += TpPIDdata[sec].Iturn;	// I累計值 = I累計值 + I項計算值
			TpPIDdata[sec].Out =  TpPIDdata[sec].Isum1 + TpPIDdata[sec].Pturn - TpPIDdata[sec].Dturn;
			//
			if((TpPIDdata[sec].Out > OutputPwmMaxUnit[sec]) || (TpPIDdata[sec].Out < Upmin))// OutputPwmMaxUnit = 32767, Upmin = 0
			{
				if(TpPIDdata[sec].Out > OutputPwmMaxUnit[sec])
				{
					if(TpPIDdata[sec].Err >= Upmin)
					{
						TpPIDdata[sec].Isum1 -= TpPIDdata[sec].Iturn;
					}
					//
					TpPIDdata[sec].Out = OutputPwmMaxUnit[sec];
				}
				else
				{
					TpPIDdata[sec].Out = Upmin;
				}
			}
		}
		else if(thermal_couple[sec] > TpPIDdata[sec].Set)
		{
			TpPIDdata[sec].Out= Upmin;
		}
		else
		{
			TpPIDdata[sec].Out = OutputPwmMaxUnit[sec];
		}
		//
		if(TpPIDdata[sec].Isum1 <= Upmin)
		{
			TpPIDdata[sec].Isum1 = Upmin;	// ErrorSum = MIN
		}
		//
		if(TpPIDdata[sec].Isum1 >= OutputPwmMaxUnit[sec])
		{
			TpPIDdata[sec].Isum1 = OutputPwmMaxUnit[sec];
		}
		//
		if(TpPIDdata[sec].P == 0xffff)
		{
			TpPIDdata[sec].Out= Upmin;
			TpPIDdata[sec].Isum1 = Upmin;	// ErrorSum = MIN	
		}
		//
		if((TpPIDdata[sec].Err < 0))
		{
			if(fabs(TpPIDdata[sec].Err) > 150)		// 15.0 C
				TpPIDdata[sec].Out = Upmin;
		}
		//	Fu 106/05/22
		//if((TpPIDdata[sec].Err >= 0) && (TwoTpHeatFg & (0x01<<sec)) && (((*(tempData[0].SynchronFun) & (0x01<<sec)) == 0) || (((*(tempData[0].SynchronFun) & (0x01<<sec)) != 0) && (TpRangCnt >= 48)))) 	// �G�q�ɷŤw��F : Fu 107/11/01 �F��ƷŤ��~���p��
		//{
			//if((Output100Pr[sec] != 0) && (thermal_couple[sec] <= TpPIDdata[sec].OldCurr))	// Fu 106/06/01
			//{
				//TpPIDdata[sec].Out = OutputPwmMaxUnit[sec];
			//}
		//}
		//else
		//{
			//	Fu 106/05/31
			//if(TwoTpHeatFg & (0x01<<sec))	// �G�q�ɷŤw��F
				//{	// Fu 108/06/24 : �P�_�O�_���]�w�Ť�{�b�Ű�
				//if((Output100Pr[sec] != 0) && (thermal_couple[sec] >= TpPIDdata[sec].OldCurr))	
				//if((Output100Pr[sec] != 0) && (thermal_couple[sec] >= TpPIDdata[sec].Set))	
				//{
					//if(Output100Pr[sec] == 4)
					//{
						//TpPIDdata[sec].Out = Upmin;
					//}
					//else if(Output100Pr[sec] == 1)
					//{
						//TpPIDdata[sec].Out = TpPIDdata[sec].Out/10;
					//}
				//}
			//}
		//}
		//
		Output = (unsigned short)(TpPIDdata[sec].Out);
		//
		return Output;
	}
}
//
//	Fu 106/05/16
void TempSpUpDnMath(unsigned short i)
{
	unsigned short SpBkUnit;
	int AdOffset;
	//
		AdOffset = (int)thermal_couple[i] - (int)TempUpDnSpMathTab[i].Lpm_BK;
		TempUpDnSpMathTab[i].DeltaCv[1+TempUpDnSpMathTab[i].DeltaCv_head] = AdOffset;
		TempUpDnSpMathTab[i].DeltaCv[0] = (TempUpDnSpMathTab[i].DeltaCv[0] + AdOffset) - TempUpDnSpMathTab[i].DeltaCv[1+TempUpDnSpMathTab[i].DeltaCv_tail];
		TempUpDnSpMathTab[i].DeltaCvTotal = TempUpDnSpMathTab[i].DeltaCv[0];
		//
		TempUpDnSpMathTab[i].DctMb_ch[1+TempUpDnSpMathTab[i].DeltaCv_head] = TempUpDnSpMathTab[i].DeltaCvTotal;
		TempUpDnSpMathTab[i].DctMb_ch[0] = (TempUpDnSpMathTab[i].DctMb_ch[0]+TempUpDnSpMathTab[i].DeltaCvTotal) - TempUpDnSpMathTab[i].DctMb_ch[1+TempUpDnSpMathTab[i].DeltaCv_tail];
		//
		SpBkUnit = (unsigned short)abs(TempUpDnSpMathTab[i].DctMb_ch[0]);  // Fu 101/06/29 : 25ms
		BkTempSpRamp[i] = SpBkUnit;
		//
		TempUpDnSpMathTab[i].DeltaCv_head = TempUpDnSpMathTab[i].DeltaCv_tail;
		if(TempUpDnSpMathTab[i].DeltaCv_tail >= 4)  // 4 * 250ms = 1 Sec
				TempUpDnSpMathTab[i].DeltaCv_tail = OFF;
		else
				TempUpDnSpMathTab[i].DeltaCv_tail += 1;
		//
		if(*(tempData[i].PwmOutput) == 1000)	// 100% output
		{
			if(BkTempSpRamp[i] >= TempUpDnSpMathTab[i].UpMax)
			{
				TempUpDnSpMathTab[i].UpMax = BkTempSpRamp[i];
			}
		}
		//
		if(*(tempData[i].PwmOutput) == 0)	// 0% output
		{
			if(BkTempSpRamp[i] <= TempUpDnSpMathTab[i].DnMin)
			{
				TempUpDnSpMathTab[i].DnMin = BkTempSpRamp[i];
			}				 			
		}
		//
		TempUpDnSpMathTab[i].Lpm_BK = thermal_couple[i];
}
//----------------------------------------------------------
// 	2015/04/21
void TempAutoDateRun(void)
{
	unsigned short i;
	static unsigned short heatmode;
	static unsigned short oldheatmode = 0xffff, autoheatosh = 0, autopreheatosh = 0;
	static unsigned short oldautoheat = 0, oldautopreheat = 0;
	static unsigned short autoheat, autopreheat, showweek;
	//
	heatmode = *(tempData[0].HeatCtrlMd) & 0x0007; // heating & holding & autoholding
	//
	if(autoheatosh)
	{
		heatmode = 1;
		autoheatosh = 0;
		*(tempData[0].HeatCtrlMd) &= ~0x04;
		*(tempData[0].HeatCtrlMd) |= 0x01;
	}
	else if(autopreheatosh) // �q���O��(Keep Temperature)
	{
		heatmode = 2;
		autopreheatosh = 0;
		*(tempData[0].HeatCtrlMd) &= ~0x03;
		*(tempData[0].HeatCtrlMd) |= 0x02;
	}
	else if((heatmode == 0) || (heatmode == 3) || (heatmode == 4))
		*(tempData[0].HeatCtrlMd) |= heatmode;
	//
	if(heatmode < 3)
		*(tempData[0].HeatCtrlMd) |= heatmode;
	//
	if(heatmode != oldheatmode)
		oldheatmode = heatmode;
	//
	showweek = *tempData[0].Week; // ����ɶ�_New(�g)
	//
	autoheat = 0;
	autopreheat = 0;
	//
	for(i = 0; (i < AuWeekCnt) && (*tempData[0].AutoHeatMainSwitch == ON) && (*tempData[0].Year > 0); i++)
	{
		if(!(*tempData[0].AutoPerHeatBranchSwitch & (0x01<<i)) && (showweek==i) && (heatmode==3) && (((*tempData[0].Hour*100) + *tempData[0].Min) == *tempData[i].HeatPerTm) && (*tempData[i].HeatPerTm < 2360))
		{                                                                                                    
			autopreheat = 1;                                                   
			break;
		}
		//
		if(!(*tempData[0].AutoHeatBranchSwitch & (0x01<<i)) && (showweek==i) && (heatmode==4) && (((*tempData[0].Hour*100) + *tempData[0].Min) == *tempData[i].HeatTm) && (*tempData[i].HeatTm < 2360))
		{
			autoheat = 1;
			break;
		}
	}
	//
	if(!oldautoheat && autoheat)
		autoheatosh = 1;
	else if(!oldautopreheat && autopreheat)
		autopreheatosh = 1;
	//
	oldautoheat = autoheat;
	oldautopreheat = autopreheat;
}
