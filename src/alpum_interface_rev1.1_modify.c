#include "stm32f4xx_i2c.h"
#include "stm32f4xx_usart.h"
#include <stdlib.h>
#include <stdio.h>
#include "alpum_interface_rev1.1.h"
#include "delay.h"  // for DELAY_CYCLES

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#define DEBUGER_MODE	0
#define PRINTFMODE	0
unsigned short d;
unsigned char error_code;
unsigned short test = 0;

#ifdef BYPASS
unsigned char alpum_tx_data[8];
unsigned char alpum_rx_data[10];
unsigned char alpum_ex_data[8];
unsigned char buffer_data[8];

unsigned char _alpu_rand(void);

#endif

#ifndef RANDOM

extern unsigned short EEPROM_data[10];


#endif
//extern  void UARTprintf(const char *pcString, ...);

#ifndef BYPASS
/*
_alpum_process fuction has been declared in the library file.
You should call this function by extern to ecrypt your system.
*/
extern unsigned char _alpum_process(void);

/*
These Variables below has been declared in the library file.
You should call this function by extern. These Variables are for ckecking the data from ecryption mode.
*/


extern unsigned char alpum_tx_data[8];
extern unsigned char alpum_rx_data[10];
extern unsigned char alpum_ex_data[8];


#endif

extern unsigned char temp;
extern unsigned char eeprom[8];

unsigned char _i2c_write(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
  unsigned char i; 

	printf("_i2c_write:\r\n (0x%x\t0x%x)\t", device_addr, sub_addr);
	for(i = 0; i < ByteNo; i++)
		printf("0x%x\t", *(buff+i));  // Actually
//		printf("%c\t", *(buff+i));  // for test character pattern
	printf("\r\n\r\n");
	while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY));
	I2C_GenerateSTART(I2C3, ENABLE);
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT));
//	I2CMasterSlaveAddrSet(I2C0_MASTER_BASE, (device_addr >> 1), false);
//	I2CMasterDataPut(I2C0_MASTER_BASE, sub_addr);
//	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
	I2C_Send7bitAddress(I2C3, ((device_addr) ), I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	I2C_SendData(I2C3, sub_addr);

	DELAY;
	for(i = 0; i < ByteNo - 1; i++)
	{
		while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
//	    I2CMasterDataPut(I2C0_MASTER_BASE, buff[i]);
//	    I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
		I2C_SendData(I2C3, buff[i]);
    DELAY;
	}
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
//	I2CMasterDataPut(I2C0_MASTER_BASE, buff[ByteNo - 1]);
//	I2CMasterControl(I2C0_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
	I2C_SendData(I2C3, buff[ByteNo - 1]);
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
	I2C_GenerateSTOP(I2C3, ENABLE);
	DELAY;
	return ERROR_CODE_TRUE;
}

unsigned char _i2c_read(unsigned char device_addr, unsigned char sub_addr, unsigned char *buff, int ByteNo)
{
	unsigned char i;

  	while(I2C_GetFlagStatus(I2C3, I2C_FLAG_BUSY));
	I2C_GenerateSTART(I2C3, ENABLE);
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT));
//	DELAY;
	I2C_Send7bitAddress(I2C3, ((device_addr) ), I2C_Direction_Transmitter);
//	DELAY;
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
	I2C_SendData(I2C3, sub_addr);
//	DELAY;
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
//	I2C_GenerateSTOP(I2C3, ENABLE);
	
//	DELAY;

	I2C_GenerateSTART(I2C3, ENABLE);  //S of transfer sequence diagram
	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_MODE_SELECT));  // EV5 of transfer sequence diagram
	I2C_Send7bitAddress(I2C3, ((device_addr) ), I2C_Direction_Receiver);  // Address of transfer sequence diagram

	DELAY;

	for(i = 0; i<ByteNo; i++)
	{
		while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
		buff[i] = I2C_ReceiveData(I2C3);
		DELAY;		
	}

	while(!I2C_CheckEvent(I2C3, I2C_EVENT_MASTER_BYTE_RECEIVED));
	I2C_GenerateSTOP(I2C3, ENABLE);  // STOP request in EV7_1 of transfer sequence diagram
	
	printf("_i2c_read:\r\n (0x%x\t0x%x)\t", device_addr, sub_addr);
	for(i = 0; i < ByteNo; i++)
		printf("0x%x\t", *(buff+i));
	printf("\r\n\r\n");
	return ERROR_CODE_TRUE;

}

