# CLAUDE.md — MT-12 Temperature Controller Firmware

## Project Overview

MT-12 is a **12-channel industrial temperature controller** (温控板) used in injection molding machines (模温机).  
MCU: **STM32F407ZG** (Cortex-M4, 1MB Flash, 192KB SRAM).  
Original toolchain: Keil MDK (ARMCC). This repo uses **CMake + Ninja + arm-none-eabi-gcc**.

## Memory Layout

```
0x08000000 ┬─────────────────────────┐
           │  Bootloader (32KB)      │  Temp_M4_Bootloader_A_v7.bin
0x08008000 ├─────────────────────────┤  ← App entry point
           │  Application (992KB)    │  M4Temp.bin
0x08100000 └─────────────────────────┘
0x20000000 ┬─────────────────────────┐
           │  SRAM (128KB)           │
0x2001FFF0 └─────────────────────────┘  ← _estack
```

- **Bootloader**: 0x08000000–0x08007FFF (32KB), binary only (no source)
- **Application**: 0x08008000–0x080FFFFF (992KB)
- **Vector table**: At 0x08008000, `WWDG` slot = literal `6` (board ID, NOT a handler)
- **Stack**: Top at 0x2001FFF0

## Build

```bash
cmake --preset default          # Configure
cmake --build --preset main     # M4Temp.elf (main firmware)
cmake --build --preset rs485-echo  # rs485_echo.elf (RS485 echo test)
cmake --build --preset all      # Both targets
```

Output: `build/default/M4Temp.{elf,bin,hex}` and `rs485_echo.{elf,bin,hex}`

## Flash

```bash
st-flash erase
st-flash write bootloader/Temp_M4_Bootloader_A_v7.bin 0x08000000
st-flash write build/default/M4Temp.bin 0x08008000
```

## Hardware

| Peripheral | Function |
|------------|----------|
| USART1 (PA9/PA10) | RS485 Modbus-like protocol, 38400 8N1 |
| PA12 | RS485 direction (RTS): LOW=RX, HIGH=TX |
| I2C1 | ALPU-M encryption IC (0x7A) + EEPROM (0x50) |
| SPI (PA4/5/7) | 3× ADS1248 temperature ADC (T_CS1/T_CS2/T_CS3) |
| TIM2 | ISR_Timer0: ADC sampling + PID + PWM output (0.2ms) |
| TIM3 | ISR_Timer1: auxiliary timing |
| TIM4 | ISR_Timer2: temperature scan |
| TIM5 | ISR_Timer3: RS485 packet processing (0.5ms) |
| PG2/PG5 | Green/Red LEDs |

## Key Source Files

| File | Purpose |
|------|---------|
| `src/main.c` | Main loop: RS485 TX, EEPROM compare, LED, Date |
| `src/EEPROM.c` | HW_config() — all GPIO/SPI/I2C/USART/TIM init |
| `src/DataInit.c` | CDMInit() — register memory (CDM) initialization |
| `src/IntTm0.c` | TIM2 ISR: temperature ADC read + PID calculation |
| `src/IntTm1.c` | TIM4 ISR: TempSubScan() |
| `src/IntTm3.c` | TIM5 ISR: RS485 packet parsing (0x30/0x33/0x60) |
| `src/RS485.c` | USART1 ISR: byte-level RS485 receive |
| `src/Temp.c` | Temperature linearization + calibration |
| `src/alpum_interface_rev1.1.c` | ALPU-M encryption IC interface |
| `src/missing_symbols.c` | GCC stubs for symbols from Keil libraries |

## RS485 Protocol (Private, NOT standard Modbus)

- Slave ID: 0x00–0x07 (DIP switch)
- `0x30`: Read multiple non-contiguous registers
- `0x60`: Write multiple non-contiguous registers
- `0xF0`: Error response (0x81=length, 0x82=data, 0x83=CRC)
- CRC16: table-based, init 0xFFFF, **CRCLo first on wire**

Key registers: see `docs/register_map.md`

## GCC Porting Notes

1. **Optimization**: Must use `-O1` (Keil default). At `-O0`, TIM2 ISR starves main loop.
2. **alpum.lib**: ELF32 ARM format — GCC links directly. Contains `_alpum_init`, `_alpum_enc`, `_alpum_process`.
3. **Startup**: GCC uses `startup_stm32f407xx.s` (GAS syntax). WWDG vector = literal 6.
4. **Syscalls**: `src/syscalls.c` provides newlib stubs (brew toolchain has no newlib).
5. **missing_symbols.c**: Stubs for `DMA_Receive`, `_alpum_process` return 0 (success).
6. **`--allow-multiple-definition`**: Required because `main.h` defines globals in header.

## Known Issues (GCC build)

- TIM2 ISR (temperature ADC + PID) runs every 0.2ms and is CPU-intensive.
  At `-O0` it takes >0.2ms → main loop never executes → RS485 TX never fires.
- RS485 RTS (PA12) must be LOW after init for receive mode. If stuck HIGH, board can't receive.
- The `_alpum_process()` stub returns 0. Real encryption requires the ALPU-M IC on I2C1.

## Documentation

- `docs/MT12_PROTOCOL.md` — RS485 protocol quick reference
- `docs/MT12_manual_V2.1.md` — Full product manual (translated from V2.1 .doc)
- `docs/register_map.md` — Complete register address map
- `docs/FLASH_SOP.md` — Flashing procedure
