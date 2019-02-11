/* In Perl, BREAK and CONTINUE don't work (it silently goes to outer.
 * loops) in a do { ... } while (...) loop. So use `goto' instead of
 * BREAK and CONTINUE in such loops.
 *
 * !! Is a `for (;;) { ...; last unless ...; }' loop faster than a do-while
 *    loop in Perl?
 */
/* TODO(pts): Simplify doublings: e.g.
 * LOCAL_VAR(drSymbol) = (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol))
 * SET_LOCALB(distance, 251, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)));
 * SET_LOCALB(distance, 257, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)) + 1);
  */
/* The code doesn't have overflowing / /= % %=, so we don't create macros for these. */
/* These work only if IS_SMALL(x) and 0 <= y <= 31. */
/* These work only if IS_SMALL(x) && IS_SMALL(y). */
/* genpl.sh has the 32-bit (slow) and 64-bit (fast) implementations of
 * EQ0, NE0 and LT.
 */
/* --- */
/* --- */
/*#define SZ_MAYBE_FINISHED_WITHOUT_MARK SZ_OK*/ /* LzmaDec_DecodeToDic, there is probability that stream was finished without end mark */
/* 6 is maximum LZMA chunk header size.
 * 65536 is maximum cs (compressed size) of LZMA2 chunk.
 * 6 is maximum LZMA chunk header size for the next chunk.
 */
/* --- */
/* For LZMA streams, LE_SMALL(lc + lp, 8 + 4), LE 12.
 * For LZMA2 streams, LE_SMALL(lc + lp, 4).
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: sizeof(GET_ARY16(probs, 0)) * value == (2 or 4) * value bytes.
 */
