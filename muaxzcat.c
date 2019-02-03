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

#ifdef CONFIG_LANG_C
/* In Perl, CONTINUE doesn't work (sometimes it silently goes to upper
 * loops) in a do { ... } while (...) loop. So don't have such loops.
 */
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
#define ENSURE_32BIT(x) x
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
#define GET_ARY16(a, idx) (+global.a##16[idx])
/* TRUNCATE_TO_16BIT is must be called on value manually if needed. */
#define SET_ARY16(a, idx, value) (global.a##16[idx] = value)
#define CLEAR_ARY8(a)
#define GET_ARY8(a, idx) (+global.a##8[idx])
/* TRUNCATE_TO_8BIT must be called on value manually if needed. */
#define SET_ARY8(a, idx, value) (global.a##8[idx] = value)
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

/* !! Mask the inputs and outputs of GET_ARY8, GET_ARY16, SET_ARY8, SET_ARY16? */
/* !! Mask the inputs of these operators: >> >>= < > <= >= == != */
#ifdef CONFIG_LANG_C
#define SHR(x, y) ((x) >> (y))
#define SHR_SMALL(x, y) ((x) >> (y))  /* When 0 <= x < (2 ** 31). */
#define SET_SHR(x, y) ((x) >>= (y))
#define LT(x, y) ((x) < (y))
#define LE(x, y) ((x) <= (y))
#define EQ(x, y) ((x) == (y))
#define NE(x, y) ((x) != (y))
/* !! > >= / /= */
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_PERL
#define SHR(x, y) ((x & 0xffffffff) >> (y))
#define SHR_SMALL(x, y) ((x) >> (y))
#define SET_SHR(x, y) ((x) = ((x) & 0xffffffff) >> (y))
#define LT(x, y) ((x) & 0xffffffff < (y) & 0xffffffff)
#define LE(x, y) ((x) & 0xffffffff <= (y) & 0xffffffff)
#define EQ(x, y) ((((x) - (y)) & 0xffffffff) == 0)
#define NE(x, y) ((((x) - (y)) & 0xffffffff) != 0)
/* !! > >= */
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
static void DumpVars(void);
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ({ LOCAL_##name op value; DEBUGF("SET_LOCAL @%d %s=%u\n", setid, #name, (int)TRUNCATE_TO_32BIT(LOCAL_##name)); LOCAL_##name; })
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) *({ DumpVars(); DEBUGF("SET_GLOBAL %s @%d\n", #name, setid); &global.name; }) op
#undef  SET_ARY16
#define SET_ARY16(a, idx, value) ({ const UInt32 idx2 = idx; global.a##16[idx2] = value; DEBUGF("SET_ARY16 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##16[idx2])); global.a##16[idx2]; })
#undef  SET_ARY8
#define SET_ARY8(a, idx, value) ({ const UInt32 idx2 = idx; global.a##8[idx2] = value; DEBUGF("SET_ARY8 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##8[idx2])); global.a##8[idx2];  })
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
sub DumpVars();
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ${ $##name op value; DEBUGF("SET_LOCAL @%d %s=%u\n", setid, #name, TRUNCATE_TO_32BIT($##name)); \$##name; }
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) ${DumpVars(), DEBUGF("SET_GLOBAL %s @%d\n", #name, setid), \$##name} op
#undef  SET_ARY16
#define SET_ARY16(a, idx, value) ${ my $setidx = TRUNCATE_TO_32BIT(idx); vec($##a, $setidx, 16) = value; DEBUGF("SET_ARY16 %s[%d]=%u\n", #a, $setidx, vec($##a, $setidx, 16)); \vec($##a, $setidx, 16) }
#undef  SET_ARY8
#define SET_ARY8(a, idx, value) ${ my $setidx = TRUNCATE_TO_32BIT(idx); vec($##a, $setidx, 8) = value; DEBUGF("SET_ARY8 %s[%d]=%u\n", #a, $setidx, vec($##a, $setidx, 8)); \vec($##a, $setidx, 8) }
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
#endif  /* CONFIG_LANG_C */

/* For LZMA streams, lc + lp <= 8 + 4 <= 12.
 * For LZMA2 streams, lc + lp <= 4.
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
ENDGLOBALS

#ifdef CONFIG_DEBUG
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
  if (GLOBAL_VAR(remainLen) != 0 && GLOBAL_VAR(remainLen) < kMatchSpecLenStart) {
    LOCAL_INIT(UInt32, localLen, GLOBAL_VAR(remainLen));
    if (LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos) < LOCAL_VAR(localLen)) {
      LOCAL_VAR(localLen) = LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos);
    }
    if (GLOBAL_VAR(checkDicSize) == 0 && GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos) <= LOCAL_VAR(localLen)) {
      GLOBAL_VAR(checkDicSize) = GLOBAL_VAR(dicSize);
    }
    GLOBAL_VAR(processedPos) += LOCAL_VAR(localLen);
    GLOBAL_VAR(remainLen) -= LOCAL_VAR(localLen);
    while (LOCAL_VAR(localLen) != 0) {
      LOCAL_VAR(localLen)--;
      SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + ((GLOBAL_VAR(dicPos) < GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
      GLOBAL_VAR(dicPos)++;
    }
  }
ENDFUNC

/* Modifies GLOBAL_VAR(bufCur) etc. */
FUNC_ARG2(SRes, LzmaDec_DecodeReal2, const UInt32, dicLimit, const UInt32, bufLimit)
  LOCAL_INIT(const UInt32, pbMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(pb))) - 1);
  LOCAL_INIT(const UInt32, lpMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(lp))) - 1);
  goto do1; while (GLOBAL_VAR(dicPos) < LOCAL_VAR(dicLimit) && GLOBAL_VAR(bufCur) < LOCAL_VAR(bufLimit) && GLOBAL_VAR(remainLen) < kMatchSpecLenStart) { do1: ;
    LOCAL_INIT(const UInt32, dicLimit2, GLOBAL_VAR(checkDicSize) == 0 && GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos) < LOCAL_VAR(dicLimit) - GLOBAL_VAR(dicPos) ? GLOBAL_VAR(dicPos) + (GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos)) : LOCAL_VAR(dicLimit));
    LOCAL_INIT(UInt32, localLen, 0);
    LOCAL_INIT(UInt32, rangeLocal, GLOBAL_VAR(range));
    LOCAL_INIT(UInt32, codeLocal, GLOBAL_VAR(code));
    goto do2; while (GLOBAL_VAR(dicPos) < LOCAL_VAR(dicLimit2) && GLOBAL_VAR(bufCur) < LOCAL_VAR(bufLimit)) { do2: ;
      LOCAL(UInt32, probIdx);
      LOCAL(UInt32, bound);
      LOCAL(UInt32, ttt);
      LOCAL_INIT(UInt32, posState, GLOBAL_VAR(processedPos) & LOCAL_VAR(pbMask));

      LOCAL_VAR(probIdx) = IsMatch + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(posState);
      LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
      if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
        LOCAL(UInt32, symbol);
        LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + (SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits))));;
        LOCAL_VAR(probIdx) = Literal;
        if (GLOBAL_VAR(checkDicSize) != 0 || GLOBAL_VAR(processedPos) != 0) {
          LOCAL_VAR(probIdx) += (LZMA_LIT_SIZE * (((GLOBAL_VAR(processedPos) & LOCAL_VAR(lpMask)) << GLOBAL_VAR(lc)) + SHR(GET_ARY8(dic, (GLOBAL_VAR(dicPos) == 0 ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), (8 - GLOBAL_VAR(lc)))));
        }
        if (GLOBAL_VAR(state) < kNumLitStates) {
          GLOBAL_VAR(state) -= (GLOBAL_VAR(state) < 4) ? GLOBAL_VAR(state) : 3;
          LOCAL_VAR(symbol) = 1;
          goto do3; while (LOCAL_VAR(symbol) < 0x100) { do3: ;
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = (SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits)) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1; ;; }
          }
        } else {
          LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + ((GLOBAL_VAR(dicPos) < GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
          LOCAL_INIT(UInt32, offs, 0x100);
          GLOBAL_VAR(state) -= (GLOBAL_VAR(state) < 10) ? 3 : 6;
          LOCAL_VAR(symbol) = 1;
          goto do4; while (LOCAL_VAR(symbol) < 0x100) { do4: ;
            LOCAL(UInt32, localBit);
            LOCAL(UInt32, probLitIdx);
            LOCAL_VAR(matchByte) <<= 1;
            LOCAL_VAR(localBit) = (LOCAL_VAR(matchByte) & LOCAL_VAR(offs));
            LOCAL_VAR(probLitIdx) = LOCAL_VAR(probIdx) + LOCAL_VAR(offs) + LOCAL_VAR(localBit) + LOCAL_VAR(symbol);
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLitIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLitIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)); LOCAL_VAR(offs) &= ~LOCAL_VAR(localBit); } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLitIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1; LOCAL_VAR(offs) &= LOCAL_VAR(localBit); }
          }
        }
        SET_ARY8(dic, GLOBAL_VAR(dicPos)++, TRUNCATE_TO_8BIT(LOCAL_VAR(symbol)));
        GLOBAL_VAR(processedPos)++;
        CONTINUE;
      } else {
        LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
        LOCAL_VAR(probIdx) = IsRep + GLOBAL_VAR(state);
        LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
        if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
          LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
          GLOBAL_VAR(state) += kNumStates;
          LOCAL_VAR(probIdx) = LenCoder;
        } else {
          LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
          if (GLOBAL_VAR(checkDicSize) == 0 && GLOBAL_VAR(processedPos) == 0) {
            return SZ_ERROR_DATA;
          }
          LOCAL_VAR(probIdx) = IsRepG0 + GLOBAL_VAR(state);
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
          if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
            LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
            LOCAL_VAR(probIdx) = IsRep0Long + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(posState);
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
            if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
              LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
              SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + ((GLOBAL_VAR(dicPos) < GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
              GLOBAL_VAR(dicPos)++;
              GLOBAL_VAR(processedPos)++;
              GLOBAL_VAR(state) = GLOBAL_VAR(state) < kNumLitStates ? 9 : 11;
              CONTINUE;
            }
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
          } else {
            LOCAL(UInt32, distance);
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
            LOCAL_VAR(probIdx) = IsRepG1 + GLOBAL_VAR(state);
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
            if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
              LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
              LOCAL_VAR(distance) = GLOBAL_VAR(rep1);
            } else {
              LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
              LOCAL_VAR(probIdx) = IsRepG2 + GLOBAL_VAR(state);
              LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
              if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
                LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
                LOCAL_VAR(distance) = GLOBAL_VAR(rep2);
              } else {
                LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
                LOCAL_VAR(distance) = GLOBAL_VAR(rep3);
                GLOBAL_VAR(rep3) = GLOBAL_VAR(rep2);
              }
              GLOBAL_VAR(rep2) = GLOBAL_VAR(rep1);
            }
            GLOBAL_VAR(rep1) = GLOBAL_VAR(rep0);
            GLOBAL_VAR(rep0) = LOCAL_VAR(distance);
          }
          GLOBAL_VAR(state) = GLOBAL_VAR(state) < kNumLitStates ? 8 : 11;
          LOCAL_VAR(probIdx) = RepLenCoder;
        }
        {
          LOCAL(UInt32, limitSub);
          LOCAL(UInt32, offset);
          LOCAL_INIT(UInt32, probLenIdx, LOCAL_VAR(probIdx) + LenChoice);
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLenIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
          if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
            LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
            LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenLow + (LOCAL_VAR(posState) << (kLenNumLowBits));
            LOCAL_VAR(offset) = 0;
            LOCAL_VAR(limitSub) = (ENSURE_32BIT(1) << (kLenNumLowBits));
          } else {
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
            LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenChoice2;
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLenIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
            if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
              LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));;
              LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenMid + (LOCAL_VAR(posState) << (kLenNumMidBits));
              LOCAL_VAR(offset) = kLenNumLowSymbols;
              LOCAL_VAR(limitSub) = ENSURE_32BIT(1) << (kLenNumMidBits);
            } else {
              LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probLenIdx), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));;
              LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenHigh;
              LOCAL_VAR(offset) = kLenNumLowSymbols + kLenNumMidSymbols;
              LOCAL_VAR(limitSub) = ENSURE_32BIT(1) << (kLenNumHighBits);
            }
          }
          {
            LOCAL_VAR(localLen) = 1;
            goto do5; while (LOCAL_VAR(localLen) < LOCAL_VAR(limitSub)) { do5: ;
              { LOCAL_VAR(ttt) = GET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen))); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localLen) = (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, (LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localLen) = (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)) + 1; ;; }; };
            }
            LOCAL_VAR(localLen) -= LOCAL_VAR(limitSub);
          };
          LOCAL_VAR(localLen) += LOCAL_VAR(offset);
        }

        if (GLOBAL_VAR(state) >= kNumStates) {
          LOCAL(UInt32, distance);
          LOCAL_VAR(probIdx) = PosSlotCode + (ENSURE_32BIT(LOCAL_VAR(localLen) < kNumLenToPosStates ? LOCAL_VAR(localLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits));
          {
            LOCAL_VAR(distance) = 1;
            goto do6; while (LOCAL_VAR(distance) < (1 << 6)) { do6: ;
              { LOCAL_VAR(ttt) = GET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance))); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(distance) = (LOCAL_VAR(distance) + LOCAL_VAR(distance)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, (LOCAL_VAR(probIdx) + LOCAL_VAR(distance)), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(distance) = (LOCAL_VAR(distance) + LOCAL_VAR(distance)) + 1; ;; };};
            }
            LOCAL_VAR(distance) -= (1 << 6);
          };
          if (LOCAL_VAR(distance) >= kStartPosModelIndex) {
            LOCAL_INIT(const UInt32, posSlot, LOCAL_VAR(distance));
            LOCAL_INIT(UInt32, numDirectBits, SHR(LOCAL_VAR(distance), 1) - 1);
            LOCAL_VAR(distance) = (2 | (LOCAL_VAR(distance) & 1));
            if (LOCAL_VAR(posSlot) < kEndPosModelIndex) {
              LOCAL_VAR(distance) <<= LOCAL_VAR(numDirectBits);
              LOCAL_VAR(probIdx) = SpecPos + LOCAL_VAR(distance) - LOCAL_VAR(posSlot) - 1;
              {
                LOCAL_INIT(UInt32, mask, 1);
                LOCAL_INIT(UInt32, localI, 1);
                goto do7; while (--LOCAL_VAR(numDirectBits) != 0) { do7: ;
                  LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; LOCAL_VAR(distance) |= LOCAL_VAR(mask); };
                  LOCAL_VAR(mask) <<= 1;
                }
              }
            } else {
              LOCAL_VAR(numDirectBits) -= kNumAlignBits;
              goto do8; while (--LOCAL_VAR(numDirectBits) != 0) { do8: ;
                LOCAL(UInt32, localT);
                if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }
                SET_SHR(LOCAL_VAR(rangeLocal), 1);
                LOCAL_VAR(codeLocal) -= LOCAL_VAR(rangeLocal);
                LOCAL_VAR(localT) = (0 - (ENSURE_32BIT(SHR(LOCAL_VAR(codeLocal), 31))));
                LOCAL_VAR(distance) = (LOCAL_VAR(distance) << 1) + (LOCAL_VAR(localT) + 1);
                LOCAL_VAR(codeLocal) += LOCAL_VAR(rangeLocal) & LOCAL_VAR(localT);
              }
              LOCAL_VAR(probIdx) = Align;
              LOCAL_VAR(distance) <<= kNumAlignBits;
              {
                LOCAL_INIT(UInt32, localI, 1);
                LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; LOCAL_VAR(distance) |= 1; };
                LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; LOCAL_VAR(distance) |= 2; };
                LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; LOCAL_VAR(distance) |= 4; };
                LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) + SHR((kBitModelTotal - LOCAL_VAR(ttt)), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); SET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI), TRUNCATE_TO_16BIT(LOCAL_VAR(ttt) - SHR(LOCAL_VAR(ttt), kNumMoveBits)));; LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; LOCAL_VAR(distance) |= 8; };
              }
              if (LOCAL_VAR(distance) == ENSURE_32BIT(0xffffffff)) {
                LOCAL_VAR(localLen) += kMatchSpecLenStart;
                GLOBAL_VAR(state) -= kNumStates;
                BREAK;
              }
            }
          }
          GLOBAL_VAR(rep3) = GLOBAL_VAR(rep2);
          GLOBAL_VAR(rep2) = GLOBAL_VAR(rep1);
          GLOBAL_VAR(rep1) = GLOBAL_VAR(rep0);
          GLOBAL_VAR(rep0) = LOCAL_VAR(distance) + 1;
          if (GLOBAL_VAR(checkDicSize) == 0) {
            if (LOCAL_VAR(distance) >= GLOBAL_VAR(processedPos)) {
              return SZ_ERROR_DATA;
            }
          } else {
            if (LOCAL_VAR(distance) >= GLOBAL_VAR(checkDicSize)) {
              return SZ_ERROR_DATA;
            }
          }
          GLOBAL_VAR(state) = (GLOBAL_VAR(state) < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        }

        LOCAL_VAR(localLen) += kMatchMinLen;

        if (LOCAL_VAR(dicLimit2) == GLOBAL_VAR(dicPos)) {
          return SZ_ERROR_DATA;
        }
        {
          LOCAL_INIT(UInt32, rem, LOCAL_VAR(dicLimit2) - GLOBAL_VAR(dicPos));
          LOCAL_INIT(UInt32, curLen, ((LOCAL_VAR(rem) < LOCAL_VAR(localLen)) ? LOCAL_VAR(rem) : LOCAL_VAR(localLen)));
          LOCAL_INIT(UInt32, pos, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + ((GLOBAL_VAR(dicPos) < GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0));

          GLOBAL_VAR(processedPos) += LOCAL_VAR(curLen);

          LOCAL_VAR(localLen) -= LOCAL_VAR(curLen);
          if (LOCAL_VAR(pos) + LOCAL_VAR(curLen) <= GLOBAL_VAR(dicBufSize)) {
            ASSERT(GLOBAL_VAR(dicPos) > LOCAL_VAR(pos));
            ASSERT(LOCAL_VAR(curLen) > 0);
            goto do9; while (--LOCAL_VAR(curLen) != 0) { do9: ;
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
            }
          } else {
            goto do10; while (--LOCAL_VAR(curLen) != 0) { do10: ;
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
              if (LOCAL_VAR(pos) == GLOBAL_VAR(dicBufSize)) { LOCAL_VAR(pos) = 0; }
            }
          }
        }
      }
    }
    if (LOCAL_VAR(rangeLocal) < kTopValue) { LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++)); };
    GLOBAL_VAR(range) = LOCAL_VAR(rangeLocal);
    GLOBAL_VAR(code) = LOCAL_VAR(codeLocal);
    GLOBAL_VAR(remainLen) = LOCAL_VAR(localLen);
    if (GLOBAL_VAR(processedPos) >= GLOBAL_VAR(dicSize)) {
      GLOBAL_VAR(checkDicSize) = GLOBAL_VAR(dicSize);
    }
    LzmaDec_WriteRem(LOCAL_VAR(dicLimit));
  }

  if (GLOBAL_VAR(remainLen) > kMatchSpecLenStart) {
    GLOBAL_VAR(remainLen) = kMatchSpecLenStart;
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

    LOCAL_VAR(probIdx) = IsMatch + (LOCAL_VAR(stateLocal) << (kNumPosBitsMax)) + LOCAL_VAR(posState);
    LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
    if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
      LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
      LOCAL_VAR(probIdx) = Literal;
      if (GLOBAL_VAR(checkDicSize) != 0 || GLOBAL_VAR(processedPos) != 0) {
        LOCAL_VAR(probIdx) += (LZMA_LIT_SIZE * ((((GLOBAL_VAR(processedPos)) & ((1 << (GLOBAL_VAR(lp))) - 1)) << GLOBAL_VAR(lc)) + SHR(GET_ARY8(dic, (GLOBAL_VAR(dicPos) == 0 ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), (8 - GLOBAL_VAR(lc)))));
      }

      if (LOCAL_VAR(stateLocal) < kNumLitStates) {
        LOCAL_INIT(UInt32, symbol, 1);
        goto do11; while (LOCAL_VAR(symbol) < 0x100) { do11: ;
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(symbol)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1; ;; }
        }
      } else {
        LOCAL_INIT(UInt32, matchByte, GET_ARY8(dic, GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0) + ((GLOBAL_VAR(dicPos) < GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
        LOCAL_INIT(UInt32, offs, 0x100);
        LOCAL_INIT(UInt32, symbol, 1);
        goto do12; while (LOCAL_VAR(symbol) < 0x100) { do12: ;
          LOCAL(UInt32, localBit);
          LOCAL(UInt32, probLitIdx);
          LOCAL_VAR(matchByte) <<= 1;
          LOCAL_VAR(localBit) = (LOCAL_VAR(matchByte) & LOCAL_VAR(offs));
          LOCAL_VAR(probLitIdx) = LOCAL_VAR(probIdx) + LOCAL_VAR(offs) + LOCAL_VAR(localBit) + LOCAL_VAR(symbol);
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLitIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)); LOCAL_VAR(offs) &= ~LOCAL_VAR(localBit); } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(symbol) = (LOCAL_VAR(symbol) + LOCAL_VAR(symbol)) + 1; LOCAL_VAR(offs) &= LOCAL_VAR(localBit); }
        }
      }
      LOCAL_VAR(res) = DUMMY_LIT;
    } else {
      LOCAL(UInt32, localLen);
      LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
      LOCAL_VAR(probIdx) = IsRep + LOCAL_VAR(stateLocal);
      LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
       if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
        LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
        LOCAL_VAR(stateLocal) = 0;
        LOCAL_VAR(probIdx) = LenCoder;
        LOCAL_VAR(res) = DUMMY_MATCH;
      } else {
        LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
        LOCAL_VAR(res) = DUMMY_REP;
        LOCAL_VAR(probIdx) = IsRepG0 + LOCAL_VAR(stateLocal);
        LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
        if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
          LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
          LOCAL_VAR(probIdx) = IsRep0Long + (LOCAL_VAR(stateLocal) << (kNumPosBitsMax)) + LOCAL_VAR(posState);
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
          if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
            LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
            if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); };
            return DUMMY_REP;
          } else {
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
          }
        } else {
          LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
          LOCAL_VAR(probIdx) = IsRepG1 + LOCAL_VAR(stateLocal);
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
          if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
            LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
          } else {
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
            LOCAL_VAR(probIdx) = IsRepG2 + LOCAL_VAR(stateLocal);
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
            if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
              LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
            } else {
              LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
            }
          }
        }
        LOCAL_VAR(stateLocal) = kNumStates;
        LOCAL_VAR(probIdx) = RepLenCoder;
      }
      {
        LOCAL(UInt32, limitSub);
        LOCAL(UInt32, offset);
        LOCAL_INIT(UInt32, probLenIdx, LOCAL_VAR(probIdx) + LenChoice);
        LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLenIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
        if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
          LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
          LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenLow + (LOCAL_VAR(posState) << (kLenNumLowBits));
          LOCAL_VAR(offset) = 0;
          LOCAL_VAR(limitSub) = ENSURE_32BIT(1) << (kLenNumLowBits);
        } else {
          LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
          LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenChoice2;
          LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLenIdx)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt);
          if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) {
            LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound);
            LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenMid + (LOCAL_VAR(posState) << (kLenNumMidBits));
            LOCAL_VAR(offset) = kLenNumLowSymbols;
            LOCAL_VAR(limitSub) = ENSURE_32BIT(1) << (kLenNumMidBits);
          } else {
            LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound);
            LOCAL_VAR(probLenIdx) = LOCAL_VAR(probIdx) + LenHigh;
            LOCAL_VAR(offset) = kLenNumLowSymbols + kLenNumMidSymbols;
            LOCAL_VAR(limitSub) = ENSURE_32BIT(1) << (kLenNumHighBits);
          }
        }
        {
          LOCAL_VAR(localLen) = 1;
          goto do13; while (LOCAL_VAR(localLen) < LOCAL_VAR(limitSub)) { do13: ;
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probLenIdx) + LOCAL_VAR(localLen)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); LOCAL_VAR(localLen) = (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(localLen) = (LOCAL_VAR(localLen) + LOCAL_VAR(localLen)) + 1; ;; }
          }
          LOCAL_VAR(localLen) -= LOCAL_VAR(limitSub);
        };
        LOCAL_VAR(localLen) += LOCAL_VAR(offset);
      }

      if (LOCAL_VAR(stateLocal) < 4) {
        LOCAL(UInt32, posSlot);
        LOCAL_VAR(probIdx) = PosSlotCode + (ENSURE_32BIT(LOCAL_VAR(localLen) < kNumLenToPosStates ? LOCAL_VAR(localLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits));
        {
          LOCAL_VAR(posSlot) = 1;
          goto do14; while (LOCAL_VAR(posSlot) < ENSURE_32BIT(1) << (kNumPosSlotBits)) { do14: ;
            LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(posSlot)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); LOCAL_VAR(posSlot) = (LOCAL_VAR(posSlot) + LOCAL_VAR(posSlot)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(posSlot) = (LOCAL_VAR(posSlot) + LOCAL_VAR(posSlot)) + 1; ;; }
          }
          LOCAL_VAR(posSlot) -= ENSURE_32BIT(1) << (kNumPosSlotBits);
        };
        if (LOCAL_VAR(posSlot) >= kStartPosModelIndex) {
          LOCAL_INIT(UInt32, numDirectBits, SHR_SMALL(LOCAL_VAR(posSlot), 1) - 1);
          if (LOCAL_VAR(posSlot) < kEndPosModelIndex) {
            LOCAL_VAR(probIdx) = SpecPos + ((2 | (LOCAL_VAR(posSlot) & 1)) << LOCAL_VAR(numDirectBits)) - LOCAL_VAR(posSlot) - 1;
          } else {
            LOCAL_VAR(numDirectBits) -= kNumAlignBits;
            goto do15; while (--LOCAL_VAR(numDirectBits) != 0) { do15: ;
              if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }
              SET_SHR(LOCAL_VAR(rangeLocal), 1);
              LOCAL_VAR(codeLocal) -= LOCAL_VAR(rangeLocal) & (SHR((LOCAL_VAR(codeLocal) - LOCAL_VAR(rangeLocal)), 31) - 1);
            }
            LOCAL_VAR(probIdx) = Align;
            LOCAL_VAR(numDirectBits) = kNumAlignBits;
          }
          {
            LOCAL_INIT(UInt32, localI, 1);
            goto do16; while (--LOCAL_VAR(numDirectBits) != 0) { do16: ;
              LOCAL_VAR(ttt) = GET_ARY16(probs, LOCAL_VAR(probIdx) + LOCAL_VAR(localI)); if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); }; LOCAL_VAR(bound) = SHR(LOCAL_VAR(rangeLocal), kNumBitModelTotalBits) * LOCAL_VAR(ttt); if (LOCAL_VAR(codeLocal) < LOCAL_VAR(bound)) { LOCAL_VAR(rangeLocal) = LOCAL_VAR(bound); LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)); ;; } else { LOCAL_VAR(rangeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(codeLocal) -= LOCAL_VAR(bound); LOCAL_VAR(localI) = (LOCAL_VAR(localI) + LOCAL_VAR(localI)) + 1; ;; };
            }
          }
        }
      }
    }
  }
  if (LOCAL_VAR(rangeLocal) < kTopValue) { if (LOCAL_VAR(bufDummyCur) >= LOCAL_VAR(bufLimit)) { return DUMMY_ERROR; } LOCAL_VAR(rangeLocal) <<= 8; LOCAL_VAR(codeLocal) = (LOCAL_VAR(codeLocal) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(bufDummyCur)++)); };
  return LOCAL_VAR(res);
