#include "DataInit.h"

void CheckEEPROM(unsigned short CheckNumber);
//extern unsigned short Right_temperature_IC(void);  // 2018/12/10 by kf
extern short Right_temperature_IC(void);  // 2018/12/10 by kf
//extern unsigned short Left_temperature_IC(void);  // 2018/12/10 by kf
extern short Left_temperature_IC(void);  // 2018/12/10 by kf
unsigned short DataInitSet(unsigned short SetNumber);

extern unsigned short R_L_Normal_Temp_Dir_Fg;	// Fu 108/12/25
extern unsigned short R_Normal_Temp_Dir_Fg;	// Fu 108/12/25
extern unsigned short L_Normal_Temp_Dir_Fg;	// Fu 108/12/25
extern unsigned short thermal_PosAndNeg[3][4];  // Fu 2018/12/10 : 記錄正負溫度狀態
extern unsigned long bootloader_ver;
extern unsigned short BkPidPUnit[12];
extern unsigned short BkPidIUnit[12];
extern unsigned short BkPidDUnit[12];

////////////////////
//
//
void init(void)
{
	unsigned short i;
	id = (GPIO_ReadInputData(GPIOF) & 0x0F) >> 1;
	id = id ^ 0x07;
	HT_OUT(heat);
	
	BURN_RST_H;
	HT_CS_H;
	// for heat out overload detection
	BURN_RST_L;
	for(i = 0; i < 255; i++);				 
	BURN_RST_H;
	
	heat = 0xff;
	
	// Clear EEPROM_WP pin for EEPROM writing
	GPIO_ResetBits(GPIOE, GPIO_Pin_14);
	// Reset USART1_RTS
	USART1_RTS_L;
	
	
	bootloader_ver = (*(unsigned long *)0x2001FFFC);
	// Check receive data register of USART1(RS-485) and clear it
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE))
		USART_ReceiveData(USART1);
	id = (GPIO_ReadInputData(GPIOF) & 0x0F) >> 1;
	id = id ^ 0x07;
	
	for(i = 0; i < 8; i++)
		alpum_tx_data[i] = _alpu_rand();
	_ALPU_action();
	
	T_START_L;
	T_CS13_H;
	T_CS2_H;
	for(i = 0; i < 50000; i++);
	
	// Reset command for ADS 1248 to reset 3 devices(0000.0110)
	{
		T_CS13_L;
		T_CS2_L;
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_H;  //1
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_H;  //1
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_H;
		T_DIN_L;  //0
		for(i = 0; i < 3; i++);
		T_SCLK_L;
		for(i = 0; i < 5; i++);
		T_SCLK_L;
		// Delay more than 1715 ns
		for(i = 0; i < 48; i++);
		T_CS13_H;
		T_CS2_H;
		for(i = 0; i < 3; i++);
		// Wait ~0.6 ms to restart
		for(i = 0; i < 20000; i++);
	}
	
	T_CS13_L;
	T_CS2_L;
	
	//  set 3 ADS1248s' write address from 02h(MUX1)
	WREG(0x0, 0x7, 0x40);
	//	set 3 ADS1248s' number of bytes after indicated address will be written
	WREG(0x0, 0x7, 0x03);
	//  set 3 ADS1248s' 00h(MUX0)-select messured positive and negative input
	WREG(0x0, 0x7, 0x01);//00110111
	//  set 3 ADS1248s' 01h(VBIAS)-select which pin internally connect to Vbias
	WREG(0x0, 0x7, 0x00);
	//  set 3 ADS1248s' 02h(MUX1)-select reference and oscillator and operation mode
	WREG(0x0, 0x7, 0x30);
	//  set 3 ADS1248s' 03h(SYS0)-data rate=20SPS, PGA=32
	WREG(0x0, 0x7, 0x62);
	//
	//  set 3 ADS1248s' write address from 07h(FSC0)
	WREG(0x0, 0x7, 0x47);
	//	set 3 ADS1248s' number of bytes after indicated address will be written
	WREG(0x0, 0x7, 0x01);
	//  set 3 ADS1248s' 07h(FSC0)-Full-Scale Calibration Register[0](LSByte)
	WREG(0x0, 0x7, 0x00);
	//  set 3 ADS1248s' 08h(FSC1)-Full-Scale Calibration Register[1]
	WREG(0x0, 0x7, 0x00);

	//  set 3 ADS1248s' write address from 09h(IDAC0)
	WREG(0x0, 0x7, 0x4A);
	//	set 3 ADS1248s' number of bytes after indicated address will be written
	WREG(0x0, 0x7, 0x00);
	//  set 3 ADS1248s' 09h(IDAC0)-IDAC Control Register[0] for DRDY MODE bit
	WREG(0x0, 0x7, 0x08);
	//
	T_DIN_L;
	T_SCLK_L;
	// Delay more than 1715 ns
	for(i = 0; i < 40; i++);
	T_CS13_H;
	T_CS2_H;
	for(i = 0; i < 3; i++);
	
	//
	// Turn on VBIAS of one ADS 1248(1st) on each channels' positive input 2011/09/20
	//
	T_CS1_L;
	
	//  set 3 ADS1248s' write address from 01h(VBIAS)
	WREG(0x0, 0x1, 0x41);
	//	set 3 ADS1248s' number of bytes after indicated address will be written
	WREG(0x0, 0x1, 0x00);
	//  set 3 ADS1248s' 01h(VBIAS)-select which pin internally connect to Vbias
	WREG(0x0, 0x1, 0x00);  // 01010101
	//WREG(0x0, 0x1, 0);  // 01010101	  // 取消VBISA功能
	//
	T_DIN_L;
	T_SCLK_L;
	// Delay more than 1715 ns
	for(i = 0; i < 40; i++);
	T_CS1_H;
	for(i = 0; i < 3; i++);
	
	//
	// Set 3 ADS 1248 data output rate with 20SPS, PGA with 32
	//
	T_CS13_L;
	T_CS2_L;

	WREG(0x0, 0x7, 0x43);
	WREG(0x0, 0x7, 0x00);
	WREG(0x0, 0x7, 0x62);
	T_SCLK_L;
	T_DIN_L;
	// Delay more than 1715 ns
	for(i = 0; i < 40; i++);
	T_CS13_H;
	T_CS2_H;
	for(i = 0; i < 3;i++);
	
	//  read j- or k-table from FRAM
