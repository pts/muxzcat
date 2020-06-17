CC = gcc
.PHONY: all clean generate
all: muxzcat #muaxzcat

muxzcat: muxzcat.c
	$(CC) -ansi -s -O2 -W -Wall -Wextra -o $@ $<

muaxzcat: muaxzcat.c
	$(CC) -ansi -O2 -W -Wall -Wextra -o $@ $<

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
