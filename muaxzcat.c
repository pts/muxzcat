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

#ifdef CONFIG_DEBUG_VARS
#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG 1
#endif
#endif

#ifdef CONFIG_LANG_PERL
START_PREPROCESSED
#else
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
#include <unistd.h>  /* read(), write() */
#ifdef _WIN32
#  include <windows.h>
#endif
#if defined(MSDOS) || defined(_WIN32)
#  include <fcntl.h>  /* setmode() */
#endif
#include <stdint.h>
#endif  /* __XTINY__ */
#endif  /* Not __TINYC__. */
#endif  /* Not CONFIG_LANG_PERL. */

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

/* In Perl, BREAK and CONTINUE don't work (it silently goes to outer.
 * loops) in a do { ... } while (...) loop. So use `goto' instead of
 * BREAK and CONTINUE in such loops.
 *
 * !! Is a `for (;;) { ...; last unless ...; }' loop faster than a do-while
 *    loop in Perl?
 */
#ifdef CONFIG_LANG_C
#define ELSE_IF else if
#define BREAK break
#define CONTINUE continue
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define ELSE_IF elsif
#define BREAK last
#define CONTINUE next
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_C
#define ENSURE_32BIT(x) ((uint32_t)(x))  /* Only C needs it. */
#define TRUNCATE_TO_32BIT(x) ((uint32_t)(x))
#define TRUNCATE_TO_16BIT(x) ((uint16_t)(x))
#define TRUNCATE_TO_8BIT(x) ((uint8_t)(x))
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define ENSURE_32BIT(x) (x)
#define TRUNCATE_TO_32BIT(x) ((x) & 0xffffffff)
#define TRUNCATE_TO_16BIT(x) ((x) & 0xffff)
#define TRUNCATE_TO_8BIT(x) ((x) & 0xff)
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_C
#define GLOBALS struct Global {
#define GLOBAL(type, name) type name
#define GLOBAL_ARY16(a, size) uint16_t a##16[size]
#define GLOBAL_ARY8(a, size) uint8_t a##8[size]
#define ENDGLOBALS } global;
#define GLOBAL_VAR(name) global.name  /* Get or set a global variable. */
#define SET_GLOBAL(name, setid, op) global.name op
#define GET_ARY16(a, idx) ((UInt32)global.a##16[TRUNCATE_TO_32BIT(idx)])
/* TRUNCATE_TO_16BIT is must be called on value manually if needed. */
#define SET_ARY16(a, idx, value) (global.a##16[TRUNCATE_TO_32BIT(idx)] = value)
#define CLEAR_ARY8(a)
#define GET_ARY8(a, idx) ((UInt32)global.a##8[TRUNCATE_TO_32BIT(idx)])
/* TRUNCATE_TO_8BIT must be called on value manually if needed. */
#define SET_ARY8(a, idx, value) (global.a##8[TRUNCATE_TO_32BIT(idx)] = value)
#define CLEAR_ARY16(a)
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) (read(0, &global.a##8[fromIdx], (size)))
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) (write(1, &global.a##8[fromIdx], (size)))
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define GLOBALS
#define GLOBAL(type, name) GLOBAL my($##name) = 0
#define GLOBAL_ARY16(a, size) GLOBAL my($##a) = ''
#define GLOBAL_ARY8(a, size) GLOBAL my($##a) = ''
#define ENDGLOBALS
#define GLOBAL_VAR(name) $##name
#define SET_GLOBAL(name, setid, op) $##name op
/* !! Not all indexes can be more than 32-bit. Optimize away the long ones. */
#define GET_ARY16(a, idx) vec($##a, TRUNCATE_TO_32BIT(idx), 16)
#define SET_ARY16(a, idx, value) vec($##a, TRUNCATE_TO_32BIT(idx), 16) = value
#define CLEAR_ARY16(a) $##a = ''
#define GET_ARY8(a, idx) vec($##a, TRUNCATE_TO_32BIT(idx), 8)
#define SET_ARY8(a, idx, value) vec($##a, TRUNCATE_TO_32BIT(idx), 8) = value
#define CLEAR_ARY8(a) $##a = ''
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) UndefToMinus1(sysread(STDIN, $##a, (size), (fromIdx)))
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) UndefToMinus1(syswrite(STDOUT, $##a, (size), (fromIdx)))
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_C
#define LOCAL(type, name) type LOCAL_##name
#define LOCAL_INIT(type, name, value) type LOCAL_##name = value
#define LOCAL_VAR(name) LOCAL_##name  /* Get or set a local variable or function argument. */
#define SET_LOCALB(name, setid, op, value) LOCAL_##name op value
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define LOCAL(type, name) my $##name
#define LOCAL_INIT(type, name, value) my $##name = value
#define LOCAL_VAR(name) $##name  /* Get or set a local variable or function argument. */
#define SET_LOCALB(name, setid, op, value) $##name op value
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_C
#define FUNC_ARG0(return_type, name) return_type name(void) {
#define FUNC_ARG1(return_type, name, arg1_type, arg1) return_type name(arg1_type LOCAL_##arg1) {
#define FUNC_ARG2(return_type, name, arg1_type, arg1, arg2_type, arg2) return_type name(arg1_type LOCAL_##arg1, arg2_type LOCAL_##arg2) {
#define ENDFUNC }
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define FUNC_ARG0(return_type, name) sub name() {
#define FUNC_ARG1(return_type, name, arg1_type, arg1) sub name($) { my $##arg1 = $_[0];
#define FUNC_ARG2(return_type, name, arg1_type, arg1, arg2_type, arg2) sub name($$) { my($##arg1, $##arg2) = @_;
#define ENDFUNC }
#endif  /* CONFIG_LANG_PERL */

/* TODO(pts): Simplify doublings: e.g .LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) */
/* !! Instrument ++ and -- operators. */
/* !! Optimized masking of the index of GET_ARY8, GET_ARY16, SET_ARY8, SET_ARY16? */
/* !! Optimized masking of EQ(x, 0) and NE(x, 0), GT(x, 0) etc. */
/* !! Inline shr32 with constant values. */
/* !! Use SHR_SMALL more instead of SHR1, SHR5, SHR11, wherever it works.
 * SHR1(LOCAL_VAR(distance));  Is distance small?
 * SHR1(LOCAL_VAR(rangeLocal));  Is rangeLocal small?
 * SHR11(LOCAL_VAR(rangeLocal))
 */
/* !! Use comparison *_SMALL more, wherever it works. */
/* The code doesn't have overflowing / /= % %=, so we don't create macros for these. */
#ifdef CONFIG_LANG_C
#if defined(CONFIG_UINT64) || defined(CONFIG_INT64)
#define SHR_SMALL(x, y) ((x) >> (y))
#define SHR1(x) (((x) & 0xffffffff) >> 1)
#define SHR11(x) (((x) & 0xffffffff) >> 11)
#define EQ(x, y) ((((x) - (y)) & 0xffffffff) == 0)
#define NE(x, y) ((((x) - (y)) & 0xffffffff) != 0)
#define LT(x, y) (((x) & 0xffffffff) <  ((y) & 0xffffffff))
#define LE(x, y) (((x) & 0xffffffff) <= ((y) & 0xffffffff))
#define GT(x, y) (((x) & 0xffffffff) >  ((y) & 0xffffffff))
#define GE(x, y) (((x) & 0xffffffff) >= ((y) & 0xffffffff))
#define EQ_SMALL(x, y) EQ(x, y)
#define NE_SMALL(x, y) NE(x, y)
#define LT_SMALL(x, y) LT(x, y)
#define LE_SMALL(x, y) LE(x, y)
#define GT_SMALL(x, y) GT(x, y)
#define GE_SMALL(x, y) GE(x, y)
#else
#define SHR_SMALL(x, y) ((x) >> (y))  /* When 0 <= x < (2 ** 31). */
#define SHR1(x) ((x) >> 1)
#define SHR11(x) ((x) >> 11)
#define EQ(x, y) ((x) == (y))
#define NE(x, y) ((x) != (y))
#define LT(x, y) ((x) < (y))
#define LE(x, y) ((x) <= (y))
#define GT(x, y) ((x) > (y))
#define GE(x, y) ((x) >= (y))
#define EQ_SMALL(x, y) EQ(x, y)
#define NE_SMALL(x, y) NE(x, y)
#define LT_SMALL(x, y) LT(x, y)
#define LE_SMALL(x, y) LE(x, y)
#define GT_SMALL(x, y) GT(x, y)
#define GE_SMALL(x, y) GE(x, y)
#endif
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define EQ(x, y) ((((x) - (y)) & 0xffffffff) == 0)
#define NE(x, y) ((((x) - (y)) & 0xffffffff) != 0)
#if 0  /* This is for 64-bit Perl only. !! Autodetect 64-bit Perl. */
#define SHR_SMALL(x, y) ((x) >> (y))
#define SHR1(x) (((x) & 0xffffffff) >> 1)
#define SHR11(x) (((x) & 0xffffffff) >> 11)
#define LT(x, y) (((x) & 0xffffffff) <  ((y) & 0xffffffff))
#define LE(x, y) (((x) & 0xffffffff) <= ((y) & 0xffffffff))
#define GT(x, y) (((x) & 0xffffffff) >  ((y) & 0xffffffff))
#define GE(x, y) (((x) & 0xffffffff) >= ((y) & 0xffffffff))
#else
/* This works in both 32-bit a 64-bit Perl with `use integer'. */
sub lt32($$) {
  my $a = $_[0] & 0xffffffff;
  my $b = $_[1] & 0xffffffff;
  ($a < 0 ? $b >= 0 : $b < 0) ? $b < 0 : $a < $b
}
#define SHR_SMALL(x, y) ((x) >> (y))
#define SHR1(x) (((x) >> 1) & 0x7fffffff)
#define SHR11(x) (((x) >> 11) & (0x7fffffff >> 10))
#define LT(x, y) lt32(x, y)
#define LE(x, y) (!lt32(y, x))
#define GT(x, y) lt32(y, x)
#define GE(x, y) (!lt32(x, y))
#endif
#if 0  /* !! */
#define EQ_SMALL(x, y) ((x) == (y))
#define NE_SMALL(x, y) ((x) != (y))
#define LT_SMALL(x, y) ((x) < (y))
#define LE_SMALL(x, y) ((x) <= (y))
#define GT_SMALL(x, y) ((x) > (y))
#define GE_SMALL(x, y) ((x) >= (y))
#else
#define EQ_SMALL(x, y) EQ(x, y)
#define NE_SMALL(x, y) NE(x, y)
#define LT_SMALL(x, y) LT(x, y)
#define LE_SMALL(x, y) LE(x, y)
#define GT_SMALL(x, y) GT(x, y)
#define GE_SMALL(x, y) GE(x, y)
#endif
#endif  /* CONFIG_LANG_PERL */

/* --- */

#ifdef CONFIG_LANG_C
#ifdef CONFIG_DEBUG
/* This is guaranteed to work with Linux and gcc only. For example, %lld in
 * printf doesn't work with MinGW.
 */
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#define DEBUGF(...) fprintf(stderr, "DEBUG: " __VA_ARGS__)
#define ASSERT(condition) assert(condition)
#undef  SHR_SMALL
#define SHR_SMALL(x, y) ({ const UInt32 x2 = (x); ASSERT((x2 & ~0x7fffffff) == 0); x2 >> (y); })
#ifdef CONFIG_DEBUG_VARS
static void DumpVars(void);
/* ++ and -- operators for global and local variables could be instrumented
 * for completeness here.
 */
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ({ LOCAL_##name op value; DEBUGF("SET_LOCAL @%d %s=%u\n", setid, #name, (int)TRUNCATE_TO_32BIT(LOCAL_##name)); LOCAL_##name; })
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) *({ DumpVars(); DEBUGF("SET_GLOBAL %s @%d\n", #name, setid); &global.name; }) op
#undef  SET_ARY16
#define SET_ARY16(a, idx, value) ({ const UInt32 idx2 = idx; global.a##16[idx2] = value; DEBUGF("SET_ARY16 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##16[idx2])); global.a##16[idx2]; })
#undef  SET_ARY8
#define SET_ARY8(a, idx, value) ({ const UInt32 idx2 = idx; global.a##8[idx2] = value; DEBUGF("SET_ARY8 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##8[idx2])); global.a##8[idx2];  })
#endif  /* CONFIG_DEBUG_VARS */
#else
#define DEBUGF(...)
/* Just check that it compiles. */
#define ASSERT(condition) do {} while (0 && (condition))
#endif  /* !CONFIG_DEBUG */
#endif  /* CONFIG_LANG_C */

#ifdef CONFIG_LANG_PERL
#ifdef CONFIG_DEBUG
#define DEBUGF(...) printf(STDERR "DEBUG: " . __VA_ARGS__)
#define ASSERT(condition) die "ASSERT: " . #condition if !(condition)
#ifdef CONFIG_DEBUG_VARS
/* ${...} produced by SET_ARY16 and SET_ARY16. */
BEGIN { eval { require warnings; unimport warnings qw(void) }; }
sub DumpVars();
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ${ $##name op value; DEBUGF("SET_LOCAL @%d %s=%u\n", setid, #name, TRUNCATE_TO_32BIT($##name)); \$##name; }
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) ${DumpVars(), DEBUGF("SET_GLOBAL %s @%d\n", #name, setid), \$##name} op
#undef  SET_ARY16
#define SET_ARY16(a, idx, value) ${ my $setidx = TRUNCATE_TO_32BIT(idx); vec($##a, $setidx, 16) = value; DEBUGF("SET_ARY16 %s[%d]=%u\n", #a, $setidx, vec($##a, $setidx, 16)); \vec($##a, $setidx, 16) }
#undef  SET_ARY8
#define SET_ARY8(a, idx, value) ${ my $setidx = TRUNCATE_TO_32BIT(idx); vec($##a, $setidx, 8) = value; DEBUGF("SET_ARY8 %s[%d]=%u\n", #a, $setidx, vec($##a, $setidx, 8)); \vec($##a, $setidx, 8) }
#endif  /* CONFIG_DEBUG_VARS */
#else
#define DEBUGF(...)
/* Just check that it compiles. */
#define ASSERT(condition) do {} while (0 && (condition))
#endif  /* !CONFIG_DEBUG */
#endif  /* CONFIG_LANG_PERL */

/* --- */

#ifndef CONFIG_LANG_PERL  /* NUMERIC_CONSTANTS */
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
#endif  /* !CONFIG_LANG_PERL */

/* --- */

#ifdef CONFIG_LANG_C
#ifdef CONFIG_INT64  /* Specify -fwrapv for correct results. */
typedef int64_t UInt32;
typedef int64_t Byte;
typedef int64_t SRes;
typedef int64_t Bool;
#else
#ifdef CONFIG_UINT64  /* Make everything 64-bit, for compatibility tests before porting to other languages. */
typedef uint64_t UInt32;
typedef uint64_t Byte;
typedef uint64_t SRes;
typedef uint64_t Bool;
#else
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
#endif
#endif
#endif  /* CONFIG_LANG_C */

/* For LZMA streams, LE(lc + lp, 8 + 4), LE 12.
 * For LZMA2 streams, LE(lc + lp, 4).
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: sizeof(GET_ARY16(probs, 0)) * value == (2 or 4) * value bytes.
 */
/*#define LzmaProps_GetNumProbs(p) TRUNCATE_TO_32BIT(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp))) */

#ifdef CONFIG_LANG_C
/* This fails to compile if any condition after the : is false. */
struct LzmaAsserts {
  int Lzma2MaxNumProbsIsCorrect : LZMA2_MAX_NUM_PROBS == TRUNCATE_TO_32BIT(LZMA_BASE_SIZE + (LZMA_LIT_SIZE << LZMA2_LCLP_MAX));
};
#endif  /* CONFIG_LANG_C */

GLOBALS
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
  GLOBAL(Byte, lc);  /* Configured in prop byte. */
  GLOBAL(Byte, lp);  /* Configured in prop byte. */
  GLOBAL(Byte, pb);  /* Configured in prop byte. */
  GLOBAL(Byte, lcm8);  /* Cached (8 - lc), for speed. */
  GLOBAL_ARY16(probs, LZMA2_MAX_NUM_PROBS);  /* Probabilities for bit decoding. */
  /* The first READBUF_SIZE bytes is readBuf, then the
   * LZMA_REQUIRED_INPUT_MAX bytes is tempBuf.
   */
  GLOBAL_ARY8(readBuf, READBUF_SIZE + LZMA_REQUIRED_INPUT_MAX);
  /* Contains the uncompressed data.
   *
   * Array size is about 1.61 GB.
   * We rely on virtual memory so that if we don't use the end of array for
   * small files, then the operating system won't take the entire array away
   * from other processes.
   */
  GLOBAL_ARY8(dic, DIC_ARRAY_SIZE / 10);
ENDGLOBALS

#ifdef CONFIG_DEBUG_VARS
FUNC_ARG0(void, DumpVars)
  DEBUGF("GLOBALS bufCur=%u dicSize=%u range=%u code=%u dicPos=%u dicBufSize=%u processedPos=%u checkDicSize=%u state=%u rep0=%u rep1=%u rep2=%u rep3=%u remainLen=%u tempBufSize=%u readCur=%u readEnd=%u needFlush=%u needInitLzma=%u needInitDic=%u needInitState=%u needInitProp=%u lc=%u lp=%u pb=%u\n",
      ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(bufCur))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(dicSize))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(range))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(code))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(dicPos))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(dicBufSize))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(processedPos))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(checkDicSize))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(state))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(rep0))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(rep1))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(rep2))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(rep3))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(remainLen))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(tempBufSize))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(readCur))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(readEnd))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(needFlush))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(needInitLzma))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(needInitDic))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(needInitState))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(needInitProp))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(lc))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(lp))), ENSURE_32BIT(TRUNCATE_TO_32BIT(GLOBAL_VAR(pb))));
