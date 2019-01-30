/* by pts@fazekas.hu at Wed Jan 30 15:15:23 CET 2019
 *
 * Based on https://github.com/pts/pts-tiny-7z-sfx/commit/b9a101b076672879f861d472665afaa6caa6fec1
 *
 * Can use: -DCONFIG_NO_INT64
 * Can use: -DCONFIG_NO_SIZE_T
 * Can use: -DCONFIG_PROB32
 * Can use: -DCONFIG_SIZE_OPT  (How much smaller does the code become?)
 *
 * $ xtiny gcc -s -Os -W -Wall -Wextra -o muxzcat muxzcat.c && ls -l muxzcat
 */

#ifdef __XTINY__

#include <xtiny.h>
typedef ssize_t ptrdiff_t;
typedef int64_t Int64;
typedef uint64_t UInt64;
typedef int32_t Int32;
typedef uint32_t UInt32;
#define UINT64_CONST(n) n ## ULL
/* !! malloc, free */

#else

#include <stddef.h>  /* size_t */
#include <stdlib.h>  /* malloc(), free() */
#include <string.h>  /* memcpy() */

#ifdef _WIN32
#include <windows.h>
#endif

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

/* --- Memory allocation */

#ifdef __XTINY__

static char malloc_buf[1000000], *malloc_next = malloc_buf;

/* !! get rid of SzAlloc */
static void *SzAlloc(size_t size) {
  char *result = malloc_next;
  malloc_next += size;
  return result;
}

static void SzFree(void *address) {
  (void)address;
}

#else
#define SzAlloc(size) malloc(size)
#define SzFree(address) free(address)
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
#define SZ_ERROR_BAD_FILENAME 13
#define SZ_ERROR_UNSAFE_FILENAME 14

#define SZ_ERROR_ARCHIVE 16
#define SZ_ERROR_NO_ARCHIVE 17

#define SZ_ERROR_OVERWRITE 21
#define SZ_ERROR_WRITE_OPEN 22
#define SZ_ERROR_WRITE_CHMOD 23
#define SZ_ERROR_WRITE_MKDIR 24
#define SZ_ERROR_WRITE_MKDIR_CHMOD 25
#define SZ_ERROR_WRITE_SYMLINK 26

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

#define LookToRead_BUF_SIZE (1 << 14)

typedef struct
{
  int fd;
  size_t pos;
  size_t size;
  Byte buf[LookToRead_BUF_SIZE];
} CLookToRead;

/* static void LookToRead_Init(CLookToRead *p) */
#define LOOKTOREAD_INIT(p) do { (p)->pos = (p)->size = 0; } while (0)
/* 1. If less than *size bytes are already in the input buffer, then fills the
 *    rest of the input buffer from disk.
 * 2. Sets *size to the number of bytes now in the input buffer. Can be more or
 *    less or equal to the original *size. Detect EOF by calling
 *    LOOKTOREAD_SKIP(*size), calling LookToRead_Look again, and then checking
 *    *size == 0.
 */
static SRes LookToRead_Look(CLookToRead *p, const void **buf, size_t *size);
/* offset must be <= output(*size) of Look */
/* static SRes LookToRead_Skip(CLookToRead *p, size_t offset) */
#define LOOKTOREAD_SKIP(p, offset) do { (p)->pos += (offset); } while (0)

/* CONFIG_PROB32 can increase the speed on some CPUs,
   but memory usage for CLzmaDec::probs will be doubled in that case
   !! how much is it typically?
   */
#ifdef CONFIG_PROB32
#define CLzmaProb UInt32
#else
#define CLzmaProb UInt16
#endif

#define LZMA_PROPS_SIZE 5

typedef struct _CLzmaProps
{
  unsigned lc, lp, pb;
  UInt32 dicSize;
} CLzmaProps;

static SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size);

#define LZMA_REQUIRED_INPUT_MAX 20

typedef struct
{
  CLzmaProps prop;
  CLzmaProb *probs;
  Byte *dic;
  const Byte *buf;
  UInt32 range, code;
  size_t dicPos;
  size_t dicBufSize;
  UInt32 processedPos;
  UInt32 checkDicSize;
  unsigned state;
  UInt32 reps[4];
  unsigned remainLen;
  int needFlush;
  int needInitState;
  UInt32 numProbs;
  unsigned tempBufSize;
  Byte tempBuf[LZMA_REQUIRED_INPUT_MAX];
} CLzmaDec;

#define LzmaDec_Construct(p) { (p)->dic = 0; (p)->probs = 0; }

static void LzmaDec_Init(CLzmaDec *p);

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

static SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize);
static void LzmaDec_FreeProbs(CLzmaDec *p);

static SRes LzmaDec_DecodeToDic(CLzmaDec *p, size_t dicLimit,
    const Byte *src, size_t *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

typedef struct
{
  CLzmaDec decoder;
  UInt32 packSize;
  UInt32 unpackSize;
  int state;
  Byte control;
  Bool needInitDic;
  Bool needInitState;
  Bool needInitProp;
} CLzma2Dec;

#define Lzma2Dec_Construct(p) LzmaDec_Construct(&(p)->decoder)
#define Lzma2Dec_FreeProbs(p) LzmaDec_FreeProbs(&(p)->decoder);
#define Lzma2Dec_Free(p) LzmaDec_Free(&(p)->decoder);

static void Lzma2Dec_Init(CLzma2Dec *p);

static SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, size_t dicLimit,
    const Byte *src, size_t *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status);

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

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

#define LzmaProps_GetNumProbs(p) ((UInt32)LZMA_BASE_SIZE + (LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))

#if Literal != LZMA_BASE_SIZE
StopCompilingDueBUG
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

static void LzmaDec_Init(CLzmaDec *p)
{
  p->dicPos = 0;
  LzmaDec_InitDicAndState(p, True, True);
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

static void LzmaDec_FreeProbs(CLzmaDec *p)
{
  SzFree(p->probs);
  p->probs = 0;
}

static SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size)
{
  UInt32 dicSize;
  Byte d;

  if (size < LZMA_PROPS_SIZE)
    return SZ_ERROR_UNSUPPORTED;
  else
    dicSize = data[1] | ((UInt32)data[2] << 8) | ((UInt32)data[3] << 16) | ((UInt32)data[4] << 24);

  if (dicSize < LZMA_DIC_MIN)
    dicSize = LZMA_DIC_MIN;
  p->dicSize = dicSize;

  d = data[0];
  if (d >= (9 * 5 * 5))
    return SZ_ERROR_UNSUPPORTED;

  p->lc = d % 9;
  d /= 9;
  p->pb = d / 5;
  p->lp = d % 5;

  return SZ_OK;
}

/* !! Merge with LzmaDec_AllocateProbs */
static SRes LzmaDec_AllocateProbs2(CLzmaDec *p, const CLzmaProps *propNew)
{
  UInt32 numProbs = LzmaProps_GetNumProbs(propNew);
  if (p->probs == 0 || numProbs != p->numProbs)
  {
    LzmaDec_FreeProbs(p);
    p->probs = (CLzmaProb *)SzAlloc(numProbs * sizeof(CLzmaProb));
    p->numProbs = numProbs;
    if (p->probs == 0)
      return SZ_ERROR_MEM;
  }
  return SZ_OK;
}

static SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize)
{
  CLzmaProps propNew;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize));
  RINOK(LzmaDec_AllocateProbs2(p, &propNew));
  p->prop = propNew;
  return SZ_OK;
}

#define LZMA2_CONTROL_LZMA (1 << 7)
#define LZMA2_CONTROL_COPY_NO_RESET 2
#define LZMA2_CONTROL_COPY_RESET_DIC 1
#define LZMA2_CONTROL_EOF 0

#define LZMA2_IS_UNCOMPRESSED_STATE(p) (((p)->control & LZMA2_CONTROL_LZMA) == 0)

#define LZMA2_GET_LZMA_MODE(p) (((p)->control >> 5) & 3)
#define LZMA2_IS_THERE_PROP(mode) ((mode) >= 2)

#define LZMA2_LCLP_MAX 4
#define LZMA2_DIC_SIZE_FROM_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

typedef enum
{
  LZMA2_STATE_CONTROL,
  LZMA2_STATE_UNPACK0,
  LZMA2_STATE_UNPACK1,
  LZMA2_STATE_PACK0,
  LZMA2_STATE_PACK1,
  LZMA2_STATE_PROP,
  LZMA2_STATE_DATA,
  LZMA2_STATE_DATA_CONT,
  LZMA2_STATE_FINISHED,
  LZMA2_STATE_ERROR
} ELzma2State;

static SRes Lzma2Dec_GetOldProps(Byte prop, Byte *props)
{
  UInt32 dicSize;
  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;
  dicSize = (prop == 40) ? 0xFFFFFFFF : LZMA2_DIC_SIZE_FROM_PROP(prop);
  props[0] = (Byte)LZMA2_LCLP_MAX;
  props[1] = (Byte)(dicSize);
  props[2] = (Byte)(dicSize >> 8);
  props[3] = (Byte)(dicSize >> 16);
  props[4] = (Byte)(dicSize >> 24);
  return SZ_OK;
}

