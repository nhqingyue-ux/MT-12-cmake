#include "stm32f4xx_i2c.h"

#define EEPROM_ADDRESS		0x50
#define NACK_CHECK_MAX 5500

//*************************************************************************
//
//  						EEPROM write function!
//
//  1st parameter<addr> represents address of EEPROM. the range of 128K-byte
//    is 0x0000H ~ 0xFFFFH(word)
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
#if 0
unsigned char
write_EEPROM(unsigned long addr, unsigned short *ptr_data, unsigned short cnt)
{
	unsigned char addr_high_byte, addr_low_byte;
	unsigned short i, less_than_page_size, Ack_check_count = 0;

	//if((addr + cnt) > 0x0FFFF)
	if((addr + cnt) > 0x10000)
		return 0xFF;
	if((addr & 0x003F))  // Transmit the data whose start address doesn't align start address of page buffer of EEPROM
	{
		less_than_page_size = ((64 - (addr & 0x003F)) > cnt) ? cnt : (64 - (addr & 0x003F));

		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
		I2C_GenerateSTART(I2C2, ENABLE);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
		else  // Bank 0-Lower 512 Kbits
			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
		
		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
		}

		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		addr_high_byte = addr >> 7;  // Word-address to byte-address
		addr_low_byte = addr << 1;  // Word-address to byte-address
		
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
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		I2C_SendData(I2C2, addr_low_byte);

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
		}
		for(i = 0; i < less_than_page_size; i++)
		{
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTOP(I2C2, ENABLE);
//		for(i = 0; i < 750; i++)  // Delay 3 ms for page write operation
//			DELAY;
	}

	while(cnt >= 64)  // Transmit the data, whose start address aligns start address of page buffer of EEPROM, equal or more than 64-word
	{
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
		I2C_GenerateSTART(I2C2, ENABLE);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
		else  // Bank 0-Lower 512 Kbits
			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
		}