/*#define LzmaProps_GetNumProbs(p) TRUNCATE_TO_32BIT(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp))) */
public class muxzcat {
public static void EnsureDicSize() {
  int newCapacity = dic8.length;
  while (newCapacity > 0 && ((newCapacity) < (GLOBAL_dicBufSize))) {
    newCapacity <<= 1;
  }
  if (newCapacity < 0 || ((1610612736) < (newCapacity))) {
    newCapacity = 1610612736;
  }
  if (((dic8.length) < (newCapacity))) {
    /*final*/ byte newDic[] = new byte[newCapacity];
    System.arraycopy(dic8, 0, newDic, 0, GLOBAL_dicPos);
    dic8 = newDic;
  }
}
  static int GLOBAL_bufCur;
  static int GLOBAL_dicSize; /* Configured in prop byte. */
  static int GLOBAL_range;
  static int GLOBAL_code;
  static int GLOBAL_dicPos;
  static int GLOBAL_dicBufSize;
  static int GLOBAL_processedPos;
  static int GLOBAL_checkDicSize;
  static int GLOBAL_state;
  static int GLOBAL_rep0;
  static int GLOBAL_rep1;
  static int GLOBAL_rep2;
  static int GLOBAL_rep3;
  static int GLOBAL_remainLen;
  static int GLOBAL_tempBufSize;
  static int GLOBAL_readCur; /* Index within (or at end of) readBuf. */
  static int GLOBAL_readEnd; /* Index within (or at end of) readBuf. */
  static int GLOBAL_needFlush;
  static int GLOBAL_needInitLzma;
  static int GLOBAL_needInitDic;
  static int GLOBAL_needInitState;
  static int GLOBAL_needInitProp;
  /* lc, lp and pb would fit into a byte, but i386 code is shorter as UInt32.
   *
   * Constraints:
   *
   * * (0 <= lc <= 8) by LZMA.
   * * 0 <= lc <= 4 by LZMA2 and muxzcat.
   * * 0 <= lp <= 4.
   * * 0 <= pb <= 4.
   * * (0 <= lc + lp == 8 + 4 <= 12) by LZMA.
   * * 0 <= lc + lp <= 4 by LZMA2 and muxzcat.
   */
  static int GLOBAL_lc; /* Configured in prop byte. */
  static int GLOBAL_lp; /* Configured in prop byte. */
  static int GLOBAL_pb; /* Configured in prop byte. */
  static int GLOBAL_lcm8; /* Cached (8 - lc), for speed. */
  static short probs16[]; /* Probabilities for bit decoding. */
  /* The first READBUF_SIZE bytes is readBuf, then the
   * LZMA_REQUIRED_INPUT_MAX bytes is tempBuf.
   */
  static byte readBuf8[];
  /* Contains the uncompressed data.
   *
   * Array size is about 1.61 GB.
   * We rely on virtual memory so that if we don't use the end of array for
   * small files, then the operating system won't take the entire array away
   * from other processes.
   */
  static byte dic8[];
/* --- */
static final void LzmaDec_WriteRem(int LOCAL_wrDicLimit) {
  if (((GLOBAL_remainLen) != (0)) && ((GLOBAL_remainLen) < (274))) {
    int LOCAL_wrLen = GLOBAL_remainLen;
    if (((LOCAL_wrDicLimit - GLOBAL_dicPos) < (LOCAL_wrLen))) {
      LOCAL_wrLen = LOCAL_wrDicLimit - GLOBAL_dicPos ;
    }
    if (((GLOBAL_checkDicSize) == (0)) && ((GLOBAL_dicSize - GLOBAL_processedPos) <= (LOCAL_wrLen))) {
      GLOBAL_checkDicSize = GLOBAL_dicSize;
    }
    GLOBAL_processedPos += LOCAL_wrLen;
    GLOBAL_remainLen -= LOCAL_wrLen;
    while (((LOCAL_wrLen) != (0))) {
      LOCAL_wrLen--;
      dic8[GLOBAL_dicPos] = (byte)((dic8[(GLOBAL_dicPos - GLOBAL_rep0) + (((GLOBAL_dicPos) < (GLOBAL_rep0)) ? GLOBAL_dicBufSize : 0)] & 0xff));
      GLOBAL_dicPos++;
    }
  }
}
/* Modifies GLOBAL_VAR(bufCur) etc. */
static final int LzmaDec_DecodeReal2(int LOCAL_drDicLimit, int LOCAL_drBufLimit) {
  int LOCAL_pbMask = ((1) << (GLOBAL_pb)) - 1;
  int LOCAL_lpMask = ((1) << (GLOBAL_lp)) - 1;
  int LOCAL_drI;
  do {
    int LOCAL_drDicLimit2 = ((GLOBAL_checkDicSize) == (0)) && ((GLOBAL_dicSize - GLOBAL_processedPos) < (LOCAL_drDicLimit - GLOBAL_dicPos)) ? GLOBAL_dicPos + (GLOBAL_dicSize - GLOBAL_processedPos) : LOCAL_drDicLimit;
    GLOBAL_remainLen = 0;
    do {
      int LOCAL_drProbIdx;
      int LOCAL_drBound;
      int LOCAL_drTtt; /* 0 <= LOCAL_VAR(drTtt) <= kBitModelTotal. */
      int LOCAL_distance;
      int LOCAL_drPosState = GLOBAL_processedPos & LOCAL_pbMask;
      LOCAL_drProbIdx = 0 + (GLOBAL_state << (4)) + LOCAL_drPosState ;
      LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
      if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
        int LOCAL_drSymbol;
        GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + (((2048 - LOCAL_drTtt) >> (5))));
        LOCAL_drProbIdx = 1846 ;
        if (((GLOBAL_checkDicSize) != (0)) || ((GLOBAL_processedPos) != (0))) {
          LOCAL_drProbIdx += (768 * (((GLOBAL_processedPos & LOCAL_lpMask) << GLOBAL_lc) + (((dic8[(((GLOBAL_dicPos) == (0)) ? GLOBAL_dicBufSize : GLOBAL_dicPos) - 1] & 0xff)) >> (GLOBAL_lcm8)))) ;
        }
        if (((GLOBAL_state) < (7))) {
          GLOBAL_state -= (((GLOBAL_state) < (4))) ? GLOBAL_state : 3;
          LOCAL_drSymbol = 1 ;
          do {
            LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drSymbol] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = (((GLOBAL_range) >>> 11)) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drSymbol] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drSymbol = (LOCAL_drSymbol + LOCAL_drSymbol); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drSymbol] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drSymbol = (LOCAL_drSymbol + LOCAL_drSymbol) + 1; }
          } while (((LOCAL_drSymbol) < (0x100)));
        } else {
          int LOCAL_drMatchByte = (dic8[(GLOBAL_dicPos - GLOBAL_rep0) + (((GLOBAL_dicPos) < (GLOBAL_rep0)) ? GLOBAL_dicBufSize : 0)] & 0xff);
          int LOCAL_drMatchMask = 0x100; /* 0 or 0x100. */
          GLOBAL_state -= ((GLOBAL_state) < (10)) ? 3 : 6;
          LOCAL_drSymbol = 1 ;
          do {
            int LOCAL_drBit;
            int LOCAL_drProbLitIdx;
            LOCAL_drMatchByte <<= 1 ;
            LOCAL_drBit = (LOCAL_drMatchByte & LOCAL_drMatchMask) ;
            LOCAL_drProbLitIdx = LOCAL_drProbIdx + LOCAL_drMatchMask + LOCAL_drBit + LOCAL_drSymbol ;
            LOCAL_drTtt = (probs16[LOCAL_drProbLitIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbLitIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drSymbol = (LOCAL_drSymbol + LOCAL_drSymbol) ; LOCAL_drMatchMask &= ~LOCAL_drBit ; } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbLitIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drSymbol = (LOCAL_drSymbol + LOCAL_drSymbol) + 1 ; LOCAL_drMatchMask &= LOCAL_drBit ; }
          } while (((LOCAL_drSymbol) < (0x100)));
        }
        dic8[GLOBAL_dicPos++] = (byte)(LOCAL_drSymbol);
        GLOBAL_processedPos++;
        continue;
      } else {
        GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
        LOCAL_drProbIdx = 192 + GLOBAL_state ;
        LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
        if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
          GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
          GLOBAL_state += 12;
          LOCAL_drProbIdx = 818 ;
        } else {
          GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
          if (((GLOBAL_checkDicSize) == (0)) && ((GLOBAL_processedPos) == (0))) {
            return 1;
          }
          LOCAL_drProbIdx = 204 + GLOBAL_state ;
          LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
          if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
            GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
            LOCAL_drProbIdx = 240 + (GLOBAL_state << (4)) + LOCAL_drPosState ;
            LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
            if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
              GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
              dic8[GLOBAL_dicPos] = (byte)((dic8[(GLOBAL_dicPos - GLOBAL_rep0) + (((GLOBAL_dicPos) < (GLOBAL_rep0)) ? GLOBAL_dicBufSize : 0)] & 0xff));
              GLOBAL_dicPos++;
              GLOBAL_processedPos++;
              GLOBAL_state = ((GLOBAL_state) < (7)) ? 9 : 11;
              continue;
            }
            GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
          } else {
            GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
            LOCAL_drProbIdx = 216 + GLOBAL_state ;
            LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
            if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
              GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
              LOCAL_distance = GLOBAL_rep1 ;
            } else {
              GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
              LOCAL_drProbIdx = 228 + GLOBAL_state ;
              LOCAL_drTtt = (probs16[LOCAL_drProbIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
              if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
                GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
                LOCAL_distance = GLOBAL_rep2 ;
              } else {
                GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
                LOCAL_distance = GLOBAL_rep3 ;
                GLOBAL_rep3 = GLOBAL_rep2;
              }
              GLOBAL_rep2 = GLOBAL_rep1;
            }
            GLOBAL_rep1 = GLOBAL_rep0;
            GLOBAL_rep0 = LOCAL_distance;
          }
          GLOBAL_state = ((GLOBAL_state) < (7)) ? 8 : 11;
          LOCAL_drProbIdx = 1332 ;
        }
        {
          int LOCAL_drLimitSub;
          int LOCAL_drOffset;
          int LOCAL_drProbLenIdx = LOCAL_drProbIdx + 0;
          LOCAL_drTtt = (probs16[LOCAL_drProbLenIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
          if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
            GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbLenIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
            LOCAL_drProbLenIdx = LOCAL_drProbIdx + 2 + (LOCAL_drPosState << (3)) ;
            LOCAL_drOffset = 0 ;
            LOCAL_drLimitSub = ((1) << (3)) ;
          } else {
            GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbLenIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
            LOCAL_drProbLenIdx = LOCAL_drProbIdx + 1 ;
            LOCAL_drTtt = (probs16[LOCAL_drProbLenIdx] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ;
            if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) {
              GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbLenIdx] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5)));
              LOCAL_drProbLenIdx = LOCAL_drProbIdx + 130 + (LOCAL_drPosState << (3)) ;
              LOCAL_drOffset = 8 ;
              LOCAL_drLimitSub = (1) << (3) ;
            } else {
              GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbLenIdx] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5)));
              LOCAL_drProbLenIdx = LOCAL_drProbIdx + 258 ;
              LOCAL_drOffset = 8 + 8 ;
              LOCAL_drLimitSub = (1) << (8) ;
            }
          }
          {
            GLOBAL_remainLen = (1) ;
            do {
              { LOCAL_drTtt = (probs16[(LOCAL_drProbLenIdx + GLOBAL_remainLen)] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[(LOCAL_drProbLenIdx + GLOBAL_remainLen)] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); GLOBAL_remainLen = ((GLOBAL_remainLen + GLOBAL_remainLen)); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[(LOCAL_drProbLenIdx + GLOBAL_remainLen)] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); GLOBAL_remainLen = ((GLOBAL_remainLen + GLOBAL_remainLen) + 1); } }
            } while (((GLOBAL_remainLen) < (LOCAL_drLimitSub)));
            GLOBAL_remainLen -= (LOCAL_drLimitSub) ;
          }
          GLOBAL_remainLen += (LOCAL_drOffset) ;
        }
        if (((GLOBAL_state) >= (12))) {
          LOCAL_drProbIdx = 432 + ((((GLOBAL_remainLen) < (4)) ? GLOBAL_remainLen : 4 - 1) << (6)) ;
          {
            LOCAL_distance = 1 ;
            do {
              { LOCAL_drTtt = (probs16[(LOCAL_drProbIdx + LOCAL_distance)] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[(LOCAL_drProbIdx + LOCAL_distance)] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_distance = (LOCAL_distance + LOCAL_distance); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[(LOCAL_drProbIdx + LOCAL_distance)] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_distance = (LOCAL_distance + LOCAL_distance) + 1; } }
            } while (((LOCAL_distance) < ((1 << 6))));
            LOCAL_distance -= (1 << 6) ;
          }
          if (((LOCAL_distance) >= (4))) {
            int LOCAL_drPosSlot = LOCAL_distance;
            int LOCAL_drDirectBitCount = ((LOCAL_distance) >> (1)) - 1;
            LOCAL_distance = (2 | (LOCAL_distance & 1)) ;
            if (((LOCAL_drPosSlot) < (14))) {
              LOCAL_distance <<= LOCAL_drDirectBitCount ;
              LOCAL_drProbIdx = 688 + LOCAL_distance - LOCAL_drPosSlot - 1 ;
              {
                int LOCAL_mask = 1;
                LOCAL_drI = 1;
                do {
                  LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drI] & 0xffff) ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI) + 1 ; LOCAL_distance |= LOCAL_mask ; }
                  LOCAL_mask <<= 1 ;
                } while (((--LOCAL_drDirectBitCount) != (0)));
              }
            } else {
              LOCAL_drDirectBitCount -= 4 ;
              do {
                if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; }
                /* Here GLOBAL_VAR(range) can be non-small, so we can't use SHR_SMALLX instead of SHR1. */
                GLOBAL_range = (((GLOBAL_range) >>> 1));
                if ((((GLOBAL_code - GLOBAL_range) & 0x80000000) != 0)) {
                  LOCAL_distance <<= 1;
                } else {
                  GLOBAL_code -= (GLOBAL_range);
                  /* This won't be faster in Perl: <<= 1, ++ */
                  LOCAL_distance = (LOCAL_distance << 1) + 1;
                }
              } while (((--LOCAL_drDirectBitCount) != (0)));
              LOCAL_drProbIdx = 802 ;
              LOCAL_distance <<= 4 ;
              {
                LOCAL_drI = 1;
                LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drI] & 0xffff); ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI) + 1 ; LOCAL_distance |= 1 ; }
                LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drI] & 0xffff); ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI) + 1 ; LOCAL_distance |= 2 ; }
                LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drI] & 0xffff); ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI) + 1 ; LOCAL_distance |= 4 ; }
                LOCAL_drTtt = (probs16[LOCAL_drProbIdx + LOCAL_drI] & 0xffff); ; if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; } LOCAL_drBound = ((GLOBAL_range) >>> 11) * LOCAL_drTtt ; if (((GLOBAL_code) - 0x80000000 < (LOCAL_drBound) - 0x80000000)) { GLOBAL_range = (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt + ((2048 - LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI); } else { GLOBAL_range -= (LOCAL_drBound) ; GLOBAL_code -= (LOCAL_drBound) ; probs16[LOCAL_drProbIdx + LOCAL_drI] = (short)(LOCAL_drTtt - ((LOCAL_drTtt) >> (5))); LOCAL_drI = (LOCAL_drI + LOCAL_drI) + 1 ; LOCAL_distance |= 8 ; }
              }
              if (((~LOCAL_distance) == 0)) {
                GLOBAL_remainLen += (274) ;
                GLOBAL_state -= 12;
                break;
              }
            }
          }
          /* TODO(pts): Do the 2 instances of SZ_ERROR_DATA below also check this? */
          GLOBAL_rep3 = GLOBAL_rep2;
          GLOBAL_rep2 = GLOBAL_rep1;
          GLOBAL_rep1 = GLOBAL_rep0;
          GLOBAL_rep0 = LOCAL_distance + 1;
          if (((GLOBAL_checkDicSize) == (0))) {
            if (((LOCAL_distance) >= (GLOBAL_processedPos))) {
              return 1;
            }
          } else {
            if (((LOCAL_distance) >= (GLOBAL_checkDicSize))) {
              return 1;
            }
          }
          GLOBAL_state = ((GLOBAL_state) < (12 + 7)) ? 7 : 7 + 3;
        }
        GLOBAL_remainLen += (2) ;
        if (((LOCAL_drDicLimit2) == (GLOBAL_dicPos))) {
          return 1;
        }
        {
          int LOCAL_drRem = LOCAL_drDicLimit2 - GLOBAL_dicPos;
          int LOCAL_curLen = (((LOCAL_drRem) < (GLOBAL_remainLen)) ? LOCAL_drRem : GLOBAL_remainLen);
          int LOCAL_pos = (GLOBAL_dicPos - GLOBAL_rep0) + (((GLOBAL_dicPos) < (GLOBAL_rep0)) ? GLOBAL_dicBufSize : 0);
          GLOBAL_processedPos += LOCAL_curLen;
          GLOBAL_remainLen -= (LOCAL_curLen) ;
          if (((LOCAL_pos + LOCAL_curLen) <= (GLOBAL_dicBufSize))) {
            do {
              /* Here pos can be negative if 64-bit. */
              dic8[GLOBAL_dicPos++] = (byte)((dic8[LOCAL_pos++] & 0xff));
            } while (((--LOCAL_curLen) != (0)));
          } else {
            do {
              dic8[GLOBAL_dicPos++] = (byte)((dic8[LOCAL_pos++] & 0xff));
              if (((LOCAL_pos) == (GLOBAL_dicBufSize))) { LOCAL_pos = 0 ; }
            } while (((--LOCAL_curLen) != (0)));
          }
        }
      }
    } while (((GLOBAL_dicPos) < (LOCAL_drDicLimit2)) && ((GLOBAL_bufCur) < (LOCAL_drBufLimit)));
    if (((GLOBAL_range) - 0x80000000 < (16777216) - 0x80000000)) { GLOBAL_range <<= (8) ; GLOBAL_code = ((GLOBAL_code << 8) | ((readBuf8[GLOBAL_bufCur++] & 0xff))) ; }
    if (((GLOBAL_processedPos) >= (GLOBAL_dicSize))) {
      GLOBAL_checkDicSize = GLOBAL_dicSize;
    }
    LzmaDec_WriteRem(LOCAL_drDicLimit);
  } while (((GLOBAL_dicPos) < (LOCAL_drDicLimit)) && ((GLOBAL_bufCur) < (LOCAL_drBufLimit)) && ((GLOBAL_remainLen) < (274)));
  if (((GLOBAL_remainLen) > (274))) {
    GLOBAL_remainLen = 274;
  }
  return 0;
}
static final int LzmaDec_TryDummy(int LOCAL_tdCur, int LOCAL_tdBufLimit) {
  int LOCAL_tdRange = GLOBAL_range;
  int LOCAL_tdCode = GLOBAL_code;
  int LOCAL_tdState = GLOBAL_state;
  int LOCAL_tdRes;
  int LOCAL_tdProbIdx;
  int LOCAL_tdBound;
  int LOCAL_tdTtt;
  int LOCAL_tdPosState = (GLOBAL_processedPos) & ((1 << GLOBAL_pb) - 1);
  LOCAL_tdProbIdx = 0 + (LOCAL_tdState << (4)) + LOCAL_tdPosState ;
  LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
  if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
    int LOCAL_tdSymbol = 1;
    LOCAL_tdRange = LOCAL_tdBound ;
    LOCAL_tdProbIdx = 1846 ;
    if (((GLOBAL_checkDicSize) != (0)) || ((GLOBAL_processedPos) != (0))) {
      LOCAL_tdProbIdx += (768 * ((((GLOBAL_processedPos) & ((1 << (GLOBAL_lp)) - 1)) << GLOBAL_lc) + (((dic8[(((GLOBAL_dicPos) == (0)) ? GLOBAL_dicBufSize : GLOBAL_dicPos) - 1] & 0xff)) >> (GLOBAL_lcm8)))) ;
    }
    if (((LOCAL_tdState) < (7))) {
      do {
        LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx + LOCAL_tdSymbol] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ; if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) { LOCAL_tdRange = LOCAL_tdBound ; LOCAL_tdSymbol = (LOCAL_tdSymbol + LOCAL_tdSymbol); } else { LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ; LOCAL_tdSymbol = (LOCAL_tdSymbol + LOCAL_tdSymbol) + 1; }
      } while (((LOCAL_tdSymbol) < (0x100)));
    } else {
      int LOCAL_tdMatchByte = (dic8[GLOBAL_dicPos - GLOBAL_rep0 + (((GLOBAL_dicPos) < (GLOBAL_rep0)) ? GLOBAL_dicBufSize : 0)] & 0xff);
      int LOCAL_tdMatchMask = 0x100; /* 0 or 0x100. */
      do {
        int LOCAL_tdBit;
        int LOCAL_tdProbLitIdx;
        LOCAL_tdMatchByte <<= 1 ;
        LOCAL_tdBit = (LOCAL_tdMatchByte & LOCAL_tdMatchMask) ;
        LOCAL_tdProbLitIdx = LOCAL_tdProbIdx + LOCAL_tdMatchMask + LOCAL_tdBit + LOCAL_tdSymbol ;
        LOCAL_tdTtt = (probs16[LOCAL_tdProbLitIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ; if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) { LOCAL_tdRange = LOCAL_tdBound ; LOCAL_tdSymbol = (LOCAL_tdSymbol + LOCAL_tdSymbol) ; LOCAL_tdMatchMask &= ~LOCAL_tdBit ; } else { LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ; LOCAL_tdSymbol = (LOCAL_tdSymbol + LOCAL_tdSymbol) + 1 ; LOCAL_tdMatchMask &= LOCAL_tdBit ; }
      } while (((LOCAL_tdSymbol) < (0x100)));
    }
    LOCAL_tdRes = 1 ;
  } else {
    int LOCAL_tdLen;
    LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
    LOCAL_tdProbIdx = 192 + LOCAL_tdState ;
    LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
    if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
      LOCAL_tdRange = LOCAL_tdBound ;
      LOCAL_tdState = 0 ;
      LOCAL_tdProbIdx = 818 ;
      LOCAL_tdRes = 2 ;
    } else {
      LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
      LOCAL_tdRes = 3 ;
      LOCAL_tdProbIdx = 204 + LOCAL_tdState ;
      LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
      if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
        LOCAL_tdRange = LOCAL_tdBound ;
        LOCAL_tdProbIdx = 240 + (LOCAL_tdState << (4)) + LOCAL_tdPosState ;
        LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
        if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
          LOCAL_tdRange = LOCAL_tdBound ;
          if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; }
          return 3;
        } else {
          LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
        }
      } else {
        LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
        LOCAL_tdProbIdx = 216 + LOCAL_tdState ;
        LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
        if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
          LOCAL_tdRange = LOCAL_tdBound ;
        } else {
          LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
          LOCAL_tdProbIdx = 228 + LOCAL_tdState ;
          LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
          if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
            LOCAL_tdRange = LOCAL_tdBound ;
          } else {
            LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
          }
        }
      }
      LOCAL_tdState = 12 ;
      LOCAL_tdProbIdx = 1332 ;
    }
    {
      int LOCAL_tdLimitSub;
      int LOCAL_tdOffset;
      int LOCAL_tdProbLenIdx = LOCAL_tdProbIdx + 0;
      LOCAL_tdTtt = (probs16[LOCAL_tdProbLenIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
      if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
        LOCAL_tdRange = LOCAL_tdBound ;
        LOCAL_tdProbLenIdx = LOCAL_tdProbIdx + 2 + (LOCAL_tdPosState << (3)) ;
        LOCAL_tdOffset = 0 ;
        LOCAL_tdLimitSub = (1) << (3) ;
      } else {
        LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
        LOCAL_tdProbLenIdx = LOCAL_tdProbIdx + 1 ;
        LOCAL_tdTtt = (probs16[LOCAL_tdProbLenIdx] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ;
        if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) {
          LOCAL_tdRange = LOCAL_tdBound ;
          LOCAL_tdProbLenIdx = LOCAL_tdProbIdx + 130 + (LOCAL_tdPosState << (3)) ;
          LOCAL_tdOffset = 8 ;
          LOCAL_tdLimitSub = (1) << (3) ;
        } else {
          LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ;
          LOCAL_tdProbLenIdx = LOCAL_tdProbIdx + 258 ;
          LOCAL_tdOffset = 8 + 8 ;
          LOCAL_tdLimitSub = (1) << (8) ;
        }
      }
      {
        LOCAL_tdLen = 1 ;
        do {
          LOCAL_tdTtt = (probs16[LOCAL_tdProbLenIdx + LOCAL_tdLen] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ; if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) { LOCAL_tdRange = LOCAL_tdBound ; LOCAL_tdLen = (LOCAL_tdLen + LOCAL_tdLen); } else { LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ; LOCAL_tdLen = (LOCAL_tdLen + LOCAL_tdLen) + 1; }
        } while (((LOCAL_tdLen) < (LOCAL_tdLimitSub)));
        LOCAL_tdLen -= LOCAL_tdLimitSub ;
      }
      LOCAL_tdLen += LOCAL_tdOffset ;
    }
    if (((LOCAL_tdState) < (4))) {
      int LOCAL_tdPosSlot;
      LOCAL_tdProbIdx = 432 + ((((LOCAL_tdLen) < (4)) ? LOCAL_tdLen : 4 - 1) << (6)) ;
      {
        LOCAL_tdPosSlot = 1 ;
        do {
          LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx + LOCAL_tdPosSlot] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ; if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) { LOCAL_tdRange = LOCAL_tdBound ; LOCAL_tdPosSlot = (LOCAL_tdPosSlot + LOCAL_tdPosSlot); } else { LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ; LOCAL_tdPosSlot = (LOCAL_tdPosSlot + LOCAL_tdPosSlot) + 1; }
        } while (((LOCAL_tdPosSlot) < ((1) << (6))));
        LOCAL_tdPosSlot -= (1) << (6) ;
      }
      /* Small enough for SHR_SMALLX(LOCAL_VAR(tdPosSlot), ...). */
      if (((LOCAL_tdPosSlot) >= (4))) {
        int LOCAL_tdDirectBitCount = ((LOCAL_tdPosSlot) >> (1)) - 1;
        if (((LOCAL_tdPosSlot) < (14))) {
          LOCAL_tdProbIdx = 688 + ((2 | (LOCAL_tdPosSlot & 1)) << LOCAL_tdDirectBitCount) - LOCAL_tdPosSlot - 1 ;
        } else {
          LOCAL_tdDirectBitCount -= 4 ;
          do {
            if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; }
            LOCAL_tdRange = ((LOCAL_tdRange) >>> 1);
            if ((((LOCAL_tdCode - LOCAL_tdRange) & 0x80000000) == 0)) {
              LOCAL_tdCode -= LOCAL_tdRange;
            }
          } while (((--LOCAL_tdDirectBitCount) != (0)));
          LOCAL_tdProbIdx = 802 ;
          LOCAL_tdDirectBitCount = 4 ;
        }
        {
          int LOCAL_tdI = 1;
          do {
            LOCAL_tdTtt = (probs16[LOCAL_tdProbIdx + LOCAL_tdI] & 0xffff) ; if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; } LOCAL_tdBound = ((LOCAL_tdRange) >>> 11) * LOCAL_tdTtt ; if (((LOCAL_tdCode) - 0x80000000 < (LOCAL_tdBound) - 0x80000000)) { LOCAL_tdRange = LOCAL_tdBound ; LOCAL_tdI = (LOCAL_tdI + LOCAL_tdI); } else { LOCAL_tdRange -= LOCAL_tdBound ; LOCAL_tdCode -= LOCAL_tdBound ; LOCAL_tdI = (LOCAL_tdI + LOCAL_tdI) + 1; }
          } while (((--LOCAL_tdDirectBitCount) != (0)));
        }
      }
    }
  }
  if (((LOCAL_tdRange) - 0x80000000 < (16777216) - 0x80000000)) { if (((LOCAL_tdCur) >= (LOCAL_tdBufLimit))) { return 0; } LOCAL_tdRange <<= 8 ; LOCAL_tdCode = (LOCAL_tdCode << 8) | ((readBuf8[LOCAL_tdCur++] & 0xff)) ; }
  return LOCAL_tdRes;
}
static final void LzmaDec_InitDicAndState(int LOCAL_idInitDic, int LOCAL_idInitState) {
  GLOBAL_needFlush = 1;
  GLOBAL_remainLen = 0;
  GLOBAL_tempBufSize = 0;
  if (((LOCAL_idInitDic) != 0)) {
    GLOBAL_processedPos = 0;
    GLOBAL_checkDicSize = 0;
    GLOBAL_needInitLzma = 1;
  }
  if (((LOCAL_idInitState) != 0)) {
    GLOBAL_needInitLzma = 1;
  }
}
/* Decompress LZMA stream in
 * readBuf8[GLOBAL_VAR(readCur) : GLOBAL_VAR(readCur) + LOCAL_VAR(ddSrcLen)].
 * On success (and on some errors as well), adds LOCAL_VAR(ddSrcLen) to GLOBAL_VAR(readCur).
 */
