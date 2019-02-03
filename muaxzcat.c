/*
 * muaxzcat.c: tiny .xz and .lzma decompression filter without pointers
 * by pts@fazekas.hu at Sat Feb  2 13:28:42 CET 2019
 *
 * Compile with any of:
 *
 *   $ gcc -ansi -O2 -W -Wall -Wextra -o muaxzcat muaxzcat.c
 *   $ g++ -ansi -O2 -W -Wall -Wextra -o muaxzcat muaxzcat.c
 *
 * This is a variant of muxzcat.c for easier porting to other programming
 * languages:
 *
 * * It doesn't use sizeof.
 * * It doesn't use pointers. (It uses arrays and array indexes.)
 * * It doesn't use macros for code (except for integer constants).
 * * It has more state in global variables (rather than function
 *   arguments).
 *
 * Most users should use muxzcat.c instead, because that one runs faster.
 */

#undef CONFIG_DEBUG
#ifdef __TINYC__  /* tcc https://bellard.org/tcc/ , pts-tcc https://github.com/pts/pts-tcc */

#undef  CONFIG_LANG_C
#define CONFIG_LANG_C 1

typedef unsigned size_t;  /* TODO(pts): Support 64-bit tcc */
typedef int ssize_t;  /* TODO(pts): Support 64-bit tcc */
typedef int int32_t;
typedef unsigned uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#else
#ifdef __XTINY__  /* xtiny https://github.com/pts/pts-xtiny */

#undef  CONFIG_LANG_C
#define CONFIG_LANG_C 1
#include <xtiny.h>

#else  /* Not __XTINY__. */

#undef  CONFIG_LANG_C
#define CONFIG_LANG_C 1
#define CONFIG_DEBUG 1
#include <unistd.h>  /* read(), write() */
#ifdef _WIN32
#  include <windows.h>
#endif
#if defined(MSDOS) || defined(_WIN32)
#  include <fcntl.h>  /* setmode() */
#endif
#include <stdint.h>
#endif  /* __XTINY__ */
#endif  /* __TINYC__ */

#ifdef CONFIG_LANG_C
/* This fails to compile if any condition after the : is false. */
struct IntegerTypeAsserts {
  int UInt8TIsInteger : (uint8_t)1 / 2 == 0;
  int UInt8TIs8Bits : sizeof(uint8_t) == 1;
  int UInt8TIsUnsigned : (uint8_t)-1 > 0;
  int UInt16TIsInteger : (uint16_t)1 / 2 == 0;
  int UInt16TIs16Bits : sizeof(uint16_t) == 2;
  int UInt16TIsUnsigned : (uint16_t)-1 > 0;
  int UInt32TIsInteger : (uint32_t)1 / 2 == 0;
  int UInt32TIs32Bits : sizeof(uint32_t) == 4;
  int UInt32TIsUnsigned : (uint32_t)-1 > 0;
};
#endif  /* CONFIG_LANG_C */

#ifdef CONFIG_LANG_C
#define TRUNCATE_TO_32BIT(x) ((uint32_t)(x))
#define TRUNCATE_TO_16BIT(x) ((uint16_t)(x))
#define TRUNCATE_TO_8BIT(x) ((uint8_t)(x))
#define GLOBAL(type, name) type name
#define GLOBAL_ARY16(a, size) uint16_t a##16[size]
#define GLOBAL_ARY8(a, size) uint8_t a##8[size]
#define L(name) name  /* Local variable or argument of a function. */
#define LOCAL(type, name) type name
#define LOCAL_INIT(type, name, value) type name = (value)
#define FUNC_ARG0(return_type, name) return_type name(void) {
#define FUNC_ARG1(return_type, name, arg1_type, arg1) return_type name(arg1_type arg1) {
#define FUNC_ARG2(return_type, name, arg1_type, arg1, arg2_type, arg2) return_type name(arg1_type arg1, arg2_type arg2) {
#define ENDFUNC }
#endif  /* CONFIG_LANG_C */

/* --- */

#ifdef CONFIG_DEBUG
/* This is guaranteed to work with Linux and gcc only. For example, %lld in
 * printf doesn't work with MinGW.
 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#define DEBUGF(...) fprintf(stderr, "DEBUG: " __VA_ARGS__)
#define ASSERT(condition) assert(condition)
#else
#define DEBUGF(...)
/* Just check that it compiles. */
#define ASSERT(condition) do {} while (0 && (condition))
#endif

