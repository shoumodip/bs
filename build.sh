#!/bin/sh

CFLAGS=$(cat compile_flags.txt)

mkdir -p bin lib/.build
for file in $(ls src/bs); do
    cc $CFLAGS -o lib/.build/$file.o -c -fPIC src/bs/$file &
done
wait

cc $CFLAGS -o bin/bs src/main.c lib/.build/* -lm &
cc $CFLAGS -o lib/libbs.so -shared lib/.build/* -lm &
ar rcs lib/libbs.a lib/.build/* &
wait