//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		addr_high_byte = addr >> 7;  // Word-address to byte-address
		addr_low_byte = addr << 1;  // Word-address to byte-address
		
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
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		I2C_SendData(I2C2, addr_low_byte);

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
		}
		for(i = 0; i < 64; i++)
		{
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
		addr = addr + 64;
		cnt = cnt - 64;
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTOP(I2C2, ENABLE);
//		for(i = 0; i < 750; i++)  // Delay 3 ms for page write operation
//			DELAY;
	}

	if(cnt > 0)	 // Transmit the data(remain), whose start address aligns start address of page buffer of EEPROM, less than 64-word
	{
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
		I2C_GenerateSTART(I2C2, ENABLE);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
		else  // Bank 0-Lower 512 Kbits
			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
		}
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		addr_high_byte = addr >> 7;  // Word-address to byte-address
		addr_low_byte = addr << 1;  // Word-address to byte-address
		
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
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
		I2C_SendData(I2C2, addr_low_byte);

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
		}
		for(; cnt > 0; cnt--)
		{
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
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
//    is 0x0000H ~ 0xFFFFH(word)
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
read_EEPROM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt)
{
	unsigned char separate_access = 0;
	unsigned char addr_high_byte, addr_low_byte;
	unsigned short y, temp_cnt = 0, Ack_check_count = 0;
	union
	{
		unsigned char a[2];
		unsigned short b;
	}c;
	//if((addr + cnt) > 0x0FFFF)
	if((addr + cnt) > 0x10000)
		return 0xFF;
	if(cnt != 0)
	{
//		for(y = 60000; y > 0; y--);
//		for(y = 38000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
//		for(y = 60000; y > 0; y--);
		while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTART(I2C2, ENABLE);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
		if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
		else  // Bank 0-Lower 512 Kbits
		{
			I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			if((addr + cnt) > 0x8000)  // Separate respective read operation to bank 0 and bank 1 
				separate_access = 1;
		}

		Ack_check_count = 0;  // Clean to count the non-acknowledge
		while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
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
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '0'
			while(!(I2C_GetFlagStatus(I2C2, I2C_FLAG_TXE)) && !(I2C_GetFlagStatus(I2C2, I2C_FLAG_AF)));
		}
		while(!I2C_GetFlagStatus(I2C2, I2C_FLAG_TRA));
//		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
		addr_high_byte = addr >> 7;  // Word-address to byte-address
		addr_low_byte = addr << 1;  // Word-address to byte-address
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
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_SendData(I2C2, addr_low_byte);
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
		I2C_GenerateSTOP(I2C2, ENABLE);

		//
		//  Process bank 0 accessing for cross-bank accessing
		//
		if(separate_access)
		{
			temp_cnt = 0x08000 - addr;
			addr = addr + temp_cnt;
			cnt = cnt - temp_cnt;
			if(temp_cnt > 1)  // Read data from bank 0 more than one word
			{
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
				I2C_GenerateSTART(I2C2, ENABLE);  //S of transfer sequence diagram
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // EV5 of transfer sequence diagram
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Address of transfer sequence diagram
	
//				I2C_AcknowledgeConfig(I2C2, DISABLE);
//				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
				
				// Determine if the address-value is sent
//				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
//				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET);
	
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
				c.a[0] = I2C_ReceiveData(I2C2);  // Data1 of transfer sequence diagram
				y = 0x1;
				while((temp_cnt > 1) || (y != 1))
				{
					while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
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
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
				c.a[1] = I2C_ReceiveData(I2C2);
	
				*ptr_buff = c.b;
				ptr_buff++;
	
//				while(I2C2->CR1 & I2C_CR1_STOP);
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF));
				
				I2C_AcknowledgeConfig(I2C2, ENABLE);
			}
			else  // Read data from bank 0 one word
			{
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
				I2C_GenerateSTART(I2C2, ENABLE);  // S of transfer sequence diagram
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // EV5 of transfer sequence diagram
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Address of transfer sequence diagram
				I2C_AcknowledgeConfig(I2C2, DISABLE);
				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET);
				I2C_GenerateSTOP(I2C2, ENABLE);
				c.a[0] = I2C_ReceiveData(I2C2);
				c.a[1] = I2C_ReceiveData(I2C2);
	
				*ptr_buff = c.b;
				ptr_buff++;
				
				while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF));
				I2C_AcknowledgeConfig(I2C2, ENABLE);
				I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
			}

			//
			//	Carry cross-bank(bank 1) reading out
			//
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
			I2C_GenerateSTART(I2C2, ENABLE);
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
	  		I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
			addr_high_byte = addr >> 7;  // Word-address to byte-address
			addr_low_byte = addr << 1;  // Word-address to byte-address
			Ack_check_count = 0;
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
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
				I2C_GenerateSTART(I2C2, ENABLE);
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
				I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Transmitter);  // Set B0 bit of Control Byte as '1'
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
			}
			I2C_ClearFlag(I2C2, I2C_FLAG_AF);
			I2C_SendData(I2C2, addr_high_byte);
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
			I2C_SendData(I2C2, addr_low_byte);
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
			I2C_GenerateSTOP(I2C2, ENABLE);
		}

		//
		// Process the remain data accessing in the same bank
		//
		if(cnt > 1)  // The remain data is more one word
		{
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
			I2C_GenerateSTART(I2C2, ENABLE);  //S of transfer sequence diagram
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // EV5 of transfer sequence diagram
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
	  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '0'

//			I2C_AcknowledgeConfig(I2C2, DISABLE);
//			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
			
			// Determine if the address-value is sent
//			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
//			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_ADDR) == RESET);

			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
			c.a[0] = I2C_ReceiveData(I2C2);  // Data1 of transfer sequence diagram
			y = 0x1;
			while((cnt > 1) || (y != 1))
			{
				while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
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
					I2C_GenerateSTOP(I2C2, ENABLE);  // STOP request in EV7_1 of transfer sequence diagram
				}
			}
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED));  // EV7 of transfer sequence diagram
			c.a[1] = I2C_ReceiveData(I2C2);

			*ptr_buff = c.b;

//			while(I2C2->CR1 & I2C_CR1_STOP);
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF));
			
			I2C_AcknowledgeConfig(I2C2, ENABLE);
		}
		else  // The remain data is one word
		{
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));
			I2C_GenerateSTART(I2C2, ENABLE);  // S of transfer sequence diagram
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  // EV5 of transfer sequence diagram
			if(addr > 0x7FFF)  // Bank 1-Upper 512 Kbits
	  			I2C_Send7bitAddress(I2C2, ((EEPROM_ADDRESS | 0x4) << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '1'
			else  // Bank 0-Lower 512 Kbits
				I2C_Send7bitAddress(I2C2, (EEPROM_ADDRESS << 1), I2C_Direction_Receiver);  // Set B0 bit of Control Byte as '0'
			I2C_AcknowledgeConfig(I2C2, DISABLE);
			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Next);
			while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));  // EV6 of transfer sequence diagram
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BTF) == RESET);
			I2C_GenerateSTOP(I2C2, ENABLE);
			c.a[0] = I2C_ReceiveData(I2C2);
			c.a[1] = I2C_ReceiveData(I2C2);

			*ptr_buff = c.b;
			
			while(I2C_GetFlagStatus(I2C2, I2C_FLAG_STOPF));
			I2C_AcknowledgeConfig(I2C2, ENABLE);
			I2C_NACKPositionConfig(I2C2, I2C_NACKPosition_Current);
		}
	}
	return 0;
}
#endif
