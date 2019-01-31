/* by pts@fazekas.hu at Wed Jan 30 15:15:23 CET 2019
 *
 * Based on https://github.com/pts/pts-tiny-7z-sfx/commit/b9a101b076672879f861d472665afaa6caa6fec1
 *
 * Limitations of this decompressor:
 *
 * * It keeps both uncompressed data in memory, and it needs 120 KiB of
 *   memory on top of it: readBuf is about 64 KiB, CLzma2Dec.prob is about
 *   14 KiB, the rest decompressBuf (containing the entire uncompressed
 *   data) and a small constant overhead.
 * * It doesn't support decompressed data larger than 1610612736 (~1.61 GB).
 *   FYI linux-4.20.5.tar is about half as much, 854855680 bytes.
 * * It supports only LZMA2 (no other filters).
 * * It doesn't verify checksums.
 * * It extracts the first stream only, and ignores the index.
 * * It doesn't support dictionary sizes larger than 1610612736 bytes (~1.61 GB).
 *   (This is not a problem in practice, because even `xz -9e' generates
 *   only 64 MiB dictionary size.)
 * * !! Split DecodeLzma2 to per-chunk, thus limiting the compressed memory usage to 64 KiB.
 * * !! 32-bit sizes? Make it optional.
 *
 * Can use: -DCONFIG_DEBUG
 * Can use: -DCONFIG_NO_INT64
 * Can use: -DCONFIG_NO_SIZE_T
 * Can use: -DCONFIG_PROB32
 * Can use: -DCONFIG_SIZE_OPT  (!!How much smaller does the code become?)
 *
 * $ xtiny gcc -s -Os -W -Wall -Wextra -o muxzcat muxzcat.c && ls -l muxzcat
 * -rwxr-xr-x 1 pts pts 9672 Jan 30 20:05 muxzcat
 *
 * Examples:
 *
 *   # Smallest possible dictionary:
 *   $ xz --lzma2=preset=8,dict=4096 <ta8.tar >ta4k.tar.xz
 */

#ifdef __XTINY__

#include <xtiny.h>
typedef ssize_t ptrdiff_t;
typedef int64_t Int64;
typedef uint64_t UInt64;
typedef int32_t Int32;
typedef uint32_t UInt32;
#define UINT64_CONST(n) n ## ULL

/* !! TODO(pts): Replace this with inline assembly: rep movsb. */
static void MemmoveBackward(void *dest, const void *src, size_t n) {
  char *destCp = (char*)dest;
  char *srcCp = (char*)src;
  for (; n > 0; --n) {
    *destCp++ = *srcCp++;
  }
}

#else

#include <stddef.h>  /* size_t */
#include <string.h>  /* memcpy(), memmove() */
#include <unistd.h>  /* read(), write() */

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(MSDOS) || defined(_WIN32)
#include <fcntl.h>  /* setmode() */
#endif

#define MemmoveBackward(dest, src, n) memmove(dest, src, n)

#ifdef CONFIG_NO_INT64

typedef long Int64;
typedef unsigned long UInt64;
typedef int Int32;
typedef unsigned int Int64;

#else

#include <stdint.h>

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#define UINT64_CONST(n) n
typedef int Int32;
typedef unsigned int Int64;
#else
typedef int64_t Int64;
typedef uint64_t UInt64;
#define UINT64_CONST(n) n ## ULL
typedef int32_t Int32;
typedef uint32_t UInt32;
#endif

#endif

#endif  /* USE_MINIINC1 */

#ifdef CONFIG_DEBUG
/* This is guaranteed to work with Linux and gcc only. */
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
#define SZ_ERROR_PROGRESS 10
#define SZ_ERROR_FAIL 11
#define SZ_ERROR_THREAD 12

typedef int SRes;

#ifndef RINOK
#define RINOK(x) { int __result__ = (x); if (__result__ != 0) return __result__; }
#endif

typedef unsigned char Byte;
typedef short Int16;
typedef unsigned short UInt16;

#ifdef CONFIG_NO_SIZE_T
typedef UInt64 size_t;
typedef Int64 ssize_t;
#endif

typedef char Bool;
#define True 1
#define False 0

#define LZMA_REQUIRED_INPUT_MAX 20

typedef struct _CLzmaProps
{
  unsigned lc, lp, pb;
  UInt32 dicSize;
} CLzmaProps;

/* CONFIG_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case
   CONFIG_PROB32 increases memory usage by 28268 bytes.
   */
#ifdef CONFIG_PROB32
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

typedef struct
{
  CLzmaProps prop;
  CLzmaProb *probs;
  Byte *dic;
  const Byte *buf;
  UInt32 range, code;
  size_t dicPos;
  size_t dicBufSize;  /* !! Change these to UInt32. */
  UInt32 processedPos;
  UInt32 checkDicSize;
  unsigned state;
  UInt32 reps[4];
  unsigned remainLen;
  int needFlush;  /* !! Change these to Bool. */
  int needInitState;
  UInt32 numProbs;
  unsigned tempBufSize;
  Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
} CLzmaDec;

