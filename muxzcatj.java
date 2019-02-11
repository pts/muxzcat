/*
 * Build instructions:
 *
 * $ wget -O xz-java-1.8.zip https://tukaani.org/xz/xz-java-1.8.zip
 * $ unzip -o xz-java-1.8.zip
 * $ perl -pi -e 's@\bassert\b.*?;@;@g' `find src -type f -name '*.java'`
 * $ (cd src && cp ../muxzcatj.java ./)
 * # Works with javac 1.8.
 * $ (cd src && javac -target 1.1 -source 1.2 -bootclasspath jdk1.2.2/jre/lib/rt.jar muxzcatj.java) ||
 *   (cd src && javac -target 1.1 -source 1.2 muxzcatj.java)
 * $ mkdir -p src/META-INF
 * $ (echo 'Manifest-Version: 1.0'; echo 'Main-Class: muxzcatj') >src/META-INF/MANIFEST.MF
 * $ rm -f muxzcatj12.jar
 * $ (cd src && zip -9r ../muxzcatj12.jar META-INF/MANIFEST.MF `find * -type f -name '*.class'`)
 * $ advzip -z -4 muxzcatj12.jar
 * $ ls -l muxzcatj12.jar
 * -rw-r--r-- 1 pts eng 52462 Feb 11 23:59 muxzcatj12.jar
 */

import java.io.BufferedInputStream;
import java.io.InputStream;
import java.io.PrintStream;
import java.io.IOException;
import org.tukaani.xz.LZMAInputStream;
import org.tukaani.xz.XZInputStream;

public class muxzcatj {
  private static final int SZ_ERROR_READ = 8;
  private static final int SZ_ERROR_WRITE = 9;
  private static final int SZ_ERROR_BAD_MAGIC = 51;
  private static final int DIC_ARRAY_SIZE = 1610612736;
  private static final int LZMA_DIC_MIN = 4096;
  private static final int getLe4(byte buffer[], int i) {
    return (buffer[i] & 0xff) | ((buffer[i + 1] & 0xff) << 8) |
           ((buffer[i + 2] & 0xff) << 16) | ((buffer[i + 3] & 0xff) << 24);
  }
  public static void main(String args[]) {
    try {
      BufferedInputStream in = new BufferedInputStream(System.in);
      final byte[] buffer = new byte[65536];
      in.mark(14);
      if (in.read(buffer, 0, 14) != 14) System.exit(SZ_ERROR_BAD_MAGIC);
      in.reset();
      InputStream cin;
      int bhf, dicSize;
      /* Same autodetection logic as in muxzcat.c and muaxzcat.c. */
      if (buffer[0] == (byte)0xfd && buffer[1] == 0x37 && buffer[2] == 0x7a &&
          buffer[3] == 0x58 && buffer[4] == 0x5a && buffer[5] == 0 &&
          buffer[6] == 0) {  /* .xz: "\xFD7zXZ\0\0" */
        cin = new XZInputStream(in);
      } else if ((buffer[0] & 0xff) <= 225 && buffer[13] == 0 &&
                 /* High 4 bytes of uncompressed size: 0 or -1. */
                 ((bhf = getLe4(buffer, 9)) - 0x7fffffff <= -0x7fffffff) &&
                 (dicSize = getLe4(buffer, 1)) - 0x80000000 >=
                     LZMA_DIC_MIN - 0x80000000 &&
                 (dicSize = getLe4(buffer, 1)) - 0x80000000 <=
                     DIC_ARRAY_SIZE - 0x80000000) {
        cin = new LZMAInputStream(in);
      } else {
        System.exit(SZ_ERROR_BAD_MAGIC);
        return;  /* Pacify javac (cin is not initialized). */
      }
      final PrintStream out = System.out;
      int n;
      while (0 < (n = cin.read(buffer))) {
        out.write(buffer, 0, n);
        if (out.checkError()) System.exit(SZ_ERROR_WRITE);
      }
      /* We could close cin or in, but we don't bother. */
    } catch (IOException e) {
      System.exit(SZ_ERROR_READ);
    }
  }
}
