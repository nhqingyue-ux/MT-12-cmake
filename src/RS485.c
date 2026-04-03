#include "RS485.h"			    

unsigned long uart_int_status;
unsigned short buffer_index = 0;
unsigned short max_TimerBase_package_cnt = 0;
extern unsigned char package_in_flag;
extern unsigned short TimerBase_package_cut;
void FunAlarm(void);	// Fu 107/06/29
// extern unsigned char capture;
// extern unsigned short l_temperature, r_temperature;
// unsigned short thermocouple[12] = {0};
// unsigned char max = 0;
//////////////////////
//
//	Function
//
#ifdef RS485_ENABLE
void ISR_RS485(void)
#endif
{
	unsigned short wait_cnt;
	
	if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET)  // 2018/03/08 by kf
	{
		package_in_flag = 1;  // 2018/03/08 by kf
		TimerBase_package_cut = 0;  // 2018/03/08 by kf
		if(buffer_index >= RX_BUFFER_LEN)  // 2018/08/14 by kf
			USART_ReceiveData(USART1);  // 2018/08/14 by kf
		else  // 2018/08/14 by kf
			rx_buffer[buffer_index++] = (unsigned char)USART_ReceiveData(USART1);  // 2018/03/08 by kf
		USART_GetFlagStatus(USART1, USART_FLAG_ORE);  // 2018/03/08 by kf
	}
	
	if(SendRs485Fg == 0)
	{
		uart_int_status = USART_GetITStatus(USART1, USART_IT_RXNE);
		if((uart_int_status == SET) && (DataSet500W == 0))
		{
			
			package_in_flag = 1;
			if(TimerBase_package_cut > max_TimerBase_package_cnt)
				max_TimerBase_package_cnt = TimerBase_package_cut;
			TimerBase_package_cut = 0;
			wait_cnt = 0;
			while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
			{
				if(buffer_index >= RX_BUFFER_LEN)  // 2018/08/14 by kf
					USART_ReceiveData(USART1);  // 2018/08/14 by kf
				else  // 2018/08/14 by kf
					rx_buffer[buffer_index++] = (unsigned char)USART_ReceiveData(USART1);
				wait_cnt++;
				if(wait_cnt > 64)
					break;
			}
			USART_ClearITPendingBit(USART1, USART_IT_RXNE);
		}
		else if(USART_GetFlagStatus(USART1, USART_FLAG_ORE) == SET)  // 2018/03/08 by kf
		{
			package_in_flag = 1;  // 2018/03/08 by kf
			TimerBase_package_cut = 0;  // 2018/03/08 by kf
			if(buffer_index >= RX_BUFFER_LEN)  // 2018/08/14 by kf
				USART_ReceiveData(USART1);  // 2018/08/14 by kf
			else  // 2018/08/14 by kf
				rx_buffer[buffer_index++] = (unsigned char)USART_ReceiveData(USART1);  // 2018/03/08 by kf
			USART_GetFlagStatus(USART1, USART_FLAG_ORE);  // 2018/03/08 by kf
		}
	}
}
////////////////////
//
//									   
unsigned short SaveDataLenSub(void)
{
	unsigned short wait_cnt;
	//
	//	case 1 : 記錄資料長度
	//
	error_rs485 = 0;
	//
	rx_buffer[2] = 0;
	rx_buffer[3] = 0;
	//
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != SET)
	{
		if(wait_cnt > TIMEOUT_RS485)
		{
			error_rs485 = 1;
			break;
		}
		else
		{
			wait_cnt++;
			error_rs485 = 0;
		}
	}
	//
	if(error_rs485 == 0)
	{
		rx_buffer[2] = (unsigned char)USART_ReceiveData(USART1);
		wait_cnt = 0;
		while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != SET)
		{
			if(wait_cnt > TIMEOUT_RS485)
				error_rs485 = 1;
			else
				wait_cnt++;
		}
		rx_buffer[3] = (unsigned char)USART_ReceiveData(USART1);
	}
	//
	return error_rs485;
}
///////////////////////////////
//
//
unsigned short RdDataSub(unsigned char DevId, unsigned char Fun)
{
	unsigned short DataLenCnt, wait_cnt, cnt;
	//
	if((Fun == 0x03) || (Fun == 0x06))
		DataLenCnt = 2;	// Address & Date (4 bytes)
	else												   
		DataLenCnt =  (rx_buffer[2]<<8) | (rx_buffer[3]);
	//
	for(cnt = 0; cnt<((DataLenCnt+2)/2); cnt++)	// +1 of CRC
	{
		wait_cnt = 0;
		while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != SET)
		{
			if(wait_cnt > TIMEOUT_RS485)
			{
				error_rs485 = 1;
				break;
			}  // end if
			else
			{
				wait_cnt++;
				error_rs485 = 0;
			}  // end else
		}  // end which
		if(error_rs485)
			break;
		rx_buffer[4+(cnt*2)] = (unsigned char)USART_ReceiveData(USART1);  // Low byte
		wait_cnt = 0;
		while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != SET)
		{
			if(wait_cnt > TIMEOUT_RS485)
				error_rs485 = 1;
			else
				wait_cnt++;
		}  // end which
		if(error_rs485)
			break;
		rx_buffer[5+(cnt*2)] = (unsigned char)USART_ReceiveData(USART1);  // High byte
	}  // end for
	
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)  // if there are some data in register, flush data register
		USART_ReceiveData(USART1);
	
	if((error_rs485 == 1) || (DevId != id))
		error_rs485 = 1;
		//break;
	//
	return error_rs485;
}
////////////////////////////////////
//
//
void LenAlarm(void)
{
	// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
	tx_buffer[0] = rx_buffer[0];  // Device address and Function code
	tx_buffer[1] = 0xF0;  // Function
	//
	tx_buffer[2] = 0x02;		// len
	tx_buffer[3] = 0x80;		// address : Hi
	tx_buffer[4] = 0x80;		    // address : Low
	//
	crc_temp = CRC16((unsigned char *)tx_buffer, 5); 	// 資料 + ID + FUN + 長度
	tx_buffer[5] = crc_temp & 0x00ff;
	tx_buffer[6] = (crc_temp & 0xff00)>>8;
}
////////////////////////////////////
//
//
void DataAlarm(void)
{
	// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
	tx_buffer[0] = rx_buffer[0];  // Device address and Function code
	tx_buffer[1] = 0xF0;  // Function
	//
	tx_buffer[2] = 0x02;		// len
	tx_buffer[3] = 0x81;		// address : Hi
	tx_buffer[4] = 0x81;		    // address : Low
	//
	crc_temp = CRC16((unsigned char *)tx_buffer, 5); 	// 資料 + ID + FUN + 長度
	tx_buffer[5] = crc_temp & 0x00ff;
	tx_buffer[6] = (crc_temp & 0xff00)>>8;
}
////////////////////////////////////
//
//
void CrcAlarm(void)
{
	// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
	tx_buffer[0] = rx_buffer[0];  // Device address and Function code
	tx_buffer[1] = 0xF0;  // Function
	//
	tx_buffer[2] = 0x02;		// len
	tx_buffer[3] = 0x82;		// address : Hi
	tx_buffer[4] = 0x82;		    // address : Low
	//
	crc_temp = CRC16((unsigned char *)tx_buffer, 5); 	// 資料 + ID + FUN + 長度
	tx_buffer[5] = crc_temp & 0x00ff;
	tx_buffer[6] = (crc_temp & 0xff00)>>8;
}
////////////////////////////////////
// Fu 107/06/29
// 
void FunAlarm(void)
{
	// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
	tx_buffer[0] = rx_buffer[0];  // Device address and Function code
	tx_buffer[1] = 0xF0;  // Function
	//
	tx_buffer[2] = 0x02;		// len
	tx_buffer[3] = 0x83;		// address : Hi
	tx_buffer[4] = 0x83;		    // address : Low
	//
	crc_temp = CRC16((unsigned char *)tx_buffer, 5); 	// 資料 + ID + FUN + 長度
	tx_buffer[5] = crc_temp & 0x00ff;
	tx_buffer[6] = (crc_temp & 0xff00)>>8;
}
