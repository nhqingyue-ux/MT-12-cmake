/**
  ******************************************************************************
  * @file    stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
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
#include "stm32f4xx_it.h"
#include "main.h"
extern uint16_t SIZE;

extern uint8_t TxBuffer1[];	  
extern uint8_t TxBuffer2[];
extern uint8_t TxBuffer3[];

extern uint8_t RxBuffer1[];	  
extern uint8_t RxBuffer2[];
extern uint8_t RxBuffer3[];

extern uint8_t RxBuffer1_tmp[];
extern uint8_t RxBuffer2_tmp[];
extern uint8_t RxBuffer3_tmp[];


extern uint16_t RxCounter;
extern __IO uint32_t TimeOut;
extern uint8_t EXTIflag;

uint16_t Rx_index1=0;
uint16_t Rx_index2=0;
uint16_t Rx_index3=0;

int cnt = 0;
extern int Receive_Size;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
extern uint16_t PrescalerValue;

/** @addtogroup STM32F4_Discovery_Peripheral_Examples
  * @{
  */

/** @addtogroup IO_Toggle
  * @{
  */ 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	if(TimeOut != 0x00)
	{
		TimeOut--;
	}		
}

/****************************************************************************
	DMAx Streamx for USARTx Tx
*****************************************************************************/

/**
  * @brief  This function handles DMA2 Stream7 for USART1 Tx  global interrupt request.
  */
void DMA2_Stream7_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7))
	{
		while(USART_GetFlagStatus(USART1, USART_FLAG_TC)==RESET);
		DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);  
		GPIO_SetBits(GPIOD, GPIO_Pin_15); 
	}
}
/**
  * @brief  This function handles DMA1 Stream6 for USART2 Tx  global interrupt request.
  */

void DMA1_Stream6_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream6, DMA_IT_TCIF6))
	{
		while(USART_GetFlagStatus(USART2, USART_FLAG_TC)==RESET);
		DMA_ClearITPendingBit(DMA1_Stream6, DMA_IT_TCIF6);  
		GPIO_SetBits(GPIOD, GPIO_Pin_12);
	}
}
/**
  * @brief  This function handles DMA1 Stream3 for USART3 Tx  global interrupt request.
  */
void DMA1_Stream3_IRQHandler(void)
{
	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA1_Stream3, DMA_IT_TCIF3))
	{
		while(USART_GetFlagStatus(USART3, USART_FLAG_TC)==RESET);
		DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3); 
		GPIO_SetBits(GPIOD, GPIO_Pin_14); 
	}
}
/****************************************************************************
	DMAx Streamx for USARTx Rx
*****************************************************************************/

/**
  * @brief  This function handles DMA2 Stream5 for USART1 Rx  global interrupt request.
  */
void DMA2_Stream5_IRQHandler(void)
{
	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;
	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA2_Stream5, DMA_IT_TCIF5))
	{
		CurrDataCount = DMA_GetCurrDataCounter(DMA2_Stream5);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;		
	}
	if(RS485RxNum>0)
	{	
		for(i=0; i<RS485RxNum; i++)
		{
			RxBuffer1[Rx_index1++] =  RxBuffer1_tmp[i];	
		}								   
	}

	DMA_Receive(COM1, RxBuffer1_tmp, 8);	

	TIM_Cmd(TIM4, DISABLE);	
	TIM_DeInit(TIM4);

	TIM_TimeBaseStructure.TIM_Prescaler = 50;			
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 65535;	  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

	TIM_ClearFlag(TIM4, TIM_FLAG_Update);	//清除中斷，以免一啟用就中斷
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
		   		   
}
/**
  * @brief  This function handles DMA1 Stream5 for USART2 Rx  global interrupt request.
  */
