#! /bin/bash --
# by pts@fazekas.hu at Sun Feb  3 14:50:42 CET 2019

set -ex
# Example #1: -DCONFIG_DEBUG
gcc -C -E -DCONFIG_LANG_JAVA "$@" -ansi -O2 -W -Wall -Wextra -Werror muaxzcat.c >muaxzcat.java.tmp1
<muaxzcat.java.tmp1 perl -0777 -pe 's@\A.*START_PREPROCESSED\s+@@s' >muaxzcat.java.tmp2
# !! Reuse numeric constants.
(<muaxzcat.java.tmp2 perl -0777 -pe 's@^#(?!!).*\n@@gm; s@^[ \t]*;[ \t]*\n@@mg') >muaxzcat.java || exit "$?"
javac -target 1.1 -source 1.2 muaxzcat.java
: eval 'perl -pe "s@^public class muaxzcat @public class muxzcat @" <muaxzcat.java >muxzcat.java'
: javac -target 1.1 -source 1.2 muxzcat.java

: genjava.sh OK.
