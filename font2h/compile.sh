#!/bin/bash

set -e

NEED_PACKAGES=""

if ! [ -f /usr/include/freetype2/ft2build.h ]; then
    NEED_PACKAGES+="libfreetype-dev "
fi

# if ! [ -f /usr/include/SDL2/SDL.h ]; then
#     NEED_PACKAGES+="libsdl2-dev "
# fi

if [ -n "$NEED_PACKAGES" ]; then
    echo "Installing required packages: $NEED_PACKAGES"
    sudo apt update -qq
    sudo apt install -y $NEED_PACKAGES
fi

echo "Compiling..."
mkdir -p bin
g++ ./font2h/font2h.cpp -o ./font2h/bin/font2h -lfreetype -O2 -I/usr/include/freetype2
# g++ ./font2h/pvw.cpp -o ./font2h/bin/preview -lfreetype -lSDL2 -O2 -I/usr/include/freetype2 -I/usr/include/SDL2

echo "Done."