/* There are two types of LZMA streams:
     0) Stream with end mark. That end mark adds about 6 bytes to compressed size.
     1) Stream without end mark. You must know exact uncompressed size to decompress such stream. */
typedef enum
{
  LZMA_FINISH_ANY,   /* finish at any point */
  LZMA_FINISH_END    /* block must be finished at the end */
} ELzmaFinishMode;

typedef enum
{
  LZMA_STATUS_NOT_SPECIFIED,               /* use main error code instead */
  LZMA_STATUS_FINISHED_WITH_MARK,          /* stream was finished with end mark. */
  LZMA_STATUS_NOT_FINISHED,                /* stream was not finished */
  LZMA_STATUS_NEEDS_MORE_INPUT,            /* you must provide more input bytes */
  LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK  /* there is probability that stream was finished without end mark */
} ELzmaStatus;

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768
#define LZMA2_LCLP_MAX 4

/* For LZMA streams, lc + lp <= 8 + 4 <= 12.
 * For LZMA2 streams, lc + lp <= 4.
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: sizeof(CLzmaProb) * value == (2 or 4) * value bytes.
 */
#define LzmaProps_GetNumProbs(p) ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))
/* 14134 */
#define Lzma2Props_GetMaxNumProbs() ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << LZMA2_LCLP_MAX))

typedef struct
{
  CLzmaDec decoder;
  Bool needInitDic;
  Bool needInitState;
  Bool needInitProp;
  CLzmaProb probs[Lzma2Props_GetMaxNumProbs()];
} CLzma2Dec;

/* --- */

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_INIT_SIZE 5

#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
  { UPDATE_0(p); i = (i + i); A0; } else \
  { UPDATE_1(p); i = (i + i) + 1; A1; }
#define GET_BIT(p, i) GET_BIT2(p, i, ; , ;)

