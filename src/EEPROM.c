#include "EEPROM.h"


//*************************************************************************
//
//  						EEPROM write function!
//
//  1st parameter<addr> represents address of EEPROM. the range of 2K-byte
//    is 0x000H ~ 0x3FFH(word)
//  2nd parameter<*ptr_data> represents pointer of data variable(ex. data_16)
//  3rd parameter<cnt> represents number of sequential words(16-bit) to 
//	  transmit
//	
//	return error code-> 0x00:OK(No error) 
//						0x01:No acknowledge response after counting non-acknowledge
//						  more than NACK_CHECK_MAX.(No EEPROM or EEPROM is writing)
//						0xFF:out boundary of memory size and no memory operation
//							  has been done
//
//*************************************************************************
unsigned char
write_FRAM(unsigned long addr, unsigned short *ptr_data, unsigned short cnt)
{
// 	unsigned char addr_high_byte, addr_low_byte;  // 2015/03/09 by kf
	unsigned char addr_high_byte;  // 2015/03/09 by kf
	unsigned short i, less_than_page_size, Ack_check_count = 0;
	volatile unsigned short wait_cnt = 0;  /* volatile+short: GCC -O1 loop too fast for I2C timing */

	//if((addr + cnt) > 0x0FFFF)
	if((addr + cnt) > 0x400)
		return 0xFF;
	if((addr & EEPROM_PAGE_MASK))  // Transmit the data whose start address doesn't align start address of page buffer of EEPROM
	{
		less_than_page_size = ((EEPROM_PAGE_SIZE - (addr & EEPROM_PAGE_MASK)) > cnt) ? cnt : (EEPROM_PAGE_SIZE - (addr & EEPROM_PAGE_MASK));

		wait_cnt = 0;  // 2018/02/06 by kf
//		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));  // 2018/02/06 by kf
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 2;  // 2018/02/06 by kf
		}
		I2C_GenerateSTART(I2C2, ENABLE);
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 15)  // 2018/02/06 by kf
				return 3;  // 2018/02/06 by kf
		}
// 		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 		else  // Bank 0-Lower 512 Kbits
// 			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
		
		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
		
		Ack_check_count = 0;  // Clean to count the non-acknowledge
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 45)  // 2018/02/06 by kf
				return 4;  // 2018/02/06 by kf
		}
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_GenerateSTART(I2C2, ENABLE);
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 5;  // 2018/02/06 by kf
			}
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 6;  // 2018/02/06 by kf
			}
		}

		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 7;  // 2018/02/06 by kf
		}
// 		addr_high_byte = addr >> 7;  // Word-address to byte-address
// 		addr_low_byte = addr << 1;  // Word-address to byte-address
		
		addr_high_byte = (addr << 1) & 0xFF;  // Word-address to byte-address
		
		I2C_ClearFlag(I2C2, I2C_FLAG_AF);
		I2C_SendData(I2C2, addr_high_byte);
		
		wait_cnt = 0;  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 75)  // 2018/02/06 by kf
				return 33;  // 2018/02/06 by kf
		}
		
		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send high-byte of sub address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_high_byte);
//		}  // 2018/02/06 by kf
			wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1; // 2018/02/06 by kf
				if(wait_cnt > 75) // 2018/02/06 by kf
					return 8; // 2018/02/06 by kf
			}
		}  // 2018/02/06 by kf
		wait_cnt = 0; // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA)); // 2018/02/06 by kf
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA)) // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1; // 2018/02/06 by kf
			if(wait_cnt > 3) // 2018/02/06 by kf
				return 9; // 2018/02/06 by kf
		}
/*		I2C_SendData(I2C2, addr_low_byte);

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send low-byte of sub address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_low_byte);
		}*/
		for(i = 0; i < less_than_page_size; i++)
		{
			wait_cnt = 0; // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1; // 2018/02/06 by kf
				if(wait_cnt > 75) // 2018/02/06 by kf
					return 10; // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, *ptr_data);

			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send low-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, *ptr_data);
			}
			wait_cnt = 0; // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1; // 2018/02/06 by kf
				if(wait_cnt > 75) // 2018/02/06 by kf
					return 11; // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, (*ptr_data) >> 8);

			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send high-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, (*ptr_data) >> 8);
			}
			ptr_data++;
		}
		addr = addr + less_than_page_size;
		cnt = cnt - less_than_page_size;
		wait_cnt = 0; // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1; // 2018/02/06 by kf
			if(wait_cnt > 150) // 2018/02/06 by kf
				return 12; // 2018/02/06 by kf
		}
		I2C_GenerateSTOP(I2C2, ENABLE);
//		for(i = 0; i < 750; i++)  // Delay 3 ms for page write operation
//			DELAY;
	}

	while(cnt >= EEPROM_PAGE_SIZE)  // Transmit the data, whose start address aligns start address of page buffer of EEPROM, equal or more than 64-word
	{
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));  // 2018/02/06 by kf
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 90)  // 2018/02/06 by kf
				return 13;  // 2018/02/06 by kf
		}
		I2C_GenerateSTART(I2C2, ENABLE);
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 15)  // 2018/02/06 by kf
				return 14;  // 2018/02/06 by kf
		}
