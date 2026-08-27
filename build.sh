#!/bin/bash

# ============================================================
# MCUXpresso SDK RT1011 构建脚本
# ============================================================

# 设置 SDK 路径
if [ -z "$MCUX_SDK_PATH" ]; then
    export MCUX_SDK_PATH="${HOME}/picoproj/mcuxpresso-sdk/mcuxsdk"
    echo "MCUX_SDK_PATH set to: $MCUX_SDK_PATH"
else
    echo "MCUX_SDK_PATH: $MCUX_SDK_PATH"
fi

# 验证 SDK 是否存在
if [ ! -d "$MCUX_SDK_PATH" ]; then
    echo "ERROR: MCUXpresso SDK not found at: $MCUX_SDK_PATH"
    exit 1
fi

# 验证设备目录是否存在
DEVICE_PATH="${MCUX_SDK_PATH}/devices/RT/RT1010/MIMXRT1011"
if [ ! -d "$DEVICE_PATH" ]; then
    echo "ERROR: Device MIMXRT1011 not found at: $DEVICE_PATH"
    exit 1
fi
echo "Device path: $DEVICE_PATH"

# 设置工具链路径
if [ -z "$ARMGCC_DIR" ]; then
    export ARMGCC_DIR="/usr"
    echo "ARMGCC_DIR set to: $ARMGCC_DIR"
else
    echo "ARMGCC_DIR: $ARMGCC_DIR"
fi

# 检查 arm-none-eabi-gcc 是否存在
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo "ERROR: arm-none-eabi-gcc not found in PATH"
    echo "Please install ARM GCC toolchain:"
    echo "  sudo apt install gcc-arm-none-eabi"
    exit 1
fi

# ============================================================
# 构建 font2h
# ============================================================
echo "Building font2h..."
if [ -d "font2h" ]; then
    bash font2h/compile.sh 2>/dev/null || echo "Warning: font2h compile failed"
else
    echo "Warning: font2h directory not found"
fi

# ============================================================
# 提取字体数据
# ============================================================
echo "Extracting font data..."
if [ -x "./font2h/bin/font2h" ]; then
    ./font2h/bin/font2h ./font2h/fonts/ClassWizCWDisplay-Regular.otf ./fonts/CW.h 12 2>/dev/null || echo "Warning: font extraction failed"
    ./font2h/bin/font2h ./font2h/fonts/ClassWizMathCW-Regular.otf ./fonts/CWMath.h 12 2>/dev/null || echo "Warning: font extraction failed"
else
    echo "Warning: font2h binary not found"
fi

# ============================================================
# 构建主程序
# ============================================================
echo "Building with cmake..."

rm -rf build
mkdir build
cd build

CMAKE_ARGS=".. -DMCUX_SDK_PATH=${MCUX_SDK_PATH} -DARMGCC_DIR=${ARMGCC_DIR}"

if [ "$1" = "test" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DTEST=1"
fi

if [ "$1" = "debug" ] || [ "$2" = "debug" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_BUILD_TYPE=Debug"
fi

cmake ${CMAKE_ARGS}
if [ $? -ne 0 ]; then
    echo "CMake configuration failed"
    exit 1
fi

make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed"
    exit 1
fi

echo "========================================"
echo "Build complete!"
echo "Output:"
echo "  build/calculator.elf"
echo "  build/calculator.bin"
echo "  build/calculator.hex"
echo "========================================"