#define GET_ARY16(a, idx) (+global.a##16[idx])
/* TRUNCATE_TO_16BIT is must be called on value manually if needed. */
#define SET_ARY16(a, idx, value) (global.a##16[idx] = value)
#define GET_ARY8(a, idx) (+global.a##8[idx])
/* TRUNCATE_TO_8BIT must be called on value manually if needed. */
#define SET_ARY8(a, idx, value) (global.a##8[idx] = value)
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) (read(0, &global.a##8[fromIdx], (size)))
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) (write(1, &global.a##8[fromIdx], (size)))

/* --- */

#define SZ_OK 0
#define SZ_ERROR_DATA 1
#define SZ_ERROR_MEM 2
#define SZ_ERROR_CRC 3
#define SZ_ERROR_UNSUPPORTED 4
#define SZ_ERROR_PARAM 5
#define SZ_ERROR_INPUT_EOF 6
#define SZ_ERROR_OUTPUT_EOF 7
#define SZ_ERROR_READ 8
#define SZ_ERROR_WRITE 9
#define SZ_ERROR_FINISHED_WITH_MARK 15            /* LzmaDec_DecodeToDic stream was finished with end mark. */
#define SZ_ERROR_NOT_FINISHED 16                  /* LzmaDec_DecodeToDic stream was not finished */
#define SZ_ERROR_NEEDS_MORE_INPUT 17              /* LzmaDec_DecodeToDic, you must provide more input bytes */
/*#define SZ_MAYBE_FINISHED_WITHOUT_MARK SZ_OK*/  /* LzmaDec_DecodeToDic, there is probability that stream was finished without end mark */
#define SZ_ERROR_CHUNK_NOT_CONSUMED 18
#define SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL 17      /* LzmaDec_DecodeToDic, more input needed, but existing input was partially processed */
#define SZ_ERROR_BAD_MAGIC 51
#define SZ_ERROR_BAD_STREAM_FLAGS 52  /* SZ_ERROR_BAD_MAGIC is reported instead. */
#define SZ_ERROR_UNSUPPORTED_FILTER_COUNT 53
#define SZ_ERROR_BAD_BLOCK_FLAGS 54
#define SZ_ERROR_UNSUPPORTED_FILTER_ID 55
#define SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE 56
#define SZ_ERROR_BAD_PADDING 57
#define SZ_ERROR_BLOCK_HEADER_TOO_LONG 58
#define SZ_ERROR_BAD_CHUNK_CONTROL_BYTE 59
#define SZ_ERROR_BAD_CHECKSUM_TYPE 60
#define SZ_ERROR_BAD_DICTIONARY_SIZE 61
#define SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE 62
#define SZ_ERROR_FEED_CHUNK 63
#define SZ_ERROR_NOT_FINISHED_WITH_MARK 64
#define SZ_ERROR_BAD_DICPOS 65
#define SZ_ERROR_MISSING_INITPROP 67
#define SZ_ERROR_BAD_LCLPPB_PROP 68

#define TRUE 1
#define FALSE 0

#define LZMA_REQUIRED_INPUT_MAX 20

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768
#define LZMA2_LCLP_MAX 4

#define LZMA2_MAX_NUM_PROBS 14134

#define DIC_ARRAY_SIZE 1610612736

#define RC_INIT_SIZE 5

#define kNumMoveBits 5
#define kNumTopBits 24
#define kNumBitModelTotalBits 11
#define kNumPosBitsMax 4
#define kLenNumLowBits 3
#define kLenNumMidBits 3
#define kLenNumHighBits 8
#define kNumStates 12
#define kNumLitStates 7
#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumPosSlotBits 6
#define kNumLenToPosStates 4
#define kNumAlignBits 4
#define kMatchMinLen 2
#define kTopValue 16777216
#define kBitModelTotal 2048
#define kNumPosStatesMax 16
#define kLenNumLowSymbols 8
#define kLenNumMidSymbols 8
#define kLenNumHighSymbols 256
#define LenChoice 0
#define LenChoice2 1
#define LenLow 2
#define LenMid 130
#define LenHigh 258
#define kNumLenProbs 514
#define kNumFullDistances 128
#define kAlignTableSize 16
#define kMatchSpecLenStart 274
#define IsMatch 0
#define IsRep 192
#define IsRepG0 204
#define IsRepG1 216
#define IsRepG2 228
#define IsRep0Long 240
#define PosSlotCode 432
#define SpecPos 688
#define Align 802
#define LenCoder 818
#define RepLenCoder 1332
#define Literal 1846

/* 6 is maximum LZMA chunk header size.
 * 65536 is maximum cs (compressed size) of LZMA2 chunk.
 * 6 is maximum LZMA chunk header size for the next chunk.
 */
#define READBUF_SIZE (6 + 65536 + 6)

#define LZMA_DIC_MIN (1 << 12)

#define DUMMY_ERROR 0  /* unexpected end of input stream */
#define DUMMY_LIT 1
#define DUMMY_MATCH 2
#define DUMMY_REP 3

#define FILTER_ID_LZMA2 0x21

/* --- */

#if 1  /* Make everything 32-bit, for easier porting to other languages. */
typedef uint32_t UInt32;
typedef uint32_t Byte;
typedef uint32_t SRes;
typedef uint32_t Bool;
#else
typedef uint32_t UInt32;
typedef uint8_t Byte;
typedef uint8_t SRes;
typedef uint8_t Bool;
#endif

/* For LZMA streams, lc + lp <= 8 + 4 <= 12.
 * For LZMA2 streams, lc + lp <= 4.
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: sizeof(global.probs[0]) * value == (2 or 4) * value bytes.
 */
/*#define LzmaProps_GetNumProbs(p) TRUNCATE_TO_32BIT(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp))) */

#ifdef CONFIG_LANG_C
/* This fails to compile if any condition after the : is false. */
struct LzmaAsserts {
  int Lzma2MaxNumProbsIsCorrect : LZMA2_MAX_NUM_PROBS == TRUNCATE_TO_32BIT(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << LZMA2_LCLP_MAX));
};
#endif  /* CONFIG_LANG_C */

struct Global {
  GLOBAL(UInt32, bufCur);
  GLOBAL(UInt32, dicSize);  /* Configured in prop byte. */
  GLOBAL(UInt32, range);
  GLOBAL(UInt32, code);
  GLOBAL(UInt32, dicPos);
  GLOBAL(UInt32, dicBufSize);
  GLOBAL(UInt32, processedPos);
  GLOBAL(UInt32, checkDicSize);
  GLOBAL(UInt32, state);
  GLOBAL(UInt32, rep0);
  GLOBAL(UInt32, rep1);
  GLOBAL(UInt32, rep2);
  GLOBAL(UInt32, rep3);
  GLOBAL(UInt32, remainLen);
  GLOBAL(UInt32, tempBufSize);
  GLOBAL(UInt32, readCur);  /* Index within (or at end of) readBuf. */
  GLOBAL(UInt32, readEnd);  /* Index within (or at end of) readBuf. */
  GLOBAL(Bool, needFlush);
  GLOBAL(Bool, needInitLzma);
  GLOBAL(Bool, needInitDic);
  GLOBAL(Bool, needInitState);
  GLOBAL(Bool, needInitProp);
  GLOBAL(Byte, lc);  /* Configured in prop byte. */
  GLOBAL(Byte, lp);  /* Configured in prop byte. */
  GLOBAL(Byte, pb);  /* Configured in prop byte. */
  GLOBAL_ARY16(probs, LZMA2_MAX_NUM_PROBS);  /* Probabilities for bit decoding. */
  /* The first READBUF_SIZE bytes is readBuf, then the LZMA_REQUIRED_INPUT_MAX bytes is tempBuf. */
  GLOBAL_ARY8(readBuf, READBUF_SIZE + LZMA_REQUIRED_INPUT_MAX);
  /* Contains the uncompressed data.
   *
   * Array size is about 1.61 GB.
   * We rely on virtual memory so that if we don't use the end of array for
   * small files, then the operating system won't take the entire array away
   * from other processes.
   */
  GLOBAL_ARY8(dic, DIC_ARRAY_SIZE);
} global;

/* --- */

#ifdef CONFIG_LANG_C
/* This fails to compile if any condition after the : is false. */
struct ProbsAsserts {
  int LiteralCode : Literal == LZMA_BASE_SIZE;
};
#endif  /* CONFIG_LANG_C */

FUNC_ARG1(void, LzmaDec_WriteRem, UInt32, dicLimit)
  if (global.remainLen != 0 && global.remainLen < kMatchSpecLenStart) {
    LOCAL_INIT(UInt32, len, global.remainLen);
    if (dicLimit - global.dicPos < len) {
      len = dicLimit - global.dicPos;
    }
    if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= len) {
      global.checkDicSize = global.dicSize;
    }
    global.processedPos += len;
    global.remainLen -= len;
    while (len != 0) {
      len--;
      SET_ARY8(dic, global.dicPos, GET_ARY8(dic, (global.dicPos - global.rep0) + ((global.dicPos < global.rep0) ? global.dicBufSize : 0)));
      global.dicPos++;
    }
  }
ENDFUNC

/* Modifies global.bufCur etc. */
FUNC_ARG2(SRes, LzmaDec_DecodeReal2, const UInt32, dicLimit, const UInt32, bufLimit)
  LOCAL_INIT(const UInt32, pbMask, ((UInt32)1 << (global.pb)) - 1);
  LOCAL_INIT(const UInt32, lpMask, ((UInt32)1 << (global.lp)) - 1);
  do {
    LOCAL_INIT(const UInt32, dicLimit2, global.checkDicSize == 0 && global.dicSize - global.processedPos < dicLimit - global.dicPos ? global.dicPos + (global.dicSize - global.processedPos) : dicLimit);
    LOCAL_INIT(UInt32, len, 0);
    LOCAL_INIT(UInt32, rangeLocal, global.range);
    LOCAL_INIT(UInt32, codeLocal, global.code);
    do {
      LOCAL(UInt32, probIdx);
      LOCAL(UInt32, bound);
      LOCAL(UInt32, ttt);
      LOCAL_INIT(UInt32, posState, global.processedPos & pbMask);

      probIdx = IsMatch + (global.state << kNumPosBitsMax) + posState;
      ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
      if (codeLocal < bound) {
        LOCAL(UInt32, symbol);
        rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
        probIdx = Literal;
        if (global.checkDicSize != 0 || global.processedPos != 0) {
          probIdx += (LZMA_LIT_SIZE * (((global.processedPos & lpMask) << global.lc) + (GET_ARY8(dic, (global.dicPos == 0 ? global.dicBufSize : global.dicPos) - 1) >> (8 - global.lc))));
        }
        if (global.state < kNumLitStates) {
          global.state -= (global.state < 4) ? global.state : 3;
          symbol = 1;
          do { ttt = GET_ARY16(probs, probIdx + symbol); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + symbol, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; symbol = (symbol + symbol); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + symbol, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; symbol = (symbol + symbol) + 1; ;; } } while (symbol < 0x100);
        } else {
          LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, (global.dicPos - global.rep0) + ((global.dicPos < global.rep0) ? global.dicBufSize : 0)));
          LOCAL_INIT(UInt32, offs, 0x100);
          global.state -= (global.state < 10) ? 3 : 6;
          symbol = 1;
          do {
            LOCAL(UInt32, bit);
            LOCAL(UInt32, probLitIdx);
            matchByte <<= 1;
            bit = (matchByte & offs);
            probLitIdx = probIdx + offs + bit + symbol;
            ttt = GET_ARY16(probs, probLitIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probLitIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; symbol = (symbol + symbol); offs &= ~bit; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probLitIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; symbol = (symbol + symbol) + 1; offs &= bit; }
          } while (symbol < 0x100);
        }
        SET_ARY8(dic, global.dicPos++, TRUNCATE_TO_8BIT(symbol));
        global.processedPos++;
        continue;
      } else {
        rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
        probIdx = IsRep + global.state;
        ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
        if (codeLocal < bound) {
          rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
          global.state += kNumStates;
          probIdx = LenCoder;
        } else {
          rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
          if (global.checkDicSize == 0 && global.processedPos == 0) {
            return SZ_ERROR_DATA;
          }
          probIdx = IsRepG0 + global.state;
          ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
          if (codeLocal < bound) {
            rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
            probIdx = IsRep0Long + (global.state << kNumPosBitsMax) + posState;
            ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
            if (codeLocal < bound) {
              rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
              SET_ARY8(dic, global.dicPos, GET_ARY8(dic, (global.dicPos - global.rep0) + ((global.dicPos < global.rep0) ? global.dicBufSize : 0)));
              global.dicPos++;
              global.processedPos++;
              global.state = global.state < kNumLitStates ? 9 : 11;
              continue;
            }
            rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
          } else {
            LOCAL(UInt32, distance);
            rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
            probIdx = IsRepG1 + global.state;
            ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
            if (codeLocal < bound) {
              rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
              distance = global.rep1;
            } else {
              rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
              probIdx = IsRepG2 + global.state;
              ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
              if (codeLocal < bound) {
                rangeLocal = bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
                distance = global.rep2;
              } else {
                rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
                distance = global.rep3;
                global.rep3 = global.rep2;
              }
              global.rep2 = global.rep1;
            }
            global.rep1 = global.rep0;
            global.rep0 = distance;
          }
          global.state = global.state < kNumLitStates ? 8 : 11;
          probIdx = RepLenCoder;
        }
        {
          LOCAL(UInt32, limitSub);
          LOCAL(UInt32, offset);
          LOCAL_INIT(UInt32, probLenIdx, probIdx + LenChoice);
          ttt = GET_ARY16(probs, probLenIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
          if (codeLocal < bound) {
            rangeLocal = bound; SET_ARY16(probs, probLenIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
            probLenIdx = probIdx + LenLow + (posState << kLenNumLowBits);
            offset = 0;
            limitSub = (1 << kLenNumLowBits);
          } else {
            rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probLenIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
            probLenIdx = probIdx + LenChoice2;
            ttt = GET_ARY16(probs, probLenIdx); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
            if (codeLocal < bound) {
              rangeLocal = bound; SET_ARY16(probs, probLenIdx, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));;
              probLenIdx = probIdx + LenMid + (posState << kLenNumMidBits);
              offset = kLenNumLowSymbols;
              limitSub = (1 << kLenNumMidBits);
            } else {
              rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probLenIdx, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));;
              probLenIdx = probIdx + LenHigh;
              offset = kLenNumLowSymbols + kLenNumMidSymbols;
              limitSub = (1 << kLenNumHighBits);
            }
          }
          { len = 1; do { { ttt = GET_ARY16(probs, (probLenIdx + len)); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, (probLenIdx + len), TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; len = (len + len); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, (probLenIdx + len), TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; len = (len + len) + 1; ;; }; }; } while (len < limitSub); len -= limitSub; };
          len += offset;
        }

        if (global.state >= kNumStates) {
          LOCAL(UInt32, distance);
          probIdx = PosSlotCode + ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
          { distance = 1; do { { ttt = GET_ARY16(probs, (probIdx + distance)); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, (probIdx + distance), TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; distance = (distance + distance); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, (probIdx + distance), TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; distance = (distance + distance) + 1; ;; }; }; } while (distance < (1 << 6)); distance -= (1 << 6); };
          if (distance >= kStartPosModelIndex) {
            LOCAL_INIT(const UInt32, posSlot, distance);
            LOCAL_INIT(UInt32, numDirectBits, (distance >> 1) - 1);
            distance = (2 | (distance & 1));
            if (posSlot < kEndPosModelIndex) {
              distance <<= numDirectBits;
              probIdx = SpecPos + distance - posSlot - 1;
              {
                LOCAL_INIT(UInt32, mask, 1);
                LOCAL_INIT(UInt32, i, 1);
                do {
                  ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; i = (i + i) + 1; distance |= mask; };
                  mask <<= 1;
                } while (--numDirectBits != 0);
              }
            } else {
              numDirectBits -= kNumAlignBits;
              do {
                LOCAL(UInt32, t);
                if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }
                rangeLocal >>= 1;
                codeLocal -= rangeLocal;
                t = (0 - ((UInt32)codeLocal >> 31));
                distance = (distance << 1) + (t + 1);
                codeLocal += rangeLocal & t;
              } while (--numDirectBits != 0);
              probIdx = Align;
              distance <<= kNumAlignBits;
              {
                LOCAL_INIT(UInt32, i, 1);
                ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; i = (i + i) + 1; distance |= 1; };
                ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; i = (i + i) + 1; distance |= 2; };
                ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; i = (i + i) + 1; distance |= 4; };
                ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)));; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; SET_ARY16(probs, probIdx + i, TRUNCATE_TO_16BIT(ttt - (ttt >> kNumMoveBits)));; i = (i + i) + 1; distance |= 8; };
              }
              if (distance == (UInt32)0xFFFFFFFF) {
                len += kMatchSpecLenStart;
                global.state -= kNumStates;
                break;
              }
            }
          }
          global.rep3 = global.rep2;
          global.rep2 = global.rep1;
          global.rep1 = global.rep0;
          global.rep0 = distance + 1;
          if (global.checkDicSize == 0) {
            if (distance >= global.processedPos) {
              return SZ_ERROR_DATA;
            }
          }
          else if (distance >= global.checkDicSize) {
            return SZ_ERROR_DATA;
          }
          global.state = (global.state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        }

        len += kMatchMinLen;

        if (dicLimit2 == global.dicPos) {
          return SZ_ERROR_DATA;
        }
        {
          LOCAL_INIT(UInt32, rem, dicLimit2 - global.dicPos);
          LOCAL_INIT(UInt32, curLen, ((rem < len) ? (UInt32)rem : len));
          LOCAL_INIT(UInt32, pos, (global.dicPos - global.rep0) + ((global.dicPos < global.rep0) ? global.dicBufSize : 0));

          global.processedPos += curLen;

          len -= curLen;
          if (pos + curLen <= global.dicBufSize) {
            ASSERT(global.dicPos > pos);
            ASSERT(curLen > 0);
            do {
              SET_ARY8(dic, global.dicPos++, GET_ARY8(dic, pos++));
            } while (--curLen != 0);
          } else {
            do {
              SET_ARY8(dic, global.dicPos++, GET_ARY8(dic, pos++));
              if (pos == global.dicBufSize) pos = 0;
            } while (--curLen != 0);
          }
        }
      }
    } while (global.dicPos < dicLimit2 && global.bufCur < bufLimit);
    if (rangeLocal < kTopValue) { rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, global.bufCur++)); };
    global.range = rangeLocal;
    global.code = codeLocal;
    global.remainLen = len;
    if (global.processedPos >= global.dicSize) {
      global.checkDicSize = global.dicSize;
    }
    LzmaDec_WriteRem(dicLimit);
  } while (global.dicPos < dicLimit && global.bufCur < bufLimit && global.remainLen < kMatchSpecLenStart);

  if (global.remainLen > kMatchSpecLenStart) {
    global.remainLen = kMatchSpecLenStart;
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG2(Byte, LzmaDec_TryDummy, UInt32, bufDummyCur, const UInt32, bufLimit)
  LOCAL_INIT(UInt32, rangeLocal, global.range);
  LOCAL_INIT(UInt32, codeLocal, global.code);
  LOCAL_INIT(UInt32, stateLocal, global.state);
  LOCAL(Byte, res);
  {
    LOCAL(UInt32, probIdx);
    LOCAL(UInt32, bound);
    LOCAL(UInt32, ttt);
    LOCAL_INIT(UInt32, posState, (global.processedPos) & ((1 << global.pb) - 1));

    probIdx = IsMatch + (stateLocal << kNumPosBitsMax) + posState;
    ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
    if (codeLocal < bound) {
      rangeLocal = bound;
      probIdx = Literal;
      if (global.checkDicSize != 0 || global.processedPos != 0) {
        probIdx += (LZMA_LIT_SIZE * ((((global.processedPos) & ((1 << (global.lp)) - 1)) << global.lc) + (GET_ARY8(dic, (global.dicPos == 0 ? global.dicBufSize : global.dicPos) - 1) >> (8 - global.lc))));
      }

      if (stateLocal < kNumLitStates) {
        LOCAL_INIT(UInt32, symbol, 1);
        do { ttt = GET_ARY16(probs, probIdx + symbol); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; symbol = (symbol + symbol); ;; } else { rangeLocal -= bound; codeLocal -= bound; symbol = (symbol + symbol) + 1; ;; } } while (symbol < 0x100);
      } else {
        LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, global.dicPos - global.rep0 + ((global.dicPos < global.rep0) ? global.dicBufSize : 0)));
        LOCAL_INIT(UInt32, offs, 0x100);
        LOCAL_INIT(UInt32, symbol, 1);
        do {
          LOCAL(UInt32, bit);
          LOCAL(UInt32, probLitIdx);
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLitIdx = probIdx + offs + bit + symbol;
          ttt = GET_ARY16(probs, probLitIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; symbol = (symbol + symbol); offs &= ~bit; } else { rangeLocal -= bound; codeLocal -= bound; symbol = (symbol + symbol) + 1; offs &= bit; }
        } while (symbol < 0x100);
      }
      res = DUMMY_LIT;
    } else {
      LOCAL(UInt32, len);
      rangeLocal -= bound; codeLocal -= bound;
      probIdx = IsRep + stateLocal;
      ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
       if (codeLocal < bound) {
        rangeLocal = bound;
        stateLocal = 0;
        probIdx = LenCoder;
        res = DUMMY_MATCH;
      } else {
        rangeLocal -= bound; codeLocal -= bound;
        res = DUMMY_REP;
        probIdx = IsRepG0 + stateLocal;
        ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
        if (codeLocal < bound) {
          rangeLocal = bound;
          probIdx = IsRep0Long + (stateLocal << kNumPosBitsMax) + posState;
          ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
          if (codeLocal < bound) {
            rangeLocal = bound;
            if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); };
            return DUMMY_REP;
          } else {
            rangeLocal -= bound; codeLocal -= bound;
          }
        } else {
          rangeLocal -= bound; codeLocal -= bound;
          probIdx = IsRepG1 + stateLocal;
          ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
          if (codeLocal < bound) {
            rangeLocal = bound;
          } else {
            rangeLocal -= bound; codeLocal -= bound;
            probIdx = IsRepG2 + stateLocal;
            ttt = GET_ARY16(probs, probIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
            if (codeLocal < bound) {
              rangeLocal = bound;
            } else {
              rangeLocal -= bound; codeLocal -= bound;
            }
          }
        }
        stateLocal = kNumStates;
        probIdx = RepLenCoder;
      }
      {
        LOCAL(UInt32, limitSub);
        LOCAL(UInt32, offset);
        LOCAL_INIT(UInt32, probLenIdx, probIdx + LenChoice);
        ttt = GET_ARY16(probs, probLenIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
        if (codeLocal < bound) {
          rangeLocal = bound;
          probLenIdx = probIdx + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          limitSub = 1 << kLenNumLowBits;
        } else {
          rangeLocal -= bound; codeLocal -= bound;
          probLenIdx = probIdx + LenChoice2;
          ttt = GET_ARY16(probs, probLenIdx); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt;
          if (codeLocal < bound) {
            rangeLocal = bound;
            probLenIdx = probIdx + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            limitSub = 1 << kLenNumMidBits;
          } else {
            rangeLocal -= bound; codeLocal -= bound;
            probLenIdx = probIdx + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            limitSub = 1 << kLenNumHighBits;
          }
        }
        { len = 1; do { ttt = GET_ARY16(probs, probLenIdx + len); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; len = (len + len); ;; } else { rangeLocal -= bound; codeLocal -= bound; len = (len + len) + 1; ;; } } while (len < limitSub); len -= limitSub; };
        len += offset;
      }

      if (stateLocal < 4) {
        LOCAL(UInt32, posSlot);
        probIdx = PosSlotCode + ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
        { posSlot = 1; do { ttt = GET_ARY16(probs, probIdx + posSlot); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; posSlot = (posSlot + posSlot); ;; } else { rangeLocal -= bound; codeLocal -= bound; posSlot = (posSlot + posSlot) + 1; ;; } } while (posSlot < 1 << kNumPosSlotBits); posSlot -= 1 << kNumPosSlotBits; };
        if (posSlot >= kStartPosModelIndex) {
          LOCAL_INIT(UInt32, numDirectBits, (posSlot >> 1) - 1);
          if (posSlot < kEndPosModelIndex) {
            probIdx = SpecPos + ((2 | (posSlot & 1)) << numDirectBits) - posSlot - 1;
          } else {
            numDirectBits -= kNumAlignBits;
            do {
              if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }
              rangeLocal >>= 1;
              codeLocal -= rangeLocal & (((codeLocal - rangeLocal) >> 31) - 1);
            } while (--numDirectBits != 0);
            probIdx = Align;
            numDirectBits = kNumAlignBits;
          }
          {
            LOCAL_INIT(UInt32, i, 1);
            do
            {
              ttt = GET_ARY16(probs, probIdx + i); if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); }; bound = (rangeLocal >> kNumBitModelTotalBits) * ttt; if (codeLocal < bound) { rangeLocal = bound; i = (i + i); ;; } else { rangeLocal -= bound; codeLocal -= bound; i = (i + i) + 1; ;; };
            } while (--numDirectBits != 0);
          }
        }
      }
    }
  }
  if (rangeLocal < kTopValue) { if (bufDummyCur >= bufLimit) { return DUMMY_ERROR; } rangeLocal <<= 8; codeLocal = (codeLocal << 8) | (GET_ARY8(readBuf, bufDummyCur++)); };
  return res;
