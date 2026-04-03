/*
 * STM32F407ZG linker script for MT-12 temperature controller
 *
 * Converted from Keil scatter file M4Temp.sct:
 *   LR_IROM1 0x08008000 0x000F8000
 *     ER_IROM1 0x08008000 0x000F8000  (code + readonly)
 *     RW_IRAM1 0x20000000 0x0001FFF0  (read-write + zero-init)
 *
 * Flash starts at 0x08008000 (after 32KB bootloader).
 * RAM top (_estack) = 0x2001FFF0.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08008000, LENGTH = 992K   /* 0xF8000 */
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K   /* 0x1FFF0 usable */
    CCMRAM(xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}

_estack = 0x2001FFF0;
_Min_Heap_Size  = 0x400;
_Min_Stack_Size = 0x800;

SECTIONS
{
    /* ── Vector table (must be first in Flash) ────────────────────────────── */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* ── Code ─────────────────────────────────────────────────────────────── */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* ── Read-only data ───────────────────────────────────────────────────── */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ── ARM exception-unwind tables ──────────────────────────────────────── */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } >FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* ── C/C++ init/fini arrays ───────────────────────────────────────────── */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH

    /* LMA (load address) of .data initializers in Flash */
    _sidata = LOADADDR(.data);

    /* ── Initialized data (copied from Flash to RAM by startup) ───────── */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* ── BSS (zero-filled by startup) ──────────────────────────────────── */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* ── Heap + stack check ────────────────────────────────────────────── */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* ── Discard debug from .lib ──────────────────────────────────────── */
    /DISCARD/ :
    {
        *(.ARM.attributes)
    }
}
