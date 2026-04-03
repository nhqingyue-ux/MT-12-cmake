/**
 * startup_stm32f407xx.s  —  GCC GAS startup for STM32F407
 *
 * Board: MT-12 temperature controller
 *
 * Special note:
 *   The WWDG vector entry (IRQ #0, position 16 in the table) is set to the
 *   literal value 6 instead of a handler address. This is intentional — it
 *   acts as a board-ID / firmware-version tag read by the bootloader.
 *   Do NOT change this to a real handler address.
 *
 * Cortex-M4 with FPU (fpv4-sp-d16, hard-float ABI)
 */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

/* ── Linker-script symbols ───────────────────────────────────────────────── */
.global g_pfnVectors
.global Default_Handler

.word _sidata   /* LMA of .data initializers in Flash                        */
.word _sdata    /* VMA start of .data in RAM                                 */
.word _edata    /* VMA end   of .data in RAM                                 */
.word _sbss     /* start of .bss                                              */
.word _ebss     /* end   of .bss                                              */

/* ── Reset_Handler ──────────────────────────────────────────────────────── */
    .section .text.Reset_Handler,"ax",%progbits
    .weak   Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    ldr   sp, =_estack          /* set initial stack pointer                  */

    /* --- copy .data from Flash to RAM --------------------------------------- */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4

LoopCopyDataInit:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   CopyDataInit

    /* --- zero .bss --------------------------------------------------------- */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     LoopFillZerobss

FillZerobss:
    str   r3, [r2]
    adds  r2, r2, #4

LoopFillZerobss:
    cmp   r2, r4
    bcc   FillZerobss

    /* --- chip-level init --------------------------------------------------- */
    bl    SystemInit

    /* --- C++ / init_array (also covers C constructors) --------------------- */
    bl    __libc_init_array

    /* --- application entry point ------------------------------------------- */
    bl    main

    /* should never reach here; spin if it does */
    b     .

    .size Reset_Handler, .-Reset_Handler

/* ── Default_Handler (infinite loop) ───────────────────────────────────── */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b     Infinite_Loop
    .size Default_Handler, .-Default_Handler

/* ══════════════════════════════════════════════════════════════════════════
 * Vector table
 * Must be placed at the start of Flash (handled by .isr_vector in ld script).
 *
 * Offset  0 : Initial stack pointer
 * Offset  4 : Reset_Handler
 * Offsets 8–3B : Core exceptions
 * Offset 40+ : STM32F407 device IRQs
 *
 * !! WWDG slot (first device IRQ, table[16]) = literal 6 = board ID !!
 * ══════════════════════════════════════════════════════════════════════════ */
    .section .isr_vector,"a",%progbits
    .type    g_pfnVectors, %object
    .align   2