ENDFUNC
#ifdef CONFIG_LANG_C
void *DumpVarsUsed = (void*)DumpVars;
#endif  /* CONFIG_LANG_C */
#endif

#ifdef CONFIG_LANG_C
/* This fails to compile if any condition after the : is false. */
struct ProbsAsserts {
  int LiteralCode : Literal == LZMA_BASE_SIZE;
};
#endif  /* CONFIG_LANG_C */

#ifdef CONFIG_LANG_PERL
FUNC_ARG1(UInt32, UndefToMinus1, UInt32, value)
  return defined(LOCAL_VAR(value)) ? LOCAL_VAR(value) : -1;
ENDFUNC
#endif  /* CONFIG_LANG_PERL */

/* --- */

FUNC_ARG1(void, LzmaDec_WriteRem, UInt32, dicLimit)
  if (NE(GLOBAL_VAR(remainLen), 0) && LT(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    LOCAL_INIT(UInt32, localLen, GLOBAL_VAR(remainLen));
    if (LT(LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos), LOCAL_VAR(localLen))) {
      SET_LOCALB(localLen, 3, =, LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos)) ;
    }
    if (EQ(GLOBAL_VAR(checkDicSize), 0) && LE(GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos), LOCAL_VAR(localLen))) {
      SET_GLOBAL(checkDicSize, 2, =) GLOBAL_VAR(dicSize);
    }
    SET_GLOBAL(processedPos, 4, +=) LOCAL_VAR(localLen);
    SET_GLOBAL(remainLen, 6, -=) LOCAL_VAR(localLen);
    while (NE(LOCAL_VAR(localLen), 0)) {
      LOCAL_VAR(localLen)--;
      SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
      GLOBAL_VAR(dicPos)++;
    }
  }
ENDFUNC

