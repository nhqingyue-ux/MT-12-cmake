/* stm32f4xx_it.h — interrupt handler declarations */
#ifndef __STM32F4xx_IT_H
#define __STM32F4xx_IT_H

#include "stm32f4xx.h"
#include "stm32f4xx_dma.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── COM port identifiers (project-specific, originally in MDK library) ─── */
#define COM1  0
#define COM2  1
#define COM3  2

/* ── DMA receive wrapper — implemented in DataInit.c or RS485.c ─────────── */
extern void DMA_Receive(uint8_t com, uint8_t *buf, uint16_t len);

/* ── Shared RS485 state ─────────────────────────────────────────────────── */
/* Rx_index defined in stm32f4xx_it.c as uint16_t, Receive_Size as int */

/* ── Exception handlers ─────────────────────────────────────────────────── */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4xx_IT_H */
