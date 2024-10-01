#!/bin/sh

pkg-config --list-all | grep pcre # Temporary
# CFLAGS="-I./include -O3 -I$(pkg-config -- pcre)/include"
# LIBS="-L$(brew --prefix pcre)/lib -lm -ldl -lpcre"

# rm -rf bin lib
# mkdir -p bin lib/.build

# for file in $(ls src/bs); do
#     cc $CFLAGS -o lib/.build/$file.o -c -fPIC src/bs/$file
# done

# cc $CFLAGS -o bin/bs src/bs.c lib/.build/* $LIBS
# cc $CFLAGS -o lib/libbs.dylib -shared lib/.build/* $LIBS
# ar rcs lib/libbs.a lib/.build/*