ENDFUNC

FUNC_ARG2(void, LzmaDec_InitDicAndState, const Bool, initDic, const Bool, initState)
  global.needFlush = TRUE;
  global.remainLen = 0;
  global.tempBufSize = 0;

  if (initDic) {
    global.processedPos = 0;
    global.checkDicSize = 0;
    global.needInitLzma = TRUE;
  }
  if (initState) {
    global.needInitLzma = TRUE;
  }
ENDFUNC

/* Decompress LZMA stream in
 * GET_ARY8(readBuf, global.readCur : global.readCur + srcLen).
 * On success (and on some errors as well), adds srcLen to global.readCur.
 */
FUNC_ARG1(SRes, LzmaDec_DecodeToDic, const UInt32, srcLen)
  /* Index limit in global.readBuf. */
  LOCAL_INIT(const UInt32, decodeLimit, global.readCur + srcLen);
  LzmaDec_WriteRem(global.dicBufSize);

  while (global.remainLen != kMatchSpecLenStart) {
    LOCAL(Bool, checkEndMarkNow);

    if (global.needFlush) {
      /* Read 5 bytes (RC_INIT_SIZE) to tempBuf, first of which must be
       * 0, initialize the range coder with the 4 bytes after the 0 byte.
       */
      for (; decodeLimit > global.readCur && global.tempBufSize < RC_INIT_SIZE;) {
        SET_ARY8(readBuf, READBUF_SIZE + global.tempBufSize++, GET_ARY8(readBuf, global.readCur++));
      }
      if (global.tempBufSize < RC_INIT_SIZE) {
       on_needs_more_input:
        if (decodeLimit != global.readCur) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
        return SZ_ERROR_NEEDS_MORE_INPUT;
      }
      if (GET_ARY8(readBuf, READBUF_SIZE) != 0) {
        return SZ_ERROR_DATA;
      }
      global.code = ((UInt32)GET_ARY8(readBuf, READBUF_SIZE + 1) << 24) | ((UInt32)GET_ARY8(readBuf, READBUF_SIZE + 2) << 16) | ((UInt32)GET_ARY8(readBuf, READBUF_SIZE + 3) << 8) | ((UInt32)GET_ARY8(readBuf, READBUF_SIZE + 4));
      global.range = 0xFFFFFFFF;
      global.needFlush = FALSE;
      global.tempBufSize = 0;
    }

    checkEndMarkNow = FALSE;
    if (global.dicPos >= global.dicBufSize) {
      if (global.remainLen == 0 && global.code == 0) {
        if (decodeLimit != global.readCur) { return SZ_ERROR_CHUNK_NOT_CONSUMED; }
        return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
      }
      if (global.remainLen != 0) {
        return SZ_ERROR_NOT_FINISHED;
      }
      checkEndMarkNow = TRUE;
    }

    if (global.needInitLzma) {
      LOCAL_INIT(UInt32, numProbs, Literal + ((UInt32)LZMA_LIT_SIZE << (global.lc + global.lp)));
      LOCAL(UInt32, i);
      for (i = 0; i < numProbs; i++) {
        SET_ARY16(probs, i, kBitModelTotal >> 1);
      }
      global.rep0 = global.rep1 = global.rep2 = global.rep3 = 1;
      global.state = 0;
      global.needInitLzma = FALSE;
    }

    if (global.tempBufSize == 0) {
      LOCAL(UInt32, bufLimit);
      if (decodeLimit - global.readCur < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
        LOCAL(SRes, dummyRes);
        dummyRes = LzmaDec_TryDummy(global.readCur, decodeLimit);
        if (dummyRes == DUMMY_ERROR) {
          /* This line can be triggered by passing srcLen==1 to LzmaDec_DecodeToDic. */
          global.tempBufSize = 0;
          while (global.readCur != decodeLimit) {
            SET_ARY8(readBuf, READBUF_SIZE + global.tempBufSize++, GET_ARY8(readBuf, global.readCur++));
          }
          goto on_needs_more_input;
        }
        if (checkEndMarkNow && dummyRes != DUMMY_MATCH) {
          return SZ_ERROR_NOT_FINISHED;
        }
        bufLimit = global.readCur;
      }
      else
        bufLimit = decodeLimit - LZMA_REQUIRED_INPUT_MAX;
      global.bufCur = global.readCur;
      if (LzmaDec_DecodeReal2(global.dicBufSize, bufLimit) != 0) {
        return SZ_ERROR_DATA;
      }
      global.readCur = global.bufCur;
    } else {
      LOCAL_INIT(UInt32, rem, global.tempBufSize);
      LOCAL_INIT(UInt32, lookAhead, 0);
      while (rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < decodeLimit - global.readCur) {
        SET_ARY8(readBuf, READBUF_SIZE + rem++, GET_ARY8(readBuf, global.readCur + lookAhead++));
      }
      global.tempBufSize = rem;
      if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow) {
        LOCAL(SRes, dummyRes);
        dummyRes = LzmaDec_TryDummy(READBUF_SIZE, READBUF_SIZE + rem);
        if (dummyRes == DUMMY_ERROR) {
          global.readCur += lookAhead;
          goto on_needs_more_input;
        }
        if (checkEndMarkNow && dummyRes != DUMMY_MATCH) {
          return SZ_ERROR_NOT_FINISHED;
        }
      }
      /* This line can be triggered by passing srcLen==1 to LzmaDec_DecodeToDic. */
      global.bufCur = READBUF_SIZE;  /* tempBuf. */
      if (LzmaDec_DecodeReal2(0, READBUF_SIZE) != 0) {
        return SZ_ERROR_DATA;
      }
      lookAhead -= rem - (global.bufCur - READBUF_SIZE);
      global.readCur += lookAhead;
      global.tempBufSize = 0;
    }
  }
  if (global.code != 0) { return SZ_ERROR_DATA; }
  return SZ_ERROR_FINISHED_WITH_MARK;