#define TREE_GET_BIT(probs, i) { GET_BIT((probs + i), i); }
#define TREE_DECODE(probs, limit, i) \
  { i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

#ifdef CONFIG_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
  { i = 1; \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  TREE_GET_BIT(probs, i); \
  i -= 0x40; }
#endif

#define NORMALIZE_CHECK if (range < kTopValue) { if (buf >= bufLimit) return DUMMY_ERROR; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK; bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) \
  { UPDATE_0_CHECK; i = (i + i); A0; } else \
  { UPDATE_1_CHECK; i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ; , ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
  { i = 1; do { GET_BIT_CHECK(probs + i, i) } while (i < limit); i -= limit; }


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumMidBits 3
#define kLenNumMidSymbols (1 << kLenNumMidBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenChoice 0
#define LenChoice2 (LenChoice + 1)
#define LenLow (LenChoice2 + 1)
#define LenMid (LenLow + (kNumPosStatesMax << kLenNumLowBits))
#define LenHigh (LenMid + (kNumPosStatesMax << kLenNumMidBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)


#define kNumStates 12
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols + kLenNumMidSymbols + kLenNumHighSymbols)

#define IsMatch 0
#define IsRep (IsMatch + (kNumStates << kNumPosBitsMax))
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define IsRep0Long (IsRepG2 + kNumStates)
#define PosSlot (IsRep0Long + (kNumStates << kNumPosBitsMax))
#define SpecPos (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define Align (SpecPos + kNumFullDistances - kEndPosModelIndex)
#define LenCoder (Align + kAlignTableSize)
#define RepLenCoder (LenCoder + kNumLenProbs)
#define Literal (RepLenCoder + kNumLenProbs)

#if Literal != LZMA_BASE_SIZE
#error StopCompilingDueBUG
#endif

#define LZMA_DIC_MIN (1 << 12)

static int LzmaDec_DecodeReal(CLzmaDec *p, size_t limit, const Byte *bufLimit)
{
  CLzmaProb *probs = p->probs;

  unsigned state = p->state;
  UInt32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
  unsigned pbMask = ((unsigned)1 << (p->prop.pb)) - 1;
  unsigned lpMask = ((unsigned)1 << (p->prop.lp)) - 1;
  unsigned lc = p->prop.lc;

  Byte *dic = p->dic;
  size_t dicBufSize = p->dicBufSize;
  size_t dicPos = p->dicPos;

  UInt32 processedPos = p->processedPos;
  UInt32 checkDicSize = p->checkDicSize;
  unsigned len = 0;

  const Byte *buf = p->buf;
  UInt32 range = p->range;
  UInt32 code = p->code;

  do
  {
    CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = processedPos & pbMask;

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0(prob)
    {
      unsigned symbol;
      UPDATE_0(prob);
      prob = probs + Literal;
      if (checkDicSize != 0 || processedPos != 0)
        prob += (LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) +
        (dic[(dicPos == 0 ? dicBufSize : dicPos) - 1] >> (8 - lc))));

      if (state < kNumLitStates)
      {
        state -= (state < 4) ? state : 3;
        symbol = 1;
        do { GET_BIT(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        unsigned matchByte = p->dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
        unsigned offs = 0x100;
        state -= (state < 10) ? 3 : 6;
        symbol = 1;
        do
        {
          unsigned bit;
          CLzmaProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLit = prob + offs + bit + symbol;
          GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)
        }
        while (symbol < 0x100);
      }
      dic[dicPos++] = (Byte)symbol;
      processedPos++;
      continue;
    }
    else
    {
      UPDATE_1(prob);
      prob = probs + IsRep + state;
      IF_BIT_0(prob)
      {
        UPDATE_0(prob);
        state += kNumStates;
        prob = probs + LenCoder;
      }
      else
      {
        UPDATE_1(prob);
        if (checkDicSize == 0 && processedPos == 0)
          return SZ_ERROR_DATA;
        prob = probs + IsRepG0 + state;
        IF_BIT_0(prob)
        {
          UPDATE_0(prob);
          prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob);
            dic[dicPos] = dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
            dicPos++;
            processedPos++;
            state = state < kNumLitStates ? 9 : 11;
            continue;
          }
          UPDATE_1(prob);
        }
        else
        {
          UInt32 distance;
          UPDATE_1(prob);
          prob = probs + IsRepG1 + state;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob);
            distance = rep1;
          }
          else
          {
            UPDATE_1(prob);
            prob = probs + IsRepG2 + state;
            IF_BIT_0(prob)
            {
              UPDATE_0(prob);
              distance = rep2;
            }
            else
            {
              UPDATE_1(prob);
              distance = rep3;
              rep3 = rep2;
            }
            rep2 = rep1;
          }
          rep1 = rep0;
          rep0 = distance;
        }
        state = state < kNumLitStates ? 8 : 11;
        prob = probs + RepLenCoder;
      }
      {
        unsigned limit, offset;
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen);
          probLen = prob + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          limit = (1 << kLenNumLowBits);
        }
        else
        {
          UPDATE_1(probLen);
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen);
            probLen = prob + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            limit = (1 << kLenNumMidBits);
          }
          else
          {
            UPDATE_1(probLen);
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            limit = (1 << kLenNumHighBits);
          }
        }
        TREE_DECODE(probLen, limit, len);
        len += offset;
      }

      if (state >= kNumStates)
      {
        UInt32 distance;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
        TREE_6_DECODE(prob, distance);
        if (distance >= kStartPosModelIndex)
        {
          unsigned posSlot = (unsigned)distance;
          int numDirectBits = (int)(((distance >> 1) - 1));
          distance = (2 | (distance & 1));
          if (posSlot < kEndPosModelIndex)
          {
            distance <<= numDirectBits;
            prob = probs + SpecPos + distance - posSlot - 1;
            {
              UInt32 mask = 1;
              unsigned i = 1;
              do
              {
                GET_BIT2(prob + i, i, ; , distance |= mask);
                mask <<= 1;
              }
              while (--numDirectBits != 0);
            }
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE
              range >>= 1;

              {
                UInt32 t;
                code -= range;
                t = (0 - ((UInt32)code >> 31)); /* (UInt32)((Int32)code >> 31) */
                distance = (distance << 1) + (t + 1);
                code += range & t;
              }
              /*
              distance <<= 1;
              if (code >= range)
              {
                code -= range;
                distance |= 1;
              }
              */
            }
            while (--numDirectBits != 0);
            prob = probs + Align;
            distance <<= kNumAlignBits;
            {
              unsigned i = 1;
              GET_BIT2(prob + i, i, ; , distance |= 1);
              GET_BIT2(prob + i, i, ; , distance |= 2);
              GET_BIT2(prob + i, i, ; , distance |= 4);
              GET_BIT2(prob + i, i, ; , distance |= 8);
            }
            if (distance == (UInt32)0xFFFFFFFF)
            {
              len += kMatchSpecLenStart;
              state -= kNumStates;
              break;
            }
          }
        }
        rep3 = rep2;
        rep2 = rep1;
        rep1 = rep0;
        rep0 = distance + 1;
        if (checkDicSize == 0)
        {
          if (distance >= processedPos)
            return SZ_ERROR_DATA;
        }
        else if (distance >= checkDicSize)
          return SZ_ERROR_DATA;
        state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
      }

      len += kMatchMinLen;

      if (limit == dicPos)
        return SZ_ERROR_DATA;
      {
        size_t rem = limit - dicPos;
        unsigned curLen = ((rem < len) ? (unsigned)rem : len);
        size_t pos = (dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0);

        processedPos += curLen;

        len -= curLen;
        if (pos + curLen <= dicBufSize)
        {
          Byte *dest = dic + dicPos;
          ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
          const Byte *lim = dest + curLen;
          dicPos += curLen;
          do
            *(dest) = (Byte)*(dest + src);
          while (++dest != lim);
        }
        else
        {
          do
          {
            dic[dicPos++] = dic[pos];
            if (++pos == dicBufSize)
              pos = 0;
          }
          while (--curLen != 0);
        }
      }
    }
  }
  while (dicPos < limit && buf < bufLimit);
  NORMALIZE;
  p->buf = buf;
  p->range = range;
  p->code = code;
  p->remainLen = len;
  p->dicPos = dicPos;
  p->processedPos = processedPos;
  p->reps[0] = rep0;
  p->reps[1] = rep1;
  p->reps[2] = rep2;
  p->reps[3] = rep3;
  p->state = state;

  return SZ_OK;
}

