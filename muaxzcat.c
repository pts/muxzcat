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

#if defined(CONFIG_DEBUG_VARS) || defined(CONFIG_DEBUG_VAR_RANGES)
#ifndef CONFIG_DEBUG
#define CONFIG_DEBUG 1
#endif
#endif

#ifdef CONFIG_LANG_JAVA
START_PREPROCESSED
#else
#ifdef CONFIG_LANG_PERL
START_PREPROCESSED
#else
#ifdef __TINYC__  /* tcc https://bellard.org/tcc/ , pts-tcc https://github.com/pts/pts-tcc */

#ifdef CONFIG_DEBUG
#error __TINYC__ does not support CONFIG_DEBUG
#endif

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

#ifdef CONFIG_DEBUG
#error __XTINY__ does not support CONFIG_DEBUG
#endif

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
#endif  /* Not CONFIG_LANG_JAVA. */

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
#define BOOL_TO_INT(x) (x)
#define TO_BOOL(x) (x)
#define TO_BOOL_NEG(x) (!(x))
#define HAS_GOTO 1
#define DEFAULT_CONTINUE_TO(label) continue
#define DEFAULT_BREAK_TO(label) break
#define DEFAULT_LABEL(label)
#define ELSE_IF else if
#define BREAK break
#define CONTINUE continue
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_JAVA  /* !! Use symbolic constants in the .java file. */
#undef HAS_GOTO
#define BOOL_TO_INT(x) ((x) ? 1 : 0)
#define TO_BOOL(x) ((x) != 0)  /* !! TODO(pts): Faster with boolean type. */
#define TO_BOOL_NEG(x) ((x) == 0)  /* !! TODO(pts): Faster with boolean type. */
#define DEFAULT_CONTINUE_TO(label) continue
#define DEFAULT_BREAK_TO(label) break
#define DEFAULT_LABEL(label)
#define ELSE_IF else if
#define BREAK break
#define CONTINUE continue
#endif  /* CONFIG_LANG_JAVA */
#ifdef CONFIG_LANG_PERL
#define HAS_GOTO 1
#define BOOL_TO_INT(x) (x)
#define TO_BOOL(x) (x)
#define TO_BOOL_NEG(x) (!(x))
#define DEFAULT_CONTINUE_TO(label) goto label
#define DEFAULT_BREAK_TO(label) goto label
#define DEFAULT_LABEL(label) label: ;
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
#ifdef CONFIG_LANG_JAVA
#define ENSURE_32BIT(x) (x)
#define TRUNCATE_TO_32BIT(x) ((x) & 0xffffffff)
#define TRUNCATE_TO_16BIT(x) ((x) & 0xffff)
#define TRUNCATE_TO_8BIT(x) ((x) & 0xff)
#endif  /* CONFIG_LANG_JAVA */
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
#define GET_ARY16(a, idx) ((UInt32)global.a##16[ASSERT_IS_SMALL(idx)])
/* If the base type was larger than uint16_t, we'd have to call TRUNCATE_TO_16BIT(value) here. */
#define SET_ARY16(a, idx, value) (global.a##16[ASSERT_IS_SMALL(idx)] = value)
#define CLEAR_ARY8(a)
#define GET_ARY8(a, idx) ((UInt32)global.a##8[ASSERT_IS_SMALL(idx)])
/* If the base type was larger than uint8_t, we'd have to call TRUNCATE_TO_8BIT(value) here. */
#define SET_ARY8(a, idx, value) (global.a##8[ASSERT_IS_SMALL(idx)] = value)
#define CLEAR_ARY16(a)
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) (read(0, &global.a##8[fromIdx], size))
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) (write(1, &global.a##8[fromIdx], size))
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_JAVA
#define GLOBALS
#define GLOBAL(type, name) static int GLOBAL_##name
#define GLOBAL_ARY16(a, size) static short a##16[]
#define GLOBAL_ARY8(a, size) static byte a##8[]
#define ENDGLOBALS
#define GLOBAL_VAR(name) GLOBAL_##name  /* Get or set a global variable. */
#define SET_GLOBAL(name, setid, op) GLOBAL_##name op
#define GET_ARY16(a, idx) (a##16[idx] & 0xffff)
#define SET_ARY16(a, idx, value) a##16[idx] = (short)(value)  /* Parens would cause ``not a statement'' error. */
#define CLEAR_ARY8(a)
#define GET_ARY8(a, idx) (a##8[idx] & 0xff)
#define SET_ARY8(a, idx, value) a##8[idx] = (byte)(value)  /* Parens would cause ``not a statement'' error. */
#define CLEAR_ARY16(a)
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) System.in.read(a##8, fromIdx, size)
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) System.out.write(a##8, fromIdx, size)
#endif  /* CONFIG_LANG_JAVA */
#ifdef CONFIG_LANG_PERL
#define GLOBALS
#define GLOBAL(type, name) GLOBAL my($##name) = 0
#define GLOBAL_ARY16(a, size) GLOBAL my($##a) = ''
#define GLOBAL_ARY8(a, size) GLOBAL my($##a) = ''
#define ENDGLOBALS
#define GLOBAL_VAR(name) $##name
#define SET_GLOBAL(name, setid, op) $##name op
#define GET_ARY16(a, idx) vec($##a, idx, 16)
#define SET_ARY16(a, idx, value) vec($##a, idx, 16) = value
#define CLEAR_ARY16(a) $##a = ''
#define GET_ARY8(a, idx) vec($##a, idx, 8)
#define SET_ARY8(a, idx, value) vec($##a, idx, 8) = value
#define CLEAR_ARY8(a) $##a = ''
#define READ_FROM_STDIN_TO_ARY8(a, fromIdx, size) UndefToMinus1(sysread(STDIN, $##a, size, fromIdx))
#define WRITE_TO_STDOUT_FROM_ARY8(a, fromIdx, size) UndefToMinus1(syswrite(STDOUT, $##a, size, fromIdx))
#endif  /* CONFIG_LANG_PERL */

#ifdef CONFIG_LANG_C
#define LOCAL(type, name) type LOCAL_##name
#define LOCAL_INIT(type, name, value) type LOCAL_##name = value
#define LOCAL_VAR(name) LOCAL_##name  /* Get or set a local variable or function argument. */
#define SET_LOCALB(name, setid, op, value) LOCAL_##name op value
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_JAVA
#define LOCAL(type, name) int LOCAL_##name
#define LOCAL_INIT(type, name, value) int LOCAL_##name = value
#define LOCAL_VAR(name) LOCAL_##name  /* Get or set a local variable or function argument. */
#define SET_LOCALB(name, setid, op, value) LOCAL_##name op value
#endif  /* CONFIG_LANG_JAVA */
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
#ifdef CONFIG_LANG_JAVA
#define RETURN_TYPE_UInt32 int
#define RETURN_TYPE_SRes int
#define RETURN_TYPE_Byte int
#define RETURN_TYPE_void void
#define FUNC_ARG0(return_type, name) static final RETURN_TYPE_##return_type name() {
#define FUNC_ARG1(return_type, name, arg1_type, arg1) static final RETURN_TYPE_##return_type name(int LOCAL_##arg1) {
#define FUNC_ARG2(return_type, name, arg1_type, arg1, arg2_type, arg2) static final RETURN_TYPE_##return_type name(int LOCAL_##arg1, int LOCAL_##arg2) {
#define ENDFUNC }
#endif  /* CONFIG_LANG_JAVA */
#ifdef CONFIG_LANG_PERL
#define FUNC_ARG0(return_type, name) sub name() {
#define FUNC_ARG1(return_type, name, arg1_type, arg1) sub name($) { my $##arg1 = $_[0];
#define FUNC_ARG2(return_type, name, arg1_type, arg1, arg2_type, arg2) sub name($$) { my($##arg1, $##arg2) = @_;
#define ENDFUNC }
#endif  /* CONFIG_LANG_PERL */

/* TODO(pts): Simplify doublings: e.g.
 * LOCAL_VAR(drSymbol) = (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol))
 * SET_LOCALB(distance, 251, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)));
 * SET_LOCALB(distance, 257, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)) + 1);
  */
/* The code doesn't have overflowing / /= % %=, so we don't create macros for these. */
#define IS_SMALL(x) (((x) & ~0x7fffffff) == 0)
#ifdef CONFIG_DEUBG
#define SHR_SMALLX(x, y) (ASSERT_IS_SMALL(x) >> ASSERT_IS_5BIT(y))
#define EQ_SMALL(x, y) (ASSERT_IS_SMALL(x) == ASSERT_IS_SMALL(y))
#define NE_SMALL(x, y) (ASSERT_IS_SMALL(x) != ASSERT_IS_SMALL(y))
#define LT_SMALL(x, y) (ASSERT_IS_SMALL(x) < ASSERT_IS_SMALL(y))
#define LE_SMALL(x, y) (ASSERT_IS_SMALL(x) <= ASSERT_IS_SMALL(y))
#define GT_SMALL(x, y) (ASSERT_IS_SMALL(x) > ASSERT_IS_SMALL(y))
#define GE_SMALL(x, y) (ASSERT_IS_SMALL(x) >= ASSERT_IS_SMALL(y))
#else
/* These work only if IS_SMALL(x) and 0 <= y <= 31. */
#define SHR_SMALLX(x, y) ((x) >> (y))
/* These work only if IS_SMALL(x) && IS_SMALL(y). */
#define EQ_SMALL(x, y) ((x) == (y))
#define NE_SMALL(x, y) ((x) != (y))
#define LT_SMALL(x, y) ((x) < (y))
#define LE_SMALL(x, y) ((x) <= (y))
#define GT_SMALL(x, y) ((x) > (y))
#define GE_SMALL(x, y) ((x) >= (y))
#endif  /* CONFIG_DEBUG */
#ifdef CONFIG_LANG_C
#if defined(CONFIG_UINT64) || defined(CONFIG_INT64)
/* The CONFIG_LANG_PERL version would also work for SHR1 and SHR11. */
#define SHR1(x) (((x) & 0xffffffff) >> 1)
#define SHR11(x) (((x) & 0xffffffff) >> 11)
#define EQ0(x) (((x) & 0xffffffff) == 0)
#define NE0(x) (((x) & 0xffffffff) != 0)
#define LT(x, y) (((x) & 0xffffffff) < ((y) & 0xffffffff))
/* Like LT(x, y), but only works if IS_SMALL(y). */
#define LTX(x, y) (((x) & 0xffffffff) < ASSERT_IS_SMALL(y))
#else
#define SHR1(x) ((x) >> 1)
#define SHR11(x) ((x) >> 11)
#define EQ0(x) ((x) == 0)
#define NE0(x) ((x) != 0)
#define LT(x, y) ((x) < (y))
#define LTX(x, y) ((x) < ASSERT_IS_SMALL(y))
#endif
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_JAVA
#define SHR1(x) ((x) >>> 1)
#define SHR11(x) ((x) >>> 11)
/* genpl.sh has the 32-bit (slow) and 64-bit (fast) implementations of
 * EQ0, NE0 and LT.
 */
#define EQ0(x) ((x) == 0)
#define NE0(x) ((x) != 0)
#define LT(x, y) Lt32(x, y)  /* !! TODO(pts): Faster using local variables or long. */
#define LTX(x, y) Ltx32(x, y)  /* !! TODO(pts): Faster using local variables or long. */
#endif  /* CONFIG_LANG_JAVA */
#ifdef CONFIG_LANG_PERL
#define LTX(x, y) LTX[x],[y]
#define SHR1(x) (((x) >> 1) & 0x7fffffff)
#define SHR11(x) (((x) >> 11) & (0x7fffffff >> 10))
/* genpl.sh has the 32-bit (slow) and 64-bit (fast) implementations of
 * EQ0, NE0 and LT.
 */
#define EQ0(x) EQ0[x]
#define NE0(x) NE0[x]
#define LT(x, y) LT[x],[y]
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
#define ASSERT_IS_SMALL(x) ({ const UInt32 x2 = (x); ASSERT(IS_SMALL(x2)); x2; })
#define ASSERT_IS_5BIT(x) ({ const UInt32 x2 = (x); ASSERT((x2 & ~31) == 0); x2; })
#ifdef CONFIG_DEBUG_VARS
static void DumpVars(void);
/* ++ and -- operators for global and local variables could be instrumented
 * for completeness here.
 */
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ({ LOCAL_##name op value; DEBUGF("SET_LOCAL @%d %s=%u\n", setid, #name, (int)TRUNCATE_TO_32BIT(LOCAL_##name)); LOCAL_##name; })
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) ({ DumpVars(); DEBUGF("SET_GLOBAL %s @%d\n", #name, setid); &global.name; })[0] op
#undef  SET_ARY16
#define SET_ARY16(a, idx, value) ({ const UInt32 idx2 = ASSERT_IS_SMALL(idx); global.a##16[idx2] = value; DEBUGF("SET_ARY16 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##16[idx2])); global.a##16[idx2]; })
#undef  SET_ARY8
#define SET_ARY8(a, idx, value) ({ const UInt32 idx2 = ASSERT_IS_SMALL(idx); global.a##8[idx2] = value; DEBUGF("SET_ARY8 %s[%d]=%u\n", #a, (int)idx2, (int)(global.a##8[idx2])); global.a##8[idx2];  })
#endif  /* CONFIG_DEBUG_VARS */
#else
#define DEBUGF(...)
/* Just check that it compiles. */
#define ASSERT(condition) do {} while (0 && (condition))
#define ASSERT_IS_SMALL(x) (x)
#define ASSERT_IS_5BIT(x) (x)
#endif  /* !CONFIG_DEBUG */
#endif  /* CONFIG_LANG_C */
#ifdef CONFIG_LANG_JAVA
#define ASSERT_IS_SMALL(x) (x)
#define ASSERT_IS_5BIT(x) (x)
#define DEBUGF(...)
#define ASSERT(CONDITION)
#endif  /* CONFIG_LANG_JAVA */
#ifdef CONFIG_LANG_PERL
#define ASSERT_IS_SMALL(x) (x)
#define ASSERT_IS_5BIT(x) (x)
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

#define NOTICE_LOCAL_RANGE(name)
#define NOTICE_LOCAL_RANGE_VALUE(name, value)
#define NOTICE_GLOBAL_RANGE(name)
#ifdef CONFIG_LANG_C
#ifdef CONFIG_DEBUG_VAR_RANGES
/* Only these variables seem to be non-small (IS_SMALL(...) is false):
 * blockSizePad, bhf, distance (either ~0 or IS_SMALL), code, drBound,
 * range, tdBound, tdCode, tdRange, dicSize (only near the beginning in
 * DecompressXzOrLzma), dicBufSize (only near the beginning in
 * DecompressXzorLzma), readBufUS (only near the beginning in
 * DecompressXzorLzma).
 */
/* gcc -DCONFIG_DEBUG_VAR_RANGES -DCONFIG_INT64 -ansi -O2 -W -Wall -Wextra -Werror -o muxzcat muaxzcat.c */
#define DECLARE_VRMINMAX(name) int64_t VRMIN_##name = 0, VRMAX_##name = 0
#undef  NOTICE_GLOBAL_RANGE
#define NOTICE_GLOBAL_RANGE(name) do { if ((int64_t)global.name < VRMIN_##name) VRMIN_##name = global.name; if ((int64_t)global.name > VRMAX_##name) VRMAX_##name = global.name; } while (0)
#undef  SET_GLOBAL
#define SET_GLOBAL(name, setid, op) ({ NOTICE_GLOBAL_RANGE(name); &global.name; })[0] op
#undef  NOTICE_LOCAL_RANGE
#define NOTICE_LOCAL_RANGE(name) do { if ((int64_t)LOCAL_##name < VRMIN_##name) VRMIN_##name = LOCAL_##name; if ((int64_t)LOCAL_##name > VRMAX_##name) VRMAX_##name = LOCAL_##name; } while (0)
#undef  NOTICE_LOCAL_RANGE_VALUE
#define NOTICE_LOCAL_RANGE_VALUE(name, value) ({ const UInt32 value2 = (value); if ((int64_t)value2 < VRMIN_##name) VRMIN_##name = value2; if (value2 > VRMAX_##name) VRMAX_##name = value2; value2; })
#undef  LOCAL
#define LOCAL(type, name) type LOCAL_##name = 0  /* Initialize to 0 to prevent NOTICE_LOCAL_RANGE in LOCAL_VAR from reading an uninitialized value. */
#undef  LOCAL_INIT
#define LOCAL_INIT(type, name, value) type LOCAL_##name = NOTICE_LOCAL_RANGE_VALUE(name, value)
#undef  LOCAL_VAR
#define LOCAL_VAR(name) ({ NOTICE_LOCAL_RANGE(name); &LOCAL_##name; })[0]
#undef  SET_LOCALB
#define SET_LOCALB(name, setid, op, value) ({ LOCAL_##name op value; NOTICE_LOCAL_RANGE(name); LOCAL_##name; })
/**/
DECLARE_VRMINMAX(bufCur);
DECLARE_VRMINMAX(dicSize);
DECLARE_VRMINMAX(range);
DECLARE_VRMINMAX(code);
DECLARE_VRMINMAX(dicPos);
DECLARE_VRMINMAX(dicBufSize);
DECLARE_VRMINMAX(processedPos);
DECLARE_VRMINMAX(checkDicSize);
DECLARE_VRMINMAX(state);
DECLARE_VRMINMAX(rep0);
DECLARE_VRMINMAX(rep1);
DECLARE_VRMINMAX(rep2);
DECLARE_VRMINMAX(rep3);
DECLARE_VRMINMAX(remainLen);
DECLARE_VRMINMAX(tempBufSize);
DECLARE_VRMINMAX(readCur);
DECLARE_VRMINMAX(readEnd);
DECLARE_VRMINMAX(needFlush);
DECLARE_VRMINMAX(needInitLzma);
DECLARE_VRMINMAX(needInitDic);
DECLARE_VRMINMAX(needInitState);
DECLARE_VRMINMAX(needInitProp);
DECLARE_VRMINMAX(lc);
DECLARE_VRMINMAX(lp);
DECLARE_VRMINMAX(pb);
DECLARE_VRMINMAX(lcm8);
DECLARE_VRMINMAX(umValue);
DECLARE_VRMINMAX(wrDicLimit);
DECLARE_VRMINMAX(wrLen);
DECLARE_VRMINMAX(drDicLimit);
DECLARE_VRMINMAX(drBufLimit);
DECLARE_VRMINMAX(pbMask);
DECLARE_VRMINMAX(lpMask);
DECLARE_VRMINMAX(drI);
DECLARE_VRMINMAX(drDicLimit2);
DECLARE_VRMINMAX(drProbIdx);
DECLARE_VRMINMAX(drBound);
DECLARE_VRMINMAX(drTtt);
DECLARE_VRMINMAX(distance);
DECLARE_VRMINMAX(drPosState);
DECLARE_VRMINMAX(drSymbol);
DECLARE_VRMINMAX(drMatchByte);
DECLARE_VRMINMAX(drMatchMask);
DECLARE_VRMINMAX(drBit);
DECLARE_VRMINMAX(drProbLitIdx);
DECLARE_VRMINMAX(drLimitSub);
DECLARE_VRMINMAX(drOffset);
DECLARE_VRMINMAX(drProbLenIdx);
DECLARE_VRMINMAX(drPosSlot);
DECLARE_VRMINMAX(drDirectBitCount);
DECLARE_VRMINMAX(mask);
DECLARE_VRMINMAX(drRem);
DECLARE_VRMINMAX(curLen);
DECLARE_VRMINMAX(pos);
DECLARE_VRMINMAX(tdCur);
DECLARE_VRMINMAX(tdBufLimit);
DECLARE_VRMINMAX(tdRange);
DECLARE_VRMINMAX(tdCode);
DECLARE_VRMINMAX(tdState);
DECLARE_VRMINMAX(tdRes);
DECLARE_VRMINMAX(tdProbIdx);
DECLARE_VRMINMAX(tdBound);
DECLARE_VRMINMAX(tdTtt);
DECLARE_VRMINMAX(tdPosState);
DECLARE_VRMINMAX(tdSymbol);
DECLARE_VRMINMAX(tdMatchByte);
DECLARE_VRMINMAX(tdMatchMask);
DECLARE_VRMINMAX(tdBit);
DECLARE_VRMINMAX(tdProbLitIdx);
DECLARE_VRMINMAX(tdLen);
DECLARE_VRMINMAX(tdLimitSub);
DECLARE_VRMINMAX(tdOffset);
DECLARE_VRMINMAX(tdProbLenIdx);
DECLARE_VRMINMAX(tdPosSlot);
DECLARE_VRMINMAX(tdDirectBitCount);
DECLARE_VRMINMAX(tdI);
DECLARE_VRMINMAX(idInitDic);
DECLARE_VRMINMAX(idInitState);
DECLARE_VRMINMAX(ddSrcLen);
DECLARE_VRMINMAX(decodeLimit);
DECLARE_VRMINMAX(checkEndMarkNow);
DECLARE_VRMINMAX(dummyRes);
DECLARE_VRMINMAX(numProbs);
DECLARE_VRMINMAX(ddProbIdx);
DECLARE_VRMINMAX(bufLimit);
DECLARE_VRMINMAX(ddRem);
DECLARE_VRMINMAX(lookAhead);
DECLARE_VRMINMAX(prSize);
DECLARE_VRMINMAX(prPos);
DECLARE_VRMINMAX(prGot);
DECLARE_VRMINMAX(izCount);
DECLARE_VRMINMAX(glPos);
DECLARE_VRMINMAX(ipByte);
DECLARE_VRMINMAX(wfDicPos);
DECLARE_VRMINMAX(wfGot);
DECLARE_VRMINMAX(checksumSize);
DECLARE_VRMINMAX(bhf);
DECLARE_VRMINMAX(dxRes);
DECLARE_VRMINMAX(readBufUS);
DECLARE_VRMINMAX(srcLen);
DECLARE_VRMINMAX(fromDicPos);
DECLARE_VRMINMAX(blockSizePad);
DECLARE_VRMINMAX(bhs);
DECLARE_VRMINMAX(bhs2);
DECLARE_VRMINMAX(dicSizeProp);
DECLARE_VRMINMAX(readAtBlock);
DECLARE_VRMINMAX(chunkUS);
DECLARE_VRMINMAX(chunkCS);
DECLARE_VRMINMAX(initDic);
DECLARE_VRMINMAX(control);
DECLARE_VRMINMAX(mode);
DECLARE_VRMINMAX(initState);
DECLARE_VRMINMAX(isProp);
DECLARE_VRMINMAX(deRes);
#endif /* CONFIG_DEBUG_VAR_RANGES */
#endif /* CONFIG_LANG_C */

/* For LZMA streams, LE_SMALL(lc + lp, 8 + 4), LE 12.
 * For LZMA2 streams, LE_SMALL(lc + lp, 4).
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

#ifdef CONFIG_LANG_JAVA
public class muaxzcat {
public static final boolean Lt32(int x, int y) {
  return (x < 0 ? y >= 0 : y < 0) ? y < 0 : x < y;
}
public static final boolean Ltx32(int x, int y) {
  return x < y && x >= 0;
}
#endif  /* CONFIG_LANG_JAVA */

#ifdef CONFIG_LANG_JAVA
public static void EnsureDicSize() {
  int newCapacity = dic8.length;
  while (newCapacity > 0 && LT_SMALL(newCapacity, GLOBAL_VAR(dicBufSize))) {
    newCapacity <<= 1;
  }
  if (newCapacity < 0 || LT_SMALL(DIC_ARRAY_SIZE, newCapacity)) {
    newCapacity = DIC_ARRAY_SIZE;
  }
  if (LT_SMALL(dic8.length, newCapacity)) {
    /*final*/ byte newDic[] = new byte[newCapacity];
    System.arraycopy(dic8, 0, newDic, 0, GLOBAL_VAR(dicPos));
    dic8 = newDic;
  }
}
#define ENSURE_DIC_SIZE() if (LT_SMALL(dic8.length, GLOBAL_VAR(dicBufSize))) EnsureDicSize()
#else
#define ENSURE_DIC_SIZE()
#endif  /* !CONFIG_LANG_JAVA */


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
FUNC_ARG1(UInt32, UndefToMinus1, UInt32, umValue)
  NOTICE_LOCAL_RANGE(umValue);
  return defined(LOCAL_VAR(umValue)) ? LOCAL_VAR(umValue) : -1;
ENDFUNC
#endif  /* CONFIG_LANG_PERL */

/* --- */

FUNC_ARG1(void, LzmaDec_WriteRem, UInt32, wrDicLimit)
  NOTICE_LOCAL_RANGE(wrDicLimit);
  if (NE_SMALL(GLOBAL_VAR(remainLen), 0) && LT_SMALL(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    LOCAL_INIT(UInt32, wrLen, GLOBAL_VAR(remainLen));
    if (LT_SMALL(LOCAL_VAR(wrDicLimit) - GLOBAL_VAR(dicPos), LOCAL_VAR(wrLen))) {
      SET_LOCALB(wrLen, 3, =, LOCAL_VAR(wrDicLimit) - GLOBAL_VAR(dicPos)) ;
    }
    if (EQ_SMALL(GLOBAL_VAR(checkDicSize), 0) && LE_SMALL(GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos), LOCAL_VAR(wrLen))) {
      SET_GLOBAL(checkDicSize, 2, =) GLOBAL_VAR(dicSize);
    }
    SET_GLOBAL(processedPos, 4, +=) LOCAL_VAR(wrLen);
    SET_GLOBAL(remainLen, 6, -=) LOCAL_VAR(wrLen);
    while (NE_SMALL(LOCAL_VAR(wrLen), 0)) {
      LOCAL_VAR(wrLen)--;
      SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
      GLOBAL_VAR(dicPos)++;
    }
  }
ENDFUNC

/* Modifies GLOBAL_VAR(bufCur) etc. */
FUNC_ARG2(SRes, LzmaDec_DecodeReal2, const UInt32, drDicLimit, const UInt32, drBufLimit)
  LOCAL_INIT(const UInt32, pbMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(pb))) - 1);
  LOCAL_INIT(const UInt32, lpMask, (ENSURE_32BIT(1) << (GLOBAL_VAR(lp))) - 1);
  LOCAL(UInt32, drI);
  NOTICE_LOCAL_RANGE(drDicLimit);
  NOTICE_LOCAL_RANGE(drBufLimit);
  do {
    LOCAL_INIT(const UInt32, drDicLimit2, EQ_SMALL(GLOBAL_VAR(checkDicSize), 0) && LT_SMALL(GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos), LOCAL_VAR(drDicLimit) - GLOBAL_VAR(dicPos)) ? GLOBAL_VAR(dicPos) + (GLOBAL_VAR(dicSize) - GLOBAL_VAR(processedPos)) : LOCAL_VAR(drDicLimit));
    SET_GLOBAL(remainLen, 8, =) 0;
    do {
      LOCAL(UInt32, drProbIdx);
      LOCAL(UInt32, drBound);
      LOCAL(UInt32, drTtt);  /* 0 <= LOCAL_VAR(drTtt) <= kBitModelTotal. */
      LOCAL(UInt32, distance);
      LOCAL_INIT(UInt32, drPosState, GLOBAL_VAR(processedPos) & LOCAL_VAR(pbMask));

      SET_LOCALB(drProbIdx, 5, =, IsMatch + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(drPosState)) ;
      SET_LOCALB(drTtt, 7, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 9, <<=) (8) ; SET_GLOBAL(code, 11, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 13, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
      if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
        LOCAL(UInt32, drSymbol);
        ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal));
        SET_GLOBAL(range, 15, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + (SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)));
        SET_LOCALB(drProbIdx, 17, =, Literal) ;
        if (NE_SMALL(GLOBAL_VAR(checkDicSize), 0) || NE_SMALL(GLOBAL_VAR(processedPos), 0)) {
          SET_LOCALB(drProbIdx, 19, +=, (LZMA_LIT_SIZE * (((GLOBAL_VAR(processedPos) & LOCAL_VAR(lpMask)) << GLOBAL_VAR(lc)) + SHR_SMALLX(GET_ARY8(dic, (EQ_SMALL(GLOBAL_VAR(dicPos), 0) ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), GLOBAL_VAR(lcm8))))) ;
        }
        if (LT_SMALL(GLOBAL_VAR(state), kNumLitStates)) {
          SET_GLOBAL(state, 8, -=) (LT_SMALL(GLOBAL_VAR(state), 4)) ? GLOBAL_VAR(state) : 3;
          SET_LOCALB(drSymbol, 21, =, 1) ;
          do {
            SET_LOCALB(drTtt, 23, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drSymbol))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 25, <<=) (8) ; SET_GLOBAL(code, 27, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 29, =, (SHR11(GLOBAL_VAR(range))) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 31, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drSymbol), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drSymbol, 33, =, (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol))); } else { SET_GLOBAL(range, 35, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 37, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drSymbol), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drSymbol, 39, =, (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol)) + 1); }
          } while (LT_SMALL(LOCAL_VAR(drSymbol), 0x100));
        } else {
          LOCAL_INIT(UInt32, drMatchByte, GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
          LOCAL_INIT(UInt32, drMatchMask, 0x100);  /* 0 or 0x100. */
          SET_GLOBAL(state, 10, -=) LT_SMALL(GLOBAL_VAR(state), 10) ? 3 : 6;
          SET_LOCALB(drSymbol, 41, =, 1) ;
          do {
            LOCAL(UInt32, drBit);
            LOCAL(UInt32, drProbLitIdx);
            ASSERT(LOCAL_VAR(drMatchMask) == 0 || LOCAL_VAR(drMatchMask) == 0x100);
            SET_LOCALB(drMatchByte, 43, <<=, 1) ;
            SET_LOCALB(drBit, 45, =, (LOCAL_VAR(drMatchByte) & LOCAL_VAR(drMatchMask))) ;
            SET_LOCALB(drProbLitIdx, 47, =, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drMatchMask) + LOCAL_VAR(drBit) + LOCAL_VAR(drSymbol)) ;
            SET_LOCALB(drTtt, 49, =, GET_ARY16(probs, LOCAL_VAR(drProbLitIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 51, <<=) (8) ; SET_GLOBAL(code, 53, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 55, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 57, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLitIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drSymbol, 59, =, (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol))) ; SET_LOCALB(drMatchMask, 61, &=, ~LOCAL_VAR(drBit)) ; } else { SET_GLOBAL(range, 63, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 65, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLitIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drSymbol, 67, =, (LOCAL_VAR(drSymbol) + LOCAL_VAR(drSymbol)) + 1) ; SET_LOCALB(drMatchMask, 69, &=, LOCAL_VAR(drBit)) ; }
          } while (LT_SMALL(LOCAL_VAR(drSymbol), 0x100));
        }
        SET_ARY8(dic, GLOBAL_VAR(dicPos)++, LOCAL_VAR(drSymbol));
        GLOBAL_VAR(processedPos)++;
        DEFAULT_CONTINUE_TO(continue_do2);
      } else {
        SET_GLOBAL(range, 71, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 73, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
        SET_LOCALB(drProbIdx, 75, =, IsRep + GLOBAL_VAR(state)) ;
        SET_LOCALB(drTtt, 77, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 79, <<=) (8) ; SET_GLOBAL(code, 81, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 83, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
        if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
          ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal));
          SET_GLOBAL(range, 85, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
          SET_GLOBAL(state, 12, +=) kNumStates;
          SET_LOCALB(drProbIdx, 87, =, LenCoder) ;
        } else {
          SET_GLOBAL(range, 89, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 91, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
          if (EQ_SMALL(GLOBAL_VAR(checkDicSize), 0) && EQ_SMALL(GLOBAL_VAR(processedPos), 0)) {
            return SZ_ERROR_DATA;
          }
          SET_LOCALB(drProbIdx, 93, =, IsRepG0 + GLOBAL_VAR(state)) ;
          SET_LOCALB(drTtt, 95, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 97, <<=) (8) ; SET_GLOBAL(code, 99, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 101, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
          if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
            ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal));
            SET_GLOBAL(range, 103, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
            SET_LOCALB(drProbIdx, 105, =, IsRep0Long + (GLOBAL_VAR(state) << (kNumPosBitsMax)) + LOCAL_VAR(drPosState)) ;
            SET_LOCALB(drTtt, 107, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 109, <<=) (8) ; SET_GLOBAL(code, 111, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 113, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
            if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
              SET_GLOBAL(range, 115, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
              SET_ARY8(dic, GLOBAL_VAR(dicPos), GET_ARY8(dic, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
              GLOBAL_VAR(dicPos)++;
              GLOBAL_VAR(processedPos)++;
              SET_GLOBAL(state, 14, =) LT_SMALL(GLOBAL_VAR(state), kNumLitStates) ? 9 : 11;
              DEFAULT_CONTINUE_TO(continue_do2);
            }
            SET_GLOBAL(range, 117, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 119, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
          } else {
            SET_GLOBAL(range, 121, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 123, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
            SET_LOCALB(drProbIdx, 125, =, IsRepG1 + GLOBAL_VAR(state)) ;
            SET_LOCALB(drTtt, 127, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 129, <<=) (8) ; SET_GLOBAL(code, 131, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 133, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
            if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
              SET_GLOBAL(range, 135, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
              SET_LOCALB(distance, 137, =, GLOBAL_VAR(rep1)) ;
            } else {
              SET_GLOBAL(range, 139, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 141, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
              SET_LOCALB(drProbIdx, 143, =, IsRepG2 + GLOBAL_VAR(state)) ;
              SET_LOCALB(drTtt, 145, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 147, <<=) (8) ; SET_GLOBAL(code, 149, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 151, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
              if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
                SET_GLOBAL(range, 153, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
                SET_LOCALB(distance, 155, =, GLOBAL_VAR(rep2)) ;
              } else {
                SET_GLOBAL(range, 157, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 159, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
                SET_LOCALB(distance, 161, =, GLOBAL_VAR(rep3)) ;
                SET_GLOBAL(rep3, 16, =) GLOBAL_VAR(rep2);
              }
              SET_GLOBAL(rep2, 18, =) GLOBAL_VAR(rep1);
            }
            SET_GLOBAL(rep1, 20, =) GLOBAL_VAR(rep0);
            SET_GLOBAL(rep0, 22, =) LOCAL_VAR(distance);
          }
          SET_GLOBAL(state, 24, =) LT_SMALL(GLOBAL_VAR(state), kNumLitStates) ? 8 : 11;
          SET_LOCALB(drProbIdx, 163, =, RepLenCoder) ;
        }
        {
          LOCAL(UInt32, drLimitSub);
          LOCAL(UInt32, drOffset);
          LOCAL_INIT(UInt32, drProbLenIdx, LOCAL_VAR(drProbIdx) + LenChoice);
          SET_LOCALB(drTtt, 165, =, GET_ARY16(probs, LOCAL_VAR(drProbLenIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 167, <<=) (8) ; SET_GLOBAL(code, 169, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 171, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
          if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
            ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal));
            SET_GLOBAL(range, 173, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLenIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
            SET_LOCALB(drProbLenIdx, 175, =, LOCAL_VAR(drProbIdx) + LenLow + (LOCAL_VAR(drPosState) << (kLenNumLowBits))) ;
            SET_LOCALB(drOffset, 177, =, 0) ;
            SET_LOCALB(drLimitSub, 179, =, (ENSURE_32BIT(1) << (kLenNumLowBits))) ;
          } else {
            SET_GLOBAL(range, 181, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 183, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLenIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
            SET_LOCALB(drProbLenIdx, 185, =, LOCAL_VAR(drProbIdx) + LenChoice2) ;
            SET_LOCALB(drTtt, 187, =, GET_ARY16(probs, LOCAL_VAR(drProbLenIdx))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 189, <<=) (8) ; SET_GLOBAL(code, 191, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 193, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ;
            if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) {
              ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal));
              SET_GLOBAL(range, 195, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLenIdx), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5));
              SET_LOCALB(drProbLenIdx, 197, =, LOCAL_VAR(drProbIdx) + LenMid + (LOCAL_VAR(drPosState) << (kLenNumMidBits))) ;
              SET_LOCALB(drOffset, 199, =, kLenNumLowSymbols) ;
              SET_LOCALB(drLimitSub, 201, =, ENSURE_32BIT(1) << (kLenNumMidBits)) ;
            } else {
              SET_GLOBAL(range, 203, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 205, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbLenIdx), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5));
              SET_LOCALB(drProbLenIdx, 207, =, LOCAL_VAR(drProbIdx) + LenHigh) ;
              SET_LOCALB(drOffset, 209, =, kLenNumLowSymbols + kLenNumMidSymbols) ;
              SET_LOCALB(drLimitSub, 211, =, ENSURE_32BIT(1) << (kLenNumHighBits)) ;
            }
          }
          {
            SET_GLOBAL(remainLen, 213, =) (1) ;
            do {
              { SET_LOCALB(drTtt, 215, =, GET_ARY16(probs, (LOCAL_VAR(drProbLenIdx) + GLOBAL_VAR(remainLen)))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 217, <<=) (8) ; SET_GLOBAL(code, 219, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 221, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 223, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, (LOCAL_VAR(drProbLenIdx) + GLOBAL_VAR(remainLen)), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_GLOBAL(remainLen, 225, =) ((GLOBAL_VAR(remainLen) + GLOBAL_VAR(remainLen))); } else { SET_GLOBAL(range, 227, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 229, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, (LOCAL_VAR(drProbLenIdx) + GLOBAL_VAR(remainLen)), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_GLOBAL(remainLen, 231, =) ((GLOBAL_VAR(remainLen) + GLOBAL_VAR(remainLen)) + 1); } }
            } while (LT_SMALL(GLOBAL_VAR(remainLen), LOCAL_VAR(drLimitSub)));
            SET_GLOBAL(remainLen, 233, -=) (LOCAL_VAR(drLimitSub)) ;
          }
          SET_GLOBAL(remainLen, 235, +=) (LOCAL_VAR(drOffset)) ;
        }

        if (GE_SMALL(GLOBAL_VAR(state), kNumStates)) {
          SET_LOCALB(drProbIdx, 237, =, PosSlotCode + (ENSURE_32BIT(LT_SMALL(GLOBAL_VAR(remainLen), kNumLenToPosStates) ? GLOBAL_VAR(remainLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits))) ;
          {
            SET_LOCALB(distance, 239, =, 1) ;
            do {
              { SET_LOCALB(drTtt, 241, =, GET_ARY16(probs, (LOCAL_VAR(drProbIdx) + LOCAL_VAR(distance)))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 243, <<=) (8) ; SET_GLOBAL(code, 245, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 247, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 249, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, (LOCAL_VAR(drProbIdx) + LOCAL_VAR(distance)), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(distance, 251, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance))); } else { SET_GLOBAL(range, 253, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 255, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, (LOCAL_VAR(drProbIdx) + LOCAL_VAR(distance)), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(distance, 257, =, (LOCAL_VAR(distance) + LOCAL_VAR(distance)) + 1); } }
            } while (LT_SMALL(LOCAL_VAR(distance), (1 << 6)));
            SET_LOCALB(distance, 259, -=, (1 << 6)) ;
          }
          ASSERT(IS_SMALL(LOCAL_VAR(distance)) && LT_SMALL(LOCAL_VAR(distance), 64));
          if (GE_SMALL(LOCAL_VAR(distance), kStartPosModelIndex)) {
            LOCAL_INIT(const UInt32, drPosSlot, LOCAL_VAR(distance));
            LOCAL_INIT(UInt32, drDirectBitCount, SHR_SMALLX(LOCAL_VAR(distance), 1) - 1);
            SET_LOCALB(distance, 261, =, (2 | (LOCAL_VAR(distance) & 1))) ;
            if (LT_SMALL(LOCAL_VAR(drPosSlot), kEndPosModelIndex)) {
              SET_LOCALB(distance, 263, <<=, LOCAL_VAR(drDirectBitCount)) ;
              SET_LOCALB(drProbIdx, 265, =, SpecPos + LOCAL_VAR(distance) - LOCAL_VAR(drPosSlot) - 1) ;
              {
                LOCAL_INIT(UInt32, mask, 1);
                SET_LOCALB(drI, 2651, =, 1);
                do {
                  SET_LOCALB(drTtt, 267, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI))) ; if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 269, <<=) (8) ; SET_GLOBAL(code, 271, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 273, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 275, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 277, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI))); } else { SET_GLOBAL(range, 279, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 281, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 283, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI)) + 1) ; SET_LOCALB(distance, 285, |=, LOCAL_VAR(mask)) ; }
                  SET_LOCALB(mask, 287, <<=, 1) ;
                } while (NE_SMALL(--LOCAL_VAR(drDirectBitCount), 0));
              }
            } else {
              SET_LOCALB(drDirectBitCount, 289, -=, kNumAlignBits) ;
              do {
                if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 291, <<=) (8) ; SET_GLOBAL(code, 293, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; }
                /* Here GLOBAL_VAR(range) can be non-small, so we can't use SHR_SMALLX instead of SHR1. */
                SET_GLOBAL(range, 2951, =) (SHR1(GLOBAL_VAR(range)));
                if (TO_BOOL((GLOBAL_VAR(code) - GLOBAL_VAR(range)) & 0x80000000)) {
                  SET_LOCALB(distance, 297, <<=, 1);
                } else {
                  SET_GLOBAL(code, 295, -=) (GLOBAL_VAR(range));
                  /* This won't be faster in Perl: <<= 1, ++ */
                  SET_LOCALB(distance, 301, =, (LOCAL_VAR(distance) << 1) + 1);
                }
              } while (NE_SMALL(--LOCAL_VAR(drDirectBitCount), 0));
              SET_LOCALB(drProbIdx, 303, =, Align) ;
              SET_LOCALB(distance, 305, <<=, kNumAlignBits) ;
              {
                SET_LOCALB(drI, 3051, =, 1);
                SET_LOCALB(drTtt, 307, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI))); ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal)); if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 309, <<=) (8) ; SET_GLOBAL(code, 311, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 313, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 315, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 317, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI))); } else { SET_GLOBAL(range, 319, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 321, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 323, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI)) + 1) ; SET_LOCALB(distance, 325, |=, 1) ; }
                SET_LOCALB(drTtt, 327, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI))); ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal)); if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 329, <<=) (8) ; SET_GLOBAL(code, 331, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 333, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 335, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 337, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI))); } else { SET_GLOBAL(range, 339, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 341, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 343, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI)) + 1) ; SET_LOCALB(distance, 345, |=, 2) ; }
                SET_LOCALB(drTtt, 347, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI))); ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal)); if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 349, <<=) (8) ; SET_GLOBAL(code, 351, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 353, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 355, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 357, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI))); } else { SET_GLOBAL(range, 359, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 361, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 363, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI)) + 1) ; SET_LOCALB(distance, 365, |=, 4) ; }
                SET_LOCALB(drTtt, 367, =, GET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI))); ASSERT(IS_SMALL(LOCAL_VAR(drTtt)) && LE_SMALL(LOCAL_VAR(drTtt), kBitModelTotal)); if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 369, <<=) (8) ; SET_GLOBAL(code, 371, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; } SET_LOCALB(drBound, 373, =, SHR11(GLOBAL_VAR(range)) * LOCAL_VAR(drTtt)) ; if (LT(GLOBAL_VAR(code), LOCAL_VAR(drBound))) { SET_GLOBAL(range, 375, =) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) + SHR_SMALLX(kBitModelTotal - LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 377, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI))); } else { SET_GLOBAL(range, 379, -=) (LOCAL_VAR(drBound)) ; SET_GLOBAL(code, 381, -=) (LOCAL_VAR(drBound)) ; SET_ARY16(probs, LOCAL_VAR(drProbIdx) + LOCAL_VAR(drI), LOCAL_VAR(drTtt) - SHR_SMALLX(LOCAL_VAR(drTtt), 5)); SET_LOCALB(drI, 383, =, (LOCAL_VAR(drI) + LOCAL_VAR(drI)) + 1) ; SET_LOCALB(distance, 385, |=, 8) ; }
              }
              if (EQ0(~LOCAL_VAR(distance))) {
                SET_GLOBAL(remainLen, 387, +=) (kMatchSpecLenStart) ;
                SET_GLOBAL(state, 26, -=) kNumStates;
                DEFAULT_BREAK_TO(break_do2);
              }
            }
          }
          /* TODO(pts): Do the 2 instances of SZ_ERROR_DATA below also check this? */
          ASSERT(IS_SMALL(LOCAL_VAR(distance)) && LE_SMALL(LOCAL_VAR(distance), DIC_ARRAY_SIZE));
          SET_GLOBAL(rep3, 28, =) GLOBAL_VAR(rep2);
          SET_GLOBAL(rep2, 30, =) GLOBAL_VAR(rep1);
          SET_GLOBAL(rep1, 32, =) GLOBAL_VAR(rep0);
          SET_GLOBAL(rep0, 34, =) LOCAL_VAR(distance) + 1;
          if (EQ_SMALL(GLOBAL_VAR(checkDicSize), 0)) {
            if (GE_SMALL(LOCAL_VAR(distance), GLOBAL_VAR(processedPos))) {
              return SZ_ERROR_DATA;
            }
          } else {
            if (GE_SMALL(LOCAL_VAR(distance), GLOBAL_VAR(checkDicSize))) {
              return SZ_ERROR_DATA;
            }
          }
          SET_GLOBAL(state, 36, =) LT_SMALL(GLOBAL_VAR(state), kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        }

        SET_GLOBAL(remainLen, 389, +=) (kMatchMinLen) ;

        if (EQ_SMALL(LOCAL_VAR(drDicLimit2), GLOBAL_VAR(dicPos))) {
          return SZ_ERROR_DATA;
        }
        {
          LOCAL_INIT(UInt32, drRem, LOCAL_VAR(drDicLimit2) - GLOBAL_VAR(dicPos));
          LOCAL_INIT(UInt32, curLen, (LT_SMALL(LOCAL_VAR(drRem), GLOBAL_VAR(remainLen)) ? LOCAL_VAR(drRem) : GLOBAL_VAR(remainLen)));
          LOCAL_INIT(UInt32, pos, (GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0)) + (LT_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0));

          SET_GLOBAL(processedPos, 38, +=) LOCAL_VAR(curLen);

          SET_GLOBAL(remainLen, 391, -=) (LOCAL_VAR(curLen)) ;
          if (LE_SMALL(LOCAL_VAR(pos) + LOCAL_VAR(curLen), GLOBAL_VAR(dicBufSize))) {
            ASSERT(GT_SMALL(GLOBAL_VAR(dicPos), LOCAL_VAR(pos)));
            ASSERT(GT_SMALL(LOCAL_VAR(curLen), 0));
            do {
              /* Here pos can be negative if 64-bit. */
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
            } while (NE_SMALL(--LOCAL_VAR(curLen), 0));
          } else {
            do {
              SET_ARY8(dic, GLOBAL_VAR(dicPos)++, GET_ARY8(dic, LOCAL_VAR(pos)++));
              if (EQ_SMALL(LOCAL_VAR(pos), GLOBAL_VAR(dicBufSize))) { SET_LOCALB(pos, 393, =, 0) ; }
            } while (NE_SMALL(--LOCAL_VAR(curLen), 0));
          }
        }
      }
     DEFAULT_LABEL(continue_do2)
    } while (LT_SMALL(GLOBAL_VAR(dicPos), LOCAL_VAR(drDicLimit2)) && LT_SMALL(GLOBAL_VAR(bufCur), LOCAL_VAR(drBufLimit)));
    DEFAULT_LABEL(break_do2)
    if (LTX(GLOBAL_VAR(range), kTopValue)) { SET_GLOBAL(range, 395, <<=) (8) ; SET_GLOBAL(code, 397, =) ((GLOBAL_VAR(code) << 8) | (GET_ARY8(readBuf, GLOBAL_VAR(bufCur)++))) ; }
    if (GE_SMALL(GLOBAL_VAR(processedPos), GLOBAL_VAR(dicSize))) {
      SET_GLOBAL(checkDicSize, 46, =) GLOBAL_VAR(dicSize);
    }
    LzmaDec_WriteRem(LOCAL_VAR(drDicLimit));
  } while (LT_SMALL(GLOBAL_VAR(dicPos), LOCAL_VAR(drDicLimit)) && LT_SMALL(GLOBAL_VAR(bufCur), LOCAL_VAR(drBufLimit)) && LT_SMALL(GLOBAL_VAR(remainLen), kMatchSpecLenStart));

  if (GT_SMALL(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    SET_GLOBAL(remainLen, 48, =) kMatchSpecLenStart;
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG2(Byte, LzmaDec_TryDummy, UInt32, tdCur, const UInt32, tdBufLimit)
  LOCAL_INIT(UInt32, tdRange, GLOBAL_VAR(range));
  LOCAL_INIT(UInt32, tdCode, GLOBAL_VAR(code));
  LOCAL_INIT(UInt32, tdState, GLOBAL_VAR(state));
  LOCAL(Byte, tdRes);
  LOCAL(UInt32, tdProbIdx);
  LOCAL(UInt32, tdBound);
  LOCAL(UInt32, tdTtt);
  LOCAL_INIT(UInt32, tdPosState, (GLOBAL_VAR(processedPos)) & ((1 << GLOBAL_VAR(pb)) - 1));
  NOTICE_LOCAL_RANGE(tdCur);
  NOTICE_LOCAL_RANGE(tdBufLimit);
  SET_LOCALB(tdProbIdx, 399, =, IsMatch + (LOCAL_VAR(tdState) << (kNumPosBitsMax)) + LOCAL_VAR(tdPosState)) ;
  SET_LOCALB(tdTtt, 401, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 403, <<=, 8) ; SET_LOCALB(tdCode, 405, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 407, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
  if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
    LOCAL_INIT(UInt32, tdSymbol, 1);
    SET_LOCALB(tdRange, 409, =, LOCAL_VAR(tdBound)) ;
    SET_LOCALB(tdProbIdx, 411, =, Literal) ;
    if (NE_SMALL(GLOBAL_VAR(checkDicSize), 0) || NE_SMALL(GLOBAL_VAR(processedPos), 0)) {
      SET_LOCALB(tdProbIdx, 413, +=, (LZMA_LIT_SIZE * ((((GLOBAL_VAR(processedPos)) & ((1 << (GLOBAL_VAR(lp))) - 1)) << GLOBAL_VAR(lc)) + SHR_SMALLX(GET_ARY8(dic, (EQ_SMALL(GLOBAL_VAR(dicPos), 0) ? GLOBAL_VAR(dicBufSize) : GLOBAL_VAR(dicPos)) - 1), GLOBAL_VAR(lcm8))))) ;
    }

    if (LT_SMALL(LOCAL_VAR(tdState), kNumLitStates)) {
      do {
        SET_LOCALB(tdTtt, 415, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx) + LOCAL_VAR(tdSymbol))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 417, <<=, 8) ; SET_LOCALB(tdCode, 419, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 421, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ; if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) { SET_LOCALB(tdRange, 423, =, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdSymbol, 425, =, (LOCAL_VAR(tdSymbol) + LOCAL_VAR(tdSymbol))); } else { SET_LOCALB(tdRange, 427, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 429, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdSymbol, 431, =, (LOCAL_VAR(tdSymbol) + LOCAL_VAR(tdSymbol)) + 1); }
      } while (LT_SMALL(LOCAL_VAR(tdSymbol), 0x100));
    } else {
      LOCAL_INIT(UInt32, tdMatchByte, GET_ARY8(dic, GLOBAL_VAR(dicPos) - GLOBAL_VAR(rep0) + (LT_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(rep0)) ? GLOBAL_VAR(dicBufSize) : 0)));
      LOCAL_INIT(UInt32, tdMatchMask, 0x100);  /* 0 or 0x100. */
      do {
        LOCAL(UInt32, tdBit);
        LOCAL(UInt32, tdProbLitIdx);
        ASSERT(LOCAL_VAR(tdMatchMask) == 0 || LOCAL_VAR(tdMatchMask) == 0x100);
        SET_LOCALB(tdMatchByte, 433, <<=, 1) ;
        SET_LOCALB(tdBit, 435, =, (LOCAL_VAR(tdMatchByte) & LOCAL_VAR(tdMatchMask))) ;
        SET_LOCALB(tdProbLitIdx, 437, =, LOCAL_VAR(tdProbIdx) + LOCAL_VAR(tdMatchMask) + LOCAL_VAR(tdBit) + LOCAL_VAR(tdSymbol)) ;
        SET_LOCALB(tdTtt, 439, =, GET_ARY16(probs, LOCAL_VAR(tdProbLitIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 441, <<=, 8) ; SET_LOCALB(tdCode, 443, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 445, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ; if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) { SET_LOCALB(tdRange, 447, =, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdSymbol, 449, =, (LOCAL_VAR(tdSymbol) + LOCAL_VAR(tdSymbol))) ; SET_LOCALB(tdMatchMask, 451, &=, ~LOCAL_VAR(tdBit)) ; } else { SET_LOCALB(tdRange, 453, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 455, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdSymbol, 457, =, (LOCAL_VAR(tdSymbol) + LOCAL_VAR(tdSymbol)) + 1) ; SET_LOCALB(tdMatchMask, 459, &=, LOCAL_VAR(tdBit)) ; }
      } while (LT_SMALL(LOCAL_VAR(tdSymbol), 0x100));
    }
    SET_LOCALB(tdRes, 461, =, DUMMY_LIT) ;
  } else {
    LOCAL(UInt32, tdLen);
    SET_LOCALB(tdRange, 463, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 465, -=, LOCAL_VAR(tdBound)) ;
    SET_LOCALB(tdProbIdx, 467, =, IsRep + LOCAL_VAR(tdState)) ;
    SET_LOCALB(tdTtt, 469, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 471, <<=, 8) ; SET_LOCALB(tdCode, 473, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 475, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
    if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
      SET_LOCALB(tdRange, 477, =, LOCAL_VAR(tdBound)) ;
      SET_LOCALB(tdState, 479, =, 0) ;
      SET_LOCALB(tdProbIdx, 481, =, LenCoder) ;
      SET_LOCALB(tdRes, 483, =, DUMMY_MATCH) ;
    } else {
      SET_LOCALB(tdRange, 485, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 487, -=, LOCAL_VAR(tdBound)) ;
      SET_LOCALB(tdRes, 489, =, DUMMY_REP) ;
      SET_LOCALB(tdProbIdx, 491, =, IsRepG0 + LOCAL_VAR(tdState)) ;
      SET_LOCALB(tdTtt, 493, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 495, <<=, 8) ; SET_LOCALB(tdCode, 497, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 499, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
      if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
        SET_LOCALB(tdRange, 501, =, LOCAL_VAR(tdBound)) ;
        SET_LOCALB(tdProbIdx, 503, =, IsRep0Long + (LOCAL_VAR(tdState) << (kNumPosBitsMax)) + LOCAL_VAR(tdPosState)) ;
        SET_LOCALB(tdTtt, 505, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 507, <<=, 8) ; SET_LOCALB(tdCode, 509, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 511, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
        if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
          SET_LOCALB(tdRange, 513, =, LOCAL_VAR(tdBound)) ;
          if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 515, <<=, 8) ; SET_LOCALB(tdCode, 517, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; }
          return DUMMY_REP;
        } else {
          SET_LOCALB(tdRange, 519, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 521, -=, LOCAL_VAR(tdBound)) ;
        }
      } else {
        SET_LOCALB(tdRange, 523, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 525, -=, LOCAL_VAR(tdBound)) ;
        SET_LOCALB(tdProbIdx, 527, =, IsRepG1 + LOCAL_VAR(tdState)) ;
        SET_LOCALB(tdTtt, 529, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 531, <<=, 8) ; SET_LOCALB(tdCode, 533, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 535, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
        if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
          SET_LOCALB(tdRange, 537, =, LOCAL_VAR(tdBound)) ;
        } else {
          SET_LOCALB(tdRange, 539, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 541, -=, LOCAL_VAR(tdBound)) ;
          SET_LOCALB(tdProbIdx, 543, =, IsRepG2 + LOCAL_VAR(tdState)) ;
          SET_LOCALB(tdTtt, 545, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 547, <<=, 8) ; SET_LOCALB(tdCode, 549, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 551, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
          if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
            SET_LOCALB(tdRange, 553, =, LOCAL_VAR(tdBound)) ;
          } else {
            SET_LOCALB(tdRange, 555, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 557, -=, LOCAL_VAR(tdBound)) ;
          }
        }
      }
      SET_LOCALB(tdState, 559, =, kNumStates) ;
      SET_LOCALB(tdProbIdx, 561, =, RepLenCoder) ;
    }
    {
      LOCAL(UInt32, tdLimitSub);
      LOCAL(UInt32, tdOffset);
      LOCAL_INIT(UInt32, tdProbLenIdx, LOCAL_VAR(tdProbIdx) + LenChoice);
      SET_LOCALB(tdTtt, 563, =, GET_ARY16(probs, LOCAL_VAR(tdProbLenIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 565, <<=, 8) ; SET_LOCALB(tdCode, 567, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 569, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
      if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
        SET_LOCALB(tdRange, 571, =, LOCAL_VAR(tdBound)) ;
        SET_LOCALB(tdProbLenIdx, 573, =, LOCAL_VAR(tdProbIdx) + LenLow + (LOCAL_VAR(tdPosState) << (kLenNumLowBits))) ;
        SET_LOCALB(tdOffset, 575, =, 0) ;
        SET_LOCALB(tdLimitSub, 577, =, ENSURE_32BIT(1) << (kLenNumLowBits)) ;
      } else {
        SET_LOCALB(tdRange, 579, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 581, -=, LOCAL_VAR(tdBound)) ;
        SET_LOCALB(tdProbLenIdx, 583, =, LOCAL_VAR(tdProbIdx) + LenChoice2) ;
        SET_LOCALB(tdTtt, 585, =, GET_ARY16(probs, LOCAL_VAR(tdProbLenIdx))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 587, <<=, 8) ; SET_LOCALB(tdCode, 589, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 591, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ;
        if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) {
          SET_LOCALB(tdRange, 593, =, LOCAL_VAR(tdBound)) ;
          SET_LOCALB(tdProbLenIdx, 595, =, LOCAL_VAR(tdProbIdx) + LenMid + (LOCAL_VAR(tdPosState) << (kLenNumMidBits))) ;
          SET_LOCALB(tdOffset, 597, =, kLenNumLowSymbols) ;
          SET_LOCALB(tdLimitSub, 599, =, ENSURE_32BIT(1) << (kLenNumMidBits)) ;
        } else {
          SET_LOCALB(tdRange, 601, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 603, -=, LOCAL_VAR(tdBound)) ;
          SET_LOCALB(tdProbLenIdx, 605, =, LOCAL_VAR(tdProbIdx) + LenHigh) ;
          SET_LOCALB(tdOffset, 607, =, kLenNumLowSymbols + kLenNumMidSymbols) ;
          SET_LOCALB(tdLimitSub, 609, =, ENSURE_32BIT(1) << (kLenNumHighBits)) ;
        }
      }
      {
        SET_LOCALB(tdLen, 611, =, 1) ;
        do {
          SET_LOCALB(tdTtt, 613, =, GET_ARY16(probs, LOCAL_VAR(tdProbLenIdx) + LOCAL_VAR(tdLen))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 615, <<=, 8) ; SET_LOCALB(tdCode, 617, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 619, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ; if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) { SET_LOCALB(tdRange, 621, =, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdLen, 623, =, (LOCAL_VAR(tdLen) + LOCAL_VAR(tdLen))); } else { SET_LOCALB(tdRange, 625, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 627, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdLen, 629, =, (LOCAL_VAR(tdLen) + LOCAL_VAR(tdLen)) + 1); }
        } while (LT_SMALL(LOCAL_VAR(tdLen), LOCAL_VAR(tdLimitSub)));
        SET_LOCALB(tdLen, 631, -=, LOCAL_VAR(tdLimitSub)) ;
      }
      SET_LOCALB(tdLen, 633, +=, LOCAL_VAR(tdOffset)) ;
    }

    if (LT_SMALL(LOCAL_VAR(tdState), 4)) {
      LOCAL(UInt32, tdPosSlot);
      SET_LOCALB(tdProbIdx, 635, =, PosSlotCode + (ENSURE_32BIT(LT_SMALL(LOCAL_VAR(tdLen), kNumLenToPosStates) ? LOCAL_VAR(tdLen) : kNumLenToPosStates - 1) << (kNumPosSlotBits))) ;
      {
        SET_LOCALB(tdPosSlot, 637, =, 1) ;
        do {
          SET_LOCALB(tdTtt, 639, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx) + LOCAL_VAR(tdPosSlot))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 641, <<=, 8) ; SET_LOCALB(tdCode, 643, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 645, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ; if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) { SET_LOCALB(tdRange, 647, =, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdPosSlot, 649, =, (LOCAL_VAR(tdPosSlot) + LOCAL_VAR(tdPosSlot))); } else { SET_LOCALB(tdRange, 651, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 653, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdPosSlot, 655, =, (LOCAL_VAR(tdPosSlot) + LOCAL_VAR(tdPosSlot)) + 1); }
        } while (LT_SMALL(LOCAL_VAR(tdPosSlot), ENSURE_32BIT(1) << (kNumPosSlotBits)));
        SET_LOCALB(tdPosSlot, 657, -=, ENSURE_32BIT(1) << (kNumPosSlotBits)) ;
      }
      /* Small enough for SHR_SMALLX(LOCAL_VAR(tdPosSlot), ...). */
      ASSERT(IS_SMALL(LOCAL_VAR(tdPosSlot)) && LT_SMALL(LOCAL_VAR(tdPosSlot), ENSURE_32BIT(1) << (kNumPosSlotBits)));
      if (GE_SMALL(LOCAL_VAR(tdPosSlot), kStartPosModelIndex)) {
        LOCAL_INIT(UInt32, tdDirectBitCount, SHR_SMALLX(LOCAL_VAR(tdPosSlot), 1) - 1);
        if (LT_SMALL(LOCAL_VAR(tdPosSlot), kEndPosModelIndex)) {
          SET_LOCALB(tdProbIdx, 659, =, SpecPos + ((2 | (LOCAL_VAR(tdPosSlot) & 1)) << LOCAL_VAR(tdDirectBitCount)) - LOCAL_VAR(tdPosSlot) - 1) ;
        } else {
          SET_LOCALB(tdDirectBitCount, 661, -=, kNumAlignBits) ;
          do {
            if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 663, <<=, 8) ; SET_LOCALB(tdCode, 665, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; }
            SET_LOCALB(tdRange, 6651, =, SHR1(LOCAL_VAR(tdRange)));
            if (TO_BOOL_NEG((LOCAL_VAR(tdCode) - LOCAL_VAR(tdRange)) & 0x80000000)) {
              SET_LOCALB(tdCode, 667, -=, LOCAL_VAR(tdRange));
            }
          } while (NE_SMALL(--LOCAL_VAR(tdDirectBitCount), 0));
          SET_LOCALB(tdProbIdx, 669, =, Align) ;
          SET_LOCALB(tdDirectBitCount, 671, =, kNumAlignBits) ;
        }
        {
          LOCAL_INIT(UInt32, tdI, 1);
          do {
            SET_LOCALB(tdTtt, 673, =, GET_ARY16(probs, LOCAL_VAR(tdProbIdx) + LOCAL_VAR(tdI))) ; if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 675, <<=, 8) ; SET_LOCALB(tdCode, 677, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; } SET_LOCALB(tdBound, 679, =, SHR11(LOCAL_VAR(tdRange)) * LOCAL_VAR(tdTtt)) ; if (LT(LOCAL_VAR(tdCode), LOCAL_VAR(tdBound))) { SET_LOCALB(tdRange, 681, =, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdI, 683, =, (LOCAL_VAR(tdI) + LOCAL_VAR(tdI))); } else { SET_LOCALB(tdRange, 685, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdCode, 687, -=, LOCAL_VAR(tdBound)) ; SET_LOCALB(tdI, 689, =, (LOCAL_VAR(tdI) + LOCAL_VAR(tdI)) + 1); }
          } while (NE_SMALL(--LOCAL_VAR(tdDirectBitCount), 0));
        }
      }
    }
  }
  if (LTX(LOCAL_VAR(tdRange), kTopValue)) { if (GE_SMALL(LOCAL_VAR(tdCur), LOCAL_VAR(tdBufLimit))) { return DUMMY_ERROR; } SET_LOCALB(tdRange, 691, <<=, 8) ; SET_LOCALB(tdCode, 693, =, (LOCAL_VAR(tdCode) << 8) | (GET_ARY8(readBuf, LOCAL_VAR(tdCur)++))) ; }
  return LOCAL_VAR(tdRes);