ENDFUNC

/* Tries to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if r <= READBUF_SIZE.
 */
FUNC_ARG1(UInt32, Preread, const UInt32, r)
  LOCAL_INIT(UInt32, p, global.readEnd - global.readCur);
  ASSERT(r <= READBUF_SIZE);
  if (p < r) {  /* Not enough pending available. */
    if (READBUF_SIZE - global.readCur + 0U < r) {
      /* If no room for r bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", p);
      for (global.readEnd = 0; global.readEnd < p; ++global.readEnd) {
        SET_ARY8(readBuf, global.readEnd, GET_ARY8(readBuf, global.readCur + global.readEnd));
      }
      global.readCur = 0;
    }
    while (p < r) {
      /* Instead of (r - p) we could use (global.readBuf + READBUF_SIZE -
       * global.readEnd) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", r - p);
      LOCAL_INIT(UInt32, got, READ_FROM_STDIN_TO_ARY8(readBuf, global.readEnd, r - p));
      if ((UInt32)(got - 1) & 0x80000000) break;  /* EOF or error on input. */
      global.readEnd += got;
      p += got;
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", r, p);
  return p;
ENDFUNC

FUNC_ARG0(void, IgnoreVarint)
  while (GET_ARY8(readBuf, global.readCur++) >= 0x80) {}
ENDFUNC

FUNC_ARG1(SRes, IgnoreZeroBytes, UInt32, c)
  for (; c > 0; --c) {
    if (GET_ARY8(readBuf, global.readCur++) != 0) {
      return SZ_ERROR_BAD_PADDING;
    }
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG1(UInt32, GetLE4, const UInt32, p)
  return GET_ARY8(readBuf, p) | GET_ARY8(readBuf, p + 1) << 8 | GET_ARY8(readBuf, p + 2) << 16 | GET_ARY8(readBuf, p + 3) << 24;
ENDFUNC

/* Expects global.dicSize be set already. Can be called before or after InitProp. */
FUNC_ARG0(void, InitDecode)
  /* global.lc = global.pb = global.lp = 0; */  /* needInitProp will initialize it */
  global.dicBufSize = 0;  /* We'll increment it later. */
  global.needInitDic = TRUE;
  global.needInitState = TRUE;
  global.needInitProp = TRUE;
  global.dicPos = 0;
  LzmaDec_InitDicAndState(TRUE, TRUE);
ENDFUNC

FUNC_ARG1(SRes, InitProp, Byte, b)
  LOCAL(UInt32, lc);
  LOCAL(UInt32, lp);
  if (b >= (9 * 5 * 5)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  lc = b % 9;
  b /= 9;
  global.pb = b / 5;
  lp = b % 5;
  if (lc + lp > LZMA2_LCLP_MAX) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  global.lc = lc;
  global.lp = lp;
  global.needInitProp = FALSE;
  return SZ_OK;
ENDFUNC

/* Writes uncompressed data (GET_ARY8(dic, fromDicPos : global.dicPos) to stdout. */
FUNC_ARG1(SRes, WriteFrom, UInt32, fromDicPos)
  DEBUGF("WRITE %d dicPos=%d\n", global.dicPos - fromDicPos, global.dicPos);
  while (fromDicPos != global.dicPos) {
    LOCAL_INIT(UInt32, got, WRITE_TO_STDOUT_FROM_ARY8(dic, fromDicPos, global.dicPos - fromDicPos));
    if (got & 0x80000000) { return SZ_ERROR_WRITE; }
    fromDicPos += got;
  }
  return SZ_OK;
ENDFUNC

/* Reads .xz or .lzma data from stdin, writes uncompressed bytes to stdout,
 * uses global.dic. It verifies some aspects of the file format (so it
 * can't be tricked to an infinite loop etc.), itdoesn't verify checksums
 * (e.g. CRC32).
 */
FUNC_ARG0(SRes, DecompressXzOrLzma)
  LOCAL(Byte, checksumSize);
  LOCAL(UInt32, bhf);  /* Block header flags */
  LOCAL(SRes, res);

  /* 12 for the stream header + 12 for the first block header + 6 for the
   * first chunk header. empty.xz is 32 bytes.
   */
  if (Preread(12 + 12 + 6) < 12 + 12 + 6) { return SZ_ERROR_INPUT_EOF; }
  /* readbuf[7] is actually stream flags, should also be 0. */
  if (GET_ARY8(readBuf, 0) == 0xfd && GET_ARY8(readBuf, 1) == 0x37 &&
      GET_ARY8(readBuf, 2) == 0x7a && GET_ARY8(readBuf, 3) == 0x58 &&
      GET_ARY8(readBuf, 4) == 0x5a && GET_ARY8(readBuf, 5) == 0 &&
      GET_ARY8(readBuf, 6) == 0) {  /* .xz: "\xFD""7zXZ\0" */
  } else if (GET_ARY8(readBuf, global.readCur) <= 225 && GET_ARY8(readBuf, global.readCur + 13) == 0 &&  /* .lzma */
        /* High 4 bytes of uncompressed size. */
        ((bhf = GetLE4(global.readCur + 9)) == 0 || bhf == ~(UInt32)0) &&
        (global.dicSize = GetLE4(global.readCur + 1)) >= LZMA_DIC_MIN &&
        global.dicSize <= DIC_ARRAY_SIZE) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    LOCAL(UInt32, us);
    LOCAL(UInt32, srcLen);
    LOCAL(UInt32, fromDicPos);
    InitDecode();
    /* LZMA restricts lc + lp <= 4. LZMA requires lc + lp <= 12.
     * We apply the LZMA2 restriction here (to save memory in
     * global.probs), thus we are not able to extract some legitimate
     * .lzma files.
     */
    if ((res = InitProp(GET_ARY8(readBuf, global.readCur))) != SZ_OK) {
      return res;
    }
    if (bhf == 0) {
      global.dicBufSize = us = GetLE4(global.readCur + 5);
      if (us > DIC_ARRAY_SIZE) { return SZ_ERROR_MEM; }
    } else {
      us = bhf;  /* max UInt32. */
      global.dicBufSize = DIC_ARRAY_SIZE;
    }
    global.readCur += 13;  /* Start decompressing the 0 byte. */
    DEBUGF("LZMA dicSize=0x%x us=%d bhf=%d\n", global.dicSize, us, bhf);
    /* TODO(pts): Limit on uncompressed size unless 8 bytes of -1 is
     * specified.
     */
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while ((srcLen = Preread(READBUF_SIZE)) > 0) {
      LOCAL(SRes, res);
      fromDicPos = global.dicPos;
      res = LzmaDec_DecodeToDic(srcLen);
      DEBUGF("LZMADEC res=%d\n", res);
      if (global.dicPos > us) global.dicPos = us;
      if ((res = WriteFrom(fromDicPos)) != SZ_OK) { return res; }
      if (res == SZ_ERROR_FINISHED_WITH_MARK) break;
      if (res != SZ_ERROR_NEEDS_MORE_INPUT && res != SZ_OK) { return res; }
      if (global.dicPos == us) break;
    }
    return SZ_OK;
  } else {
    return SZ_ERROR_BAD_MAGIC;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  switch (GET_ARY8(readBuf, global.readCur + 7)) {
   case 0: /* None */ checksumSize = 1; break;
   case 1: /* CRC32 */ checksumSize = 4; break;
   case 4: /* CRC64, typical xz output. */ checksumSize = 8; break;
   default: return SZ_ERROR_BAD_CHECKSUM_TYPE;
  }
  /* Also ignore the CRC32 after checksumSize. */
  global.readCur += 12;
  for (;;) {  /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    LOCAL_INIT(Byte, blockSizePad, 3);
    LOCAL(UInt32, bhs);
    LOCAL(UInt32, bhs2);  /* Block header size */
    LOCAL(Byte, dicSizeProp);
    LOCAL(UInt32, readAtBlock);
    ASSERT(global.readEnd - global.readCur >= 12);  /* At least 12 bytes preread. */
    readAtBlock = global.readCur;
    if ((bhs = GET_ARY8(readBuf, global.readCur++)) == 0) break;  /* Last block, index follows. */
    /* Block header size includes the bhs field above and the CRC32 below. */
    bhs = (bhs + 1) << 2;
    DEBUGF("bhs=%d\n", bhs);
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (Preread(bhs) < bhs) { return SZ_ERROR_INPUT_EOF; }
    readAtBlock = global.readCur;
    bhf = GET_ARY8(readBuf, global.readCur++);
    if ((bhf & 2) != 0) { return SZ_ERROR_UNSUPPORTED_FILTER_COUNT; }
    DEBUGF("filter count=%d\n", (bhf & 2) + 1);
    if ((bhf & 20) != 0) { return SZ_ERROR_BAD_BLOCK_FLAGS; }
    if (bhf & 64) {  /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (bhf & 128) {  /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (GET_ARY8(readBuf, global.readCur++) != FILTER_ID_LZMA2) { return SZ_ERROR_UNSUPPORTED_FILTER_ID; }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (GET_ARY8(readBuf, global.readCur++) != 1) { return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE; }
    dicSizeProp = GET_ARY8(readBuf, global.readCur++);
    /* Typical large dictionary sizes:
     *
     *  * 35: 805306368 bytes == 768 MiB
     *  * 36: 1073741824 bytes == 1 GiB
     *  * 37: 1610612736 bytes, largest supported by .xz
     *  * 38: 2147483648 bytes == 2 GiB
     *  * 39: 3221225472 bytes == 3 GiB
     *  * 40: 4294967295 bytes, largest supported by .xz
     */
    DEBUGF("dicSizeProp=0x%02x\n", dicSizeProp);
    if (dicSizeProp > 40) { return SZ_ERROR_BAD_DICTIONARY_SIZE; }
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (dicSizeProp > 37) { return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE; }
    global.dicSize = (((UInt32)2 | ((dicSizeProp) & 1)) << ((dicSizeProp) / 2 + 11));
    ASSERT(global.dicSize >= LZMA_DIC_MIN);
    DEBUGF("dicSize39=%u\n", (((UInt32)2 | ((39) & 1)) << ((39) / 2 + 11)));
    DEBUGF("dicSize38=%u\n", (((UInt32)2 | ((38) & 1)) << ((38) / 2 + 11)));
    DEBUGF("dicSize37=%u\n", (((UInt32)2 | ((37) & 1)) << ((37) / 2 + 11)));
    DEBUGF("dicSize36=%u\n", (((UInt32)2 | ((36) & 1)) << ((36) / 2 + 11)));
    DEBUGF("dicSize35=%u\n", (((UInt32)2 | ((35) & 1)) << ((35) / 2 + 11)));
    bhs2 = global.readCur - readAtBlock + 5;  /* Won't overflow. */
    DEBUGF("bhs=%d bhs2=%d\n", bhs, bhs2);
    if (bhs2 > bhs) { return SZ_ERROR_BLOCK_HEADER_TOO_LONG; }
    if ((res = IgnoreZeroBytes(bhs - bhs2)) != SZ_OK) { return res; }
    global.readCur += 4;  /* Ignore CRC32. */
    /* Typically it's offset 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2\n");
    {  /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      LOCAL(UInt32, us);  /* Uncompressed chunk sizes. */
      LOCAL(UInt32, cs);  /* Compressed chunk size. */
      InitDecode();

      for (;;) {
        LOCAL(Byte, control);
        ASSERT(global.dicPos == global.dicBufSize);
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (Preread(6) < 6) { return SZ_ERROR_INPUT_EOF; }
        control = GET_ARY8(readBuf, global.readCur);
        DEBUGF("CONTROL control=0x%02x at=? inbuf=%d\n", control, global.readCur);
        if (control == 0) {
          DEBUGF("LASTFED\n");
          ++global.readCur;
          break;
        } else if (TRUNCATE_TO_8BIT(control - 3) < 0x80 - 3U) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        us = (GET_ARY8(readBuf, global.readCur + 1) << 8) + GET_ARY8(readBuf, global.readCur + 2) + 1;
        if (control < 3) {  /* Uncompressed chunk. */
          LOCAL_INIT(const Bool, initDic, control == 1);
          cs = us;
          global.readCur += 3;
          /* TODO(pts): Porting: TRUNCATE_TO_8BIT(blockSizePad) for Python and other unlimited-integer-range languages. */
          blockSizePad -= 3;
          if (initDic) {
            global.needInitProp = global.needInitState = TRUE;
            global.needInitDic = FALSE;
          } else if (global.needInitDic) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(initDic, FALSE);
        } else {  /* LZMA chunk. */
          LOCAL_INIT(const Byte, mode, (((control) >> 5) & 3));
          LOCAL_INIT(const Bool, initDic, mode == 3);
          LOCAL_INIT(const Bool, initState, mode > 0);
          LOCAL_INIT(const Bool, isProp, (control & 64) != 0);
          us += (control & 31) << 16;
          cs = (GET_ARY8(readBuf, global.readCur + 3) << 8) + GET_ARY8(readBuf, global.readCur + 4) + 1;
          if (isProp) {
            if ((res = InitProp(GET_ARY8(readBuf, global.readCur + 5))) != SZ_OK) {
              return res;
            }
            ++global.readCur;
            --blockSizePad;
          } else {
            if (global.needInitProp) { return SZ_ERROR_MISSING_INITPROP; }
          }
          global.readCur += 5;
          blockSizePad -= 5;
          if ((!initDic && global.needInitDic) || (!initState && global.needInitState)) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(initDic, initState);
          global.needInitDic = FALSE;
          global.needInitState = FALSE;
        }
        ASSERT(global.dicPos == global.dicBufSize);
        global.dicBufSize += us;
        /* Decompressed data too long, won't fit to global.dic. */
        if (global.dicBufSize > DIC_ARRAY_SIZE) { return SZ_ERROR_MEM; }
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (Preread(cs + 6) < cs) { return SZ_ERROR_INPUT_EOF; }
        DEBUGF("FEED us=%d cs=%d dicPos=%d\n", us, cs, global.dicPos);
        if (control < 0x80) {  /* Uncompressed chunk. */
          DEBUGF("DECODE uncompressed\n");
          while (global.dicPos != global.dicBufSize) {
            SET_ARY8(dic, global.dicPos++, GET_ARY8(readBuf, global.readCur++));
          }
          if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= us) {
            global.checkDicSize = global.dicSize;
          }
          global.processedPos += us;
        } else {  /* Compressed chunk. */
          DEBUGF("DECODE call\n");
          /* This call doesn't change global.dicBufSize. */
          if ((res = LzmaDec_DecodeToDic(cs)) != SZ_OK) { return res; }
        }
        if (global.dicPos != global.dicBufSize) { return SZ_ERROR_BAD_DICPOS; }
        if ((res = WriteFrom(global.dicPos - us)) != SZ_OK) { return res; }
        blockSizePad -= cs;
        /* We can't discard decompressbuf[:global.dicBufSize] now,
         * because we need it a dictionary in which subsequent calls to
         * Lzma2Dec_DecodeToDic will look up backreferences.
         */
      }
    }  /* End of LZMA2 stream. */
    /* End of block. */
    /* 7 for padding4 and CRC32 + 12 for the next block header + 6 for the next
     * chunk header.
     */
    if (Preread(7 + 12 + 6) < 7 + 12 + 6) { return SZ_ERROR_INPUT_EOF; }
    DEBUGF("ALTELL blockSizePad=%d\n", blockSizePad & 3);
    /* Ignore block padding. */
    if ((res = IgnoreZeroBytes(blockSizePad & 3)) != SZ_OK) { return res; }
    global.readCur += checksumSize;  /* Ignore CRC32, CRC64 etc. */
  }
  /* The .xz input file continues with the index, which we ignore from here. */
  return SZ_OK;
ENDFUNC

#ifdef CONFIG_LANG_C
int main(int argc, char **argv) {
  (void)argc; (void)argv;
#if defined(MSDOS) || defined(_WIN32)  /* Also MinGW. Good. */
  setmode(0, O_BINARY);
  setmode(1, O_BINARY);
#endif
  return DecompressXzOrLzma();
}
#endif