/* Modifies GLOBAL_VAR(bufCur) etc. */
FUNC_ARG2(SRes, LzmaDec_DecodeReal2, const UInt32, dicLimit, const UInt32, bufLimit)
  LOCAL_INIT(const UInt32, pbMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(pb))) - 1);
  LOCAL_INIT(const UInt32, lpMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(lp))) - 1);
  do {
    LOCAL_INIT(const UInt32, dicLimit2, EQ(GLOBAL_VAR(checkDicSize), 0) && LT(GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos), LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos)) ? GLOBAL_VAR(dicPos) + (GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos)) : LOCAL_VAR(dicLimit));
    LOCAL_INIT(UInt32, localLen, 0);
    LOCAL_INIT(UInt32, rangeLocal, GLOBAL_VAR(range));
    LOCAL_INIT(UInt32, codeLocal, GLOBAL_VAR(code));
    do {
      LOCAL(UInt32, probIdx);
      LOCAL(UInt32, bound);
      LOCAL(UInt32, ttt);  /* 0 <= LOCAL_VAR(ttt) <= kBitModelTotal. */
      LOCAL_INIT(UInt32, posState, GLOBAL_VAR(processedPos) & LOCAL_VAR(pbMask));

      SET_LOCALB(probIdx, 5, =, IsMatch + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(posState)) ;
      SET_LOCALB(ttt, 7, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 9, <<=, 8) ; SET_LOCALB(codeLocal, 11, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 13, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
      if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
        LOCAL(UInt32, symbol);
        ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal));
        SET_LOCALB(rangeLocal, 15, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + (SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))));
        SET_LOCALB(probIdx, 17, =, Literal) ;
        if (NE(GLOBAL_VAR(checkDicSize), 0) || NE(GLOBAL_VAR(processedPos), 0)) {
          SET_LOCALB(probIdx, 19, +=, (LZMA_LIT_SIZE * (((GLOBAL_VAR(processedPos) & LOCAL_VAR(lpMask)) << GLOBAL_VAR(lc)) + SHR_SMALL(GET_ARY8(dic, (EQ(GLOBAL_VAR(dicPos), 0) ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), GLOBAL_VAR(lcm8))))) ;
        }
        if (LT(GLOBAL_VAR(state), kNumLitStates)) {
          SET_GLOBAL(state, 8, -=) (LT(GLOBAL_VAR(state), 4)) ? GLOBAL_VAR(state) : 3;
          SET_LOCALB(symbol, 21, =, 1) ;
          do {
            SET_LOCALB(ttt, 23, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 25, <<=, 8) ; SET_LOCALB(codeLocal, 27, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 29, =, (SHR11(LOCAL_VAR(rangeLocal))) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 31, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(symbol, 33, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol))); } else { SET_LOCALB(rangeLocal, 35, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 37, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(symbol, 39, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1); }
          } while (LT_SMALL(LOCAL_VAR(symbol), 0x100));
        } else {
          LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
          LOCAL_INIT(UInt32, offs, 0x100);
          SET_GLOBAL(state, 10, -=) LT(GLOBAL_VAR(state), 10) ? 3 : 6;
          SET_LOCALB(symbol, 41, =, 1) ;
          do {
            LOCAL(UInt32, localBit);
            LOCAL(UInt32, probLitIdx);
            SET_LOCALB(matchByte, 43, <<=, 1) ;
            SET_LOCALB(localBit, 45, =, (LOCAL_VAR(matchByte) & LOCAL_VAR(offs))) ;
            SET_LOCALB(probLitIdx, 47, =, LOCAL_VAR(probIdx) + LOCAL_VAR(offs) + LOCAL_VAR(localBit) + LOCAL_VAR(symbol)) ;
            SET_LOCALB(ttt, 49, =, GET_ARY16(probs, LOCAL_VAR(probLitIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 51, <<=, 8) ; SET_LOCALB(codeLocal, 53, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 55, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 57, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLitIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(symbol, 59, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol))) ; SET_LOCALB(offs, 61, &=, ~LOCAL_VAR(localBit)) ; } else { SET_LOCALB(rangeLocal, 63, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 65, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLitIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(symbol, 67, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1) ; SET_LOCALB(offs, 69, &=, LOCAL_VAR(localBit)) ; }
          } while (LT_SMALL(LOCAL_VAR(symbol), 0x100));
        }
        SET_ARY8(dic, GLOBAL_VAR(dicPos)++, TRUNCATE_TO_8BIT(LOCAL_VAR(symbol)));
        GLOBAL_VAR(processedPos)++;
        goto continue_do2;  /* CONTINUE; */
      } else {
        SET_LOCALB(rangeLocal, 71, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 73, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
        SET_LOCALB(probIdx, 75, =, IsRep + GLOBAL_VAR(state)) ;
        SET_LOCALB(ttt, 77, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 79, <<=, 8) ; SET_LOCALB(codeLocal, 81, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 83, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
        if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
          ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal));
          SET_LOCALB(rangeLocal, 85, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
          SET_GLOBAL(state, 12, +=) kNumStates;
          SET_LOCALB(probIdx, 87, =, LenCoder) ;
        } else {
          SET_LOCALB(rangeLocal, 89, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 91, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
          if (EQ(GLOBAL_VAR(checkDicSize), 0) && EQ(GLOBAL_VAR(processedPos), 0)) {
            return SZ_ERROR_DATA;
          }
          SET_LOCALB(probIdx, 93, =, IsRepG0 + GLOBAL_VAR(state)) ;
          SET_LOCALB(ttt, 95, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 97, <<=, 8) ; SET_LOCALB(codeLocal, 99, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 101, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
          if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
            ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal));
            SET_LOCALB(rangeLocal, 103, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
            SET_LOCALB(probIdx, 105, =, IsRep0Long + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(posState)) ;
            SET_LOCALB(ttt, 107, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 109, <<=, 8) ; SET_LOCALB(codeLocal, 111, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 113, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
            if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
              SET_LOCALB(rangeLocal, 115, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
              SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
              GLOBAL_VAR(dicPos)++;
              GLOBAL_VAR(processedPos)++;
              SET_GLOBAL(state, 14, =) LT(GLOBAL_VAR(state), kNumLitStates) ? 9 : 11;
              goto continue_do2;  /* CONTINUE; */
            }
            SET_LOCALB(rangeLocal, 117, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 119, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
          } else {
            LOCAL(UInt32, distance);
            SET_LOCALB(rangeLocal, 121, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 123, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
            SET_LOCALB(probIdx, 125, =, IsRepG1 + GLOBAL_VAR(state)) ;
            SET_LOCALB(ttt, 127, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 129, <<=, 8) ; SET_LOCALB(codeLocal, 131, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 133, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
            if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
              SET_LOCALB(rangeLocal, 135, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
              SET_LOCALB(distance, 137, =, GLOBAL_VAR(rep1)) ;
            } else {
              SET_LOCALB(rangeLocal, 139, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 141, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
              SET_LOCALB(probIdx, 143, =, IsRepG2 + GLOBAL_VAR(state)) ;
              SET_LOCALB(ttt, 145, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 147, <<=, 8) ; SET_LOCALB(codeLocal, 149, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 151, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
              if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
                SET_LOCALB(rangeLocal, 153, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
                SET_LOCALB(distance, 155, =, GLOBAL_VAR(rep2)) ;
              } else {
                SET_LOCALB(rangeLocal, 157, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 159, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
                SET_LOCALB(distance, 161, =, GLOBAL_VAR(rep3)) ;
                SET_GLOBAL(rep3, 16, =) GLOBAL_VAR(rep2);
              }
              SET_GLOBAL(rep2, 18, =) GLOBAL_VAR(rep1);
            }
            SET_GLOBAL(rep1, 20, =) GLOBAL_VAR(rep0);
            SET_GLOBAL(rep0, 22, =) LOCAL_VAR(distance);
          }
          SET_GLOBAL(state, 24, =) LT(GLOBAL_VAR(state), kNumLitStates) ? 8 : 11;
          SET_LOCALB(probIdx, 163, =, RepLenCoder) ;
        }
        {
          LOCAL(UInt32, limitSub);
          LOCAL(UInt32, offset);
          LOCAL_INIT(UInt32, probLenIdx, LOCAL_VAR(probIdx) + LenChoice);
          SET_LOCALB(ttt, 165, =, GET_ARY16(probs, LOCAL_VAR(probLenIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 167, <<=, 8) ; SET_LOCALB(codeLocal, 169, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 171, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
          if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
            ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal));
            SET_LOCALB(rangeLocal, 173, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
            SET_LOCALB(probLenIdx, 175, =, LOCAL_VAR(probIdx) + LenLow + (LOCAL_VAR(posState) << (kLenNumLowBits))) ;
            SET_LOCALB(offset, 177, =, 0) ;
            SET_LOCALB(limitSub, 179, =, (ENSURE_32BIT(1) << (kLenNumLowBits))) ;
          } else {
            SET_LOCALB(rangeLocal, 181, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 183, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
            SET_LOCALB(probLenIdx, 185, =, LOCAL_VAR(probIdx) + LenChoice2) ;
            SET_LOCALB(ttt, 187, =, GET_ARY16(probs, LOCAL_VAR(probLenIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 189, <<=, 8) ; SET_LOCALB(codeLocal, 191, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 193, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
            if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
              ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal));
              SET_LOCALB(rangeLocal, 195, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5)));
              SET_LOCALB(probLenIdx, 197, =, LOCAL_VAR(probIdx) + LenMid + (LOCAL_VAR(posState) << (kLenNumMidBits))) ;
              SET_LOCALB(offset, 199, =, kLenNumLowSymbols) ;
              SET_LOCALB(limitSub, 201, =, ENSURE_32BIT(1) << (kLenNumMidBits)) ;
            } else {
              SET_LOCALB(rangeLocal, 203, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 205, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5)));
              SET_LOCALB(probLenIdx, 207, =, LOCAL_VAR(probIdx) + LenHigh) ;
              SET_LOCALB(offset, 209, =, kLenNumLowSymbols + kLenNumMidSymbols) ;
              SET_LOCALB(limitSub, 211, =, ENSURE_32BIT(1) << (kLenNumHighBits)) ;
            }
          }
          {
            SET_LOCALB(localLen, 213, =, 1) ;
            do {
              { SET_LOCALB(ttt, 215, =, GET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 217, <<=, 8) ; SET_LOCALB(codeLocal, 219, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 221, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 223, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localLen, 225, =, (LOCAL_VAR(localLen) + LOCAL_VAR(localLen))); } else { SET_LOCALB(rangeLocal, 227, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 229, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localLen, 231, =, (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)) + 1); } }
            } while (LT(LOCAL_VAR(localLen), LOCAL_VAR(limitSub)));
            SET_LOCALB(localLen, 233, -=, LOCAL_VAR(limitSub)) ;
          }
          SET_LOCALB(localLen, 235, +=, LOCAL_VAR(offset)) ;
        }

        if (GE(GLOBAL_VAR(state), kNumStates)) {
          LOCAL(UInt32, distance);
          SET_LOCALB(probIdx, 237, =, PosSlotCode + (ENSURE_32BIT(LT(LOCAL_VAR(localLen), kNumLenToPosStates) ? LOCAL_VAR(localLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits))) ;
          {
            SET_LOCALB(distance, 239, =, 1) ;
            do {
              { SET_LOCALB(ttt, 241, =, GET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance)))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 243, <<=, 8) ; SET_LOCALB(codeLocal, 245, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 247, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 249, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(distance, 251, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance))); } else { SET_LOCALB(rangeLocal, 253, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 255, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(distance, 257, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)) + 1); } }
            } while (LT_SMALL(LOCAL_VAR(distance), (1 << 6)));
            SET_LOCALB(distance, 259, -=, (1 << 6)) ;
          }
          if (GE_SMALL(LOCAL_VAR(distance), kStartPosModelIndex)) {
            LOCAL_INIT(const UInt32, posSlot, LOCAL_VAR(distance));
            LOCAL_INIT(UInt32, numDirectBits, SHR1(LOCAL_VAR(distance)) - 1);
            SET_LOCALB(distance, 261, =, (2 | (LOCAL_VAR(distance) & 1))) ;
            if (LT(LOCAL_VAR(posSlot), kEndPosModelIndex)) {
              SET_LOCALB(distance, 263, <<=, LOCAL_VAR(numDirectBits)) ;
              SET_LOCALB(probIdx, 265, =, SpecPos + LOCAL_VAR(distance) - LOCAL_VAR(posSlot) - 1) ;
              {
                LOCAL_INIT(UInt32, mask, 1);
                LOCAL_INIT(UInt32, localI, 1);
                do {
                  SET_LOCALB(ttt, 267, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 269, <<=, 8) ; SET_LOCALB(codeLocal, 271, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 273, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 275, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 277, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 279, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 281, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 283, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1) ; SET_LOCALB(distance, 285, |=, LOCAL_VAR(mask)) ; }
                  SET_LOCALB(mask, 287, <<=, 1) ;
                } while (NE(--LOCAL_VAR(numDirectBits), 0));
              }
            } else {
              SET_LOCALB(numDirectBits, 289, -=, kNumAlignBits) ;
              do {
                if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 291, <<=, 8) ; SET_LOCALB(codeLocal, 293, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; }
                SET_LOCALB(rangeLocal, 2951, =, SHR1(LOCAL_VAR(rangeLocal)));
                if ((LOCAL_VAR(codeLocal) - LOCAL_VAR(rangeLocal)) & 0x80000000) {
                  SET_LOCALB(distance, 297, <<=, 1);
                } else {
                  SET_LOCALB(codeLocal, 295, -=, LOCAL_VAR(rangeLocal));
                  /* This won't be faster in Perl: <<= 1, ++ */
                  SET_LOCALB(distance, 301, =, (LOCAL_VAR(distance) << 1) + 1);
                }
              } while (NE(--LOCAL_VAR(numDirectBits), 0));
              SET_LOCALB(probIdx, 303, =, Align) ;
              SET_LOCALB(distance, 305, <<=, kNumAlignBits) ;
              {
                LOCAL_INIT(UInt32, localI, 1);
                SET_LOCALB(ttt, 307, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))); ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal)); if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 309, <<=, 8) ; SET_LOCALB(codeLocal, 311, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 313, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 315, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 317, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 319, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 321, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 323, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1) ; SET_LOCALB(distance, 325, |=, 1) ; }
                SET_LOCALB(ttt, 327, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))); ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal)); if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 329, <<=, 8) ; SET_LOCALB(codeLocal, 331, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 333, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 335, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 337, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 339, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 341, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 343, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1) ; SET_LOCALB(distance, 345, |=, 2) ; }
                SET_LOCALB(ttt, 347, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))); ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal)); if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 349, <<=, 8) ; SET_LOCALB(codeLocal, 351, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 353, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 355, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 357, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 359, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 361, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 363, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1) ; SET_LOCALB(distance, 365, |=, 4) ; }
                SET_LOCALB(ttt, 367, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))); ASSERT(LE(LOCAL_VAR(ttt), kBitModelTotal)); if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 369, <<=, 8) ; SET_LOCALB(codeLocal, 371, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(bound, 373, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 375, =, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR_SMALL(kBitModelTotal - LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 377, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 379, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 381, -=, LOCAL_VAR(bound)) ; SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR_SMALL(LOCAL_VAR(ttt), 5))); SET_LOCALB(localI, 383, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1) ; SET_LOCALB(distance, 385, |=, 8) ; }
              }
              if (EQ(~LOCAL_VAR(distance), 0)) {
                SET_LOCALB(localLen, 387, +=, kMatchSpecLenStart) ;
                SET_GLOBAL(state, 26, -=) kNumStates;
                goto break_do2;  /* BREAK; */
              }
            }
          }
          SET_GLOBAL(rep3, 28, =) GLOBAL_VAR(rep2);
          SET_GLOBAL(rep2, 30, =) GLOBAL_VAR(rep1);
          SET_GLOBAL(rep1, 32, =) GLOBAL_VAR(rep0);
          SET_GLOBAL(rep0, 34, =) LOCAL_VAR(distance) + 1;
          if (EQ(GLOBAL_VAR(checkDicSize), 0)) {
            if (GE(LOCAL_VAR(distance), GLOBAL_VAR(processedPos))) {
              return SZ_ERROR_DATA;
            }
          } else {
            if (GE(LOCAL_VAR(distance), GLOBAL_VAR(checkDicSize))) {
              return SZ_ERROR_DATA;
            }
          }
          SET_GLOBAL(state, 36, =) LT(GLOBAL_VAR(state), kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        }

        SET_LOCALB(localLen, 389, +=, kMatchMinLen) ;

        if (EQ(LOCAL_VAR(dicLimit2), GLOBAL_VAR(dicPos))) {
          return SZ_ERROR_DATA;
        }
        {
          LOCAL_INIT(UInt32, rem, LOCAL_VAR(dicLimit2) - GLOBAL_VAR(dicPos));
          LOCAL_INIT(UInt32, curLen, (LT(LOCAL_VAR(rem), LOCAL_VAR(localLen)) ? LOCAL_VAR(rem) : LOCAL_VAR(localLen)));
          LOCAL_INIT(UInt32, pos, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0));

          SET_GLOBAL(processedPos, 38, +=) LOCAL_VAR(curLen);

          SET_LOCALB(localLen, 391, -=, LOCAL_VAR(curLen)) ;
          if (LE(LOCAL_VAR(pos) + LOCAL_VAR(curLen), GLOBAL_VAR(dicBufSize))) {
            ASSERT(GT(GLOBAL_VAR(dicPos), LOCAL_VAR(pos)));
            ASSERT(GT(LOCAL_VAR(curLen), 0));
            do {
              /* Here pos can be negative if 64-bit. */
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
            } while (NE(--LOCAL_VAR(curLen), 0));
          } else {
            do {
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
              if (EQ(LOCAL_VAR(pos), GLOBAL_VAR(dicBufSize))) { SET_LOCALB(pos, 393, =, 0) ; }
            } while (NE(--LOCAL_VAR(curLen), 0));
          }
        }
      }
     continue_do2: ;
    } while (LT(GLOBAL_VAR(dicPos), LOCAL_VAR(dicLimit2)) && LT(GLOBAL_VAR(bufCur), LOCAL_VAR(bufLimit)));
    break_do2: ;
    if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { SET_LOCALB(rangeLocal, 395, <<=, 8) ; SET_LOCALB(codeLocal, 397, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; }
    SET_GLOBAL(range, 40, =) LOCAL_VAR(rangeLocal);
    SET_GLOBAL(code, 42, =) LOCAL_VAR(codeLocal);
    SET_GLOBAL(remainLen, 44, =) LOCAL_VAR(localLen);
    if (GE_SMALL(GLOBAL_VAR(processedPos), GLOBAL_VAR(dicSize))) {
      SET_GLOBAL(checkDicSize, 46, =) GLOBAL_VAR(dicSize);
    }
    LzmaDec_WriteRem(LOCAL_VAR(dicLimit));
  } while (LT(GLOBAL_VAR(dicPos), LOCAL_VAR(dicLimit)) && LT(GLOBAL_VAR(bufCur), LOCAL_VAR(bufLimit)) && LT(GLOBAL_VAR(remainLen), kMatchSpecLenStart));

  if (GT(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    SET_GLOBAL(remainLen, 48, =) kMatchSpecLenStart;
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG2(Byte, LzmaDec_TryDummy, UInt32, bufDummyCur, const UInt32, bufLimit)
  LOCAL_INIT(UInt32, rangeLocal, GLOBAL_VAR(range));
  LOCAL_INIT(UInt32, codeLocal, GLOBAL_VAR(code));
  LOCAL_INIT(UInt32, stateLocal, GLOBAL_VAR(state));
  LOCAL(Byte, res);
  {
    LOCAL(UInt32, probIdx);
    LOCAL(UInt32, bound);
    LOCAL(UInt32, ttt);
    LOCAL_INIT(UInt32, posState, (GLOBAL_VAR(processedPos)) & ((1 << GLOBAL_VAR(pb)) - 1));

    SET_LOCALB(probIdx, 399, =, IsMatch + (LOCAL_VAR(stateLocal) << (kNumPosBitsMax)) + LOCAL_VAR(posState)) ;
    SET_LOCALB(ttt, 401, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 403, <<=, 8) ; SET_LOCALB(codeLocal, 405, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 407, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
    if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
      SET_LOCALB(rangeLocal, 409, =, LOCAL_VAR(bound)) ;
      SET_LOCALB(probIdx, 411, =, Literal) ;
      if (NE(GLOBAL_VAR(checkDicSize), 0) || NE(GLOBAL_VAR(processedPos), 0)) {
        SET_LOCALB(probIdx, 413, +=, (LZMA_LIT_SIZE * ((((GLOBAL_VAR(processedPos)) & ((1 << (GLOBAL_VAR(lp))) - 1)) << GLOBAL_VAR(lc)) + SHR_SMALL(GET_ARY8(dic, (EQ(GLOBAL_VAR(dicPos), 0) ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), GLOBAL_VAR(lcm8))))) ;
      }

      if (LT(LOCAL_VAR(stateLocal), kNumLitStates)) {
        LOCAL_INIT(UInt32, symbol, 1);
        do {
          SET_LOCALB(ttt, 415, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 417, <<=, 8) ; SET_LOCALB(codeLocal, 419, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 421, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 423, =, LOCAL_VAR(bound)) ; SET_LOCALB(symbol, 425, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol))); } else { SET_LOCALB(rangeLocal, 427, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 429, -=, LOCAL_VAR(bound)) ; SET_LOCALB(symbol, 431, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1); }
        } while (LT_SMALL(LOCAL_VAR(symbol), 0x100));
      } else {
        LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0) + (LT(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
        LOCAL_INIT(UInt32, offs, 0x100);
        LOCAL_INIT(UInt32, symbol, 1);
        do {
          LOCAL(UInt32, localBit);
          LOCAL(UInt32, probLitIdx);
          SET_LOCALB(matchByte, 433, <<=, 1) ;
          SET_LOCALB(localBit, 435, =, (LOCAL_VAR(matchByte) & LOCAL_VAR(offs))) ;
          SET_LOCALB(probLitIdx, 437, =, LOCAL_VAR(probIdx) + LOCAL_VAR(offs) + LOCAL_VAR(localBit) + LOCAL_VAR(symbol)) ;
          SET_LOCALB(ttt, 439, =, GET_ARY16(probs, LOCAL_VAR(probLitIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 441, <<=, 8) ; SET_LOCALB(codeLocal, 443, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 445, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 447, =, LOCAL_VAR(bound)) ; SET_LOCALB(symbol, 449, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol))) ; SET_LOCALB(offs, 451, &=, ~LOCAL_VAR(localBit)) ; } else { SET_LOCALB(rangeLocal, 453, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 455, -=, LOCAL_VAR(bound)) ; SET_LOCALB(symbol, 457, =, (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1) ; SET_LOCALB(offs, 459, &=, LOCAL_VAR(localBit)) ; }
        } while (LT_SMALL(LOCAL_VAR(symbol), 0x100));
      }
      SET_LOCALB(res, 461, =, DUMMY_LIT) ;
    } else {
      LOCAL(UInt32, localLen);
      SET_LOCALB(rangeLocal, 463, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 465, -=, LOCAL_VAR(bound)) ;
      SET_LOCALB(probIdx, 467, =, IsRep + LOCAL_VAR(stateLocal)) ;
      SET_LOCALB(ttt, 469, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 471, <<=, 8) ; SET_LOCALB(codeLocal, 473, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 475, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
      if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
        SET_LOCALB(rangeLocal, 477, =, LOCAL_VAR(bound)) ;
        SET_LOCALB(stateLocal, 479, =, 0) ;
        SET_LOCALB(probIdx, 481, =, LenCoder) ;
        SET_LOCALB(res, 483, =, DUMMY_MATCH) ;
      } else {
        SET_LOCALB(rangeLocal, 485, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 487, -=, LOCAL_VAR(bound)) ;
        SET_LOCALB(res, 489, =, DUMMY_REP) ;
        SET_LOCALB(probIdx, 491, =, IsRepG0 + LOCAL_VAR(stateLocal)) ;
        SET_LOCALB(ttt, 493, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 495, <<=, 8) ; SET_LOCALB(codeLocal, 497, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 499, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
        if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
          SET_LOCALB(rangeLocal, 501, =, LOCAL_VAR(bound)) ;
          SET_LOCALB(probIdx, 503, =, IsRep0Long + (LOCAL_VAR(stateLocal) << (kNumPosBitsMax)) + LOCAL_VAR(posState)) ;
          SET_LOCALB(ttt, 505, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 507, <<=, 8) ; SET_LOCALB(codeLocal, 509, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 511, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
          if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
            SET_LOCALB(rangeLocal, 513, =, LOCAL_VAR(bound)) ;
            if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 515, <<=, 8) ; SET_LOCALB(codeLocal, 517, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; }
            return DUMMY_REP;
          } else {
            SET_LOCALB(rangeLocal, 519, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 521, -=, LOCAL_VAR(bound)) ;
          }
        } else {
          SET_LOCALB(rangeLocal, 523, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 525, -=, LOCAL_VAR(bound)) ;
          SET_LOCALB(probIdx, 527, =, IsRepG1 + LOCAL_VAR(stateLocal)) ;
          SET_LOCALB(ttt, 529, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 531, <<=, 8) ; SET_LOCALB(codeLocal, 533, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 535, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
          if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
            SET_LOCALB(rangeLocal, 537, =, LOCAL_VAR(bound)) ;
          } else {
            SET_LOCALB(rangeLocal, 539, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 541, -=, LOCAL_VAR(bound)) ;
            SET_LOCALB(probIdx, 543, =, IsRepG2 + LOCAL_VAR(stateLocal)) ;
            SET_LOCALB(ttt, 545, =, GET_ARY16(probs, LOCAL_VAR(probIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 547, <<=, 8) ; SET_LOCALB(codeLocal, 549, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 551, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
            if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
              SET_LOCALB(rangeLocal, 553, =, LOCAL_VAR(bound)) ;
            } else {
              SET_LOCALB(rangeLocal, 555, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 557, -=, LOCAL_VAR(bound)) ;
            }
          }
        }
        SET_LOCALB(stateLocal, 559, =, kNumStates) ;
        SET_LOCALB(probIdx, 561, =, RepLenCoder) ;
      }
      {
        LOCAL(UInt32, limitSub);
        LOCAL(UInt32, offset);
        LOCAL_INIT(UInt32, probLenIdx, LOCAL_VAR(probIdx) + LenChoice);
        SET_LOCALB(ttt, 563, =, GET_ARY16(probs, LOCAL_VAR(probLenIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 565, <<=, 8) ; SET_LOCALB(codeLocal, 567, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 569, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
        if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
          SET_LOCALB(rangeLocal, 571, =, LOCAL_VAR(bound)) ;
          SET_LOCALB(probLenIdx, 573, =, LOCAL_VAR(probIdx) + LenLow + (LOCAL_VAR(posState) << (kLenNumLowBits))) ;
          SET_LOCALB(offset, 575, =, 0) ;
          SET_LOCALB(limitSub, 577, =, ENSURE_32BIT(1) << (kLenNumLowBits)) ;
        } else {
          SET_LOCALB(rangeLocal, 579, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 581, -=, LOCAL_VAR(bound)) ;
          SET_LOCALB(probLenIdx, 583, =, LOCAL_VAR(probIdx) + LenChoice2) ;
          SET_LOCALB(ttt, 585, =, GET_ARY16(probs, LOCAL_VAR(probLenIdx))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 587, <<=, 8) ; SET_LOCALB(codeLocal, 589, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 591, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ;
          if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) {
            SET_LOCALB(rangeLocal, 593, =, LOCAL_VAR(bound)) ;
            SET_LOCALB(probLenIdx, 595, =, LOCAL_VAR(probIdx) + LenMid + (LOCAL_VAR(posState) << (kLenNumMidBits))) ;
            SET_LOCALB(offset, 597, =, kLenNumLowSymbols) ;
            SET_LOCALB(limitSub, 599, =, ENSURE_32BIT(1) << (kLenNumMidBits)) ;
          } else {
            SET_LOCALB(rangeLocal, 601, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 603, -=, LOCAL_VAR(bound)) ;
            SET_LOCALB(probLenIdx, 605, =, LOCAL_VAR(probIdx) + LenHigh) ;
            SET_LOCALB(offset, 607, =, kLenNumLowSymbols + kLenNumMidSymbols) ;
            SET_LOCALB(limitSub, 609, =, ENSURE_32BIT(1) << (kLenNumHighBits)) ;
          }
        }
        {
          SET_LOCALB(localLen, 611, =, 1) ;
          do {
            SET_LOCALB(ttt, 613, =, GET_ARY16(probs, LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 615, <<=, 8) ; SET_LOCALB(codeLocal, 617, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 619, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 621, =, LOCAL_VAR(bound)) ; SET_LOCALB(localLen, 623, =, (LOCAL_VAR(localLen) + LOCAL_VAR(localLen))); } else { SET_LOCALB(rangeLocal, 625, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 627, -=, LOCAL_VAR(bound)) ; SET_LOCALB(localLen, 629, =, (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)) + 1); }
          } while (LT(LOCAL_VAR(localLen), LOCAL_VAR(limitSub)));
          SET_LOCALB(localLen, 631, -=, LOCAL_VAR(limitSub)) ;
        }
        SET_LOCALB(localLen, 633, +=, LOCAL_VAR(offset)) ;
      }

      if (LT(LOCAL_VAR(stateLocal), 4)) {
        LOCAL(UInt32, posSlot);
        SET_LOCALB(probIdx, 635, =, PosSlotCode + (ENSURE_32BIT(LT(LOCAL_VAR(localLen), kNumLenToPosStates) ? LOCAL_VAR(localLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits))) ;
        {
          SET_LOCALB(posSlot, 637, =, 1) ;
          do {
            SET_LOCALB(ttt, 639, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(posSlot))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 641, <<=, 8) ; SET_LOCALB(codeLocal, 643, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 645, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 647, =, LOCAL_VAR(bound)) ; SET_LOCALB(posSlot, 649, =, (LOCAL_VAR(posSlot) + LOCAL_VAR(posSlot))); } else { SET_LOCALB(rangeLocal, 651, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 653, -=, LOCAL_VAR(bound)) ; SET_LOCALB(posSlot, 655, =, (LOCAL_VAR(posSlot) + LOCAL_VAR(posSlot)) + 1); }
          } while (LT_SMALL(LOCAL_VAR(posSlot), ENSURE_32BIT(1) << (kNumPosSlotBits)));
          SET_LOCALB(posSlot, 657, -=, ENSURE_32BIT(1) << (kNumPosSlotBits)) ;
        }
        /* Small enough for SHR_SMALL(LOCAL_VAR(posSlot), ...). */
        ASSERT(LT(LOCAL_VAR(posSlot), ENSURE_32BIT(1) << (kNumPosSlotBits)));
        if (GE(LOCAL_VAR(posSlot), kStartPosModelIndex)) {
          LOCAL_INIT(UInt32, numDirectBits, SHR_SMALL(LOCAL_VAR(posSlot), 1) - 1);
          if (LT(LOCAL_VAR(posSlot), kEndPosModelIndex)) {
            SET_LOCALB(probIdx, 659, =, SpecPos + ((2 | (LOCAL_VAR(posSlot) & 1)) << LOCAL_VAR(numDirectBits)) - LOCAL_VAR(posSlot) - 1) ;
          } else {
            SET_LOCALB(numDirectBits, 661, -=, kNumAlignBits) ;
            do {
              if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 663, <<=, 8) ; SET_LOCALB(codeLocal, 665, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; }
              SET_LOCALB(rangeLocal, 6651, =, SHR1(LOCAL_VAR(rangeLocal)));
              if (!((LOCAL_VAR(codeLocal) - LOCAL_VAR(rangeLocal)) & 0x80000000)) {
                SET_LOCALB(codeLocal, 667, -=, LOCAL_VAR(rangeLocal));
              }
            } while (NE(--LOCAL_VAR(numDirectBits), 0));
            SET_LOCALB(probIdx, 669, =, Align) ;
            SET_LOCALB(numDirectBits, 671, =, kNumAlignBits) ;
          }
          {
            LOCAL_INIT(UInt32, localI, 1);
            do {
              SET_LOCALB(ttt, 673, =, GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI))) ; if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 675, <<=, 8) ; SET_LOCALB(codeLocal, 677, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; } SET_LOCALB(bound, 679, =, SHR11(LOCAL_VAR(rangeLocal)) * LOCAL_VAR(ttt)) ; if (LT(LOCAL_VAR(codeLocal), LOCAL_VAR(bound))) { SET_LOCALB(rangeLocal, 681, =, LOCAL_VAR(bound)) ; SET_LOCALB(localI, 683, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI))); } else { SET_LOCALB(rangeLocal, 685, -=, LOCAL_VAR(bound)) ; SET_LOCALB(codeLocal, 687, -=, LOCAL_VAR(bound)) ; SET_LOCALB(localI, 689, =, (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1); }
            } while (NE(--LOCAL_VAR(numDirectBits), 0));
          }
        }
      }
    }
  }
  if (LT(LOCAL_VAR(rangeLocal), kTopValue)) { if (GE_SMALL(LOCAL_VAR(bufDummyCur), LOCAL_VAR(bufLimit))) { return DUMMY_ERROR; } SET_LOCALB(rangeLocal, 691, <<=, 8) ; SET_LOCALB(codeLocal, 693, =, (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++))) ; }
  return LOCAL_VAR(res);
