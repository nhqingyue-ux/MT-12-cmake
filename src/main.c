/**
  ******************************************************************************
  * @file    I2C/EEPROM/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    30-September-2011
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
//
unsigned short R_L_Normal_Temp_Dir_Fg;	// Fu 108/12/25
unsigned short R_Normal_Temp_Dir_Fg;	// Fu 108/12/25
unsigned short L_Normal_Temp_Dir_Fg;	// Fu 108/12/25
unsigned short TempPosAndNegFg=0;	// Fu 105/12/02
unsigned short DateCnt=0u;
unsigned short StartCountFlag=0u;
unsigned char eeprom_access_ret_val = 0u;  // 2018/02/06 by kf
unsigned short ADErrorFlag=0u;
volatile unsigned char i2c2_fail_step = 0;  /* debug: which step of Right_temperature_IC failed */
volatile unsigned char adc_sampling_active = 0;  /* B-1: freeze heater output during ADC sampling */
unsigned short RelayTm[12u];
unsigned short ThermostatTm[12u];
unsigned short ActThermostatTm[12u];
unsigned short thermocouple_zero_cnt[12u];
float OutputPwmMaxUnit[TpMaxLp]={32767.0F};	// Fu 108/01/03 : �ѳ�q�ܧ󬰦h�q
unsigned short TemperatureSensor[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
unsigned short TemperatureOutput[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
void BkTpCurrDisplaySecSub(void);
unsigned short CmpIOChgPointStatsSub(unsigned short Len1, unsigned short Len2, unsigned short Address, unsigned short *TpChgPoingBack, unsigned short TpChgPointBit);
void BaudSet(void);
void Date(void);
unsigned char APVer[4]="AP06";	// Fu 107/07/13
unsigned short DnApSysFg=0u;	// Fu 107/07/05;
unsigned short DelayApToBlTime=0u;	// Fu 107/07/05

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/** @addtogroup I2C_EEPROM
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

/* Private define ------------------------------------------------------------*/
/* Uncomment the following line to enable using LCD screen for messages display */

/* Private macro -------------------------------------------------------------*/

unsigned short rand_seed;

unsigned long bootloader_ver;

extern unsigned char alpum_tx_data[8];
extern unsigned char alpum_rx_data[10];
extern unsigned char alpum_ex_data[8];

// For EEPROM
extern unsigned char read_EEPROM(unsigned long, unsigned short*, unsigned short);
extern unsigned char write_EEPROM(unsigned long, unsigned short*, unsigned short);

// For ALPU-M
extern unsigned char _alpu_rand(void);
extern void _ALPU_action(void);

// RS485 RX state (for init cleanup)
extern unsigned short buffer_index;
extern unsigned char package_in_flag;
extern unsigned short TimerBase_package_cut;
extern unsigned short FirstRsTxRxStsOKFlag;
extern unsigned short TpClLpFg3;
extern unsigned short ThermostatFun;
extern unsigned short RealTpOutPwm;
extern unsigned short BkPidPUnit[12];

#include "dbg_uart.h"

/* Private function prototypes -----------------------------------------------*/

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private functions ---------------------------------------------------------*/

short Right_temperature_IC(void);
short Left_temperature_IC(void);

/*
 * I2C2 bus recovery — STM32F4 errata workaround.
 * Bit-bang 9 SCL pulses + STOP to release stuck slave, then re-init peripheral.
 */
static void I2C2_BusRecovery(void)
{
	GPIO_InitTypeDef gpio;
	unsigned char i;

	I2C_Cmd(I2C2, DISABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, DISABLE);
	DELAY_CYCLES(500);

	gpio.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
	gpio.GPIO_Mode  = GPIO_Mode_OUT;
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &gpio);

	GPIO_SetBits(GPIOB, GPIO_Pin_10);
	GPIO_SetBits(GPIOB, GPIO_Pin_11);
	DELAY_CYCLES(1000);

	for(i = 0; i < 9; i++)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_10);
		DELAY_CYCLES(500);
		GPIO_SetBits(GPIOB, GPIO_Pin_10);
		DELAY_CYCLES(500);
	}

	GPIO_ResetBits(GPIOB, GPIO_Pin_11);  /* SDA LOW */
	DELAY_CYCLES(500);
	GPIO_SetBits(GPIOB, GPIO_Pin_10);    /* SCL HIGH */
	DELAY_CYCLES(500);
	GPIO_SetBits(GPIOB, GPIO_Pin_11);    /* SDA HIGH → STOP */
	DELAY_CYCLES(500);

	gpio.GPIO_Pin   = GPIO_Pin_10 | GPIO_Pin_11;
	gpio.GPIO_Mode  = GPIO_Mode_AF;
	gpio.GPIO_OType = GPIO_OType_OD;
	gpio.GPIO_PuPd  = GPIO_PuPd_UP;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	DELAY_CYCLES(200);
	I2C_SoftwareResetCmd(I2C2, ENABLE);
	DELAY_CYCLES(500);
	I2C_SoftwareResetCmd(I2C2, DISABLE);
	DELAY_CYCLES(200);

	{
		I2C_InitTypeDef I2C_InitStructure;
		I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
		I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;
		I2C_InitStructure.I2C_OwnAddress1 = 0x01;
		I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
		I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
		I2C_InitStructure.I2C_ClockSpeed = 400000;
		I2C_Init(I2C2, &I2C_InitStructure);
		I2C_Cmd(I2C2, ENABLE);
	}
}
//
// Fu 107/01/29
//unsigned long Tm1Cnt=0;
//unsigned long Tm2Cnt=0;
//unsigned long Tm3Cnt=0;
//unsigned long MainCnt=0;
//unsigned short MainErrCnt=0;

void Read_Single_AD(unsigned char, unsigned char);
void Vbias_Set(unsigned short, unsigned short);
void Sel_Channel(unsigned char, unsigned char);