static void LzmaDec_WriteRem(CLzmaDec *p, size_t limit)
{
  if (p->remainLen != 0 && p->remainLen < kMatchSpecLenStart)
  {
    Byte *dic = p->dic;
    size_t dicPos = p->dicPos;
    size_t dicBufSize = p->dicBufSize;
    unsigned len = p->remainLen;
    UInt32 rep0 = p->reps[0];
    if (limit - dicPos < len)
      len = (unsigned)(limit - dicPos);

    if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len)
      p->checkDicSize = p->prop.dicSize;

    p->processedPos += len;
    p->remainLen -= len;
    while (len != 0)
    {
      len--;
      dic[dicPos] = dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
      dicPos++;
    }
    p->dicPos = dicPos;
  }
}

static int LzmaDec_DecodeReal2(CLzmaDec *p, size_t limit, const Byte *bufLimit)
{
  do
  {
    size_t limit2 = limit;
    if (p->checkDicSize == 0)
    {
      UInt32 rem = p->prop.dicSize - p->processedPos;
      if (limit - p->dicPos > rem)
        limit2 = p->dicPos + rem;
    }
    RINOK(LzmaDec_DecodeReal(p, limit2, bufLimit));
    if (p->processedPos >= p->prop.dicSize)
      p->checkDicSize = p->prop.dicSize;
    LzmaDec_WriteRem(p, limit);
  }
  while (p->dicPos < limit && p->buf < bufLimit && p->remainLen < kMatchSpecLenStart);

  if (p->remainLen > kMatchSpecLenStart)
  {
    p->remainLen = kMatchSpecLenStart;
  }
  return 0;
}

typedef enum
{
  DUMMY_ERROR, /* unexpected end of input stream */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec *p, const Byte *buf, size_t inSize)
{
  UInt32 range = p->range;
  UInt32 code = p->code;
  const Byte *bufLimit = buf + inSize;
  CLzmaProb *probs = p->probs;
  unsigned state = p->state;
  ELzmaDummy res;

  {
    CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = (p->processedPos) & ((1 << p->prop.pb) - 1);

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0_CHECK(prob)
    {
      UPDATE_0_CHECK

      /* if (bufLimit - buf >= 7) return DUMMY_LIT; */

      prob = probs + Literal;
      if (p->checkDicSize != 0 || p->processedPos != 0)
        prob += (LZMA_LIT_SIZE *
          ((((p->processedPos) & ((1 << (p->prop.lp)) - 1)) << p->prop.lc) +
          (p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));

      if (state < kNumLitStates)
      {
        unsigned symbol = 1;
        do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        unsigned matchByte = p->dic[p->dicPos - p->reps[0] +
            ((p->dicPos < p->reps[0]) ? p->dicBufSize : 0)];
        unsigned offs = 0x100;
        unsigned symbol = 1;
        do
        {
          unsigned bit;
          CLzmaProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLit = prob + offs + bit + symbol;
          GET_BIT2_CHECK(probLit, symbol, offs &= ~bit, offs &= bit)
        }
        while (symbol < 0x100);
      }
      res = DUMMY_LIT;
    }
    else
    {
      unsigned len;
      UPDATE_1_CHECK;

      prob = probs + IsRep + state;
      IF_BIT_0_CHECK(prob)
      {
        UPDATE_0_CHECK;
        state = 0;
        prob = probs + LenCoder;
        res = DUMMY_MATCH;
      }
      else
      {
        UPDATE_1_CHECK;
        res = DUMMY_REP;
        prob = probs + IsRepG0 + state;
        IF_BIT_0_CHECK(prob)
        {
          UPDATE_0_CHECK;
          prob = probs + IsRep0Long + (state << kNumPosBitsMax) + posState;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK;
            NORMALIZE_CHECK;
            return DUMMY_REP;
          }
          else
          {
            UPDATE_1_CHECK;
          }
        }
        else
        {
          UPDATE_1_CHECK;
          prob = probs + IsRepG1 + state;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK;
          }
          else
          {
            UPDATE_1_CHECK;
            prob = probs + IsRepG2 + state;
            IF_BIT_0_CHECK(prob)
            {
              UPDATE_0_CHECK;
            }
            else
            {
              UPDATE_1_CHECK;
            }
          }
        }
        state = kNumStates;
        prob = probs + RepLenCoder;
      }
      {
        unsigned limit, offset;
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0_CHECK(probLen)
        {
          UPDATE_0_CHECK;
          probLen = prob + LenLow + (posState << kLenNumLowBits);
          offset = 0;
          limit = 1 << kLenNumLowBits;
        }
        else
        {
          UPDATE_1_CHECK;
          probLen = prob + LenChoice2;
          IF_BIT_0_CHECK(probLen)
          {
            UPDATE_0_CHECK;
            probLen = prob + LenMid + (posState << kLenNumMidBits);
            offset = kLenNumLowSymbols;
            limit = 1 << kLenNumMidBits;
          }
          else
          {
            UPDATE_1_CHECK;
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols + kLenNumMidSymbols;
            limit = 1 << kLenNumHighBits;
          }
        }
        TREE_DECODE_CHECK(probLen, limit, len);
        len += offset;
      }

      if (state < 4)
      {
        unsigned posSlot;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) <<
            kNumPosSlotBits);
        TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot);
        if (posSlot >= kStartPosModelIndex)
        {
          int numDirectBits = ((posSlot >> 1) - 1);

          /* if (bufLimit - buf >= 8) return DUMMY_MATCH; */

          if (posSlot < kEndPosModelIndex)
          {
            prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits) - posSlot - 1;
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE_CHECK
              range >>= 1;
              code -= range & (((code - range) >> 31) - 1);
              /* if (code >= range) code -= range; */
            }
            while (--numDirectBits != 0);
            prob = probs + Align;
            numDirectBits = kNumAlignBits;
          }
          {
            unsigned i = 1;
            do
            {
              GET_BIT_CHECK(prob + i, i);
            }
            while (--numDirectBits != 0);
          }
        }
      }
    }
  }
  NORMALIZE_CHECK;
  return res;
}


