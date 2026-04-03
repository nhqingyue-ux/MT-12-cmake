/**
 * rs485_echo.c — Minimal RS485 echo test for MT-12 board (STM32F407ZG)
 *
 * Register-level only — no StdPeriph library dependency.
 *
 * Hardware:
 *   - USART1: PA9=TX(AF7), PA10=RX(AF7)
 *   - RS485 RTS: PA12 (push-pull output, LOW=receive, HIGH=transmit)
 *   - LED green: PG2, LED red: PG5
 *   - Baud: 38400, 8N1
 *   - System clock: 168 MHz (HSE 8MHz → PLL, configured by SystemInit)
 *
 * Behaviour:
 *   On receiving a complete frame (detected by 7ms silence after last byte),
 *   echo the exact received bytes back on RS485, then return to receive mode.
 *   Toggle GREEN LED on each valid frame. RED LED toggles on overrun errors.
 */

#include <stdint.h>

/* ── SystemInit from system_stm32f4xx.c ──────────────────────────────────── */
extern void SystemInit(void);

/* ══════════════════════════════════════════════════════════════════════════════
 * Register definitions (STM32F407)
 * ══════════════════════════════════════════════════════════════════════════════ */

/* --- RCC ------------------------------------------------------------------ */
#define RCC_BASE        0x40023800U
#define RCC_AHB1ENR     (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR     (*(volatile uint32_t *)(RCC_BASE + 0x44))

/* --- GPIOA (AHB1, bit 0) ------------------------------------------------- */
#define GPIOA_BASE      0x40020000U
#define GPIOA_MODER     (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPER    (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPEEDR   (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPDR     (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_ODR       (*(volatile uint32_t *)(GPIOA_BASE + 0x14))
#define GPIOA_BSRR      (*(volatile uint32_t *)(GPIOA_BASE + 0x18))
#define GPIOA_AFRH      (*(volatile uint32_t *)(GPIOA_BASE + 0x24))

/* --- GPIOG (AHB1, bit 6) ------------------------------------------------- */
#define GPIOG_BASE      0x40021800U
#define GPIOG_MODER     (*(volatile uint32_t *)(GPIOG_BASE + 0x00))
#define GPIOG_OTYPER    (*(volatile uint32_t *)(GPIOG_BASE + 0x04))
#define GPIOG_OSPEEDR   (*(volatile uint32_t *)(GPIOG_BASE + 0x08))
#define GPIOG_ODR       (*(volatile uint32_t *)(GPIOG_BASE + 0x14))
#define GPIOG_BSRR      (*(volatile uint32_t *)(GPIOG_BASE + 0x18))

/* --- USART1 (APB2, bit 4) ------------------------------------------------ */
#define USART1_BASE     0x40011000U
#define USART1_SR       (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR       (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR      (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1      (*(volatile uint32_t *)(USART1_BASE + 0x0C))
#define USART1_CR2      (*(volatile uint32_t *)(USART1_BASE + 0x10))
#define USART1_CR3      (*(volatile uint32_t *)(USART1_BASE + 0x14))

/* USART1 SR bits */
#define USART_SR_RXNE   (1U << 5)
#define USART_SR_TC     (1U << 6)
#define USART_SR_TXE    (1U << 7)
#define USART_SR_ORE    (1U << 3)

/* USART1 CR1 bits */
#define USART_CR1_UE    (1U << 13)
#define USART_CR1_TE    (1U << 3)
#define USART_CR1_RE    (1U << 2)
#define USART_CR1_RXNEIE (1U << 5)

/* --- NVIC ----------------------------------------------------------------- */
#define NVIC_ISER1      (*(volatile uint32_t *)0xE000E104U)   /* IRQ 32-63 */

/* --- SysTick -------------------------------------------------------------- */
#define SYSTICK_CTRL    (*(volatile uint32_t *)0xE000E010U)
#define SYSTICK_LOAD    (*(volatile uint32_t *)0xE000E014U)
#define SYSTICK_VAL     (*(volatile uint32_t *)0xE000E018U)

/* ══════════════════════════════════════════════════════════════════════════════
 * Globals
 * ══════════════════════════════════════════════════════════════════════════════ */

#define RX_BUF_SIZE 512
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_idx = 0;
static volatile uint32_t tick_ms = 0;
static volatile uint32_t last_rx_tick = 0;
static volatile uint8_t  frame_ready = 0;

/* ── RTS control macros ──────────────────────────────────────────────────── */
#define RTS_HIGH()  (GPIOA_BSRR = (1U << 12))         /* transmit mode */
#define RTS_LOW()   (GPIOA_BSRR = (1U << (12 + 16)))  /* receive mode  */

/* ── LED macros ───────────────────────────────────────────────────────────── */
#define LED_GRN_TOGGLE()  (GPIOG_ODR ^= (1U << 2))
#define LED_RED_TOGGLE()  (GPIOG_ODR ^= (1U << 5))

/* ══════════════════════════════════════════════════════════════════════════════
 * Interrupt handlers
 * ══════════════════════════════════════════════════════════════════════════════ */

void SysTick_Handler(void)
{
    tick_ms++;

    /* Check for frame completion: >7ms since last received byte */
    if (rx_idx > 0 && !frame_ready)
    {
        if ((tick_ms - last_rx_tick) >= 7)
            frame_ready = 1;
    }
}

void USART1_IRQHandler(void)
{
    uint32_t sr = USART1_SR;

    /* Overrun error — read SR then DR to clear */
    if (sr & USART_SR_ORE)
    {
        (void)USART1_DR;
        LED_RED_TOGGLE();
    }

    /* RXNE — received a byte */
    if (sr & USART_SR_RXNE)
    {
        uint8_t byte = (uint8_t)USART1_DR;
        if (rx_idx < RX_BUF_SIZE)
            rx_buf[rx_idx++] = byte;
        last_rx_tick = tick_ms;
    }
}