/**
  * @brief   Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	unsigned int wait, wait1;
	unsigned short i;
	//
	//	Fu 107/07/05
	DnApSysFg = 0u;
	DelayApToBlTime = 0u;
	//
	// Wait 16 ms for internal reset
	for(wait = 0; wait < 11; wait++)
		for(wait1 = 0; wait1 < 50000; wait1++);
	//
	HW_config();
	//
	init();
	//
	/* I2C2 bus recovery after init() — init calls Right_temperature_IC()
	   which may leave I2C2 BUSY if the right TMP112 is absent or NACKs */
	if(I2C2->SR2 & 0x0002) {
		I2C2_BusRecovery();
	}
	/* Probe I2C2 address 0x49 (right TMP112) to check if device is present */
	{
		unsigned short tc = 0;
		I2C_GenerateSTART(I2C2, ENABLE);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)) {
			if(++tc > 5000) break;
		}
		I2C_Send7bitAddress(I2C2, (R_TMP112_ADDRESS << 1), I2C_Direction_Transmitter);
		tc = 0;
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))) {
			if(++tc > 5000) break;
		}
		if(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)) {
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			dbg_puts("[I2C2] TMP112@0x49 NOT found (NACK)\r\n");
		} else {
			dbg_puts("[I2C2] TMP112@0x49 ACK OK!\r\n");
		}
		I2C_GenerateSTOP(I2C2, ENABLE);
		DELAY_CYCLES(5000);
		if(I2C2->SR2 & 0x0002) I2C2_BusRecovery();
	}
	//
	/* Print RCC clock config for diagnosis */
	{
		RCC_ClocksTypeDef clk;
		RCC_GetClocksFreq(&clk);
		dbg_puts("\r\n[CLK] SYSCLK="); dbg_putu(clk.SYSCLK_Frequency);
		dbg_puts(" HCLK="); dbg_putu(clk.HCLK_Frequency);
		dbg_puts(" PCLK1="); dbg_putu(clk.PCLK1_Frequency);
		dbg_puts(" PCLK2="); dbg_putu(clk.PCLK2_Frequency);
		dbg_puts("\r\n PLLM="); dbg_putu(RCC->PLLCFGR & 0x3F);
		dbg_puts(" PLLN="); dbg_putu((RCC->PLLCFGR >> 6) & 0x1FF);
		dbg_puts(" PLLP="); dbg_putu(((RCC->PLLCFGR >> 16) & 0x3) * 2 + 2);
		dbg_puts(" PLLSRC="); dbg_putu((RCC->PLLCFGR >> 22) & 1);
		dbg_puts(((RCC->PLLCFGR >> 22) & 1) ? " (HSE)\r\n" : " (HSI)\r\n");
	}
	dbg_puts("[I2C2] SR1="); dbg_putu(I2C2->SR1);
	dbg_puts(" SR2="); dbg_putu(I2C2->SR2);
	dbg_puts(" CR1="); dbg_putu(I2C2->CR1);
	dbg_puts(" BUSY="); dbg_putu((I2C2->SR2 >> 1) & 1);
	dbg_puts("\r\n");
	dbg_puts("[INIT] l="); dbg_puti(l_temperature);
	dbg_puts(" r="); dbg_puti(r_temperature);
	dbg_puts(" lr="); dbg_puti(l_r_temperature);
	dbg_puts(" err="); dbg_putu((unsigned)error_temperature);
	dbg_puts("\r\n");
	/* Ensure I2C2 BUSY is clear before CDMInit reads EEPROM via I2C2 */
	if(I2C2->SR2 & 0x0002) {
		I2C2_BusRecovery();
		dbg_puts("[I2C2] Pre-CDMInit recovery, BUSY=");
		dbg_putu((I2C2->SR2 >> 1) & 1);
		dbg_puts("\r\n");
	}
	CDMInit();
	//
	/* Print EEPROM read result and PID parameters */
	dbg_puts("[EEPROM] ret="); dbg_putu(eeprom_access_ret_val);
	dbg_puts(" P0="); dbg_putu(*(tempData[0].PIDp));
	dbg_puts(" I0="); dbg_putu(*(tempData[0].PIDi));
	dbg_puts(" D0="); dbg_putu(*(tempData[0].PIDd));
	dbg_puts(" Set0="); dbg_putu(*(tempData[0].HeatSet));
	dbg_puts("\r\n");
	//
	while(!I2C1_MUX_lock());
	TempIintData();
	I2C1_MUX_unlock();
	//
	BaudSet(); // 2015/08/12
	//
	// wait ~570 ms delay (one time only at power up)
	for(wait = 0; wait < 384; wait++)
		DELAY_CYCLES(50000);
	//
	dbg_puts("[PRE-BK] CDM20201="); dbg_putu(*GetPtrCDM2(20201));
	dbg_puts(" PIDp_ptr="); dbg_putu(*(tempData[0].PIDp));
	dbg_puts(" pA="); dbg_putu((unsigned)(pA20001));
	dbg_puts(" PIDp="); dbg_putu((unsigned)(tempData[0].PIDp));
	dbg_puts(" diff="); dbg_putu((unsigned)(tempData[0].PIDp - pA20001));
	dbg_puts("\r\n");
	for(i=0; i<12; i++)
	{
		BkPidPUnit[i] = *(tempData[i].PIDp);
		BkPidIUnit[i] = *(tempData[i].PIDi);
		BkPidDUnit[i] = *(tempData[i].PIDd);
	}
	dbg_puts("[PID] P0="); dbg_putu(BkPidPUnit[0]);
	dbg_puts(" I0="); dbg_putu(BkPidIUnit[0]);
	dbg_puts(" D0="); dbg_putu(BkPidDUnit[0]);
	dbg_puts("\r\n");
	//
	*GetPtrCDM2(20559u) = 0u;	// Fu 107/07/05
	// Fu 107/07/13
	APVer[2] = *(unsigned char *)0x2001FFF8u;
	APVer[3] = *(unsigned char *)0x2001FFF9u;
	//
	// Fix: Clear RS485 state before main loop.
	// During init, TIM5 ISR processes HMI requests → sets RTS HIGH + SendRs485Fg=1.
	// Main loop hasn't started → RS485TxSub never runs → RTS stuck HIGH.
	// Also flush stale USART RX data to prevent parsing garbage.
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
		USART_ReceiveData(USART1);
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET)
		USART_ReceiveData(USART1);
	buffer_index = 0;
	package_in_flag = 0;
	TimerBase_package_cut = 0;
	USART1_RTS_L;
	SendRs485Fg = 0;
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	//
	/* Debug: print ADS1248 diagnostics */
	dbg_puts("\r\n[BOOT] UART4 OK\r\n");
	while (1u)
	{
// Fu 107/01/29
//		MainCnt = 0;	// !!!!!!!
//		MainErrCnt = 1;
		//
		/* --- Multi-channel debug log --- */
		{
			static unsigned int dbg_iter = 0;
			if(++dbg_iter >= 1000) {
				dbg_iter = 0;
				unsigned char ch;
				/* SecTempSw, TpClLpFg3, HeatFlashBit (per-channel bit fields) */
				dbg_puts("[ST] HCM="); dbg_putu(*(tempData[0].HeatCtrlMd));
				dbg_puts(" Fg="); dbg_putu(TpClLpFg3);
				dbg_puts(" Flash="); dbg_putu(*(tempData[0].HeatFlashBit));
				dbg_puts(" RPwm="); dbg_putu(RealTpOutPwm);
				dbg_puts("\r\nPV: ");
				for(ch = 0; ch < 12; ch++) {
					dbg_putu(*(tempData[ch].TpDisplay));
					dbg_putc(' ');
				}
				dbg_puts("\r\nST: ");  /* HeatSet per channel */
				for(ch = 0; ch < 12; ch++) {
					dbg_putu(*(tempData[ch].HeatSet));
					dbg_putc(' ');
				}
				dbg_puts("\r\nPW: ");  /* PwmOutput per channel */
				for(ch = 0; ch < 12; ch++) {
					dbg_putu(*(tempData[ch].PwmOutput));
					dbg_putc(' ');
				}
				dbg_puts("\r\n");
			}
		}
		//
		if(Int1Sts)
			LED_GRN_H;
		else
			LED_GRN_L;
		if(RsTxRxSts)
			LED_RED_H;
		else
			LED_RED_L;
		//
		//MainErrCnt = 2;
		TempDisplay();
		//
		//MainErrCnt = 3;
		HeatErrMd();
		//
		//MainErrCnt = 4;
		RS485TxSub();
		//
		//if(I2C1_MUX_lock())
		//{
		//MainErrCnt = 5;  // !!!!!!!
			EEPROMCmp();
			//I2C1_MUX_unlock();
		//}
		//
		//MainErrCnt = 6;
		//
		Date();
		//
		if(*GetPtrCDM2(20558u) != 0u)
		{	
			i = DataInitSet(*GetPtrCDM2(20558u));
			//
			//if(i != 0x1368)
			if(i == 0x1368u)
			{
				*GetPtrCDM2(20558u) = 0u;
			}
		}
		// AP to Bootloader // Fu 107/07/05
		if((DnApSysFg == 0x2479u) || (*GetPtrCDM2(20559u) == 0x1368u) || (*GetPtrCDM2(20559u) == 0x2479u))
		{
				if(DelayApToBlTime >= 10u)
				{
					if(DnApSysFg == 0x2479)
					{
						for(i=0u; i<10u; i++)
						{
							*(unsigned char *)0x2001FFFBu = 1u;
							*GetPtrCDM2(20559u) = 0;
						}
						//
						NVIC_SystemReset();
					}
					else
					{
						*GetPtrCDM2(20559u) = 0x2479u;
						//
						if(DelayApToBlTime >= 20u)
						{
							for(i=0u; i<10u; i++)
							{
								*(unsigned char *)0x2001FFFBu = 1u;
								*GetPtrCDM2(20559u) = 0;
							}
							//
							NVIC_SystemReset();
						}
					}
				}
		}
	}
}
#ifdef I2C1_MUX
////////////////////////////
//
//
unsigned char
I2C1_MUX_lock(void)
{
	static unsigned char order = 0;
	
	order++;
	if(order == 1)
	{
		if(i2c1_mux)
		{
			order--;
			return 0;
		}
		else
		{
			i2c1_mux++;
			if(i2c1_mux == 1)
			{
				order--;
				return 1;
			}
			else
				i2c1_mux--;
		}
	}
	order--;
	return 0;
}
void
I2C1_MUX_unlock(void)
{
	if(i2c1_mux)
		i2c1_mux--;
}
#endif
#ifdef I2C2_MUX
////////////////////////////
//
//
unsigned char
I2C2_MUX_lock(void)
{
	static unsigned char order = 0;

	order++;
	if(order == 1)
	{
		if(i2c2_mux)
		{
			order--;
			return 0;
		}
		else
		{
			i2c2_mux++;
			if(i2c2_mux == 1)
			{
				order--;
				return 1;
			}
			else
				i2c2_mux--;
		}
	}
	order--;
	return 0;
}
void
I2C2_MUX_unlock(void)
{
	if(i2c2_mux)
		i2c2_mux--;
}
#endif
////////////////////////////
//
//
void EEPROMCmp(void)
{
	unsigned short i, cnt;
	//								 
	for(i=0; i<500; i++)
	{
		if(*GetPtrCDM2(20001+i) != *GetPtrCDM3(30001+i))
		{
			BkTpCurrDisplaySecSub(); // 2014/10/16 IO��V.
			/*databack[0] = *GetPtrCDM2(20001+i);
			*GetPtrCDM3(30001+i) = databack[0];
			cnt = 1;
			write_FRAM(0x200+i, databack, cnt);
			break;*/
			//
			databack[0] = *GetPtrCDM2(20001+i);  // 2017/11/06 by kf
			cnt = 1;  // 2017/11/06 by kf
			if(I2C1_MUX_lock())  // 2017/11/06 by kf
			{
//				write_FRAM(0x200+i, databack, cnt);  // 2017/11/06 by kf  // 2018/02/06 by kf
				eeprom_access_ret_val = write_FRAM(0x200+i, databack, cnt);  // 2018/02/06 by kf
				I2C1_MUX_unlock();  // 2017/11/06 by kf
				*GetPtrCDM3(30001+i) = databack[0];  // 2017/11/06 by kf
			}
			break;									   
		}
	}
	//
	if(*(tempData[0].KAndJSel) != OldKAndJSel)
	{
		OldKAndJSel = *(tempData[0].KAndJSel);
		TpTypeMdSet();
	}
	// Fu 108/12/25 : �[�J�ūץ��t���A
	if(thermal_PosAndNeg[0][0] != 0)  
	{
		*(tempData[0].TempLinearErr) |= 0x0001;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0001);
	}
	//
	if(thermal_PosAndNeg[0][1] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0002;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0002);
	}
	//
	if(thermal_PosAndNeg[1][0] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0004;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0004);
	}
	//
	if(thermal_PosAndNeg[1][1] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0008;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0008);
	}
	//
	if(thermal_PosAndNeg[2][0] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0010;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0010);
	}
	//
	if(thermal_PosAndNeg[2][1] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0020;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0020);
	}
	//
	if(thermal_PosAndNeg[0][2] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0040;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0040);
	}
	//
	if(thermal_PosAndNeg[0][3] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0080;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0080);
	}
	//
	if(thermal_PosAndNeg[1][2] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0100;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0100);
	}
	//
	if(thermal_PosAndNeg[1][3] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0200;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0200);
	}
	//
	if(thermal_PosAndNeg[2][2] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0400;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0400);
	}
	//
	if(thermal_PosAndNeg[2][3] != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x0800;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x0800);
	}	
	// Fu 108/12/25 : �[�J�`�ť��t�V�ūת��A
	if(L_Normal_Temp_Dir_Fg != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x1000;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x1000);
	}	
	//
	if(R_Normal_Temp_Dir_Fg != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x2000;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x2000);
	}	
	//
	if(R_L_Normal_Temp_Dir_Fg != 0) 
	{
		*(tempData[0].TempLinearErr) |= 0x4000;
	}
	else
	{
		*(tempData[0].TempLinearErr) &= ~(0x4000);
	}		
	//
	//	Fu 100/12/01
	//	�P�_�]�w�ȬO�_���ܤ� , �p��, ���G�q�W�ť\��FLAG���M�����ʧ@
	//
	for(i=0; i<TpMaxLp; i++)
	{
		if(TpPIDdata[i].OldControl != *tempData[i].TpControl)
		{
			TpPIDdata[i].OldControl = *tempData[i].TpControl;
			TwoTpHeatFg &= ~(0x01<<i);
			TwoTpFg &= ~(0x01<<i);
		}
	}
	//	
	//	Fu 101/08/30
	//
	for(i=0; i<12; i++)
	{
		ThermostatTm[i] = ((*(tempData[i].Thermostat) * Sec0100_TRIG_MS)/100);
		ActThermostatTm[i] = (ThermostatTm[i] * *(tempData[i].Proportion))/100;
		RelayTm[i] = *(tempData[i].CycleTm);
		// Fu 108/01/03 : �i�H�C�q�W�߿��MT12�Ҧ��άOMJ86�Ҧ�
		if((*(tempData[0].ThermostatFun) & 0x8000) && ((*tempData[0].TwoUpFun & (0x01<<i)) == 0))	// 15 Bit : MJ86 - 12��PID�Ҧ�
		{ 
			OutputPwmMaxUnit[i] = 4095;
		}
		else
		{
			OutputPwmMaxUnit[i] = Upmax;
		}
	}
	//	Fu 106/07/19
	//if(*(tempData[0].ThermostatFun) & 0x4000)	// 14 Bit
	//{
		//PwmCounterUnit = 2047;
	//}
	//else
	//{
		PwmCounterUnit = 32767;
	//}
}
////////////////////////////
//
//
void RS485TxSub(void)
{
	unsigned short cnt, i, DataLen;
	//
	if(TimerBase22 >= Sec10_TRIG_MS)	   	// 10 sec
	{
		TimerBase22 = Sec10_TRIG_MS;
		RsTxRxSts = 0;
	}
	else
	{
		RsTxRxSts = 0x04;
	}
	//
	if(SendRs485Fg != 0u)
	{
		// Send data from RS-485
		if(SendRs485Fg == 1)
		{
			SendRs485Fg = 2;
			for(i=0; i<250; i++)
				tx_buffer_bk[i] = tx_buffer[i];
			TimerBase3=0;
		}
		else
		{
			if(TimerBase3 >= Sec0030_TRIG_MS)	// 30ms
			{
				if(tx_buffer_bk[1]  == 0x06)
				{
					for(cnt=0; cnt<6; cnt++) 	
					{
						while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
						USART_SendData(USART1, tx_buffer_bk[cnt]);  		// Low byte of device address
					}  // end for
					// Send CRC16 code from RS-485
					while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
					USART_SendData(USART1, tx_buffer_bk[6]);  		// Low byte of device address
					while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
					USART_SendData(USART1, tx_buffer_bk[7]);  		// Low byte of device address
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
					SendRs485Fg = 0;  // 2018/02/26 by kf
					// Set USART1_RTS 'L' for RS-232 convert RS-485 IC to receive data
					USART1_RTS_L;
//					SendRs485Fg = 0;  // 2018/02/26 by kf
				}
				else
				{
					DataLen = tx_buffer_bk[2];
					for(cnt=0; cnt<DataLen+3; cnt++) 	
					{
						while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
						USART_SendData(USART1, tx_buffer_bk[cnt]);  		// Low byte of device address
					}  // end for
					// Send CRC16 code from RS-485
					while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
					USART_SendData(USART1, tx_buffer_bk[DataLen+3]);  		// Low byte of device address
					while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) != SET);
					USART_SendData(USART1, tx_buffer_bk[DataLen+4]);  		// Low byte of device address
					while(USART_GetFlagStatus(USART1, USART_FLAG_TC) != SET);
					SendRs485Fg = 0;  // 2018/02/26 by kf
					// Set UART1_RTS 'L' for RS-232 convert RS-485 IC to receive data
					USART1_RTS_L;
//					SendRs485Fg = 0;  // 2018/02/26 by kf
					// Fu 107/07/05
					if(DnApSysFg == 0x1368)
					{
						DnApSysFg = 0x2479;
					}
				}
			}
		}
	}
}
// 
#ifdef EXTI_ENABLE
void BURN_DETECTED(void)   
#endif
{
	if(EXTI_GetITStatus(EXTI_Line12) != RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line12);
		BURN_DETECT = 1;
		heat = 0xFFFF;
		HT_OUT(heat);  // Modify heat out
	}
}
///////////////////////////////////////////////////////////////////////////
//
// 				Set Vbias register of ADS1248 function
//
//  1st parameter<cs> represents enable which chip select of 3 ADS1248, 
//    bit 0(LSB): CHIP1, bit 1: CHIP2, bit 2: CHIP3
//  2nd parameter<vbias_reg> represents what value will be write VBIAS
//    register(bit 0:AIN0, bit 1:AIN1, bit 2:AIN2, ...etc
//
///////////////////////////////////////////////////////////////////////////
void
Vbias_Set(unsigned short cs, unsigned short vbias_reg)
{
	unsigned short wait;
	if(cs & 0x01)
		T_CS1_L;
	if(cs & 0x02)
		T_CS2_L;
	if(cs & 0x04)
		T_CS3_L;
	
	WREG(0x0, cs, 0x41);
	WREG(0x0, cs, 0x00);
	WREG(0x0, cs, vbias_reg);
	T_SCLK_L;
	T_DIN_L;
	// Delay more than 1715 ns
	DELAY_CYCLES(40);
	T_CS2_H;
	T_CS13_H;
	DELAY_CYCLES(3);
}

