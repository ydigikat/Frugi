# ------------------------------------------------------------------------------
#  Frugi
#  Author: ydigikat
# ------------------------------------------------------------------------------
#  MIT License
#  Copyright (c) 2025 YDigiKat
# 
#  Permission to use, copy, modify, and/or distribute this code for any purpose
#  with or without fee is hereby granted, provided the above copyright notice an
#  this permission notice appear in all copies.
# ------------------------------------------------------------------------------
set(TARGET app.elf)

# Board support
#set(BOARD stm32f411-blackpill)
set(BOARD stm32f411-disco)


# Folders
set(SYNTH_DIR ${PROJECT_SOURCE_DIR}/source/synth)
set(DAE_DIR ${PROJECT_SOURCE_DIR}/source/dae)
set(BSP_DIR ${PROJECT_SOURCE_DIR}/source/bsp)

# ------------------------------------------------------------------------------
# Application/synth sources and includes
# ------------------------------------------------------------------------------
set(SRCS_APP 
  # Core
  ${SYNTH_DIR}/main.c
  ${SYNTH_DIR}/syscalls.c
  ${SYNTH_DIR}/sysmem.c
  ${SYNTH_DIR}/tasks.c

  # UI code
  ${SYNTH_DIR}/ui/ui.c

  # Synth
  ${SYNTH_DIR}/frugi_synth.c
  ${SYNTH_DIR}/frugi_voice.c  
  ${SYNTH_DIR}/frugi_osc.c
  ${SYNTH_DIR}/frugi_env_gen.c
  ${SYNTH_DIR}/frugi_amp.c
  ${SYNTH_DIR}/frugi_lfo.c
  ${SYNTH_DIR}/frugi_noise.c
  ${SYNTH_DIR}/frugi_filter.c
  ${SYNTH_DIR}/frugi_params.c
)

set(INCL_APP 
  ${SYNTH_DIR}/ui 
  ${SYNTH_DIR}/synth)

# App specific defines
set(DEFS_APP
  CHECK_BUFFER
  DAE_BLOCK_SIZE=128
  DAE_SAMPLE_RATE=96000
)

# ------------------------------------------------------------------------------
# The DAE sources & includes
# ------------------------------------------------------------------------------
set(SRCS_DAE
  ${DAE_DIR}/dae.c
  ${DAE_DIR}/dsp_core.c
  ${DAE_DIR}/dsp_math.c
  ${DAE_DIR}/midi.c
  ${DAE_DIR}/params.c
)

set(INCL_DAE 
  ${DAE_DIR}
)

# ------------------------------------------------------------------------------
# Drivers & Middleware
# ------------------------------------------------------------------------------
set(SRCS_BSP
  # Core
  ${BSP_DIR}/core/system_stm32f4xx.c
  ${BSP_DIR}/core/core_board.c
   
  # LL Drivers
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_spi.c
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_rcc.c
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_gpio.c
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_dma.c
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usart.c
  ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_i2c.c
  
  # FreeRTOS 
  ${BSP_DIR}/middleware/FreeRTOS/Source/tasks.c
  ${BSP_DIR}/middleware/FreeRTOS/Source/list.c
  ${BSP_DIR}/middleware/FreeRTOS/Source/queue.c
  ${BSP_DIR}/middleware/FreeRTOS/Source/portable/MemMang/heap_4.c
  ${BSP_DIR}/middleware/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c    

  # Segger RTT - only useful for J-Link probes
  ${BSP_DIR}/middleware/Segger/RTT/SEGGER_RTT_printf.c
  ${BSP_DIR}/middleware/Segger/RTT/SEGGER_RTT.c
)

set(INCL_BSP

 # LL drivers
 ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Inc
 ${BSP_DIR}/drivers/CMSIS/Include
 ${BSP_DIR}/drivers/CMSIS/Device/ST/STM32F4xx/Include

 # FreeRTOS
 ${BSP_DIR}/middleware/FreeRTOS/Source/include
 ${BSP_DIR}/middleware/FreeRTOS/Source/portable/GCC/ARM_CM4F

   # Segger RTT - only useful for J-Link probes
  ${BSP_DIR}/middleware/Segger/RTT
  ${BSP_DIR}/middleware/Segger/Config
)

set(DEFS_BSP
  USE_FULL_LL_DRIVER  
  RTT_ENABLED
  $<$<CONFIG:DEBUG>: DEBUG> 
)

# Compiler options
set(COPTS_BSP 
  -mcpu=cortex-m4  
  -mfpu=fpv4-sp-d16 
  -mfloat-abi=hard 
  -Wall -Wextra -Werror
  -Wpointer-arith
  -Wno-unused-parameter 
  -Wno-unused-variable 
  -Wno-unused-function 
  -Wno-unused-but-set-variable 
  -Wno-unused-value 
  -Wno-unused-label 
  -Wno-unused-local-typedefs
)

# Link options
set(LOPTS_BSP
  -mcpu=cortex-m4  
  -mfpu=fpv4-sp-d16 
  -mfloat-abi=hard 
)

# ------------------------------------------------------------------------------
# Board support: specific sources, includes, and compiler flags
# ------------------------------------------------------------------------------
if(NOT DEFINED BOARD)
  message(FATAL_ERROR "BOARD type must be set")
endif()


list(APPEND SRCS_BSP
  ${BSP_DIR}/core/system_stm32f4xx.c
  ${BSP_DIR}/${BOARD}/board.c
)

list(APPEND INCL_BSP
    ${BSP_DIR}/core
    ${BSP_DIR}/${BOARD}
)

# Board specific support 
message(STATUS "Configuring board support for '${BOARD}'")

if(BOARD STREQUAL stm32f411-blackpill)   
  # Basic generic 411CE "blackpill" board.
  set(LINKER_SCRIPT STM32F411CEUX_FLASH.ld)  
  list(APPEND SRCS_BSP ${BSP_DIR}/${BOARD}/startup_stm32f411xe.s)
  list(APPEND DEFS_BSP HSE_VALUE=25000000 STM32F411xE)

elseif(BOARD STREQUAL stm32f411-disco)  
  # STM discovery 411VE - onboard CODEC
  set(LINKER_SCRIPT STM32F411VEHX_FLASH.ld)  
  list(APPEND SRCS_BSP ${BSP_DIR}/${BOARD}/startup_stm32f411xe.s 
                       ${BSP_DIR}/${BOARD}/cs43l22.c)
  list(APPEND DEFS_BSP HSE_VALUE=8000000 STM32F411xE DAE_IS_USING_MCLOCK)
else()
  message(FATAL_ERROR "No configuration found for board: '${BOARD}'.")  
endif()  




# ------------------------------------------------------------------------------
# Build targets
# ------------------------------------------------------------------------------
add_executable(${TARGET} ${SRCS_APP} ${SRCS_DAE} ${SRCS_BSP})
target_include_directories(${TARGET} PRIVATE ${INCL_APP} ${INCL_DAE} ${INCL_BSP})
target_compile_definitions(${TARGET} PRIVATE ${DEFS_APP} ${DEFS_BSP})
target_compile_options(${TARGET} PRIVATE ${COPTS_BSP})
target_link_options(${TARGET} PRIVATE ${LOPTS_BSP} -T ${BSP_DIR}/${BOARD}/${LINKER_SCRIPT})
