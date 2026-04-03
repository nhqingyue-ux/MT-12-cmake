#include "Data.h"
///////////
//
//
//
///////////
void ISR_RS485(void);
unsigned short SaveDataLenSub(void);
unsigned short RdDataSub(unsigned char DevId, unsigned char Fun);
void LenAlarm(void);
void DataAlarm(void);
void CrcAlarm(void);
//void MainRS485TxSub(void);
//
extern unsigned short CRC16(unsigned char *puchMsg, unsigned short usDataLen);
// extern void read_FRAM(unsigned short addr, unsigned short *ptr_buff, unsigned short cnt);
// extern void write_FRAM(unsigned short addr, unsigned short *ptr_data, unsigned short cnt);
extern unsigned char
read_FRAM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt);
extern unsigned char
read_FRAM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt);
extern void RS485TxSub(void);
/////////////////////
//						 	    
//	data
//
////////////////////
//extern unsigned short FirstRS485RunFg;
extern unsigned short RsTxRxSts;
extern unsigned short DataSet500W;
extern unsigned short  TimerBase3;
extern unsigned short SendRs485Fg;
extern unsigned short TimerBase22;
extern unsigned short *pSplcY;
extern unsigned short *pSplcM;
extern unsigned short *pSplcX;
extern unsigned short *pSplcK;
extern unsigned short *pSplcT;
extern unsigned short *pSplcC;
extern unsigned short *pSplcF;
extern unsigned short *pCDM;
extern unsigned short *pA00001;
extern unsigned short *pA02001;
extern unsigned short *pA10001;
extern unsigned short *pA10129;
extern unsigned short *pA11001;
extern unsigned short *pA11501;
extern unsigned short *pA12001;
extern unsigned short *pA20001;
//**************************************************************************
//
//                             Global Variable
//
//**************************************************************************
//  data storage for transmission between CU_3505(A8) and CU_ADA(M3)

//extern unsigned short thermal_couple[12];
//extern unsigned short l_temperature, r_temperature;  // 2018/12/10 by kf
extern short l_temperature, r_temperature;  // 2018/12/10 by kf
extern unsigned short thermal_hex[12];

extern unsigned short heat;

extern unsigned short id;
extern unsigned char tx_buffer[TX_BUFFER_LEN];
extern unsigned char tx_buffer_bk[TX_BUFFER_LEN];
extern unsigned char rx_buffer[RX_BUFFER_LEN];
extern unsigned char rx_bufferBk[RX_BUFFER_LEN];
extern unsigned short error_rs485;
extern unsigned short crc_temp;

extern unsigned short g_debug;
extern unsigned char err_msg;
extern unsigned short err_cnt;
extern unsigned short TEMPerr_cnt;
extern unsigned short CH_check;
//extern unsigned char b_test;
extern unsigned short rand_seed;

extern short difference;

extern unsigned char reg[4][3];

//  for other use
extern unsigned int T_DO[4];
extern unsigned int thermal[4];
extern unsigned short ch_test;

extern unsigned short j_ch1[26], j_ch2[26], j_ch3[26];
extern unsigned short k_ch1[35], k_ch2[35], k_ch3[35];
extern unsigned char left, right;
extern unsigned char mid;
extern int calc;								   
/*
These Variables below has been declared in the library file.
You should call this function by extern. These Variables are for ckecking the data from ecryption mode.
*/
extern unsigned char alpum_tx_data[8];
extern unsigned char alpum_rx_data[10];
extern unsigned char alpum_ex_data[8];

extern unsigned char _alpu_rand(void);

