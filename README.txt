muxzcat: tiny and portable .xz and .lzma decompression filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
muxzcat is decompression filter for .xz and .lzma compressed files
implemented in C (also works in C++), Perl 5 and Java. muxzcat.c is
portable, platform-independent and backwards-compatible with old (pre-2000)
versions of Perl, Java, C, Linux and Windows. The C version is
size-optimized for Linux i386 and is self-contained: it uses only system
calls: read(2) and write(2) from the standard C library.

The following binaries are released on https://github.com/pts/muxzcat/releases :

* muxzcat: Linux i386 executable (also runs on Linux amd64)
* muxzcat.upx: Linux i386 executable, compressed with upxbc
* muxzcat.exe: Windows i386 Win32 executable using kernel32.dll only
* muxzcat.darwinc32: macOS i386 executable
* muxzcat.darwinc64: macOS amd64 (x86_64) executable
* muxzcat.pl: Perl script which works on Perl 5.004_04 or later
* muxzcat.class: Java command-line application which works on Java 1.0.2 or
  later
* muxzcatj12.jar: Java-with-lib command-line application which works on
  Java 1.2 or later

muxzcat.c is size-optimized for Linux i386 (also runs on Linux amd64) with
`xtiny gcc': the final statically linked executable is 7376 bytes, and with
upxbc (`upxbc --elftiny -f -o muxzcat.upx muxzcat') it can be compressed to
4678 bytes. (Compare it with xzcat-only busybox on Linux i386, which is >20
KiB.)

muxzcat.c is size-optimized for Windows i386 (also runs on Windows amd64)
with gcc-mingw32 and some command-line flags (see muxzcat.exe in the
Makefile): the final muxzcat.exe is 10240 bytes. (Compare it with xzdec.exe
in https://fossies.org/windows/misc/xz-5.2.4-windows.zip/ , which is >71
kB.)

To use the C implementation (muxzcat.c), either download the binary
executable from https://github.com/pts/muxzcat/releases or compile it from
source (see the beginning of muxzcat.c for compilation instructions), and
then run it with any of:

  $ ./muxzcat <input.xz >output.bin
  $ ./muxzcat <input.lzma >output.bin

  Error is indicated as a non-zero exit status.

  It ignores command-line flags, so you can specify e.g. `-cd'.

Here is how to use the Perl implementation (muxzcat.pl):

  $ perl muxzcat.pl <input.xz >output.bin
  $ perl muxzcat.pl <input.lzma >output.bin

  Error is indicated as a non-zero exit status.

  It ignores command-line flags, so you can specify e.g. `-cd'.

Here is how to use the Java implementation (muxzcat.java). After
compilation or downloading of muxzcat.class, run with any of:

  $ java muxzcat <input.xz >output.bin
  $ java muxzcat <input.lzma >output.bin

  Error is indicated as a non-zero exit status.

  It ignores command-line flags, so you can specify e.g. `-cd'.

Here is how to use the Java-with-lib implementation:

  $ java -jar muxzcatj12.jar <input.xz >output.bin

  Error is indicated as a non-zero exit status and an exception report on
  stderr.

  It ignores command-line flags, so you can specify e.g. `-cd'.

muxzcat is a drop-in replacement for the following commands:

  $ xz -cd              <input.xz >output.bin
  $ unxz -cd            <input.xz >output.bin
  $ xzcat -cd           <input.xz >output.bin
  $ xzdec -cd           <input.xz >output.bin
  $ busybox xz -cd      <input.xz >output.bin
  $ busybox unxz -cd    <input.xz >output.bin
  $ busybox xzcat -cd   <input.xz >output.bin
  $ xz -cd              <input.lzma >output.bin
  $ unxz -cd            <input.lzma >output.bin
  $ xzcat -cd           <input.lzma >output.bin
  $ lzma -cd            <input.lzma >output.bin
  $ unlzma -cd          <input.lzma >output.bin
  $ lzmadec -cd         <input.lzma >output.bin
  $ busybox lzma -cd    <input.lzma >output.bin
  $ busybox unlzma -cd  <input.lzma >output.bin
  $ busybox lzmadec -cd <input.lzma >output.bin

muxzcat is free software, GNU GPL >=2.0. There is NO WARRANTY. Use at your risk.

Limitations of muxzcat.c, muxzcat.pl and muxzcat.java:

* In worst case it keeps 2 times the compression dictionary size in
  dynamic memory (also multiply it by 3 for realloc overhead), and it
  needs 130 KiB of memory on top of it: readBuf is about 64 KiB,
  CLzmaDec.prob is about 28 KiB, the rest is decompressBuf (containing
  the entire uncompressed data) and a small constant overhead.
