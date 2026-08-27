cmake_minimum_required(VERSION 3.13)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ============================================================
# MCUXpresso SDK for RT1011
# ============================================================

if(NOT DEFINED MCUX_SDK_PATH)
    if(DEFINED ENV{MCUX_SDK_PATH})
        set(MCUX_SDK_PATH $ENV{MCUX_SDK_PATH})
    else()
        set(MCUX_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/mcuxsdk)
    endif()
endif()

message(STATUS "MCUX_SDK_PATH: ${MCUX_SDK_PATH}")

if(NOT EXISTS ${MCUX_SDK_PATH})
    message(FATAL_ERROR "MCUXpresso SDK not found at: ${MCUX_SDK_PATH}")
endif()

if(NOT DEFINED ARMGCC_DIR)
    if(DEFINED ENV{ARMGCC_DIR})
        set(ARMGCC_DIR $ENV{ARMGCC_DIR})
    else()
        set(ARMGCC_DIR "/usr")
    endif()
endif()

set(TOOLCHAIN_FILE ${MCUX_SDK_PATH}/cmake/toolchain/armgcc.cmake)
if(NOT EXISTS ${TOOLCHAIN_FILE})
    set(TOOLCHAIN_FILE ${MCUX_SDK_PATH}/tools/cmake_toolchain_files/armgcc.cmake)
endif()

if(NOT EXISTS ${TOOLCHAIN_FILE})
    message(FATAL_ERROR "Toolchain file not found at: ${TOOLCHAIN_FILE}")
endif()

set(CMAKE_TOOLCHAIN_FILE ${TOOLCHAIN_FILE})

project(calculator C CXX ASM)

# ============================================================
# 芯片型号
# ============================================================
add_definitions(-DCPU_MIMXRT1011DAE5A)
set(MCU MIMXRT1011)
set(DEVICE_PATH ${MCUX_SDK_PATH}/devices/RT/RT1010/${MCU})
set(PERIPH_PATH ${MCUX_SDK_PATH}/devices/RT/RT1010/periph)
set(CMSIS_PATH ${MCUX_SDK_PATH}/arch/arm/CMSIS/Core/Include)

if(NOT EXISTS ${DEVICE_PATH})
    message(FATAL_ERROR "Device path not found: ${DEVICE_PATH}")
endif()

# ============================================================
# 头文件路径
# ============================================================
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/inc
    ${CMAKE_CURRENT_LIST_DIR}/fonts
    ${DEVICE_PATH}
    ${DEVICE_PATH}/drivers
    ${PERIPH_PATH}
    ${CMSIS_PATH}
    ${MCUX_SDK_PATH}/CMSIS/Core/Include
    ${MCUX_SDK_PATH}/CMSIS/Core_A/Include
    ${MCUX_SDK_PATH}/components/osa
    ${MCUX_SDK_PATH}/components/lists
    ${MCUX_SDK_PATH}/components/utilities
    ${MCUX_SDK_PATH}/drivers/common
    ${MCUX_SDK_PATH}/drivers/lpi2c
    ${MCUX_SDK_PATH}/drivers/lpspi
    ${MCUX_SDK_PATH}/drivers/iomuxc
    ${MCUX_SDK_PATH}/drivers/clock
)

# ============================================================
# 预处理器定义
# ============================================================
add_compile_definitions(
    CPU_MIMXRT1011DAE5A
    CPU_MIMXRT1011DAE5A_cm7
    XIP_EXTERNAL_FLASH=1
    XIP_BOOT_HEADER_ENABLE=1
    SDK_DEBUGCONSOLE=0
    CR_INTEGER_PRINTF
    PRINTF_FLOAT_ENABLE=0
    __MCUXPRESSO
    __USE_CMSIS
    DEBUG_CONSOLE_DISABLE
    _MB_CAPABLE=0
    # _WCHAR_T_IS_16_BIT=1
    # __WCHAR_MAX__=65535
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_VERBOSE_MAKEFILE 1)
    add_compile_definitions(DEBUG)
endif()

if(TEST)
    add_compile_definitions(TEST)
endif()

# ============================================================
# 源文件
# ============================================================
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp
)

# ============================================================
# MCUXpresso SDK 源文件
# ============================================================
set(MCUX_SDK_SRC
    ${DEVICE_PATH}/system_${MCU}.c
    ${DEVICE_PATH}/gcc/startup_${MCU}.S
    ${MCUX_SDK_PATH}/drivers/common/fsl_common.c
    ${MCUX_SDK_PATH}/drivers/lpi2c/fsl_lpi2c.c
    ${MCUX_SDK_PATH}/drivers/lpspi/fsl_lpspi.c
    ${MCUX_SDK_PATH}/components/lists/fsl_component_generic_list.c
)

# ============================================================
# 链接脚本
# ============================================================
set(LINKER_FILE ${DEVICE_PATH}/gcc/MIMXRT1011xxxxx_flexspi_nor.ld)
if(NOT EXISTS ${LINKER_FILE})
    set(LINKER_FILE ${DEVICE_PATH}/gcc/MIMXRT1011xxxxx_ram.ld)
endif()

if(NOT EXISTS ${LINKER_FILE})
    message(FATAL_ERROR "Linker file not found: ${LINKER_FILE}")
endif()

message(STATUS "Using linker script: ${LINKER_FILE}")

foreach(SRC ${MCUX_SDK_SRC})
    if(NOT EXISTS ${SRC})
        message(WARNING "Source file not found: ${SRC}")
    endif()
endforeach()

# ============================================================
# 可执行文件
# ============================================================
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${MCUX_SDK_SRC}
)

# ============================================================
# 编译选项
# ============================================================
target_compile_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -Os
    -g
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -fno-builtin
    -fno-common
    # -fshort-wchar
)

# ============================================================
# 链接选项
# ============================================================
target_link_options(${PROJECT_NAME} PRIVATE
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-sp-d16
    -mfloat-abi=hard
    -T ${LINKER_FILE}
    -specs=nano.specs
    -specs=nosys.specs
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map
    -Wl,--undefined=__ssputws_r
    -u _printf_float
)

# ============================================================
# 链接库
# ============================================================
target_link_libraries(${PROJECT_NAME}
    -Wl,--start-group
    m
    c_nano
    gcc
    stdc++_nano
    supc++_nano
    nosys
    -Wl,--end-group
    -lc_nano
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".elf"
    LINKER_LANGUAGE CXX
)

# ============================================================
# 生成输出文件
# ============================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJDUMP} -d ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.lst
    COMMAND ${CMAKE_OBJDUMP} -t ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.elf > ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.sym
    COMMENT "Generating binary, hex, listing and symbol files"
)

message(STATUS "========================================")
message(STATUS "MCUXpresso SDK Path: ${MCUX_SDK_PATH}")
message(STATUS "Device Path: ${DEVICE_PATH}")
message(STATUS "Periph Path: ${PERIPH_PATH}")
message(STATUS "CMSIS Path: ${CMSIS_PATH}")
message(STATUS "Toolchain: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "MCU: ${MCU}")
message(STATUS "Linker: ${LINKER_FILE}")
message(STATUS "========================================")