#! /bin/bash --
# by pts@fazekas.hu at Sun Feb  3 14:50:42 CET 2019

set -ex
# Example #1: -DCONFIG_DEBUG
gcc -C -E -DCONFIG_LANG_JAVA "$@" -ansi -O2 -W -Wall -Wextra -Werror muaxzcat.c >muaxzcat.java.tmp1
<muaxzcat.java.tmp1 perl -0777 -pe 's@\A.*START_PREPROCESSED\s+@@s' >muaxzcat.java.tmp2
# !! Reuse numeric constants.
(<muaxzcat.java.tmp2 perl -0777 -pe 's@^#(?!!).*\n@@gm; s@^[ \t]*;[ \t]*\n@@mg; s@(\n[ t]*)(?=\n)@@g') >muaxzcat.java || exit "$?"
# Don't do it automatically.
: eval 'perl -pe "s@^public class muaxzcat @public class muxzcat @" <muaxzcat.java >muxzcat.java'
#perl -pe 's@^public class muaxzcat @public class muxzcat @' <muaxzcat.java >muxzcat.java
if test "$1"; then
  BCP=
  test -f java102rt.jar && BCP='-bootclasspath java102rt.jar'
  # Works with Java 1.8.
  javac -Xlint:-options -target 1.1 -source 1.2 $BCP muaxzcat.java
  : javac -Xlint:-options -target 1.1 -source 1.2 $BCP muxzcat.java
  ls -l muaxzcat.class
fi

: genjava.sh OK.