// 		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
//   			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 		else  // Bank 0-Lower 512 Kbits
// 			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
		
		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 45)  // 2018/02/06 by kf
				return 15;  // 2018/02/06 by kf
		}
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE));
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_GenerateSTART(I2C2, ENABLE);
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 16;
			}
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 17;  // 2018/02/06 by kf
			}
		}

//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 18;  // 2018/02/06 by kf
		}
// 		addr_high_byte = addr >> 7;  // Word-address to byte-address
// 		addr_low_byte = addr << 1;  // Word-address to byte-address
		
		addr_high_byte = (addr << 1) & 0xFF;  // Word-address to byte-address
		
		I2C_ClearFlag(I2C2, I2C_FLAG_AF);
		I2C_SendData(I2C2, addr_high_byte);

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send high-byte of sub address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_high_byte);
		}
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));  // 2018/02/06 by kf
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 19;  // 2018/02/06 by kf
		}
/*		I2C_SendData(I2C2, addr_low_byte);

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send low-byte of sub address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_low_byte);
		}*/
		for(i = 0; i < EEPROM_PAGE_SIZE; i++)
		{
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 75)  // 2018/02/06 by kf
					return 20;  // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, *ptr_data);
			
			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send low-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, *ptr_data);
			}
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 75)  // 2018/02/06 by kf
					return 21;  // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, (*ptr_data) >> 8);

			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send low-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, (*ptr_data) >> 8);
			}
			ptr_data++;
		}
		addr = addr + EEPROM_PAGE_SIZE;
		cnt = cnt - EEPROM_PAGE_SIZE;
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 150)  // 2018/02/06 by kf
				return 22;  // 2018/02/06 by kf
		}
		I2C_GenerateSTOP(I2C2, ENABLE);
//		for(i = 0; i < 750; i++)  // Delay 3 ms for page write operation
//			DELAY;
	}

	if(cnt > 0)	 // Transmit the data(remain), whose start address aligns start address of page buffer of EEPROM, less than 64-word
	{
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));  // 2018/02/06 by kf
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 9)  // 2018/02/06 by kf
				return 23;  // 2018/02/06 by kf
		}
		I2C_GenerateSTART(I2C2, ENABLE);
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 15)  // 2018/02/06 by kf
				return 24;  // 2018/02/06 by kf
		}
// 		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
//   			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 		else  // Bank 0-Lower 512 Kbits
// 			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
		
		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 45)  // 2018/02/06 by kf
				return 25;  // 2018/02/06 by kf
		}
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_GenerateSTART(I2C2, ENABLE);
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 26;  // 2018/02/06 by kf
			}
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/02/06 by kf  // 2018/03/22 by kf
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 27;  // 2018/02/06 by kf
			}
		}
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))  // 2018/02/06 by kf  // 2018/03/22 by kf
		while(!(I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 28;  // 2018/02/06 by kf
		}
// 		addr_high_byte = addr >> 7;  // Word-address to byte-address
// 		addr_low_byte = addr << 1;  // Word-address to byte-address
		
		addr_high_byte = (addr << 1) & 0xFF;  // Word-address to byte-address
		
		I2C_ClearFlag(I2C2, I2C_FLAG_AF);
		I2C_SendData(I2C2, addr_high_byte);

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send high-byte of sub address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_high_byte);
		}
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));  // 2018/02/06 by kf
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 29;  // 2018/02/06 by kf
		}
/*		I2C_SendData(I2C2, addr_low_byte);

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_SendData(I2C2, addr_low_byte);
		}*/
		for(; cnt > 0; cnt--)
		{
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 75)  // 2018/02/06 by kf
					return 30;  // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, *ptr_data);

			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send low-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, *ptr_data);
			}
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 75)  // 2018/02/06 by kf
					return 31;  // 2018/02/06 by kf
			}
			I2C_SendData(I2C2, (*ptr_data) >> 8);

			Ack_check_count = 0;  // Clean to count the non-acknowledge
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send high-byte of data for acknowledge polling
				//
				I2C_SendData(I2C2, (*ptr_data) >> 8);
			}
			ptr_data++;
		}
//		addr = addr + 8;
//		cnt = cnt - 8;
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 150)  // 2018/02/06 by kf
				return 32;  // 2018/02/06 by kf
		}
		I2C_GenerateSTOP(I2C2, ENABLE);
		// delay 3 ms