static void Lzma2Dec_Init(CLzma2Dec *p)
{
  p->state = LZMA2_STATE_CONTROL;
  p->needInitDic = True;
  p->needInitState = True;
  p->needInitProp = True;
  LzmaDec_Init(&p->decoder);
}

static ELzma2State Lzma2Dec_UpdateState(CLzma2Dec *p, Byte b)
{
  switch(p->state)
  {
    case LZMA2_STATE_CONTROL:
      p->control = b;
      if (p->control == 0)
        return LZMA2_STATE_FINISHED;
      if (LZMA2_IS_UNCOMPRESSED_STATE(p))
      {
        if ((p->control & 0x7F) > 2)
          return LZMA2_STATE_ERROR;
        p->unpackSize = 0;
      }
      else
        p->unpackSize = (UInt32)(p->control & 0x1F) << 16;
      return LZMA2_STATE_UNPACK0;

    case LZMA2_STATE_UNPACK0:
      p->unpackSize |= (UInt32)b << 8;
      return LZMA2_STATE_UNPACK1;

    case LZMA2_STATE_UNPACK1:
      p->unpackSize |= (UInt32)b;
      p->unpackSize++;
      return (LZMA2_IS_UNCOMPRESSED_STATE(p)) ? LZMA2_STATE_DATA : LZMA2_STATE_PACK0;

    case LZMA2_STATE_PACK0:
      p->packSize = (UInt32)b << 8;
      return LZMA2_STATE_PACK1;

    case LZMA2_STATE_PACK1:
      p->packSize |= (UInt32)b;
      p->packSize++;
      return LZMA2_IS_THERE_PROP(LZMA2_GET_LZMA_MODE(p)) ? LZMA2_STATE_PROP:
        (p->needInitProp ? LZMA2_STATE_ERROR : LZMA2_STATE_DATA);

    case LZMA2_STATE_PROP:
    {
      int lc, lp;
      if (b >= (9 * 5 * 5))
        return LZMA2_STATE_ERROR;
      lc = b % 9;
      b /= 9;
      p->decoder.prop.pb = b / 5;
      lp = b % 5;
      if (lc + lp > LZMA2_LCLP_MAX)
        return LZMA2_STATE_ERROR;
      p->decoder.prop.lc = lc;
      p->decoder.prop.lp = lp;
      p->needInitProp = False;
      return LZMA2_STATE_DATA;
    }
  }
  return LZMA2_STATE_ERROR;
}

static void LzmaDec_UpdateWithUncompressed(CLzmaDec *p, const Byte *src, size_t size)
{
  memcpy(p->dic + p->dicPos, src, size);
  p->dicPos += size;
  if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= size)
    p->checkDicSize = p->prop.dicSize;
  p->processedPos += (UInt32)size;
}

static void LzmaDec_InitDicAndState(CLzmaDec *p, Bool initDic, Bool initState);

static SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, size_t dicLimit,
    const Byte *src, size_t *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  size_t inSize = *srcLen;
  *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;

  while (p->state != LZMA2_STATE_FINISHED)
  {
    size_t dicPos = p->decoder.dicPos;
    if (p->state == LZMA2_STATE_ERROR)
      return SZ_ERROR_DATA;
    if (dicPos == dicLimit && finishMode == LZMA_FINISH_ANY)
    {
      *status = LZMA_STATUS_NOT_FINISHED;
      return SZ_OK;
    }
    if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT)
    {
      if (*srcLen == inSize)
      {
        *status = LZMA_STATUS_NEEDS_MORE_INPUT;
        return SZ_OK;
      }
      (*srcLen)++;
      p->state = Lzma2Dec_UpdateState(p, *src++);
      continue;
    }
    {
      size_t destSizeCur = dicLimit - dicPos;
      size_t srcSizeCur = inSize - *srcLen;
      ELzmaFinishMode curFinishMode = LZMA_FINISH_ANY;

      if (p->unpackSize <= destSizeCur)
      {
        destSizeCur = (size_t)p->unpackSize;
        curFinishMode = LZMA_FINISH_END;
      }

      if (LZMA2_IS_UNCOMPRESSED_STATE(p))
      {
        if (*srcLen == inSize)
        {
          *status = LZMA_STATUS_NEEDS_MORE_INPUT;
          return SZ_OK;
        }

        if (p->state == LZMA2_STATE_DATA)
        {
          Bool initDic = (p->control == LZMA2_CONTROL_COPY_RESET_DIC);
          if (initDic)
            p->needInitProp = p->needInitState = True;
          else if (p->needInitDic)
            return SZ_ERROR_DATA;
          p->needInitDic = False;
          LzmaDec_InitDicAndState(&p->decoder, initDic, False);
        }

        if (srcSizeCur > destSizeCur)
          srcSizeCur = destSizeCur;

        if (srcSizeCur == 0)
          return SZ_ERROR_DATA;

        LzmaDec_UpdateWithUncompressed(&p->decoder, src, srcSizeCur);

        src += srcSizeCur;
        *srcLen += srcSizeCur;
        p->unpackSize -= (UInt32)srcSizeCur;
        p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
      }
      else
      {
        size_t outSizeProcessed;
        SRes res;

        if (p->state == LZMA2_STATE_DATA)
        {
          int mode = LZMA2_GET_LZMA_MODE(p);
          Bool initDic = (mode == 3);
          Bool initState = (mode > 0);
          if ((!initDic && p->needInitDic) || (!initState && p->needInitState))
            return SZ_ERROR_DATA;

          LzmaDec_InitDicAndState(&p->decoder, initDic, initState);
          p->needInitDic = False;
          p->needInitState = False;
          p->state = LZMA2_STATE_DATA_CONT;
        }
        if (srcSizeCur > p->packSize)
          srcSizeCur = (size_t)p->packSize;

        res = LzmaDec_DecodeToDic(&p->decoder, dicPos + destSizeCur, src, &srcSizeCur, curFinishMode, status);

        src += srcSizeCur;
        *srcLen += srcSizeCur;
        p->packSize -= (UInt32)srcSizeCur;

        outSizeProcessed = p->decoder.dicPos - dicPos;
        p->unpackSize -= (UInt32)outSizeProcessed;

        RINOK(res);
        if (*status == LZMA_STATUS_NEEDS_MORE_INPUT)
          return res;

        if (srcSizeCur == 0 && outSizeProcessed == 0)
        {
          if (*status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK ||
              p->unpackSize != 0 || p->packSize != 0)
            return SZ_ERROR_DATA;
          p->state = LZMA2_STATE_CONTROL;
        }
        if (*status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK)
          *status = LZMA_STATUS_NOT_FINISHED;
      }
    }
  }
  *status = LZMA_STATUS_FINISHED_WITH_MARK;
  return SZ_OK;
}

/* !! Replace with real inplementation */
static SRes SzDecodeLzma2(Byte prop, UInt64 inSize, CLookToRead *inStream,
    Byte *outBuffer, size_t outSize)
{
  CLzma2Dec state;
  SRes res = SZ_OK;

  Lzma2Dec_Construct(&state);
  {
    Byte props[LZMA_PROPS_SIZE];
    RINOK(Lzma2Dec_GetOldProps(prop, props));
    RINOK(LzmaDec_AllocateProbs(&state.decoder, props, LZMA_PROPS_SIZE));
  }
  state.decoder.dic = outBuffer;
  state.decoder.dicBufSize = outSize;
  Lzma2Dec_Init(&state);

  for (;;)
  {
    Byte *inBuf = NULL;
    size_t inProcessed = inSize > LookToRead_BUF_SIZE ?
        LookToRead_BUF_SIZE : inSize;
    res = LookToRead_Look(inStream, (const void **)&inBuf, &inProcessed);
    if (res != SZ_OK)
      break;

    {
      size_t dicPos = state.decoder.dicPos;
      ELzmaStatus status;
      res = Lzma2Dec_DecodeToDic(&state, outSize, inBuf, &inProcessed, LZMA_FINISH_END, &status);
      inSize -= inProcessed;
      if (res != SZ_OK)
        break;
      if (state.decoder.dicPos == state.decoder.dicBufSize || (inProcessed == 0 && dicPos == state.decoder.dicPos))
      {
        if (state.decoder.dicBufSize != outSize || inSize != 0 ||
            (status != LZMA_STATUS_FINISHED_WITH_MARK))
          res = SZ_ERROR_DATA;
        break;
      }
      LOOKTOREAD_SKIP(inStream, inProcessed);
    }
  }

  Lzma2Dec_FreeProbs(&state);
  return res;
}

/* !! Replace with real implementation */
static SRes LookToRead_Look(CLookToRead *p, const void **buf, size_t *size) {
  (void)p;
  (void)buf;
  *size += 42;
  return SZ_OK;
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  const UInt64 inSize = 2000;
  CLookToRead inStream;
  Byte outBuffer[10000];
  const size_t outSize = sizeof(outBuffer);
  const Byte prop = 0x90;
  SzDecodeLzma2(prop, inSize, &inStream, outBuffer, outSize);
  return 0;
}