///////////////////////////////////////////////////////////////////////////
//
// 				Select channel in ADS1248 for conversion
//
//  1st parameter<cs> represents enable which chip select of 3 ADS1248, 
//    bit 0(LSB): CHIP1, bit 1: CHIP2, bit 2: CHIP3
//  2nd parameter<mux_reg> represents what value will be write MUX
//    register to indicate which channel be select
//
///////////////////////////////////////////////////////////////////////////
void
Sel_Channel(unsigned char cs, unsigned char mux_reg)
{
	unsigned short wait;
	if(cs & 0x01)
		T_CS1_L;
	if(cs & 0x02)
		T_CS2_L;
	if(cs & 0x04)
		T_CS3_L;
	WREG(0x0, cs, 0x40);
	WREG(0x0, cs, 0x00);
	WREG(0x0, cs, mux_reg);
	T_SCLK_L;
	T_DIN_L;
	// Delay more than 1715 ns
	DELAY_CYCLES(40);
	T_CS2_H;
	T_CS13_H;
	DELAY_CYCLES(3);
}

///////////////////////////////////////////////////////////////////////////
//
// 				Set PGA_DOR register of ADS1248 function
//
//  1st parameter<cs> represents enable which chip select of 3 ADS1248, 
//    bit 0(LSB): CHIP1, bit 1: CHIP2, bit 2: CHIP3
//  2nd parameter<sys0_reg> represents what value will be write SYS0
//    register to indicate what value be assign to PGA and DOR
//
///////////////////////////////////////////////////////////////////////////
void
PGA_DOR_Set(unsigned char cs, unsigned char sys0_reg)
{
	unsigned short wait;
	if(cs & 0x01)
		T_CS1_L;
	if(cs & 0x02)
		T_CS2_L;
	if(cs & 0x04)
		T_CS3_L;
	WREG(0x0, cs, 0x43);
	WREG(0x0, cs, 0x00);
	WREG(0x0, cs, sys0_reg);
	T_SCLK_L;
	T_DIN_L;
	// Delay more than 1715 ns
	DELAY_CYCLES(40);
	T_CS2_H;
	T_CS13_H;
	DELAY_CYCLES(3);
}

///////////////////////////////////////////////////////////////////////////
//
// 				Read conversion data of one of three ADS1248 chips
//
//  1st parameter<cs> represents enable which chip select of 3 ADS1248, 
//    bit 0(LSB): CHIP1, bit 1: CHIP2, bit 2: CHIP3
//  2nd parameter<mux_channel> represents what channel will be select as
//    command
//
///////////////////////////////////////////////////////////////////////////
void
Read_Single_AD(unsigned char cs, unsigned char mux_channel)
{
	unsigned short wait;

	if(cs & 0x01)
		T_CS1_L;
	if(cs & 0x02)
		T_CS2_L;
	if(cs & 0x04)
		T_CS3_L;

	for(reg_index = 0; reg_index < 3; reg_index++)
	{
		bit_shift = 0x80;
		do
		{
				T_SCLK_H;
			if(reg[mux_channel][reg_index] & bit_shift)
				T_DIN_H;
			else
				T_DIN_L;
			DELAY_CYCLES(20);  // ADS1248 DOUT output delay (t_dod) ~100ns after SCLK rise
			T_DO[0] = ((T_DO[0] << 1) | (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)? 0x1 : 0x0));
			T_DO[1] = ((T_DO[1] << 1) | (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)? 0x1 : 0x0));
			T_DO[2] = ((T_DO[2] << 1) | (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11)? 0x1 : 0x0));
			T_SCLK_L;
			bit_shift = bit_shift >> 1;
			if(bit_shift != 0x00)
				DELAY_CYCLES(15);
			else
				DELAY_CYCLES(2);
		}while(bit_shift != 0x00);
	}
	//
	for(wait = 0; wait < 3; wait++)  // 2015/05/29 by kf
	{
		if((T_DO[wait] == 0x0) || (T_DO[wait] == 0xFFFFFF))  // 2015/05/29 by kf
		{
			if(thermocouple_zero_cnt[wait] < ADS1248_MAX_ZERO_COUNT)  // 2015/05/29 by kf
				thermocouple_zero_cnt[wait]++;  // 2015/05/29 by kf
			else if(thermocouple_zero_cnt[wait] >= ADS1248_MAX_ZERO_COUNT)
				ADErrorFlag |= (0x02<<wait);
		}
		else  // 2015/05/29 by kf
		{
			thermocouple_zero_cnt[wait] = 0;  // 2015/05/29 by kf
			ADErrorFlag &= ~(0x02<<wait);
		}
	}
	T_SCLK_L;
	T_DIN_L;
	// Delay more than 1715 ns
	DELAY_CYCLES(40);
	T_CS2_H;
	T_CS13_H;
	//
	if(cs & 0x01)
		thermal[0] = (unsigned short)(T_DO[0] >> 8);
	if(cs & 0x02)
		thermal[1] = (unsigned short)(T_DO[1] >> 8);
	if(cs & 0x04)
		thermal[2] = (unsigned short)(T_DO[2] >> 8);
	for(wait = 0; wait < 3; wait++)
		T_DO[wait] = 0;
}

///////////////////////////////////////////////////////////////////////////
//
// 				Set Vbias register of ADS1248 function
//
//  1st parameter<chip_no> represents which chip's data want to transform 
//    temperature data, 0:indicate CHIP1, 1:indicate CHIP2, 2:indicate CHIP3
//  2nd parameter<channel_no> represents which table corresponding to select
//    channel will be used to calculate temperature, 0:indicate channel 1 
//	  1:indicate channel 2, 2:indicate channel 3, 3:indicate channel 4
//
///////////////////////////////////////////////////////////////////////////
//unsigned short  // 2017/06/09 by kf
/*short
AD2Temp(unsigned short chip_no, unsigned short channel_no)
{
	unsigned short i, j;
	short temp = 0;  // 2017/06/09 by kf
	//
	// Calculate Chip 1 ~ 3thermocouple
	//
	if((*(tempData[0].KAndJSel)) & (0x1 << (chip_no * 4 + channel_no)))
	{
		if((short)thermal[chip_no] < (short)ch1[tp_ch_index[chip_no][channel_no]][4])  // 2017/06/09 by kf
		{
			// Let variable-temp acts as middle, i as left and j as right in binary search
			for(i = 0, j = 4, temp = (i + j) / 2; (j - i) > 1; temp = (i + j) / 2)  // 2017/06/09 by kf
			{
				if((short)thermal[chip_no] > (short)ch1[tp_ch_index[chip_no][channel_no]][temp])  // 2017/06/09 by kf
					i = temp;  // 2017/06/09 by kf
				else  // 2017/06/09 by kf
					j = temp;  // 2017/06/09 by kf
			}
			temp = ((((short)thermal[chip_no] - (short)ch1[tp_ch_index[chip_no][channel_no]][i]) * 400) / ((short)ch1[tp_ch_index[chip_no][channel_no]][j] - (short)ch1[tp_ch_index[chip_no][channel_no]][i])) + (400*i) - 1600;  // 2017/06/09 by kf
			return temp;  // 2017/06/09 by kf
		}
		for(i=4, j=0 ;i<(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1); i++, j++)
		{
			//		// ??										// ????
			if(((thermal[chip_no] & 0x8000) == 0x8000))
			{
				temp = 0;
				break;
			}		 // ??                                      // ??
			else if((thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][4]))
			{
				temp = 0;
				break;
			}		 // ??                                      // ??
			else if((thermal[chip_no] == 0x7fff) ||  (thermal[chip_no] >= ch1[tp_ch_index[chip_no][channel_no]][(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1)]))
			{
				temp = 12000;
				break;
			}
			else
			{
				if(thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][i+1])
				{
					temp = (unsigned short)((((long)thermal[chip_no] - (long)ch1[tp_ch_index[chip_no][channel_no]][i]) * 400) / ((long)ch1[tp_ch_index[chip_no][channel_no]][i+1] - (long)ch1[tp_ch_index[chip_no][channel_no]][i])) + (400*j);
					break;
				}
			}
		}

	}
	else
	{
		if((short)thermal[chip_no] < (short)ch1[tp_ch_index[chip_no][channel_no]][4])  // 2017/06/09 by kf
		{
			// Let variable-temp acts as middle, i as left and j as right in binary search
			for(i = 0, j = 4, temp = (i + j) / 2; (j - i) > 1; temp = (i + j) / 2)  // 2017/06/09 by kf
			{
				if((short)thermal[chip_no] > (short)ch1[tp_ch_index[chip_no][channel_no]][temp])  // 2017/06/09 by kf
					i = temp;  // 2017/06/09 by kf
				else  // 2017/06/09 by kf
					j = temp;  // 2017/06/09 by kf
			}
			temp = ((((short)thermal[chip_no] - (short)ch1[tp_ch_index[chip_no][channel_no]][i]) * 400) / ((short)ch1[tp_ch_index[chip_no][channel_no]][j] - (short)ch1[tp_ch_index[chip_no][channel_no]][i])) + (400*i) - 1600;  // 2017/06/09 by kf
			return temp;  // 2017/06/09 by kf
		}
		for(i=4, j=0 ;i<(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1); i++, j++)
		{
			//		// ??										// ????
			if(((thermal[chip_no] & 0x8000) == 0x8000))
			{
				temp = 0;
				break;
			}		 // ??                                      // ??
			else if((thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][4]))
			{
				temp = 0;
				break;
			}		 // ??                                      // ??
			else if((thermal[chip_no] == 0x7fff) ||  (thermal[chip_no] >= ch1[tp_ch_index[chip_no][channel_no]][(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1)]))
			{
				temp = 12000;
				break;
			}
			else
			{
				if(thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][i+1])
				{
					temp = (unsigned short)((((long)thermal[chip_no] - (long)ch1[tp_ch_index[chip_no][channel_no]][i]) * 400) / ((long)ch1[tp_ch_index[chip_no][channel_no]][i+1] - (long)ch1[tp_ch_index[chip_no][channel_no]][i])) + (400*j);
					break;
				}
			}
		}
	}
	return temp;
}*/
unsigned short AD2Temp(unsigned short chip_no, unsigned short channel_no)
{
	unsigned short i, j;
	static unsigned short BkTempHwUnit=0u;
	//
	calc = 0;
	for(i=4, j=0 ;i<(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1); i++, j++)
	{
		//		// �t��										// �s�ץH�U
		if(((thermal[chip_no] & 0x8000) == 0x8000) || (thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][4]))		 	
		{
			//calc = 0;
			//	Fu 105/12/02
			// 1558 : -40��
			// 2985 : -80��
			// 4234 : -120��
			TempPosAndNegFg = 1;
			if((thermal[chip_no] & 0x8000) == 0x8000)
			{
				BkTempHwUnit = (~(thermal[chip_no])) + 1;  // 2's complement
			}
			else
			{
				BkTempHwUnit = thermal[chip_no];
			}
			//
			if(BkTempHwUnit <= 1558)
			{
				calc = (BkTempHwUnit * 400) / 1558;
			}
			else if(BkTempHwUnit <= 2985)
			{
				calc = ((BkTempHwUnit-1558) * 400) / (2985-1558) + 400;
			}
			else
			{
				calc = ((BkTempHwUnit-2985) * 400) / (4234-2985) + 800;
			}
			break;
		}		 // �}��                                      // �_��
		else if((thermal[chip_no] == 0x7fff) ||  (thermal[chip_no] >= ch1[tp_ch_index[chip_no][channel_no]][(ch1_sec_num[tp_ch_index[chip_no][channel_no]]-1)]))
		{
			calc = 12000;
			break;
		}
		else
		{
			if(thermal[chip_no] <= ch1[tp_ch_index[chip_no][channel_no]][i+1])
			{
				calc = (unsigned short)((((long)thermal[chip_no] - (long)ch1[tp_ch_index[chip_no][channel_no]][i]) * 400) / ((long)ch1[tp_ch_index[chip_no][channel_no]][i+1] - (long)ch1[tp_ch_index[chip_no][channel_no]][i])) + (400*j);
				break;
			}
		}
	}
	//
	return calc;
}
///////////////////////////////////////////////////////////////////////////
//
//	Enable Vbias or Burnout Current in specific channel of specific CHIP
//
//  1st parameter<ic_no> represents which chip will be modify Vbias and  
//    Burnout utilites, 0:indicate CHIP1, 1:indicate CHIP2, 2:indicate CHIP3
//  2nd parameter<channel_no> represents which table corresponding to select
//    channel will be used to calculate temperature, 0:indicate channel 1 
//	  1:indicate channel 2, 2:indicate channel 3, 3:indicate channel 4
//  3rd parameter<vbias> represents if accord ic_no and channel_no to turn
//    on/off vbias, 0:indicate turn off, non-zero:indicate turn on
//  4th parameter<burnout> represents if accord ic_no and channel_no to turn
//    on/off burnout current, 0:indicate turn off, non-zero:indicate turn on
//
//	If both vbias and burnout are turn off, this function equal select a 
//    channel to convert(MUX)
//
///////////////////////////////////////////////////////////////////////////
void
Temp(unsigned char ic_no, unsigned char channel_no, unsigned char vbias, unsigned char burnout)
{
//	unsigned short wait;
	unsigned char MUX_reg, VBIAS_reg, cs;
	
	switch(channel_no)
	{
		case 0:
			if(burnout)
				MUX_reg = 0x81;  // 10.000.001
			else
				MUX_reg = 0x01;  // 00.000.001
			VBIAS_reg = 0x01;  // 00000001
			break;
		case 1:
			if(burnout)
				MUX_reg = 0x93;  // 10.010.011
			else
				MUX_reg = 0x13;  // 00.010.011
			VBIAS_reg = 0x04;  // 00000100
			break;
		case 2:
			if(burnout)
				MUX_reg = 0xA5;  // 10.100.101
			else
				MUX_reg = 0x25;  // 00.100.101
			VBIAS_reg = 0x10;  // 00010000
			break;
		case 3:
			if(burnout)
				MUX_reg = 0xB7;  // 10.110.111
			else
				MUX_reg = 0x37;  // 00.110.111
			VBIAS_reg = 0x40;  // 01000000
			break;
	}
	if(ic_no == 0)
	{
		cs = 0x1;
		T_CS1_L;
	}
	if(ic_no == 1)
	{
		cs = 0x2;
		T_CS2_L;
	}
	if(ic_no == 2)
	{
		cs = 0x4;
		T_CS3_L;
	}
	
	if(vbias)
	{
		Vbias_Set(cs, VBIAS_reg);
	}
	else
	{
		Vbias_Set(cs, 0x00);
	}

	if(ic_no == 0)
		T_CS1_L;
	if(ic_no == 1)
		T_CS2_L;
	if(ic_no == 2)
		T_CS3_L;

	Sel_Channel(cs, MUX_reg);
}

