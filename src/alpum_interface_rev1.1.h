
/***********************************************************************************
 *#defines
 ***********************************************************************************/
//Reference MCU for ATmega128
/*#define SCL_LOW			PORTB &= 0xEF
#define SCL_HIGH		PORTB |= 0x10
#define SDA_IN			DDRB  &= 0xF7
#define SDA_OUT			DDRB  |= 0x08
#define SDA_LOW			PORTB &= 0xF7
#define SDA_HIGH		PORTB |= 0x08
#define SDA_DETECT		PINB & 0x08
#define I2C_DELAY		delay_us(5)
#define I2C_DELAY_LONG	delay_us(100)
*/
#define ERROR_CODE_TRUE			0
#define ERROR_CODE_FALSE		1
#define ERROR_CODE_WRITE_ADDR	10
#define ERROR_CODE_WRITE_DATA	20
#define ERROR_CODE_READ_ADDR	30
#define ERROR_CODE_READ_DATA	40
#define ERROR_CODE_START_BIT	50
#define ERROR_CODE_APROCESS		60
#define ERROR_CODE_DENY			70

//#define BYPASS
#define RANDOM

#define delay_time 50
#define DELAY for(d = 0; d < delay_time; d++)

/***********************************************************************************
 * Variables
 ***********************************************************************************/

/***********************************************************************************
 * External Variables
 ***********************************************************************************/
/*
External variable, which is second of DS1337S RTC, for generating randomize value with srand() function.
*/
//extern unsigned char year, month, date, day, hour, minute, second;
extern unsigned short rand_seed;
/***********************************************************************************
 * Function Prototypes
 ***********************************************************************************/

unsigned char _i2c_write(unsigned char , unsigned char, unsigned char *, int);
unsigned char _i2c_read(unsigned char , unsigned char, unsigned char *, int);

/***********************************************************************************
 * External Function
 ***********************************************************************************/




/* EOF */

