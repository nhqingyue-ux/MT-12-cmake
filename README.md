# MT-12 CMake (GCC + Ninja)

This directory provides a complete GCC/CMake build for the MT-12 STM32F407 firmware,
while keeping all original source files in the parent project directory.

## Layout

```
dev/MT-12-cmake/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/
│   └── arm-none-eabi.cmake
├── startup_stm32f407xx.s
├── STM32F407ZG_FLASH.ld
├── syscalls.c
├── app/
│   └── main.c -> ../../../main.c
└── test/
    └── rs485_echo.c
```

## Important design choices

- **No source duplication**: main firmware sources are referenced from `../../`.
- **Bootloader-compatible flash layout**:
  - App FLASH origin: `0x08008000`
  - App FLASH length: `0x000F8000` (992 KB)
- **RAM layout** from scatter:
  - RAM origin: `0x20000000`
  - RAM length: `0x0001FFF0`
  - `_estack = 0x2001FFF0`
- **Optimization**: `-O1` (aligned to Keil setup)
- **MCU flags**:
  `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`
- **Compile definitions**:
  `USE_STDPERIPH_DRIVER STM32F4XX STM32F40XX STM32F407xx RS485_ENABLE __FPU_PRESENT=1 __FPU_USED=1`
- Startup vector table contains the required **board-ID marker**:
  - `WWDG_IRQHandler` vector entry is literal `.word 6`.

## Build targets

Two targets are defined:

1. **`M4Temp.elf`**
   - Original firmware
   - Uses parent sources + StdPeriphLib + `../../alpum.lib`

2. **`rs485_echo.elf`**
   - Minimal register-level RS485 echo test
   - Uses local `test/rs485_echo.c` + parent `system_stm32f4xx.c`

Both targets generate:
- `.elf`
- `.hex`
- `.bin`
- size report (via `arm-none-eabi-size`)

## Build with presets

From this directory (`dev/MT-12-cmake`):

```bash
cmake --preset default
cmake --build --preset main
cmake --build --preset rs485-echo
```

Or build everything:

```bash
cmake --build --preset all
```

Build outputs are in:

```
build/default/
```

## RS485 echo test behavior

`test/rs485_echo.c` implements:

- Clock setup to 168 MHz via HSE PLL
- USART1 @ 38400 8N1, RX interrupt enabled
- RS485 half-duplex direction pin on PA12
  - LOW = receive
  - HIGH = transmit
- Frame boundary detection by **7 ms silence** (SysTick)
- Echoes exact received bytes after frame end
- Toggles PG2 + PG5 on each frame
- Pure register-level peripheral access (no StdPeriph calls)

## Notes

- `app/main.c` is a symlink for convenience/documentation; actual build uses
  source paths from `../../`.
- Toolchain is configured via `cmake/arm-none-eabi.cmake` and selected in
  `CMakePresets.json`.
