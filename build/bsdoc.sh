#!/bin/sh
cc -I./include -o bin/bsdoc src/bsdoc.c lib/.build/basic.c.o lib/.build/token.c.o lib/.build/lexer.c.o
