// #include "stdlib.h"
#include "Data.h"
unsigned char package_in_flag = 0;
unsigned short TimerBase_package_cut = 0;
unsigned short package_extract_cnt = 0;
//unsigned short timer3_cnt = 0;
//extern unsigned long Tm1Cnt;
//extern unsigned long Tm2Cnt;
//extern unsigned long Tm3Cnt;
//extern unsigned long MainCnt;
//extern unsigned short MainErrCnt;

extern unsigned long bootloader_ver;// Fu 107/07/05
extern unsigned short DnApSysFg;	// Fu 107/07/05
extern unsigned char rx_buffer[RX_BUFFER_LEN];
extern unsigned char tx_buffer[TX_BUFFER_LEN];
extern unsigned short crc_temp;
extern unsigned short id;
extern volatile unsigned short SendRs485Fg;
extern unsigned short *pA20001;
extern volatile unsigned short TimerBase1;
extern volatile unsigned short TimerBase2;
extern volatile unsigned short TimerBase3;
extern unsigned short TimerBase22;
extern unsigned short RealTpOutPwm;
extern unsigned short BkPidPUnit[12];
extern unsigned short BkPidIUnit[12];
extern unsigned short BkPidDUnit[12];
extern struct TpPID_DATA TpPIDdata[12], tempData[12];
//
//extern void TempHWSave(void);
extern void FunAlarm(void);	// Fu 107/06/29
extern void LenAlarm(void);
extern void DataAlarm(void);
extern void CrcAlarm(void);
extern unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);


extern unsigned short buffer_index;
void Package_extract(void);
void HT_OUT(unsigned short);
// Fu 107/07/05
extern unsigned char APVer[4];
//
void ISR_Timer3(void)
{
// 	unsigned short ustmp = 0;
// 	static unsigned char flag = 0;
	if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update);

		// Enforce PA12 LOW (receive mode) when not transmitting
		if(SendRs485Fg == 0)
			USART1_RTS_L;

// Fu 107/01/29
//		Tm3Cnt = 0;	// !!!!!!!
//		timer3_cnt++;
		
#ifdef RS485_ENABLE
		if(package_in_flag)
			TimerBase_package_cut++;
		
		if(TimerBase_package_cut > (*tempData[0].VariableBaud != 0xA5A5 ? 14 : 23))
		{
			Package_extract();
			TimerBase_package_cut = 0;
		}
		/* 2018/08/14 by kf
		else
		{
			if(buffer_index > 255)
				Package_extract();
		}
		*/
#endif
	}
}

