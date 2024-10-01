#!/bin/sh

CFLAGS="-I./include -O3"
LIBS="-lm -ldl -lpcre"

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    cc $CFLAGS -o lib/.build/$file.o -c -fPIC src/bs/$file
done

cc $CFLAGS -o bin/bs src/bs.c lib/.build/* $LIBS
cc $CFLAGS -o lib/libbs.so -shared lib/.build/* $LIBS
ar rcs lib/libbs.a lib/.build/*