//		for(i = 0; i < 750; i++)
//			DELAY;
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////
//
// 						EEPROM read function
//
//  1st parameter<addr> represents address of EEPROM. the range of 128K-byte
//    is 0x000H ~ 0x3FFH(word)
//  2nd parameter<*ptr_data> represents pointer to data array
//  3rd parameter<cnt> represents number of sequential words to read
//
//	return error code-> 0x00:OK(No error)
//						0x01:No acknowledge response after counting non-acknowledge
//						  more than NACK_CHECK_MAX.(No EEPROM)
//						0xFF:out boundary of memory size and no memory operation
//							  has been done
///////////////////////////////////////////////////////////////////////////
unsigned char
read_FRAM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt)
{
	unsigned char separate_access = 0;
// 	unsigned char addr_high_byte, addr_low_byte;  // 2015/03/09 by kf
	unsigned char addr_high_byte;  // 2015/03/09 by kf
	unsigned short y, temp_cnt = 0, Ack_check_count = 0;
	union
	{
		unsigned char a[2];
		unsigned short b;
	}c;
	volatile unsigned short wait_cnt = 0;  /* volatile+short: GCC -O1 loop too fast for I2C timing */
	
	//if((addr + cnt) > 0x0FFFF)
	if((addr + cnt) > 0x400)
		return 0xFF;
	if(cnt != 0)
	{
//		for(y = 60000; y > 0; y--);
//		for(y = 38000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
		wait_cnt = 0;  // 2018/02/06 by kf
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 50)  // 2018/02/06 by kf
				return 51;  // 2018/02/06 by kf
		};
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTART(I2C2, ENABLE);
		wait_cnt = 0;  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 15)  // 2018/02/06 by kf
				return 52;  // 2018/02/06 by kf
		};
// 		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
//   			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 		else  // Bank 0-Lower 512 Kbits
// 		{
// 			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
// 			if((addr + cnt) > 0x8000)  // Separate respective read operation to bank 0 and bank 1 
// 				separate_access = 1;
// 		}
		
		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/03/22 by kf
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 45)  // 2018/02/06 by kf
				return 53;  // 2018/02/06 by kf
		}
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
		{
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			Ack_check_count++;
			if(Ack_check_count > NACK_CHECK_MAX)
			{
				I2C_GenerateSTOP(I2C2, ENABLE);
				return 0x01;
			}
			//
			// Re-send start and device address for acknowledge polling
			//
			I2C_GenerateSTART(I2C2, ENABLE);
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 5;  // 2018/02/06 by kf
			};
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));  // 2018/03/22 by kf
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)))  // 2018/03/22 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 55;  // 2018/02/06 by kf
			}
		}
		wait_cnt = 0;  // 2018/02/06 by kf
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA))
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 3)  // 2018/02/06 by kf
				return 56;  // 2018/02/06 by kf
		};
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
// 		addr_high_byte = addr >> 7;  // Word-address to byte-address
// 		addr_low_byte = addr << 1;  // Word-address to byte-address
		addr_high_byte = (addr << 1) & 0xFF;  // Word-address to byte-address
		Ack_check_count = 0;
		
//		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
//		{
//			Ack_check_count++;
//			if(Ack_check_count > NACK_CHECK_MAX)
//			{
//				I2C_SoftwareResetCmd(I2C2, ENABLE);
//				return 0x01;
//			}
//			//
//			// Re-send start and device address for acknowledge polling
//			//
//			I2C_GenerateSTART(I2C2, ENABLE);
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
//			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
//				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
//			else  // Bank 0-Lower 512 Kbits
//				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
//		}
		I2C_ClearFlag(I2C2, I2C_FLAG_AF);
		I2C_SendData(I2C2, addr_high_byte);
		wait_cnt = 0;  // 2018/02/06 by kf
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
		{
			wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
			if(wait_cnt > 75)  // 2018/02/06 by kf
				return 33;  // 2018/02/06 by kf
		}
// 		I2C_SendData(I2C2, addr_low_byte);
// 		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTOP(I2C2, ENABLE);

		//
		//  Process bank 0 accessing for cross-bank accessing
		//
		if(separate_access)
		{
// 			temp_cnt = 0x08000 - addr;
			temp_cnt = 0x400 - addr;
			if(temp_cnt > 1)  // Read data from bank 0 more than one word
			{
				wait_cnt = 0;  // 2018/02/06 by kf
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 3)  // 2018/02/06 by kf
						return 57;  // 2018/02/06 by kf
				};
				I2C_GenerateSTART(I2C2, ENABLE);  //S of transfer sequence diagram
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // EV5 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 15)  // 2018/02/06 by kf
						return 58;  // 2018/02/06 by kf
				};
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Address of transfer sequence diagram
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Receiver);  // 1 0 1 0 B2 B1 B0 ~W/R
	
//				I2C_AcknowledgeConfig(I2C2, DISABLE);
//				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
				
				// Determine if the address-value is sent