//#ifdef J_TYPE
	read_FRAM(0x0, j_ch1, j_element);
	read_FRAM(0x40, j_ch2, j_element);
	read_FRAM(0x80, j_ch3, j_element);
//#endif
//#ifdef K_TYPE
	read_FRAM(0x100, k_ch1, k_element);
	read_FRAM(0x140, k_ch2, k_element);
	read_FRAM(0x180, k_ch3, k_element);
//#endif

#if 0 // Old hardware version by kf 2017/06/09
	j_ch1[0] = 0;			
	j_ch1[1] = 0;		
	j_ch1[2] = 0;	    
	j_ch1[3] = 0;	    
	j_ch1[4] = 9;	    // 0
	j_ch1[5] = 2118;	// 40
	j_ch1[6] = 4297;   	// 80
	j_ch1[7] = 6523;	// 120
	j_ch1[8] = 8780;	// 160
	j_ch1[9] = 11050;	// 200
	j_ch1[10] = 13325;	// 240
	j_ch1[11] = 15602;	// 280
	j_ch1[12] = 17868;	// 320
	j_ch1[13] = 20131;	// 360
	j_ch1[14] = 22389;	// 400
	j_ch1[15] = 24653;	// 440
	j_ch1[16] = 26926;	// 480
	j_ch1[17] = 29220;	// 520
	j_ch1[18] = 31547;	// 560  error
	j_ch1[19] = 32729;	// 600  error
	j_ch1[20] = 32729;
	j_ch1[21] = 32729;
	j_ch1[22] = 32729;
	j_ch1[23] = 32729;
	j_ch1[24] = 32729;
	j_ch1[25] = 32729;
	j_ch2[0] = 0;			
	j_ch2[1] = 0;		
	j_ch2[2] = 0;	    
	j_ch2[3] = 0;	    
	j_ch2[4] = 9;	    // 0
	j_ch2[5] = 2118;	// 40
	j_ch2[6] = 4297;   	// 80
	j_ch2[7] = 6523;
	j_ch2[8] = 8780;
	j_ch2[9] = 11050;
	j_ch2[10] = 13325;
	j_ch2[11] = 15602;
	j_ch2[12] = 17868;
	j_ch2[13] = 20131;
	j_ch2[14] = 22389;
	j_ch2[15] = 24653;
	j_ch2[16] = 26926;
	j_ch2[17] = 29220;
	j_ch2[18] = 31547;  // error
	j_ch2[19] = 32729;  // error
	j_ch2[20] = 32729;
	j_ch2[21] = 32729;
	j_ch2[22] = 32729;
	j_ch2[23] = 32729;
	j_ch2[24] = 32729;
	j_ch2[25] = 32729;
	j_ch3[0] = 0;			
	j_ch3[1] = 0;		
	j_ch3[2] = 0;	    
	j_ch3[3] = 0;	    
	j_ch3[4] = 9;	    // 0
	j_ch3[5] = 2118;	// 40
	j_ch3[6] = 4297;   	// 80
	j_ch3[7] = 6523;    // 120
	j_ch3[8] = 8780;
	j_ch3[9] = 11050;
	j_ch3[10] = 13325;
	j_ch3[11] = 15602;
	j_ch3[12] = 17868;
	j_ch3[13] = 20131;
	j_ch3[14] = 22389;
	j_ch3[15] = 24653;
	j_ch3[16] = 26926;
	j_ch3[17] = 29220;
	j_ch3[18] = 31547;  // error
	j_ch3[19] = 32729;  // error
	j_ch3[20] = 32729;
	j_ch3[21] = 32729;
	j_ch3[22] = 32729;
	j_ch3[23] = 32729;
	j_ch3[24] = 32729;
	j_ch3[25] = 32729;

	k_ch1[0] = 0;			
	k_ch1[1] = 0;		
	k_ch1[2] = 0;	    
	k_ch1[3] = 0;	    
	k_ch1[4] = 7;	    // 0
	k_ch1[5] = 1659;	// 40
	k_ch1[6] = 3353;   	// 80
	k_ch1[7] = 5047;	// 120
	k_ch1[8] = 6707;	// 160
	k_ch1[9] = 8344;	// 200
	k_ch1[10] = 9992;	// 240
	k_ch1[11] = 11666;	// 280
	k_ch1[12] = 13364;	// 320
	k_ch1[13] = 15078;	// 360
	k_ch1[14] = 16804;	// 400
	k_ch1[15] = 18538;	// 440
	k_ch1[16] = 20282;	// 480  error
	k_ch1[17] = 22028;	// 520
	k_ch1[18] = 23776;	// 560
	k_ch1[19] = 25519;	// 600
	k_ch1[20] = 25535;
	k_ch1[21] = 25535;
	k_ch1[22] = 25535;
	k_ch1[23] = 25535;
	k_ch1[24] = 25535;
	k_ch1[25] = 25535;
	k_ch1[26] = 25535;
	k_ch1[27] = 25535;
	k_ch1[28] = 25535;
	k_ch1[29] = 25535;
	k_ch1[30] = 25535;
	k_ch1[31] = 25535;
	k_ch1[32] = 25535;
	k_ch1[33] = 25535;
	k_ch1[34] = 25535;
	k_ch2[0] = 0;			
	k_ch2[1] = 0;		
	k_ch2[2] = 0;	    
	k_ch2[3] = 0;	    
	k_ch2[4] = 7;	    // 0
	k_ch2[5] = 1659;	// 40
	k_ch2[6] = 3353;   	// 80
	k_ch2[7] = 5047;	// 120
	k_ch2[8] = 6707;	// 160
	k_ch2[9] = 8344;	// 200
	k_ch2[10] = 9992;	// 240
	k_ch2[11] = 11666;	// 280
	k_ch2[12] = 13364;	// 320
	k_ch2[13] = 15078;	// 360
	k_ch2[14] = 16804;	// 400
	k_ch2[15] = 18538;	// 440
	k_ch2[16] = 20282;	// 480 error
	k_ch2[17] = 22028;	// 520
	k_ch2[18] = 23776;	// 560
	k_ch2[19] = 25519;	// 600 error
	k_ch2[20] = 25535;
	k_ch2[21] = 25535;
	k_ch2[22] = 25535;
	k_ch2[23] = 25535;
	k_ch2[24] = 25535;
	k_ch2[25] = 25535;
	k_ch2[26] = 25535;
	k_ch2[27] = 25535;
	k_ch2[28] = 25535;
	k_ch2[29] = 25535;
	k_ch2[30] = 25535;
	k_ch2[31] = 25535;
	k_ch2[32] = 25535;
	k_ch2[33] = 25535;
	k_ch2[34] = 25535;
	k_ch3[0] = 0;			
	k_ch3[1] = 0;		
	k_ch3[2] = 0;	    
	k_ch3[3] = 0;	    
	k_ch3[4] = 7;	    // 0
	k_ch3[5] = 1659;	// 40
	k_ch3[6] = 3353;   	// 80
	k_ch3[7] = 5047;	// 120
	k_ch3[8] = 6707;	// 160
	k_ch3[9] = 8344;	// 200
	k_ch3[10] = 9992;	// 240
	k_ch3[11] = 11666;	// 280
	k_ch3[12] = 13364;	// 320
	k_ch3[13] = 15078;	// 360
	k_ch3[14] = 16804;	// 400
	k_ch3[15] = 18538;	// 440
	k_ch3[16] = 20282;	// 480 error
	k_ch3[17] = 22028;	// 520
	k_ch3[18] = 23776;	// 560
	k_ch3[19] = 25519;	// 600 error
	k_ch3[20] = 25535;
	k_ch3[21] = 25535;
	k_ch3[22] = 25535;
	k_ch3[23] = 25535;
	k_ch3[24] = 25535;
	k_ch3[25] = 25535;
	k_ch3[26] = 25535;
	k_ch3[27] = 25535;
	k_ch3[28] = 25535;
	k_ch3[29] = 25535;
	k_ch3[30] = 25535;
	k_ch3[31] = 25535;
	k_ch3[32] = 25535;
	k_ch3[33] = 25535;
	k_ch3[34] = 25535;