ENDFUNC

FUNC_ARG2(void, LzmaDec_InitDicAndState, const Bool, idInitDic, const Bool, idInitState)
  NOTICE_LOCAL_RANGE(idInitDic);
  NOTICE_LOCAL_RANGE(idInitState);
  SET_GLOBAL(needFlush, 50, =) TRUE;
  SET_GLOBAL(remainLen, 52, =) 0;
  SET_GLOBAL(tempBufSize, 54, =) 0;

  if (TO_BOOL(LOCAL_VAR(idInitDic))) {
    SET_GLOBAL(processedPos, 56, =) 0;
    SET_GLOBAL(checkDicSize, 58, =) 0;
    SET_GLOBAL(needInitLzma, 60, =) TRUE;
  }
  if (TO_BOOL(LOCAL_VAR(idInitState))) {
    SET_GLOBAL(needInitLzma, 62, =) TRUE;
  }
ENDFUNC

/* Decompress LZMA stream in
 * readBuf8[GLOBAL_VAR(readCur) : GLOBAL_VAR(readCur) + LOCAL_VAR(ddSrcLen)].
 * On success (and on some errors as well), adds LOCAL_VAR(ddSrcLen) to GLOBAL_VAR(readCur).
 */
FUNC_ARG1(SRes, LzmaDec_DecodeToDic, const UInt32, ddSrcLen)
  /* Index limit in GLOBAL_VAR(readBuf). */
  LOCAL_INIT(const UInt32, decodeLimit, GLOBAL_VAR(readCur) + LOCAL_VAR(ddSrcLen));
  LOCAL(Bool, checkEndMarkNow);
  LOCAL(SRes, dummyRes);
  NOTICE_LOCAL_RANGE(ddSrcLen);
  LzmaDec_WriteRem(GLOBAL_VAR(dicBufSize));

  while (NE_SMALL(GLOBAL_VAR(remainLen), kMatchSpecLenStart)) {
    if (TO_BOOL(GLOBAL_VAR(needFlush))) {
      /* Read 5 bytes (RC_INIT_SIZE) to tempBuf, first of which must be
       * 0, initialize the range coder with the 4 bytes after the 0 byte.
       */
      while (GT_SMALL(LOCAL_VAR(decodeLimit), GLOBAL_VAR(readCur)) && LT_SMALL(GLOBAL_VAR(tempBufSize), RC_INIT_SIZE)) {
        SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
      }
      if (LT_SMALL(GLOBAL_VAR(tempBufSize), RC_INIT_SIZE)) {
#ifdef HAS_GOTO
       on_needs_more_input:
        if (NE_SMALL(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
        return SZ_ERROR_NEEDS_MORE_INPUT;
#endif
      }
      if (NE_SMALL(GET_ARY8(readBuf, READBUF_SIZE), 0)) {
        return SZ_ERROR_DATA;
      }
      SET_GLOBAL(code, 64, =) (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 1)) << 24) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 2)) << 16) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 3)) << 8) | (ENSURE_32BIT(GET_ARY8(readBuf, READBUF_SIZE + 4)));
      SET_GLOBAL(range, 66, =) 0xffffffff;
      SET_GLOBAL(needFlush, 68, =) FALSE;
      SET_GLOBAL(tempBufSize, 70, =) 0;
    }

    SET_LOCALB(checkEndMarkNow, 695, =, FALSE) ;
    if (GE_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize))) {
      if (EQ_SMALL(GLOBAL_VAR(remainLen), 0) && EQ0(GLOBAL_VAR(code))) {
        if (NE_SMALL(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) { return SZ_ERROR_CHUNK_NOT_CONSUMED; }
        return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
      }
      if (NE_SMALL(GLOBAL_VAR(remainLen), 0)) {
        return SZ_ERROR_NOT_FINISHED;
      }
      SET_LOCALB(checkEndMarkNow, 697, =, TRUE) ;
    }

    if (TO_BOOL(GLOBAL_VAR(needInitLzma))) {
      LOCAL_INIT(UInt32, numProbs, Literal + (ENSURE_32BIT(LZMA_LIT_SIZE) << (GLOBAL_VAR(lc) + GLOBAL_VAR(lp))));
      LOCAL(UInt32, ddProbIdx);
      for (LOCAL_VAR(ddProbIdx) = 0; LT_SMALL(LOCAL_VAR(ddProbIdx), LOCAL_VAR(numProbs)); LOCAL_VAR(ddProbIdx)++) {
        SET_ARY16(probs, LOCAL_VAR(ddProbIdx), SHR_SMALLX(kBitModelTotal, 1));
      }
      SET_GLOBAL(rep0, 72, =) SET_GLOBAL(rep1, 74, =) SET_GLOBAL(rep2, 76, =) SET_GLOBAL(rep3, 78, =) 1;
      SET_GLOBAL(state, 80, =) 0;
      SET_GLOBAL(needInitLzma, 82, =) FALSE;
    }

    if (EQ_SMALL(GLOBAL_VAR(tempBufSize), 0)) {
      LOCAL(UInt32, bufLimit);
      if (LT_SMALL(LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur), LZMA_REQUIRED_INPUT_MAX) || TO_BOOL(LOCAL_VAR(checkEndMarkNow))) {
        SET_LOCALB(dummyRes, 699, =, LzmaDec_TryDummy(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) ;
        if (EQ_SMALL(LOCAL_VAR(dummyRes), DUMMY_ERROR)) {
          /* This line can be triggered by passing LOCAL_VAR(ddSrcLen)=1 to LzmaDec_DecodeToDic. */
          SET_GLOBAL(tempBufSize, 84, =) 0;
          while (NE_SMALL(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) {
            SET_ARY8(readBuf, READBUF_SIZE + GLOBAL_VAR(tempBufSize)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur)++));
          }
#ifdef HAS_GOTO
          goto on_needs_more_input;
#else
          if (NE_SMALL(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
          return SZ_ERROR_NEEDS_MORE_INPUT;
#endif
        }
        if (TO_BOOL(LOCAL_VAR(checkEndMarkNow)) && NE_SMALL(LOCAL_VAR(dummyRes), DUMMY_MATCH)) {
          return SZ_ERROR_NOT_FINISHED;
        }
        SET_LOCALB(bufLimit, 701, =, GLOBAL_VAR(readCur)) ;
      } else {
        SET_LOCALB(bufLimit, 703, =, LOCAL_VAR(decodeLimit) - LZMA_REQUIRED_INPUT_MAX) ;
      }
      SET_GLOBAL(bufCur, 86, =) GLOBAL_VAR(readCur);
      if (NE_SMALL(LzmaDec_DecodeReal2(GLOBAL_VAR(dicBufSize), LOCAL_VAR(bufLimit)), SZ_OK)) {
        return SZ_ERROR_DATA;
      }
      SET_GLOBAL(readCur, 88, =) GLOBAL_VAR(bufCur);
    } else {
      LOCAL_INIT(UInt32, ddRem, GLOBAL_VAR(tempBufSize));
      LOCAL_INIT(UInt32, lookAhead, 0);
      while (LT_SMALL(LOCAL_VAR(ddRem), LZMA_REQUIRED_INPUT_MAX) && LT_SMALL(LOCAL_VAR(lookAhead), LOCAL_VAR(decodeLimit) - GLOBAL_VAR(readCur))) {
        SET_ARY8(readBuf, READBUF_SIZE + LOCAL_VAR(ddRem)++, GET_ARY8(readBuf, GLOBAL_VAR(readCur) + LOCAL_VAR(lookAhead)++));
      }
      SET_GLOBAL(tempBufSize, 90, =) LOCAL_VAR(ddRem);
      if (LT_SMALL(LOCAL_VAR(ddRem), LZMA_REQUIRED_INPUT_MAX) || TO_BOOL(LOCAL_VAR(checkEndMarkNow))) {
        SET_LOCALB(dummyRes, 705, =, LzmaDec_TryDummy(READBUF_SIZE, READBUF_SIZE + LOCAL_VAR(ddRem))) ;
        if (EQ_SMALL(LOCAL_VAR(dummyRes), DUMMY_ERROR)) {
          SET_GLOBAL(readCur, 92, +=) LOCAL_VAR(lookAhead);
#ifdef HAS_GOTO
          goto on_needs_more_input;
#else
          if (NE_SMALL(GLOBAL_VAR(readCur), LOCAL_VAR(decodeLimit))) { return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL; }
          return SZ_ERROR_NEEDS_MORE_INPUT;
#endif
        }
        if (TO_BOOL(LOCAL_VAR(checkEndMarkNow)) && NE_SMALL(LOCAL_VAR(dummyRes), DUMMY_MATCH)) {
          return SZ_ERROR_NOT_FINISHED;
        }
      }
      /* This line can be triggered by passing LOCAL_VAR(ddSrcLen)=1 to LzmaDec_DecodeToDic. */
      SET_GLOBAL(bufCur, 94, =) READBUF_SIZE;  /* tempBuf. */
      if (NE_SMALL(LzmaDec_DecodeReal2(0, READBUF_SIZE), SZ_OK)) {
        return SZ_ERROR_DATA;
      }
      SET_LOCALB(lookAhead, 707, -=, LOCAL_VAR(ddRem) - (GLOBAL_VAR(bufCur) - READBUF_SIZE)) ;
      SET_GLOBAL(readCur, 96, +=) LOCAL_VAR(lookAhead);
      SET_GLOBAL(tempBufSize, 98, =) 0;
    }
  }
  if (NE0(GLOBAL_VAR(code))) { return SZ_ERROR_DATA; }
  return SZ_ERROR_FINISHED_WITH_MARK;
ENDFUNC

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
FUNC_ARG1(UInt32, Preread, const UInt32, prSize)
#ifdef CONFIG_LANG_JAVA
  try {
#endif  /* CONFIG_LANG_JAVA */
  LOCAL_INIT(UInt32, prPos, GLOBAL_VAR(readEnd) - GLOBAL_VAR(readCur));
  LOCAL(UInt32, prGot);
  NOTICE_LOCAL_RANGE(prSize);
  ASSERT(LE_SMALL(LOCAL_VAR(prSize), READBUF_SIZE));
  if (LT_SMALL(LOCAL_VAR(prPos), LOCAL_VAR(prSize))) {  /* Not enough pending available. */
    if (LT_SMALL(READBUF_SIZE - GLOBAL_VAR(readCur), LOCAL_VAR(prSize))) {
      /* If no room for LOCAL_VAR(prSize) bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", ENSURE_32BIT(LOCAL_VAR(prPos)));
      for (SET_GLOBAL(readEnd, 100, =) 0; LT_SMALL(GLOBAL_VAR(readEnd), LOCAL_VAR(prPos)); ++GLOBAL_VAR(readEnd)) {
        SET_ARY8(readBuf, GLOBAL_VAR(readEnd), GET_ARY8(readBuf, GLOBAL_VAR(readCur) + GLOBAL_VAR(readEnd)));
      }
      SET_GLOBAL(readCur, 102, =) 0;
    }
    while (LT_SMALL(LOCAL_VAR(prPos), LOCAL_VAR(prSize))) {
      /* Instead of (LOCAL_VAR(prSize) - LOCAL_VAR(prPos)) we could use (GLOBAL_VAR(readBuf) + READBUF_SIZE -
       * GLOBAL_VAR(readEnd)) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", ENSURE_32BIT(LOCAL_VAR(prSize) - LOCAL_VAR(prPos)));
      SET_LOCALB(prGot, 7071, =, READ_FROM_STDIN_TO_ARY8(readBuf, GLOBAL_VAR(readEnd), LOCAL_VAR(prSize) - LOCAL_VAR(prPos)));
      if (LE_SMALL(LOCAL_VAR(prGot) + 1, 1)) { BREAK; }  /* EOF or error on input. */
      SET_GLOBAL(readEnd, 104, +=) LOCAL_VAR(prGot);
      SET_LOCALB(prPos, 709, +=, LOCAL_VAR(prGot)) ;
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", ENSURE_32BIT(LOCAL_VAR(prSize)), ENSURE_32BIT(LOCAL_VAR(prPos)));
  return LOCAL_VAR(prPos);
#ifdef CONFIG_LANG_JAVA
  } catch (java.io.IOException e) {
    return SZ_ERROR_READ;
  }
#endif  /* CONFIG_LANG_JAVA */
ENDFUNC

FUNC_ARG0(void, IgnoreVarint)
  while (GE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), 0x80)) {}
ENDFUNC

FUNC_ARG1(SRes, IgnoreZeroBytes, UInt32, izCount)
  NOTICE_LOCAL_RANGE(izCount);
  for (; NE_SMALL(LOCAL_VAR(izCount), 0); --LOCAL_VAR(izCount)) {
    if (NE_SMALL(GET_ARY8(readBuf, GLOBAL_VAR(readCur)++), 0)) {
      return SZ_ERROR_BAD_PADDING;
    }
  }
  return SZ_OK;
ENDFUNC

FUNC_ARG1(UInt32, GetLE4, const UInt32, glPos)
  NOTICE_LOCAL_RANGE(glPos);
  return GET_ARY8(readBuf, LOCAL_VAR(glPos)) | GET_ARY8(readBuf, LOCAL_VAR(glPos) + 1) << 8 | GET_ARY8(readBuf, LOCAL_VAR(glPos) + 2) << 16 | GET_ARY8(readBuf, LOCAL_VAR(glPos) + 3) << 24;
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

FUNC_ARG1(SRes, InitProp, Byte, ipByte)
  NOTICE_LOCAL_RANGE(ipByte);
  if (GE_SMALL(LOCAL_VAR(ipByte), 9 * 5 * 5)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  SET_GLOBAL(lc, 122, =) LOCAL_VAR(ipByte) % 9;
  SET_GLOBAL(lcm8, 1222, =) 8 - GLOBAL_VAR(lc);
  SET_LOCALB(ipByte, 711, /=, 9) ;
  SET_GLOBAL(pb, 124, =) LOCAL_VAR(ipByte) / 5;
  SET_GLOBAL(lp, 126, =) LOCAL_VAR(ipByte) % 5;
  if (GT_SMALL(GLOBAL_VAR(lc) + GLOBAL_VAR(lp), LZMA2_LCLP_MAX)) { return SZ_ERROR_BAD_LCLPPB_PROP; }
  SET_GLOBAL(needInitProp, 128, =) FALSE;
  return SZ_OK;
ENDFUNC

/* Writes uncompressed data dic[LOCAL_VAR(fromDicPos) : GLOBAL_VAR(dicPos)] to stdout. */
FUNC_ARG1(SRes, WriteFrom, UInt32, wfDicPos)
  NOTICE_LOCAL_RANGE(wfDicPos);
  DEBUGF("WRITE %d dicPos=%d\n", ENSURE_32BIT(GLOBAL_VAR(dicPos) - LOCAL_VAR(wfDicPos)), ENSURE_32BIT(GLOBAL_VAR(dicPos)));
#ifdef CONFIG_LANG_JAVA
  WRITE_TO_STDOUT_FROM_ARY8(dic, LOCAL_VAR(wfDicPos), GLOBAL_VAR(dicPos) - LOCAL_VAR(wfDicPos));
#else
  while (NE_SMALL(LOCAL_VAR(wfDicPos), GLOBAL_VAR(dicPos))) {
    LOCAL_INIT(UInt32, wfGot, WRITE_TO_STDOUT_FROM_ARY8(dic, LOCAL_VAR(wfDicPos), GLOBAL_VAR(dicPos) - LOCAL_VAR(wfDicPos)));
    if (LOCAL_VAR(wfGot) & 0x80000000) { return SZ_ERROR_WRITE; }
    SET_LOCALB(wfDicPos, 713, +=, LOCAL_VAR(wfGot)) ;
  }
#endif
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
  LOCAL(SRes, dxRes);

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
        (EQ0((LOCAL_VAR(bhf) = GetLE4(GLOBAL_VAR(readCur) + 9))) || EQ0(~LOCAL_VAR(bhf))) &&
        GE_SMALL((SET_GLOBAL(dicSize, 130, =) GetLE4(GLOBAL_VAR(readCur) + 1)), LZMA_DIC_MIN) &&
        LTX(GLOBAL_VAR(dicSize), DIC_ARRAY_SIZE + 1)) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    LOCAL(UInt32, readBufUS);
    LOCAL(UInt32, srcLen);
    LOCAL(UInt32, fromDicPos);
    InitDecode();
    /* LZMA restricts LE_SMALL(lc + lp, 4). LZMA requires LE_SMALL(lc + lp,
     * 12). We apply the LZMA2 restriction here (to save memory in
     * GLOBAL_VAR(probs)), thus we are not able to extract some legitimate
     * .lzma files.
     */
    if (NE_SMALL((LOCAL_VAR(dxRes) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur)))), SZ_OK)) {
      return LOCAL_VAR(dxRes);
    }
    if (EQ0(LOCAL_VAR(bhf))) {
      SET_GLOBAL(dicBufSize, 132, =) LOCAL_VAR(readBufUS) = GetLE4(GLOBAL_VAR(readCur) + 5);
      if (!LTX(LOCAL_VAR(readBufUS), DIC_ARRAY_SIZE + 1)) { return SZ_ERROR_MEM; }
    } else {
      SET_LOCALB(readBufUS, 715, =, LOCAL_VAR(bhf)) ;  /* max UInt32. */
      /* !! Don't preallocate DIC_BUF_SIZE in Java in ENSURE_DIC_SIZE below. */
      SET_GLOBAL(dicBufSize, 134, =) DIC_ARRAY_SIZE;
    }
    ENSURE_DIC_SIZE();
    SET_GLOBAL(readCur, 136, +=) 13;  /* Start decompressing the 0 byte. */
    DEBUGF("LZMA dicSize=0x%x us=%d bhf=%d\n", ENSURE_32BIT(GLOBAL_VAR(dicSize)), ENSURE_32BIT(LOCAL_VAR(readBufUS)), ENSURE_32BIT(LOCAL_VAR(bhf)));
    /* TODO(pts): Limit on uncompressed size unless 8 bytes of -1 is
     * specified.
     */
    /* Any Preread(...) amount starting from 1 works here, but higher values
     * are faster.
     */
    while (NE_SMALL((LOCAL_VAR(srcLen) = Preread(READBUF_SIZE)), 0)) {
      SET_LOCALB(fromDicPos, 717, =, GLOBAL_VAR(dicPos)) ;
      SET_LOCALB(dxRes, 719, =, LzmaDec_DecodeToDic(LOCAL_VAR(srcLen))) ;
      DEBUGF("LZMADEC dxRes=%d\n", ENSURE_32BIT(LOCAL_VAR(dxRes)));
      if (LTX(LOCAL_VAR(readBufUS), GLOBAL_VAR(dicPos))) { SET_GLOBAL(dicPos, 138, =) LOCAL_VAR(readBufUS); }
      if (NE_SMALL((LOCAL_VAR(dxRes) = WriteFrom(LOCAL_VAR(fromDicPos))), SZ_OK)) { return LOCAL_VAR(dxRes); }
      if (EQ_SMALL(LOCAL_VAR(dxRes), SZ_ERROR_FINISHED_WITH_MARK)) { BREAK; }
      if (NE_SMALL(LOCAL_VAR(dxRes), SZ_ERROR_NEEDS_MORE_INPUT) && NE_SMALL(LOCAL_VAR(dxRes), SZ_OK)) { return LOCAL_VAR(dxRes); }
      if (EQ0(GLOBAL_VAR(dicPos) - LOCAL_VAR(readBufUS))) { BREAK; }
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
    SET_LOCALB(readAtBlock, 7191, =, GLOBAL_VAR(readCur)) ;
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
    if (NE_SMALL((LOCAL_VAR(dxRes) = IgnoreZeroBytes(LOCAL_VAR(bhs) - LOCAL_VAR(bhs2))), SZ_OK)) { return LOCAL_VAR(dxRes); }
    SET_GLOBAL(readCur, 144, +=) 4;  /* Ignore CRC32. */
    /* Typically it's LOCAL_VAR(offset) 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2\n");
    {  /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      LOCAL(UInt32, chunkUS);  /* Uncompressed chunk sizes. */
      LOCAL(UInt32, chunkCS);  /* Compressed chunk size. */
      LOCAL(Bool, initDic);
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
          SET_LOCALB(initDic, 7311, =, BOOL_TO_INT(EQ_SMALL(LOCAL_VAR(control), 1)));
          SET_LOCALB(chunkCS, 733, =, LOCAL_VAR(chunkUS)) ;
          SET_GLOBAL(readCur, 146, +=) 3;
          /* TODO(pts): Porting: TRUNCATE_TO_8BIT(LOCAL_VAR(blockSizePad)) for Python and other unlimited-integer-range languages. */
          LOCAL_VAR(blockSizePad) -= 3;
          if (TO_BOOL(LOCAL_VAR(initDic))) {
            SET_GLOBAL(needInitProp, 148, =) SET_GLOBAL(needInitState, 150, =) TRUE;
            SET_GLOBAL(needInitDic, 152, =) FALSE;
          } ELSE_IF (TO_BOOL(GLOBAL_VAR(needInitDic))) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(LOCAL_VAR(initDic), FALSE);
        } else {  /* LZMA chunk. */
          LOCAL_INIT(const Byte, mode, (SHR_SMALLX((LOCAL_VAR(control)), 5) & 3));
          LOCAL_INIT(const Bool, initState, BOOL_TO_INT(NE_SMALL(LOCAL_VAR(mode), 0)));
          LOCAL_INIT(const Bool, isProp, BOOL_TO_INT(NE_SMALL((LOCAL_VAR(control) & 64), 0)));
          SET_LOCALB(initDic, 7331, =, BOOL_TO_INT(EQ_SMALL(LOCAL_VAR(mode), 3)));
          SET_LOCALB(chunkUS, 735, +=, (LOCAL_VAR(control) & 31) << 16) ;
          SET_LOCALB(chunkCS, 737, =, (GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 3) << 8) + GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 4) + 1) ;
          if (TO_BOOL(LOCAL_VAR(isProp))) {
            if (NE_SMALL((LOCAL_VAR(dxRes) = InitProp(GET_ARY8(readBuf, GLOBAL_VAR(readCur) + 5))), SZ_OK)) {
              return LOCAL_VAR(dxRes);
            }
            ++GLOBAL_VAR(readCur);
            --LOCAL_VAR(blockSizePad);
          } else {
            if (TO_BOOL(GLOBAL_VAR(needInitProp))) { return SZ_ERROR_MISSING_INITPROP; }
          }
          SET_GLOBAL(readCur, 154, +=) 5;
          SET_LOCALB(blockSizePad, 739, -=, 5) ;
          if ((TO_BOOL_NEG(LOCAL_VAR(initDic)) && TO_BOOL(GLOBAL_VAR(needInitDic))) || (TO_BOOL_NEG(LOCAL_VAR(initState)) && TO_BOOL(GLOBAL_VAR(needInitState)))) {
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
        ENSURE_DIC_SIZE();
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
          if (NE_SMALL((LOCAL_VAR(dxRes) = LzmaDec_DecodeToDic(LOCAL_VAR(chunkCS))), SZ_OK)) { return LOCAL_VAR(dxRes); }
        }
        if (NE_SMALL(GLOBAL_VAR(dicPos), GLOBAL_VAR(dicBufSize))) { return SZ_ERROR_BAD_DICPOS; }
        if (NE_SMALL((LOCAL_VAR(dxRes) = WriteFrom(GLOBAL_VAR(dicPos) - LOCAL_VAR(chunkUS))), SZ_OK)) { return LOCAL_VAR(dxRes); }
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
    if (NE_SMALL((LOCAL_VAR(dxRes) = IgnoreZeroBytes(LOCAL_VAR(blockSizePad) & 3)), SZ_OK)) { return LOCAL_VAR(dxRes); }
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
#ifndef CONFIG_DEBUG_VAR_RANGES
  return DecompressXzOrLzma();