//				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
//				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET);
	
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 180)  // 2018/02/06 by kf
						return 59;  // 2018/02/06 by kf
				}
				c.a[0] = I2C_ReceiveData(I2C2);  // Data1 of transfer sequence diagram
				y = 0x1;
				while((temp_cnt > 1) || (y != 1))
				{
					if((temp_cnt == 2) && (y == 1))  // 2017/10/02 by kf
					{
							wait_cnt = 0;  // 2018/02/06 by kf
							while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)
							{
								wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
								if(wait_cnt > 180)  // 2018/02/06 by kf
									return 60;  // 2018/02/06 by kf
							};  // 2017/10/02 by kf
							I2C_AcknowledgeConfig(I2C2, DISABLE);  // 2017/10/02 by kf
					}
					wait_cnt = 0;  // 2018/02/06 by kf
					while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
					{
						wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
						if(wait_cnt > 180)  // 2018/02/06 by kf
							return 61;  // 2018/02/06 by kf
					};
					c.a[y] = I2C_ReceiveData(I2C2);  // Data2~DataN-1 of transfer sequence diagram
					y = y ^ 0x01;
					if(y == 0)
					{
						*ptr_buff = c.b;
						ptr_buff++;
						temp_cnt--;
					}
					if((temp_cnt == 1) && (y == 1))
					{
						I2C_AcknowledgeConfig(I2C2, DISABLE);  // Ack = 0 in EV7_1 of transfer sequence diagram
						I2C_GenerateSTOP(I2C2, ENABLE);  // STOP request in EV7_1 of transfer sequence diagram
					}
				}
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 180)  // 2018/02/06 by kf
						return 62;  // 2018/02/06 by kf
				};
				c.a[1] = I2C_ReceiveData(I2C2);
	
				*ptr_buff = c.b;
				ptr_buff++;
	
//				while(I2C2->CR1 & I2C_CR1_STOP);
				wait_cnt = 0;  // 2018/02/06 by kf
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF))
				{
					if(wait_cnt > 50)  // 2018/02/06 by kf
						return 63;  // 2018/02/06 by kf
				};
				
				I2C_AcknowledgeConfig(I2C2, ENABLE);
			}
			else  // Read data from bank 0 one word
			{
				wait_cnt = 0;  // 2018/02/06 by kf
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 3)  // 2018/02/06 by kf
						return 64;  // 2018/02/06 by kf
				};
				I2C_GenerateSTART(I2C2, ENABLE);  // S of transfer sequence diagram
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // EV5 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 15)  // 2018/02/06 by kf
						return 65;  // 2018/02/06 by kf
				};
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Address of transfer sequence diagram
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Receiver);  // 1 0 1 0 B2 B1 B0 ~W/R
				I2C_AcknowledgeConfig(I2C2, DISABLE);
				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))  // EV6 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 45)  // 2018/02/06 by kf
						return 66;  // 2018/02/06 by kf
				};
				wait_cnt = 0;  // 2018/02/06 by kf
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 180)  // 2018/02/06 by kf
						return 67;  // 2018/02/06 by kf
				};
				I2C_GenerateSTOP(I2C2, ENABLE);
				c.a[0] = I2C_ReceiveData(I2C2);
				c.a[1] = I2C_ReceiveData(I2C2);
	
				*ptr_buff = c.b;
				ptr_buff++;
				
				wait_cnt = 0;  // 2018/02/06 by kf
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF))
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 50)  // 2018/02/06 by kf
						return 68;  // 2018/02/06 by kf
				};
				I2C_AcknowledgeConfig(I2C2, ENABLE);
				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
			}

// 			addr = addr + temp_cnt;
			addr = 0;
			cnt = cnt - temp_cnt;
			//
			//	Carry cross-bank(bank 1) reading out
			//
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 69;  // 2018/02/06 by kf
			};
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
			I2C_GenerateSTART(I2C2, ENABLE);
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 70;  // 2018/02/06 by kf
			};
// 	  		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
// 			addr_high_byte = addr >> 7;  // Word-address to byte-address
// 			addr_low_byte = addr << 1;  // Word-address to byte-address
			addr_high_byte = (addr << 1) & 0xFF;  // Word-address to byte-address
			Ack_check_count = 0;
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 50)  // 2018/02/06 by kf
					return 71;  // 2018/02/06 by kf
			};
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF))  // Acknowledge response check for acknowledge polling
			{
				I2C_ClearFlag(I2C2, I2C_FLAG_AF);
				Ack_check_count++;
				if(Ack_check_count > NACK_CHECK_MAX)
				{
					I2C_GenerateSTOP(I2C2, ENABLE);
					return 0x01;
				}
				//
				// Re-send start and device address for acknowledge polling
				//
				I2C_GenerateSTART(I2C2, ENABLE);
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 15)  // 2018/02/06 by kf
						return 72;  // 2018/02/06 by kf
				};
// 				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Transmitter);  // 1 0 1 0 B2 B1 B0 ~W/R
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 50)  // 2018/02/06 by kf
						return 73;  // 2018/02/06 by kf
				};
			}
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			I2C_SendData(I2C2, addr_high_byte);
			wait_cnt = 0;  // 2018/02/06 by kf
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED))  // 2018/02/06 by kf
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 90)  // 2018/02/06 by kf
						return 74;  // 2018/02/06 by kf
			}
				
// 			I2C_SendData(I2C2, addr_low_byte);
// 			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
			I2C_GenerateSTOP(I2C2, ENABLE);
		}

		//
		// Process the remain data accessing in the same bank
		//
		if(cnt > 1)  // The remain data is more one word
		{
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 90)  // 2018/02/06 by kf
						return 75;  // 2018/02/06 by kf
			};
			I2C_GenerateSTART(I2C2, ENABLE);  //S of transfer sequence diagram
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // EV5 of transfer sequence diagram
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 45)  // 2018/02/06 by kf
					return 76;  // 2018/02/06 by kf
			}
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 	  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '0'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Receiver);  // 1 0 1 0 B2 B1 B0 ~W/R

