cmake_minimum_required(VERSION 3.13)
project(calculator C CXX ASM)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
pico_sdk_init()

add_executable(calculator
    main.cpp
)

set_target_properties(calculator PROPERTIES SUFFIX ".elf")

target_link_libraries(calculator
    pico_stdlib
    hardware_spi
)

pico_add_extra_outputs(calculator)