/*
 * Build instructions:
 *
 * $ wget -O xz-java-1.8.zip https://tukaani.org/xz/xz-java-1.8.zip
 * $ unzip -o xz-java-1.8.zip
 * $ perl -pi -e 's@\bassert\b.*?;@;@g' `find src -type f -name '*.java'`
 * $ (cd src && cp ../muxzcatj.java ./)
 * # Works with javac 1.8.
 * $ (cd src && javac -target 1.1 -source 1.2 muxzcatj.java)
 * $ mkdir -p src/META-INF
 * $ (echo 'Manifest-Version: 1.0'; echo 'Main-Class: muxzcatj') >src/META-INF/MANIFEST.MF
 * $ rm -f muxzcatj11.jar
 * $ (cd src && zip -9r ../muxzcatj11.jar META-INF/MANIFEST.MF `find * -type f -name '*.class'`)
 * $ advzip -z -4 muxzcatj11.jar
 * $ ls -l muxzcatj11.jar
 * -rw-r--r-- 1 pts pts 48504 Feb 10 22:52 muxzcatj11.jar
 */

import java.io.BufferedInputStream;
import java.io.IOException;
import org.tukaani.xz.XZInputStream;

public class muxzcatj {
  public static void main(String args[]) throws IOException {
    BufferedInputStream in = new BufferedInputStream(System.in);
    XZInputStream xzIn = new XZInputStream(in);
    try {
      final byte[] buffer = new byte[65536];
      int n = 0;
      while (-1 != (n = xzIn.read(buffer))) {
        System.out.write(buffer, 0, n);
      } 
    } finally {
      xzIn.close();
    }
  }
}