* It doesn't support dictionary sizes larger than 1610612736 (~1.61 GB).
  The default for xz (-6) is 8 MiB.
  (This is not a problem in practice, because even the ouput of `xz -9e'
  uses only 64 MiB dictionary size.)
* It doesn't support uncompressed data larger than 1610612736 (~1.61 GB).
* For .xz it supports only LZMA2 (no other filters such as BCJ).
* For .lzma it doesn't work with files with 5 <= lc + lp <= 12.
* It doesn't verify checksums (e.g. CRC-32 or CRC-64).
* It extracts the first stream only, and it ignores the index.
* muaxzcat.c, muxzcat.java and muxzcat.pl keep the entire uncompressed
  output data in memory, and they have a limit of 1610612736 (~1.61 GB).
  FYI linux-4.20.5.tar is about half as much, 854855680 bytes.
* muxzcat.java doesn't work with Avian 0.6 (OutOfMemoryError).
* muxzcat.java uses the maximum amount of memory (~1.61 GB) for some .lzma
  files which don't have their uncompressed size specified. This includes
  files created by `xz --format=lzma'.

Limitations of Java-with-lib muxzcatj12.jar:

* It doesn't work with Avian 0.6 (it uses some classes not available there).
* Its memory usage is constant + 100 KiB + dictionary size, so it doesn't
  keep the entire uncompressed data in memory.

muxzcat is portable because:

* It works with old (pre-2000) C and C++ compilers.
* It works on old (pre-2000) Linux systems: Linux 2.6 and later.
* It works on old (pre-2000) Windows systems: Windows 95 and later.
* It works on old macOS systems: Mac OS X 10.04 (released on 2005-04-29, the
  first release which supports Intel CPUs: i386 and amd64) and later.
* It has good library compatibility by not using any libraries on Linux
  (not even libc.so.6) and using only 4 functions in kernel32.dll on
  Windows.
* It works with old (pre-2000) versions of Java: the minimum is Java 1.0.2
  (released on 1995-09-16).
* It works with old (pre-2000) versions of Perl: the minimum is
  Perl 5.004_04 (released on 1997-10-15).

Which Java program to use: muxzcat.java or muxzcatj12.jar?

* Most users should use muxzcatj12.jar, because it needs less memory, it is
  faster than muxzcat.java, and it also verifies checksums.
* On very old systems where Java >=1.0.2 is available, but Java >=1.2 isn't,
  muxzcat.java should be used.
* If maximum compatibility with muaxzcat.c (also muxzcat.c) and muzcat.pl is
  desired, then muxzcat.java should be used.

Based on decompression speed measurements of linux-4.20.5.tar.xz,
size-optimized muxzcat.c (on Linux i386) is about 1.07 times slower than
speed-optimized xzcat (on Linux amd64).

Based on decompression speed measurements of a ~2 MiB .tar.xz file and of
the ~100 MiB linux-4.20.5.tar.xz, size-optimized muxzcat.c (on Linux i386)
is about 285 times faster than muxzcat.pl (on perl compiled for Linux
amd64). The C and Perl implementations are derived from the same codebase
(muxzcat.c, itself derived from the sources files in 7z922.tar.bz2).
Part of the slowness is because LZMA decompression needs 32-bit
unsigned arithmetic, and perl compiled for Linux amd64 can do 64-bit signed
arithmetic, so the inputs of some operators (e.g. >>, <, ==) need to be
bit-masked to get correct results. (Fortunately % and / are not used in LZMA
decompression, because they would be even slower when emulated on the wrong
signedness.) muxzcat.pl is even slower (by a factor of 1.1017) than that on
Perls which can do 32-bit signed aritmetic, because special handling is
needed for negative inputs of <.

More information about decompression speed differences between C and Perl:
https://ptspts.blogspot.com/2019/02/speed-of-in-memory-algorithms-in.html

muxzcat.pl is compatible with recent versions of Perl 5 (e.g. Perl 5.24) and
very old versions of Perl 5 (e.g. Perl 5.004_04, released on 1997-10-15).

Based on decompression speed measurements of the ~100 MiB
linux-4.20.5.tar.xz, size-optimized muxzcat.c (on Linux i386) is about 1.347
times faster than muxzcatj12.jar (with java 1.8 compiled for Linux amd64).
The C and Java implementations are completely different, they don't share
code.

muxzcatj12.jar needs Java 1.2 (released on 1998-12) or any more recent Java,
e.g. Java 8. Versions earlier than 1.2 don't work, because they lack
java.util.Arrays.{equals,fill}.

Based on decompression speed measurements of the ~100 MiB
linux-4.20.5.tar.xz, size-optimized muxzcat.c (on Linux i386) is about 1.439
times faster than muxzcat.java (with java 1.8 compiled for Linux amd64).
The C and Java implementations are derived from the same codebase
(muxzcat.c, itself derived from the sources files in 7z922.tar.bz2).

muxzcat.java needs Java 1.0.2 (released on 1995-09-16) or any more recent
Java, e.g. Java 8.

If you need a tiny decompressor for .gz, .zip and Flate compressed
files implemented in C, see https://github.com/pts/pts-zcat .

If you need a tiny extractor and self-extractor for .7z archives implemented
in C, see https://github.com/pts/pts-tiny-7z-sfx .

__END__
