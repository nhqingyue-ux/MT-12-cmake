cmake_minimum_required(VERSION 3.21)

# ── Project ───────────────────────────────────────────────────────────────────
# Toolchain is set via CMakePresets.json or -DCMAKE_TOOLCHAIN_FILE=...
project(MT12 C ASM)

# ── Paths ─────────────────────────────────────────────────────────────────────
set(SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(PERIPH_DIR  "${SRC_DIR}/StdPeriphLib")
set(CMSIS_DEV   "${SRC_DIR}/CMSIS/Device/ST/STM32F4xx/Include")
set(CMSIS_CORE  "${SRC_DIR}/CMSIS/Include")

# ── MCU compile flags (shared by all targets) ─────────────────────────────────
set(MCU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# ── Common compile options ────────────────────────────────────────────────────
add_compile_options(
    ${MCU_FLAGS}
    -O1                       # match Keil default optimisation
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
    -ffreestanding
)

add_compile_definitions(
    USE_STDPERIPH_DRIVER
    STM32F4XX
    STM32F40XX
    STM32F407xx
    RS485_ENABLE
    __FPU_PRESENT=1
    __FPU_USED=1
)

# ── Common include directories ────────────────────────────────────────────────
include_directories(
    ${SRC_DIR}
    ${PERIPH_DIR}
    ${CMSIS_DEV}
    ${CMSIS_CORE}
)

# ── Linker script & options (shared) ─────────────────────────────────────────
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F407ZG_FLASH.ld")

# Common link flags applied via target_link_options
set(COMMON_LINK_FLAGS
    ${MCU_FLAGS}
    -T${LINKER_SCRIPT}
    -nostdlib
    -Wl,--gc-sections
    -Wl,--allow-multiple-definition
)

# ═══════════════════════════════════════════════════════════════════════════════
# TARGET 1: M4Temp.elf  — main firmware
# ═══════════════════════════════════════════════════════════════════════════════

set(APP_SOURCES
    ${SRC_DIR}/main.c
    ${SRC_DIR}/DataInit.c
    ${SRC_DIR}/EEPROM.c
    ${SRC_DIR}/IntTm0.c
    ${SRC_DIR}/IntTm1.c
    ${SRC_DIR}/IntTm2.c
    ${SRC_DIR}/IntTm3.c
    ${SRC_DIR}/RS485.c
    ${SRC_DIR}/Temp.c
    ${SRC_DIR}/alpum_interface_rev1.1.c
    ${SRC_DIR}/eeprom_interface.c
    ${SRC_DIR}/stm32f4xx_it.c
    ${SRC_DIR}/system_stm32f4xx.c
    ${SRC_DIR}/syscalls.c
    ${SRC_DIR}/missing_symbols.c
)

set(PERIPH_SOURCES
    ${PERIPH_DIR}/misc.c
    ${PERIPH_DIR}/stm32f4xx_dma.c
    ${PERIPH_DIR}/stm32f4xx_exti.c
    ${PERIPH_DIR}/stm32f4xx_gpio.c
    ${PERIPH_DIR}/stm32f4xx_i2c.c
    ${PERIPH_DIR}/stm32f4xx_pwr.c
    ${PERIPH_DIR}/stm32f4xx_rcc.c
    ${PERIPH_DIR}/stm32f4xx_rtc.c
    ${PERIPH_DIR}/stm32f4xx_syscfg.c
    ${PERIPH_DIR}/stm32f4xx_tim.c
    ${PERIPH_DIR}/stm32f4xx_usart.c
)

set(ASM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f407xx.s
)

add_executable(M4Temp.elf
    ${APP_SOURCES}
    ${PERIPH_SOURCES}
    ${ASM_SOURCES}
)

target_link_options(M4Temp.elf PRIVATE
    ${COMMON_LINK_FLAGS}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/M4Temp.map,--cref
)

# Link alpum.lib (prebuilt static library from parent) and libgcc
target_link_libraries(M4Temp.elf PRIVATE
    ${SRC_DIR}/alpum.lib
    gcc
)

# Post-build: .hex + .bin + size
add_custom_command(TARGET M4Temp.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:M4Temp.elf>
                                        ${CMAKE_CURRENT_BINARY_DIR}/M4Temp.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:M4Temp.elf>
                                        ${CMAKE_CURRENT_BINARY_DIR}/M4Temp.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:M4Temp.elf>
    COMMENT "M4Temp post-build: generating .hex / .bin"
)

# ═══════════════════════════════════════════════════════════════════════════════
# TARGET 2: rs485_echo.elf  — minimal RS485 echo test (register-level only)
# ═══════════════════════════════════════════════════════════════════════════════

add_executable(rs485_echo.elf
    ${CMAKE_CURRENT_SOURCE_DIR}/test/rs485_echo.c
    ${SRC_DIR}/system_stm32f4xx.c
    ${SRC_DIR}/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f407xx.s
)

target_compile_definitions(rs485_echo.elf PRIVATE
    USE_STDPERIPH_DRIVER
    STM32F4XX
    STM32F40XX
    STM32F407xx
    RS485_ENABLE
    __FPU_PRESENT=1
    __FPU_USED=1
    RS485_ECHO_TEST      # sentinel so rs485_echo.c can define its own main()
)

target_include_directories(rs485_echo.elf PRIVATE
    ${SRC_DIR}
    ${CMSIS_DEV}
    ${CMSIS_CORE}
)

target_link_options(rs485_echo.elf PRIVATE
    ${COMMON_LINK_FLAGS}
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/rs485_echo.map,--cref
)

target_link_libraries(rs485_echo.elf PRIVATE gcc)

# Post-build: .hex + .bin + size
add_custom_command(TARGET rs485_echo.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:rs485_echo.elf>
                                        ${CMAKE_CURRENT_BINARY_DIR}/rs485_echo.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:rs485_echo.elf>
                                        ${CMAKE_CURRENT_BINARY_DIR}/rs485_echo.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:rs485_echo.elf>
    COMMENT "rs485_echo post-build: generating .hex / .bin"
)
