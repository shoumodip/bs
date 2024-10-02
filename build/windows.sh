#!/bin/bash

set -xe

# Will this even work lmao? ChatGPT says it can. Oh well...
pkg-config --list-all

CFLAGS="$(pkg-config --cflags libpcre) -I./include -O3"
LIBS="-lm -ldl $(pkg-config --libs libpcre)"

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    gcc $CFLAGS -o lib/.build/$file.o -c src/bs/$file
done

gcc $CFLAGS -o bin/bs.exe src/bs.c lib/.build/* $LIBS
gcc $CFLAGS -o lib/libbs.dll -shared lib/.build/* $LIBS

# Again, will this even work?
ar rcs lib/libbs.lib lib/.build/*