void DMA1_Stream5_IRQHandler(void)
{

	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;
	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA1_Stream5, DMA_IT_TCIF5))
	{
		CurrDataCount = DMA_GetCurrDataCounter(DMA1_Stream5);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;		
	}
	if(RS485RxNum>0)
	{	
		for(i=0; i<RS485RxNum; i++)
		{
			RxBuffer2[Rx_index2++] =  RxBuffer2_tmp[i];	
		}	
	}

	DMA_Receive(COM2, RxBuffer2_tmp, 8);	

	TIM_Cmd(TIM2, DISABLE);
	TIM_DeInit(TIM2);

	TIM_TimeBaseStructure.TIM_Prescaler = 50;		
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 65535;	  
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM2, PrescalerValue, TIM_PSCReloadMode_Update);

	TIM_ClearFlag(TIM2, TIM_FLAG_Update);	//清除中斷，以免一啟用就中斷
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
}
/**
  * @brief  This function handles DMA1 Stream3 for USART3 Rx  global interrupt request.
  */
void DMA1_Stream1_IRQHandler(void)
{

	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;
	/* Test on DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA1_Stream1, DMA_IT_TCIF1))
	{
		CurrDataCount = DMA_GetCurrDataCounter(DMA1_Stream1);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;		
	}
	if(RS485RxNum>0)
	{	
		for(i=0; i<RS485RxNum; i++)
		{
			RxBuffer3[Rx_index3++] =  RxBuffer3_tmp[i];	
		}	
	}

	DMA_Receive(COM3, RxBuffer3_tmp, 8);	

	TIM_Cmd(TIM3, DISABLE);
	TIM_DeInit(TIM3);

	TIM_TimeBaseStructure.TIM_Prescaler = 50;			
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 65535;	  	
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);

	TIM_ClearFlag(TIM3, TIM_FLAG_Update);	//清除中斷，以免一啟用就中斷
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
} 

/****************************************************************************
	TIMx_IRQHandler
*****************************************************************************/
void TIM4_IRQHandler(void)
{

	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;

	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
		TIM_DeInit(TIM4);
		CurrDataCount = DMA_GetCurrDataCounter(DMA2_Stream5);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;
	
		if(RS485RxNum>0)
		{
			DMA_Receive(COM1, RxBuffer1_tmp, RS485RxNum);
			for(i=0; i<RS485RxNum; i++)
			{			
				RxBuffer1[Rx_index1++] =  RxBuffer1_tmp[i];
			}
	
		}
	
	//		Rx_index1 =0;
		for(i=0; i<RxBufferSize; i++)
			 RxBuffer1_tmp[i] = 0;
	}//清除中斷旗標


												 
	DMA_Receive(COM1, RxBuffer1_tmp, 1);
	TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM4, DISABLE);
}

void TIM2_IRQHandler(void)
{

	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;

	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		TIM_DeInit(TIM2);
		CurrDataCount = DMA_GetCurrDataCounter(DMA1_Stream5);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;
	
		if(RS485RxNum>0)
		{
			DMA_Receive(COM2, RxBuffer2_tmp, RS485RxNum);
			for(i=0; i<RS485RxNum; i++)
			{			
				RxBuffer2[Rx_index2++] =  RxBuffer2_tmp[i];
			}
	
		}
	
	//		Rx_index2 =0;
		for(i=0; i<RxBufferSize; i++)
			 RxBuffer2_tmp[i] = 0;
	}//清除中斷旗標


												 
	DMA_Receive(COM2, RxBuffer2_tmp, 1);
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM2, DISABLE);
}

void TIM3_IRQHandler(void)
{

	uint16_t CurrDataCount;
	uint8_t RS485RxNum = 0;
	uint8_t RxBufferSize = Receive_Size;
	int i;

	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
		TIM_DeInit(TIM3);
		CurrDataCount = DMA_GetCurrDataCounter(DMA1_Stream1);    //讀取DMA剩餘的字元組
		RS485RxNum =  RxBufferSize - (uint8_t)CurrDataCount;
	
		if(RS485RxNum>0)
		{
			DMA_Receive(COM3, RxBuffer3_tmp, RS485RxNum);
			for(i=0; i<RS485RxNum; i++)
			{			
				RxBuffer3[Rx_index3++] =  RxBuffer3_tmp[i];
			}
	
		}
	
	//		Rx_index3 =0;
		for(i=0; i<RxBufferSize; i++)
			 RxBuffer3_tmp[i] = 0;
	}//清除中斷旗標


												 
	DMA_Receive(COM3, RxBuffer3_tmp, 1);
	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
}