#else
  return ({ const SRes mainRes = DecompressXzOrLzma();
#define DUMP_VRMINMAX(name) DEBUGF("VRMINMAX %s min=%lld max=%lld %s\n", #name, (long long)VRMIN_##name, (long long)VRMAX_##name, VRMIN_##name >= 0 && VRMAX_##name <= 0x7fffffff ? "small" : "big")
  /* TODO(pts): Don't enumerate these again. */
  NOTICE_GLOBAL_RANGE(bufCur);
  NOTICE_GLOBAL_RANGE(dicSize);
  NOTICE_GLOBAL_RANGE(range);
  NOTICE_GLOBAL_RANGE(code);
  NOTICE_GLOBAL_RANGE(dicPos);
  NOTICE_GLOBAL_RANGE(dicBufSize);
  NOTICE_GLOBAL_RANGE(processedPos);
  NOTICE_GLOBAL_RANGE(checkDicSize);
  NOTICE_GLOBAL_RANGE(state);
  NOTICE_GLOBAL_RANGE(rep0);
  NOTICE_GLOBAL_RANGE(rep1);
  NOTICE_GLOBAL_RANGE(rep2);
  NOTICE_GLOBAL_RANGE(rep3);
  NOTICE_GLOBAL_RANGE(remainLen);
  NOTICE_GLOBAL_RANGE(tempBufSize);
  NOTICE_GLOBAL_RANGE(readCur);
  NOTICE_GLOBAL_RANGE(readEnd);
  NOTICE_GLOBAL_RANGE(needFlush);
  NOTICE_GLOBAL_RANGE(needInitLzma);
  NOTICE_GLOBAL_RANGE(needInitDic);
  NOTICE_GLOBAL_RANGE(needInitState);
  NOTICE_GLOBAL_RANGE(needInitProp);
  NOTICE_GLOBAL_RANGE(lc);
  NOTICE_GLOBAL_RANGE(lp);
  NOTICE_GLOBAL_RANGE(pb);
  NOTICE_GLOBAL_RANGE(lcm8);
  /* TODO(pts): Don't enumerate these again, reuse DECLARE_VRMINMAX */
  DUMP_VRMINMAX(bhf);
  DUMP_VRMINMAX(bhs);
  DUMP_VRMINMAX(bhs2);
  DUMP_VRMINMAX(blockSizePad);
  DUMP_VRMINMAX(bufCur);
  DUMP_VRMINMAX(bufLimit);
  DUMP_VRMINMAX(checkDicSize);
  DUMP_VRMINMAX(checkEndMarkNow);
  DUMP_VRMINMAX(checksumSize);
  DUMP_VRMINMAX(chunkCS);
  DUMP_VRMINMAX(chunkUS);
  DUMP_VRMINMAX(code);
  DUMP_VRMINMAX(control);
  DUMP_VRMINMAX(curLen);
  DUMP_VRMINMAX(ddProbIdx);
  DUMP_VRMINMAX(ddRem);
  DUMP_VRMINMAX(ddSrcLen);
  DUMP_VRMINMAX(deRes);
  DUMP_VRMINMAX(decodeLimit);
  DUMP_VRMINMAX(dicBufSize);
  DUMP_VRMINMAX(dicPos);
  DUMP_VRMINMAX(dicSize);
  DUMP_VRMINMAX(dicSizeProp);
  DUMP_VRMINMAX(distance);
  DUMP_VRMINMAX(drBit);
  DUMP_VRMINMAX(drBound);
  DUMP_VRMINMAX(drBufLimit);
  DUMP_VRMINMAX(drDicLimit);
  DUMP_VRMINMAX(drDicLimit2);
  DUMP_VRMINMAX(drDirectBitCount);
  DUMP_VRMINMAX(drI);
  DUMP_VRMINMAX(drLimitSub);
  DUMP_VRMINMAX(drMatchByte);
  DUMP_VRMINMAX(drMatchMask);
  DUMP_VRMINMAX(drOffset);
  DUMP_VRMINMAX(drPosSlot);
  DUMP_VRMINMAX(drPosState);
  DUMP_VRMINMAX(drProbIdx);
  DUMP_VRMINMAX(drProbLenIdx);
  DUMP_VRMINMAX(drProbLitIdx);
  DUMP_VRMINMAX(drRem);
  DUMP_VRMINMAX(drSymbol);
  DUMP_VRMINMAX(drTtt);
  DUMP_VRMINMAX(dummyRes);
  DUMP_VRMINMAX(dxRes);
  DUMP_VRMINMAX(fromDicPos);
  DUMP_VRMINMAX(glPos);
  DUMP_VRMINMAX(idInitDic);
  DUMP_VRMINMAX(idInitState);
  DUMP_VRMINMAX(initDic);
  DUMP_VRMINMAX(initState);
  DUMP_VRMINMAX(ipByte);
  DUMP_VRMINMAX(isProp);
  DUMP_VRMINMAX(izCount);
  DUMP_VRMINMAX(lc);
  DUMP_VRMINMAX(lcm8);
  DUMP_VRMINMAX(lookAhead);
  DUMP_VRMINMAX(lp);
  DUMP_VRMINMAX(lpMask);
  DUMP_VRMINMAX(mask);
  DUMP_VRMINMAX(mode);
  DUMP_VRMINMAX(needFlush);
  DUMP_VRMINMAX(needInitDic);
  DUMP_VRMINMAX(needInitLzma);
  DUMP_VRMINMAX(needInitProp);
  DUMP_VRMINMAX(needInitState);
  DUMP_VRMINMAX(numProbs);
  DUMP_VRMINMAX(pb);
  DUMP_VRMINMAX(pbMask);
  DUMP_VRMINMAX(pos);
  DUMP_VRMINMAX(prGot);
  DUMP_VRMINMAX(prPos);
  DUMP_VRMINMAX(prSize);
  DUMP_VRMINMAX(processedPos);
  DUMP_VRMINMAX(range);
  DUMP_VRMINMAX(readAtBlock);
  DUMP_VRMINMAX(readBufUS);
  DUMP_VRMINMAX(readCur);
  DUMP_VRMINMAX(readEnd);
  DUMP_VRMINMAX(remainLen);
  DUMP_VRMINMAX(rep0);
  DUMP_VRMINMAX(rep1);
  DUMP_VRMINMAX(rep2);
  DUMP_VRMINMAX(rep3);
  DUMP_VRMINMAX(srcLen);
  DUMP_VRMINMAX(state);
  DUMP_VRMINMAX(tdBit);
  DUMP_VRMINMAX(tdBound);
  DUMP_VRMINMAX(tdBufLimit);
  DUMP_VRMINMAX(tdCode);
  DUMP_VRMINMAX(tdCur);
  DUMP_VRMINMAX(tdDirectBitCount);
  DUMP_VRMINMAX(tdI);
  DUMP_VRMINMAX(tdLen);
  DUMP_VRMINMAX(tdLimitSub);
  DUMP_VRMINMAX(tdMatchByte);
  DUMP_VRMINMAX(tdMatchMask);
  DUMP_VRMINMAX(tdOffset);
  DUMP_VRMINMAX(tdPosSlot);
  DUMP_VRMINMAX(tdPosState);
  DUMP_VRMINMAX(tdProbIdx);
  DUMP_VRMINMAX(tdProbLenIdx);
  DUMP_VRMINMAX(tdProbLitIdx);
  DUMP_VRMINMAX(tdRange);
  DUMP_VRMINMAX(tdRes);
  DUMP_VRMINMAX(tdState);
  DUMP_VRMINMAX(tdSymbol);
  DUMP_VRMINMAX(tdTtt);
  DUMP_VRMINMAX(tempBufSize);
  DUMP_VRMINMAX(umValue);
  DUMP_VRMINMAX(wfDicPos);
  DUMP_VRMINMAX(wfGot);
  DUMP_VRMINMAX(wrDicLimit);
  DUMP_VRMINMAX(wrLen);
  /**/
  mainRes; });
#endif  /* CONFIG_DEBUG_VAR_RANGES */
}
#endif  /* CONFIG_LANG_C */

#ifdef CONFIG_LANG_JAVA
public static void main(String args[]) {
  probs16 = new short[LZMA2_MAX_NUM_PROBS];  /* !! TODO(pts): Automatic size. */
  readBuf8 = new byte[READBUF_SIZE + LZMA_REQUIRED_INPUT_MAX];  /* !! TODO(pts): Automatic size. */
  dic8 = new byte[65536];
  System.exit(DecompressXzOrLzma());
}
}
#endif  /* CONFIG_LANG_JAVA */

#ifdef CONFIG_LANG_PERL
FUNC_ARG0(SRes, Decompress)
  LOCAL(SRes, deRes);
  CLEAR_ARY16(probs);
  CLEAR_ARY8(readBuf);
  CLEAR_ARY8(dic);
  binmode(STDIN);
  binmode(STDOUT);
  SET_LOCALB(deRes, 743, =, DecompressXzOrLzma()) ;
  CLEAR_ARY16(probs);
  CLEAR_ARY8(readBuf);
  CLEAR_ARY8(dic);
  return LOCAL_VAR(deRes);
ENDFUNC
#endif  /* CONFIG_LANG_PERL */