#else  // New hardware version 2017/06/09 by kf
	j_ch1[0] = 58550;			
	j_ch1[1] = 59979;		
	j_ch1[2] = 61659;	    
	j_ch1[3] = 63530;	    
	j_ch1[4] = 9;	    // 0
	j_ch1[5] = 2119;	// 40
	j_ch1[6] = 4299;   	// 80
	j_ch1[7] = 6525;	// 120
	j_ch1[8] = 8782;	// 160
	j_ch1[9] = 11052;	// 200
	j_ch1[10] = 13328;	// 240
	j_ch1[11] = 15601;	// 280
	j_ch1[12] = 17872;	// 320
	j_ch1[13] = 20134;	// 360
	j_ch1[14] = 22394;	// 400
	j_ch1[15] = 24658;	// 440
	j_ch1[16] = 26930;	// 480
	j_ch1[17] = 29225;	// 520
	j_ch1[18] = 31552;	// 560  error
	j_ch1[19] = 32738;	// 600  error
	j_ch1[20] = 32738;
	j_ch1[21] = 32738;
	j_ch1[22] = 32738;
	j_ch1[23] = 32738;
	j_ch1[24] = 32738;
	j_ch1[25] = 32738;
	j_ch2[0] = 58550;			
	j_ch2[1] = 59979;		
	j_ch2[2] = 61659;	    
	j_ch2[3] = 63530;	    
	j_ch2[4] = 9;	    // 0
	j_ch2[5] = 2119;	// 40
	j_ch2[6] = 4299;   	// 80
	j_ch2[7] = 6525;	// 120
	j_ch2[8] = 8782;	// 160
	j_ch2[9] = 11052;	// 200
	j_ch2[10] = 13328;	// 240
	j_ch2[11] = 15601;	// 280
	j_ch2[12] = 17872;	// 320
	j_ch2[13] = 20134;	// 360
	j_ch2[14] = 22394;	// 400
	j_ch2[15] = 24658;	// 440
	j_ch2[16] = 26930;	// 480
	j_ch2[17] = 29225;	// 520
	j_ch2[18] = 31552;	// 560  error
	j_ch2[19] = 32738;	// 600  error
	j_ch2[20] = 32738;
	j_ch2[21] = 32738;
	j_ch2[22] = 32738;
	j_ch2[23] = 32738;
	j_ch2[24] = 32738;
	j_ch2[25] = 32738;
	j_ch3[0] = 58550;			
	j_ch3[1] = 59979;		
	j_ch3[2] = 61659;	    
	j_ch3[3] = 63530;	    
	j_ch3[4] = 9;	    // 0
	j_ch3[5] = 2119;	// 40
	j_ch3[6] = 4299;   	// 80
	j_ch3[7] = 6525;	// 120
	j_ch3[8] = 8782;	// 160
	j_ch3[9] = 11052;	// 200
	j_ch3[10] = 13328;	// 240
	j_ch3[11] = 15601;	// 280
	j_ch3[12] = 17872;	// 320
	j_ch3[13] = 20134;	// 360
	j_ch3[14] = 22394;	// 400
	j_ch3[15] = 24658;	// 440
	j_ch3[16] = 26930;	// 480
	j_ch3[17] = 29225;	// 520
	j_ch3[18] = 31552;	// 560  error
	j_ch3[19] = 32738;	// 600  error
	j_ch3[20] = 32738;
	j_ch3[21] = 32738;
	j_ch3[22] = 32738;
	j_ch3[23] = 32738;
	j_ch3[24] = 32738;
	j_ch3[25] = 32738;

	k_ch1[0] = 60273;			// -160
	k_ch1[1] = 61300;		
	k_ch1[2] = 62548;	    
	k_ch1[3] = 63975;	    
	k_ch1[4] = 8;	    // 0
	k_ch1[5] = 1662;	// 40
	k_ch1[6] = 3355;   	// 80
	k_ch1[7] = 5050;	// 120
	k_ch1[8] = 6710;	// 160
	k_ch1[9] = 8347;	// 200
	k_ch1[10] = 9996;	// 240
	k_ch1[11] = 11670;	// 280
	k_ch1[12] = 13368;	// 320
	k_ch1[13] = 15083;	// 360
	k_ch1[14] = 16808;	// 400
	k_ch1[15] = 18543;	// 440
	k_ch1[16] = 20287;	// 480  error
	k_ch1[17] = 22033;	// 520
	k_ch1[18] = 23782;	// 560
	k_ch1[19] = 25526;	// 600
	k_ch1[20] = 27265;
	k_ch1[21] = 27265;
	k_ch1[22] = 27265;
	k_ch1[23] = 27265;
	k_ch1[24] = 27265;
	k_ch1[25] = 27265;
	k_ch1[26] = 27265;
	k_ch1[27] = 27265;
	k_ch1[28] = 27265;
	k_ch1[29] = 27265;
	k_ch1[30] = 27265;
	k_ch1[31] = 27265;
	k_ch1[32] = 27265;
	k_ch1[33] = 27265;
	k_ch1[34] = 27265;
	k_ch2[0] = 60273;			// -160
	k_ch2[1] = 61300;		
	k_ch2[2] = 62548;	    
	k_ch2[3] = 63975;	    
	k_ch2[4] = 8;	    // 0
	k_ch2[5] = 1662;	// 40
	k_ch2[6] = 3355;   	// 80
	k_ch2[7] = 5050;	// 120
	k_ch2[8] = 6710;	// 160
	k_ch2[9] = 8347;	// 200
	k_ch2[10] = 9996;	// 240
	k_ch2[11] = 11670;	// 280
	k_ch2[12] = 13368;	// 320
	k_ch2[13] = 15083;	// 360
	k_ch2[14] = 16808;	// 400
	k_ch2[15] = 18543;	// 440
	k_ch2[16] = 20287;	// 480  error
	k_ch2[17] = 22033;	// 520
	k_ch2[18] = 23782;	// 560
	k_ch2[19] = 25526;	// 600
	k_ch2[20] = 27265;
	k_ch2[21] = 27265;
	k_ch2[22] = 27265;
	k_ch2[23] = 27265;
	k_ch2[24] = 27265;
	k_ch2[25] = 27265;
	k_ch2[26] = 27265;
	k_ch2[27] = 27265;
	k_ch2[28] = 27265;
	k_ch2[29] = 27265;
	k_ch2[30] = 27265;
	k_ch2[31] = 27265;
	k_ch2[32] = 27265;
	k_ch2[33] = 27265;
	k_ch2[34] = 27265;
	k_ch3[0] = 60273;			// -160
	k_ch3[1] = 61300;		
	k_ch3[2] = 62548;	    
	k_ch3[3] = 63975;	    
	k_ch3[4] = 8;	    // 0
	k_ch3[5] = 1662;	// 40
	k_ch3[6] = 3355;   	// 80
	k_ch3[7] = 5050;	// 120
	k_ch3[8] = 6710;	// 160
	k_ch3[9] = 8347;	// 200
	k_ch3[10] = 9996;	// 240
	k_ch3[11] = 11670;	// 280
	k_ch3[12] = 13368;	// 320
	k_ch3[13] = 15083;	// 360
	k_ch3[14] = 16808;	// 400
	k_ch3[15] = 18543;	// 440
	k_ch3[16] = 20287;	// 480  error
	k_ch3[17] = 22033;	// 520
	k_ch3[18] = 23782;	// 560
	k_ch3[19] = 25526;	// 600
	k_ch3[20] = 27265;
	k_ch3[21] = 27265;
	k_ch3[22] = 27265;
	k_ch3[23] = 27265;
	k_ch3[24] = 27265;
	k_ch3[25] = 27265;
	k_ch3[26] = 27265;
	k_ch3[27] = 27265;
	k_ch3[28] = 27265;
	k_ch3[29] = 27265;
	k_ch3[30] = 27265;
	k_ch3[31] = 27265;
	k_ch3[32] = 27265;
	k_ch3[33] = 27265;
	k_ch3[34] = 27265;
