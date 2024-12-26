#!/bin/sh
cc -o executables/echo_args executables/echo_args.c
cc -o executables/echo_stdin executables/echo_stdin.c
cc -I../include -o executables/addsub.so -fPIC -shared executables/addsub.c ../lib/libbs.so
cc -I../include -o executables/invalid.so -fPIC -shared executables/invalid.c ../lib/libbs.so
