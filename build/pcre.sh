#!/bin/sh

PKG=pcre2-10.44

mkdir -p thirdparty && cd thirdparty

if [ ! -d "$PKG" ]; then
    curl -LO "https://github.com/PCRE2Project/pcre2/releases/download/$PKG/$PKG.tar.bz2"
    tar fjx "$PKG.tar.bz2"
    rm -rf "$PKG.tar.bz2"
fi

if [ "$1" = "win32" ]; then
    if [ ! -d pcre-win32 ]; then
        cd "$PKG"
        make clean

        mkdir build && cd build

        # cmake -G "Visual Studio 16 2019" -A x64 -DPCRE2_STATIC=ON -DCMAKE_INSTALL_PREFIX=../pcre-win32 ..
        # cmake -A x64 -DPCRE2_STATIC=ON -DCMAKE_INSTALL_PREFIX=../../pcre-win32 ..
        cmake -G "MinGW Makefiles" -A x64 -DPCRE2_STATIC=ON -DCMAKE_INSTALL_PREFIX=../../pcre-win32 ..

        # Build and install
        cmake --build . --config Release --target install

        # ./configure --host=x86_64-w64-mingw32 --prefix=$(pwd)/../pcre-win32 --enable-static --disable-shared
        # make -j3
        # make install
    fi
else
    if [ ! -d pcre-posix ]; then
        cd "$PKG"
        make clean
        ./configure --prefix=$(pwd)/../pcre-posix --enable-static --disable-shared
        make -j3
        make install
    fi
fi
