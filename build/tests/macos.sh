#!/bin/sh
cc -o executables/echo_args executables/echo_args.c
cc -o executables/echo_stdin executables/echo_stdin.c
cc -I../include -o executables/addsub.dylib -fPIC -shared executables/addsub.c ../lib/libbs.dylib
cc -I../include -o executables/invalid.dylib -fPIC -shared executables/invalid.c ../lib/libbs.dylib
