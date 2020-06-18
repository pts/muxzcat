CC = gcc
TCC = pts-tcc
CFLAGS =
.PHONY: all clean generate
all: muxzcat #muaxzcat

muxzcat: muxzcat.c
	$(CC) -ansi -s -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<

muaxzcat: muaxzcat.c
	$(CC) -ansi -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<


muxzcat.tcc: muxzcat.c
	$(TCC) -s -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<


# `-Wl,-n' would cause segfault when run with wine-4.0.
CFLAGS_MINGW = -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -m32 -mconsole -s -Os -W -Wall -Wextra -Werror=implicit-function-declaration -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-ident -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -mpreferred-stack-boundary=2 -falign-jumps=1 -falign-loops=1

muxzcat.msvcrt.exe: muxzcat.c
	i686-w64-mingw32-gcc $(CFLAGS_MINGW) -o muxzcat.msvcrt.exe muxzcat.c

muxzcat.exe: muxzcat.c
	i686-w64-mingw32-gcc $(CFLAGS_MINGW) -o muxzcat.exe muxzcat.c -nostdlib -lkernel32 -D__KERNEL32TINY__


CFLAGS_XTINY = -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -W -Wall -Wextra -Werror=implicit-function-declaration

muxzcat.xtiny: muxzcat.c
	xtiny gcc $(CFLAGS_XTINY) -o muxzcat.xtiny muxzcat.c

muxzcat.upx: muxzcat.xtiny upxbc upx.pts
	./upxbc --upx=./upx.pts --elftiny -f -o muxzcat.upx muxzcat.xtiny


muaxzcat.pl: muaxzcat.c genpl.sh
	./genpl.sh

muaxzcat.java: muaxzcat.c genjava.sh
	./genjava.sh

# Regenerates muxzcat.pl and muxzcat.java.
generate: muaxzcat.pl muaxzcat.java
	cp -a muaxzcat.pl muxzcat.pl
	perl -pe "s@^public class muaxzcat @public class muxzcat @" <muaxzcat.java >muxzcat.java

clean:
	rm -f muxzcat muaxzcat
