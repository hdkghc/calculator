set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

project(calculator C CXX ASM)
pico_sdk_init()

# library configuration (override if necessary, see pico-st7735 for a complete list)
set(SPI_TFT_PORT "spi0" CACHE STRING "TFT spi port-name")
set(SPI_TFT_CS   "17"   CACHE STRING "TFT CS  pin number")
set(SPI_TFT_DC   "21"   CACHE STRING "TFT DC  pin number")
set(SPI_TFT_RST  "20"   CACHE STRING "TFT RST pin number")
set(TFT_OPTIONS TFT_ENABLE_BLACK TFT_ENABLE_RESET TFT_ENABLE_TEXT TFT_ENABLE_SHAPES
                TFT_ENABLE_ROTATE TFT_ENABLE_SCROLL TFT_ENABLE_FONTS
            CACHE STRING "TFT options/functions")

# build the st7735-library from our submodule
add_subdirectory(pico-st7735)

include_directories(${CMAKE_CURRENT_LIST_DIR}/pico-st7735/include)
include_directories(${CMAKE_CURRENT_LIST_DIR}/fonts)

#program configuration
set(SPI_PORT "spi0" CACHE STRING "spi port-name")
set(SPI_RX   "16"   CACHE STRING "RX  pin number")
set(SPI_TX   "19"   CACHE STRING "TX  pin number")
set(SPI_SCK  "24"   CACHE STRING "SCK pin number")

add_compile_definitions(SPI_PORT=${SPI_PORT})
add_compile_definitions(SPI_RX=${SPI_RX})
add_compile_definitions(SPI_TX=${SPI_TX})
add_compile_definitions(SPI_SCK=${SPI_SCK})
foreach(opt IN LISTS TFT_OPTIONS)
    message(">>> using: ${opt}")
    add_compile_definitions(${opt})
endforeach(opt)

# add preprocessor-constant DEBUG for Debug-builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_VERBOSE_MAKEFILE 1)
  add_compile_definitions(DEBUG)
else()
endif()


add_executable(calculator
    main.cpp
)

pico_enable_stdio_uart(${PROJECT_NAME} 1)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

pico_set_program_name(${PROJECT_NAME} "calculator")
pico_set_program_version(${PROJECT_NAME} "0.1")

set_target_properties(calculator PROPERTIES SUFFIX ".elf")

target_link_libraries(calculator
    pico_stdlib
    hardware_spi
    st7735
)

pico_add_extra_outputs(calculator)