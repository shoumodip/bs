#!/bin/sh

CFLAGS=$(cat compile_flags.txt)

mkdir -p lib
cc $CFLAGS -o lib/libbs.so -shared -fPIC src/bs/*.c -Wl,-rpath=./

mkdir -p bin
cc $CFLAGS -o bin/bs src/main.c -L./lib/ -lbs -Wl,-rpath=$PWD/lib/
