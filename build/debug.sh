#!/bin/sh

CFLAGS=$(cat compile_flags.txt)
LIBS="-lm -ldl -lpcre"

rm -rf bin lib
mkdir -p bin lib/.build

for file in $(ls src/bs); do
    cc $CFLAGS -o lib/.build/$file.o -c -fPIC src/bs/$file &
done
wait

cc $CFLAGS -o bin/bs src/bs.c lib/.build/* $LIBS &
cc $CFLAGS -o bin/bsdoc src/bsdoc.c lib/.build/basic.c.o lib/.build/token.c.o lib/.build/lexer.c.o &

cc $CFLAGS -o lib/libbs.so -shared lib/.build/* $LIBS &
ar rcs lib/libbs.a lib/.build/* &
wait