g_pfnVectors:
    /* ── ARM Cortex-M4 core vectors ──────────────────────────────────────── */
    .word  _estack                          /* 0x000 Initial SP              */
    .word  Reset_Handler                    /* 0x004 Reset                   */
    .word  NMI_Handler                      /* 0x008 NMI                     */
    .word  HardFault_Handler                /* 0x00C HardFault               */
    .word  MemManage_Handler                /* 0x010 MemManage               */
    .word  BusFault_Handler                 /* 0x014 BusFault                */
    .word  UsageFault_Handler               /* 0x018 UsageFault              */
    .word  0                                /* 0x01C Reserved                */
    .word  0                                /* 0x020 Reserved                */
    .word  0                                /* 0x024 Reserved                */
    .word  0                                /* 0x028 Reserved                */
    .word  SVC_Handler                      /* 0x02C SVCall                  */
    .word  DebugMon_Handler                 /* 0x030 DebugMonitor            */
    .word  0                                /* 0x034 Reserved                */
    .word  PendSV_Handler                   /* 0x038 PendSV                  */
    .word  SysTick_Handler                  /* 0x03C SysTick                 */

    /* ── STM32F407 device IRQs (IRQ0 = offset 0x040) ─────────────────────── */
    .word  6                                /* IRQ 0  WWDG  ← BOARD-ID = 6   */
    .word  PVD_IRQHandler                   /* IRQ 1  PVD                    */
    .word  TAMP_STAMP_IRQHandler            /* IRQ 2  Tamper/Timestamp       */
    .word  RTC_WKUP_IRQHandler              /* IRQ 3  RTC Wakeup             */
    .word  FLASH_IRQHandler                 /* IRQ 4  Flash                  */
    .word  RCC_IRQHandler                   /* IRQ 5  RCC                    */
    .word  EXTI0_IRQHandler                 /* IRQ 6  EXTI0                  */
    .word  EXTI1_IRQHandler                 /* IRQ 7  EXTI1                  */
    .word  EXTI2_IRQHandler                 /* IRQ 8  EXTI2                  */
    .word  EXTI3_IRQHandler                 /* IRQ 9  EXTI3                  */
    .word  EXTI4_IRQHandler                 /* IRQ 10 EXTI4                  */
    .word  DMA1_Stream0_IRQHandler          /* IRQ 11 DMA1 Stream0           */
    .word  DMA1_Stream1_IRQHandler          /* IRQ 12 DMA1 Stream1           */
    .word  DMA1_Stream2_IRQHandler          /* IRQ 13 DMA1 Stream2           */
    .word  DMA1_Stream3_IRQHandler          /* IRQ 14 DMA1 Stream3           */
    .word  DMA1_Stream4_IRQHandler          /* IRQ 15 DMA1 Stream4           */
    .word  DMA1_Stream5_IRQHandler          /* IRQ 16 DMA1 Stream5           */
    .word  DMA1_Stream6_IRQHandler          /* IRQ 17 DMA1 Stream6           */
    .word  ADC_IRQHandler                   /* IRQ 18 ADC1/2/3               */
    .word  CAN1_TX_IRQHandler               /* IRQ 19 CAN1 TX                */
    .word  CAN1_RX0_IRQHandler              /* IRQ 20 CAN1 RX0               */
    .word  CAN1_RX1_IRQHandler              /* IRQ 21 CAN1 RX1               */
    .word  CAN1_SCE_IRQHandler              /* IRQ 22 CAN1 SCE               */
    .word  EXTI9_5_IRQHandler               /* IRQ 23 EXTI[9:5]              */
    .word  TIM1_BRK_TIM9_IRQHandler         /* IRQ 24 TIM1 Break / TIM9      */
    .word  TIM1_UP_TIM10_IRQHandler         /* IRQ 25 TIM1 Update / TIM10    */
    .word  TIM1_TRG_COM_TIM11_IRQHandler    /* IRQ 26 TIM1 Trig+COM / TIM11  */
    .word  TIM1_CC_IRQHandler               /* IRQ 27 TIM1 CC                */
    .word  TIM2_IRQHandler                  /* IRQ 28 TIM2  (ISR_Timer0)     */
    .word  TIM3_IRQHandler                  /* IRQ 29 TIM3                   */
    .word  TIM4_IRQHandler                  /* IRQ 30 TIM4                   */
    .word  I2C1_EV_IRQHandler               /* IRQ 31 I2C1 Event             */
    .word  I2C1_ER_IRQHandler               /* IRQ 32 I2C1 Error             */
    .word  I2C2_EV_IRQHandler               /* IRQ 33 I2C2 Event             */
    .word  I2C2_ER_IRQHandler               /* IRQ 34 I2C2 Error             */
    .word  SPI1_IRQHandler                  /* IRQ 35 SPI1                   */
    .word  SPI2_IRQHandler                  /* IRQ 36 SPI2                   */
    .word  USART1_IRQHandler                /* IRQ 37 USART1                 */
    .word  USART2_IRQHandler                /* IRQ 38 USART2                 */
    .word  USART3_IRQHandler                /* IRQ 39 USART3                 */
    .word  EXTI15_10_IRQHandler             /* IRQ 40 EXTI[15:10]            */
    .word  RTC_Alarm_IRQHandler             /* IRQ 41 RTC Alarm A/B          */
    .word  OTG_FS_WKUP_IRQHandler           /* IRQ 42 USB OTG FS Wakeup      */
    .word  TIM8_BRK_TIM12_IRQHandler        /* IRQ 43 TIM8 Break / TIM12     */
    .word  TIM8_UP_TIM13_IRQHandler         /* IRQ 44 TIM8 Update / TIM13    */
    .word  TIM8_TRG_COM_TIM14_IRQHandler    /* IRQ 45 TIM8 Trig+COM / TIM14  */
    .word  TIM8_CC_IRQHandler               /* IRQ 46 TIM8 CC                */
    .word  DMA1_Stream7_IRQHandler          /* IRQ 47 DMA1 Stream7           */
    .word  FSMC_IRQHandler                  /* IRQ 48 FSMC                   */
    .word  SDIO_IRQHandler                  /* IRQ 49 SDIO                   */
    .word  TIM5_IRQHandler                  /* IRQ 50 TIM5                   */
    .word  SPI3_IRQHandler                  /* IRQ 51 SPI3                   */
    .word  UART4_IRQHandler                 /* IRQ 52 UART4                  */
    .word  UART5_IRQHandler                 /* IRQ 53 UART5                  */
    .word  TIM6_DAC_IRQHandler              /* IRQ 54 TIM6 / DAC underrun    */
    .word  TIM7_IRQHandler                  /* IRQ 55 TIM7                   */
    .word  DMA2_Stream0_IRQHandler          /* IRQ 56 DMA2 Stream0           */
    .word  DMA2_Stream1_IRQHandler          /* IRQ 57 DMA2 Stream1           */
    .word  DMA2_Stream2_IRQHandler          /* IRQ 58 DMA2 Stream2           */
    .word  DMA2_Stream3_IRQHandler          /* IRQ 59 DMA2 Stream3           */
    .word  DMA2_Stream4_IRQHandler          /* IRQ 60 DMA2 Stream4           */
    .word  ETH_IRQHandler                   /* IRQ 61 Ethernet               */
    .word  ETH_WKUP_IRQHandler              /* IRQ 62 Ethernet Wakeup        */
    .word  CAN2_TX_IRQHandler               /* IRQ 63 CAN2 TX                */
    .word  CAN2_RX0_IRQHandler              /* IRQ 64 CAN2 RX0               */
    .word  CAN2_RX1_IRQHandler              /* IRQ 65 CAN2 RX1               */
    .word  CAN2_SCE_IRQHandler              /* IRQ 66 CAN2 SCE               */
    .word  OTG_FS_IRQHandler                /* IRQ 67 USB OTG FS             */
    .word  DMA2_Stream5_IRQHandler          /* IRQ 68 DMA2 Stream5           */
    .word  DMA2_Stream6_IRQHandler          /* IRQ 69 DMA2 Stream6           */
    .word  DMA2_Stream7_IRQHandler          /* IRQ 70 DMA2 Stream7           */
    .word  USART6_IRQHandler                /* IRQ 71 USART6                 */
    .word  I2C3_EV_IRQHandler               /* IRQ 72 I2C3 Event             */
    .word  I2C3_ER_IRQHandler               /* IRQ 73 I2C3 Error             */
    .word  OTG_HS_EP1_OUT_IRQHandler        /* IRQ 74 USB OTG HS EP1 OUT     */
    .word  OTG_HS_EP1_IN_IRQHandler         /* IRQ 75 USB OTG HS EP1 IN      */
    .word  OTG_HS_WKUP_IRQHandler           /* IRQ 76 USB OTG HS Wakeup      */
    .word  OTG_HS_IRQHandler                /* IRQ 77 USB OTG HS             */
    .word  DCMI_IRQHandler                  /* IRQ 78 DCMI                   */
    .word  0                                /* IRQ 79 CRYP (not on F407)     */
    .word  HASH_RNG_IRQHandler              /* IRQ 80 Hash / RNG             */
    .word  FPU_IRQHandler                   /* IRQ 81 FPU                    */

    .size g_pfnVectors, .-g_pfnVectors

