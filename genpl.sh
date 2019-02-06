#! /bin/bash --
# by pts@fazekas.hu at Sun Feb  3 14:50:42 CET 2019

set -ex
# Example #1: -DCONFIG_DEBUG
gcc -C -E -DCONFIG_LANG_PERL "$@" -ansi -O2 -W -Wall -Wextra -Werror muaxzcat.c >muaxzcat.pl.tmp1
(
echo '#! /usr/bin/env perl
BEGIN { $^W = 1 }
# Silence warning about goto-into-while.
BEGIN { eval { require warnings; unimport warnings qw(deprecated) }; }
use integer;
use strict;

' &&
<muaxzcat.c perl -ne '
    $_ = join("", <STDIN>);
    die "Missing numeric constants.\n" if !s@NUMERIC_CONSTANTS[^\n]*\n(.*?\n)#endif@@s;
    $_ = "$1\n";
    s@/[*].*?[*]/@ @sg;
    s@^#define\s+(\w+)\s+(.*?)\s*$@sub $1() { $2 }@mg;
    s@\s*\n\s*@\n@g;
    s@\A\s+@@;
    print "$_\n\n"' &&
<muaxzcat.pl.tmp1 perl -0777 -pe 's@\A.*START_PREPROCESSED\s+@@s') >muaxzcat.pl.tmp2 || exit "$?"
<muaxzcat.pl.tmp2 perl -0777 -pe 's@^#(?!!).*\n@@gm; sub cont($) { my $s = $_[0]; $s =~ s@\A\s+@@; $s =~ s@\s+\Z(?!\n)@@; $s =~ s@\n@\n# @g; $s } s@/[*](.*?)[*]/\n*@ "# " . cont($1) . "\n" @gse;
    1 while s@^([ \t]+)([^#\n]*)(#[^\n]*\n)(?=#)@$1$2$3$1@mg;  # Correct but slow to indent comment continuation lines.
    s@^[ \t]*GLOBAL @@mg' >muaxzcat.pl
# TODO(pts): Unindent some comments.
# TODO(pts): Remove empty `;' lines (DEBUG).
# TODO(pts): Keep empty lines above FUNC_ARG0(SRes, Decompress).

: genpl.sh OK.