//unsigned short  // 2018/12/10 by kf
short
Right_temperature_IC(void)
{
	short r_value;
	unsigned short r_fraction;
	unsigned short time_cnt = 0;

	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
	{
		time_cnt++;
		if(time_cnt > 1600)
		{
			i2c2_fail_step = 1;
			I2C2_BusRecovery();  /* Try to unstick the bus for next call */
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	//
	// Send cmd to indicate register
	//
	I2C_GenerateSTART(I2C2, ENABLE);
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
	{
		time_cnt++;
		if(time_cnt > 50)
		{
			i2c2_fail_step = 2;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	
	I2C_Send7bitAddress(I2C2, (R_TMP112_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
	
	time_cnt = 0;
	while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))
	{
		time_cnt++;
		if(time_cnt > 900)
		{
			i2c2_fail_step = 3;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
	{
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			time_cnt++;
			if(time_cnt > 20)
			{
				i2c2_fail_step = 4;
				I2C_GenerateSTOP(I2C2, ENABLE);
				DELAY_CYCLES(5000);
				if(I2C2->SR2 & 0x0002) I2C2_BusRecovery();
				error_temperature = error_temperature | 0x0F;
				return r_temperature;
			}
			I2C_GenerateSTART(I2C2, ENABLE);
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
			{
				time_cnt++;
				if(time_cnt > 20)
				{
					i2c2_fail_step = 5;
					error_temperature = error_temperature | 0x0F;
					return r_temperature;
				}
			}

			I2C_Send7bitAddress(I2C2, (R_TMP112_ADDRESS << 1), I2C_Direction_Transmitter);
	}

	time_cnt = 0;
	while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))
	{
		time_cnt++;
		if(time_cnt > 20)
		{
			i2c2_fail_step = 6;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}

	I2C_SendData(I2C2, 0x0);
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
	{
		time_cnt++;
		if(time_cnt > 900)
		{
			i2c2_fail_step = 7;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	I2C_GenerateSTOP(I2C2, ENABLE);

	//
	// Send cmd to receive temperature data
	//
	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
	{
		time_cnt++;
		if(time_cnt > 900)
		{
			i2c2_fail_step = 8;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	I2C_GenerateSTART(I2C2, ENABLE);
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
	{
		time_cnt++;
		if(time_cnt > 100)
		{
			i2c2_fail_step = 9;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}

	I2C_Send7bitAddress(I2C2, (R_TMP112_ADDRESS << 1), I2C_Direction_Receiver);

	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET)
	{
		time_cnt++;
		if(time_cnt > 6000)
		{
			i2c2_fail_step = 10;
			error_temperature = error_temperature | 0xF;
			return r_temperature;
		}
	}

	I2C_AcknowledgeConfig(I2C2, DISABLE);
	I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);

	(void)I2C2->SR2;

	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)
	{
		time_cnt++;
		if(time_cnt > 6000)
		{
			i2c2_fail_step = 11;
			error_temperature = error_temperature | 0xF;
			return r_temperature;
		}
	}
	
	I2C_GenerateSTOP(I2C2, ENABLE);  // 2017/11/06 by kf
	
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))
	{
		time_cnt++;
		if(time_cnt > 1600)
		{
			i2c2_fail_step = 12;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}

	r_value = I2C_ReceiveData(I2C2);

	I2C_AcknowledgeConfig(I2C2, DISABLE);
	I2C_GenerateSTOP(I2C2, ENABLE);

	time_cnt = 0;
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))
	{
		time_cnt++;
		if(time_cnt > 1600)
		{
			i2c2_fail_step = 13;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}

	r_fraction = I2C_ReceiveData(I2C2) >> 4;

	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF))
	{
		time_cnt++;
		if(time_cnt > 20)
		{
			i2c2_fail_step = 14;
			error_temperature = error_temperature | 0x0F;
			return r_temperature;
		}
	}
	I2C_AcknowledgeConfig(I2C2, ENABLE);
	I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
	i2c2_fail_step = 0;  /* success */

	if(r_value & 0x0080)  // 2018/12/10 by kf
	{
		/* 2's complement recover to positive representation */
		r_fraction = (r_value << 4) | r_fraction;  // 2018/12/10 by kf
		r_fraction = (r_fraction ^ 0x0FFF) + 1;  // 2018/12/10 by kf
		
		// Extract integer portion and shift left 1 decimal digit, then recover negative flag(bit-7)
		r_value = ((short)((unsigned char)(r_fraction >> 4)) * 10) | 0x8000;  // 2018/12/10 by kf
		// Extract decimal portion
		r_fraction = r_fraction & 0xF;  // 2018/12/10 by kf
	}
	else  // 2018/12/10 by kf
		r_value = r_value * 10;  // 2018/12/10 by kf
	
	r_fraction = (r_fraction >> 3) * 5000 + ((r_fraction >>2) & 0x1) * 2500 + \
				((r_fraction >> 1) & 0x1) * 1250 + (r_fraction & 0x1) * 625;
	r_fraction = r_fraction / 1000;
//	if(r_value < 0)  // 2018/12/10 by kf
//		return (r_value - r_fraction);  // 2018/12/10 by kf
  // Check if original temperature value is negative
	// Fu 108/12/25 : �[�J�`�ŭt�ūת��A
	if(r_value & 0x8000)  // 2018/12/10 by kf
	{
    // Clear pseudo negative flag(bit-7)
		r_value = r_value & 0x0FFF;  // 2018/12/10 by kf
		R_Normal_Temp_Dir_Fg = 1;	// �t�ū�
		
		return (r_value + r_fraction) * -1;  // 2018/12/10 by kf
	}
	else
	{
		R_Normal_Temp_Dir_Fg = 0;	// ���ū�
		return (r_value + r_fraction);
	}
}

//unsigned short  // 2018/12/10 by kf
short
Left_temperature_IC(void)
{
	short l_value;
	unsigned short l_fraction;
	unsigned short time_cnt = 0;
	
	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 1600)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	//
	// Send cmd to indicate register
	//
	I2C_GenerateSTART(I2C1, ENABLE);  // SCL=L, SDA=L
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
	{
		time_cnt++;
		//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 50)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	
	I2C_Send7bitAddress(I2C1, (L_TMP112_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'  // SCL=CLK, SDA=ADDR.
	// SCL=L, SDA=H
	time_cnt = 0;
	while(!(I2C_GetFlagStatus(I2C1, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF)))
	{
		time_cnt++;
		//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 900)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
	{
			I2C_ClearFlag(I2C1, I2C_FLAG_AF);
			time_cnt++;
//			if(time_cnt > NACK_CHECK_MAX)  // 2017/09/25 by kf
			if(time_cnt > 20)  // 2017/09/25 by kf
			{
				I2C_GenerateSTOP(I2C1, ENABLE);
				error_temperature = error_temperature | 0xF0;
				return l_temperature;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_GenerateSTART(I2C1, ENABLE);  // SCL=L, SDA=L
			while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
			{
				time_cnt++;
//				if(time_cnt > 63000)  // 2017/09/25 by kf
				if(time_cnt > 20)  // 2017/09/25 by kf
				{
					error_temperature = error_temperature | 0xF0;
					return l_temperature;
				}
			}

			I2C_Send7bitAddress(I2C1, (L_TMP112_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
	}
	
	time_cnt = 0;
	while(!I2C_GetFlagStatus(I2C1, I2C_FLAG_TRA))  // Read TRA in SR2
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 20)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}

	I2C_SendData(I2C1, 0x0);
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING))
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 900)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	I2C_GenerateSTOP(I2C1, ENABLE);

	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 900)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	//
	// Send cmd to receive temperature data
	//
	I2C_GenerateSTART(I2C1, ENABLE);
	time_cnt = 0;
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 100)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	
	I2C_Send7bitAddress(I2C1, (L_TMP112_ADDRESS << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '0'
	
	time_cnt = 0;  // 2017/11/06 by kf
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_ADDR) == RESET)  // 2017/11/06 by kf
	{
		time_cnt++;  // 2017/11/06 by kf
		if(time_cnt > 6000)  // 2017/11/06 by kf
		{
			error_temperature = error_temperature | 0xF0;  // 2017/11/06 by kf
			return l_temperature;  // 2017/11/06 by kf
		}
	}

	I2C_AcknowledgeConfig(I2C1, DISABLE);  // 2017/11/06 by kf
	I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Next);  // 2017/11/06 by kf

	(void)I2C1->SR2;  // 2017/11/06 by kf

	time_cnt = 0;  // 2017/11/06 by kf
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BTF) == RESET)  // 2017/11/06 by kf
	{
		time_cnt++;  // 2017/11/06 by kf
		if(time_cnt > 6000)  // 2017/11/06 by kf
		{
			error_temperature = error_temperature | 0xF0;  // 2017/11/06 by kf
			return l_temperature;  // 2017/11/06 by kf
		}
	}
	
	I2C_GenerateSTOP(I2C1, ENABLE);  // 2017/11/06 by kf

	time_cnt = 0;
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 1600)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}

	l_value = I2C_ReceiveData(I2C1);  // Data1 of transfer sequence diagram  // SCL=CLK, SDA=DATA
	/* 2018/12/10 by kf
	if(l_value & 0x0080)
		l_value = l_value * (-10);
	else
		l_value = l_value * 10;
	*/
	// SCL=L, SDA=H
	I2C_AcknowledgeConfig(I2C1, DISABLE);  // Ack = 0 in EV7_1 of transfer sequence diagram
	I2C_GenerateSTOP(I2C1, ENABLE);  // STOP request in EV7_1 of transfer sequence diagram

	time_cnt = 0;
	while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 1600)
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	
	l_fraction = I2C_ReceiveData(I2C1) >> 4;  // Data2~DataN-1 of transfer sequence diagram
	
	time_cnt = 0;
	while(I2C_GetFlagStatus(I2C1, I2C_FLAG_STOPF))
	{
		time_cnt++;
//		if(time_cnt > 63000)  // 2017/09/25 by kf
		if(time_cnt > 20)  // 2017/09/25 by kf
		{
			error_temperature = error_temperature | 0xF0;
			return l_temperature;
		}
	}
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C_NACKPositionConfig(I2C1, I2C_NACKPosition_Current);  // 2017/11/06 by kf
	
	if(l_value & 0x0080)  // 2018/12/10 by kf
	{
		/* 2's complement recover to positive representation */
		l_fraction = (l_value << 4) | l_fraction;  // 2018/12/10 by kf
		l_fraction = (l_fraction ^ 0x0FFF) + 1;  // 2018/12/10 by kf
		
		// Extract integer portion and shift left 1 decimal digit, then recover negative flag(bit-7)
		l_value = ((short)((unsigned char)(l_fraction >> 4)) * 10) | 0x8000;  // 2018/12/10 by kf
		// Extract decimal portion
		l_fraction = l_fraction & 0xF;  // 2018/12/10 by kf
	}
	else  // 2018/12/10 by kf
		l_value = l_value * 10;  // 2018/12/10 by kf
	
	l_fraction = (l_fraction >> 3) * 5000 + ((l_fraction >>2) & 0x1) * 2500 + \
				((l_fraction >> 1) & 0x1) * 1250 + (l_fraction & 0x1) * 625;
	l_fraction = l_fraction / 1000;
	