static void LzmaDec_InitRc(CLzmaDec *p, const Byte *data)
{
  p->code = ((UInt32)data[1] << 24) | ((UInt32)data[2] << 16) | ((UInt32)data[3] << 8) | ((UInt32)data[4]);
  p->range = 0xFFFFFFFF;
  p->needFlush = 0;
}

static void LzmaDec_InitDicAndState(CLzmaDec *p, Bool initDic, Bool initState)
{
  p->needFlush = 1;
  p->remainLen = 0;
  p->tempBufSize = 0;

  if (initDic)
  {
    p->processedPos = 0;
    p->checkDicSize = 0;
    p->needInitState = 1;
  }
  if (initState)
    p->needInitState = 1;
}

static void LzmaDec_InitStateReal(CLzmaDec *p)
{
  UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (p->prop.lc + p->prop.lp));
  UInt32 i;
  CLzmaProb *probs = p->probs;
  for (i = 0; i < numProbs; i++)
    probs[i] = kBitModelTotal >> 1;
  p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
  p->state = 0;
  p->needInitState = 0;
}

static SRes LzmaDec_DecodeToDic(CLzmaDec *p, size_t dicLimit, const Byte *src, size_t *srcLen,
    ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  size_t inSize = *srcLen;
  (*srcLen) = 0;
  LzmaDec_WriteRem(p, dicLimit);

  *status = LZMA_STATUS_NOT_SPECIFIED;

  while (p->remainLen != kMatchSpecLenStart)
  {
      int checkEndMarkNow;

      if (p->needFlush != 0)
      {
        for (; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--)
          p->tempBuf[p->tempBufSize++] = *src++;
        if (p->tempBufSize < RC_INIT_SIZE)
        {
          *status = LZMA_STATUS_NEEDS_MORE_INPUT;
          return SZ_OK;
        }
        if (p->tempBuf[0] != 0)
          return SZ_ERROR_DATA;

        LzmaDec_InitRc(p, p->tempBuf);
        p->tempBufSize = 0;
      }

      checkEndMarkNow = 0;
      if (p->dicPos >= dicLimit)
      {
        if (p->remainLen == 0 && p->code == 0)
        {
          *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
          return SZ_OK;
        }
        if (finishMode == LZMA_FINISH_ANY)
        {
          *status = LZMA_STATUS_NOT_FINISHED;
          return SZ_OK;
        }
        if (p->remainLen != 0)
        {
          *status = LZMA_STATUS_NOT_FINISHED;
          return SZ_ERROR_DATA;
        }
        checkEndMarkNow = 1;
      }

      if (p->needInitState)
        LzmaDec_InitStateReal(p);

      if (p->tempBufSize == 0)
      {
        size_t processed;
        const Byte *bufLimit;
        if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          int dummyRes = LzmaDec_TryDummy(p, src, inSize);
          if (dummyRes == DUMMY_ERROR)
          {
            memcpy(p->tempBuf, src, inSize);
            p->tempBufSize = (unsigned)inSize;
            (*srcLen) += inSize;
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            *status = LZMA_STATUS_NOT_FINISHED;
            return SZ_ERROR_DATA;
          }
          bufLimit = src;
        }
        else
          bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;
        p->buf = src;
        if (LzmaDec_DecodeReal2(p, dicLimit, bufLimit) != 0)
          return SZ_ERROR_DATA;
        processed = (size_t)(p->buf - src);
        (*srcLen) += processed;
        src += processed;
        inSize -= processed;
      }
      else
      {
        unsigned rem = p->tempBufSize, lookAhead = 0;
        while (rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < inSize)
          p->tempBuf[rem++] = src[lookAhead++];
        p->tempBufSize = rem;
        if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          int dummyRes = LzmaDec_TryDummy(p, p->tempBuf, rem);
          if (dummyRes == DUMMY_ERROR)
          {
            (*srcLen) += lookAhead;
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            *status = LZMA_STATUS_NOT_FINISHED;
            return SZ_ERROR_DATA;
          }
        }
        p->buf = p->tempBuf;
        if (LzmaDec_DecodeReal2(p, dicLimit, p->buf) != 0)
          return SZ_ERROR_DATA;
        lookAhead -= (rem - (unsigned)(p->buf - p->tempBuf));
        (*srcLen) += lookAhead;
        src += lookAhead;
        inSize -= lookAhead;
        p->tempBufSize = 0;
      }
  }
  if (p->code == 0)
    *status = LZMA_STATUS_FINISHED_WITH_MARK;
  return (p->code == 0) ? SZ_OK : SZ_ERROR_DATA;
}

