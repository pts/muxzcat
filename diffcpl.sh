#! /bin/bash
#
# diffcpl.sh: show a variable diff between the C and the Perl implementation ot muaxzcat.c
# by pts@fazekas.hu at Wed Feb  6 14:12:48 CET 2019
#
# This script generates cvars.out and plvars.out, each of which contains:
#
# * For each change in a GLOBAL, a SET_GLOBAL line with an even @setid number
#   identifying the code location.
# * For each change in a GLOBAL_ARY16 element, a SET_ARY16 line.
# * For each change in a GLOBAL_ARY8 element, a SET_ARY8 line.
# * For some changes in a LOCAL, a SET_LOCAL line with an odd @setid number
#   identifying the code location.
#
# Required instrumentation in muaxzcat.c: 
#
# 1. All instances of GLOBAL_VAR changes replaced by SET_GLOBAL:
#    perl -pi -we 'our $c; BEGIN { $c = 0; }; s@GLOBAL_VAR\((\w+)\) *(?!==|!=|<=|>=)([^\s\w]*=) *@$c += 2; "SET_GLOBAL($1, $c, $2) "@ge;' muaxzcat.c
# 2. Some instances of LOCAL_VAR changes replaced by SET_LOCALB:
#    perl -0777 -pi -we 'our $c; BEGIN { $c = 1; }; 1 while s@([{};=]\s*)LOCAL_VAR\((\w+)\)\s*(?!>=|<=|!=|==)([^\s\w]*=)\s*(?![^;{}]*=[^,])([^;{}]+)(?=;)@$c += 2; "${1}SET_LOCALB($2, $c, $3, $4) "@ge' muaxzcat.c
#

function die() {
  echo "fatal: $*" >&2
  exit 1
}

set -x  # Echo.

gcc -DCONFIG_DEBUG_VARS -ansi -s -Os -W -Wall -Wextra -Werror -o muxzcat muaxzcat.c || die "COMPILE"
./muxzcat <xa8.tar.xz >xa8.tar.dec 2>/dev/null || die "ERROR:$?"  # This is very slow (several minutes).
cmp xa.tar xa8.tar.dec || die "CMP"
TRANSFORM_PL='
    our %g; our $lc; our @buf; BEGIN { $lc = 20000 } exit if !--$lc;
    if (m@^DEBUG: GLOBALS (.*)$@) { $_ = $1; pos($_) = 0; while (m@\G(\S+)=(\S+)\s*@gc) { $g{$1} = $2 }; for (@buf) { if (m@^DEBUG: SET_GLOBAL (\S+) (\@\S+)@) { $_ = "DEBUG: SET_GLOBAL $2 $1=$g{$1}\n" } print }; @buf = (); }
    elsif (m@^DEBUG: (?:SET_GLOBAL|SET_ARY8|SET_ARY16|SET_LOCAL) @) { push @buf, $_ }
    END { print @buf }'
./muxzcat <xa8.tar.xz 2>&1 >xa8.tar.dec | perl -wne "$TRANSFORM_PL" >cvars.out || die "DUMP"
: less cvars.out
./genpl.sh -DCONFIG_DEBUG_VARS || die "GENPLSH"
perl ./muaxzcat.pl <xa8.tar.xz 2>&1 >xa8.tar.dec | perl -wne "$TRANSFORM_PL" >plvars.out || die "DUMP"
: less plvars.out
: diff -U10 cvars.out plvars.out

: "$0" OK.