//	if(l_value < 0)  // 2018/12/10 by kf
//		return (l_value - l_fraction);  // 2018/12/10 by kf
	// Check if original temperature value is negative
	// Fu 108/12/25 : �[�J�`�ŭt�ūת��A
	if(l_value & 0x8000)  // 2018/12/10 by kf
	{
    // Clear pseudo negative flag(bit-7)
		l_value = l_value & 0x0FFF;  // 2018/12/10 by kf
		
		L_Normal_Temp_Dir_Fg = 1;	// �t�ū�
		return (l_value + l_fraction) * -1;  // 2018/12/10 by kf
	}
	else
	{
		L_Normal_Temp_Dir_Fg = 0;	// ���ū�
		return (l_value + l_fraction);
	}
}
//////////////////
//
//
void TempHWSave(void)
{
	unsigned int wait;
	static short wait_cnt = 0;
	
	switch(StepCnt)
	{
		case 0:  // Select converted-channel and enable start signal
			Sel_Channel(0x07, reg[ch_index][2]);
			//
			StepCnt = 1;		// next case
			TimerBase1 = 0;		// clear timer
			TpDriveOn(IcCh);
			wait_cnt = 0;
			adc_sampling_active = 1;  /* B-1: freeze heater output during ADC conversion */
			DELAY_CYCLES(2);
			T_START_H;
			DELAY_CYCLES(25);  // 750ns (match Keil)
			break;
		//
		case 1:  // If complete conversion, disable start signal then obtain and translate A/D to temperature and burnout the channel double next this.
			StepCnt = 1;
			if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14) == 0x0)
			{
				if((GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0x0) && (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11) == 0x0))
			{
					T_START_L;
					Read_Single_AD(0x7, ch_index);
					adc_sampling_active = 0;  /* B-1: ADC read done, unfreeze heater */
						TempPosAndNegFg = 0;	// Fu 105/12/02
					calc = AD2Temp(0x0, ch_index);
					//Nthermal_couple[0][ch_index] = ((short)(calc+l_r_temperature) > 12000) ? 12000 : (short)(calc + l_r_temperature);  // 2017/06/09 by kf
					//	Fu 105/12/02
					if(TempPosAndNegFg != 0)	// �ƷŬ��t�ū� ?
					{
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							Nthermal_couple[0][ch_index] = l_r_temperature + calc;
							thermal_PosAndNeg[0][ch_index] = 1;// �t�ū�
						}
						else
						{
							if(l_r_temperature >= calc)	// �`�Ŭ����ū�
							{
								Nthermal_couple[0][ch_index] = l_r_temperature - calc;
								thermal_PosAndNeg[0][ch_index] = 0; // Fu 107/12/10
							}
							else
							{
								//Nthermal_couple[0][ch_index] = 0;
								Nthermal_couple[0][ch_index] = calc - l_r_temperature;
								thermal_PosAndNeg[0][ch_index] = 1; // Fu 107/12/10
							}
						}
					}
					else
					{
						// Fu 108/12/25 : �[�J�`�ŭt�ūת��A
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							if(l_r_temperature >= calc)	// �`�Ŭ��t�ū�
							{
								Nthermal_couple[0][ch_index] = l_r_temperature - calc;
								//
								if(Nthermal_couple[0][ch_index] >= 12000)
								{
									Nthermal_couple[0][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[0][ch_index] = 1; // Fu 107/12/10
							}
							else
							{
								Nthermal_couple[0][ch_index] = calc - l_r_temperature;
								//
								if(Nthermal_couple[0][ch_index] >= 12000)
								{
									Nthermal_couple[0][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[0][ch_index] = 0; // Fu 107/12/10
							}
						}
						else
						{
							Nthermal_couple[0][ch_index] = ((calc+l_r_temperature) > 12000) ? 12000 : (calc + l_r_temperature);  // 2012/05/08
							thermal_PosAndNeg[0][ch_index] = 0; // Fu 107/12/10
						}
					}					
					Nthermal_hex[0][ch_index] = thermal[0];
					
					TempPosAndNegFg = 0;	// Fu 105/12/02
					calc = AD2Temp(0x1, ch_index);
					//Nthermal_couple[1][ch_index] = ((short)(calc+l_r_temperature) > 12000) ? 12000 : (short)(calc + l_r_temperature);  // 2017/06/09 by kf
					//	Fu 105/12/02
					if(TempPosAndNegFg != 0)
					{
						/*if(l_r_temperature >= calc)
						{
							Nthermal_couple[1][ch_index] = l_r_temperature - calc;
							thermal_PosAndNeg[1][ch_index] = 0; // Fu 107/12/10
						}
						else
						{
							Nthermal_couple[1][ch_index] = 0;
							thermal_PosAndNeg[1][ch_index] = 1; // Fu 107/12/10
						}*/
						//
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							Nthermal_couple[1][ch_index] = l_r_temperature + calc;
							thermal_PosAndNeg[1][ch_index] = 1;// �t�ū�
						}
						else
						{
							if(l_r_temperature >= calc)	// �`�Ŭ����ū�
							{
								Nthermal_couple[1][ch_index] = l_r_temperature - calc;
								thermal_PosAndNeg[1][ch_index] = 0; // Fu 107/12/10
							}
							else
							{
								//Nthermal_couple[1][ch_index] = 0;
								Nthermal_couple[1][ch_index] = calc - l_r_temperature;
								thermal_PosAndNeg[1][ch_index] = 1; // Fu 107/12/10
							}
						}
					}
					else
					{
						/*
						Nthermal_couple[1][ch_index] = ((calc+l_r_temperature) > 12000) ? 12000 : (calc + l_r_temperature);  // 2012/05/08
						thermal_PosAndNeg[1][ch_index] = 0; // Fu 107/12/10
						*/
						// Fu 108/12/25 : �[�J�`�ŭt�ūת��A
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							if(l_r_temperature >= calc)	// �`�Ŭ��t�ū�
							{
								Nthermal_couple[1][ch_index] = l_r_temperature - calc;
								//
								if(Nthermal_couple[1][ch_index] >= 12000)
								{
									Nthermal_couple[1][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[1][ch_index] = 1; // Fu 107/12/10
							}
							else
							{
								Nthermal_couple[1][ch_index] = calc - l_r_temperature;
								//
								if(Nthermal_couple[1][ch_index] >= 12000)
								{
									Nthermal_couple[1][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[1][ch_index] = 0; // Fu 107/12/10
							}
						}
						else
						{
							Nthermal_couple[1][ch_index] = ((calc+l_r_temperature) > 12000) ? 12000 : (calc + l_r_temperature);  // 2012/05/08
							thermal_PosAndNeg[1][ch_index] = 0; // Fu 107/12/10
						}						
					}
					Nthermal_hex[1][ch_index] = thermal[1];
					
					TempPosAndNegFg = 0;	// Fu 105/12/02
					calc = AD2Temp(0x2, ch_index);
					//Nthermal_couple[2][ch_index] = ((short)(calc+l_r_temperature) > 12000) ? 12000 : (short)(calc + l_r_temperature);  // 2017/06/09 by kf
					//	Fu 105/12/02
					if(TempPosAndNegFg != 0)
					{
						/*if(l_r_temperature >= calc)
						{
							Nthermal_couple[2][ch_index] = l_r_temperature - calc;
							thermal_PosAndNeg[2][ch_index] = 0; // Fu 107/12/10
						}
						else
						{
							Nthermal_couple[2][ch_index] = 0;
							thermal_PosAndNeg[2][ch_index] = 1; // Fu 107/12/10
						}*/
						//
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							Nthermal_couple[2][ch_index] = l_r_temperature + calc;
							thermal_PosAndNeg[2][ch_index] = 1;// �t�ū�
						}
						else
						{
							if(l_r_temperature >= calc)	// �`�Ŭ����ū�
							{
								Nthermal_couple[2][ch_index] = l_r_temperature - calc;
								thermal_PosAndNeg[2][ch_index] = 0; // Fu 107/12/10
							}
							else
							{
								//Nthermal_couple[2][ch_index] = 0;
								Nthermal_couple[2][ch_index] = calc - l_r_temperature;
								thermal_PosAndNeg[2][ch_index] = 1; // Fu 107/12/10
							}
						}						
					}
					else
					{
						/*
						Nthermal_couple[2][ch_index] = ((calc+l_r_temperature) > 12000) ? 12000 : (calc + l_r_temperature);  // 2012/05/08
						thermal_PosAndNeg[2][ch_index] = 0; // Fu 107/12/10
						*/
						// Fu 108/12/25 : �[�J�`�ŭt�ūת��A
						if(R_L_Normal_Temp_Dir_Fg != 0)	// �`�Ŭ��t�ū�
						{
							if(l_r_temperature >= calc)	// �`�Ŭ��t�ū�
							{
								Nthermal_couple[2][ch_index] = l_r_temperature - calc;
								//
								if(Nthermal_couple[2][ch_index] >= 12000)
								{
									Nthermal_couple[2][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[2][ch_index] = 1; // Fu 107/12/10
							}
							else
							{
								Nthermal_couple[2][ch_index] = calc - l_r_temperature;
								//
								if(Nthermal_couple[2][ch_index] >= 12000)
								{
									Nthermal_couple[2][ch_index] = 12000;
								}
								//
								thermal_PosAndNeg[2][ch_index] = 0; // Fu 107/12/10
							}
						}
						else
						{
							Nthermal_couple[2][ch_index] = ((calc+l_r_temperature) > 12000) ? 12000 : (calc + l_r_temperature);  // 2012/05/08
							thermal_PosAndNeg[2][ch_index] = 0; // Fu 107/12/10
						}						
					}
					Nthermal_hex[2][ch_index] = thermal[2];
					
					ch_burnout = (ch_index + 2) % 4;
					conv_round = (conv_round + 1) % 12;
					
					Sel_Channel(0x07, Nreg[ch_burnout][2]);  // Select a channel with burnout command exploit Nreg
					TimerBase1 = 0;  // clear int timer
					StepCnt = 2;

					ch_index = (ch_index + 1) % 4;
					if(ch_index == 0)
						IcCh = (IcCh + 1) % 3;

				}
				else
					wait_cnt++;
			}
			else
				wait_cnt++;

			if(wait_cnt > 10)
			{
				adc_sampling_active = 0;  /* B-1: timeout, unfreeze */
				TimerBase1 = 0;
				StepCnt = 2;

				Nthermal_couple[0][ch_index] = 12000;
				Nthermal_couple[1][ch_index] = 12000;
				Nthermal_couple[2][ch_index] = 12000;
				
				ch_burnout = (ch_index + 2) % 4;
				conv_round = (conv_round + 1) % 12;
					
				Sel_Channel(0x07, Nreg[ch_burnout][2]);  // Select a channel with burnout command exploit Nreg
				
				ch_index = (ch_index + 1) % 4;
				if(ch_index == 0)
					IcCh = (IcCh + 1) % 3;
			}
			break;
		case 2:	 // Turn off Burnout current
			Sel_Channel(0x07, reg[ch_index][2]);
			if(ch_index == 0)
			{
				StepCnt = 3;
				thermal_couple[TemperatureSensor[0]] = Nthermal_couple[0][0]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[1]] = Nthermal_couple[0][1]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[2]] = Nthermal_couple[1][0]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[3]] = Nthermal_couple[1][1]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[4]] = Nthermal_couple[2][0]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[5]] = Nthermal_couple[2][1]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[6]] = Nthermal_couple[0][2]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[7]] = Nthermal_couple[0][3]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[8]] = Nthermal_couple[1][2];	 	// 2014/10/13
				thermal_couple[TemperatureSensor[9]] = Nthermal_couple[1][3];		// 2014/10/13
				thermal_couple[TemperatureSensor[10]] = Nthermal_couple[2][2]; 	// 2014/10/13
				thermal_couple[TemperatureSensor[11]] = Nthermal_couple[2][3]; 	// 2014/10/13				
			}
			else
				StepCnt = 0;
			break;
		case 3:
			error_temperature = 0;
			//
