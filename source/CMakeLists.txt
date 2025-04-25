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

# Supported boards
set(BOARD stm32f411-blackpill)
# set(BOARD stm32f411-disco)

# Uncomment this if you have a JLink probe and want RTT (tracing) enabled.
set(JLINK ON)

# Folders
set(SRC_DIR ${PROJECT_SOURCE_DIR}/source)
set(UI_DIR ${SRC_DIR}/ui)
set(SYNTH_DIR ${SRC_DIR}/synth)
set(DAE_DIR ${SRC_DIR}/dae)
set(BSP_DIR ${SRC_DIR}/bsp)

# ------------------------------------------------------------------------------
# Application/synth sources and includes
# ------------------------------------------------------------------------------
set(SRCS_APP 
  # Core
  ${SRC_DIR}/main.c
  ${SRC_DIR}/syscalls.c
  ${SRC_DIR}/sysmem.c
  ${SRC_DIR}/tasks.c

  # UI code
  ${UI_DIR}/ui.c

  # Synth
  ${SYNTH_DIR}/synth.c
  ${SYNTH_DIR}/voice.c
  ${SYNTH_DIR}/osc.c  
  ${SYNTH_DIR}/env_gen.c
  ${SYNTH_DIR}/amp.c
  ${SYNTH_DIR}/params.c
  ${SYNTH_DIR}/lfo.c
  ${SYNTH_DIR}/filter.c
)

set(INCL_APP 
  ${UI_DIR}
  ${SYNTH_DIR})

# App specific defines
set(DEFS_APP    
  $<$<CONFIG:DEBUG>: DEBUG> 
  # Dim (reduce) level of output by 75%
  DIM_OUTPUT
  # Filter saturation type - SATURATION_TANH | SATURATION_TANH_APPROX | SATURATION_FEEDBACK
  SATURATION_FEEDBACK
  # Applies a soft-clip the the oscillators for a little VA feel with some loss of amplitude
  OSC_SOFT_SATURATION
)

# ------------------------------------------------------------------------------
# The DAE sources & includes
# ------------------------------------------------------------------------------
set(SRCS_DAE
  ${DAE_DIR}/dae.c
  ${DAE_DIR}/dsp_core.c
  ${DAE_DIR}/dsp_math.c
  ${DAE_DIR}/midi.c
  ${DAE_DIR}/param_store.c
)

set(INCL_DAE 
  ${DAE_DIR}
)

# DAE specific defines
set(DEFS_DAE  
  $<$<CONFIG:DEBUG>: CHECK_BUFFER> 
  DAE_BLOCK_SIZE=128
  DAE_SAMPLE_RATE=48000
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
)

set(INCL_BSP

 # LL drivers
 ${BSP_DIR}/drivers/STM32F4xx_HAL_Driver/Inc
 ${BSP_DIR}/drivers/CMSIS/Include
 ${BSP_DIR}/drivers/CMSIS/Device/ST/STM32F4xx/Include

 # FreeRTOS
 ${BSP_DIR}/middleware/FreeRTOS/Source/include
 ${BSP_DIR}/middleware/FreeRTOS/Source/portable/GCC/ARM_CM4F
 
)

set(DEFS_BSP
  USE_FULL_LL_DRIVER     
)

# Compiler options
set(COPTS_BSP 
  -mcpu=cortex-m4  
  -mfpu=fpv4-sp-d16 
  -mfloat-abi=hard   
  -Wall -Wextra -Werror
  -Wdouble-promotion
  # -Wconversion        # This can generate a lot of warning from 3rd party code
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
# Segger RTT - only useful for J-Link probes
# ------------------------------------------------------------------------------
if(DEFINED JLINK) 
  list(APPEND SRCS_BSP 
      ${BSP_DIR}/middleware/Segger/RTT/SEGGER_RTT_printf.c
      ${BSP_DIR}/middleware/Segger/RTT/SEGGER_RTT.c
      )
  list(APPEND INCL_BSP   
      ${BSP_DIR}/middleware/Segger/RTT
      ${BSP_DIR}/middleware/Segger/Config
      )
  list(APPEND DEFS_BSP
      RTT_ENABLED
      )
endif()



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
target_compile_definitions(${TARGET} PRIVATE ${DEFS_APP} ${DEFS_DAE} ${DEFS_BSP})
target_compile_options(${TARGET} PRIVATE ${COPTS_BSP})
target_link_options(${TARGET} PRIVATE ${LOPTS_BSP} -T ${BSP_DIR}/${BOARD}/${LINKER_SCRIPT})

# ------------------------------------------------------------------------------
# Post-build: show memory usage
# ------------------------------------------------------------------------------
add_custom_command(TARGET ${TARGET} POST_BUILD
#   COMMAND ${CMAKE_OBJCOPY} -O ihex ${TARGET} ${TARGET}.hex
#   COMMAND ${CMAKE_OBJCOPY} -O binary ${TARGET} ${TARGET}.bin
COMMAND ${CMAKE_SIZE} ${TARGET}
)

