cmake_minimum_required(VERSION 3.10)

#
# Toolchain 
#
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(TOOLS_PREFIX "arm-none-eabi-")

 

if(WIN32)
    # Windows cubeclt path (adjust as needed for your installation)
    set(DEFAULT_TOOLS_DIR "C:/ST/STM32CubeCLT_1.17.0/GNU-tools-for-STM32/bin/")
    set(EXE_SUFFIX ".exe")    
else()
    # Linux path 
    set(DEFAULT_TOOLS_DIR "/opt/st/stm32cubeclt_1.18.0/GNU-tools-for-STM32/bin/")    
    set(EXE_SUFFIX "")    
endif()


# Command line override of tools dir.
if(DEFINED ENV{ARM_TOOLCHAIN_DIR})
    set(TOOLS_DIR $ENV{ARM_TOOLCHAIN_DIR})
elseif(NOT DEFINED TOOLS_DIR)
    set(TOOLS_DIR ${DEFAULT_TOOLS_DIR})
endif()

# Make sure path ends with directory separator
if(NOT TOOLS_DIR MATCHES "/$" AND NOT TOOLS_DIR MATCHES "\\\\$")
    if(WIN32)
        set(TOOLS_DIR "${TOOLS_DIR}\\")
    else()
        set(TOOLS_DIR "${TOOLS_DIR}/")
    endif()
endif()

# Set tool paths
set(CMAKE_C_COMPILER ${TOOLS_DIR}${TOOLS_PREFIX}gcc${EXE_SUFFIX})
set(CMAKE_ASM_COMPILER ${TOOLS_DIR}${TOOLS_PREFIX}gcc${EXE_SUFFIX})
set(CMAKE_CXX_COMPILER ${TOOLS_DIR}${TOOLS_PREFIX}g++${EXE_SUFFIX})
set(CMAKE_OBJCOPY ${TOOLS_DIR}${TOOLS_PREFIX}objcopy${EXE_SUFFIX})
set(CMAKE_SIZE ${TOOLS_DIR}${TOOLS_PREFIX}size${EXE_SUFFIX})

# Enforce C17 standard (this can be overriden in CMakeLists.txt)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# None board-specific compiler flags
set(CMAKE_C_FLAGS "-fdata-sections -fstack-usage -ffunction-sections --specs=nano.specs -Wl,--gc-sections -u _printf_float")
set(CMAKE_ASM_FLAGS "-x assembler-with-cpp")
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")
set(CMAKE_C_FLAGS_RELEASE "-Ofast")

# We're cross-compiling, so disable CMake test executables
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Print toolchain configuration for debugging
#message(STATUS "Using toolchain directory: ${TOOLS_DIR}")
#message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")