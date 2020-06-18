CC = gcc
CXX = g++
TCC = pts-tcc
CFLAGS =
.PHONY: all clean generate
all: muxzcat #muaxzcat

muxzcat: muxzcat.c
	$(CC) -ansi -s -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<

muaxzcat: muaxzcat.c
	$(CC) -ansi -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<


muxzcat.xx: muxzcat.c
	$(CXX) -ansi -O2 -W -Wall -Wextra $(CFLAGS) -o $@ $<


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


# The minimum value is -mmacosx-version-min=10.4 for
# multiarch/crossbuild@sha256:84a53371f554a3b3d321c9d1dfd485b8748ad6f378ab1ebed603fe1ff01f7b4d .
DARWIN_CFLAGS='-mmacosx-version-min=10.4 -DCONFIG_SIZE_OPT -DCONFIG_PROB32 -Os -W -Wall -Wextra -Werror=implicit-function-declaration -fno-pic -fno-stack-protector -fomit-frame-pointer -fno-ident -fno-unwind-tables -fno-asynchronous-unwind-tables'

muxzcat.darwinc32: muxzcat.c
	docker run -v "$$PWD:/workdir" -u "$(id -u):$$(id -g)" --rm -it multiarch/crossbuild /usr/osxcross/bin/o32-clang $(DARWIN_CFLAGS) -o $@ $< -lSystem -nodefaultlibs
	docker run -v "$$PWD:/workdir" -u "$(id -u):$$(id -g)" --rm -it multiarch/crossbuild /usr/osxcross/bin/i386-apple-darwin14-strip     $@

muxzcat.darwinc64: muxzcat.c
	docker run -v "$$PWD:/workdir" -u "$$(id -u):$$(id -g)" --rm -it multiarch/crossbuild /usr/osxcross/bin/o64-clang $(DARWIN_CFLAGS) -o $@ $< -lSystem -nodefaultlibs
	docker run -v "$$PWD:/workdir" -u "$$(id -u):$$(id -g)" --rm -it multiarch/crossbuild /usr/osxcross/bin/x86_64-apple-darwin14-strip   $@


muaxzcat.pl: muaxzcat.c genpl.sh
	./genpl.sh

muaxzcat.java: muaxzcat.c genjava.sh
	./genjava.sh

# javac in openjdk-11-jdk doesn't support `-target 1.1 -source 1.2'.
JAVAC = /usr/lib/jvm/java-8-openjdk-amd64/bin/javac

muaxzcat.class: muaxzcat.java java102rt.jar
	$(JAVAC) -Xlint:-options -target 1.1 -source 1.2 -bootclasspath java102rt.jar muaxzcat.java

# Regenerates muxzcat.pl and muxzcat.java.
generate: muaxzcat.pl muaxzcat.java muaxzcat.java
	cp -a muaxzcat.pl muxzcat.pl
	perl -pe "s@^public class muaxzcat @public class muxzcat @" <muaxzcat.java >muxzcat.java
	$(JAVAC) -Xlint:-options -target 1.1 -source 1.2 -bootclasspath java102rt.jar muxzcat.java  # Creates muxzcat.class

clean:
	rm -f muxzcat muaxzcat
