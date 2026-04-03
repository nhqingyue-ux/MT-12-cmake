/**
 * stm32f4xx_dma.c — minimal SPL DMA implementation
 * Wraps direct register access for the subset used by this project.
 */
#include "stm32f4xx_dma.h"

/**
 * @brief  Check DMA transfer-complete IT flag
 *         Bit mapping matches SPL convention:
 *         bit[28] = 1 → use HISR, else LISR
 *         Remaining bits select the stream's TCIFx bit position.
 */
FlagStatus DMA_GetITStatus(DMA_Stream_TypeDef *DMAy_Streamx, uint32_t DMA_IT)
{
    DMA_TypeDef *dma;
    uint32_t   hiflag;
    uint32_t   tmpreg;

    /* Determine DMA controller */
    if (DMAy_Streamx < DMA2_Stream0)
        dma = DMA1;
    else
        dma = DMA2;

    hiflag = DMA_IT & 0x10000000u;
    uint32_t bit = DMA_IT & 0x0FFFFFFFu;

    if (hiflag)
        tmpreg = dma->HISR;
    else
        tmpreg = dma->LISR;

    return (tmpreg & bit) ? SET : RESET;
}

/**
 * @brief  Clear DMA transfer-complete IT pending bit
 */
void DMA_ClearITPendingBit(DMA_Stream_TypeDef *DMAy_Streamx, uint32_t DMA_IT)
{
    DMA_TypeDef *dma;
    uint32_t   hiflag;

    if (DMAy_Streamx < DMA2_Stream0)
        dma = DMA1;
    else
        dma = DMA2;

    hiflag = DMA_IT & 0x10000000u;
    uint32_t bit = DMA_IT & 0x0FFFFFFFu;

    if (hiflag)
        dma->HIFCR = bit;
    else
        dma->LIFCR = bit;
}

/**
 * @brief  Return current DMA NDTR (number of data items remaining)
 */
uint16_t DMA_GetCurrDataCounter(DMA_Stream_TypeDef *DMAy_Streamx)
{
    return (uint16_t)(DMAy_Streamx->NDTR);
}
