#!/bin/sh
cc $(cat compile_flags.txt) -o bs src/*.c -Wl,-rpath=./