//			I2C_AcknowledgeConfig(I2C2, DISABLE);
//			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
			
			// Determine if the address-value is sent
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
//			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET);

			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 180)  // 2018/02/06 by kf
					return 77;  // 2018/02/06 by kf
			};
			c.a[0] = I2C_ReceiveData(I2C2);  // Data1 of transfer sequence diagram
			y = 0x1;
			while((cnt > 1) || (y != 1))
			{
				if((cnt == 2) && (y == 1))  // 2017/10/02 by kf
				{
//					while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET);
//					I2C_AcknowledgeConfig(I2C2, DISABLE);  // Ack = 0 in EV7_1 of transfer sequence diagram
						wait_cnt = 0;  // 2018/02/06 by kf
						while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)  // 2017/10/02 by kf
						{
							wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
							if(wait_cnt > 180)  // 2018/02/06 by kf
								return 78;  // 2018/02/06 by kf
						};
						I2C_AcknowledgeConfig(I2C2, DISABLE);  // 2017/10/02 by kf
				}
				wait_cnt = 0;  // 2018/02/06 by kf
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
				{
					wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
					if(wait_cnt > 180)  // 2018/02/06 by kf
						return 79;  // 2018/02/06 by kf
				};
				c.a[y] = I2C_ReceiveData(I2C2);  // Data2~DataN-1 of transfer sequence diagram
				y = y ^ 0x01;
				if(y == 0)
				{
					*ptr_buff = c.b;
					ptr_buff++;
					cnt--;
				}
				if((cnt == 1) && (y == 1))
				{
					I2C_AcknowledgeConfig(I2C2, DISABLE);  // Ack = 0 in EV7_1 of transfer sequence diagram
//					I2C_GenerateSTOP(I2C2, ENABLE);  // STOP request in EV7_1 of transfer sequence diagram  // 2017/10/02 by kf
				}
			}
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED))  // EV7 of transfer sequence diagram
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 180)  // 2018/02/06 by kf
					return 80;  // 2018/02/06 by kf
			};
			c.a[1] = I2C_ReceiveData(I2C2);
			I2C_GenerateSTOP(I2C2, ENABLE);  // 2017/10/02 by kf

			*ptr_buff = c.b;

//			while(I2C2->CR1 & I2C_CR1_STOP);
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 50)  // 2018/02/06 by kf
					return 81;  // 2018/02/06 by kf
			};
			
			I2C_AcknowledgeConfig(I2C2, ENABLE);
		}
		else  // The remain data is one word
		{
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 90)  // 2018/02/06 by kf
					return 82;  // 2018/02/06 by kf
			};
			I2C_GenerateSTART(I2C2, ENABLE);  // S of transfer sequence diagram
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))  // EV5 of transfer sequence diagram
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 15)  // 2018/02/06 by kf
					return 83;  // 2018/02/06 by kf
			};
// 			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
// 	  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '1'
// 			else  // Bank 0-Lower 512 Kbits
// 				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '0'
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | (addr >> 7)) << 1), I2C_Direction_Receiver);  // 1 0 1 0 B2 B1 B0 ~W/R
			I2C_AcknowledgeConfig(I2C2, DISABLE);
			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
			wait_cnt = 0;  // 2018/02/06 by kf
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))  // EV6 of transfer sequence diagram
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 90)  // 2018/02/06 by kf
					return 84;  // 2018/02/06 by kf
			};
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET)
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 180)  // 2018/02/06 by kf
					return 85;  // 2018/02/06 by kf
			};
			I2C_GenerateSTOP(I2C2, ENABLE);
			c.a[0] = I2C_ReceiveData(I2C2);
			c.a[1] = I2C_ReceiveData(I2C2);

			*ptr_buff = c.b;
			
			wait_cnt = 0;  // 2018/02/06 by kf
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF))
			{
				wait_cnt = wait_cnt + 1;  // 2018/02/06 by kf
				if(wait_cnt > 50)  // 2018/02/06 by kf
					return 86;  // 2018/02/06 by kf
			};
			I2C_AcknowledgeConfig(I2C2, ENABLE);
			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////
