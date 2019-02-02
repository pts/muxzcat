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
 * * !! TODO(pts): It doesn't use pointers. (It uses arrays and array indexes.)
 * * !! TODO(pts): It doesn't use macros (except for constants).
 * * !! TODO(pts): It has more state in global variables (rather than function
 *   arguments).
 *
 * Most users should use muxzcat.c instead, because that one runs faster.
 */

#ifdef __TINYC__  /* tcc https://bellard.org/tcc/ , pts-tcc https://github.com/pts/pts-tcc */

typedef unsigned size_t;  /* TODO(pts): Support 64-bit tcc */
typedef int ssize_t;  /* TODO(pts): Support 64-bit tcc */
typedef int int32_t;
typedef unsigned uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

int memcmp(const void *s1, const void *s2, size_t n);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#else
#ifdef __XTINY__  /* xtiny https://github.com/pts/pts-xtiny */

#include <xtiny.h>

#else  /* Not __XTINY__. */

#include <string.h>  /* memcmp() */
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

typedef int32_t  Int32;
typedef uint32_t UInt32;
typedef int16_t  Int16;
typedef uint16_t UInt16;
typedef uint8_t  Byte;

/* --- */

#undef CONFIG_DEBUG
#define CONFIG_DEBUG 1
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

typedef UInt32 SRes;

#ifndef RINOK
#define RINOK(x) { int __result__ = (x); if (__result__ != 0) return __result__; }
#endif

typedef Byte Bool;
#define True 1
#define False 0

#define LZMA_REQUIRED_INPUT_MAX 20

/* CONFIG_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case
   CONFIG_PROB32 increases memory usage by 28268 bytes.
   */
#if 0  /*#ifdef CONFIG_PROB32*/
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768
#define LZMA2_LCLP_MAX 4

/* For LZMA streams, lc + lp <= 8 + 4 <= 12.
 * For LZMA2 streams, lc + lp <= 4.
 * Minimum value: 1846.
 * Maximum value for LZMA streams: 1846 + (768 << (8 + 4)) == 3147574.
 * Maximum value for LZMA2 streams: 1846 + (768 << 4) == 14134.
 * Memory usage of prob: size_of_CLzmaProb * value == (2 or 4) * value bytes.
 */
#define LzmaProps_GetNumProbs(p) ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))
/* 14134 */
#define Lzma2Props_GetMaxNumProbs() ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << LZMA2_LCLP_MAX))

#define DIC_ARRAY_SIZE 1610612736

/* 6 is maximum LZMA chunk header size.
 * 65536 is maximum cs (compressed size) of LZMA2 chunk.
 * 6 is maximum LZMA chunk header size for the next chunk.
 */
#define READBUF_SIZE (6 + 65536 + 6)

typedef struct {
  UInt32 bufCur;
  UInt32 dicSize;  /* Configured in prop byte. */
  UInt32 range, code;
  UInt32 dicPos;
  UInt32 dicBufSize;
  UInt32 processedPos;
  UInt32 checkDicSize;
  UInt32 state;
  UInt32 reps[4];
  UInt32 remainLen;
  UInt32 tempBufSize;
  UInt32 readCur;  /* Index within (or at end of) readBuf. */
  UInt32 readEnd;  /* Index within (or at end of) readBuf. */
  Bool needFlush;
  Bool needInitLzma;
  Bool needInitDic;
  Bool needInitState;
  Bool needInitProp;
  Byte lc, lp, pb;  /* Configured in prop byte. Also works as UInt32. */
  CLzmaProb probs[Lzma2Props_GetMaxNumProbs()];
  /* The first READBUF_SIZE bytes is readBuf, then the LZMA_REQUIRED_INPUT_MAX bytes is tempBuf. */
  Byte readBuf[READBUF_SIZE + LZMA_REQUIRED_INPUT_MAX];
  /* Contains the uncompressed data.
   *
   * Array size is about 1.61 GB.
   * We rely on virtual memory so that if we don't use the end of array for
   * small files, then the operating system won't take the entire array away
   * from other processes.
   */
  Byte dic[DIC_ARRAY_SIZE];
} CLzmaDec;