void _alpu_delay_ms(unsigned int k)
{
	int m;
	DELAY_CYCLES(k*2);
}

#ifdef BYPASS
void
_alpum_bypass(void)
{
	int i;

	for(i = 0; i < 8; i++)
		alpum_ex_data[i] = alpum_tx_data[i] ^ 0x01;
}

unsigned char 
_alpum_process(void)
{
	unsigned char error_code;
	int i;

	//Read Protocol Test(No Stop Condition)
	error_code = _i2c_read(0x7a, 0x75, buffer_data, 8);
	if(error_code)
		return error_code;
#ifdef RANDOM
	//Seed Generate
	for(i = 0; i < 8; i++)
		alpum_tx_data[i] = _alpu_rand();
#endif
	//Writh Seed Data to ALPU-M
	//sub_address=0x80(Bypass Mode Set)
	error_code = _i2c_write(0x7a, 0x80, alpum_tx_data, 8);
	if(error_code)
		return error_code;

	//Read Result Data from ALPU-M
	error_code = _i2c_read(0x7a, 0x80, alpum_rx_data, 10);
	if(error_code)
		return error_code;

	//XOR operation
	_alpum_bypass();

	//Compare the encoded data and received data
	for(i = 0; i < 8; i++)
		if(alpum_ex_data[i] != alpum_rx_data[i])
			return 60;
	return 0;
}
#endif

unsigned char _alpu_rand(void)   //Modify this fuction using RTC. But you should not change the function name.
{
/*
  static unsigned long seed; // 2byte, must be a static variable

		rand_seed = 1017;

  srand(rand_seed);
  seed = seed + rand(); // rand(); <------------------ add time value
  seed =  seed * 1103515245 + 12345;

  return (seed/65536) % 32768;
*/
	static unsigned long seed = 0; // 2byte, must be a static variable
	if(seed % 2)
	{
		temp = eeprom[seed];
//		temp = (EEPROM_data[(seed / 2)] >> 8);
		seed++;
		return temp;
	}
	else
	{
		temp = eeprom[seed];
//		temp = EEPROM_data[(seed / 2)];
		seed++;
		return temp;
	}
}

void _ALPU_action(void)
{
	int i;
	int j=0;

    for(j=0; j<1; j++)   {
    
		error_code = _alpum_process();

		if(error_code)
		{
			printf("\r\nAlpu-M Encryption Test Fail!!!\r\n");
			printf("\r\nError Code : %d",error_code);
			printf("\n\r========================ALPU-M IC Encryption========================");
			printf("\r\n Tx Data : "); for (i=0; i<8; i++) printf("0x%2x ", alpum_tx_data[i]); printf("\r\n");
			printf(" Rx Data : "); for (i=0; i<10; i++) printf("0x%2x ", alpum_rx_data[i]); printf("\r\n");
			printf(" Ex Data : "); for (i=0; i<8; i++) printf("0x%2x ", alpum_ex_data[i]); printf("\r\n");
			printf("\n\r====================================================================");

			break;
		}

		else
		{
			printf("\r\nAlpu-M Encryption Test Success!!!\r\n");
	    	printf("\r\n\nError_code : %d", error_code);
			printf("\n\r========================ALPU-M IC Encryption========================");
			printf("\r\n Tx Data : "); for (i=0; i<8; i++) printf("0x%2x ", alpum_tx_data[i]); printf("\r\n");
			printf(" Rx Data : "); for (i=0; i<10; i++) printf("0x%2x ", alpum_rx_data[i]); printf("\r\n");
			printf(" Ex Data : "); for (i=0; i<8; i++) printf("0x%2x ", alpum_ex_data[i]); printf("\r\n");
			printf("\n\r====================================================================");
			printf("\r\n");
			return;
		}

    }

	//other functions..
	while(1);
}



/* EOF */