ENDFUNC

FUNC_ARG2(void, LzmaDec_InitDicAndState, const Bool, initDic, const Bool, initState)
  GLOBAL_VAR(needFlush) = TRUE;
  GLOBAL_VAR(remainLen) = 0;
  GLOBAL_VAR(tempBufSize) = 0;

  if (LOCAL_VAR(initDic)) {
    GLOBAL_VAR(processedPos) = 0;
    GLOBAL_VAR(checkDicSize) = 0;
    GLOBAL_VAR(needInitLzma) = TRUE;
  }
  if (LOCAL_VAR(initState)) {
    GLOBAL_VAR(needInitLzma) = TRUE;
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

  while (GLOBAL_VAR(remainLen) != kMatchSpecLenStart) {
    LOCAL(Bool, checkEndMarkNow);

    if (GLOBAL_VAR(needFlush)) {
      /* Read 5 bytes (RC_INIT_SIZE) to tempBuf, first of which must be
       * 0, initialize the range coder with the 4 bytes after the 0 byte.
       */
      for (; LOCAL_VAR(decodeLimit) > GLOBAL_VAR(readCur) && GLOBAL_VAR(tempBufSize) < RC_INIT_SIZE;) {
        SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
      }
      if (GLOBAL_VAR(tempBufSize) < RC_INIT_SIZE) {
       on_needs_more_input:
        if (LOCAL_VAR(decodeLimit) != GLOBAL_VAR(readCur)) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
        return SZ_ERROR_NEEDS_MORE_INPUT;
      }
      if (GET_ARY8(readBuf, READBUF_SIZE) != 0) {
        return SZ_ERROR_DATA;
      }
      GLOBAL_VAR(code) = (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 1)) << 24) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 2)) << 16) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 3)) << 8) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 4)));
      GLOBAL_VAR(range) = 0xFFFFFFFF;
      GLOBAL_VAR(needFlush) = FALSE;
      GLOBAL_VAR(tempBufSize) = 0;
    }

    LOCAL_VAR(checkEndMarkNow) = FALSE;
    if (GLOBAL_VAR(dicPos) >= GLOBAL_VAR(dicBufSize)) {
      if (GLOBAL_VAR(remainLen) == 0 && GLOBAL_VAR(code) == 0) {
        if (LOCAL_VAR(decodeLimit) != GLOBAL_VAR(readCur)) { return SZ_ERROR_CHUNK_NOT_CONSUMED; }
        return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
      }
      if (GLOBAL_VAR(remainLen) != 0) {
        return SZ_ERROR_NOT_FINISHED;
      }
      LOCAL_VAR(checkEndMarkNow) = TRUE;
    }

    if (GLOBAL_VAR(needInitLzma)) {
      LOCAL_INIT(UInt32, numProbs, Literal + (ENSURE_32BIT(LZMA_LIT_SIZE) << (GLOBAL_VAR(lc) + GLOBAL_VAR(lp))));
      LOCAL(UInt32, probIdx);
      for (LOCAL_VAR(probIdx) = 0; LOCAL_VAR(probIdx) < LOCAL_VAR(numProbs); LOCAL_VAR(probIdx)++) {
        SET_ARY16(probs, LOCAL_VAR(probIdx), SHR_SMALL(kBitModelTotal, 1));
      }
      GLOBAL_VAR(rep0) = GLOBAL_VAR(rep1) = GLOBAL_VAR(rep2) = GLOBAL_VAR(rep3) = 1;
      GLOBAL_VAR(state) = 0;
      GLOBAL_VAR(needInitLzma) = FALSE;
    }

    if (GLOBAL_VAR(tempBufSize) == 0) {
      LOCAL(UInt32, bufLimit);
      if (LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur) < LZMA_REQUIRED_INPUT_MAX || LOCAL_VAR(checkEndMarkNow)) {
        LOCAL(SRes, dummyRes);
        LOCAL_VAR(dummyRes) = LzmaDec_TryDummy(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit));
        if (LOCAL_VAR(dummyRes) == DUMMY_ERROR) {
          /* This line can be triggered by passing LOCAL_VAR(srcLen)==1 to LzmaDec_DecodeToDic. */
          GLOBAL_VAR(tempBufSize) = 0;
          while (GLOBAL_VAR(readCur) != LOCAL_VAR(decodeLimit)) {
            SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
          }
          goto on_needs_more_input;
        }
        if (LOCAL_VAR(checkEndMarkNow) && LOCAL_VAR(dummyRes) != DUMMY_MATCH) {
          return SZ_ERROR_NOT_FINISHED;
        }
        LOCAL_VAR(bufLimit) = GLOBAL_VAR(readCur);
      } else {
        LOCAL_VAR(bufLimit) = LOCAL_VAR(decodeLimit) - LZMA_REQUIRED_INPUT_MAX;
      }
      GLOBAL_VAR(bufCur) = GLOBAL_VAR(readCur);
      if (LzmaDec_DecodeReal2(GLOBAL_VAR(dicBufSize), LOCAL_VAR(bufLimit)) != 0) {
        return SZ_ERROR_DATA;
      }
      GLOBAL_VAR(readCur) = GLOBAL_VAR(bufCur);
    } else {
      LOCAL_INIT(UInt32, rem, GLOBAL_VAR(tempBufSize));
      LOCAL_INIT(UInt32, lookAhead, 0);
      while (LOCAL_VAR(rem) < LZMA_REQUIRED_INPUT_MAX && LOCAL_VAR(lookAhead) < LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur)) {
        SET_ARY8(readBuf, READBUF_SIZE + LOCAL_VAR(rem)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur) + LOCAL_VAR(lookAhead)++));
      }
      GLOBAL_VAR(tempBufSize) = LOCAL_VAR(rem);
      if (LOCAL_VAR(rem) < LZMA_REQUIRED_INPUT_MAX || LOCAL_VAR(checkEndMarkNow)) {
        LOCAL(SRes, dummyRes);
        LOCAL_VAR(dummyRes) = LzmaDec_TryDummy(READBUF_SIZE, READBUF_SIZE + LOCAL_VAR(rem));
        if (LOCAL_VAR(dummyRes) == DUMMY_ERROR) {
          GLOBAL_VAR(readCur) += LOCAL_VAR(lookAhead);
          goto on_needs_more_input;
        }
        if (LOCAL_VAR(checkEndMarkNow) && LOCAL_VAR(dummyRes) != DUMMY_MATCH) {
          return SZ_ERROR_NOT_FINISHED;
        }
      }
      /* This line can be triggered by passing LOCAL_VAR(srcLen)==1 to LzmaDec_DecodeToDic. */
      GLOBAL_VAR(bufCur) = READBUF_SIZE;  /* tempBuf. */
      if (LzmaDec_DecodeReal2(0, READBUF_SIZE) != 0) {
        return SZ_ERROR_DATA;
      }
      LOCAL_VAR(lookAhead) -= LOCAL_VAR(rem) - (GLOBAL_VAR(bufCur) - READBUF_SIZE);
      GLOBAL_VAR(readCur) += LOCAL_VAR(lookAhead);
      GLOBAL_VAR(tempBufSize) = 0;
    }
  }
  if (GLOBAL_VAR(code) != 0) { return SZ_ERROR_DATA; }
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
FUNC_ARG1(UInt32, Preread, const UInt32, prereadSize)
  LOCAL_INIT(UInt32, prereadPos, GLOBAL_VAR(readEnd) - GLOBAL_VAR(readCur));
  ASSERT(LOCAL_VAR(prereadSize) <= READBUF_SIZE);
  if (LOCAL_VAR(prereadPos) < LOCAL_VAR(prereadSize)) {  /* Not enough pending available. */
    if (READBUF_SIZE - GLOBAL_VAR(readCur) < LOCAL_VAR(prereadSize)) {
      /* If no room for LOCAL_VAR(prereadSize) bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", LOCAL_VAR(prereadPos));
      for (GLOBAL_VAR(readEnd) = 0; GLOBAL_VAR(readEnd) < LOCAL_VAR(prereadPos); ++GLOBAL_VAR(readEnd)) {
        SET_ARY8(readBuf, GLOBAL_VAR(readEnd), GET_ARY8(readBuf, GLOBAL_VAR(readCur) + GLOBAL_VAR(readEnd)));
      }
      GLOBAL_VAR(readCur) = 0;
    }
    while (LOCAL_VAR(prereadPos) < LOCAL_VAR(prereadSize)) {
      /* Instead of (LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos)) we could use (GLOBAL_VAR(readBuf) + READBUF_SIZE -
       * GLOBAL_VAR(readEnd)) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos));
      LOCAL_INIT(UInt32, got, READ_FROM_STDIN_TO_ARY8(readBuf, GLOBAL_VAR(readEnd), LOCAL_VAR(prereadSize) - LOCAL_VAR(prereadPos)));
      if (TRUNCATE_TO_32BIT(LOCAL_VAR(got) - 1) & 0x80000000) { BREAK; }  /* EOF or error on input. */
      GLOBAL_VAR(readEnd) += LOCAL_VAR(got);
      LOCAL_VAR(prereadPos) += LOCAL_VAR(got);
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", LOCAL_VAR(prereadSize), LOCAL_VAR(prereadPos));
  return LOCAL_VAR(prereadPos);
ENDFUNC

FUNC_ARG0(void, IgnoreVarint)
  while (GET_ARY8(readBuf, GLOBAL_VAR(readCur)++) >= 0x80) {}
ENDFUNC

FUNC_ARG1(SRes, IgnoreZeroBytes, UInt32, zeroByteCount)
  for (; LOCAL_VAR(zeroByteCount) > 0; --LOCAL_VAR(zeroByteCount)) {
    if (GET_ARY8(readBuf, GLOBAL_VAR(readCur)++) != 0) {
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
  /* GLOBAL_VAR(lc) = GLOBAL_VAR(pb) = GLOBAL_VAR(lp) = 0; */
  GLOBAL_VAR(dicBufSize) = 0;  /* We'll increment it later. */
  GLOBAL_VAR(needInitDic) = TRUE;
  GLOBAL_VAR(needInitState) = TRUE;
  GLOBAL_VAR(needInitProp) = TRUE;
  GLOBAL_VAR(dicPos) = 0;
  LzmaDec_InitDicAndState(TRUE, TRUE);
ENDFUNC

FUNC_ARG1(SRes, InitProp, Byte, propByte)
  if (LOCAL_VAR(propByte) >= (9 * 5 * 5)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  GLOBAL_VAR(lc) = LOCAL_VAR(propByte) % 9;
  LOCAL_VAR(propByte) /= 9;
  GLOBAL_VAR(pb) = LOCAL_VAR(propByte) / 5;
  GLOBAL_VAR(lp) = LOCAL_VAR(propByte) % 5;
  if (GLOBAL_VAR(lc) + GLOBAL_VAR(lp) > LZMA2_LCLP_MAX) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  GLOBAL_VAR(needInitProp) = FALSE;
  return SZ_OK;
ENDFUNC

/* Writes uncompressed data (GET_ARY8(dic, LOCAL_VAR(fromDicPos) : GLOBAL_VAR(dicPos)) to stdout. */
FUNC_ARG1(SRes, WriteFrom, UInt32, fromDicPos)
  DEBUGF("WRITE %d dicPos=%d\n", GLOBAL_VAR(dicPos) - LOCAL_VAR(fromDicPos), GLOBAL_VAR(dicPos));
  while (LOCAL_VAR(fromDicPos) != GLOBAL_VAR(dicPos)) {
    LOCAL_INIT(UInt32, got, WRITE_TO_STDOUT_FROM_ARY8(dic, LOCAL_VAR(fromDicPos), GLOBAL_VAR(dicPos) - LOCAL_VAR(fromDicPos)));
    if (LOCAL_VAR(got) & 0x80000000) { return SZ_ERROR_WRITE; }
    LOCAL_VAR(fromDicPos) += LOCAL_VAR(got);
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
  if (Preread(12 + 12 + 6) < 12 + 12 + 6) { return SZ_ERROR_INPUT_EOF; }
  /* readBuf[6] is actually stream flags, should also be 0. */
  if (GET_ARY8(readBuf, 0) == 0xfd && GET_ARY8(readBuf, 1) == 0x37 &&
      GET_ARY8(readBuf, 2) == 0x7a && GET_ARY8(readBuf, 3) == 0x58 &&
      GET_ARY8(readBuf, 4) == 0x5a && GET_ARY8(readBuf, 5) == 0 &&
      GET_ARY8(readBuf, 6) == 0) {  /* .xz: "\xFD""7zXZ\0" */
  } ELSE_IF (GET_ARY8(readBuf, GLOBAL_VAR(readCur)) <= 225 && GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 13) == 0 &&  /* .lzma */
        /* High 4 bytes of uncompressed size. */
        ((LOCAL_VAR(bhf) = GetLE4(GLOBAL_VAR(readCur) + 9)) == 0 || LOCAL_VAR(bhf) == TRUNCATE_TO_32BIT(-1)) &&
        (GLOBAL_VAR(dicSize) = GetLE4(GLOBAL_VAR(readCur) + 1)) >= LZMA_DIC_MIN &&
        GLOBAL_VAR(dicSize) <= DIC_ARRAY_SIZE) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    LOCAL(UInt32, readBufUS);
    LOCAL(UInt32, srcLen);
    LOCAL(UInt32, fromDicPos);
    InitDecode();
    /* LZMA restricts lc + lp <= 4. LZMA requires lc + lp <= 12.
     * We apply the LZMA2 restriction here (to save memory in
     * GLOBAL_VAR(probs)), thus we are not able to extract some legitimate
     * .lzma files.
     */
    if ((LOCAL_VAR(res) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur)))) != SZ_OK) {
      return LOCAL_VAR(res);
    }
    if (LOCAL_VAR(bhf) == 0) {
      GLOBAL_VAR(dicBufSize) = LOCAL_VAR(readBufUS) = GetLE4(GLOBAL_VAR(readCur) + 5);
      if (LOCAL_VAR(readBufUS) > DIC_ARRAY_SIZE) { return SZ_ERROR_MEM; }
    } else {
      LOCAL_VAR(readBufUS) = LOCAL_VAR(bhf);  /* max UInt32. */
      GLOBAL_VAR(dicBufSize) = DIC_ARRAY_SIZE;
    }
    GLOBAL_VAR(readCur) += 13;  /* Start decompressing the 0 byte. */
    DEBUGF("LZMA dicSize=0x%x us=%d bhf=%d\n", GLOBAL_VAR(dicSize), LOCAL_VAR(readBufUS), LOCAL_VAR(bhf));
    /* TODO(pts): Limit on uncompressed size unless 8 bytes of -1 is
     * specified.
     */
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while ((LOCAL_VAR(srcLen) = Preread(READBUF_SIZE)) > 0) {
      LOCAL(SRes, res);
      LOCAL_VAR(fromDicPos) = GLOBAL_VAR(dicPos);
      LOCAL_VAR(res) = LzmaDec_DecodeToDic(LOCAL_VAR(srcLen));
      DEBUGF("LZMADEC res=%d\n", LOCAL_VAR(res));
      if (GLOBAL_VAR(dicPos) > LOCAL_VAR(readBufUS)) { GLOBAL_VAR(dicPos) = LOCAL_VAR(readBufUS); }
      if ((LOCAL_VAR(res) = WriteFrom(LOCAL_VAR(fromDicPos))) != SZ_OK) { return LOCAL_VAR(res); }
      if (LOCAL_VAR(res) == SZ_ERROR_FINISHED_WITH_MARK) { BREAK; }
      if (LOCAL_VAR(res) != SZ_ERROR_NEEDS_MORE_INPUT && LOCAL_VAR(res) != SZ_OK) { return LOCAL_VAR(res); }
      if (GLOBAL_VAR(dicPos) == LOCAL_VAR(readBufUS)) { BREAK; }
    }
    return SZ_OK;
  } else {
    return SZ_ERROR_BAD_MAGIC;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  LOCAL_VAR(checksumSize) = GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 7);
  if (LOCAL_VAR(checksumSize) == 0) { /* None */ LOCAL_VAR(checksumSize) = 1; }
  ELSE_IF (LOCAL_VAR(checksumSize) == 1) { /* CRC32 */ LOCAL_VAR(checksumSize) = 4; }
  ELSE_IF (LOCAL_VAR(checksumSize) == 4) { /* CRC64, typical xz output. */ LOCAL_VAR(checksumSize) = 8; }
  else { return SZ_ERROR_BAD_CHECKSUM_TYPE; }
  /* Also ignore the CRC32 after LOCAL_VAR(checksumSize). */
  GLOBAL_VAR(readCur) += 12;
  for (;;) {  /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    LOCAL_INIT(Byte, blockSizePad, 3);
    LOCAL(UInt32, bhs);
    LOCAL(UInt32, bhs2);  /* Block header size */
    LOCAL(Byte, dicSizeProp);
    LOCAL(UInt32, readAtBlock);
    ASSERT(GLOBAL_VAR(readEnd) - GLOBAL_VAR(readCur) >= 12);  /* At least 12 bytes preread. */
    LOCAL_VAR(readAtBlock) = GLOBAL_VAR(readCur);
    /* Last block, index follows. */
    if ((LOCAL_VAR(bhs) = GET_ARY8(readBuf, GLOBAL_VAR(readCur)++)) == 0) { BREAK; }
    /* Block header size includes the LOCAL_VAR(bhs) field above and the CRC32 below. */
    LOCAL_VAR(bhs) = (LOCAL_VAR(bhs) + 1) << 2;
    DEBUGF("LOCAL_VAR(bhs)=%d\n", LOCAL_VAR(bhs));
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (Preread(LOCAL_VAR(bhs)) < LOCAL_VAR(bhs)) { return SZ_ERROR_INPUT_EOF; }
    LOCAL_VAR(readAtBlock) = GLOBAL_VAR(readCur);
    LOCAL_VAR(bhf) = GET_ARY8(readBuf, GLOBAL_VAR(readCur)++);
    if ((LOCAL_VAR(bhf) & 2) != 0) { return SZ_ERROR_UNSUPPORTED_FILTER_COUNT; }
    DEBUGF("filter count=%d\n", (LOCAL_VAR(bhf) & 2) + 1);
    if ((LOCAL_VAR(bhf) & 20) != 0) { return SZ_ERROR_BAD_BLOCK_FLAGS; }
    if (LOCAL_VAR(bhf) & 64) {  /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (LOCAL_VAR(bhf) & 128) {  /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (GET_ARY8(readBuf, GLOBAL_VAR(readCur)++) != FILTER_ID_LZMA2) { return SZ_ERROR_UNSUPPORTED_FILTER_ID; }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (GET_ARY8(readBuf, GLOBAL_VAR(readCur)++) != 1) { return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE; }
    LOCAL_VAR(dicSizeProp) = GET_ARY8(readBuf, GLOBAL_VAR(readCur)++);
    /* Typical large dictionary sizes:
     *
     *  * 35: 805306368 bytes == 768 MiB
     *  * 36: 1073741824 bytes == 1 GiB
     *  * 37: 1610612736 bytes, largest supported by .xz
     *  * 38: 2147483648 bytes == 2 GiB
     *  * 39: 3221225472 bytes == 3 GiB
     *  * 40: 4294967295 bytes, largest supported by .xz
     */
    DEBUGF("LOCAL_VAR(dicSizeProp)=0x%02x\n", LOCAL_VAR(dicSizeProp));
    if (LOCAL_VAR(dicSizeProp) > 40) { return SZ_ERROR_BAD_DICTIONARY_SIZE; }
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (LOCAL_VAR(dicSizeProp) > 37) { return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE; }
    GLOBAL_VAR(dicSize) = ((ENSURE_32BIT(2) | ((LOCAL_VAR(dicSizeProp)) & 1)) << ((LOCAL_VAR(dicSizeProp)) / 2 + 11));
    ASSERT(GLOBAL_VAR(dicSize) >= LZMA_DIC_MIN);
    DEBUGF("dicSize39=%u\n", ((ENSURE_32BIT(2) | ((39) & 1)) << ((39) / 2 + 11)));
    DEBUGF("dicSize38=%u\n", ((ENSURE_32BIT(2) | ((38) & 1)) << ((38) / 2 + 11)));
    DEBUGF("dicSize37=%u\n", ((ENSURE_32BIT(2) | ((37) & 1)) << ((37) / 2 + 11)));
    DEBUGF("dicSize36=%u\n", ((ENSURE_32BIT(2) | ((36) & 1)) << ((36) / 2 + 11)));
    DEBUGF("dicSize35=%u\n", ((ENSURE_32BIT(2) | ((35) & 1)) << ((35) / 2 + 11)));
    LOCAL_VAR(bhs2) = GLOBAL_VAR(readCur) - LOCAL_VAR(readAtBlock) + 5;  /* Won't overflow. */
    DEBUGF("LOCAL_VAR(bhs)=%d LOCAL_VAR(bhs2)=%d\n", LOCAL_VAR(bhs), LOCAL_VAR(bhs2));
    if (LOCAL_VAR(bhs2) > LOCAL_VAR(bhs)) { return SZ_ERROR_BLOCK_HEADER_TOO_LONG; }
    if ((LOCAL_VAR(res) = IgnoreZeroBytes(LOCAL_VAR(bhs) - LOCAL_VAR(bhs2))) != SZ_OK) { return LOCAL_VAR(res); }
    GLOBAL_VAR(readCur) += 4;  /* Ignore CRC32. */
    /* Typically it's LOCAL_VAR(offset) 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2\n");
    {  /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      LOCAL(UInt32, chunkUS);  /* Uncompressed chunk sizes. */
      LOCAL(UInt32, chunkCS);  /* Compressed chunk size. */
      InitDecode();

      for (;;) {
        LOCAL(Byte, control);
        ASSERT(GLOBAL_VAR(dicPos) == GLOBAL_VAR(dicBufSize));
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (Preread(6) < 6) { return SZ_ERROR_INPUT_EOF; }
        LOCAL_VAR(control) = GET_ARY8(readBuf, GLOBAL_VAR(readCur));
        DEBUGF("CONTROL control=0x%02x at=? inbuf=%d\n", LOCAL_VAR(control), GLOBAL_VAR(readCur));
        if (LOCAL_VAR(control) == 0) {
          DEBUGF("LASTFED\n");
          ++GLOBAL_VAR(readCur);
          BREAK;
        } ELSE_IF (TRUNCATE_TO_8BIT(LOCAL_VAR(control) - 3) < 0x80 - 3) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        LOCAL_VAR(chunkUS) = (GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 1) << 8) + GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 2) + 1;
        if (LOCAL_VAR(control) < 3) {  /* Uncompressed chunk. */
          LOCAL_INIT(const Bool, initDic, LOCAL_VAR(control) == 1);
          LOCAL_VAR(chunkCS) = LOCAL_VAR(chunkUS);
          GLOBAL_VAR(readCur) += 3;
          /* TODO(pts): Porting: TRUNCATE_TO_8BIT(LOCAL_VAR(blockSizePad)) for Python and other unlimited-integer-range languages. */
          LOCAL_VAR(blockSizePad) -= 3;
          if (LOCAL_VAR(initDic)) {
            GLOBAL_VAR(needInitProp) = GLOBAL_VAR(needInitState) = TRUE;
            GLOBAL_VAR(needInitDic) = FALSE;
          } ELSE_IF (GLOBAL_VAR(needInitDic)) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(LOCAL_VAR(initDic), FALSE);
        } else {  /* LZMA chunk. */
          LOCAL_INIT(const Byte, mode, (SHR_SMALL((LOCAL_VAR(control)), 5) & 3));
          LOCAL_INIT(const Bool, initDic, LOCAL_VAR(mode) == 3);
          LOCAL_INIT(const Bool, initState, LOCAL_VAR(mode) > 0);
          LOCAL_INIT(const Bool, isProp, (LOCAL_VAR(control) & 64) != 0);
          LOCAL_VAR(chunkUS) += (LOCAL_VAR(control) & 31) << 16;
          LOCAL_VAR(chunkCS) = (GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 3) << 8) + GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 4) + 1;
          if (LOCAL_VAR(isProp)) {
            if ((LOCAL_VAR(res) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 5))) != SZ_OK) {
              return LOCAL_VAR(res);
            }
            ++GLOBAL_VAR(readCur);
            --LOCAL_VAR(blockSizePad);
          } else {
            if (GLOBAL_VAR(needInitProp)) { return SZ_ERROR_MISSING_INITPROP; }
          }
          GLOBAL_VAR(readCur) += 5;
          LOCAL_VAR(blockSizePad) -= 5;
          if ((!LOCAL_VAR(initDic) && GLOBAL_VAR(needInitDic)) || (!LOCAL_VAR(initState) && GLOBAL_VAR(needInitState))) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(LOCAL_VAR(initDic), LOCAL_VAR(initState));
          GLOBAL_VAR(needInitDic) = FALSE;
          GLOBAL_VAR(needInitState) = FALSE;
        }
        ASSERT(GLOBAL_VAR(dicPos) == GLOBAL_VAR(dicBufSize));
        GLOBAL_VAR(dicBufSize) += LOCAL_VAR(chunkUS);
        /* Decompressed data too long, won't fit to GLOBAL_VAR(dic). */
        if (GLOBAL_VAR(dicBufSize) > DIC_ARRAY_SIZE) { return SZ_ERROR_MEM; }
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (Preread(LOCAL_VAR(chunkCS) + 6) < LOCAL_VAR(chunkCS)) { return SZ_ERROR_INPUT_EOF; }
        DEBUGF("FEED us=%d cs=%d dicPos=%d\n", LOCAL_VAR(chunkUS), LOCAL_VAR(chunkCS), GLOBAL_VAR(dicPos));
        if (LOCAL_VAR(control) < 0x80) {  /* Uncompressed chunk. */
          DEBUGF("DECODE uncompressed\n");
          while (GLOBAL_VAR(dicPos) != GLOBAL_VAR(dicBufSize)) {
            SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
          }
          if (GLOBAL_VAR(checkDicSize) == 0 && GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos) <= LOCAL_VAR(chunkUS)) {
            GLOBAL_VAR(checkDicSize) = GLOBAL_VAR(dicSize);
          }
          GLOBAL_VAR(processedPos) += LOCAL_VAR(chunkUS);
        } else {  /* Compressed chunk. */
          DEBUGF("DECODE call\n");
          /* This call doesn't change GLOBAL_VAR(dicBufSize). */
          if ((LOCAL_VAR(res) = LzmaDec_DecodeToDic(LOCAL_VAR(chunkCS))) != SZ_OK) { return LOCAL_VAR(res); }
        }
        if (GLOBAL_VAR(dicPos) != GLOBAL_VAR(dicBufSize)) { return SZ_ERROR_BAD_DICPOS; }
        if ((LOCAL_VAR(res) = WriteFrom(GLOBAL_VAR(dicPos) - LOCAL_VAR(chunkUS))) != SZ_OK) { return LOCAL_VAR(res); }
        LOCAL_VAR(blockSizePad) -= LOCAL_VAR(chunkCS);
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
    if (Preread(7 + 12 + 6) < 7 + 12 + 6) { return SZ_ERROR_INPUT_EOF; }
    DEBUGF("ALTELL blockSizePad=%d\n", LOCAL_VAR(blockSizePad) & 3);
    /* Ignore block padding. */
    if ((LOCAL_VAR(res) = IgnoreZeroBytes(LOCAL_VAR(blockSizePad) & 3)) != SZ_OK) { return LOCAL_VAR(res); }
    GLOBAL_VAR(readCur) += LOCAL_VAR(checksumSize);  /* Ignore CRC32, CRC64 etc. */
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
  LOCAL_VAR(res) = DecompressXzOrLzma();
  CLEAR_ARY16(probs);
  CLEAR_ARY8(readBuf);
  CLEAR_ARY8(dic);
  return LOCAL_VAR(res);
ENDFUNC

my $res = Decompress();
die "PERL_ERROR:$res\n" if $res;
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_PERL
my $res = Decompress();
die "PERL_ERROR:$res" if $res;
#endif