static final int LzmaDec_DecodeToDic(int LOCAL_ddSrcLen) {
  /* Index limit in GLOBAL_VAR(readBuf). */
  int LOCAL_decodeLimit = GLOBAL_readCur + LOCAL_ddSrcLen;
  int LOCAL_checkEndMarkNow;
  int LOCAL_dummyRes;
  LzmaDec_WriteRem(GLOBAL_dicBufSize);
  while (((GLOBAL_remainLen) != (274))) {
    if (((GLOBAL_needFlush) != 0)) {
      /* Read 5 bytes (RC_INIT_SIZE) to tempBuf, first of which must be
       * 0, initialize the range coder with the 4 bytes after the 0 byte.
       */
      while (((LOCAL_decodeLimit) > (GLOBAL_readCur)) && ((GLOBAL_tempBufSize) < (5))) {
        readBuf8[(6 + 65536 + 6) + GLOBAL_tempBufSize++] = (byte)((readBuf8[GLOBAL_readCur++] & 0xff));
      }
      if (((GLOBAL_tempBufSize) < (5))) {
      }
      if ((((readBuf8[(6 + 65536 + 6)] & 0xff)) != (0))) {
        return 1;
      }
      GLOBAL_code = (((readBuf8[(6 + 65536 + 6) + 1] & 0xff)) << 24) | (((readBuf8[(6 + 65536 + 6) + 2] & 0xff)) << 16) | (((readBuf8[(6 + 65536 + 6) + 3] & 0xff)) << 8) | (((readBuf8[(6 + 65536 + 6) + 4] & 0xff)));
      GLOBAL_range = 0xffffffff;
      GLOBAL_needFlush = 0;
      GLOBAL_tempBufSize = 0;
    }
    LOCAL_checkEndMarkNow = 0 ;
    if (((GLOBAL_dicPos) >= (GLOBAL_dicBufSize))) {
      if (((GLOBAL_remainLen) == (0)) && ((GLOBAL_code) == 0)) {
        if (((GLOBAL_readCur) != (LOCAL_decodeLimit))) { return 18; }
        return 0 /* MAYBE_FINISHED_WITHOUT_MARK */;
      }
      if (((GLOBAL_remainLen) != (0))) {
        return 16;
      }
      LOCAL_checkEndMarkNow = 1 ;
    }
    if (((GLOBAL_needInitLzma) != 0)) {
      int LOCAL_numProbs = 1846 + ((768) << (GLOBAL_lc + GLOBAL_lp));
      int LOCAL_ddProbIdx;
      for (LOCAL_ddProbIdx = 0; ((LOCAL_ddProbIdx) < (LOCAL_numProbs)); LOCAL_ddProbIdx++) {
        probs16[LOCAL_ddProbIdx] = (short)(((2048) >> (1)));
      }
      GLOBAL_rep0 = GLOBAL_rep1 = GLOBAL_rep2 = GLOBAL_rep3 = 1;
      GLOBAL_state = 0;
      GLOBAL_needInitLzma = 0;
    }
    if (((GLOBAL_tempBufSize) == (0))) {
      int LOCAL_bufLimit;
      if (((LOCAL_decodeLimit - GLOBAL_readCur) < (20)) || ((LOCAL_checkEndMarkNow) != 0)) {
        LOCAL_dummyRes = LzmaDec_TryDummy(GLOBAL_readCur, LOCAL_decodeLimit) ;
        if (((LOCAL_dummyRes) == (0))) {
          /* This line can be triggered by passing LOCAL_VAR(ddSrcLen)=1 to LzmaDec_DecodeToDic. */
          GLOBAL_tempBufSize = 0;
          while (((GLOBAL_readCur) != (LOCAL_decodeLimit))) {
            readBuf8[(6 + 65536 + 6) + GLOBAL_tempBufSize++] = (byte)((readBuf8[GLOBAL_readCur++] & 0xff));
          }
          if (((GLOBAL_readCur) != (LOCAL_decodeLimit))) { return 17; }
          return 17;
        }
        if (((LOCAL_checkEndMarkNow) != 0) && ((LOCAL_dummyRes) != (2))) {
          return 16;
        }
        LOCAL_bufLimit = GLOBAL_readCur ;
      } else {
        LOCAL_bufLimit = LOCAL_decodeLimit - 20 ;
      }
      GLOBAL_bufCur = GLOBAL_readCur;
      if (((LzmaDec_DecodeReal2(GLOBAL_dicBufSize, LOCAL_bufLimit)) != (0))) {
        return 1;
      }
      GLOBAL_readCur = GLOBAL_bufCur;
    } else {
      int LOCAL_ddRem = GLOBAL_tempBufSize;
      int LOCAL_lookAhead = 0;
      while (((LOCAL_ddRem) < (20)) && ((LOCAL_lookAhead) < (LOCAL_decodeLimit - GLOBAL_readCur))) {
        readBuf8[(6 + 65536 + 6) + LOCAL_ddRem++] = (byte)((readBuf8[GLOBAL_readCur + LOCAL_lookAhead++] & 0xff));
      }
      GLOBAL_tempBufSize = LOCAL_ddRem;
      if (((LOCAL_ddRem) < (20)) || ((LOCAL_checkEndMarkNow) != 0)) {
        LOCAL_dummyRes = LzmaDec_TryDummy((6 + 65536 + 6), (6 + 65536 + 6) + LOCAL_ddRem) ;
        if (((LOCAL_dummyRes) == (0))) {
          GLOBAL_readCur += LOCAL_lookAhead;
          if (((GLOBAL_readCur) != (LOCAL_decodeLimit))) { return 17; }
          return 17;
        }
        if (((LOCAL_checkEndMarkNow) != 0) && ((LOCAL_dummyRes) != (2))) {
          return 16;
        }
      }
      /* This line can be triggered by passing LOCAL_VAR(ddSrcLen)=1 to LzmaDec_DecodeToDic. */
      GLOBAL_bufCur = (6 + 65536 + 6); /* tempBuf. */
      if (((LzmaDec_DecodeReal2(0, (6 + 65536 + 6))) != (0))) {
        return 1;
      }
      LOCAL_lookAhead -= LOCAL_ddRem - (GLOBAL_bufCur - (6 + 65536 + 6)) ;
      GLOBAL_readCur += LOCAL_lookAhead;
      GLOBAL_tempBufSize = 0;
    }
  }
  if (((GLOBAL_code) != 0)) { return 1; }
  return 15;
}
/* Tries to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if LE_SMALL(prereadPos, READBUF_SIZE).
 *
 * Maximum allowed prereadSize is READBUF_SIZE (< 66000).
 */