/* ══════════════════════════════════════════════════════════════════════════
 * Weak aliases — any C file that defines one of these names wins.
 * ══════════════════════════════════════════════════════════════════════════ */

    .weak      NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak      HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler

    .weak      MemManage_Handler
    .thumb_set MemManage_Handler,Default_Handler

    .weak      BusFault_Handler
    .thumb_set BusFault_Handler,Default_Handler

    .weak      UsageFault_Handler
    .thumb_set UsageFault_Handler,Default_Handler

    .weak      SVC_Handler
    .thumb_set SVC_Handler,Default_Handler

    .weak      DebugMon_Handler
    .thumb_set DebugMon_Handler,Default_Handler

    .weak      PendSV_Handler
    .thumb_set PendSV_Handler,Default_Handler

    .weak      SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler

    /* NOTE: WWDG_IRQHandler is NOT listed here — its vector is the literal
     * value 6 (board ID), not a function pointer.                            */

    .weak      PVD_IRQHandler
    .thumb_set PVD_IRQHandler,Default_Handler

    .weak      TAMP_STAMP_IRQHandler
    .thumb_set TAMP_STAMP_IRQHandler,Default_Handler

    .weak      RTC_WKUP_IRQHandler
    .thumb_set RTC_WKUP_IRQHandler,Default_Handler

    .weak      FLASH_IRQHandler
    .thumb_set FLASH_IRQHandler,Default_Handler

    .weak      RCC_IRQHandler
    .thumb_set RCC_IRQHandler,Default_Handler

    .weak      EXTI0_IRQHandler
    .thumb_set EXTI0_IRQHandler,Default_Handler

    .weak      EXTI1_IRQHandler
    .thumb_set EXTI1_IRQHandler,Default_Handler

    .weak      EXTI2_IRQHandler
    .thumb_set EXTI2_IRQHandler,Default_Handler

    .weak      EXTI3_IRQHandler
    .thumb_set EXTI3_IRQHandler,Default_Handler

    .weak      EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler,Default_Handler

    .weak      DMA1_Stream0_IRQHandler
    .thumb_set DMA1_Stream0_IRQHandler,Default_Handler

    .weak      DMA1_Stream1_IRQHandler
    .thumb_set DMA1_Stream1_IRQHandler,Default_Handler

    .weak      DMA1_Stream2_IRQHandler
    .thumb_set DMA1_Stream2_IRQHandler,Default_Handler

    .weak      DMA1_Stream3_IRQHandler
    .thumb_set DMA1_Stream3_IRQHandler,Default_Handler

    .weak      DMA1_Stream4_IRQHandler
    .thumb_set DMA1_Stream4_IRQHandler,Default_Handler

    .weak      DMA1_Stream5_IRQHandler
    .thumb_set DMA1_Stream5_IRQHandler,Default_Handler

    .weak      DMA1_Stream6_IRQHandler
    .thumb_set DMA1_Stream6_IRQHandler,Default_Handler

    .weak      ADC_IRQHandler
    .thumb_set ADC_IRQHandler,Default_Handler

    .weak      CAN1_TX_IRQHandler
    .thumb_set CAN1_TX_IRQHandler,Default_Handler

    .weak      CAN1_RX0_IRQHandler
    .thumb_set CAN1_RX0_IRQHandler,Default_Handler

    .weak      CAN1_RX1_IRQHandler
    .thumb_set CAN1_RX1_IRQHandler,Default_Handler

    .weak      CAN1_SCE_IRQHandler
    .thumb_set CAN1_SCE_IRQHandler,Default_Handler

    .weak      EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler,Default_Handler

    .weak      TIM1_BRK_TIM9_IRQHandler
    .thumb_set TIM1_BRK_TIM9_IRQHandler,Default_Handler

    .weak      TIM1_UP_TIM10_IRQHandler
    .thumb_set TIM1_UP_TIM10_IRQHandler,Default_Handler

    .weak      TIM1_TRG_COM_TIM11_IRQHandler
    .thumb_set TIM1_TRG_COM_TIM11_IRQHandler,Default_Handler

    .weak      TIM1_CC_IRQHandler
    .thumb_set TIM1_CC_IRQHandler,Default_Handler

    .weak      TIM2_IRQHandler
    .thumb_set TIM2_IRQHandler,Default_Handler

    .weak      TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler,Default_Handler

    .weak      TIM4_IRQHandler
    .thumb_set TIM4_IRQHandler,Default_Handler

    .weak      I2C1_EV_IRQHandler
    .thumb_set I2C1_EV_IRQHandler,Default_Handler

    .weak      I2C1_ER_IRQHandler
    .thumb_set I2C1_ER_IRQHandler,Default_Handler

    .weak      I2C2_EV_IRQHandler
    .thumb_set I2C2_EV_IRQHandler,Default_Handler

    .weak      I2C2_ER_IRQHandler
    .thumb_set I2C2_ER_IRQHandler,Default_Handler

    .weak      SPI1_IRQHandler
    .thumb_set SPI1_IRQHandler,Default_Handler

    .weak      SPI2_IRQHandler
    .thumb_set SPI2_IRQHandler,Default_Handler

    .weak      USART1_IRQHandler
    .thumb_set USART1_IRQHandler,Default_Handler

    .weak      USART2_IRQHandler
    .thumb_set USART2_IRQHandler,Default_Handler

    .weak      USART3_IRQHandler
    .thumb_set USART3_IRQHandler,Default_Handler

    .weak      EXTI15_10_IRQHandler
    .thumb_set EXTI15_10_IRQHandler,Default_Handler

    .weak      RTC_Alarm_IRQHandler
    .thumb_set RTC_Alarm_IRQHandler,Default_Handler

    .weak      OTG_FS_WKUP_IRQHandler
    .thumb_set OTG_FS_WKUP_IRQHandler,Default_Handler

    .weak      TIM8_BRK_TIM12_IRQHandler
    .thumb_set TIM8_BRK_TIM12_IRQHandler,Default_Handler

    .weak      TIM8_UP_TIM13_IRQHandler
    .thumb_set TIM8_UP_TIM13_IRQHandler,Default_Handler

    .weak      TIM8_TRG_COM_TIM14_IRQHandler
    .thumb_set TIM8_TRG_COM_TIM14_IRQHandler,Default_Handler

    .weak      TIM8_CC_IRQHandler
    .thumb_set TIM8_CC_IRQHandler,Default_Handler

    .weak      DMA1_Stream7_IRQHandler
    .thumb_set DMA1_Stream7_IRQHandler,Default_Handler

    .weak      FSMC_IRQHandler
    .thumb_set FSMC_IRQHandler,Default_Handler

    .weak      SDIO_IRQHandler
    .thumb_set SDIO_IRQHandler,Default_Handler

    .weak      TIM5_IRQHandler
    .thumb_set TIM5_IRQHandler,Default_Handler

    .weak      SPI3_IRQHandler
    .thumb_set SPI3_IRQHandler,Default_Handler

    .weak      UART4_IRQHandler
    .thumb_set UART4_IRQHandler,Default_Handler

    .weak      UART5_IRQHandler
    .thumb_set UART5_IRQHandler,Default_Handler

    .weak      TIM6_DAC_IRQHandler
    .thumb_set TIM6_DAC_IRQHandler,Default_Handler

    .weak      TIM7_IRQHandler
    .thumb_set TIM7_IRQHandler,Default_Handler

    .weak      DMA2_Stream0_IRQHandler
    .thumb_set DMA2_Stream0_IRQHandler,Default_Handler

    .weak      DMA2_Stream1_IRQHandler
    .thumb_set DMA2_Stream1_IRQHandler,Default_Handler

    .weak      DMA2_Stream2_IRQHandler
    .thumb_set DMA2_Stream2_IRQHandler,Default_Handler

    .weak      DMA2_Stream3_IRQHandler
    .thumb_set DMA2_Stream3_IRQHandler,Default_Handler

    .weak      DMA2_Stream4_IRQHandler
    .thumb_set DMA2_Stream4_IRQHandler,Default_Handler

    .weak      ETH_IRQHandler
    .thumb_set ETH_IRQHandler,Default_Handler

    .weak      ETH_WKUP_IRQHandler
    .thumb_set ETH_WKUP_IRQHandler,Default_Handler

    .weak      CAN2_TX_IRQHandler
    .thumb_set CAN2_TX_IRQHandler,Default_Handler

    .weak      CAN2_RX0_IRQHandler
    .thumb_set CAN2_RX0_IRQHandler,Default_Handler

    .weak      CAN2_RX1_IRQHandler
    .thumb_set CAN2_RX1_IRQHandler,Default_Handler

    .weak      CAN2_SCE_IRQHandler
    .thumb_set CAN2_SCE_IRQHandler,Default_Handler

    .weak      OTG_FS_IRQHandler
    .thumb_set OTG_FS_IRQHandler,Default_Handler

    .weak      DMA2_Stream5_IRQHandler
    .thumb_set DMA2_Stream5_IRQHandler,Default_Handler

    .weak      DMA2_Stream6_IRQHandler
    .thumb_set DMA2_Stream6_IRQHandler,Default_Handler

    .weak      DMA2_Stream7_IRQHandler
    .thumb_set DMA2_Stream7_IRQHandler,Default_Handler

    .weak      USART6_IRQHandler
    .thumb_set USART6_IRQHandler,Default_Handler

    .weak      I2C3_EV_IRQHandler
    .thumb_set I2C3_EV_IRQHandler,Default_Handler

    .weak      I2C3_ER_IRQHandler
    .thumb_set I2C3_ER_IRQHandler,Default_Handler

    .weak      OTG_HS_EP1_OUT_IRQHandler
    .thumb_set OTG_HS_EP1_OUT_IRQHandler,Default_Handler

    .weak      OTG_HS_EP1_IN_IRQHandler
    .thumb_set OTG_HS_EP1_IN_IRQHandler,Default_Handler

    .weak      OTG_HS_WKUP_IRQHandler
    .thumb_set OTG_HS_WKUP_IRQHandler,Default_Handler

    .weak      OTG_HS_IRQHandler
    .thumb_set OTG_HS_IRQHandler,Default_Handler

    .weak      DCMI_IRQHandler
    .thumb_set DCMI_IRQHandler,Default_Handler

    .weak      HASH_RNG_IRQHandler
    .thumb_set HASH_RNG_IRQHandler,Default_Handler

    .weak      FPU_IRQHandler
    .thumb_set FPU_IRQHandler,Default_Handler
