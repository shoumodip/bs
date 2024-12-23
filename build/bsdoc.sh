#!/bin/sh

set -xe
cc -I./include -o bin/bsdoc src/bsdoc.c src/bs/basic.c src/bs/token.c src/bs/lexer.c