#define LZMA2_CONTROL_LZMA (1 << 7)
#define LZMA2_CONTROL_COPY_NO_RESET 2
#define LZMA2_CONTROL_COPY_RESET_DIC 1
#define LZMA2_CONTROL_EOF 0

#define LZMA2_IS_UNCOMPRESSED_STATE(pc) (((pc) & LZMA2_CONTROL_LZMA) == 0)

#define LZMA2_GET_LZMA_MODE(pc) (((pc) >> 5) & 3)
#define LZMA2_IS_THERE_PROP(mode) ((mode) >= 2)

/* Works if p <= 39. */
#define LZMA2_DIC_SIZE_FROM_SMALL_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

static SRes Lzma2Dec_DecodeToDicCompressed(CLzma2Dec *p, Byte control, UInt32 packSize, const Byte *src) {
  const Byte mode = LZMA2_GET_LZMA_MODE(control);
  const Bool initDic = (mode == 3);
  const Bool initState = (mode > 0);
  ELzmaStatus status;
  size_t srcSizeCur;
  DEBUGF("DECODE call\n");
  ASSERT(!LZMA2_IS_UNCOMPRESSED_STATE(control));
  if ((!initDic && p->needInitDic) || (!initState && p->needInitState))
    return SZ_ERROR_DATA;
  LzmaDec_InitDicAndState(&p->decoder, initDic, initState);
  p->needInitDic = False;
  p->needInitState = False;
  srcSizeCur = packSize;
  /* !! Keep only LZMA_FINISH_END,implemented. */
  RINOK(LzmaDec_DecodeToDic(&p->decoder, p->decoder.dicBufSize, src, &srcSizeCur, LZMA_FINISH_END, &status));
  if (p->decoder.dicBufSize != p->decoder.dicPos || srcSizeCur != packSize ||
      status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) {
    return SZ_ERROR_DATA;  /* Compressed or uncompressed chunk size not exactly correct. */
  }
  return SZ_OK;
}

static SRes Lzma2Dec_DecodeToDicUncompressed(CLzma2Dec *p, Byte control, const Byte *src) {
  UInt32 unpackSize = p->decoder.dicBufSize - p->decoder.dicPos;
  Bool initDic = (control == LZMA2_CONTROL_COPY_RESET_DIC);
  DEBUGF("DECODE uncompressed\n");
  ASSERT(LZMA2_IS_UNCOMPRESSED_STATE(control));
  ASSERT(0 != unpackSize);
  if (initDic)
    p->needInitProp = p->needInitState = True;
  else if (p->needInitDic)
    return SZ_ERROR_DATA;
  p->needInitDic = False;
  LzmaDec_InitDicAndState(&p->decoder, initDic, False);
  memcpy(p->decoder.dic + p->decoder.dicPos, src, unpackSize);
  p->decoder.dicPos += unpackSize;
  if (p->decoder.checkDicSize == 0 && p->decoder.prop.dicSize - p->decoder.processedPos <= unpackSize)
    p->decoder.checkDicSize = p->decoder.prop.dicSize;
  p->decoder.processedPos += unpackSize;
  return SZ_OK;
}

/* !! Read at least 65536 bytes in the beginning to decompressBuf, no need for input buffering? */
static Byte readBuf[65535 + 6], *readCur = readBuf, *readEnd = readBuf;
static UInt64 readFileOfs = 0;

/* Try to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if r <= sizeof(readBuf).
 */
static UInt32 Preread(UInt32 r) {
  UInt32 p = readEnd - readCur;
  ASSERT(r <= sizeof(readBuf));
  if (p < r) {  /* Not enough pending available. */
    if (readBuf + sizeof(readBuf) - readCur + 0U < r) {
      /* If no room for r bytes to the end, discard bytes from the beginning. */
      MemmoveBackward(readBuf, readCur, p);
      readEnd = readBuf + p;
      readCur = readBuf;
    }
    while (p < r) {
      /* Instead of (r - p) we could use (readBuf + sizeof(readBuf) -
       * readEnd) to read as much as the buffer has room for.
       */
      ssize_t got = read(0, readEnd, r - p);
      if (got <= 0) break;  /* EOF on input. */
      readEnd += got;
      p += got;
      readFileOfs += got;
    }
  }
  return p;
}

/* Return the number of bytes read from stdin. */
static UInt64 Tell() {
  return readFileOfs - (readEnd - readCur);
}

static int GetByte() {
  if (readCur == readEnd) {
    ssize_t got = read(0, readBuf, sizeof(readBuf));
    if (got <= 0) return -1;
    readCur = readBuf;
    readEnd = readBuf + got;
    readFileOfs += got;
  }
  return *readCur++;
}