CLzmaDec global;

/* --- */

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_INIT_SIZE 5

#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (global.readBuf[bufCur++]); }

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

#if 1  /*#ifdef CONFIG_SIZE_OPT*/
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

#define NORMALIZE_CHECK if (range < kTopValue) { if (bufDummyCur >= bufLimit) return DUMMY_ERROR; range <<= 8; code = (code << 8) | (global.readBuf[bufDummyCur++]); }

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

/* Modifies global.bufCur etc. */
SRes LzmaDec_DecodeReal(UInt32 limit, UInt32 bufLimit)
{
  CLzmaProb *probs = global.probs;

  UInt32 state = global.state;
  UInt32 rep0 = global.reps[0], rep1 = global.reps[1], rep2 = global.reps[2], rep3 = global.reps[3];
  UInt32 pbMask = ((UInt32)1 << (global.pb)) - 1;
  UInt32 lpMask = ((UInt32)1 << (global.lp)) - 1;
  UInt32 lc = global.lc;

  UInt32 dicBufSize = global.dicBufSize;
  UInt32 dicPos = global.dicPos;

  UInt32 processedPos = global.processedPos;
  UInt32 checkDicSize = global.checkDicSize;
  UInt32 len = 0;

  UInt32 bufCur = global.bufCur;
  UInt32 range = global.range;
  UInt32 code = global.code;

  do
  {
    CLzmaProb *prob;
    UInt32 bound;
    UInt32 ttt;
    UInt32 posState = processedPos & pbMask;

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0(prob)
    {
      UInt32 symbol;
      UPDATE_0(prob);
      prob = probs + Literal;
      if (checkDicSize != 0 || processedPos != 0)
        prob += (LZMA_LIT_SIZE * (((processedPos & lpMask) << lc) +
        (global.dic[(dicPos == 0 ? dicBufSize : dicPos) - 1] >> (8 - lc))));

      if (state < kNumLitStates)
      {
        state -= (state < 4) ? state : 3;
        symbol = 1;
        do { GET_BIT(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        UInt32 matchByte = global.dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
        UInt32 offs = 0x100;
        state -= (state < 10) ? 3 : 6;
        symbol = 1;
        do
        {
          UInt32 bit;
          CLzmaProb *probLit;
          matchByte <<= 1;
          bit = (matchByte & offs);
          probLit = prob + offs + bit + symbol;
          GET_BIT2(probLit, symbol, offs &= ~bit, offs &= bit)
        }
        while (symbol < 0x100);
      }
      global.dic[dicPos++] = (Byte)symbol;
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
            global.dic[dicPos] = global.dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
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
        UInt32 limit, offset;
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
          UInt32 posSlot = (UInt32)distance;
          int numDirectBits = (int)(((distance >> 1) - 1));
          distance = (2 | (distance & 1));
          if (posSlot < kEndPosModelIndex)
          {
            distance <<= numDirectBits;
            prob = probs + SpecPos + distance - posSlot - 1;
            {
              UInt32 mask = 1;
              UInt32 i = 1;
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
                t = (0 - ((UInt32)code >> 31));
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
              UInt32 i = 1;
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
        UInt32 rem = limit - dicPos;
        UInt32 curLen = ((rem < len) ? (UInt32)rem : len);
        UInt32 pos = (dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0);

        processedPos += curLen;

        len -= curLen;
        if (pos + curLen <= dicBufSize)
        {
          ASSERT(dicPos > pos);
          ASSERT(curLen > 0);
          do {
            global.dic[dicPos++] = global.dic[pos++];
          } while (--curLen != 0);
        }
        else
        {
          do {
            global.dic[dicPos++] = global.dic[pos++];
            if (pos == dicBufSize) pos = 0;
          } while (--curLen != 0);
        }
      }
    }
  }
  while (dicPos < limit && bufCur < bufLimit);
  NORMALIZE;
  global.bufCur = bufCur;
  global.range = range;
  global.code = code;
  global.remainLen = len;
  global.dicPos = dicPos;
  global.processedPos = processedPos;
  global.reps[0] = rep0;
  global.reps[1] = rep1;
  global.reps[2] = rep2;
  global.reps[3] = rep3;
  global.state = state;

  return SZ_OK;
}

void LzmaDec_WriteRem(UInt32 limit)
{
  if (global.remainLen != 0 && global.remainLen < kMatchSpecLenStart)
  {
    Byte *dic = global.dic;
    UInt32 dicPos = global.dicPos;
    UInt32 dicBufSize = global.dicBufSize;
    UInt32 len = global.remainLen;
    UInt32 rep0 = global.reps[0];
    if (limit - dicPos < len)
      len = (UInt32)(limit - dicPos);

    if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= len)
      global.checkDicSize = global.dicSize;

    global.processedPos += len;
    global.remainLen -= len;
    while (len != 0)
    {
      len--;
      dic[dicPos] = dic[(dicPos - rep0) + ((dicPos < rep0) ? dicBufSize : 0)];
      dicPos++;
    }
    global.dicPos = dicPos;
  }
}

/* Modifies global.bufCur etc. */
SRes LzmaDec_DecodeReal2(UInt32 limit, UInt32 bufLimit)
{
  do
  {
    UInt32 limit2 = limit;
    if (global.checkDicSize == 0)
    {
      UInt32 rem = global.dicSize - global.processedPos;
      if (limit - global.dicPos > rem)
        limit2 = global.dicPos + rem;
    }
    RINOK(LzmaDec_DecodeReal(limit2, bufLimit));
    if (global.processedPos >= global.dicSize)
      global.checkDicSize = global.dicSize;
    LzmaDec_WriteRem(limit);
  }
  while (global.dicPos < limit && global.bufCur < bufLimit && global.remainLen < kMatchSpecLenStart);

  if (global.remainLen > kMatchSpecLenStart)
  {
    global.remainLen = kMatchSpecLenStart;
  }
  return SZ_OK;
}

typedef enum
{
  DUMMY_ERROR, /* unexpected end of input stream */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;

/* Replace pointer argument buf here with an UInt32 argument. */
ELzmaDummy LzmaDec_TryDummy(UInt32 bufDummyCur, const UInt32 bufLimit)
{
  UInt32 range = global.range;
  UInt32 code = global.code;
  const CLzmaProb *probs = global.probs;
  UInt32 state = global.state;
  ELzmaDummy res;

  {
    const CLzmaProb *prob;
    UInt32 bound;
    UInt32 ttt;
    UInt32 posState = (global.processedPos) & ((1 << global.pb) - 1);

    prob = probs + IsMatch + (state << kNumPosBitsMax) + posState;
    IF_BIT_0_CHECK(prob)
    {
      UPDATE_0_CHECK

      /* if (bufLimit - buf >= 7) return DUMMY_LIT; */

      prob = probs + Literal;
      if (global.checkDicSize != 0 || global.processedPos != 0)
        prob += (LZMA_LIT_SIZE *
          ((((global.processedPos) & ((1 << (global.lp)) - 1)) << global.lc) +
          (global.dic[(global.dicPos == 0 ? global.dicBufSize : global.dicPos) - 1] >> (8 - global.lc))));

      if (state < kNumLitStates)
      {
        UInt32 symbol = 1;
        do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        UInt32 matchByte = global.dic[global.dicPos - global.reps[0] +
            ((global.dicPos < global.reps[0]) ? global.dicBufSize : 0)];
        UInt32 offs = 0x100;
        UInt32 symbol = 1;
        do
        {
          UInt32 bit;
          const CLzmaProb *probLit;
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
      UInt32 len;
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
        UInt32 limit, offset;
        const CLzmaProb *probLen = prob + LenChoice;
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
        UInt32 posSlot;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) <<
            kNumPosSlotBits);
        TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot);
        if (posSlot >= kStartPosModelIndex)
        {
          UInt32 numDirectBits = ((posSlot >> 1) - 1);

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
            UInt32 i = 1;
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


void LzmaDec_InitRc(const Byte *data)
{
  global.code = ((UInt32)data[1] << 24) | ((UInt32)data[2] << 16) | ((UInt32)data[3] << 8) | ((UInt32)data[4]);
  global.range = 0xFFFFFFFF;
  global.needFlush = False;
}

void LzmaDec_InitDicAndState(Bool initDic, Bool initState)
{
  global.needFlush = True;
  global.remainLen = 0;
  global.tempBufSize = 0;

  if (initDic)
  {
    global.processedPos = 0;
    global.checkDicSize = 0;
    global.needInitLzma = True;
  }
  if (initState)
    global.needInitLzma = True;
}

void LzmaDec_InitStateReal(void)
{
  UInt32 numProbs = Literal + ((UInt32)LZMA_LIT_SIZE << (global.lc + global.lp));
  UInt32 i;
  CLzmaProb *probs = global.probs;
  for (i = 0; i < numProbs; i++)
    probs[i] = kBitModelTotal >> 1;
  global.reps[0] = global.reps[1] = global.reps[2] = global.reps[3] = 1;
  global.state = 0;
  global.needInitLzma = False;
}

/* Decompress LZMA stream in
 * global.readBuf[global.readCur : global.readCur + srcLen].
 * On success (and on some errors as well), adds srcLen to global.readCur.
 */
SRes LzmaDec_DecodeToDic(const UInt32 srcLen) {
  /* Index limit in global.readBuf. */
  const UInt32 decodeLimit = global.readCur + srcLen;
  const UInt32 dicLimit = global.dicBufSize;
  LzmaDec_WriteRem(dicLimit);

  while (global.remainLen != kMatchSpecLenStart)
  {
      Bool checkEndMarkNow;

      if (global.needFlush)
      {
        for (; decodeLimit > global.readCur && global.tempBufSize < RC_INIT_SIZE;)
          global.readBuf[READBUF_SIZE + global.tempBufSize++] = global.readBuf[global.readCur++];
        if (global.tempBufSize < RC_INIT_SIZE)
        {
         on_needs_more_input:
          if (decodeLimit != global.readCur) return SZ_ERROR_NEEDS_MORE_INPUT_PARTIAL;
          return SZ_ERROR_NEEDS_MORE_INPUT;
        }
        if (global.readBuf[READBUF_SIZE] != 0)
          return SZ_ERROR_DATA;

        LzmaDec_InitRc(&global.readBuf[READBUF_SIZE]);  /* tempBuf. */
        global.tempBufSize = 0;
      }

      checkEndMarkNow = False;
      if (global.dicPos >= dicLimit)
      {
        if (global.remainLen == 0 && global.code == 0)
        {
          if (decodeLimit != global.readCur) return SZ_ERROR_CHUNK_NOT_CONSUMED;
          return SZ_OK /* MAYBE_FINISHED_WITHOUT_MARK */;
        }
        if (global.remainLen != 0)
        {
          return SZ_ERROR_NOT_FINISHED;
        }
        checkEndMarkNow = True;
      }

      if (global.needInitLzma) {
        LzmaDec_InitStateReal();
      }

      if (global.tempBufSize == 0)
      {
        UInt32 bufLimit;
        if (decodeLimit - global.readCur < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          SRes dummyRes;
          dummyRes = LzmaDec_TryDummy(global.readCur, decodeLimit);
          if (dummyRes == DUMMY_ERROR)
          {
            /* This line can be triggered by passing srcLen==1 to LzmaDec_DecodeToDic. */
            global.tempBufSize = 0;
            while (global.readCur != decodeLimit) {
              global.readBuf[READBUF_SIZE + global.tempBufSize++] = global.readBuf[global.readCur++];
            }
            goto on_needs_more_input;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            return SZ_ERROR_NOT_FINISHED;
          }
          bufLimit = global.readCur;
        }
        else
          bufLimit = decodeLimit - LZMA_REQUIRED_INPUT_MAX;
        global.bufCur = global.readCur;  /* !! Use readCur instead of global.bufCur? */
        if (LzmaDec_DecodeReal2(dicLimit, bufLimit) != 0)
          return SZ_ERROR_DATA;
        global.readCur = global.bufCur;
      }
      else
      {
        UInt32 rem = global.tempBufSize, lookAhead = 0;
        while (rem < LZMA_REQUIRED_INPUT_MAX && lookAhead < decodeLimit - global.readCur)
          global.readBuf[READBUF_SIZE + rem++] = global.readBuf[global.readCur + lookAhead++];
        global.tempBufSize = rem;
        if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          SRes dummyRes;
          dummyRes = LzmaDec_TryDummy(READBUF_SIZE, READBUF_SIZE + rem);
          if (dummyRes == DUMMY_ERROR)
          {
            global.readCur += lookAhead;
            goto on_needs_more_input;
          }
          if (checkEndMarkNow && dummyRes != DUMMY_MATCH)
          {
            return SZ_ERROR_NOT_FINISHED;
          }
        }
        /* This line can be triggered by passing srcLen==1 to LzmaDec_DecodeToDic. */
        global.bufCur = READBUF_SIZE;  /* tempBuf. */
        if (LzmaDec_DecodeReal2(0, READBUF_SIZE) != 0)
          return SZ_ERROR_DATA;
        lookAhead -= rem - (global.bufCur - READBUF_SIZE);
        global.readCur += lookAhead;
        global.tempBufSize = 0;
      }
  }
  if (global.code != 0) return SZ_ERROR_DATA;
  return SZ_ERROR_FINISHED_WITH_MARK;
}

#define LZMA2_GET_LZMA_MODE(pc) (((pc) >> 5) & 3)

/* Works if p <= 39. */
#define LZMA2_DIC_SIZE_FROM_SMALL_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

/* Tries to preread r bytes to the read buffer. Returns the number of bytes
 * available in the read buffer. If smaller than r, that indicates EOF.
 *
 * Doesn't try to preread more than absolutely necessary, to avoid copies in
 * the future.
 *
 * Works only if r <= READBUF_SIZE.
 */
UInt32 Preread(UInt32 r) {
  UInt32 p = global.readEnd - global.readCur;
  ASSERT(r <= READBUF_SIZE);
  if (p < r) {  /* Not enough pending available. */
    if (READBUF_SIZE - global.readCur + 0U < r) {
      UInt32 copyIdx;
      /* If no room for r bytes to the end, discard bytes from the beginning. */
      DEBUGF("MEMMOVE size=%d\n", p);
      for (copyIdx = 0; copyIdx < p; ++copyIdx) {
        global.readBuf[copyIdx] = global.readBuf[global.readCur + copyIdx];
      }
      global.readEnd = p;
      global.readCur = 0;
    }
    while (p < r) {
      /* Instead of (r - p) we could use (global.readBuf + READBUF_SIZE -
       * global.readEnd) to read as much as the buffer has room for.
       */
      DEBUGF("READ size=%d\n", r - p);
      const Int32 got = read(0, &global.readBuf[global.readEnd], r - p);
      if (got <= 0) break;  /* EOF on input. */
      global.readEnd += got;
      p += got;
    }
  }
  DEBUGF("PREREAD r=%d p=%d\n", r, p);
  return p;
}

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

void IgnoreVarint(void) {
  while (global.readBuf[global.readCur++] >= 0x80) {}
}

SRes IgnoreZeroBytes(UInt32 c) {
  for (; c > 0; --c) {
    if (global.readBuf[global.readCur++] != 0) return SZ_ERROR_BAD_PADDING;
  }
  return SZ_OK;
}

UInt32 GetLE4(UInt32 p) {
  return global.readBuf[p] | global.readBuf[p + 1] << 8 | global.readBuf[p + 2] << 16 | p[global.readBuf + 3] << 24;
}

/* Expects global.dicSize be set already. Can be called before or after InitProp. */
void InitDecode(void) {
  /* global.lc = global.pb = global.lp = 0; */  /* needinitprop will initialize it */
  global.dicBufSize = 0;  /* We'll increment it later. */
  global.needInitDic = True;
  global.needInitState = True;
  global.needInitProp = True;
  global.dicPos = 0;
  LzmaDec_InitDicAndState(True, True);
}

SRes InitProp(Byte b) {
  UInt32 lc, lp;
  if (b >= (9 * 5 * 5)) return SZ_ERROR_BAD_LCLPPB_PROP;
  lc = b % 9;
  b /= 9;
  global.pb = b / 5;
  lp = b % 5;
  if (lc + lp > LZMA2_LCLP_MAX) return SZ_ERROR_BAD_LCLPPB_PROP;
  global.lc = lc;
  global.lp = lp;
  global.needInitProp = False;
  return SZ_OK;
}

#define FILTER_ID_LZMA2 0x21

/* Writes uncompressed data (global.dic[oldDicPos : global.dicPos] to stdout. */
SRes WriteFrom(UInt32 oldDicPos) {
  const Byte *q = global.dic + global.dicPos, *p = global.dic + oldDicPos;
  DEBUGF("WRITE %d dicPos=%d\n", (int)(q - p), global.dicPos);
  while (p != q) {
    const Int32 got = write(1, p, q - p);
    if (got <= 0) return SZ_ERROR_WRITE;
    p += got;
  }
  return SZ_OK;
}

/* Reads .xz or .lzma data from stdin, writes uncompressed bytes to stdout,
 * uses CLzmaDec.dic. It verifies some aspects of the file format (so it
 * can't be tricked to an infinite loop etc.), itdoesn't verify checksums
 * (e.g. CRC32).
 */
SRes DecompressXzOrLzma(void) {
  Byte checksumSize;
  UInt32 bhf;  /* Block header flags */
  /* 12 for the stream header + 12 for the first block header + 6 for the
   * first chunk header. empty.xz is 32 bytes.
   */
  if (Preread(12 + 12 + 6) < 12 + 12 + 6) return SZ_ERROR_INPUT_EOF;
  /* readbuf[7] is actually stream flags, should also be 0. */
  if (0 == memcmp(&global.readBuf[global.readCur], "\xFD""7zXZ\0", 7)) {  /* .xz */
  } else if (global.readBuf[global.readCur] <= 225 && global.readBuf[global.readCur + 13] == 0 &&  /* .lzma */
        /* High 4 bytes of uncompressed size. */
        ((bhf = GetLE4(global.readCur + 9)) == 0 || bhf == ~(UInt32)0) &&
        (global.dicSize = GetLE4(global.readCur + 1)) >= LZMA_DIC_MIN &&
        global.dicSize <= DIC_ARRAY_SIZE) {
    /* Based on https://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt */
    UInt32 us;
    UInt32 srcLen;
    UInt32 oldDicPos;
    InitDecode();
    /* LZMA restricts lc + lp <= 4. LZMA requires lc + lp <= 12.
     * We apply the LZMA2 restriction here (to save memory in
     * CLzmaDec.probs), thus we are not able to extract some legitimate
     * .lzma files.
     */
    RINOK(InitProp(global.readBuf[global.readCur]));
    if (bhf == 0) {
      global.dicBufSize = us = GetLE4(global.readCur + 5);
      if (us > DIC_ARRAY_SIZE) return SZ_ERROR_MEM;
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
      SRes res;
      oldDicPos = global.dicPos;
      res = LzmaDec_DecodeToDic(srcLen);
      DEBUGF("LZMADEC res=%d\n", res);
      if (global.dicPos > us) global.dicPos = us;
      RINOK(WriteFrom(oldDicPos));
      if (res == SZ_ERROR_FINISHED_WITH_MARK) break;
      if (res != SZ_ERROR_NEEDS_MORE_INPUT && res != SZ_OK) return res;
      if (global.dicPos == us) break;
    }
    return SZ_OK;
  } else {
    return SZ_ERROR_BAD_MAGIC;
  }
  /* Based on https://tukaani.org/xz/xz-file-format-1.0.4.txt */
  switch (global.readBuf[global.readCur + 7]) {
   case 0: /* None */ checksumSize = 1; break;
   case 1: /* CRC32 */ checksumSize = 4; break;
   case 4: /* CRC64, typical xz output. */ checksumSize = 8; break;
   default: return SZ_ERROR_BAD_CHECKSUM_TYPE;
  }
  /* Also ignore the CRC32 after checksumSize. */
  global.readCur += 12;
  for (;;) {  /* Next block. */
    /* We need it modulo 4, so a Byte is enough. */
    Byte blockSizePad = 3;
    UInt32 bhs, bhs2; /* Block header size */
    Byte dicSizeProp;
    UInt32 readAtBlock;
    ASSERT(global.readEnd - global.readCur >= 12);  /* At least 12 bytes preread. */
    readAtBlock = global.readCur;
    if ((bhs = global.readBuf[global.readCur++]) == 0) break;  /* Last block, index follows. */
    /* Block header size includes the bhs field above and the CRC32 below. */
    bhs = (bhs + 1) << 2;
    DEBUGF("bhs=%d\n", bhs);
    /* Typically the Preread(12 + 12 + 6) above covers it. */
    if (Preread(bhs) < bhs) return SZ_ERROR_INPUT_EOF;
    readAtBlock = global.readCur;
    bhf = global.readBuf[global.readCur++];
    if ((bhf & 2) != 0) return SZ_ERROR_UNSUPPORTED_FILTER_COUNT;
    DEBUGF("filter count=%d\n", (bhf & 2) + 1);
    if ((bhf & 20) != 0) return SZ_ERROR_BAD_BLOCK_FLAGS;
    if (bhf & 64) {  /* Compressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    if (bhf & 128) {  /* Uncompressed size present. */
      /* Usually not present, just ignore it. */
      IgnoreVarint();
    }
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (global.readBuf[global.readCur++] != FILTER_ID_LZMA2) return SZ_ERROR_UNSUPPORTED_FILTER_ID;
    /* This is actually a varint, but it's shorter to read it as a byte. */
    if (global.readBuf[global.readCur++] != 1) return SZ_ERROR_UNSUPPORTED_FILTER_PROPERTIES_SIZE;
    dicSizeProp = global.readBuf[global.readCur++];
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
    /* LZMA2 and .xz support it, we don't (for simpler memory management on
     * 32-bit systems).
     */
    if (dicSizeProp > 37) return SZ_ERROR_UNSUPPORTED_DICTIONARY_SIZE;
    global.dicSize = LZMA2_DIC_SIZE_FROM_SMALL_PROP(dicSizeProp);
    ASSERT(global.dicSize >= LZMA_DIC_MIN);
    DEBUGF("dicSize39=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(39));
    DEBUGF("dicSize38=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(38));
    DEBUGF("dicSize37=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(37));
    DEBUGF("dicSize36=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(36));
    DEBUGF("dicSize35=%u\n", LZMA2_DIC_SIZE_FROM_SMALL_PROP(35));
    bhs2 = global.readCur - readAtBlock + 5;  /* Won't overflow. */
    DEBUGF("bhs=%d bhs2=%d\n", bhs, bhs2);
    if (bhs2 > bhs) return SZ_ERROR_BLOCK_HEADER_TOO_LONG;
    RINOK(IgnoreZeroBytes(bhs - bhs2));
    global.readCur += 4;  /* Ignore CRC32. */
    /* Typically it's offset 24, xz creates it by default, minimal. */
    DEBUGF("LZMA2\n");
    { /* Parse LZMA2 stream. */
      /* Based on https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Markov_chain_algorithm#LZMA2_format */
      UInt32 us, cs;  /* Uncompressed and compressed chunk sizes. */
      InitDecode();

      for (;;) {
        Byte control;
        ASSERT(global.dicPos == global.dicBufSize);
        /* Actually 2 bytes is enough to get to the index if everything is
         * aligned and there is no block checksum.
         */
        if (Preread(6) < 6) return SZ_ERROR_INPUT_EOF;
        control = global.readBuf[global.readCur];
        DEBUGF("CONTROL control=0x%02x at=? inbuf=%d\n", control, global.readCur);
        if (control == 0) {
          DEBUGF("LASTFED\n");
          ++global.readCur;
          break;
        } else if ((Byte)(control - 3) < 0x80 - 3U) {
          return SZ_ERROR_BAD_CHUNK_CONTROL_BYTE;
        }
        us = (global.readBuf[global.readCur + 1] << 8) + global.readBuf[global.readCur + 2] + 1;
        if (control < 3) {  /* Uncompressed chunk. */
          const Bool initDic = (control == 1);
          cs = us;
          global.readCur += 3;
          blockSizePad -= 3;
          if (initDic) {
            global.needInitProp = global.needInitState = True;
            global.needInitDic = False;
          } else if (global.needInitDic) {
            return SZ_ERROR_DATA;
          }
          LzmaDec_InitDicAndState(initDic, False);
        } else {  /* LZMA chunk. */
          const Byte mode = LZMA2_GET_LZMA_MODE(control);
          const Bool initDic = (mode == 3);
          const Bool initState = (mode > 0);
          const Bool isProp = (control & 64) != 0;
          us += (control & 31) << 16;
          cs = (global.readBuf[global.readCur + 3] << 8) + global.readBuf[global.readCur + 4] + 1;
          if (isProp) {
            RINOK(InitProp(global.readBuf[global.readCur + 5]));
            ++global.readCur;
            --blockSizePad;
          } else {
            if (global.needInitProp) return SZ_ERROR_MISSING_INITPROP;
          }
          global.readCur += 5;
          blockSizePad -= 5;
          if ((!initDic && global.needInitDic) || (!initState && global.needInitState))
            return SZ_ERROR_DATA;
          LzmaDec_InitDicAndState(initDic, initState);
          global.needInitDic = False;
          global.needInitState = False;
        }
        ASSERT(global.dicPos == global.dicBufSize);
        global.dicBufSize += us;
        /* Decompressed data too long, won't fit to CLzmaDec.dic. */
        if (global.dicBufSize > DIC_ARRAY_SIZE) return SZ_ERROR_MEM;
        /* Read 6 extra bytes to optimize away a read(...) system call in
         * the Prefetch(6) call in the next chunk header.
         */
        if (Preread(cs + 6) < cs) return SZ_ERROR_INPUT_EOF;
        DEBUGF("FEED us=%d cs=%d dicPos=%d\n", us, cs, global.dicPos);
        if (control < 0x80) {  /* Uncompressed chunk. */
          DEBUGF("DECODE uncompressed\n");
          while (global.dicPos != global.dicBufSize) {
            global.dic[global.dicPos++] = global.readBuf[global.readCur++];
          }
          if (global.checkDicSize == 0 && global.dicSize - global.processedPos <= us)
            global.checkDicSize = global.dicSize;
          global.processedPos += us;
        } else {  /* Compressed chunk. */
          DEBUGF("DECODE call\n");
          /* This call doesn't change global.dicBufSize. */
          RINOK(LzmaDec_DecodeToDic(cs));
        }
        if (global.dicPos != global.dicBufSize) return SZ_ERROR_BAD_DICPOS;
        RINOK(WriteFrom(global.dicPos - us));
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
    if (Preread(7 + 12 + 6) < 7 + 12 + 6) return SZ_ERROR_INPUT_EOF;
    DEBUGF("ALTELL blockSizePad=%d\n", blockSizePad & 3);
    RINOK(IgnoreZeroBytes(blockSizePad & 3));  /* Ignore block padding. */
    global.readCur += checksumSize;  /* Ignore CRC32, CRC64 etc. */
  }
  /* The .xz input file continues with the index, which we ignore from here. */
  return SZ_OK;
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;
#if defined(MSDOS) || defined(_WIN32)  /* Also MinGW. Good. */
  setmode(0, O_BINARY);
  setmode(1, O_BINARY);
#endif
  return DecompressXzOrLzma();
}