//
// 						Write registers of ADS1248 function
//
//  1st parameter<s> represents start signal(pin) of ADS1248. when s = 1, 
//    it announce start conversion
//  2nd parameter<cs> represents cs(3 LSBs) signals of 3 ADS1248.
//  3rd parameter<din> represents data input of ADS1248 
//
///////////////////////////////////////////////////////////////////////////
void WREG(unsigned char s, unsigned char cs, unsigned char din)
{
	unsigned char cnt, b_din;
	unsigned char delay;

	T_SCLK_L;
	T_DIN_L;

	T_START_H;
	if(cs & 0x1)
		T_CS1_L;
	if(cs & 0x2)
		T_CS2_L;
	if(cs & 0x4)
		T_CS3_L;
	for(cnt = 0; cnt < 8; cnt++)
	{
		b_din = (din >> (7 - cnt)) & 0x01;
		T_SCLK_H;
		if(b_din)
			T_DIN_H;
		else
			T_DIN_L;
		DELAY_CYCLES(5);
		T_SCLK_L;
		DELAY_CYCLES(5);
	}
	T_START_L;
}
/////////////////////////
//
//
void RREG(unsigned char s, unsigned char cs, unsigned char din)
{
	unsigned char cnt, b_din, i;
	//
	if(cs & 0x1)
		T_DO[0] = 0;
	if(cs & 0x2)
		T_DO[1] = 0;
	if(cs & 0x4)
		T_DO[2] = 0;
	//send start address(register) of received data

	T_SCLK_L;
	T_DIN_L;
	DELAY_CYCLES(3);

	T_START_H;
	if(cs & 0x1)
		T_CS1_L;
	if(cs & 0x2)
		T_CS2_L;
	if(cs & 0x4)
		T_CS3_L;
	for(cnt = 0; cnt < 8; cnt++)
	{
		b_din = (din >> (7 - cnt)) & 0x01;
		T_SCLK_H;
		if(b_din)
		{
			T_DIN_H;
		}
		else
		{
			T_DIN_L;
		}
		DELAY_CYCLES(10);
		T_SCLK_L;
		DELAY_CYCLES(9);
	}
	//send number of bytes will be received(default: 0 for 1 byte(selected register))
	for(cnt = 0; cnt < 8; cnt++)
	{
		T_SCLK_H;
		T_DIN_L;
		DELAY_CYCLES(10);
		T_SCLK_L;
		DELAY_CYCLES(9);

	}
	//receive 7 bits of each selected register from DO
	for(cnt = 7; cnt >= 1; cnt--)
	{
		T_SCLK_H;
		T_DIN_H;
		DELAY_CYCLES(1);
// 		T_DO[0] = ((T_DO[0] << 1) | ((HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + (GPIO_PIN_4 << 2))))? 0x1 : 0x0));
// 		T_DO[1] = ((T_DO[1] << 1) | ((HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + (GPIO_PIN_4 << 2))))? 0x1 : 0x0));
// 		T_DO[2] = ((T_DO[2] << 1) | ((HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + (GPIO_PIN_5 << 2))))? 0x1 : 0x0));
		T_DO[0] = ((T_DO[0] << 1) | (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)? 0x1 : 0x0));
		T_DO[1] = ((T_DO[1] << 1) | (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)? 0x1 : 0x0));
		T_DO[2] = ((T_DO[2] << 1) | (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11)? 0x1 : 0x0));
		T_SCLK_L;
		DELAY_CYCLES(10);
	}
	T_SCLK_H;
	T_DIN_H;
	DELAY_CYCLES(1);
// 	T_DO[0] = ((T_DO[0] << 1) | ((HWREG(GPIO_PORTA_BASE + (GPIO_O_DATA + (GPIO_PIN_4 << 2))))? 0x1 : 0x0));
// 	T_DO[1] = ((T_DO[1] << 1) | ((HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + (GPIO_PIN_4 << 2))))? 0x1 : 0x0));
// 	T_DO[2] = ((T_DO[2] << 1) | ((HWREG(GPIO_PORTF_BASE + (GPIO_O_DATA + (GPIO_PIN_5 << 2))))? 0x1 : 0x0));
	T_DO[0] = ((T_DO[0] << 1) | (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6)? 0x1 : 0x0));
	T_DO[1] = ((T_DO[1] << 1) | (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_14)? 0x1 : 0x0));
	T_DO[2] = ((T_DO[2] << 1) | (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_11)? 0x1 : 0x0));
	T_SCLK_L;
	DELAY_CYCLES(10);
	T_START_L;
}

//*************************************************************************
//
//                         Initial Function
//
//*************************************************************************
void
HW_config(void)
{
	// Define a GPIO used structure to initial GPIO-related parameters
	GPIO_InitTypeDef GPIO_InitStructure;
	// Define a I2C used structure to initial I2C-related parameters
	I2C_InitTypeDef I2C_InitStructure;
	// Define a USART used structure to initial USART-related parameters
	USART_InitTypeDef USART_InitStructure;
	// Define a NVIC used structure to initial interrupt-related parameters
	NVIC_InitTypeDef NVIC_InitStructure;
	// Define a TIMER used structure to initial timer-related parameters
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	// Define a EXTI used structure to initial external interrupt parameters
	EXTI_InitTypeDef EXTI_InitStructure;
	
	unsigned short PrescalerValue = 0;

	//
	// Enable I2C1 peripheral, I2C2 peripheral and GPIO PORT corresponding to the selected pins of I2C1
	//
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);  // Used for EEPROM and TMP112
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);  // Used for ALPU-M and TMP112
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  // PA0~PA1 for USART4, PA4~PA7 for GPIO ADD1248, PA9~PA10 for USART1, 
														   // PA12 for GPIO USART1_RTS, PA15 for GPIO ADS1248
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  // PB5 for GPIO, PB6~PB7 for I2C1, PB10~PB11 for I2C2, PB0~PB2 for GPIO HEATER, 
														   // PB12~PB15 for GPIO ADS1248
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);  // PC10~PC12 for GPIO ADS1248
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);  // PD0~PD1 for GPIO(CAN?), PD9 for GPIO ADS1248, PD11~PD13 for GPIO HT and WD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);  // PE7~PE8 for GPIO HEATER, PE14~PE15 for GPIO
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);  // PF0~PF3 for GPIO switch, PF11~PF15 for GPIO HEATER
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);  // PG0~PG1 for GPIO HEATER, PG2 and PG5 for GPIO LED
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);  // 32-bit counter timer
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);  // 16-bit counter timer
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);  // 16-bit counter timer
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);  // 32-bit counter timer
	
	//
	// Reset I2C1 and I2C2 peripheral
	//
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);

	//
	// Enable USART1 peripheral for RS-485 and UART4 peripheral for console
	//
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	// Enable backup domain access
//	PWR_BackupAccessCmd(ENABLE);
	
