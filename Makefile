CC = gcc
.PHONY: all clean
all: muxzcat #muaxzcat

muxzcat: muxzcat.c
	$(CC) -ansi -s -O2 -W -Wall -Wextra -o $@ $<

muaxzcat: muaxzcat.c
	$(CC) -ansi -O2 -W -Wall -Wextra -o $@ $<

clean:
	rm -f muxzcat muaxzcat