static final int Preread(int LOCAL_prSize) {
  try {
  int LOCAL_prPos = GLOBAL_readEnd - GLOBAL_readCur;
  int LOCAL_prGot;
  if (((LOCAL_prPos) < (LOCAL_prSize))) { /* Not enough pending available. */
    if ((((6 + 65536 + 6) - GLOBAL_readCur) < (LOCAL_prSize))) {
      /* If no room for LOCAL_VAR(prSize) bytes to the end, discard bytes from the beginning. */
      for (GLOBAL_readEnd = 0; ((GLOBAL_readEnd) < (LOCAL_prPos)); ++GLOBAL_readEnd) {
        readBuf8[GLOBAL_readEnd] = (byte)((readBuf8[GLOBAL_readCur + GLOBAL_readEnd] & 0xff));
      }
      GLOBAL_readCur = 0;
    }
    while (((LOCAL_prPos) < (LOCAL_prSize))) {
      /* Instead of (LOCAL_VAR(prSize) - LOCAL_VAR(prPos)) we could use (GLOBAL_VAR(readBuf) + READBUF_SIZE -
       * GLOBAL_VAR(readEnd)) to read as much as the buffer has room for.
       */
      LOCAL_prGot = System.in.read(readBuf8, GLOBAL_readEnd, LOCAL_prSize - LOCAL_prPos);
      if (((LOCAL_prGot + 1) <= (1))) { break; } /* EOF or error on input. */
      GLOBAL_readEnd += LOCAL_prGot;
      LOCAL_prPos += LOCAL_prGot ;
    }
  }
  return LOCAL_prPos;
  } catch (java.io.IOException e) {
    return 8;
  }
}
static final void IgnoreVarint() {
  while ((((readBuf8[GLOBAL_readCur++] & 0xff)) >= (0x80))) {}
}
static final int IgnoreZeroBytes(int LOCAL_izCount) {
  for (; ((LOCAL_izCount) != (0)); --LOCAL_izCount) {
    if ((((readBuf8[GLOBAL_readCur++] & 0xff)) != (0))) {
      return 57;
    }
  }
  return 0;
}
static final int GetLE4(int LOCAL_glPos) {
  return (readBuf8[LOCAL_glPos] & 0xff) | (readBuf8[LOCAL_glPos + 1] & 0xff) << 8 | (readBuf8[LOCAL_glPos + 2] & 0xff) << 16 | (readBuf8[LOCAL_glPos + 3] & 0xff) << 24;
}
/* Expects GLOBAL_VAR(dicSize) be set already. Can be called before or after InitProp. */
static final void InitDecode() {
  /* needInitProp will initialize it */
  /* SET_GLOBAL(lc, 106, =) SET_GLOBAL(pb, 108, =) SET_GLOBAL(lp, 110, =) 0; */
  GLOBAL_dicBufSize = 0; /* We'll increment it later. */
  GLOBAL_needInitDic = 1;
  GLOBAL_needInitState = 1;
  GLOBAL_needInitProp = 1;
  GLOBAL_dicPos = 0;
  LzmaDec_InitDicAndState(1, 1);
}
static final int InitProp(int LOCAL_ipByte) {
  if (((LOCAL_ipByte) >= (9 * 5 * 5))) { return 68; }
  GLOBAL_lc = LOCAL_ipByte % 9;
  GLOBAL_lcm8 = 8 - GLOBAL_lc;
  LOCAL_ipByte /= 9 ;
  GLOBAL_pb = LOCAL_ipByte / 5;
  GLOBAL_lp = LOCAL_ipByte % 5;
  if (((GLOBAL_lc + GLOBAL_lp) > (4))) { return 68; }
  GLOBAL_needInitProp = 0;
  return 0;
}
/* Writes uncompressed data dic[LOCAL_VAR(fromDicPos) : GLOBAL_VAR(dicPos)] to stdout. */
static final int WriteFrom(int LOCAL_wfDicPos) {
  System.out.write(dic8, LOCAL_wfDicPos, GLOBAL_dicPos - LOCAL_wfDicPos);
  return 0;
}
/* Reads .xz or .lzma data from stdin, writes uncompressed bytes to stdout,
 * uses GLOBAL_VAR(dic). It verifies some aspects of the file format (so it
 * can't be tricked to an infinite loop etc.), itdoesn't verify checksums
 * (e.g. CRC32).
 */
