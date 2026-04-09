#include "Data.h"
///////////
//
//
//
///////////

#define EEPROM_ADDRESS		0x50
#define NACK_CHECK_MAX 5500
#define EEPROM_PAGE_SIZE 8  // EEPROM write page size = 16 Bytes = 8 Words
#define EEPROM_PAGE_MASK 0x07  // Page mask is 0x0F(4-bit for byte access) or 0x07(3-bit for word access)

// void read_FRAM(unsigned short addr, unsigned short *ptr_buff, unsigned short cnt);
// void write_FRAM(unsigned short addr, unsigned short *ptr_data, unsigned short cnt);
unsigned char
read_FRAM(unsigned long addr, unsigned short *ptr_buff, unsigned short cnt);
unsigned char
write_FRAM(unsigned long addr, unsigned short *ptr_data, unsigned short cnt);
void WREG(unsigned char s, unsigned char cs, unsigned char din);
void RREG(unsigned char s, unsigned char cs, unsigned char din);
/////////////////////
//
//	data	 							  
//
////////////////////
//**************************************************************************
//
//                             Global Variable
//
//**************************************************************************
//  data storage for transmission between CU_3505(A8) and CU_ADA(M3)

//extern unsigned short thermal_couple[12];
//extern unsigned short l_temperature, r_temperature;  // 2018/12/10 by kf
extern volatile short l_temperature, r_temperature;  // 2018/12/10 by kf
extern unsigned short thermal_hex[12];

extern unsigned short heat;

extern unsigned short id;
extern unsigned short tx_buffer_bk[TX_BUFFER_LEN];
extern unsigned short tx_buffer[TX_BUFFER_LEN];
extern unsigned short rx_buffer[RX_BUFFER_LEN];
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
extern unsigned short calc;

/*
These Variables below has been declared in the library file.
You should call this function by extern. These Variables are for ckecking the data from ecryption mode.
*/
extern unsigned char alpum_tx_data[8];
extern unsigned char alpum_rx_data[10];
extern unsigned char alpum_ex_data[8];

extern unsigned char _alpu_rand(void);

