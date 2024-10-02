#!/bin/bash

set -xe

CC=x86_64-w64-mingw32-gcc
AR=x86_64-w64-mingw32-ar

CFLAGS="-I./include -I./thirdparty/pcre-win32/include -O3"
LIBS="-L./thirdparty/pcre-win32/lib -lm -lpcre2-8"

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    $CC $CFLAGS -o lib/.build/$file.o -c src/bs/$file &
done
wait

$CC $CFLAGS -o bin/bs.exe src/bs.c lib/.build/* $LIBS &
$CC $CFLAGS -o lib/libbs.dll -shared lib/.build/* $LIBS &
$AR rcs lib/libbs.lib lib/.build/* &
wait