#endif  // 2017/06/09 by kf
	i = 0;  // 2018/12/10 by kf
	do
	{
#ifdef I2C2_MUX
		if(I2C2_MUX_lock())
		{
#endif
			l_temperature = Left_temperature_IC();
#ifdef I2C2_MUX
			I2C2_MUX_unlock();
		}
//		else  // 2018/12/10 by kf
//			continue;  // 2018/12/10 by kf

    i = i + 1;  // 2018/12/10 by kf
		if(i > 1000)  // 2018/12/10 by kf
		{
			l_temperature = 12000;  // 2018/12/10 by kf
			error_temperature = error_temperature | 0xF0;  // 2018/12/10 by kf
			break;  // 2018/12/10 by kf
		}
#endif
//	}while(l_temperature > 1000);  // 2018/12/10 by kf
	}while((l_temperature > ROOM_TEMPERATURE_UPPER_LIMIT) || (l_temperature < ROOM_TEMPERATURE_LOWER_LIMIT));  // 2018/12/10 by kf
	
	i = 0;  // 2018/12/10 by kf
	do
	{
#ifdef I2C1_MUX
		if(I2C1_MUX_lock())
		{
#endif
			r_temperature = Right_temperature_IC();
#ifdef I2C1_MUX
			I2C1_MUX_unlock();
		}
//		else  // 2018/12/10 by kf
//			continue;  // 2018/12/10 by kf
		
		i = i + 1;  // 2018/12/10 by kf
		if(i > 1000)  // 2018/12/10 by kf
		{
			r_temperature = 12000;  // 2018/12/10 by kf
			error_temperature = error_temperature | 0x0F;  // 2018/12/10 by kf
			break;  // 2018/12/10 by kf
		}
#endif
//	}while(r_temperature > 1000);  // 2018/12/10 by kf
	}while((r_temperature > ROOM_TEMPERATURE_UPPER_LIMIT) || (r_temperature < ROOM_TEMPERATURE_LOWER_LIMIT));  // 2018/12/10 by kf
	