/*
	// Set I2C3_SMBA pin as a GPIO for EX_WP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Initial I2C2_SMBA pin to 'Low' for WP pin of EEPROM(EX_WP)
	GPIO_ResetBits(GPIOA, GPIO_Pin_9);
*/

	// Set PE14 pin as a GPIO for EEPROM_WP
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// Initial PE14 pin to 'Low' for WP pin of EEPROM(EX_WP)
	GPIO_ResetBits(GPIOE, GPIO_Pin_14);
	
	// Set PA4 pin as a GPIO for T_CS1 PA5 pin as a GPIO for T_SCLK PA7 pin as a GPIO for T_DIN
	//	PA12 pin as a GPIO for USART1_RTS and PA15 pin as a GPIO for T_CS3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_12 | GPIO_Pin_15;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// Initial USART1_RTS signal as 'Low'
	USART1_RTS_L;
	
	// Set PB0~PB2 as GPIOs for D0~D2 and PB12 pin as a GPIO for T_CS2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_12;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// Set PC5 pin as a GPIO for HT_CS
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// Set PD9 pin as a GPIO for T_START and PD11 pin as a GPIO for HT_BURN_RST and PD13 pin as GPIO for WD_TRIG
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_11 | GPIO_Pin_13;
	// Set PD0 and PD1 as GPIOs for test-pin by kf 2018/02/06
	GPIO_InitStructure.GPIO_Pin = GPIO_InitStructure.GPIO_Pin | GPIO_Pin_0 | GPIO_Pin_1;  // 2018/02/06 by kf
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	// Set PE7~PE8 as GPIOs for D10~D11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	// Set PF11~PF15 as GPIOs for D3~D7
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
	
	// Set PG0~PG1 as GPIOs for D8~D9 and PG2 pin as GPIO for GRN_LED and PG5 pin as a GPIO for RED_LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_5;
	GPIO_Init(GPIOG, &GPIO_InitStructure);
	
	// Set PA6 pin as a GPIO for T_DO/RDY1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	
	// Set PB5 pin as a GPIO for TP_ALM1 and PB14 pin as GPIO for T_DO/RDY2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	// Set PC11 pin as a GPIO for T_DO/RDY3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	// Set PD12 pin as a GPIO for HT_BURN
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
// 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_0;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	
	// Set PE15 pin as a GPIO for TP_ALM2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
	
	// Set PF0 pin as a GPIO for Bootloader and PF1~PF3 as GPIOs for ID0~ID2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOF, &GPIO_InitStructure);


	//
	// Set I2C2 parameters related GPIO pins and initial I2C2
	//
	/* I2C2: PB10=SCL, PB11=SDA — both must be open-drain for I2C */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);  // I2C2_SCL
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);  // I2C2_SDA

	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;  // Operation as I2C archi.
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;  // I2C fast mode Tlow/Thigh = 16/9
	I2C_InitStructure.I2C_OwnAddress1 = 0x01;  // MCU's device address with 0x01
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;  // Enable Acknowledge
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;  // Device address is 7-bit
	I2C_InitStructure.I2C_ClockSpeed = 400000;  // Fast mode

	I2C_Cmd(I2C2, DISABLE);
	DELAY_CYCLES(50);
	I2C_Cmd(I2C2, ENABLE);
	I2C_Init(I2C2, &I2C_InitStructure);
	
	//
	// Set I2C1 parameters related GPIO pins and initial I2C1
	//
	/* I2C1: PB6=SCL, PB7=SDA — both must be open-drain for I2C */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);  // I2C1_SCL
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);  // I2C1_SDA

	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;  // Operation as I2C archi.
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_16_9;  // I2C fast mode Tlow/Thigh = 16/9
	I2C_InitStructure.I2C_OwnAddress1 = 0x01;  // MCU's device address with 0x01
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;  // Enable Acknowledge
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;  // Device address is 7-bit
	I2C_InitStructure.I2C_ClockSpeed = 400000;  // Fast mode

	I2C_Cmd(I2C1, DISABLE);
	DELAY_CYCLES(50);
	I2C_Cmd(I2C1, ENABLE);
	I2C_Init(I2C1, &I2C_InitStructure);
	
	//
	// Set USART1 parameters related GPIO pins and initial USART1
	//
	USART_InitStructure.USART_BaudRate = 38400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);  // Connect PA9 to USART1_Tx
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);  // Connect PA10 to USART1_Rx

	// Configure pin9 of GPIOA(USART1 Tx)
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Configure pin10 of GPIOA(USART1 Rx)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);

	//
	// Set UART4 parameters related GPIO pins and initial UART4
	//