/* Like GetByte(), but faster. */
/* !! #define GET_BYTE() GetByte() if CONFIG_SIZE_OPT. */
#define GET_BYTE() (readCur == readEnd ? GetByte() : *readCur++)

/* Contains the uncompressed data.
 *
 * We rely on virtual memory so that if we don't use the end of array for
 * small files, then the operating system won't take the entire array away
 * from other processes.
 */
static Byte decompressBuf[1610612736];

#define VARINT_EOF ((UInt64)-1)

/* !! Get rid of Int64 everywhere -- LZMA decoder doesn't need it. */
static UInt64 GetVarint(void) {
  UInt64 result;
  int i = 0;
  int b;
  if ((b = GET_BYTE()) < 0) return VARINT_EOF;
  result = b & 0x7F;
  while (b & 0x80) {
    if ((b = GET_BYTE()) < 0) return VARINT_EOF;
    result |= ((UInt64)(b & 0x7F)) << i;
    i += 7;
  }
  return result;
}

static SRes IgnoreFewBytes(UInt32 c) {
  while (c > 0) {
    if (GET_BYTE() < 0) return SZ_ERROR_INPUT_EOF;
    c--;
  }
  return SZ_OK;
}

#define SZ_ERROR_BAD_MAGIC 51
#define SZ_ERROR_BAD_STREAM_FLAGS 52
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

static SRes IgnoreZeroBytes(UInt32 c) {
  int i;
  while (c > 0) {
    if ((i = GET_BYTE()) < 0) return SZ_ERROR_INPUT_EOF;
    if (i != 0) return SZ_ERROR_BAD_PADDING;
    c--;
  }
  return SZ_OK;
}

static SRes IgnorePadding4() {
  return IgnoreZeroBytes(-(Byte)Tell() & 3);
}

#define FILTER_ID_LZMA2 0x21

/* Reads from stdin, writes to stdout, uses decompressBuf.
 * Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt
 */