ENDFUNC

FUNC_ARG2(void, LzmaDec_InitDicAndState, const Bool, initDic, const Bool, initState)
  SET_GLOBAL(needFlush, 50, =) TRUE;
  SET_GLOBAL(remainLen, 52, =) 0;
  SET_GLOBAL(tempBufSize, 54, =) 0;

  if (LOCAL_VAR(initDic)) {
    SET_GLOBAL(processedPos, 56, =) 0;
    SET_GLOBAL(checkDicSize, 58, =) 0;
    SET_GLOBAL(needInitLzma, 60, =) TRUE;
  }
  if (LOCAL_VAR(initState)) {
    SET_GLOBAL(needInitLzma, 62, =) TRUE;
  }
ENDFUNC

/* Decompress LZMA stream in
 * readBuf8[GLOBAL_VAR(readCur) : GLOBAL_VAR(readCur) + LOCAL_VAR(srcLen)].
 * On success (and on some errors as well), adds LOCAL_VAR(srcLen) to GLOBAL_VAR(readCur).
 */
FUNC_ARG1(SRes, LzmaDec_DecodeToDic, const UInt32, srcLen)
  /* Index limit in GLOBAL_VAR(readBuf). */
  LOCAL_INIT(const UInt32, decodeLimit, GLOBAL_VAR(readCur) + LOCAL_VAR(srcLen));
  LzmaDec_WriteRem(GLOBAL_VAR(dicBufSize));

  while (NE(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    LOCAL(Bool, checkEndMarkNow);

    if (GLOBAL_VAR(needFlush)) {
      /* Read 5 bytes (RC_INIT_SIZE) to tempBuf, first of which must be
       * 0, initialize the range coder with the 4 bytes after the 0 byte.
       */
      while (GT_SMALL(LOCAL_VAR(decodeLimit), GLOBAL_VAR(readCur)) && LT_SMALL(GLOBAL_VAR(tempBufSize), RC_INIT_SIZE)) {
        SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
      }
      if (LT_SMALL(GLOBAL_VAR(tempBufSize), RC_INIT_SIZE)) {
       on_needs_more_input:
        if (NE(LOCAL_VAR(decodeLimit), GLOBAL_VAR(readCur))) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
        return SZ_ERROR_NEEDS_MORE_INPUT;
      }
      if (NE(GET_ARY8(readBuf, READBUF_SIZE), 0)) {
        return SZ_ERROR_DATA;
      }
      SET_GLOBAL(code, 64, =) (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 1)) << 24) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 2)) << 16) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 3)) << 8) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 4)));
      SET_GLOBAL(range, 66, =) 0xffffffff;
      SET_GLOBAL(needFlush, 68, =) FALSE;
      SET_GLOBAL(tempBufSize, 70, =) 0;
    }

    SET_LOCALB(checkEndMarkNow, 695, =, FALSE) ;
    if (GE_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize))) {
      if (EQ(GLOBAL_VAR(remainLen), 0) && EQ(GLOBAL_VAR(code), 0)) {
        if (NE(LOCAL_VAR(decodeLimit), GLOBAL_VAR(readCur))) { return SZ_ERROR_CHUNK_NOT_CONSUMED; }
        return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
      }
      if (NE(GLOBAL_VAR(remainLen), 0)) {
        return SZ_ERROR_NOT_FINISHED;
      }
      SET_LOCALB(checkEndMarkNow, 697, =, TRUE) ;
    }

    if (GLOBAL_VAR(needInitLzma)) {
      LOCAL_INIT(UInt32, numProbs, Literal + (ENSURE_32BIT(LZMA_LIT_SIZE) << (GLOBAL_VAR(lc) + GLOBAL_VAR(lp))));
      LOCAL(UInt32, probIdx);
      for (LOCAL_VAR(probIdx) = 0; LT_SMALL(LOCAL_VAR(probIdx), LOCAL_VAR(numProbs)); LOCAL_VAR(probIdx)++) {
        SET_ARY16(probs, LOCAL_VAR(probIdx), SHR_SMALL(kBitModelTotal, 1));
      }
      SET_GLOBAL(rep0, 72, =) SET_GLOBAL(rep1, 74, =) SET_GLOBAL(rep2, 76, =) SET_GLOBAL(rep3, 78, =) 1;
      SET_GLOBAL(state, 80, =) 0;
      SET_GLOBAL(needInitLzma, 82, =) FALSE;
    }

    if (EQ(GLOBAL_VAR(tempBufSize), 0)) {
      LOCAL(UInt32, bufLimit);
      if (LT_SMALL(LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur), LZMA_REQUIRED_INPUT_MAX) || LOCAL_VAR(checkEndMarkNow)) {
        LOCAL(SRes, dummyRes);
        SET_LOCALB(dummyRes, 699, =, LzmaDec_TryDummy(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) ;
        if (EQ_SMALL(LOCAL_VAR(dummyRes), DUMMY_ERROR)) {
          /* This line can be triggered by passing LOCAL_VAR(srcLen)=1 to LzmaDec_DecodeToDic. */
          SET_GLOBAL(tempBufSize, 84, =) 0;
          while (NE(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) {
            SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
          }
          goto on_needs_more_input;
        }
        if (LOCAL_VAR(checkEndMarkNow) && NE(LOCAL_VAR(dummyRes), DUMMY_MATCH)) {
          return SZ_ERROR_NOT_FINISHED;
        }
        SET_LOCALB(bufLimit, 701, =, GLOBAL_VAR(readCur)) ;
      } else {
        SET_LOCALB(bufLimit, 703, =, LOCAL_VAR(decodeLimit) - LZMA_REQUIRED_INPUT_MAX) ;
      }
      SET_GLOBAL(bufCur, 86, =) GLOBAL_VAR(readCur);
      if (NE(LzmaDec_DecodeReal2(GLOBAL_VAR(dicBufSize), LOCAL_VAR(bufLimit)), 0)) {
        return SZ_ERROR_DATA;
      }
      SET_GLOBAL(readCur, 88, =) GLOBAL_VAR(bufCur);
    } else {
      LOCAL_INIT(UInt32, rem, GLOBAL_VAR(tempBufSize));
      LOCAL_INIT(UInt32, lookAhead, 0);
      while (LT_SMALL(LOCAL_VAR(rem), LZMA_REQUIRED_INPUT_MAX) && LT_SMALL(LOCAL_VAR(lookAhead), LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur))) {
        SET_ARY8(readBuf, READBUF_SIZE + LOCAL_VAR(rem)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur) + LOCAL_VAR(lookAhead)++));
      }
      SET_GLOBAL(tempBufSize, 90, =) LOCAL_VAR(rem);
      if (LT_SMALL(LOCAL_VAR(rem), LZMA_REQUIRED_INPUT_MAX) || LOCAL_VAR(checkEndMarkNow)) {
        LOCAL(SRes, dummyRes);
        SET_LOCALB(dummyRes, 705, =, LzmaDec_TryDummy(READBUF_SIZE, READBUF_SIZE + LOCAL_VAR(rem))) ;
        if (EQ_SMALL(LOCAL_VAR(dummyRes), DUMMY_ERROR)) {
          SET_GLOBAL(readCur, 92, +=) LOCAL_VAR(lookAhead);
          goto on_needs_more_input;
        }
        if (LOCAL_VAR(checkEndMarkNow) && NE(LOCAL_VAR(dummyRes), DUMMY_MATCH)) {
          return SZ_ERROR_NOT_FINISHED;
        }
      }
      /* This line can be triggered by passing LOCAL_VAR(srcLen)=1 to LzmaDec_DecodeToDic. */
      SET_GLOBAL(bufCur, 94, =) READBUF_SIZE;  /* tempBuf. */
      if (NE(LzmaDec_DecodeReal2(0, READBUF_SIZE), 0)) {
        return SZ_ERROR_DATA;
      }
      SET_LOCALB(lookAhead, 707, -=, LOCAL_VAR(rem) - (GLOBAL_VAR(bufCur) - READBUF_SIZE)) ;
      SET_GLOBAL(readCur, 96, +=) LOCAL_VAR(lookAhead);
      SET_GLOBAL(tempBufSize, 98, =) 0;
    }
  }
  if (NE(GLOBAL_VAR(code), 0)) { return SZ_ERROR_DATA; }
  return SZ_ERROR_FINISHED_WITH_MARK;