#ifdef I2C1_MUX
			if(I2C1_MUX_lock())
			{
#endif
				l_r_temperature = Right_temperature_IC();
				if(r_temperature == 0 || ((unsigned short)(l_r_temperature - r_temperature) < 30) || ((unsigned short)(r_temperature - l_r_temperature) < 30))
					r_temperature = l_r_temperature;
#ifdef I2C1_MUX
				I2C1_MUX_unlock();
			}
#endif

			#ifdef I2C2_MUX			
			if(I2C2_MUX_lock())
			{
#endif
				l_r_temperature = Left_temperature_IC();
				if(l_temperature == 0 || ((unsigned short)(l_r_temperature - l_temperature) < 30) || ((unsigned short)(l_temperature - l_r_temperature) < 30))
					l_temperature = l_r_temperature;
#ifdef I2C2_MUX
				I2C2_MUX_unlock();
			}
#endif
			// Fu 108/12/25 : �[�J�`�ŭt�ū���ܪ��A
			if(R_Normal_Temp_Dir_Fg != 0)	// �k�`�Ŭ��t�ū�
			{
				if(L_Normal_Temp_Dir_Fg != 0)
				{
					l_r_temperature = ((l_temperature + r_temperature) / 2);
					R_L_Normal_Temp_Dir_Fg = 1;	// �`�Ŭ��t�ū�
				}
				else
				{
					if(l_temperature >= r_temperature)	// ���`�Ŭ����ū� , �k���t�ū�
					{
						l_r_temperature = ((l_temperature - r_temperature) / 2);
						R_L_Normal_Temp_Dir_Fg = 0;	// �`�Ŭ����ū�
					}
					else
					{
						l_r_temperature = ((r_temperature - l_temperature) / 2);
						if(l_r_temperature == 0)
						{
							R_L_Normal_Temp_Dir_Fg = 0;	// �`�Ŭ��t�ū�
						}
						else
						{
							R_L_Normal_Temp_Dir_Fg = 1;	// �`�Ŭ��t�ū�
						}
					}
				}
			}
			else if(L_Normal_Temp_Dir_Fg != 0)	// ���`�Ŭ��t�ū� , �k�`�Ŭ����ū�
			{
					if(r_temperature >= l_temperature)	
					{
						l_r_temperature = ((r_temperature - l_temperature) / 2);
						R_L_Normal_Temp_Dir_Fg = 0;	// �`�Ŭ����ū�
					}
					else
					{
						l_r_temperature = ((l_temperature - r_temperature) / 2);
						if(l_r_temperature == 0)
						{
							R_L_Normal_Temp_Dir_Fg = 0;	// �`�Ŭ��t�ū�
						}
						else
						{
							R_L_Normal_Temp_Dir_Fg = 1;	// �`�Ŭ��t�ū�
						}
					}
			}
			else
			{
				l_r_temperature = (l_temperature + r_temperature) / 2;
				R_L_Normal_Temp_Dir_Fg = 0;	// �`�Ŭ����ū�
			}
			//
			StepCnt = 0;		// reboot
			TimerBase1 = 0;
			//
			if(Int1Sts == 0x08)
				Int1Sts = 0;
			else
				Int1Sts = 0x08;

			Bk_error_temperature = error_temperature;
			//
			*tempData[6].TpVersion = l_temperature;
			*tempData[7].TpVersion = r_temperature;
 			*tempData[8].TpVersion = BURN_DETECT | ADErrorFlag;	// Fu 101/05/16
			// *tempData[10].TpVersion = (bootloader_ver >> 16) & 0xFFFF;
			*tempData[11].TpVersion = bootloader_ver & 0xFFFF;
			//*tempData[9].TpVersion = (unsigned short)(Bk_error_temperature & 0xffff);
			//*tempData[10].TpVersion = (unsigned short)((Bk_error_temperature>>16) & 0xffff);
			*tempData[10].TpVersion = (unsigned short)(Bk_error_temperature & 0xffff);

			break;	
	}
}
////////////////////
//
//
unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen)
{
	unsigned char uchCRCHi = 0xFF; /* high byte of CRC initialized */
	unsigned char uchCRCLo = 0xFF; /* low byte of CRC initialized */
	unsigned uIndex; /* will index into CRC lookup table */
	while (usDataLen--) /* pass through message buffer */
	{
		uIndex = uchCRCHi ^ *puchMsg++ ; /* calculate the CRC */
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
		uchCRCLo = auchCRCLo[uIndex] ;
	}
	return (uchCRCHi | uchCRCLo << 8) ;
}
////////////////////////////////////////////////
//
//
void TempIintData(void)
{
	unsigned short i;
	//
	for(i=0; i<12; i++)
	{
		tempData[i].HeatSet = GetPtrCDM2(20001+i);				// 1
		tempData[i].PerHeatSet = GetPtrCDM2(20016+i);			// 2
		tempData[i].PIDp = GetPtrCDM2(20201+i);						// 3
		tempData[i].PIDi = GetPtrCDM2(20216+i);						// 4
		tempData[i].PIDd = GetPtrCDM2(20231+i);						// 5
		tempData[i].HiAlarm = GetPtrCDM2(20031+i);				// 6
		tempData[i].LoAlarm = GetPtrCDM2(20046+i);				// 7
		tempData[i].TpControl = GetPtrCDM2(20501+i);			// 8
		tempData[i].TpDisplay = GetPtrCDM2(20516+i);			// 9
		tempData[i].PwmOffSet = GetPtrCDM2(20246+i);			// 10
		tempData[i].ThermostatFun = GetPtrCDM2(20261);		// 11
		tempData[i].Thermostat = GetPtrCDM2(20276+i);			// 12
		tempData[i].Proportion = GetPtrCDM2(20291+i);			// 13
		tempData[i].TpConAddHi = GetPtrCDM2(20531+i);			// 14
		tempData[i].SynchronFun = GetPtrCDM2(20409);			// 15
		tempData[i].Synchron = GetPtrCDM2(20306+i);				// 16
		tempData[i].TwoUpOffset = GetPtrCDM2(20321+i);		// 17
		tempData[i].ContrlFun = GetPtrCDM2(20401);				// 18
		tempData[i].CycleTm = GetPtrCDM2(20346+i);				// 19
		tempData[i].AuTnFun = GetPtrCDM2(20402);	    		// 20
		tempData[i].HeatCtrlMd = GetPtrCDM2(20101);	  		// 21
		tempData[i].HeatHour = GetPtrCDM2(20551);	    		// 24
		tempData[i].HeatMin = GetPtrCDM2(20552);					// 25
		tempData[i].CoolSet = GetPtrCDM2(20061+i);	    	// 26
		tempData[i].OilHeat = GetPtrCDM2(20405);	    		// 27
		tempData[i].HeatWeek = GetPtrCDM2(20553);					// 28
		tempData[i].HiOilSet = GetPtrCDM2(20406);	    		// 42
		tempData[i].KAndJSel = GetPtrCDM2(20407);	    		// 44
		tempData[i].HeatFlashBit = GetPtrCDM2(20554);   	// 45
		tempData[i].TwoUpFun = GetPtrCDM2(20408);					// 47
		tempData[i].HeatErrSts = GetPtrCDM2(20555);	    	// 49
		tempData[i].HiAlmSts = GetPtrCDM2(20556);					// 50
		tempData[i].LowAlmSts = GetPtrCDM2(20557);				// 51
		tempData[i].LowAlmSts = GetPtrCDM2(20557);				// 51
		tempData[i].PwmOutput = GetPtrCDM2(20601+i);			// 52
		tempData[i].PwmOutputCyc = GetPtrCDM2(20640+i);	
		tempData[i].TempLinearErr = GetPtrCDM2(20615);		// 53
		tempData[i].AutoTingSts = GetPtrCDM2(20850);			// 54
		tempData[i].TpVersion = GetPtrCDM2(20800+i);			// 55
		tempData[i].TwoHeatMd = GetPtrCDM2(20411);				// 56
		tempData[i].TwoHeatPercentMd = GetPtrCDM2(20412);	
		tempData[i].TwoHeatTm = GetPtrCDM2(20106+i);			// 57
		tempData[i].TwoHeatTp = GetPtrCDM2(20076+i);			// 58
		tempData[i].AllTmAuTpSw = GetPtrCDM2(20410);			// 59
		tempData[i].SlowTpUpSw = GetPtrCDM2(20262);				// 60
		tempData[i].TpSensorSour = GetPtrCDM2(20121+i);		// 103/10/16 61
		tempData[i].TpSensorDest = GetPtrCDM2(20136+i);		// 103/10/16 62
		tempData[i].TpPointSour = GetPtrCDM2(20151+i);		// 103/10/16 63
		tempData[i].TpPointDest = GetPtrCDM2(20166+i);		// 103/10/16 64
		tempData[i].TpChgError = GetPtrCDM2(20562);				// 103/10/16 65
		tempData[i].VariableBaud = GetPtrCDM2(20043);
		tempData[i].AutoHeatMainSwitch = GetPtrCDM2(20573);
		tempData[i].AutoHeatBranchSwitch = GetPtrCDM2(20403);
		tempData[i].AutoPerHeatBranchSwitch = GetPtrCDM2(20404);
		tempData[i].HeatWattUnit = GetPtrCDM2(20431+i);	// Fu 108/12/30 : �[�J�ƥ��C�q�ūת��[���˯S��
		tempData[i].Week = GetPtrCDM2(20566);
		tempData[i].Sec = GetPtrCDM2(20567);
		tempData[i].Min = GetPtrCDM2(20568);
		tempData[i].Hour = GetPtrCDM2(20569);
		tempData[i].Day = GetPtrCDM2(20570);
		tempData[i].Month = GetPtrCDM2(20571);
		tempData[i].Year = GetPtrCDM2(20572);
		//
		if(i < 7)
		{
			tempData[i].HeatTm = GetPtrCDM2(20391+i);
			tempData[i].HeatPerTm = GetPtrCDM2(20491+i);
		}
		//
	  tempData[i].HeatOKSts = 6548;											// 29
	  tempData[i].HeatHiSts = 8009;											// 30
	  tempData[i].HeatLowSts = 8008;										// 31
	  tempData[i].PlcHeatOn = 6550;											// 32
	  tempData[i].SysHeatOn = 6545;											// 33
	  tempData[i].PlcHeatOff = 6549;										// 34
	  tempData[i].SysHeatOff = 6546;										// 35
	  tempData[i].HoldSts = 8056;												// 36
	  tempData[i].HeatSW = 6547;												// 37
	  tempData[i].HeatPerSW = 6551;											// 38
	  tempData[i].TpCoolIR = 6552+i;										// 39
	  tempData[i].OilHiSts = 8020;											// 40
	  tempData[i].OilHeatIR = 6576;											// 41
	  tempData[i].TempCurIR = 8030;											// 43
	  tempData[i].HeatErrIR = 7293; 										// 46
	  tempData[i].AllTpRdyIR = 7277;   									// 48
	}
	//  
//	read_FRAM(0x200, pA20001, 500);  // 2018/03/22 by kf
	/*
	 * Disable timer ISRs during EEPROM read to prevent I2C2 bus contention.
	 * TIM4 ISR calls TempHWSave() → Right_temperature_IC() → uses I2C2.
	 * TIM5 ISR calls RS485 parsing which may also trigger I2C operations.
	 */
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	TIM_ITConfig(TIM5, TIM_IT_Update, DISABLE);
	if(I2C2->SR2 & 0x0002) I2C2_BusRecovery();
	eeprom_access_ret_val = read_FRAM(0x200, pA20001, 500);
	if(eeprom_access_ret_val != 0) {
		I2C2_BusRecovery();
		eeprom_access_ret_val = read_FRAM(0x200, pA20001, 500);
	}
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	dbg_puts("[EE-IN] ret="); dbg_putu(eeprom_access_ret_val);
	dbg_puts(" CDM200="); dbg_putu(pA20001[200]);  /* PIDp offset */
	dbg_puts(" CDM0="); dbg_putu(pA20001[0]);       /* HeatSet */
	dbg_puts(" CDM100="); dbg_putu(pA20001[100]);   /* HeatCtrlMd */
	dbg_puts("\r\n");
	//
	for(i=0; i<1000; i++)
		*GetPtrCDM3(30001+i) = *GetPtrCDM2(20001+i);
	//
	RealTpOutPwm = 0;
	*tempData[0].HeatCtrlMd &= 0xfff0;
	*tempData[0].AutoTingSts = 0;
	*tempData[0].AuTnFun = 0;
	//
    *tempData[0].TpVersion = ('M') + ('X'<<8);
    *tempData[1].TpVersion = ('1') + ('-'<<8);						    
    *tempData[2].TpVersion = ('M') + ('4'<<8);
    *tempData[3].TpVersion = ('I') + ('1'<<8);
    *tempData[4].TpVersion = ('2') + ('0'<<8);
    *tempData[5].TpVersion = ('4') + ('0'<<8);
	//
	TpTypeMdSet();
}
//
//
//
void TempDisplay(void)
{
	unsigned short sts;
	unsigned short BK_sts;
	unsigned short heatmode;
    //
    *(tempData[0].HiAlmSts) = HiAlmSts;  // 高溫偏差狀態BIT -
    *(tempData[0].LowAlmSts) = LowAlmSts; // 低溫偏差狀態BIT
    //
    sts = 0;
    Heat_Central = 5050;
    if((LowAlmSts & 0x07FF7FF) && ((*(tempData[0].HeatCtrlMd) & 0x0007)==1))
        sts |= 0x01;                        // (不加熱)下SET料溫過low狀態
    
	if(HiAlmSts & 0x07FF7FF)
        sts |= 0x02;                        // (不加熱)下SET料溫過high狀態 sts = state
    //
    if((*(tempData[0].HeatCtrlMd) & 0x000f)==0)				// 6547電熱控制(控制電磁接觸器)(Heater-Control)
    {
        TpClLpFg4 = 0;
        InitTpHt = 0;
        TwoTpFg = 0;
        TwoTpHeatFg = 0;
				sts = sts & 0x02;                   // 不加熱下清除料溫過低狀態
	}
    //
    heatmode =  *(tempData[0].HeatCtrlMd) & 0x0007;//  MMI 45819 / heating & holding & autoholding
    //
    BK_sts = sts;
    //
    if(TpOkFg == 0)
        sts = sts & 0x02;				// 未到達料溫時清除料溫過低狀態
    //
    PID_CAN_CTRL_FG++;                  // Initial CMP Hear rday delay timev : 10 * 100 = 1 sec
    if(PID_CAN_CTRL_FG>100)
    {
        PID_CAN_CTRL_FG = 200;
        //
        if(BK_sts)
            TPopen++;
        else
            TPopen=0;
        //
        if (TPopen>=20)
        {
			if (sts & 0x02)             // sts 2高  1低
             
			   	TempHiFg = ON;          // 8009 ON	/ Temp. too High
            else
                TempHiFg = 0;       	// 8009 OFF / Temp. too High
            if (sts & 0x01)             // sts 2高  1低
              
			  	TempLowFg = ON;
            else
                TempLowFg = 0;
            TPopen=20;
        }
        //
        else
        {
            if(heatmode==1)
            {
                if(!(BK_sts))
                {
                    TpOkFg = ON;                        // 料溫正常(Temperature-Normal)
                }
            }
            TempHiFg = 0;
            TempLowFg = 0;
        }
    }
    //
    else
    {
        TempHiFg = 0;
        TempLowFg = 0;
    }
    //
    TempCool();
    //
    Synchronization();
}
//
//
//
void TempCool(void)
{
}
///////////////////////////////////////////////////
//
//
void Synchronization(void)
{    
	unsigned short i, j, LastCnt, BkSynFg, Cnt, BkTpUnit;
  //
  BkSynFg = TpMaxLp;
  Cnt = 0;
	BkTpUnit = 65535;
	//
  for(i=0; i<TpMaxLp; i++)
  {
		//	Fu 104/07/08
		TpSetRang[i] = *(tempData[i].TpControl) / 50;	// �[���Ȥ���50�������ūױ���
		//
		if((*(tempData[i].TpDisplay) >= *(tempData[i].TpControl)) || (!(*(tempData[0].SynchronFun) & (0x01<<i))) || (BkPidPUnit[i] == OFF) || (*(tempData[i].TpControl) == OFF) || (*(tempData[i].TpControl) == 0xffff)) // 2015/07/31 �Y�[���Ȥp��150��, �h�P�B�ɷŤ��ϥ�.
		{
			if(!(*(tempData[0].TwoHeatMd) & (0x01<<i)) || ((*(tempData[0].TwoHeatMd) & (0x01<<i)) && (TwoHeatSetTm[i] == (Tm1Sec1_TRIG_MS * *(tempData[i].TwoHeatTm))))) // 2016/01/08
			{
				TpClLpFg4 |= (0x01<<i);
			}
		}
    else
    {
			if((TpClLpFg3 & (0x01<<i)) && (*(tempData[0].SynchronFun) & (0x01<<i)) && !(TpClLpFg4 & (0x01<<i)))
      {
				Cnt++;
        //
        for(LastCnt=0; LastCnt<TpMaxLp; LastCnt++)
        {
					if((BkTpUnit > (*(tempData[LastCnt].TpControl) - *(tempData[LastCnt].TpDisplay))) && !(TpClLpFg4 & (0x01<<LastCnt)))
					{
						BkSynFg = LastCnt;
						BkTpUnit = *(tempData[LastCnt].TpControl) - *(tempData[LastCnt].TpDisplay);
					}
				}
      }
    }
  }
	//	Fu 104/08/13
	for(i=0; i < TpMaxLp; i++)
	{
		if((InitBellBuffFg & (0x01<<i)) == 0)
		{
			for(j = 0; j < 40; j++)
			{
				TempSetUnit[i][j] = ((*(tempData[i].TpControl) - 300) * (j+1)) / 40; // 0% ~ 90% �ֳt
			}
			//
			for(j = 0; j < 5; j++)
			{
				TempSetUnit[i][j+40] = ((200 * (j+1)) / 5) + TempSetUnit[i][39];	// 91% ~ 95% ���t
			}
			//
			for(j = 0; j < 5; j++)
			{
				TempSetUnit[i][j+45] = ((100 * (j+1)) / 5) + TempSetUnit[i][44];	// 96% ~ 100%�C�t
			}
			//
			InitBellBuffFg |= (0x01<<i);
		}
	}
	//
	if(Cnt>=2)
		SynFg = BkSynFg;
	else
    SynFg = TpMaxLp;
	//
	//	Fu 102/12/27 : �P�B�ɷű���A����
	//
	*GetPtrCDM2(20560) = TpClLpFg4;
}
//
//	100ms
//
void HeatErrMd(void)
{
	unsigned short i;
	//
	if(!(*(tempData[0].AuTnFun)))
	{
		for(i=0; i<TpMaxLp; i++)
		{
			if(TpClLpFg3 & (0x01<<i))
			{
		        if(TimerBase2 <= 3000)		// 250ms * 3000 = 750 Sec
		            OldTempSetSum[i] = OldTempSetSum[i]+((unsigned short)abs(*(tempData[i].TpDisplay) - OldTpcurr2[i]));
		        else
		        {
		            if((OldTempSetSum[i] <= 10) && (*(tempData[0].HeatCtrlMd) & 0x0007) && (*(tempData[i].TpControl) != OFF) && (*(tempData[i].TpControl) != 0xffff) && (*(tempData[i].TpDisplay) <= 5900))		                                              // 加熱異常 >1.00 c
		                *(tempData[0].HeatErrSts) = *(tempData[0].HeatErrSts) | (0x01<<i);      // 43610加�set�0~7 bit)
		            else
		                *(tempData[0].HeatErrSts) = *(tempData[0].HeatErrSts) & ~(0x01<<i);   	// 43610加�clr 0~7 bit)
		        }
			}
			else					   
			{
		     	*(tempData[0].HeatErrSts) = *(tempData[0].HeatErrSts) & ~(0x01<<i);   // 43610加�clr 0~7 bit)
				OldTempSetSum[i] = 200; 
			}
	        //
	        if(*(tempData[0].HeatErrSts))             		 
	            tempData[0].HeatErrIR = 1; 
			else
	            tempData[0].HeatErrIR = 0;             	 
			//
	        OldTpcurr2[i] =  *(tempData[i].TpDisplay);
		}
	}
	//
	if(TimerBase2 >= 3000)
	{
		for(i=0; i<TpMaxLp; i++)
			OldTempSetSum[i] = OFF;
		TimerBase2=0;
	}
}
//
//
//
void TpDriveOn(unsigned short TpCh)
{
	T_CS2_L;
	T_CS13_L;
}
//
//
//
void TpDriveOff(unsigned short TpCh)
{
	T_CS2_H;
	T_CS13_H;
}
//
//
//
void TpDriveOnNot(unsigned short TpCh)
{
	switch(TpCh)
	{
		case 0:
			T_CS1_H;
			T_CS2_L;
			T_CS3_L;
		break;
		case 1:
			T_CS1_L;
			T_CS2_H;
			T_CS3_L;
		break;
		case 2:
			T_CS1_L;
			T_CS2_L;
			T_CS3_H;
		break;
	}
}
//
//
//
void TpDriveOffNot(unsigned short TpCh)
{
	switch(TpCh)
	{
		case 0:
			T_CS1_H;
			T_CS2_H;
			T_CS3_H;
		break;
		case 1:
			T_CS1_H;
			T_CS2_H;
			T_CS3_H;
		break;
		case 2:
			T_CS1_H;
			T_CS2_H;
			T_CS3_H;
		break;
	}
}
///////////////////
//	MMI 53521 : �ū׷P����ըӷ�
//	MMI 53545 : �ū׷P����եت�
//	MMI 53569 : �ū׿�X��ըӷ�
//	MMI 53593 : �ū׿�X��եت�
//	Fu 103/10/01
//
void BkTpCurrDisplaySecSub(void)
{
	unsigned short i;
	unsigned short TpChgMdErrorFg;
	unsigned short TpSourPoint, TpDestPoint;
	unsigned short TpUsedStsFg1 = 0;
	unsigned short TpUsedStsFg2 = 0;
	unsigned short TpSourUsedStsFg1 = 0;
	unsigned short TpDestUsedStsFg1 = 0;
	unsigned short TpSourUsedStsFg2 = 0;
	unsigned short TpDestUsedStsFg2 = 0;
	//
	for(i = 0; i < TpMaxLp; i++)
	{
		if(*(tempData[i].TpSensorSour) > TpMaxLp)	// �ū׷P����ըӷ�
			*(tempData[i].TpSensorSour) = OFF;
		//
		if(*(tempData[i].TpSensorDest) > TpMaxLp) // �ū׷P����եت�
			*(tempData[i].TpSensorDest) = OFF;
		//
		if(*(tempData[i].TpPointSour) > TpMaxLp)	// �ū׿�X��ըӷ�
			*(tempData[i].TpPointSour) = OFF;
		//
		if(*(tempData[i].TpPointDest) > TpMaxLp) 	// �ū׿�X��եت�
			*(tempData[i].TpPointDest) = OFF;
	}
	//
	TpChgMdErrorFg = OFF;
	TpChgMdErrorFg = CmpIOChgPointStatsSub(TpMaxLp, TpMaxLp, 20121, &TpChgMdErrorFg, 0x0001); 	// �ū׷P����ըӷ�
	TpChgMdErrorFg = CmpIOChgPointStatsSub(TpMaxLp, TpMaxLp, 20136, &TpChgMdErrorFg, 0x0002); 	// �ū׷P����եت�
	TpChgMdErrorFg = CmpIOChgPointStatsSub(TpMaxLp, TpMaxLp, 20151, &TpChgMdErrorFg, 0x0004); 	// �ū׿�X��ըӷ�
	TpChgMdErrorFg = CmpIOChgPointStatsSub(TpMaxLp, TpMaxLp, 20166, &TpChgMdErrorFg, 0x0008); 	// �ū׿�X��եت�
	//
	for(i = 0; i < 4; i++) // Show Alarm.
	{
		if(TpChgMdErrorFg & (0x0001<<i))
			*(tempData[0].TpChgError) |= (0x0001<<i);
		else
			*(tempData[0].TpChgError) &= ~(0x0001<<i);
	}
	//
	TpSourUsedStsFg1 = OFF;
	TpDestUsedStsFg1 = OFF;
	TpUsedStsFg1 = OFF;
	//
	for(i = 0; i < TpMaxLp; i++)
	{  //  �ū׷P����ըӷ�     �ū׷P����եت�     
		if(*(tempData[i].TpSensorSour) && *(tempData[i].TpSensorDest) && !(*(tempData[0].TpChgError) & (0x0003)))
		{
			TpSourPoint = *(tempData[i].TpSensorSour) - 1;
			TpDestPoint = *(tempData[i].TpSensorDest) - 1;
			//
			TpUsedStsFg1 |= (0x00000001<<i);
			//
			TpDestUsedStsFg1 |= (0x00000001<<TpDestPoint);
			TpSourUsedStsFg1 |= (0x00000001<<TpSourPoint);
			TemperatureSensor[TpSourPoint] = TpDestPoint;
			TemperatureSensor[TpDestPoint] = TpSourPoint;
		}
		else
		{
			if((TpSourUsedStsFg1 & (0x00000001<<i)) || (TpDestUsedStsFg1 & (0x00000001<<i)));
			else
				TemperatureSensor[i] = i;
		}
		// �ū׿�X��ըӷ�     �ū׿�X��եت�     
		if(*(tempData[i].TpPointSour) && *(tempData[i].TpPointDest) && !(*(tempData[0].TpChgError) & (0x000c)))
		{
			TpSourPoint = *(tempData[i].TpPointSour) - 1;
			TpDestPoint = *(tempData[i].TpPointDest) - 1;
			//
			TpUsedStsFg2 |= (0x00000001<<i);
			//
			TpDestUsedStsFg2 |= (0x00000001<<TpDestPoint);
			TpSourUsedStsFg2 |= (0x00000001<<TpSourPoint);
			TemperatureOutput[TpSourPoint] = TpDestPoint;
			TemperatureOutput[TpDestPoint] = TpSourPoint;
		}
		else
		{
			if((TpSourUsedStsFg2 & (0x00000001<<i)) || (TpDestUsedStsFg2 & (0x00000001<<i)));
			else
				TemperatureOutput[i] = i;
		}
	}
}
/////////////////////////
//  Fu 103/10/01
//
unsigned short CmpIOChgPointStatsSub(unsigned short Len1, unsigned short Len2, unsigned short Address, unsigned short *TpChgPoingBack, unsigned short TpChgPointBit)
{
	unsigned short i, j;
	//
	for(j = 0; j < Len1; j++)
	{
		for(i = 0; i < Len2; i++)
		{
			if(*GetPtrCDM2(Address+i) && *GetPtrCDM2(Address+j))
			{
				if(i != j)
				{
					if(*GetPtrCDM2(Address+i) == *GetPtrCDM2(Address+j))
						*TpChgPoingBack |= TpChgPointBit;
				}
			}
		}
	}
	//
	return	*TpChgPoingBack;
}
//----------------------------------------------------------
// 	2015/04/20 Perpetual Calendar
void Date(void)
{
	unsigned short TmWeekFlag;
	unsigned short TmSecFlag;
	unsigned short TmMinFlag;
	unsigned short TmHourFlag;
	unsigned short TmDayFlag;
	unsigned short TmMonFlag;
	unsigned short TmYearFlag;
	//
	if(!StartCountFlag && (*tempData[0].Day || *tempData[0].Month || *tempData[0].Year))
		StartCountFlag = 1;
	//
	if(DateCnt >= Tm1Sec1_TRIG_MS)	//  1 sec unit
	{
		DateCnt = 0;
		TmWeekFlag = *tempData[0].Week;	// Week
		TmSecFlag = *tempData[0].Sec;		// SEC
		TmMinFlag = *tempData[0].Min;		// MIN
		TmHourFlag = *tempData[0].Hour;	// HOUR
		TmDayFlag = *tempData[0].Day;		// Day
		TmMonFlag = *tempData[0].Month;	// Month
		TmYearFlag = *tempData[0].Year;	// Year
		//
		TmSecFlag++;
		if(TmSecFlag > 59)
		{
			TmSecFlag = 0;
			TmMinFlag++;
			if(TmMinFlag > 59)
			{
				TmMinFlag = 0;
				TmHourFlag++;
        if(TmHourFlag > 23)
				{
					TmHourFlag = 0;
					TmDayFlag++;
					TmWeekFlag++;
					if(TmWeekFlag >= 7)
						TmWeekFlag = 0;
					//
					if((!(TmMonFlag%2) && (TmMonFlag<=6)) || ((TmMonFlag%2) && (TmMonFlag>=9)))	// 2 / 4 / 6 / 9 / 11 MONTH
					{
						if(TmMonFlag == 2)	    // 2
						{
							if(!(TmYearFlag % 4))	//
							{
								if(TmDayFlag > 29)
								{
									TmDayFlag = 1;
                  TmMonFlag++;
									if(TmMonFlag > 12)
									{
										TmYearFlag++;
										TmMonFlag = 1;
									}
								}
							}
							else
							{
								if(TmDayFlag > 28)
								{
									TmDayFlag = 1;
									TmMonFlag++;
									if(TmMonFlag > 12)
									{
										TmYearFlag++;
										TmMonFlag = 1;
									}
								}
							}
						}
						else	// 4 / 6 / 9 / 11 MONTH
						{
							if(TmDayFlag > 30)
							{
								TmDayFlag = 1;
								TmMonFlag++;
								if(TmMonFlag > 12)
								{
									TmYearFlag++;
									TmMonFlag = 1;
								}
							}
						}
					}
					else		// 1 / 3 / 5 / 7 / 8 / 10 / 12 MONTH
					{
						if(TmDayFlag > 31)
						{
							TmDayFlag = 1;
							TmMonFlag++;
							if(TmMonFlag > 12)
							{
								TmYearFlag++;
								TmMonFlag = 1;
							}
						}
					}
				}
			}
		}
		else
		{
			// Fu 103/07/31
			if((TmMonFlag == 4) || (TmMonFlag == 6) || (TmMonFlag == 9) || (TmMonFlag == 11))
			{
				if(TmDayFlag >= 31)
					TmDayFlag = 30;
			}
			else if(TmMonFlag == 2)
			{
				if(TmYearFlag % 4)
				{
					if(TmDayFlag >= 29)
						TmDayFlag = 28;
				}
				else
				{
					if(TmDayFlag >= 29)
						TmDayFlag = 29;
				}
			}
		}
		//
		*tempData[0].Week = TmWeekFlag;
		*tempData[0].Sec = TmSecFlag;
		*tempData[0].Min = TmMinFlag;
		*tempData[0].Hour = TmHourFlag;
		*tempData[0].Day = TmDayFlag;
		*tempData[0].Month = TmMonFlag;
		*tempData[0].Year = TmYearFlag;
	}
}
//
void BaudSet(void)
{
	// Define a GPIO used structure to initial GPIO-related parameters
	GPIO_InitTypeDef GPIO_InitStructure2;
	// Define a I2C used structure to initial I2C-related parameters
	//I2C_InitTypeDef I2C_InitStructure;
	//// Define a USART used structure to initial USART-related parameters
	USART_InitTypeDef USART_InitStructure2;
	// Define a NVIC used structure to initial interrupt-related parameters
	//NVIC_InitTypeDef NVIC_InitStructure;
	//// Define a TIMER used structure to initial timer-related parameters
	//TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	//// Define a EXTI used structure to initial external interrupt parameters
	//EXTI_InitTypeDef EXTI_InitStructure;	
	//
	// Set USART1 parameters related GPIO pins and initial USART1
	//
	USART_InitStructure2.USART_BaudRate = *tempData[0].VariableBaud != 0xA5A5 ? 38400 : 19200;
	USART_InitStructure2.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure2.USART_StopBits = USART_StopBits_1;
	USART_InitStructure2.USART_Parity = USART_Parity_No;
	USART_InitStructure2.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure2.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);  // Connect PA9 to USART1_Tx
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // Connect PA10 to USART1_Rx

	// Configure pin9 of GPIOA(USART1 Tx)
	GPIO_InitStructure2.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure2);

	// Configure pin10 of GPIOA(USART1 Rx)
	GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure2);

	USART_Init(USART1, &USART_InitStructure2);
	USART_Cmd(USART1, ENABLE);
}



/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(UART4, (uint8_t) ch);

  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET)
  {}

  return ch;
}




#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
