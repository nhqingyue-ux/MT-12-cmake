/**
 * missing_symbols.c — stubs for symbols that were in Keil MDK libraries
 * or source files not committed to the original project.
 * This file allows GCC linking; implementations should be filled in
 * as the actual source is recovered.
 */
#include <stdint.h>

/* ── DMA RS485 ──────────────────────────────────────────────────────────── */
void DMA_Receive(uint8_t com, uint8_t *buf, uint16_t len)
{
    (void)com; (void)buf; (void)len;
    /* TODO: implement DMA-based USART receive setup */
}

int    Receive_Size = 8;
uint8_t RxBuffer1[256]     = {0};
uint8_t RxBuffer2[256]     = {0};
uint8_t RxBuffer3[256]     = {0};
uint8_t RxBuffer1_tmp[256] = {0};
uint8_t RxBuffer2_tmp[256] = {0};
uint8_t RxBuffer3_tmp[256] = {0};

uint32_t PrescalerValue = 0;
uint32_t TimeOut         = 0;

/* ── ALPU-M library (provided as .lib in Keil, closed source) ──────────── */
unsigned char alpum_tx_data[8]  = {0};
unsigned char alpum_rx_data[10] = {0};
unsigned char alpum_ex_data[8]  = {0};

void _alpum_process(void)
{
    /* stub — original in alpum.lib / alpum_new.lib (Keil .lib) */
}

/* ── rand seed (used by our stdlib.h stub) ──────────────────────────────── */
unsigned int __rand_seed = 1;