ENDFUNC

/* Tries to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if LE(prereadPos, READBUF_SIZE).
 *
 * Maximum allowed prereadSize is READBUF_SIZE (< 66000).
 */
FUNC_ARG1(UInt32, Preread, const UInt32, prereadSize)
  LOCAL_INIT(UInt32, prereadPos, GLOBAL_VAR(readEnd) - GLOBAL_VAR(readCur));
  LOCAL(UInt32, got);
  ASSERT(LE(LOCAL_VAR(prereadSize), READBUF_SIZE));
  if (LT_SMALL(LOCAL_VAR(prereadPos), LOCAL_VAR(prereadSize))) {  /* Not enough pending available. */
    if (LT_SMALL(READBUF_SIZE - GLOBAL_VAR(readCur), LOCAL_VAR(prereadSize))) {
      /* If no room for LOCAL_VAR(prereadSize) bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", ENSURE_32BIT(LOCAL_VAR(prereadPos)));
      for (SET_GLOBAL(readEnd, 100, =) 0; LT_SMALL(GLOBAL_VAR(readEnd), LOCAL_VAR(prereadPos)); ++GLOBAL_VAR(readEnd)) {
        SET_ARY8(readBuf, GLOBAL_VAR(readEnd), GET_ARY8(readBuf, GLOBAL_VAR(readCur) + GLOBAL_VAR(readEnd)));
      }
      SET_GLOBAL(readCur, 102, =) 0;
    }
    while (LT_SMALL(LOCAL_VAR(prereadPos), LOCAL_VAR(prereadSize))) {
      /* Instead of (LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos)) we could use (GLOBAL_VAR(readBuf) + READBUF_SIZE -
       * GLOBAL_VAR(readEnd)) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", ENSURE_32BIT(LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos)));
      SET_LOCALB(got, 7071, =, READ_FROM_STDIN_TO_ARY8(readBuf, GLOBAL_VAR(readEnd), LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos)));
      if (LE_SMALL(LOCAL_VAR(got) + 1, 1)) { BREAK; }  /* EOF or error on input. */
      SET_GLOBAL(readEnd, 104, +=) LOCAL_VAR(got);
      SET_LOCALB(prereadPos, 709, +=, LOCAL_VAR(got)) ;
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", ENSURE_32BIT(LOCAL_VAR(prereadSize)), ENSURE_32BIT(LOCAL_VAR(prereadPos)));
  return LOCAL_VAR(prereadPos);
ENDFUNC

FUNC_ARG0(void, IgnoreVarint)
  while (GE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), 0x80)) {}
