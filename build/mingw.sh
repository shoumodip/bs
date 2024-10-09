#!/bin/sh

set -xe

CFLAGS="-I./include -O3"
LIBS="-lm"

CC=x86_64-w64-mingw32-gcc
AR=x86_64-w64-mingw32-ar

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    $CC $CFLAGS -o lib/.build/$file.o -c src/bs/$file &
done
wait

$CC $CFLAGS -o bin/bs.exe src/bs.c lib/.build/* $LIBS &
$CC $CFLAGS -o lib/bs.dll -shared lib/.build/* $LIBS &
$AR rcs lib/bs.lib lib/.build/*.o &
wait