#ifdef TIMER0_ENABLE
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
#endif
#ifdef TIMER3_ENABLE
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM5, ENABLE);
#endif
#ifdef TIMER2_ENABLE
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
#endif
#ifdef TIMER1_ENABLE
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
#endif
}
////////////////////////////////
//
//
void CDMInit(void)
{
	pCDM			= &BSS_RAM_CDM_START[0];
	pCDM3			= &BSS_RAM_CDM_START3[0];
	pA20001			= pCDM;	                // 20001 - 30000
	pA30001			= pCDM3;	                // 20001 - 30000
}
/////////////////////
//
//
unsigned short DataInitSet(unsigned short SetNumber)
{
  unsigned short i;
	unsigned short ReturnFg=0;
	//
	DataSet500W = ON;
	//
	switch(SetNumber)
	{
		case 0x01:
			CheckEEPROM(1);
			//
			for(i=0; i<12; i++)
			{
				*(tempData[i].HeatSet) = 2000; 		// 1 : 200.0 c
				*(tempData[i].PerHeatSet) = 1800;	// 2 : 180.0 c
				*(tempData[i].PIDp) = 350;	// 3
				BkPidPUnit[i] = 350;
				*(tempData[i].PIDi) = 25;		// 4
				BkPidIUnit[i] = 25;
				*(tempData[i].PIDd) = 100;	// 5
				BkPidDUnit[i] = 100;
				*(tempData[i].HiAlarm) = 300;			// 6
				*(tempData[i].LoAlarm) = 300;			// 7
				*(tempData[i].TpControl) = 2000;	// 8 // 2015/07/28
				*(tempData[i].TpDisplay) = 65535;	// 9
				*(tempData[i].PwmOffSet) = 0;			// 10
				*(tempData[i].ThermostatFun) = 0;	// 11
				*(tempData[i].Thermostat) = 2000;	// 12
				*(tempData[i].Proportion) = 50;		// 13
				*(tempData[i].TpConAddHi) = 2300;	// 14
				*(tempData[i].SynchronFun) = 0;		// 15
				*(tempData[i].Synchron) = 200;		// 16
				*(tempData[i].TwoUpOffset) = 200;	// 17
				*(tempData[i].ContrlFun) = 0;			// 18 : SSR & REALY
				*(tempData[i].CycleTm) = 10000;		// 19 : 10.000 SEC
				*(tempData[i].AuTnFun) = 0;	    	// 20
				*(tempData[i].HeatCtrlMd) = 0;	  // 21
				*(tempData[i].HeatHour) = 65535;	// 24
				*(tempData[i].HeatMin) = 65535;		// 25
				*(tempData[i].CoolSet) = 300;	    // 26
				*(tempData[i].OilHeat) = 400;	    // 27
				*(tempData[i].HeatWeek) = 0;			// 28
				*(tempData[i].HiOilSet) = 600;	  // 42
				*(tempData[i].KAndJSel) = 0;	    // 44  : 0-->K & 1-->J
				*(tempData[i].HeatFlashBit) = 0;  // 45
				*(tempData[i].TwoUpFun) = 0;			// 47
				*(tempData[i].HeatErrSts) = 0;	  // 49
				*(tempData[i].HiAlmSts) = 0;			// 50
				*(tempData[i].LowAlmSts) = 0;			// 51
				*(tempData[i].PwmOutput) = 0;			// 52
				*(tempData[i].PwmOutputCyc) = 0;	// RVS
				*(tempData[i].TempLinearErr) = 0;	// 53
				*(tempData[i].AutoTingSts) = 0;		// 54
				*(tempData[i].TwoHeatMd) = 0;			// 56
				*(tempData[i].TwoHeatTm) = 600;		// 57
				*(tempData[i].TwoHeatTp) = 500;		// 58
				*(tempData[i].AllTmAuTpSw) = 0;		// 59
				*(tempData[i].SlowTpUpSw) = 0;		// 60
				*(tempData[i].VariableBaud) = 0;
				*(tempData[i].AutoHeatBranchSwitch) = 127;
				*(tempData[i].AutoPerHeatBranchSwitch) = 127;
				*(tempData[i].HeatWattUnit) = 0;	// Fu 108/12/30 : 加入備份每段溫度的加熱瓦特數
				if(i < 7)
				{
					*tempData[i].HeatTm = 0;
					*tempData[i].HeatPerTm = 0;
				}
			}
			//
			CheckEEPROM(2);
		break;
		//
		case 0x02:
			for(i=0; i<12; i++)
			{
				*(tempData[i].PIDp) = 350;	// 3
				BkPidPUnit[i] = 350;
				*(tempData[i].PIDi) = 25;		// 4
				BkPidIUnit[i] = 25;
				*(tempData[i].PIDd) = 100;	// 5
				BkPidDUnit[i] = 100;
			}
		break;
	}
	//
	for(i=0; i<500; i++)
	{
		*GetPtrCDM3(30001+i) = *GetPtrCDM2(20001+i);
	}
	//
	ReturnFg = 0;
	//
	for(i = 0; i < 10; i++)  // 2017/11/06 by kf
	{
		if(I2C1_MUX_lock() != 0)  // 2017/11/06 by kf
		{
			write_FRAM(0x200, GetPtrCDM2(20001), 500);
			I2C1_MUX_unlock();  // 2017/11/06 by kf
			ReturnFg = 0x1368;  // 2017/11/06 by kf
			break;  // 2017/11/06 by kf
		}
	}
	//
	//if(i == 10)  // 2017/11/06 by kf
	//{
		//ReturnFg = 0x1368;  // 2017/11/06 by kf
	//}
	//
	DataSet500W = OFF;
	//
	return ReturnFg;
}
//
//
//
void TpTypeMdSet(void)
{
	unsigned short i, j;
	//
	OldKAndJSel = *(tempData[0].KAndJSel);
	//
	for(i=0; i<12; i++)
	{
		if(*(tempData[0].KAndJSel) & (0x01<<i))
		{
			ch1_sec_num[i]	= j_element;
			for(j=0; j<j_element; j++)
				ch1[i][j] = j_ch1[j];
		}
		else
		{
			ch1_sec_num[i]	= k_element;
			for(j=0; j<k_element; j++)
				ch1[i][j] = k_ch1[j];
		}
	}
	/*//
	*(tempData[0].TempLinearErr) = OFF;
	//
	for(i=0; i<12; i++)
	{
		for(j=4; j<(ch1_sec_num[i]-1); j++)			// 啟始段 = 0 度
		{
			if(ch1[i][j]  == 0xffff)
			{
				*(tempData[0].TempLinearErr) |= (0x01<<i);
				break;
			}
			else if(ch1[i][j+1]  <= ch1[i][j])
			{
				*(tempData[0].TempLinearErr) |= (0x01<<i);
				break;
			}
		}
	}*/
}
//----------------------------------------------------------
// 	2015/04/23 檢查EEPROM初始值是否正確
void CheckEEPROM(unsigned short CheckNumber)
{
	unsigned short CheckEEPROM_i;
	//
	switch(CheckNumber)
	{
		case 1:
			for(CheckEEPROM_i = 0; CheckEEPROM_i < 499; CheckEEPROM_i++) // EEPROM 清為0
				*GetPtrCDM2(20001+CheckEEPROM_i) = 0;
			//
			for(CheckEEPROM_i = 0; CheckEEPROM_i < 499; CheckEEPROM_i++) // 檢查EEPROM是否都為0
			{
				if(*GetPtrCDM2(20001+CheckEEPROM_i) != 0)
				{
					*GetPtrCDM2(20416) = 0xAAAA; // EEPROM不為0, 則顯示AAAA
					break;
				}
			}
			//
		break;
		//
		case 2:
			for(CheckEEPROM_i = 0; CheckEEPROM_i < 499; CheckEEPROM_i++)
			{
				if(*GetPtrCDM2(20001+CheckEEPROM_i) != EEPROMInitialValue[CheckEEPROM_i])
				{
					if(*GetPtrCDM2(20416) != 0xAAAA)
						*GetPtrCDM2(20416) = 0x5555;
					//
					*GetPtrCDM2(20417) = 0x5555;
					break;
				}
			}
			//
			if(*GetPtrCDM2(20417) != 0x5555)
			{
				if((*GetPtrCDM2(20416) != 0xAAAA) && (*GetPtrCDM2(20416) != 0x5555))
					*GetPtrCDM2(20416) = 0x5A5A;
				//
				*GetPtrCDM2(20417) = 0x72A2;
			}
			//
		break;
	}
}
