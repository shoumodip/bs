#!/bin/sh

set -xe

CFLAGS="-I./include -I./thirdparty/pcre-posix/include -O3"
LIBS="-L./thirdparty/pcre-posix/lib -lm -ldl -lpcre2-8"

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    cc $CFLAGS -o lib/.build/$file.o -c -fPIC src/bs/$file &
done
wait

cc $CFLAGS -o bin/bs src/bs.c lib/.build/* $LIBS &
cc $CFLAGS -o lib/libbs.dylib -shared lib/.build/* $LIBS &
ar rcs lib/libbs.a lib/.build/* &
wait