ENDFUNC

FUNC_ARG1(SRes, IgnoreZeroBytes, UInt32, zeroByteCount)
  for (; NE_SMALL(LOCAL_VAR(zeroByteCount), 0); --LOCAL_VAR(zeroByteCount)) {
    if (NE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), 0)) {
      return SZ_ERROR_BAD_PADDING;
    }
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG1(UInt32, GetLE4, const UInt32, localReadPos)
  return GET_ARY8(readBuf, LOCAL_VAR(localReadPos)) | GET_ARY8(readBuf, LOCAL_VAR(localReadPos) + 1) << 8 | GET_ARY8(readBuf, LOCAL_VAR(localReadPos) + 2) << 16 | GET_ARY8(readBuf, LOCAL_VAR(localReadPos) + 3) << 24;
ENDFUNC

/* Expects GLOBAL_VAR(dicSize) be set already. Can be called before or after InitProp. */
FUNC_ARG0(void, InitDecode)
  /* needInitProp will initialize it */
  /* SET_GLOBAL(lc, 106, =) SET_GLOBAL(pb, 108, =) SET_GLOBAL(lp, 110, =) 0; */
  SET_GLOBAL(dicBufSize, 112, =) 0;  /* We'll increment it later. */
  SET_GLOBAL(needInitDic, 114, =) TRUE;
  SET_GLOBAL(needInitState, 116, =) TRUE;
  SET_GLOBAL(needInitProp, 118, =) TRUE;
  SET_GLOBAL(dicPos, 120, =) 0;
  LzmaDec_InitDicAndState(TRUE, TRUE);