static SRes DecompressXz(void) {
  SRes res = SZ_OK;
  int i;
  Byte checksumSize;
  if (GET_BYTE() != 0xFD) return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != '7') return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != 'z') return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != 'X') return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != 'Z') return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != 0x00) return SZ_ERROR_BAD_MAGIC;
  if (GET_BYTE() != 0x00) return SZ_ERROR_BAD_STREAM_FLAGS;
  /* Checksum algorithm. CRC32 is 1, CRC64 is 4. */
  if ((i = GET_BYTE()) < 0 ) return SZ_ERROR_INPUT_EOF;
  checksumSize = i == 0 ? 0 : i == 1 ? 4 /* CRC32 */ : i == 4 ? 8 /* CRC64 */ : 0xff;
  if (checksumSize == 0xff) return SZ_ERROR_BAD_CHECKSUM_TYPE;
  RINOK(IgnoreFewBytes(4));  /* CRC32 */
  for (;;) {
    UInt64 ii;
    UInt32 bhs, bhs2; /* Block header size */
    UInt32 bhf;  /* Block header flags */
    UInt64 bo;  /* Block offset */
    Byte dicSizeProp;
    UInt32 dicSize;
    bo = Tell();
    if ((i = GET_BYTE()) < 0) return SZ_ERROR_INPUT_EOF;
    if (i == 0) break;  /* Last block, index follows. */
    bhs = (i + 1) << 2;
    if ((i = GET_BYTE()) < 0) return SZ_ERROR_INPUT_EOF;
    bhf = i;
    if ((bhf & 2) != 0) return SZ_ERROR_UNSUPPORTED_FILTER_COUNT;
    DEBUGF("filter count=%d\n", (bhf & 2) + 1);
    if ((bhf & 20) != 0) return SZ_ERROR_BAD_BLOCK_FLAGS;
    if (bhf & 64) {  /* Compressed size present. */
      /* Usually not present, just ignore. */
      if ((ii = GetVarint()) == VARINT_EOF) return SZ_ERROR_INPUT_EOF;
    }
    if (bhf & 128) {  /* Uncompressed size present. */
      /* Usually not present, just ignore. */
      if ((ii = GetVarint()) == VARINT_EOF) return SZ_ERROR_INPUT_EOF;
    }
    /* !! TODO(pts): Simplify it with GET_BYTE. */
    if ((ii = GetVarint()) == VARINT_EOF) return SZ_ERROR_INPUT_EOF;
    if (ii != FILTER_ID_LZMA2) return SZ_ERROR_UNSUPPORTED_FILTER_ID;
    if ((ii = GetVarint()) == VARINT_EOF) return SZ_ERROR_INPUT_EOF;
    if (ii != 1) return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE;
    if ((i = GET_BYTE()) < 0) return SZ_ERROR_INPUT_EOF;
    dicSizeProp = i;
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
    if (dicSizeProp > 40) return SZ_ERROR_BAD_DICTIONARY_SIZE;
    if (dicSizeProp > 37) return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE;
    /* !! LZMA2 supports dicSizeProp 40 as well, but we don't */
    dicSize = LZMA2_DIC_SIZE_FROM_SMALL_PROP(dicSizeProp);
    ASSERT(dicSize >= LZMA_DIC_MIN);
    DEBUGF("dicSize39=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(39));
    DEBUGF("dicSize38=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(38));
    DEBUGF("dicSize37=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(37));
    DEBUGF("dicSize36=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(36));
    DEBUGF("dicSize35=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(35));
    bhs2 = (UInt32)(Tell() - bo) + 4;  /* Won't overflow. */
    if (bhs2 > bhs) return SZ_ERROR_BLOCK_HEADER_TOO_LONG;
    RINOK(IgnoreZeroBytes(bhs - bhs2));
    RINOK(IgnoreFewBytes(4));  /* CRC32 */
    /* !! Typically it's offset 24. */
    DEBUGF("LZMA2 at %d\n", (UInt32)Tell());
    { /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      UInt32 us, cs;  /* Uncompressed and compressed chunk sizes. */
      CLzma2Dec state;

      state.decoder.prop.dicSize = dicSize;
      state.decoder.prop.lc = 0;  /* needinitprop will initialize it */
      state.decoder.prop.pb = 0;
      state.decoder.prop.lp = 0;
      state.decoder.numProbs = Lzma2Props_GetMaxNumProbs();
      state.decoder.probs = state.probs;  /* Copies pointer only. */
      state.decoder.dic = decompressBuf;
      state.decoder.dicBufSize = 0;  /* We'll increment it later. */
      state.needInitDic = True;
      state.needInitState = True;
      state.needInitProp = True;
      state.decoder.dicPos = 0;
      LzmaDec_InitDicAndState(&state.decoder, True, True);

      for (;;) {
        ASSERT(state.decoder.dicPos == state.decoder.dicBufSize);
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (Preread(6) < 6) return SZ_ERROR_INPUT_EOF;
        i = readCur[0];
        DEBUGF("CONTROL 0x%02x at=%d\n", i, (UInt32)Tell());
        if (i == 0) {
          DEBUGF("LASTFED\n");
          ++readCur;
          break;
        } else if (i >= 3 && i < 0x80) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        /* !! Remove redundancy. */
        us = (readCur[1] << 8) + readCur[2] + 1;
        if (i < 3) {  /* Uncompressed chunk. */
          cs = us;
          readCur += 3;
        } else {  /* LZMA chunk. */
          const Bool isProp = (i & 64) != 0;
          us += (i & 31) << 16;
          cs = (readCur[3] << 8) + readCur[4] + 1;
          if (isProp) {
            Byte b = readCur[5];
            int lc, lp;
            if (b >= (9 * 5 * 5)) return SZ_ERROR_BAD_LCLPPB_PROP;
            lc = b % 9;
            b /= 9;
            state.decoder.prop.pb = b / 5;
            lp = b % 5;
            if (lc + lp > LZMA2_LCLP_MAX) return SZ_ERROR_BAD_LCLPPB_PROP;
            state.decoder.prop.lc = lc;
            state.decoder.prop.lp = lp;
            state.needInitProp = False;
            ++readCur;
          } else {
            if (state.needInitProp) return SZ_ERROR_MISSING_INITPROP;
          }
          readCur += 5;
        }
        state.decoder.dicBufSize += us;
        /* Decompressed data too long, won't fit to decompressBuf. */
        if (state.decoder.dicBufSize > sizeof(decompressBuf)) return SZ_ERROR_MEM;
        if (Preread(cs) < cs) return SZ_ERROR_INPUT_EOF;
        {
          DEBUGF("FEED us=%d cs=%d dicPos=%d\n", us, cs, (int)state.decoder.dicPos);
          /* !! Get rid of Lzma2Dec_DecodeToDic, use LzmaDec_DecodeToDic directly. */
          if ((Byte)i < 0x80) {
            RINOK(Lzma2Dec_DecodeToDicUncompressed(&state, i, readCur));
          } else {
            /* This call doesn't chane state.decoder.dicBufSize. */
            RINOK(Lzma2Dec_DecodeToDicCompressed(&state, i, cs, readCur));
          }
          if (state.decoder.dicPos != state.decoder.dicBufSize) return SZ_ERROR_BAD_DICPOS;
        }
        {
          const Byte *q = decompressBuf + state.decoder.dicBufSize, *p = q - us;
          while (p != q) {
            ssize_t got = write(1, p, q - p);
            if (got <= 0) return SZ_ERROR_WRITE;
            p += got;
          }
        }
        readCur += cs;
        /* We can't discard decompressbuf[:state.decoder.dicBufSize] now,
         * because we need it a dictionary in which subsequent calls to
         * Lzma2Dec_DecodeToDic will look up backreferences.
         */
      }
    }
    DEBUGF("ALTELL %d\n", (UInt32)Tell());
    RINOK(IgnorePadding4());  /* Block padding. */
    DEBUGF("AMTELL %d\n", (UInt32)Tell());
    RINOK(IgnoreFewBytes(checksumSize));
    DEBUGF("TELL %d\n", (UInt32)Tell());
  }
  /* The .xz input file follows with the index, which we ignore from here. */
  return res;
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;
#if defined(MSDOS) || defined(_WIN32)
  setmode(0, O_BINARY);
  setmode(1, O_BINARY);
#endif
  return DecompressXz();
}