void
Package_extract(void)
{
	unsigned char dev_addr;
	unsigned char func_code;
	unsigned short length;
	unsigned short i, wait_cnt = 0;				  
	unsigned short DataLenCnt;		
	unsigned short AddressSetBk, DataSetBk, AddressSetBk2;
	unsigned short crc_temp_Bk;
	unsigned short idSw;
	//
	dev_addr = 0xff;
	func_code = 0xff;
	//
	while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
	{
		if(buffer_index >= RX_BUFFER_LEN)  // 2018/08/14 by kf
			USART_ReceiveData(USART1);  // 2018/08/14 by kf
		else  // 2018/08/14 by kf
			rx_buffer[buffer_index++] = (unsigned char)USART_ReceiveData(USART1);
		wait_cnt++;
		//
		if(wait_cnt > 64)
			break;
	}
	
	dev_addr = rx_buffer[0];
	func_code = rx_buffer[1];
	//
	if((SendRs485Fg == 0) && (dev_addr == id))
	{
		switch(func_code)
		{
			case 0x33:  // 3 of function code represents query user-modified data(system parameters) // RD
				//
				// case 1 : �O����ƪ���
				//
				DataLenCnt =  (rx_buffer[4]<<8) | rx_buffer[5];
				length = DataLenCnt;
				crc_temp = CRC16((unsigned char *)rx_buffer, 6);	// �]�tFUN & ID & ����
				crc_temp_Bk = rx_buffer[6] |  (rx_buffer[7]<<8);
				//
				if(crc_temp == crc_temp_Bk)
				{
					// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
					tx_buffer[0] = rx_buffer[0];  // Device address and Function code
					tx_buffer[1] = rx_buffer[1];  // Device address and Function code
					tx_buffer[2] = (unsigned char)length;  // Byte of length	 ���]�tFUN & ID & ����
					DataLenCnt = length;  // Byte of length	 ���]�tFUN & ID & ����(address + data = 1 word)
					//
					AddressSetBk = 	rx_buffer[2]<<8 | rx_buffer[3];
					//
					//	Fu 101/12/19
					//
					if(AddressSetBk <= AddressBase0)
					{
						tx_buffer[2] = (tx_buffer[2]*2);
						//
						if((DataLenCnt <= 0) || (DataLenCnt >= 250))
							break;
						//
						for(i=0; i<DataLenCnt; i++)
						{
							if((AddressSetBk+i) == 0)
								break;
							//if(AddressSetBk < AddressBase4)
								//AddressSetBk = AddressSetBk -  AddressBase3;
							//else
								//AddressSetBk = AddressSetBk -  AddressBase4;
							AddressSetBk2 = (AddressSetBk+i) + 20000;
							//
							//	Fu 101/10/19
							//
							if((AddressSetBk2 >= 20201) && (AddressSetBk2 <= 20212))
							{
								tx_buffer[3+(2*i)] = (BkPidPUnit[AddressSetBk2-20201] & 0xff00)>>8;
								tx_buffer[4+(2*i)] = BkPidPUnit[AddressSetBk2-20201] & 0x00ff;
							}
							else if((AddressSetBk2 >= 20216) && (AddressSetBk2 <= 20227))
							{
								tx_buffer[3+(2*i)] = (BkPidIUnit[AddressSetBk2-20216] & 0xff00)>>8;
								tx_buffer[4+(2*i)] = BkPidIUnit[AddressSetBk2-20216] & 0x00ff;
							}
							else if((AddressSetBk2 >= 20231) && (AddressSetBk2 <= 20242))
							{
								tx_buffer[3+(2*i)] = (BkPidDUnit[AddressSetBk2-20231] & 0xff00)>>8;
								tx_buffer[4+(2*i)] = BkPidDUnit[AddressSetBk2-20231] & 0x00ff;
							}
							else
							{
								tx_buffer[3+(2*i)] = (*GetPtrCDM2(AddressSetBk2) & 0xff00)>>8;
								tx_buffer[4+(2*i)] = *GetPtrCDM2(AddressSetBk2) & 0x00ff;
							}
						}
						crc_temp = CRC16((unsigned char *)tx_buffer, (DataLenCnt*2)+3); 	// ��� + ID + FUN + ����
						tx_buffer[(DataLenCnt*2)+3] = crc_temp & 0x00ff;
						tx_buffer[(DataLenCnt*2)+4] = (crc_temp & 0xff00)>>8;
						tx_buffer[(DataLenCnt*2)+5] = 0;
					}
					else
					{
						if((DataLenCnt <= 1) || (DataLenCnt >= 250))
							break;
						//
						for(i=0; i<(DataLenCnt/2); i++)
						{
							if((AddressSetBk <= AddressBase) || (AddressSetBk >= 21000))
							{													 
								tx_buffer[3+(2*i)] = 0;
								tx_buffer[4+(2*i)] = 0;
								break;
							}
							else
							{
								//
								//	Fu 101/10/19
								//
								if((AddressSetBk >= 20201) && (AddressSetBk <= 20212))
								{
									tx_buffer[3+(2*i)] = (BkPidPUnit[AddressSetBk-20201] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidPUnit[AddressSetBk-20201] & 0x00ff;
								}
								else if((AddressSetBk >= 20216) && (AddressSetBk <= 20227))
								{
									tx_buffer[3+(2*i)] = (BkPidIUnit[AddressSetBk-20216] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidIUnit[AddressSetBk-20216] & 0x00ff;
								}
								else if((AddressSetBk >= 20231) && (AddressSetBk <= 20242))
								{
									tx_buffer[3+(2*i)] = (BkPidDUnit[AddressSetBk-20231] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidDUnit[AddressSetBk-20231] & 0x00ff;
								}
								else
								{
									tx_buffer[3+(2*i)] = (*GetPtrCDM2(AddressSetBk+i) & 0xff00)>>8;
									tx_buffer[4+(2*i)] = *GetPtrCDM2(AddressSetBk+i) & 0x00ff;
								}
							}
						}
						crc_temp = CRC16((unsigned char *)tx_buffer, DataLenCnt+3); 	// ��� + ID + FUN + ����
						tx_buffer[DataLenCnt+3] = crc_temp & 0x00ff;
						tx_buffer[DataLenCnt+4] = (crc_temp & 0xff00)>>8;
						tx_buffer[DataLenCnt+5] = 0;
					}
				}
				//
				else
					CrcAlarm();
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
				break;
			//
			case 0x63:
				//
				// case 1 : �O����ƪ���
				//
				//
				// case 2 : �O���������
				//
				//
				// case 3 : �p��CRC�X
				//
				crc_temp = CRC16((unsigned char *)rx_buffer, 6);	// �]�tFUN & ID & ����
				crc_temp_Bk = rx_buffer[6] |  (rx_buffer[7]<<8);
				//
				if(crc_temp == crc_temp_Bk)
				{
					// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
					tx_buffer[0] = rx_buffer[0];  // Device address and Function code
					tx_buffer[1] = rx_buffer[1];  // Function
					//
					AddressSetBk = rx_buffer[2]<<8 | rx_buffer[3];
					//
					//	Fu 101/12/19
					//
					if(AddressSetBk <= AddressBase0)
					{
						DataSetBk = rx_buffer[4]<<8 | rx_buffer[5];
						if(AddressSetBk == 0)
							break;
						//if(AddressSetBk < AddressBase4)
							//AddressSetBk = AddressSetBk -  AddressBase3;
						//else
							//AddressSetBk = AddressSetBk -  AddressBase4;
						AddressSetBk = AddressSetBk + 20000;
						*GetPtrCDM2(AddressSetBk) = DataSetBk;
						//
						//	Fu 101/10/19
						//
						if((AddressSetBk >= 20201) && (AddressSetBk <= 20212))
							BkPidPUnit[AddressSetBk-20201] =DataSetBk;
						else if((AddressSetBk >= 20216) && (AddressSetBk <= 20227))
							BkPidIUnit[AddressSetBk-20216] =DataSetBk;
						else if((AddressSetBk >= 20231) && (AddressSetBk <= 20242))
							BkPidDUnit[AddressSetBk-20231] =DataSetBk;
						//
						AddressSetBk = 	rx_buffer[2]<<8 | rx_buffer[3];
						tx_buffer[2] = (AddressSetBk & 0xff00)>>8;	// address : Hi
						tx_buffer[3] = AddressSetBk & 0x00ff;		    // address : Low
						tx_buffer[4] = (DataSetBk & 0xff00)>>8;		// data : Hi
						tx_buffer[5] = DataSetBk & 0x00ff;			// data : Low
						//
						crc_temp = CRC16((unsigned char *)tx_buffer, 6); 	// ��� + ID + FUN + ����
						tx_buffer[6] = crc_temp & 0x00ff;
						tx_buffer[7] = (crc_temp & 0xff00)>>8;
					}
					else
					{
						DataSetBk = rx_buffer[4]<<8 | rx_buffer[5];
						if((AddressSetBk <= AddressBase) || (AddressSetBk >= 21000))
						{													 
							tx_buffer[2] = 0;
							tx_buffer[3] = 0;
							tx_buffer[4] = 0;
							tx_buffer[5] = 0;
						}
						else
						{
							*GetPtrCDM2(AddressSetBk) = DataSetBk;
							//
							//	Fu 101/10/19
							//
							if((AddressSetBk >= 20201) && (AddressSetBk <= 20212))
								BkPidPUnit[AddressSetBk-20201] =DataSetBk;
							else if((AddressSetBk >= 20216) && (AddressSetBk <= 20227))
								BkPidIUnit[AddressSetBk-20216] =DataSetBk;
							else if((AddressSetBk >= 20231) && (AddressSetBk <= 20242))
								BkPidDUnit[AddressSetBk-20231] =DataSetBk;
							//
							tx_buffer[2] = (AddressSetBk & 0xff00)>>8;	// address : Hi
							tx_buffer[3] = AddressSetBk & 0x00ff;		    // address : Low
							tx_buffer[4] = (DataSetBk & 0xff00)>>8;		// data : Hi
							tx_buffer[5] = DataSetBk & 0x00ff;			// data : Low
						}
						crc_temp = CRC16((unsigned char *)tx_buffer, 6); 	// ��� + ID + FUN + ����
						tx_buffer[6] = crc_temp & 0x00ff;
						tx_buffer[7] = (crc_temp & 0xff00)>>8;
					}
					//
				}  // end of if
				//
				else
					CrcAlarm();
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
				break;
			//	Fu 104/10/20
			case 0x13:  // �h����Ƽg�J// WR
				//
				// case 1 : �O����ƪ���
				//
				DataLenCnt =  (rx_buffer[4]<<8) | rx_buffer[5];
				length = DataLenCnt;
				crc_temp = CRC16((unsigned char *)rx_buffer, 7+rx_buffer[6]);	// �]�tFUN & ID & ����
				crc_temp_Bk = rx_buffer[7+rx_buffer[6]] |  (rx_buffer[8+rx_buffer[6]]<<8);
				//
				if(crc_temp == crc_temp_Bk)
				{
					// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
					tx_buffer[0] = rx_buffer[0];  // Device address and Function code
					tx_buffer[1] = rx_buffer[1];  // Device address and Function code
					tx_buffer[2] = rx_buffer[2];  // Statr Address1
					tx_buffer[3] = rx_buffer[3];  // Statr Address2
					tx_buffer[4] = rx_buffer[4];  // Data Len1
					tx_buffer[5] = rx_buffer[5];  // Data Len2
					DataLenCnt = length;  // Byte of length	 ���]�tFUN & ID & ����(address + data = 1 word)
					//
					AddressSetBk = 	rx_buffer[2]<<8 | rx_buffer[3];
					//
					if((DataLenCnt <= 0) || (DataLenCnt >= 125))
						break;
					//
					for(i=0; i<DataLenCnt; i++)
					{
						if((AddressSetBk+i) == 0)
							break;
						//
						AddressSetBk2 = (AddressSetBk+i) + 20000;
						//
						if((AddressSetBk2 >= 20201) && (AddressSetBk2 <= 20212))
						{ 
							BkPidPUnit[AddressSetBk2-20201] = rx_buffer[7+(2*i)]<<8 | rx_buffer[8+(2*i)];
						}
						else if((AddressSetBk2 >= 20216) && (AddressSetBk2 <= 20227))
						{
							BkPidIUnit[AddressSetBk2-20216] = rx_buffer[7+(2*i)]<<8 | rx_buffer[8+(2*i)];
						}
						else if((AddressSetBk2 >= 20231) && (AddressSetBk2 <= 20242))
						{
							BkPidDUnit[AddressSetBk2-20231] = rx_buffer[7+(2*i)]<<8 | rx_buffer[8+(2*i)];
						}
						else
						{
							*GetPtrCDM2(AddressSetBk2) = rx_buffer[7+(2*i)]<<8 | rx_buffer[8+(2*i)];
						}
					}
					crc_temp = CRC16((unsigned char *)tx_buffer, 6); 	// ��� + ID + FUN + Address + ����
					tx_buffer[6] = crc_temp & 0x00ff;
					tx_buffer[7] = (crc_temp & 0xff00)>>8;
					tx_buffer[8] = 0;
				}
				else
					CrcAlarm();
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
			break;
			//
			case 0x30:
				//
				// case 1 : �O����ƪ���
				//
//				ChkSumFg = SaveDataLenSub();  // 2012/09/29
				//
				// case 2 : �O���������
				//
//				if(ChkSumFg == 0)  // 2012/09/29
//					ChkSumFg = RdDataSub(dev_addr, func_code);  // 2012/09/29
//				else  // 2012/09/29
//					LenAlarm();  // 2012/09/29
				//
				// case 3 : �p��CRC�X
				//
//				if(ChkSumFg == 0)  // 2012/09/29
//				{  // 2012/09/29
					//DataLenCnt =  (rx_buffer[1]<<8) | (rx_buffer[1]>>8);
					DataLenCnt =  (rx_buffer[2]<<8) | rx_buffer[3];
					length = DataLenCnt;
					crc_temp = CRC16((unsigned char *)rx_buffer, length+4);	// �]�tFUN & ID & ����
					crc_temp_Bk = rx_buffer[length+4] |  (rx_buffer[length+5]<<8);
					//
					if(crc_temp == crc_temp_Bk)
					{
						// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
						tx_buffer[0] = rx_buffer[0];  // Device address and Function code
						tx_buffer[1] = rx_buffer[1];  // Device address and Function code
						tx_buffer[2] = (unsigned char)length;  // Byte of length	 ���]�tFUN & ID & ����
						DataLenCnt = length;  // Byte of length	 ���]�tFUN & ID & ����(address + data = 1 word)
						//
						if((DataLenCnt <= 1) || (DataLenCnt >= 250))
							break;
						for(i=0; i<(DataLenCnt/2); i++)
						{
							AddressSetBk = 	rx_buffer[4+(2*i)]<<8 | rx_buffer[5+(2*i)];
							if((AddressSetBk <= AddressBase) || (AddressSetBk >= 21000))
							{													 
								tx_buffer[3+(2*i)] = 0;
								tx_buffer[4+(2*i)] = 0;
							}
							else
							{
								//
								//	Fu 101/10/19
								//
								if((AddressSetBk >= 20201) && (AddressSetBk <= 20212))
								{
									tx_buffer[3+(2*i)] = (BkPidPUnit[AddressSetBk-20201] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidPUnit[AddressSetBk-20201] & 0x00ff;
								}
								else if((AddressSetBk >= 20216) && (AddressSetBk <= 20227))
								{
									tx_buffer[3+(2*i)] = (BkPidIUnit[AddressSetBk-20216] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidIUnit[AddressSetBk-20216] & 0x00ff;
								}
								else if((AddressSetBk >= 20231) && (AddressSetBk <= 20242))
								{
									tx_buffer[3+(2*i)] = (BkPidDUnit[AddressSetBk-20231] & 0xff00)>>8;
									tx_buffer[4+(2*i)] = BkPidDUnit[AddressSetBk-20231] & 0x00ff;
								}
								else
								{
									tx_buffer[3+(2*i)] = (*GetPtrCDM2(AddressSetBk) & 0xff00)>>8;
									tx_buffer[4+(2*i)] = *GetPtrCDM2(AddressSetBk) & 0x00ff;
								}
							}
						}
						crc_temp = CRC16((unsigned char *)tx_buffer, DataLenCnt+3); 	// ��� + ID + FUN + ����
						tx_buffer[DataLenCnt+3] = crc_temp & 0x00ff;
						tx_buffer[DataLenCnt+4] = (crc_temp & 0xff00)>>8;
						tx_buffer[DataLenCnt+5] = 0;
						//
						TimerBase3 = 0;
	//				SendRs485Fg = 1;  // 2018/02/26 by kf
					USART1_RTS_H;
					SendRs485Fg = 1;  // 2018/02/26 by kf
					}
					//
					else
						CrcAlarm();
//				}
//				else  // 2012/09/29
//					DataAlarm();  // 2012/09/29
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
				break;
			//
			case 0x60:
				//
				// case 1 : �O����ƪ���
				//
//				ChkSumFg = SaveDataLenSub();  // 2012/09/29
				//
				// case 2 : �O���������
				//
//				if(ChkSumFg == 0)  // 2012/09/29
//					ChkSumFg = RdDataSub(dev_addr, func_code);  // 2012/09/29
//				else  // 2012/09/29
//					LenAlarm();  // 2012/09/29
				//
				// case 3 : �p��CRC�X
				//
//				if(ChkSumFg == 0)
//				{
					DataLenCnt =  (rx_buffer[2]<<8) | rx_buffer[3];
					length = DataLenCnt;
					crc_temp = CRC16((unsigned char *)rx_buffer, length+4);	// �]�tFUN & ID & ����
					crc_temp_Bk = rx_buffer[length+4] |  (rx_buffer[length+5]<<8);
					//
					if(crc_temp == crc_temp_Bk)
					{
						// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
						tx_buffer[0] = rx_buffer[0];  // Device address and Function code
						tx_buffer[1] = rx_buffer[1];  // Device address and Function code
						tx_buffer[2] = (unsigned char)length;  // Byte of length	 ���]�tFUN & ID & ����
						DataLenCnt = length;  // Byte of length	 ���]�tFUN & ID & ����(address + data = 1 word)
						//
						if((DataLenCnt <= 3) || (DataLenCnt >= 250)) // 2014/09/04
							break;
						//
						for(i=0; i<(DataLenCnt/4); i++) //2014/09/04
						{
							AddressSetBk = 	rx_buffer[4+(4*i)]<<8 | rx_buffer[5+(4*i)]; // 2014/09/04
							DataSetBk = rx_buffer[6+(4*i)]<<8 | rx_buffer[7+(4*i)]; // 2014/09/04
							if((AddressSetBk <= AddressBase) || (AddressSetBk >= 21000))
							{													 
								tx_buffer[5+(4*i)] = 0; // 2014/09/04
								tx_buffer[6+(4*i)] = 0; // 2014/09/04
							}
							else
							{
								*GetPtrCDM2(AddressSetBk) = DataSetBk;
								//
								//	Fu 101/10/19
								//
								if((AddressSetBk >= 20201) && (AddressSetBk <= 20212))
									BkPidPUnit[AddressSetBk-20201] =DataSetBk;
								else if((AddressSetBk >= 20216) && (AddressSetBk <= 20227))
									BkPidIUnit[AddressSetBk-20216] =DataSetBk;
								else if((AddressSetBk >= 20231) && (AddressSetBk <= 20242))
									BkPidDUnit[AddressSetBk-20231] =DataSetBk;
								//
								tx_buffer[3+(i*4)] = (AddressSetBk & 0xff00)>>8;	// address : Hi
								tx_buffer[4+(i*4)] = AddressSetBk & 0x00ff;		    // address : Low
								tx_buffer[5+(i*4)] = (DataSetBk & 0xff00)>>8;			// data : Hi
								tx_buffer[6+(i*4)] = DataSetBk & 0x00ff;					// data : Low
							}
						}	
						crc_temp = CRC16((unsigned char *)tx_buffer, DataLenCnt+3); 	// ��� + ID + FUN + ����
						tx_buffer[DataLenCnt+3] = crc_temp & 0x00ff;
						tx_buffer[DataLenCnt+4] = (crc_temp & 0xff00)>>8;
						tx_buffer[DataLenCnt+5] = 0;
						//
						TimerBase3 = 0;
		//				SendRs485Fg = 1;  // 2018/02/26 by kf
						USART1_RTS_H;
						SendRs485Fg = 1;  // 2018/02/26 by kf
						//
					}  // end of if
					else
						CrcAlarm();
//				}
//				else  // 2012/09/29
//					DataAlarm();  // 2012/09/29
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
				break;
			// Fu 107/07/05
			case 0xEFu :
				//
				// case 1 : �O����ƪ���
				//
				DataLenCnt =  (rx_buffer[2]<<8) | rx_buffer[3];
				length = DataLenCnt;
				crc_temp = CRC16((unsigned char *)rx_buffer, 4);	// �]�tFUN & ID & ����
				crc_temp_Bk = rx_buffer[4] |  (rx_buffer[5]<<8);
				//
				if(crc_temp == crc_temp_Bk)
				{
					// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
					tx_buffer[0u] = rx_buffer[0u];  // Device address and Function code
					tx_buffer[1u] = rx_buffer[1u];  // Device address and Function code
					tx_buffer[2u] = 0x10u;  // Byte of length	 ���]�tFUN & ID & ����
					tx_buffer[3u] = APVer[0u];
					tx_buffer[4u] = APVer[1u];
					tx_buffer[5u] = APVer[2u];
					tx_buffer[6u] = APVer[3u];
					idSw = (GPIO_ReadInputData(GPIOF) & 0x0Fu);
					idSw = idSw ^ 0x0Fu;
					tx_buffer[7u] = idSw;
					tx_buffer[8u] = 0u;
					tx_buffer[9u] = (bootloader_ver & 0xffu);
					tx_buffer[10u] = ((bootloader_ver>>8u) & 0xffu);
					tx_buffer[11u] = 'A';
					tx_buffer[12u] = 'P';
					tx_buffer[13u] = 'V';
					tx_buffer[14u] = '1';
					tx_buffer[15u] = '2';
					tx_buffer[16u] = '0';
					tx_buffer[17u] = '4';
					tx_buffer[18u] = '0';
					//
					crc_temp = CRC16((unsigned char *)tx_buffer, (16u+3u)); 	// ��� + ID + FUN + ����
					tx_buffer[16u+3u] = crc_temp & 0x00ffu;
					tx_buffer[16u+4u] = (crc_temp & 0xff00u)>>8u;
					tx_buffer[16u+5u] = 0;
				}
				//
				else
				{
					CrcAlarm();
				}
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
			break;
			// Fu 107/06/29
			case 0xF1:  
					DataLenCnt =  (rx_buffer[2]<<8) | rx_buffer[3];
					//
					length = DataLenCnt;
					crc_temp = CRC16((unsigned char *)rx_buffer, length+4);	// �]�tFUN & ID & ����
					crc_temp_Bk = rx_buffer[length+4] |  (rx_buffer[length+5]<<8);
					//
					if(crc_temp == crc_temp_Bk)
					{
						// Set UART1_RTS 'H' for RS-232 convert RS-485 IC to send data
						tx_buffer[0] = rx_buffer[0];  // Device address and Function code
						tx_buffer[1] = rx_buffer[1];  // Device address and Function code
						tx_buffer[2] = (unsigned char)length;  // Byte of length	 ���]�tFUN & ID & ����
						DataLenCnt = length;  // Byte of length	 ���]�tFUN & ID & ����(address + data = 1 word)
						//
						if((DataLenCnt <= 3) || (DataLenCnt >= 250)) // 2014/09/04
						{
							LenAlarm();	// Fu 107/07/12
						}
						else
						{
							DnApSysFg = 0x1368;	// Fu 107/07/05
							//
							for(i=0; i<(DataLenCnt/4); i++) //2014/09/04
							{
								AddressSetBk = 	rx_buffer[4+(4*i)]<<8 | rx_buffer[5+(4*i)]; // 2014/09/04
								DataSetBk = rx_buffer[6+(4*i)]<<8 | rx_buffer[7+(4*i)]; // 2014/09/04
								tx_buffer[3+(i*4)] = (AddressSetBk & 0xff00)>>8;	// address : Hi
								tx_buffer[4+(i*4)] = AddressSetBk & 0x00ff;		    // address : Low
								tx_buffer[5+(i*4)] = (DataSetBk & 0xff00)>>8;			// data : Hi
								tx_buffer[6+(i*4)] = DataSetBk & 0x00ff;					// data : Low
							}	
							//
							crc_temp = CRC16((unsigned char *)tx_buffer, DataLenCnt+3); 	// ��� + ID + FUN + ����
							tx_buffer[DataLenCnt+3] = crc_temp & 0x00ff;
							tx_buffer[DataLenCnt+4] = (crc_temp & 0xff00)>>8;
							tx_buffer[DataLenCnt+5] = 0;
							//
							TimerBase3 = 0;
			//				SendRs485Fg = 1;  // 2018/02/26 by kf
							USART1_RTS_H;
							SendRs485Fg = 1;  // 2018/02/26 by kf
							//
						}
					}  // end of if
					else
					{
						CrcAlarm();
					}
//				}
//				else  // 2012/09/29
//					DataAlarm();  // 2012/09/29
				//
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();
				break;					
			//
			default:  // the field of function code is wrong
				// Clear the asserted interrupts.
				FunAlarm();	// Fu 107/06/29
				/*// Fu 107/01/29
				wait_cnt = 0;
				while(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
				{
					wait_cnt++;
					if(wait_cnt > 64)
						break;
					USART_ReceiveData(USART1);
				}*/
				TimerBase3 = 0;
//				SendRs485Fg = 1;  // 2018/02/26 by kf
				USART1_RTS_H;
				SendRs485Fg = 1;  // 2018/02/26 by kf
				TimerBase22 = 0;
				//MainRS485TxSub();			
				break;
		}  // end of switch
	}
		buffer_index = 0;
		package_in_flag = 0;
		TimerBase_package_cut = 0;
		package_extract_cnt++;
//	}
}
