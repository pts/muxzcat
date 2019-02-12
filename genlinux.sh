#! /bin/bash --
# by pts@fazekas.hu at Tue Feb 12 02:33:04 CET 2019

set -ex

xtiny gcc -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -W -Wall -Wextra -Werror=implicit-function-declaration -o muxzcat muxzcat.c
ls -l muxzcat
./upxbc --upx=./upx.pts --elftiny -f -o muxzcat.upx muxzcat
ls -l muxzcat.upx

: "$0" OK.