static final int DecompressXzOrLzma() {
  int LOCAL_checksumSize;
  int LOCAL_bhf; /* Block header flags */
  int LOCAL_dxRes;
  /* 12 for the stream header + 12 for the first block header + 6 for the
   * first chunk header. empty.xz is 32 bytes.
   */
  if (((Preread(12 + 12 + 6)) < (12 + 12 + 6))) { return 6; }
  /* readBuf[6] is actually stream flags, should also be 0. */
  if ((((readBuf8[0] & 0xff)) == (0xfd)) && (((readBuf8[1] & 0xff)) == (0x37)) &&
      (((readBuf8[2] & 0xff)) == (0x7a)) && (((readBuf8[3] & 0xff)) == (0x58)) &&
      (((readBuf8[4] & 0xff)) == (0x5a)) && (((readBuf8[5] & 0xff)) == (0)) &&
      (((readBuf8[6] & 0xff)) == (0))) { /* .xz: "\xFD""7zXZ\0" */
  } else if ((((readBuf8[GLOBAL_readCur] & 0xff)) <= (225)) && (((readBuf8[GLOBAL_readCur + 13] & 0xff)) == (0)) && /* .lzma */
        /* High 4 bytes of uncompressed size. */
        ((((LOCAL_bhf = GetLE4(GLOBAL_readCur + 9))) == 0) || ((~LOCAL_bhf) == 0)) &&
        (((GLOBAL_dicSize = GetLE4(GLOBAL_readCur + 1))) >= ((1 << 12))) &&
        ((GLOBAL_dicSize) - 0x80000000 < (1610612736 + 1) - 0x80000000)) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    int LOCAL_readBufUS;
    int LOCAL_srcLen;
    int LOCAL_fromDicPos;
    InitDecode();
    /* LZMA restricts LE_SMALL(lc + lp, 4). LZMA requires LE_SMALL(lc + lp,
     * 12). We apply the LZMA2 restriction here (to save memory in
     * GLOBAL_VAR(probs)), thus we are not able to extract some legitimate
     * .lzma files.
     */
    if ((((LOCAL_dxRes = InitProp((readBuf8[GLOBAL_readCur] & 0xff)))) != (0))) {
      return LOCAL_dxRes;
    }
    if (((LOCAL_bhf) == 0)) {
      GLOBAL_dicBufSize = LOCAL_readBufUS = GetLE4(GLOBAL_readCur + 5);
      if (!((LOCAL_readBufUS) - 0x80000000 < (1610612736 + 1) - 0x80000000)) { return 2; }
    } else {
      LOCAL_readBufUS = LOCAL_bhf ; /* max UInt32. */
      /* !! Don't preallocate DIC_BUF_SIZE in Java in ENSURE_DIC_SIZE below. */
      GLOBAL_dicBufSize = 1610612736;
    }
    if (((dic8.length) < (GLOBAL_dicBufSize))) EnsureDicSize();
    GLOBAL_readCur += 13; /* Start decompressing the 0 byte. */
    /* TODO(pts): Limit on uncompressed size unless 8 bytes of -1 is
     * specified.
     */
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while ((((LOCAL_srcLen = Preread((6 + 65536 + 6)))) != (0))) {
      LOCAL_fromDicPos = GLOBAL_dicPos ;
      LOCAL_dxRes = LzmaDec_DecodeToDic(LOCAL_srcLen) ;
      if (((LOCAL_readBufUS) - 0x80000000 < (GLOBAL_dicPos) - 0x80000000)) { GLOBAL_dicPos = LOCAL_readBufUS; }
      if ((((LOCAL_dxRes = WriteFrom(LOCAL_fromDicPos))) != (0))) { return LOCAL_dxRes; }
      if (((LOCAL_dxRes) == (15))) { break; }
      if (((LOCAL_dxRes) != (17)) && ((LOCAL_dxRes) != (0))) { return LOCAL_dxRes; }
      if (((GLOBAL_dicPos - LOCAL_readBufUS) == 0)) { break; }
    }
    return 0;
  } else {
    return 51;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  LOCAL_checksumSize = (readBuf8[GLOBAL_readCur + 7] & 0xff);
  if (((LOCAL_checksumSize) == (0))) { /* None */ LOCAL_checksumSize = 1; }
  else if (((LOCAL_checksumSize) == (1))) { /* CRC32 */ LOCAL_checksumSize = 4; }
  else if (((LOCAL_checksumSize) == (4))) { /* CRC64, typical xz output. */ LOCAL_checksumSize = 8; }
  else { return 60; }
  /* Also ignore the CRC32 after LOCAL_VAR(checksumSize). */
  GLOBAL_readCur += 12;
  for (;;) { /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    int LOCAL_blockSizePad = 3;
    int LOCAL_bhs;
    int LOCAL_bhs2; /* Block header size */
    int LOCAL_dicSizeProp;
    int LOCAL_readAtBlock;
    ; /* At least 12 bytes preread. */
    LOCAL_readAtBlock = GLOBAL_readCur ;
    /* Last block, index follows. */
    if ((((LOCAL_bhs = (readBuf8[GLOBAL_readCur++] & 0xff))) == (0))) { break; }
    /* Block header size includes the LOCAL_VAR(bhs) field above and the CRC32 below. */
    LOCAL_bhs = (LOCAL_bhs + 1) << 2;
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (((Preread(LOCAL_bhs)) < (LOCAL_bhs))) { return 6; }
    LOCAL_readAtBlock = GLOBAL_readCur ;
    LOCAL_bhf = (readBuf8[GLOBAL_readCur++] & 0xff) ;
    if (((LOCAL_bhf & 2) != (0))) { return 53; }
    if (((LOCAL_bhf & 20) != (0))) { return 54; }
    if (((LOCAL_bhf & 64) != (0))) { /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (((LOCAL_bhf & 128) != (0))) { /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if ((((readBuf8[GLOBAL_readCur++] & 0xff)) != (0x21))) { return 55; }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if ((((readBuf8[GLOBAL_readCur++] & 0xff)) != (1))) { return 56; }
    LOCAL_dicSizeProp = (readBuf8[GLOBAL_readCur++] & 0xff) ;
    /* Typical large dictionary sizes:
     *
     *  * 35: 805306368 bytes == 768 MiB
     *  * 36: 1073741824 bytes == 1 GiB
     *  * 37: 1610612736 bytes, largest supported by .xz
     *  * 38: 2147483648 bytes == 2 GiB
     *  * 39: 3221225472 bytes == 3 GiB
     *  * 40: 4294967295 bytes, largest supported by .xz
     */
    if (((LOCAL_dicSizeProp) > (40))) { return 61; }
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (((LOCAL_dicSizeProp) > (37))) { return 62; }
    GLOBAL_dicSize = (((2) | ((LOCAL_dicSizeProp) & 1)) << ((LOCAL_dicSizeProp) / 2 + 11));
    LOCAL_bhs2 = GLOBAL_readCur - LOCAL_readAtBlock + 5 ; /* Won't overflow. */
    if (((LOCAL_bhs2) > (LOCAL_bhs))) { return 58; }
    if ((((LOCAL_dxRes = IgnoreZeroBytes(LOCAL_bhs - LOCAL_bhs2))) != (0))) { return LOCAL_dxRes; }
    GLOBAL_readCur += 4; /* Ignore CRC32. */
    /* Typically it's LOCAL_VAR(offset) 24, xz creates it by default, minimal. */
    { /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      int LOCAL_chunkUS; /* Uncompressed chunk sizes. */
      int LOCAL_chunkCS; /* Compressed chunk size. */
      int LOCAL_initDic;
      InitDecode();
      for (;;) {
        int LOCAL_control;
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (((Preread(6)) < (6))) { return 6; }
        LOCAL_control = (readBuf8[GLOBAL_readCur] & 0xff) ;
        if (((LOCAL_control) == (0))) {
          ++GLOBAL_readCur;
          break;
        } else if (((((LOCAL_control - 3) & 0xff)) < (0x80 - 3))) {
          return 59;
        }
        LOCAL_chunkUS = ((readBuf8[GLOBAL_readCur + 1] & 0xff) << 8) + (readBuf8[GLOBAL_readCur + 2] & 0xff) + 1 ;
        if (((LOCAL_control) < (3))) { /* Uncompressed chunk. */
          LOCAL_initDic = ((((LOCAL_control) == (1))) ? 1 : 0);
          LOCAL_chunkCS = LOCAL_chunkUS ;
          GLOBAL_readCur += 3;
          /* TODO(pts): Porting: TRUNCATE_TO_8BIT(LOCAL_VAR(blockSizePad)) for Python and other unlimited-integer-range languages. */
          LOCAL_blockSizePad -= 3;
          if (((LOCAL_initDic) != 0)) {
            GLOBAL_needInitProp = GLOBAL_needInitState = 1;
            GLOBAL_needInitDic = 0;
          } else if (((GLOBAL_needInitDic) != 0)) {
            return 1;
          }
          LzmaDec_InitDicAndState(LOCAL_initDic, 0);
        } else { /* LZMA chunk. */
          int LOCAL_mode = ((((LOCAL_control)) >> (5)) & 3);
          int LOCAL_initState = ((((LOCAL_mode) != (0))) ? 1 : 0);
          int LOCAL_isProp = (((((LOCAL_control & 64)) != (0))) ? 1 : 0);
          LOCAL_initDic = ((((LOCAL_mode) == (3))) ? 1 : 0);
          LOCAL_chunkUS += (LOCAL_control & 31) << 16 ;
          LOCAL_chunkCS = ((readBuf8[GLOBAL_readCur + 3] & 0xff) << 8) + (readBuf8[GLOBAL_readCur + 4] & 0xff) + 1 ;
          if (((LOCAL_isProp) != 0)) {
            if ((((LOCAL_dxRes = InitProp((readBuf8[GLOBAL_readCur + 5] & 0xff)))) != (0))) {
              return LOCAL_dxRes;
            }
            ++GLOBAL_readCur;
            --LOCAL_blockSizePad;
          } else {
            if (((GLOBAL_needInitProp) != 0)) { return 67; }
          }
          GLOBAL_readCur += 5;
          LOCAL_blockSizePad -= 5 ;
          if ((((LOCAL_initDic) == 0) && ((GLOBAL_needInitDic) != 0)) || (((LOCAL_initState) == 0) && ((GLOBAL_needInitState) != 0))) {
            return 1;
          }
          LzmaDec_InitDicAndState(LOCAL_initDic, LOCAL_initState);
          GLOBAL_needInitDic = 0;
          GLOBAL_needInitState = 0;
        }
        GLOBAL_dicBufSize += LOCAL_chunkUS;
        /* Decompressed data too long, won't fit to GLOBAL_VAR(dic). */
        if (((GLOBAL_dicBufSize) > (1610612736))) { return 2; }
        if (((dic8.length) < (GLOBAL_dicBufSize))) EnsureDicSize();
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (((Preread(LOCAL_chunkCS + 6)) < (LOCAL_chunkCS))) { return 6; }
        if (((LOCAL_control) < (0x80))) { /* Uncompressed chunk. */
          while (((GLOBAL_dicPos) != (GLOBAL_dicBufSize))) {
            dic8[GLOBAL_dicPos++] = (byte)((readBuf8[GLOBAL_readCur++] & 0xff));
          }
          if (((GLOBAL_checkDicSize) == (0)) && ((GLOBAL_dicSize - GLOBAL_processedPos) <= (LOCAL_chunkUS))) {
            GLOBAL_checkDicSize = GLOBAL_dicSize;
          }
          GLOBAL_processedPos += LOCAL_chunkUS;
        } else { /* Compressed chunk. */
          /* This call doesn't change GLOBAL_VAR(dicBufSize). */
          if ((((LOCAL_dxRes = LzmaDec_DecodeToDic(LOCAL_chunkCS))) != (0))) { return LOCAL_dxRes; }
        }
        if (((GLOBAL_dicPos) != (GLOBAL_dicBufSize))) { return 65; }
        if ((((LOCAL_dxRes = WriteFrom(GLOBAL_dicPos - LOCAL_chunkUS))) != (0))) { return LOCAL_dxRes; }
        LOCAL_blockSizePad -= LOCAL_chunkCS ;
        /* We can't discard decompressbuf[:GLOBAL_VAR(dicBufSize)] now,
         * because we need it a dictionary in which subsequent calls to
         * Lzma2Dec_DecodeToDic will look up backreferences.
         */
      }
    } /* End of LZMA2 stream. */
    /* End of block. */
    /* 7 for padding4 and CRC32 + 12 for the next block header + 6 for the next
     * chunk header.
     */
    if (((Preread(7 + 12 + 6)) < (7 + 12 + 6))) { return 6; }
    /* Ignore block padding. */
    if ((((LOCAL_dxRes = IgnoreZeroBytes(LOCAL_blockSizePad & 3))) != (0))) { return LOCAL_dxRes; }
    GLOBAL_readCur += LOCAL_checksumSize; /* Ignore CRC32, CRC64 etc. */
  }
  /* The .xz input file continues with the index, which we ignore from here. */
  return 0;
}
public static void main(String args[]) {
  probs16 = new short[14134]; /* !! TODO(pts): Automatic size. */
  readBuf8 = new byte[(6 + 65536 + 6) + 20]; /* !! TODO(pts): Automatic size. */
  dic8 = new byte[65536];
  System.exit(DecompressXzOrLzma());
}
}
