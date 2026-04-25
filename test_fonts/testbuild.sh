#! /bin/bash

# build font2h
echo "Building font2h..."
bash ../font2h/compile.sh

# extract font data
echo "Extracting font data..."
./../font2h/bin/font2h ./font2h/fonts/ClassWizCWDisplay-Regular.otf ./fonts/CW.h 12
./../font2h/bin/font2h ./font2h/fonts/ClassWizMathCW-Regular.otf ./fonts/CWMath.h 12

# build main program
echo "Building test program with cmake..."
mkdir -p build
cd build
rm -rf build/*
cmake .. -DCMAKE_TOOLCHAIN_FILE=$PICO_SDK_PATH/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake
make -j4