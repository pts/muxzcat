#! /bin/sh --
# by pts@fazekas.hu at Tue Feb 12 02:37:09 CET 2019

set -ex

CFLAGS='-DCONFIG_SIZE_OPT -DCONFIG_PROB32 -m32 -mconsole -s -Os -W -Wall -Wextra -Werror=implicit-function-declaration -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-ident -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -mpreferred-stack-boundary=2 -falign-jumps=1 -falign-loops=1 '
i686-w64-mingw32-gcc $CFLAGS -o muxzcat.msvcrt.exe muxzcat.c
# `-Wl,-n' would cause segfault when run with wine-4.0.
i686-w64-mingw32-gcc $CFLAGS -o muxzcat.exe muxzcat.c -nostdlib -lkernel32 -D__KERNEL32TINY__
ls -l muxzcat.exe
# TODO(pts): upx.pts: muxzcat.exe: CantPackException: mem_size 2; take care
# ./upx.pts -qq -f -o muxzcat.upx.exe muxzcat.exe
# ls -l muxzcat.upx.exe

: "$0" OK.
