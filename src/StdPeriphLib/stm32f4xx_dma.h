/**
 * stm32f4xx_dma.h — minimal SPL DMA stub for M4-NEW-MX1 project
 * Only declares the functions actually used by stm32f4xx_it.c
 */
#ifndef __STM32F4xx_DMA_H
#define __STM32F4xx_DMA_H

#include "stm32f4xx.h"

/* ── DMA IT flags (HISR/LISR bit positions as used by SPL) ─────────────── */
#define DMA_IT_TCIF0   ((uint32_t)0x00000020)
#define DMA_IT_TCIF1   ((uint32_t)0x00000800)
#define DMA_IT_TCIF2   ((uint32_t)0x00200000)
#define DMA_IT_TCIF3   ((uint32_t)0x08000000)
#define DMA_IT_TCIF4   ((uint32_t)0x10000020)
#define DMA_IT_TCIF5   ((uint32_t)0x10000800)
#define DMA_IT_TCIF6   ((uint32_t)0x10200000)
#define DMA_IT_TCIF7   ((uint32_t)0x18000000)

/* ── Function prototypes ────────────────────────────────────────────────── */
FlagStatus DMA_GetITStatus(DMA_Stream_TypeDef *DMAy_Streamx, uint32_t DMA_IT);
void       DMA_ClearITPendingBit(DMA_Stream_TypeDef *DMAy_Streamx, uint32_t DMA_IT);
uint16_t   DMA_GetCurrDataCounter(DMA_Stream_TypeDef *DMAy_Streamx);

#endif /* __STM32F4xx_DMA_H */