ENDFUNC

FUNC_ARG1(SRes, InitProp, Byte, propByte)
  if (GE_SMALL(LOCAL_VAR(propByte), 9 * 5 * 5)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  SET_GLOBAL(lc, 122, =) LOCAL_VAR(propByte) % 9;
  SET_GLOBAL(lcm8, 1222, =) 8 - GLOBAL_VAR(lc);
  SET_LOCALB(propByte, 711, /=, 9) ;
  SET_GLOBAL(pb, 124, =) LOCAL_VAR(propByte) / 5;
  SET_GLOBAL(lp, 126, =) LOCAL_VAR(propByte) % 5;
  if (GT_SMALL(GLOBAL_VAR(lc) + GLOBAL_VAR(lp), LZMA2_LCLP_MAX)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  SET_GLOBAL(needInitProp, 128, =) FALSE;
  return SZ_OK;
ENDFUNC

/* Writes uncompressed data dic[LOCAL_VAR(fromDicPos) : GLOBAL_VAR(dicPos)] to stdout. */
FUNC_ARG1(SRes, WriteFrom, UInt32, fromDicPos)
  DEBUGF("WRITE %d dicPos=%d\n", ENSURE_32BIT(GLOBAL_VAR(dicPos) - LOCAL_VAR(fromDicPos)), ENSURE_32BIT(GLOBAL_VAR(dicPos)));
  while (NE_SMALL(LOCAL_VAR(fromDicPos), GLOBAL_VAR(dicPos))) {
    LOCAL_INIT(UInt32, got, WRITE_TO_STDOUT_FROM_ARY8(dic, LOCAL_VAR(fromDicPos), GLOBAL_VAR(dicPos) - LOCAL_VAR(fromDicPos)));
    if (LOCAL_VAR(got) & 0x80000000) { return SZ_ERROR_WRITE; }
    SET_LOCALB(fromDicPos, 713, +=, LOCAL_VAR(got)) ;
  }
  return SZ_OK;
ENDFUNC

/* Reads .xz or .lzma data from stdin, writes uncompressed bytes to stdout,
 * uses GLOBAL_VAR(dic). It verifies some aspects of the file format (so it
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
  if (LT_SMALL(Preread(12 + 12 + 6), 12 + 12 + 6)) { return SZ_ERROR_INPUT_EOF; }
  /* readBuf[6] is actually stream flags, should also be 0. */
  if (EQ_SMALL(GET_ARY8(readBuf, 0), 0xfd) && EQ_SMALL(GET_ARY8(readBuf, 1), 0x37) &&
      EQ_SMALL(GET_ARY8(readBuf, 2), 0x7a) && EQ_SMALL(GET_ARY8(readBuf, 3), 0x58) &&
      EQ_SMALL(GET_ARY8(readBuf, 4), 0x5a) && EQ_SMALL(GET_ARY8(readBuf, 5), 0) &&
      EQ_SMALL(GET_ARY8(readBuf, 6), 0)) {  /* .xz: "\xFD""7zXZ\0" */
  } ELSE_IF (LE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)), 225) && EQ_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 13), 0) &&  /* .lzma */
        /* High 4 bytes of uncompressed size. */
        (EQ_SMALL((LOCAL_VAR(bhf) = GetLE4(GLOBAL_VAR(readCur) + 9)), 0) || EQ(LOCAL_VAR(bhf), 0xffffffff)) &&
        GE_SMALL((SET_GLOBAL(dicSize, 130, =) GetLE4(GLOBAL_VAR(readCur) + 1)), LZMA_DIC_MIN) &&
        LE(GLOBAL_VAR(dicSize), DIC_ARRAY_SIZE)) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    LOCAL(UInt32, readBufUS);
    LOCAL(UInt32, srcLen);
    LOCAL(UInt32, fromDicPos);
    InitDecode();
    /* LZMA restricts LE(lc + lp, 4). LZMA requires LE(lc + lp, 12).
     * We apply the LZMA2 restriction here (to save memory in
     * GLOBAL_VAR(probs)), thus we are not able to extract some legitimate
     * .lzma files.
     */
    if (NE_SMALL((LOCAL_VAR(res) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur)))), SZ_OK)) {
      return LOCAL_VAR(res);
    }
    if (EQ_SMALL(LOCAL_VAR(bhf), 0)) {
      SET_GLOBAL(dicBufSize, 132, =) LOCAL_VAR(readBufUS) = GetLE4(GLOBAL_VAR(readCur) + 5);
      if (GT(LOCAL_VAR(readBufUS), DIC_ARRAY_SIZE)) { return SZ_ERROR_MEM; }
    } else {
      SET_LOCALB(readBufUS, 715, =, LOCAL_VAR(bhf)) ;  /* max UInt32. */
      SET_GLOBAL(dicBufSize, 134, =) DIC_ARRAY_SIZE;
    }
    SET_GLOBAL(readCur, 136, +=) 13;  /* Start decompressing the 0 byte. */
    DEBUGF("LZMA dicSize=0x%x us=%d bhf=%d\n", ENSURE_32BIT(GLOBAL_VAR(dicSize)), ENSURE_32BIT(LOCAL_VAR(readBufUS)), ENSURE_32BIT(LOCAL_VAR(bhf)));
    /* TODO(pts): Limit on uncompressed size unless 8 bytes of -1 is
     * specified.
     */
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while (GT_SMALL((LOCAL_VAR(srcLen) = Preread(READBUF_SIZE)), 0)) {
      LOCAL(SRes, res);
      SET_LOCALB(fromDicPos, 717, =, GLOBAL_VAR(dicPos)) ;
      SET_LOCALB(res, 719, =, LzmaDec_DecodeToDic(LOCAL_VAR(srcLen))) ;
      DEBUGF("LZMADEC res=%d\n", ENSURE_32BIT(LOCAL_VAR(res)));
      if (GT_SMALL(GLOBAL_VAR(dicPos), LOCAL_VAR(readBufUS))) { SET_GLOBAL(dicPos, 138, =) LOCAL_VAR(readBufUS); }
      if (NE_SMALL((LOCAL_VAR(res) = WriteFrom(LOCAL_VAR(fromDicPos))), SZ_OK)) { return LOCAL_VAR(res); }
      if (EQ_SMALL(LOCAL_VAR(res), SZ_ERROR_FINISHED_WITH_MARK)) { BREAK; }
      if (NE_SMALL(LOCAL_VAR(res), SZ_ERROR_NEEDS_MORE_INPUT) && NE_SMALL(LOCAL_VAR(res), SZ_OK)) { return LOCAL_VAR(res); }
      if (EQ_SMALL(GLOBAL_VAR(dicPos), LOCAL_VAR(readBufUS))) { BREAK; }
    }
    return SZ_OK;
  } else {
    return SZ_ERROR_BAD_MAGIC;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  LOCAL_VAR(checksumSize) = GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 7);
  if (EQ_SMALL(LOCAL_VAR(checksumSize), 0)) { /* None */ LOCAL_VAR(checksumSize) = 1; }
  ELSE_IF (EQ_SMALL(LOCAL_VAR(checksumSize), 1)) { /* CRC32 */ LOCAL_VAR(checksumSize) = 4; }
  ELSE_IF (EQ_SMALL(LOCAL_VAR(checksumSize), 4)) { /* CRC64, typical xz output. */ LOCAL_VAR(checksumSize) = 8; }
  else { return SZ_ERROR_BAD_CHECKSUM_TYPE; }
  /* Also ignore the CRC32 after LOCAL_VAR(checksumSize). */
  SET_GLOBAL(readCur, 140, +=) 12;
  for (;;) {  /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    LOCAL_INIT(Byte, blockSizePad, 3);
    LOCAL(UInt32, bhs);
    LOCAL(UInt32, bhs2);  /* Block header size */
    LOCAL(Byte, dicSizeProp);
    LOCAL(UInt32, readAtBlock);
    ASSERT(GE_SMALL(GLOBAL_VAR(readEnd) - GLOBAL_VAR(readCur), 12));  /* At least 12 bytes preread. */
    LOCAL_VAR(readAtBlock) = GLOBAL_VAR(readCur);
    /* Last block, index follows. */
    if (EQ_SMALL((LOCAL_VAR(bhs) = GET_ARY8(readBuf, GLOBAL_VAR(readCur)++)), 0)) { BREAK; }
    /* Block header size includes the LOCAL_VAR(bhs) field above and the CRC32 below. */
    LOCAL_VAR(bhs) = (LOCAL_VAR(bhs) + 1) << 2;
    DEBUGF("bhs=%d\n", ENSURE_32BIT(LOCAL_VAR(bhs)));
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (LT_SMALL(Preread(LOCAL_VAR(bhs)), LOCAL_VAR(bhs))) { return SZ_ERROR_INPUT_EOF; }
    SET_LOCALB(readAtBlock, 721, =, GLOBAL_VAR(readCur)) ;
    SET_LOCALB(bhf, 723, =, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++)) ;
    if (NE_SMALL(LOCAL_VAR(bhf) & 2, 0)) { return SZ_ERROR_UNSUPPORTED_FILTER_COUNT; }
    DEBUGF("filter count=%d\n", ENSURE_32BIT((LOCAL_VAR(bhf) & 2) + 1));
    if (NE_SMALL(LOCAL_VAR(bhf) & 20, 0)) { return SZ_ERROR_BAD_BLOCK_FLAGS; }
    if (NE_SMALL(LOCAL_VAR(bhf) & 64, 0)) {  /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (NE_SMALL(LOCAL_VAR(bhf) & 128, 0)) {  /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (NE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), FILTER_ID_LZMA2)) { return SZ_ERROR_UNSUPPORTED_FILTER_ID; }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (NE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), 1)) { return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE; }
    SET_LOCALB(dicSizeProp, 725, =, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++)) ;
    /* Typical large dictionary sizes:
     *
     *  * 35: 805306368 bytes == 768 MiB
     *  * 36: 1073741824 bytes == 1 GiB
     *  * 37: 1610612736 bytes, largest supported by .xz
     *  * 38: 2147483648 bytes == 2 GiB
     *  * 39: 3221225472 bytes == 3 GiB
     *  * 40: 4294967295 bytes, largest supported by .xz
     */
    DEBUGF("dicSizeProp=0x%02x\n", ENSURE_32BIT(LOCAL_VAR(dicSizeProp)));
    if (GT_SMALL(LOCAL_VAR(dicSizeProp), 40)) { return SZ_ERROR_BAD_DICTIONARY_SIZE; }
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (GT_SMALL(LOCAL_VAR(dicSizeProp), 37)) { return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE; }
    SET_GLOBAL(dicSize, 142, =) ((ENSURE_32BIT(2) | ((LOCAL_VAR(dicSizeProp)) & 1)) << ((LOCAL_VAR(dicSizeProp)) / 2 + 11));
    ASSERT(GE_SMALL(GLOBAL_VAR(dicSize), LZMA_DIC_MIN));
    DEBUGF("dicSize39=%u\n", ((ENSURE_32BIT(2) | ((39) & 1)) << ((39) / 2 + 11)));
    DEBUGF("dicSize38=%u\n", ((ENSURE_32BIT(2) | ((38) & 1)) << ((38) / 2 + 11)));
    DEBUGF("dicSize37=%u\n", ((ENSURE_32BIT(2) | ((37) & 1)) << ((37) / 2 + 11)));
    DEBUGF("dicSize36=%u\n", ((ENSURE_32BIT(2) | ((36) & 1)) << ((36) / 2 + 11)));
    DEBUGF("dicSize35=%u\n", ((ENSURE_32BIT(2) | ((35) & 1)) << ((35) / 2 + 11)));
    SET_LOCALB(bhs2, 727, =, GLOBAL_VAR(readCur) - LOCAL_VAR(readAtBlock) + 5) ;  /* Won't overflow. */
    DEBUGF("bhs=%d bhs2=%d\n", ENSURE_32BIT(LOCAL_VAR(bhs)), ENSURE_32BIT(LOCAL_VAR(bhs2)));
    if (GT_SMALL(LOCAL_VAR(bhs2), LOCAL_VAR(bhs))) { return SZ_ERROR_BLOCK_HEADER_TOO_LONG; }
    if (NE_SMALL((LOCAL_VAR(res) = IgnoreZeroBytes(LOCAL_VAR(bhs) - LOCAL_VAR(bhs2))), SZ_OK)) { return LOCAL_VAR(res); }
    SET_GLOBAL(readCur, 144, +=) 4;  /* Ignore CRC32. */
    /* Typically it's LOCAL_VAR(offset) 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2\n");
    {  /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      LOCAL(UInt32, chunkUS);  /* Uncompressed chunk sizes. */
      LOCAL(UInt32, chunkCS);  /* Compressed chunk size. */
      InitDecode();

      for (;;) {
        LOCAL(Byte, control);
        ASSERT(EQ_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize)));
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (LT_SMALL(Preread(6), 6)) { return SZ_ERROR_INPUT_EOF; }
        SET_LOCALB(control, 729, =, GET_ARY8(readBuf, GLOBAL_VAR(readCur))) ;
        DEBUGF("CONTROL control=0x%02x at=? inbuf=%d\n", ENSURE_32BIT(LOCAL_VAR(control)), ENSURE_32BIT(GLOBAL_VAR(readCur)));
        if (EQ_SMALL(LOCAL_VAR(control), 0)) {
          DEBUGF("LASTFED\n");
          ++GLOBAL_VAR(readCur);
          BREAK;
        } ELSE_IF (LT_SMALL(TRUNCATE_TO_8BIT(LOCAL_VAR(control) - 3), 0x80 - 3)) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        SET_LOCALB(chunkUS, 731, =, (GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 1) << 8) + GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 2) + 1) ;
        if (LT_SMALL(LOCAL_VAR(control), 3)) {  /* Uncompressed chunk. */
          LOCAL_INIT(const Bool, initDic, EQ_SMALL(LOCAL_VAR(control), 1));
          SET_LOCALB(chunkCS, 733, =, LOCAL_VAR(chunkUS)) ;
          SET_GLOBAL(readCur, 146, +=) 3;
          /* TODO(pts): Porting: TRUNCATE_TO_8BIT(LOCAL_VAR(blockSizePad)) for Python and other unlimited-integer-range languages. */
          LOCAL_VAR(blockSizePad) -= 3;
          if (LOCAL_VAR(initDic)) {
            SET_GLOBAL(needInitProp, 148, =) SET_GLOBAL(needInitState, 150, =) TRUE;
            SET_GLOBAL(needInitDic, 152, =) FALSE;
          } ELSE_IF (GLOBAL_VAR(needInitDic)) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(LOCAL_VAR(initDic), FALSE);
        } else {  /* LZMA chunk. */
          LOCAL_INIT(const Byte, mode, (SHR_SMALL((LOCAL_VAR(control)), 5) & 3));
          LOCAL_INIT(const Bool, initDic, EQ_SMALL(LOCAL_VAR(mode), 3));
          LOCAL_INIT(const Bool, initState, NE_SMALL(LOCAL_VAR(mode), 0));
          LOCAL_INIT(const Bool, isProp, NE_SMALL((LOCAL_VAR(control) & 64), 0));
          SET_LOCALB(chunkUS, 735, +=, (LOCAL_VAR(control) & 31) << 16) ;
          SET_LOCALB(chunkCS, 737, =, (GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 3) << 8) + GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 4) + 1) ;
          if (LOCAL_VAR(isProp)) {
            if (NE_SMALL((LOCAL_VAR(res) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 5))), SZ_OK)) {
              return LOCAL_VAR(res);
            }
            ++GLOBAL_VAR(readCur);
            --LOCAL_VAR(blockSizePad);
          } else {
            if (GLOBAL_VAR(needInitProp)) { return SZ_ERROR_MISSING_INITPROP; }
          }
          SET_GLOBAL(readCur, 154, +=) 5;
          SET_LOCALB(blockSizePad, 739, -=, 5) ;
          if ((!LOCAL_VAR(initDic) && GLOBAL_VAR(needInitDic)) || (!LOCAL_VAR(initState) && GLOBAL_VAR(needInitState))) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(LOCAL_VAR(initDic), LOCAL_VAR(initState));
          SET_GLOBAL(needInitDic, 156, =) FALSE;
          SET_GLOBAL(needInitState, 158, =) FALSE;
        }
        ASSERT(EQ_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize)));
        SET_GLOBAL(dicBufSize, 160, +=) LOCAL_VAR(chunkUS);
        /* Decompressed data too long, won't fit to GLOBAL_VAR(dic). */
        if (GT_SMALL(GLOBAL_VAR(dicBufSize), DIC_ARRAY_SIZE)) { return SZ_ERROR_MEM; }
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (LT_SMALL(Preread(LOCAL_VAR(chunkCS) + 6), LOCAL_VAR(chunkCS))) { return SZ_ERROR_INPUT_EOF; }
        DEBUGF("FEED us=%d cs=%d dicPos=%d\n", ENSURE_32BIT(LOCAL_VAR(chunkUS)), ENSURE_32BIT(LOCAL_VAR(chunkCS)), ENSURE_32BIT(GLOBAL_VAR(dicPos)));
        if (LT_SMALL(LOCAL_VAR(control), 0x80)) {  /* Uncompressed chunk. */
          DEBUGF("DECODE uncompressed\n");
          while (NE_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize))) {
            SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
          }
          if (EQ_SMALL(GLOBAL_VAR(checkDicSize), 0) && LE_SMALL(GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos), LOCAL_VAR(chunkUS))) {
            SET_GLOBAL(checkDicSize, 162, =) GLOBAL_VAR(dicSize);
          }
          SET_GLOBAL(processedPos, 164, +=) LOCAL_VAR(chunkUS);
        } else {  /* Compressed chunk. */
          DEBUGF("DECODE call\n");
          /* This call doesn't change GLOBAL_VAR(dicBufSize). */
          if (NE_SMALL((LOCAL_VAR(res) = LzmaDec_DecodeToDic(LOCAL_VAR(chunkCS))), SZ_OK)) { return LOCAL_VAR(res); }
        }
        if (NE_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize))) { return SZ_ERROR_BAD_DICPOS; }
        if (NE_SMALL((LOCAL_VAR(res) = WriteFrom(GLOBAL_VAR(dicPos) - LOCAL_VAR(chunkUS))), SZ_OK)) { return LOCAL_VAR(res); }
        SET_LOCALB(blockSizePad, 741, -=, LOCAL_VAR(chunkCS)) ;
        /* We can't discard decompressbuf[:GLOBAL_VAR(dicBufSize)] now,
         * because we need it a dictionary in which subsequent calls to
         * Lzma2Dec_DecodeToDic will look up backreferences.
         */
      }
    }  /* End of LZMA2 stream. */
    /* End of block. */
    /* 7 for padding4 and CRC32 + 12 for the next block header + 6 for the next
     * chunk header.
     */
    if (LT_SMALL(Preread(7 + 12 + 6), 7 + 12 + 6)) { return SZ_ERROR_INPUT_EOF; }
    DEBUGF("ALTELL blockSizePad=%d\n", ENSURE_32BIT(LOCAL_VAR(blockSizePad) & 3));
    /* Ignore block padding. */
    if (NE_SMALL((LOCAL_VAR(res) = IgnoreZeroBytes(LOCAL_VAR(blockSizePad) & 3)), SZ_OK)) { return LOCAL_VAR(res); }
    SET_GLOBAL(readCur, 166, +=) LOCAL_VAR(checksumSize);  /* Ignore CRC32, CRC64 etc. */
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

#ifdef CONFIG_LANG_PERL
FUNC_ARG0(SRes, Decompress)
  LOCAL(SRes, res);
  CLEAR_ARY16(probs);
  CLEAR_ARY8(readBuf);
  CLEAR_ARY8(dic);
  binmode(STDIN);
  binmode(STDOUT);
  SET_LOCALB(res, 743, =, DecompressXzOrLzma()) ;
  CLEAR_ARY16(probs);
  CLEAR_ARY8(readBuf);
  CLEAR_ARY8(dic);
  return LOCAL_VAR(res);
ENDFUNC

exit(Decompress());
#endif  /* CONFIG_LANG_PERL */
