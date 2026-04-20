#! /bin/bash
cd build
rm -rf build/*
cmake .. -DCMAKE_TOOLCHAIN_FILE=$PICO_SDK_PATH/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake
make -j4