// 	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);  // Connect PA0 to UART4_Tx
  	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);  // Connect PA1 to UART4_Rx

	// Configure pin0 of GPIOA(UART4 Tx)
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Configure pin1 of GPIOA(UART4 Rx)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_Init(UART4, &USART_InitStructure);
	USART_Cmd(UART4, ENABLE);

	//
	// Enable the LSE OSC
	//
//	RCC_LSEConfig(RCC_LSE_ON);
//	/* Wait till LSE is ready */  
//	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
//	{
//	}

	/* Select the RTC Clock Source */
//	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	/* Enable the RTC Clock */
//	RCC_RTCCLKCmd(ENABLE);

	/* Wait for RTC APB registers synchronisation */
//	RTC_WaitForSynchro();

	/* Calendar Configuration with LSI supposed at 32KHz */
//	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
//	RTC_InitStructure.RTC_SynchPrediv	=  0xFF; /* (32KHz / 128) - 1 = 0xFF*/
//	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
//	RTC_Init(&RTC_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
#ifdef RS485_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif

#ifdef EXTI_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelCmd= ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource12);
	EXTI_InitStructure.EXTI_Line = EXTI_Line12;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);
#endif

#ifdef TIMER0_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

	if(TIMER0_TRIG_MS > 10)  // 2017/10/03 by kf
	{
		PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER0_TRIG_MS) - 1;  // 2017/10/03 by kf
		TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	}
	else  // 2017/10/03 by kf
	{
//	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER0_TRIG_MS) - 1;  // 2017/10/03 by kf
	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000) - 1;  // 2017/10/03 by kf
	
//	TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	TIM_TimeBaseStructure.TIM_Period = (1000 * TIMER0_TRIG_MS) - 1;  // 2017/10/03 by kf
	}
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
// 	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
// 	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);
	
// 	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);  // 2017/10/02 by kf
	
// 	TIM_Cmd(TIM2, ENABLE);  // 2017/10/02 by kf
#endif

#ifdef TIMER1_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	if(TIMER1_TRIG_MS > 10)  // 2017/10/03 by kf
	{
		PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER1_TRIG_MS) - 1;  // 2017/10/03 by kf
		TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	}
	else  // 2017/10/03 by kf
	{
//	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER1_TRIG_MS) - 1;  // 2017/10/03 by kf
	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000) - 1;  // 2017/10/03 by kf
	
//	TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	TIM_TimeBaseStructure.TIM_Period = (1000 * TIMER1_TRIG_MS) - 1;  // 2017/10/03 by kf
	}
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
// 	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
// 	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);
	
// 	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);  // 2017/10/02 by kf
	
// 	TIM_Cmd(TIM3, ENABLE);  // 2017/10/02 by kf
#endif

#ifdef TIMER2_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	if(TIMER2_TRIG_MS > 10)  // 2017/10/03 by kf
	{
		PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER2_TRIG_MS) - 1;  // 2017/10/03 by kf
		TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	}
	else  // 2017/10/03 by kf
	{
//	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER2_TRIG_MS) - 1;  // 2017/10/03 by kf
	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000) - 1;  // 2017/10/03 by kf
	
//	TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	TIM_TimeBaseStructure.TIM_Period = (1000 * TIMER2_TRIG_MS) - 1;  // 2017/10/03 by kf
	}
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
// 	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
// 	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);
	
// 	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);  // 2017/10/02 by kf
	
// 	TIM_Cmd(TIM4, ENABLE);  // 2017/10/02 by kf
#endif

#ifdef TIMER3_ENABLE
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	if(TIMER3_TRIG_MS > 10)  // 2017/10/03 by kf
	{
		PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER3_TRIG_MS) - 1;  // 2017/10/03 by kf
		TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	}
	else  // 2017/10/03 by kf
	{
//	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000 * TIMER3_TRIG_MS) - 1;  // 2017/10/03 by kf
	PrescalerValue = (unsigned short) ((SystemCoreClock / 2) / 1000000) - 1;  // 2017/10/03 by kf
	
//	TIM_TimeBaseStructure.TIM_Period = 999;  // 2017/10/03 by kf
	TIM_TimeBaseStructure.TIM_Period = (1000 * TIMER3_TRIG_MS) - 1;  // 2017/10/03 by kf
	}
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
// 	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
	
// 	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);
	
// 	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);  // 2017/10/02 by kf
	
// 	TIM_Cmd(TIM5, ENABLE);  // 2017/10/02 by kf
#endif
}
