#!/bin/sh

PKG=pcre2-10.44

mkdir -p thirdparty && cd thirdparty

if [ ! -d "$PKG" ]; then
    wget "https://github.com/PCRE2Project/pcre2/releases/download/$PKG/$PKG.tar.bz2"
    tar fjx "$PKG.tar.bz2"
    rm -rf "$PKG.tar.bz2"
fi

if [ "$1" = "win32" ]; then
    if [ ! -d pcre-win32 ]; then
        cd "$PKG"
        make clean
        ./configure --host=x86_64-w64-mingw32 --prefix=$(pwd)/../pcre-win32 --enable-static --disable-shared
        make -j3
        make install
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