/* ══════════════════════════════════════════════════════════════════════════════
 * Init functions
 * ══════════════════════════════════════════════════════════════════════════════ */

static void gpio_init(void)
{
    /* Enable GPIOA + GPIOG clocks */
    RCC_AHB1ENR |= (1U << 0) | (1U << 6);

    /* ── PA9  = USART1_TX (AF7, push-pull, high-speed) ────────────────────── */
    GPIOA_MODER  = (GPIOA_MODER  & ~(3U << (9*2)))  | (2U << (9*2));   /* AF mode */
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~(3U << (9*2))) | (3U << (9*2));  /* high speed */
    GPIOA_OTYPER &= ~(1U << 9);                                         /* push-pull */
    GPIOA_AFRH   = (GPIOA_AFRH & ~(0xFU << ((9-8)*4))) | (7U << ((9-8)*4)); /* AF7 */

    /* ── PA10 = USART1_RX (AF7, input with pull-up) ──────────────────────── */
    GPIOA_MODER  = (GPIOA_MODER  & ~(3U << (10*2))) | (2U << (10*2));  /* AF mode */
    GPIOA_PUPDR  = (GPIOA_PUPDR  & ~(3U << (10*2))) | (1U << (10*2));  /* pull-up */
    GPIOA_AFRH   = (GPIOA_AFRH & ~(0xFU << ((10-8)*4))) | (7U << ((10-8)*4)); /* AF7 */

    /* ── PA12 = RS485 RTS (general output, push-pull, start LOW = receive) ── */
    GPIOA_MODER  = (GPIOA_MODER  & ~(3U << (12*2))) | (1U << (12*2));  /* output */
    GPIOA_OTYPER &= ~(1U << 12);                                        /* push-pull */
    GPIOA_OSPEEDR = (GPIOA_OSPEEDR & ~(3U << (12*2))) | (3U << (12*2));/* high speed */
    RTS_LOW();

    /* ── PG2 = GREEN LED, PG5 = RED LED (general output, push-pull) ──────── */
    GPIOG_MODER  = (GPIOG_MODER  & ~(3U << (2*2))) | (1U << (2*2));
    GPIOG_MODER  = (GPIOG_MODER  & ~(3U << (5*2))) | (1U << (5*2));
    GPIOG_OTYPER &= ~((1U << 2) | (1U << 5));
}

static void usart1_init(void)
{
    /* Enable USART1 clock (APB2) */
    RCC_APB2ENR |= (1U << 4);

    /* Disable USART1 for configuration */
    USART1_CR1 = 0;

    /*
     * Baud rate calculation:
     *   APB2 clock = 84 MHz (HCLK/2 when AHB prescaler = 1 on APB2)
     *   BRR = fAPB2 / baud = 84000000 / 38400 = 2187.5
     *   Mantissa = 2187 = 0x88B >> 4? No:
     *     Mantissa = 2187, Fraction = 0.5 × 16 = 8
     *     BRR = (2187 << 4) | 8 = 0x88B8
     *
     *   Actually: USART on APB2 at 84MHz.
     *   USARTDIV = 84000000 / (16 * 38400) = 136.71875
     *   Mantissa = 136 = 0x88, Fraction = 0.71875 * 16 = 11.5 ≈ 11 = 0xB
     *   BRR = (136 << 4) | 11 = 0x88B
     */
    USART1_BRR = 0x088B;

    /* 8N1, no flow control */
    USART1_CR2 = 0;
    USART1_CR3 = 0;

    /* Enable UE + TE + RE + RXNEIE */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    /* Enable USART1 IRQ in NVIC (IRQ 37 → ISER1 bit 5) */
    NVIC_ISER1 |= (1U << 5);
}

static void systick_init(void)
{
    /* SysTick at 1ms: AHB clock = 168 MHz, reload = 168000 - 1 */
    SYSTICK_LOAD = 168000U - 1U;
    SYSTICK_VAL  = 0;
    SYSTICK_CTRL = (1U << 2)  /* clock source = AHB */
                 | (1U << 1)  /* interrupt enable */
                 | (1U << 0); /* enable */
}

/* ══════════════════════════════════════════════════════════════════════════════
 * RS485 echo: transmit buffered frame
 * ══════════════════════════════════════════════════════════════════════════════ */

static void rs485_send(const volatile uint8_t *data, uint16_t len)
{
    /* Switch to transmit mode */
    RTS_HIGH();

    /* Small delay to let transceiver switch (~1µs is enough, but be safe) */
    for (volatile int i = 0; i < 100; i++) ;

    for (uint16_t i = 0; i < len; i++)
    {
        /* Wait for TXE */
        while (!(USART1_SR & USART_SR_TXE)) ;
        USART1_DR = data[i];
    }

    /* Wait for transmission complete */
    while (!(USART1_SR & USART_SR_TC)) ;

    /* Switch back to receive mode */
    RTS_LOW();
}

/* ══════════════════════════════════════════════════════════════════════════════
 * main
 * ══════════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    SystemInit();
    gpio_init();
    usart1_init();
    systick_init();

    /* Initial LED state: both off */
    GPIOG_BSRR = (1U << (2 + 16)) | (1U << (5 + 16));  /* reset PG2, PG5 */

    while (1)
    {
        if (frame_ready)
        {
            uint16_t len = rx_idx;

            /* Echo the received frame */
            rs485_send(rx_buf, len);

            /* Toggle green LED */
            LED_GRN_TOGGLE();

            /* Reset for next frame */
            rx_idx = 0;
            frame_ready = 0;
        }

        /* Low-power wait for interrupt */
        __asm volatile ("wfi");
    }
}
