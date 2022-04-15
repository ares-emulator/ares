/* LzmaEnc.c -- LZMA Encoder
2019-01-10: Igor Pavlov : Public domain */

#include "Precomp.h"

#include <string.h>

/* #define SHOW_STAT */
/* #define SHOW_STAT2 */

#if defined(SHOW_STAT) || defined(SHOW_STAT2)
#include <stdio.h>
#endif

#include "LzmaEnc.h"

#include "LzFind.h"
#ifndef _7ZIP_ST
#include "LzFindMt.h"
#endif

#ifdef SHOW_STAT
static unsigned g_STAT_OFFSET = 0;
#endif

#define kLzmaMaxHistorySize ((UInt32)3 << 29)
/* #define kLzmaMaxHistorySize ((UInt32)7 << 29) */

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5
#define kProbInitValue (kBitModelTotal >> 1)

#define kNumMoveReducingBits 4
#define kNumBitPriceShiftBits 4
#define kBitPrice (1 << kNumBitPriceShiftBits)

#define REP_LEN_COUNT 64

void LzmaEncProps_Init(CLzmaEncProps *p)
{
  p->level = 5;
  p->dictSize = p->mc = 0;
  p->reduceSize = (UInt64)(Int64)-1;
  p->lc = p->lp = p->pb = p->algo = p->fb = p->btMode = p->numHashBytes = p->numThreads = -1;
  p->writeEndMark = 0;
}

void LzmaEncProps_Normalize(CLzmaEncProps *p)
{
  int level = p->level;
  if (level < 0) level = 5;
  p->level = level;
  
  if (p->dictSize == 0) p->dictSize = (level <= 5 ? (1 << (level * 2 + 14)) : (level <= 7 ? (1 << 25) : (1 << 26)));
  if (p->dictSize > p->reduceSize)
  {
    unsigned i;
    UInt32 reduceSize = (UInt32)p->reduceSize;
    for (i = 11; i <= 30; i++)
    {
      if (reduceSize <= ((UInt32)2 << i)) { p->dictSize = ((UInt32)2 << i); break; }
      if (reduceSize <= ((UInt32)3 << i)) { p->dictSize = ((UInt32)3 << i); break; }
    }
  }

  if (p->lc < 0) p->lc = 3;
  if (p->lp < 0) p->lp = 0;
  if (p->pb < 0) p->pb = 2;

  if (p->algo < 0) p->algo = (level < 5 ? 0 : 1);
  if (p->fb < 0) p->fb = (level < 7 ? 32 : 64);
  if (p->btMode < 0) p->btMode = (p->algo == 0 ? 0 : 1);
  if (p->numHashBytes < 0) p->numHashBytes = 4;
  if (p->mc == 0) p->mc = (16 + (p->fb >> 1)) >> (p->btMode ? 0 : 1);
  
  if (p->numThreads < 0)
    p->numThreads =
      #ifndef _7ZIP_ST
      ((p->btMode && p->algo) ? 2 : 1);
      #else
      1;
      #endif
}

UInt32 LzmaEncProps_GetDictSize(const CLzmaEncProps *props2)
{
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);
  return props.dictSize;
}

#if (_MSC_VER >= 1400)
/* BSR code is fast for some new CPUs */
/* #define LZMA_LOG_BSR */
#endif

#ifdef LZMA_LOG_BSR

#define kDicLogSizeMaxCompress 32

#define BSR2_RET(pos, res) { unsigned long zz; _BitScanReverse(&zz, (pos)); res = (zz + zz) + ((pos >> (zz - 1)) & 1); }

static unsigned GetPosSlot1(UInt32 pos)
{
  unsigned res;
  BSR2_RET(pos, res);
  return res;
}
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < 2) res = pos; else BSR2_RET(pos, res); }

#else

#define kNumLogBits (9 + sizeof(size_t) / 2)
/* #define kNumLogBits (11 + sizeof(size_t) / 8 * 3) */

#define kDicLogSizeMaxCompress ((kNumLogBits - 1) * 2 + 7)

static void LzmaEnc_FastPosInit(Byte *g_FastPos)
{
  unsigned slot;
  g_FastPos[0] = 0;
  g_FastPos[1] = 1;
  g_FastPos += 2;
  
  for (slot = 2; slot < kNumLogBits * 2; slot++)
  {
    size_t k = ((size_t)1 << ((slot >> 1) - 1));
    size_t j;
    for (j = 0; j < k; j++)
      g_FastPos[j] = (Byte)slot;
    g_FastPos += k;
  }
}

/* we can use ((limit - pos) >> 31) only if (pos < ((UInt32)1 << 31)) */
/*
#define BSR2_RET(pos, res) { unsigned zz = 6 + ((kNumLogBits - 1) & \
  (0 - (((((UInt32)1 << (kNumLogBits + 6)) - 1) - pos) >> 31))); \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }
*/

/*
#define BSR2_RET(pos, res) { unsigned zz = 6 + ((kNumLogBits - 1) & \
  (0 - (((((UInt32)1 << (kNumLogBits)) - 1) - (pos >> 6)) >> 31))); \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }
*/

#define BSR2_RET(pos, res) { unsigned zz = (pos < (1 << (kNumLogBits + 6))) ? 6 : 6 + kNumLogBits - 1; \
  res = p->g_FastPos[pos >> zz] + (zz * 2); }

/*
#define BSR2_RET(pos, res) { res = (pos < (1 << (kNumLogBits + 6))) ? \
  p->g_FastPos[pos >> 6] + 12 : \
  p->g_FastPos[pos >> (6 + kNumLogBits - 1)] + (6 + (kNumLogBits - 1)) * 2; }
*/

#define GetPosSlot1(pos) p->g_FastPos[pos]
#define GetPosSlot2(pos, res) { BSR2_RET(pos, res); }
#define GetPosSlot(pos, res) { if (pos < kNumFullDistances) res = p->g_FastPos[pos & (kNumFullDistances - 1)]; else BSR2_RET(pos, res); }

#endif


#define LZMA_NUM_REPS 4

typedef UInt16 CState;
typedef UInt16 CExtra;

typedef struct
{
  UInt32 price;
  CState state;
  CExtra extra;
      // 0   : normal
      // 1   : LIT : MATCH
      // > 1 : MATCH (extra-1) : LIT : REP0 (len)
  UInt32 len;
  UInt32 dist;
  UInt32 reps[LZMA_NUM_REPS];
} COptimal;


// 18.06
#define kNumOpts (1 << 11)
#define kPackReserve (kNumOpts * 8)
// #define kNumOpts (1 << 12)
// #define kPackReserve (1 + kNumOpts * 2)

#define kNumLenToPosStates 4
#define kNumPosSlotBits 6
#define kDicLogSizeMin 0
#define kDicLogSizeMax 32
#define kDistTableSizeMax (kDicLogSizeMax * 2)

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)
#define kAlignMask (kAlignTableSize - 1)

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

typedef
#ifdef _LZMA_PROB32
  UInt32
#else
  UInt16
#endif
  CLzmaProb;

#define LZMA_PB_MAX 4
#define LZMA_LC_MAX 8
#define LZMA_LP_MAX 4

#define LZMA_NUM_PB_STATES_MAX (1 << LZMA_PB_MAX)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)
#define kLenNumSymbolsTotal (kLenNumLowSymbols * 2 + kLenNumHighSymbols)

#define LZMA_MATCH_LEN_MIN 2
#define LZMA_MATCH_LEN_MAX (LZMA_MATCH_LEN_MIN + kLenNumSymbolsTotal - 1)

#define kNumStates 12


typedef struct
{
  CLzmaProb low[LZMA_NUM_PB_STATES_MAX << (kLenNumLowBits + 1)];
  CLzmaProb high[kLenNumHighSymbols];
} CLenEnc;


typedef struct
{
  unsigned tableSize;
  UInt32 prices[LZMA_NUM_PB_STATES_MAX][kLenNumSymbolsTotal];
  // UInt32 prices1[LZMA_NUM_PB_STATES_MAX][kLenNumLowSymbols * 2];
  // UInt32 prices2[kLenNumSymbolsTotal];
} CLenPriceEnc;

#define GET_PRICE_LEN(p, posState, len) \
    ((p)->prices[posState][(size_t)(len) - LZMA_MATCH_LEN_MIN])

/*
#define GET_PRICE_LEN(p, posState, len) \
    ((p)->prices2[(size_t)(len) - 2] + ((p)->prices1[posState][((len) - 2) & (kLenNumLowSymbols * 2 - 1)] & (((len) - 2 - kLenNumLowSymbols * 2) >> 9)))
*/

typedef struct
{
  UInt32 range;
  unsigned cache;
  UInt64 low;
  UInt64 cacheSize;
  Byte *buf;
  Byte *bufLim;
  Byte *bufBase;
  ISeqOutStream *outStream;
  UInt64 processed;
  SRes res;
} CRangeEnc;


typedef struct
{
  CLzmaProb *litProbs;

  unsigned state;
  UInt32 reps[LZMA_NUM_REPS];

  CLzmaProb posAlignEncoder[1 << kNumAlignBits];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];

  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances];
  
  CLenEnc lenProbs;
  CLenEnc repLenProbs;

} CSaveState;


typedef UInt32 CProbPrice;


typedef struct
{
  void *matchFinderObj;
  IMatchFinder matchFinder;

  unsigned optCur;
  unsigned optEnd;

  unsigned longestMatchLen;
  unsigned numPairs;
  UInt32 numAvail;

  unsigned state;
  unsigned numFastBytes;
  unsigned additionalOffset;
  UInt32 reps[LZMA_NUM_REPS];
  unsigned lpMask, pbMask;
  CLzmaProb *litProbs;
  CRangeEnc rc;

  UInt32 backRes;

  unsigned lc, lp, pb;
  unsigned lclp;

  BoolInt fastMode;
  BoolInt writeEndMark;
  BoolInt finished;
  BoolInt multiThread;
  BoolInt needInit;
  // BoolInt _maxMode;

  UInt64 nowPos64;
  
  unsigned matchPriceCount;
  // unsigned alignPriceCount;
  int repLenEncCounter;

  unsigned distTableSize;

  UInt32 dictSize;
  SRes result;

  #ifndef _7ZIP_ST
  BoolInt mtMode;
  // begin of CMatchFinderMt is used in LZ thread
  CMatchFinderMt matchFinderMt;
  // end of CMatchFinderMt is used in BT and HASH threads
  #endif

  CMatchFinder matchFinderBase;

  #ifndef _7ZIP_ST
  Byte pad[128];
  #endif
  
  // LZ thread
  CProbPrice ProbPrices[kBitModelTotal >> kNumMoveReducingBits];

  UInt32 matches[LZMA_MATCH_LEN_MAX * 2 + 2 + 1];

  UInt32 alignPrices[kAlignTableSize];
  UInt32 posSlotPrices[kNumLenToPosStates][kDistTableSizeMax];
  UInt32 distancesPrices[kNumLenToPosStates][kNumFullDistances];

  CLzmaProb posAlignEncoder[1 << kNumAlignBits];
  CLzmaProb isRep[kNumStates];
  CLzmaProb isRepG0[kNumStates];
  CLzmaProb isRepG1[kNumStates];
  CLzmaProb isRepG2[kNumStates];
  CLzmaProb isMatch[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb isRep0Long[kNumStates][LZMA_NUM_PB_STATES_MAX];
  CLzmaProb posSlotEncoder[kNumLenToPosStates][1 << kNumPosSlotBits];
  CLzmaProb posEncoders[kNumFullDistances];
  
  CLenEnc lenProbs;
  CLenEnc repLenProbs;

  #ifndef LZMA_LOG_BSR
  Byte g_FastPos[1 << kNumLogBits];
  #endif

  CLenPriceEnc lenEnc;
  CLenPriceEnc repLenEnc;

  COptimal opt[kNumOpts];

  CSaveState saveState;

  #ifndef _7ZIP_ST
  Byte pad2[128];
  #endif
} CLzmaEnc;


SRes LzmaEnc_SetProps(CLzmaEncHandle pp, const CLzmaEncProps *props2)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  CLzmaEncProps props = *props2;
  LzmaEncProps_Normalize(&props);

  if (props.lc > LZMA_LC_MAX
      || props.lp > LZMA_LP_MAX
      || props.pb > LZMA_PB_MAX
      || props.dictSize > ((UInt64)1 << kDicLogSizeMaxCompress)
      || props.dictSize > kLzmaMaxHistorySize)
    return SZ_ERROR_PARAM;

  p->dictSize = props.dictSize;
  {
    unsigned fb = props.fb;
    if (fb < 5)
      fb = 5;
    if (fb > LZMA_MATCH_LEN_MAX)
      fb = LZMA_MATCH_LEN_MAX;
    p->numFastBytes = fb;
  }
  p->lc = props.lc;
  p->lp = props.lp;
  p->pb = props.pb;
  p->fastMode = (props.algo == 0);
  // p->_maxMode = True;
  p->matchFinderBase.btMode = (Byte)(props.btMode ? 1 : 0);
  {
    unsigned numHashBytes = 4;
    if (props.btMode)
    {
      if (props.numHashBytes < 2)
        numHashBytes = 2;
      else if (props.numHashBytes < 4)
        numHashBytes = props.numHashBytes;
    }
    p->matchFinderBase.numHashBytes = numHashBytes;
  }

  p->matchFinderBase.cutValue = props.mc;

  p->writeEndMark = props.writeEndMark;

  #ifndef _7ZIP_ST
  /*
  if (newMultiThread != _multiThread)
  {
    ReleaseMatchFinder();
    _multiThread = newMultiThread;
  }
  */
  p->multiThread = (props.numThreads > 1);
  #endif

  return SZ_OK;
}


void LzmaEnc_SetDataSize(CLzmaEncHandle pp, UInt64 expectedDataSiize)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  p->matchFinderBase.expectedDataSize = expectedDataSiize;
}


#define kState_Start 0
#define kState_LitAfterMatch 4
#define kState_LitAfterRep   5
#define kState_MatchAfterLit 7
#define kState_RepAfterLit   8

static const Byte kLiteralNextStates[kNumStates] = {0, 0, 0, 0, 1, 2, 3, 4,  5,  6,   4, 5};
static const Byte kMatchNextStates[kNumStates]   = {7, 7, 7, 7, 7, 7, 7, 10, 10, 10, 10, 10};
static const Byte kRepNextStates[kNumStates]     = {8, 8, 8, 8, 8, 8, 8, 11, 11, 11, 11, 11};
static const Byte kShortRepNextStates[kNumStates]= {9, 9, 9, 9, 9, 9, 9, 11, 11, 11, 11, 11};

#define IsLitState(s) ((s) < 7)
#define GetLenToPosState2(len) (((len) < kNumLenToPosStates - 1) ? (len) : kNumLenToPosStates - 1)
#define GetLenToPosState(len) (((len) < kNumLenToPosStates + 1) ? (len) - 2 : kNumLenToPosStates - 1)

#define kInfinityPrice (1 << 30)

static void RangeEnc_Construct(CRangeEnc *p)
{
  p->outStream = NULL;
  p->bufBase = NULL;
}

#define RangeEnc_GetProcessed(p)       ((p)->processed + ((p)->buf - (p)->bufBase) + (p)->cacheSize)
#define RangeEnc_GetProcessed_sizet(p) ((size_t)(p)->processed + ((p)->buf - (p)->bufBase) + (size_t)(p)->cacheSize)

#define RC_BUF_SIZE (1 << 16)

static int RangeEnc_Alloc(CRangeEnc *p, ISzAllocPtr alloc)
{
  if (!p->bufBase)
  {
    p->bufBase = (Byte *)ISzAlloc_Alloc(alloc, RC_BUF_SIZE);
    if (!p->bufBase)
      return 0;
    p->bufLim = p->bufBase + RC_BUF_SIZE;
  }
  return 1;
}

static void RangeEnc_Free(CRangeEnc *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->bufBase);
  p->bufBase = 0;
}

static void RangeEnc_Init(CRangeEnc *p)
{
  /* Stream.Init(); */
  p->range = 0xFFFFFFFF;
  p->cache = 0;
  p->low = 0;
  p->cacheSize = 0;

  p->buf = p->bufBase;

  p->processed = 0;
  p->res = SZ_OK;
}

MY_NO_INLINE static void RangeEnc_FlushStream(CRangeEnc *p)
{
  size_t num;
  if (p->res != SZ_OK)
    return;
  num = p->buf - p->bufBase;
  if (num != ISeqOutStream_Write(p->outStream, p->bufBase, num))
    p->res = SZ_ERROR_WRITE;
  p->processed += num;
  p->buf = p->bufBase;
}

MY_NO_INLINE static void MY_FAST_CALL RangeEnc_ShiftLow(CRangeEnc *p)
{
  UInt32 low = (UInt32)p->low;
  unsigned high = (unsigned)(p->low >> 32);
  p->low = (UInt32)(low << 8);
  if (low < (UInt32)0xFF000000 || high != 0)
  {
    {
      Byte *buf = p->buf;
      *buf++ = (Byte)(p->cache + high);
      p->cache = (unsigned)(low >> 24);
      p->buf = buf;
      if (buf == p->bufLim)
        RangeEnc_FlushStream(p);
      if (p->cacheSize == 0)
        return;
    }
    high += 0xFF;
    for (;;)
    {
      Byte *buf = p->buf;
      *buf++ = (Byte)(high);
      p->buf = buf;
      if (buf == p->bufLim)
        RangeEnc_FlushStream(p);
      if (--p->cacheSize == 0)
        return;
    }
  }
  p->cacheSize++;
}

static void RangeEnc_FlushData(CRangeEnc *p)
{
  int i;
  for (i = 0; i < 5; i++)
    RangeEnc_ShiftLow(p);
}

#define RC_NORM(p) if (range < kTopValue) { range <<= 8; RangeEnc_ShiftLow(p); }

#define RC_BIT_PRE(p, prob) \
  ttt = *(prob); \
  newBound = (range >> kNumBitModelTotalBits) * ttt;

// #define _LZMA_ENC_USE_BRANCH

#ifdef _LZMA_ENC_USE_BRANCH

#define RC_BIT(p, prob, bit) { \
  RC_BIT_PRE(p, prob) \
  if (bit == 0) { range = newBound; ttt += (kBitModelTotal - ttt) >> kNumMoveBits; } \
  else { (p)->low += newBound; range -= newBound; ttt -= ttt >> kNumMoveBits; } \
  *(prob) = (CLzmaProb)ttt; \
  RC_NORM(p) \
  }

#else

#define RC_BIT(p, prob, bit) { \
  UInt32 mask; \
  RC_BIT_PRE(p, prob) \
  mask = 0 - (UInt32)bit; \
  range &= mask; \
  mask &= newBound; \
  range -= mask; \
  (p)->low += mask; \
  mask = (UInt32)bit - 1; \
  range += newBound & mask; \
  mask &= (kBitModelTotal - ((1 << kNumMoveBits) - 1)); \
  mask += ((1 << kNumMoveBits) - 1); \
  ttt += (Int32)(mask - ttt) >> kNumMoveBits; \
  *(prob) = (CLzmaProb)ttt; \
  RC_NORM(p) \
  }

#endif




#define RC_BIT_0_BASE(p, prob) \
  range = newBound; *(prob) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));

#define RC_BIT_1_BASE(p, prob) \
  range -= newBound; (p)->low += newBound; *(prob) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits)); \

#define RC_BIT_0(p, prob) \
  RC_BIT_0_BASE(p, prob) \
  RC_NORM(p)

#define RC_BIT_1(p, prob) \
  RC_BIT_1_BASE(p, prob) \
  RC_NORM(p)

static void RangeEnc_EncodeBit_0(CRangeEnc *p, CLzmaProb *prob)
{
  UInt32 range, ttt, newBound;
  range = p->range;
  RC_BIT_PRE(p, prob)
  RC_BIT_0(p, prob)
  p->range = range;
}

static void LitEnc_Encode(CRangeEnc *p, CLzmaProb *probs, UInt32 sym)
{
  UInt32 range = p->range;
  sym |= 0x100;
  do
  {
    UInt32 ttt, newBound;
    // RangeEnc_EncodeBit(p, probs + (sym >> 8), (sym >> 7) & 1);
    CLzmaProb *prob = probs + (sym >> 8);
    UInt32 bit = (sym >> 7) & 1;
    sym <<= 1;
    RC_BIT(p, prob, bit);
  }
  while (sym < 0x10000);
  p->range = range;
}

static void LzmaEnc_InitPriceTables(CProbPrice *ProbPrices)
{
  UInt32 i;
  for (i = 0; i < (kBitModelTotal >> kNumMoveReducingBits); i++)
  {
    const unsigned kCyclesBits = kNumBitPriceShiftBits;
    UInt32 w = (i << kNumMoveReducingBits) + (1 << (kNumMoveReducingBits - 1));
    unsigned bitCount = 0;
    unsigned j;
    for (j = 0; j < kCyclesBits; j++)
    {
      w = w * w;
      bitCount <<= 1;
      while (w >= ((UInt32)1 << 16))
      {
        w >>= 1;
        bitCount++;
      }
    }
    ProbPrices[i] = (CProbPrice)((kNumBitModelTotalBits << kCyclesBits) - 15 - bitCount);
    // printf("\n%3d: %5d", i, ProbPrices[i]);
  }
}


#define GET_PRICE(prob, bit) \
  p->ProbPrices[((prob) ^ (unsigned)(((-(int)(bit))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICEa(prob, bit) \
     ProbPrices[((prob) ^ (unsigned)((-((int)(bit))) & (kBitModelTotal - 1))) >> kNumMoveReducingBits];

#define GET_PRICE_0(prob) p->ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICE_1(prob) p->ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]

#define GET_PRICEa_0(prob) ProbPrices[(prob) >> kNumMoveReducingBits]
#define GET_PRICEa_1(prob) ProbPrices[((prob) ^ (kBitModelTotal - 1)) >> kNumMoveReducingBits]


static UInt32 LitEnc_GetPrice(const CLzmaProb *probs, UInt32 sym, const CProbPrice *ProbPrices)
{
  UInt32 price = 0;
  sym |= 0x100;
  do
  {
    unsigned bit = sym & 1;
    sym >>= 1;
    price += GET_PRICEa(probs[sym], bit);
  }
  while (sym >= 2);
  return price;
}


static UInt32 LitEnc_Matched_GetPrice(const CLzmaProb *probs, UInt32 sym, UInt32 matchByte, const CProbPrice *ProbPrices)
{
  UInt32 price = 0;
  UInt32 offs = 0x100;
  sym |= 0x100;
  do
  {
    matchByte <<= 1;
    price += GET_PRICEa(probs[offs + (matchByte & offs) + (sym >> 8)], (sym >> 7) & 1);
    sym <<= 1;
    offs &= ~(matchByte ^ sym);
  }
  while (sym < 0x10000);
  return price;
}



static void LenEnc_Init(CLenEnc *p)
{
  unsigned i;
  for (i = 0; i < (LZMA_NUM_PB_STATES_MAX << (kLenNumLowBits + 1)); i++)
    p->low[i] = kProbInitValue;
  for (i = 0; i < kLenNumHighSymbols; i++)
    p->high[i] = kProbInitValue;
}

static void LenEnc_Encode(CLenEnc *p, CRangeEnc *rc, unsigned sym, unsigned posState)
{
  UInt32 range, ttt, newBound;
  CLzmaProb *probs = p->low;
  range = rc->range;
  RC_BIT_PRE(rc, probs);
  if (sym >= kLenNumLowSymbols)
  {
    RC_BIT_1(rc, probs);
    probs += kLenNumLowSymbols;
    RC_BIT_PRE(rc, probs);
    if (sym >= kLenNumLowSymbols * 2)
    {
      RC_BIT_1(rc, probs);
      rc->range = range;
      // RcTree_Encode(rc, p->high, kLenNumHighBits, sym - kLenNumLowSymbols * 2);
      LitEnc_Encode(rc, p->high, sym - kLenNumLowSymbols * 2);
      return;
    }
    sym -= kLenNumLowSymbols;
  }

  // RcTree_Encode(rc, probs + (posState << kLenNumLowBits), kLenNumLowBits, sym);
  {
    unsigned m;
    unsigned bit;
    RC_BIT_0(rc, probs);
    probs += (posState << (1 + kLenNumLowBits));
    bit = (sym >> 2)    ; RC_BIT(rc, probs + 1, bit); m = (1 << 1) + bit;
    bit = (sym >> 1) & 1; RC_BIT(rc, probs + m, bit); m = (m << 1) + bit;
    bit =  sym       & 1; RC_BIT(rc, probs + m, bit);
    rc->range = range;
  }
}

static void SetPrices_3(const CLzmaProb *probs, UInt32 startPrice, UInt32 *prices, const CProbPrice *ProbPrices)
{
  unsigned i;
  for (i = 0; i < 8; i += 2)
  {
    UInt32 price = startPrice;
    UInt32 prob;
    price += GET_PRICEa(probs[1           ], (i >> 2));
    price += GET_PRICEa(probs[2 + (i >> 2)], (i >> 1) & 1);
    prob = probs[4 + (i >> 1)];
    prices[i    ] = price + GET_PRICEa_0(prob);
    prices[i + 1] = price + GET_PRICEa_1(prob);
  }
}


MY_NO_INLINE static void MY_FAST_CALL LenPriceEnc_UpdateTables(
    CLenPriceEnc *p,
    unsigned numPosStates,
    const CLenEnc *enc,
    const CProbPrice *ProbPrices)
{
  UInt32 b;
 
  {
    unsigned prob = enc->low[0];
    UInt32 a, c;
    unsigned posState;
    b = GET_PRICEa_1(prob);
    a = GET_PRICEa_0(prob);
    c = b + GET_PRICEa_0(enc->low[kLenNumLowSymbols]);
    for (posState = 0; posState < numPosStates; posState++)
    {
      UInt32 *prices = p->prices[posState];
      const CLzmaProb *probs = enc->low + (posState << (1 + kLenNumLowBits));
      SetPrices_3(probs, a, prices, ProbPrices);
      SetPrices_3(probs + kLenNumLowSymbols, c, prices + kLenNumLowSymbols, ProbPrices);
    }
  }

  /*
  {
    unsigned i;
    UInt32 b;
    a = GET_PRICEa_0(enc->low[0]);
    for (i = 0; i < kLenNumLowSymbols; i++)
      p->prices2[i] = a;
    a = GET_PRICEa_1(enc->low[0]);
    b = a + GET_PRICEa_0(enc->low[kLenNumLowSymbols]);
    for (i = kLenNumLowSymbols; i < kLenNumLowSymbols * 2; i++)
      p->prices2[i] = b;
    a += GET_PRICEa_1(enc->low[kLenNumLowSymbols]);
  }
  */
 
  // p->counter = numSymbols;
  // p->counter = 64;

  {
    unsigned i = p->tableSize;
    
    if (i > kLenNumLowSymbols * 2)
    {
      const CLzmaProb *probs = enc->high;
      UInt32 *prices = p->prices[0] + kLenNumLowSymbols * 2;
      i -= kLenNumLowSymbols * 2 - 1;
      i >>= 1;
      b += GET_PRICEa_1(enc->low[kLenNumLowSymbols]);
      do
      {
        /*
        p->prices2[i] = a +
        // RcTree_GetPrice(enc->high, kLenNumHighBits, i - kLenNumLowSymbols * 2, ProbPrices);
        LitEnc_GetPrice(probs, i - kLenNumLowSymbols * 2, ProbPrices);
        */
        // UInt32 price = a + RcTree_GetPrice(probs, kLenNumHighBits - 1, sym, ProbPrices);
        unsigned sym = --i + (1 << (kLenNumHighBits - 1));
        UInt32 price = b;
        do
        {
          unsigned bit = sym & 1;
          sym >>= 1;
          price += GET_PRICEa(probs[sym], bit);
        }
        while (sym >= 2);

        {
          unsigned prob = probs[(size_t)i + (1 << (kLenNumHighBits - 1))];
          prices[(size_t)i * 2    ] = price + GET_PRICEa_0(prob);
          prices[(size_t)i * 2 + 1] = price + GET_PRICEa_1(prob);
        }
      }
      while (i);

      {
        unsigned posState;
        size_t num = (p->tableSize - kLenNumLowSymbols * 2) * sizeof(p->prices[0][0]);
        for (posState = 1; posState < numPosStates; posState++)
          memcpy(p->prices[posState] + kLenNumLowSymbols * 2, p->prices[0] + kLenNumLowSymbols * 2, num);
      }
    }
  }
}

/*
  #ifdef SHOW_STAT
  g_STAT_OFFSET += num;
  printf("\n MovePos %u", num);
  #endif
*/
  
#define MOVE_POS(p, num) { \
    p->additionalOffset += (num); \
    p->matchFinder.Skip(p->matchFinderObj, (UInt32)(num)); }


#define MARK_LIT ((UInt32)(Int32)-1)

#define MakeAs_Lit(p)       { (p)->dist = MARK_LIT; (p)->extra = 0; }
#define MakeAs_ShortRep(p)  { (p)->dist = 0; (p)->extra = 0; }
#define IsShortRep(p)       ((p)->dist == 0)


#define GetPrice_ShortRep(p, state, posState) \
  ( GET_PRICE_0(p->isRepG0[state]) + GET_PRICE_0(p->isRep0Long[state][posState]))

#define GetPrice_Rep_0(p, state, posState) ( \
    GET_PRICE_1(p->isMatch[state][posState]) \
  + GET_PRICE_1(p->isRep0Long[state][posState])) \
  + GET_PRICE_1(p->isRep[state]) \
  + GET_PRICE_0(p->isRepG0[state])
  
MY_FORCE_INLINE
static UInt32 GetPrice_PureRep(const CLzmaEnc *p, unsigned repIndex, size_t state, size_t posState)
{
  UInt32 price;
  UInt32 prob = p->isRepG0[state];
  if (repIndex == 0)
  {
    price = GET_PRICE_0(prob);
    price += GET_PRICE_1(p->isRep0Long[state][posState]);
  }
  else
  {
    price = GET_PRICE_1(prob);
    prob = p->isRepG1[state];
    if (repIndex == 1)
      price += GET_PRICE_0(prob);
    else
    {
      price += GET_PRICE_1(prob);
      price += GET_PRICE(p->isRepG2[state], repIndex - 2);
    }
  }
  return price;
}


static SRes CheckErrors(CLzmaEnc *p)
{
  if (p->result != SZ_OK)
    return p->result;
  if (p->rc.res != SZ_OK)
    p->result = SZ_ERROR_WRITE;
  if (p->matchFinderBase.result != SZ_OK)
    p->result = SZ_ERROR_READ;
  if (p->result != SZ_OK)
    p->finished = True;
  return p->result;
}


MY_NO_INLINE static void FillAlignPrices(CLzmaEnc *p)
{
  unsigned i;
  const CProbPrice *ProbPrices = p->ProbPrices;
  const CLzmaProb *probs = p->posAlignEncoder;
  // p->alignPriceCount = 0;
  for (i = 0; i < kAlignTableSize / 2; i++)
  {
    UInt32 price = 0;
    unsigned sym = i;
    unsigned m = 1;
    unsigned bit;
    UInt32 prob;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[m], bit); m = (m << 1) + bit;
    prob = probs[m];
    p->alignPrices[i    ] = price + GET_PRICEa_0(prob);
    p->alignPrices[i + 8] = price + GET_PRICEa_1(prob);
    // p->alignPrices[i] = RcTree_ReverseGetPrice(p->posAlignEncoder, kNumAlignBits, i, p->ProbPrices);
  }
}


MY_NO_INLINE static void FillDistancesPrices(CLzmaEnc *p)
{
  // int y; for (y = 0; y < 100; y++) {

  UInt32 tempPrices[kNumFullDistances];
  unsigned i, lps;

  const CProbPrice *ProbPrices = p->ProbPrices;
  p->matchPriceCount = 0;

  for (i = kStartPosModelIndex / 2; i < kNumFullDistances / 2; i++)
  {
    unsigned posSlot = GetPosSlot1(i);
    unsigned footerBits = (posSlot >> 1) - 1;
    unsigned base = ((2 | (posSlot & 1)) << footerBits);
    const CLzmaProb *probs = p->posEncoders + (size_t)base * 2;
    // tempPrices[i] = RcTree_ReverseGetPrice(p->posEncoders + base, footerBits, i - base, p->ProbPrices);
    UInt32 price = 0;
    unsigned m = 1;
    unsigned sym = i;
    unsigned offset = (unsigned)1 << footerBits;
    base += i;
    
    if (footerBits)
    do
    {
      unsigned bit = sym & 1;
      sym >>= 1;
      price += GET_PRICEa(probs[m], bit);
      m = (m << 1) + bit;
    }
    while (--footerBits);

    {
      unsigned prob = probs[m];
      tempPrices[base         ] = price + GET_PRICEa_0(prob);
      tempPrices[base + offset] = price + GET_PRICEa_1(prob);
    }
  }

  for (lps = 0; lps < kNumLenToPosStates; lps++)
  {
    unsigned slot;
    unsigned distTableSize2 = (p->distTableSize + 1) >> 1;
    UInt32 *posSlotPrices = p->posSlotPrices[lps];
    const CLzmaProb *probs = p->posSlotEncoder[lps];
    
    for (slot = 0; slot < distTableSize2; slot++)
    {
      // posSlotPrices[slot] = RcTree_GetPrice(encoder, kNumPosSlotBits, slot, p->ProbPrices);
      UInt32 price;
      unsigned bit;
      unsigned sym = slot + (1 << (kNumPosSlotBits - 1));
      unsigned prob;
      bit = sym & 1; sym >>= 1; price  = GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      bit = sym & 1; sym >>= 1; price += GET_PRICEa(probs[sym], bit);
      prob = probs[(size_t)slot + (1 << (kNumPosSlotBits - 1))];
      posSlotPrices[(size_t)slot * 2    ] = price + GET_PRICEa_0(prob);
      posSlotPrices[(size_t)slot * 2 + 1] = price + GET_PRICEa_1(prob);
    }
    
    {
      UInt32 delta = ((UInt32)((kEndPosModelIndex / 2 - 1) - kNumAlignBits) << kNumBitPriceShiftBits);
      for (slot = kEndPosModelIndex / 2; slot < distTableSize2; slot++)
      {
        posSlotPrices[(size_t)slot * 2    ] += delta;
        posSlotPrices[(size_t)slot * 2 + 1] += delta;
        delta += ((UInt32)1 << kNumBitPriceShiftBits);
      }
    }

    {
      UInt32 *dp = p->distancesPrices[lps];
      
      dp[0] = posSlotPrices[0];
      dp[1] = posSlotPrices[1];
      dp[2] = posSlotPrices[2];
      dp[3] = posSlotPrices[3];

      for (i = 4; i < kNumFullDistances; i += 2)
      {
        UInt32 slotPrice = posSlotPrices[GetPosSlot1(i)];
        dp[i    ] = slotPrice + tempPrices[i];
        dp[i + 1] = slotPrice + tempPrices[i + 1];
      }
    }
  }
  // }
}



void LzmaEnc_Construct(CLzmaEnc *p)
{
  RangeEnc_Construct(&p->rc);
  MatchFinder_Construct(&p->matchFinderBase);
  
  #ifndef _7ZIP_ST
  MatchFinderMt_Construct(&p->matchFinderMt);
  p->matchFinderMt.MatchFinder = &p->matchFinderBase;
  #endif

  {
    CLzmaEncProps props;
    LzmaEncProps_Init(&props);
    LzmaEnc_SetProps(p, &props);
  }

  #ifndef LZMA_LOG_BSR
  LzmaEnc_FastPosInit(p->g_FastPos);
  #endif

  LzmaEnc_InitPriceTables(p->ProbPrices);
  p->litProbs = NULL;
  p->saveState.litProbs = NULL;

}

CLzmaEncHandle LzmaEnc_Create(ISzAllocPtr alloc)
{
  void *p;
  p = ISzAlloc_Alloc(alloc, sizeof(CLzmaEnc));
  if (p)
    LzmaEnc_Construct((CLzmaEnc *)p);
  return p;
}

void LzmaEnc_FreeLits(CLzmaEnc *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->litProbs);
  ISzAlloc_Free(alloc, p->saveState.litProbs);
  p->litProbs = NULL;
  p->saveState.litProbs = NULL;
}

void LzmaEnc_Destruct(CLzmaEnc *p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  #ifndef _7ZIP_ST
  MatchFinderMt_Destruct(&p->matchFinderMt, allocBig);
  #endif
  
  MatchFinder_Free(&p->matchFinderBase, allocBig);
  LzmaEnc_FreeLits(p, alloc);
  RangeEnc_Free(&p->rc, alloc);
}

void LzmaEnc_Destroy(CLzmaEncHandle p, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  LzmaEnc_Destruct((CLzmaEnc *)p, alloc, allocBig);
  ISzAlloc_Free(alloc, p);
}


#define kBigHashDicLimit ((UInt32)1 << 24)

static SRes LzmaEnc_Alloc(CLzmaEnc *p, UInt32 keepWindowSize, ISzAllocPtr alloc, ISzAllocPtr allocBig)
{
  UInt32 beforeSize = kNumOpts;
  if (!RangeEnc_Alloc(&p->rc, alloc))
    return SZ_ERROR_MEM;

  #ifndef _7ZIP_ST
  p->mtMode = (p->multiThread && !p->fastMode && (p->matchFinderBase.btMode != 0));
  #endif

  {
    unsigned lclp = p->lc + p->lp;
    if (!p->litProbs || !p->saveState.litProbs || p->lclp != lclp)
    {
      LzmaEnc_FreeLits(p, alloc);
      p->litProbs = (CLzmaProb *)ISzAlloc_Alloc(alloc, ((UInt32)0x300 << lclp) * sizeof(CLzmaProb));
      p->saveState.litProbs = (CLzmaProb *)ISzAlloc_Alloc(alloc, ((UInt32)0x300 << lclp) * sizeof(CLzmaProb));
      if (!p->litProbs || !p->saveState.litProbs)
      {
        LzmaEnc_FreeLits(p, alloc);
        return SZ_ERROR_MEM;
      }
      p->lclp = lclp;
    }
  }

  p->matchFinderBase.bigHash = (Byte)(p->dictSize > kBigHashDicLimit ? 1 : 0);

  if (beforeSize + p->dictSize < keepWindowSize)
    beforeSize = keepWindowSize - p->dictSize;

  #ifndef _7ZIP_ST
  if (p->mtMode)
  {
    RINOK(MatchFinderMt_Create(&p->matchFinderMt, p->dictSize, beforeSize, p->numFastBytes,
        LZMA_MATCH_LEN_MAX
        + 1  /* 18.04 */
        , allocBig));
    p->matchFinderObj = &p->matchFinderMt;
    p->matchFinderBase.bigHash = (Byte)(
        (p->dictSize > kBigHashDicLimit && p->matchFinderBase.hashMask >= 0xFFFFFF) ? 1 : 0);
    MatchFinderMt_CreateVTable(&p->matchFinderMt, &p->matchFinder);
  }
  else
  #endif
  {
    if (!MatchFinder_Create(&p->matchFinderBase, p->dictSize, beforeSize, p->numFastBytes, LZMA_MATCH_LEN_MAX, allocBig))
      return SZ_ERROR_MEM;
    p->matchFinderObj = &p->matchFinderBase;
    MatchFinder_CreateVTable(&p->matchFinderBase, &p->matchFinder);
  }
  
  return SZ_OK;
}

void LzmaEnc_Init(CLzmaEnc *p)
{
  unsigned i;
  p->state = 0;
  p->reps[0] =
  p->reps[1] =
  p->reps[2] =
  p->reps[3] = 1;

  RangeEnc_Init(&p->rc);

  for (i = 0; i < (1 << kNumAlignBits); i++)
    p->posAlignEncoder[i] = kProbInitValue;

  for (i = 0; i < kNumStates; i++)
  {
    unsigned j;
    for (j = 0; j < LZMA_NUM_PB_STATES_MAX; j++)
    {
      p->isMatch[i][j] = kProbInitValue;
      p->isRep0Long[i][j] = kProbInitValue;
    }
    p->isRep[i] = kProbInitValue;
    p->isRepG0[i] = kProbInitValue;
    p->isRepG1[i] = kProbInitValue;
    p->isRepG2[i] = kProbInitValue;
  }

  {
    for (i = 0; i < kNumLenToPosStates; i++)
    {
      CLzmaProb *probs = p->posSlotEncoder[i];
      unsigned j;
      for (j = 0; j < (1 << kNumPosSlotBits); j++)
        probs[j] = kProbInitValue;
    }
  }
  {
    for (i = 0; i < kNumFullDistances; i++)
      p->posEncoders[i] = kProbInitValue;
  }

  {
    UInt32 num = (UInt32)0x300 << (p->lp + p->lc);
    UInt32 k;
    CLzmaProb *probs = p->litProbs;
    for (k = 0; k < num; k++)
      probs[k] = kProbInitValue;
  }


  LenEnc_Init(&p->lenProbs);
  LenEnc_Init(&p->repLenProbs);

  p->optEnd = 0;
  p->optCur = 0;

  {
    for (i = 0; i < kNumOpts; i++)
      p->opt[i].price = kInfinityPrice;
  }

  p->additionalOffset = 0;

  p->pbMask = (1 << p->pb) - 1;
  p->lpMask = ((UInt32)0x100 << p->lp) - ((unsigned)0x100 >> p->lc);
}


void LzmaEnc_InitPrices(CLzmaEnc *p)
{
  if (!p->fastMode)
  {
    FillDistancesPrices(p);
    FillAlignPrices(p);
  }

  p->lenEnc.tableSize =
  p->repLenEnc.tableSize =
      p->numFastBytes + 1 - LZMA_MATCH_LEN_MIN;

  p->repLenEncCounter = REP_LEN_COUNT;

  LenPriceEnc_UpdateTables(&p->lenEnc, 1 << p->pb, &p->lenProbs, p->ProbPrices);
  LenPriceEnc_UpdateTables(&p->repLenEnc, 1 << p->pb, &p->repLenProbs, p->ProbPrices);
}

typedef struct
{
  ISeqOutStream vt;
  Byte *data;
  SizeT rem;
  BoolInt overflow;
} CLzmaEnc_SeqOutStreamBuf;

static size_t SeqOutStreamBuf_Write(const ISeqOutStream *pp, const void *data, size_t size)
{
  CLzmaEnc_SeqOutStreamBuf *p = CONTAINER_FROM_VTBL(pp, CLzmaEnc_SeqOutStreamBuf, vt);
  if (p->rem < size)
  {
    size = p->rem;
    p->overflow = True;
  }
  memcpy(p->data, data, size);
  p->rem -= size;
  p->data += size;
  return size;
}


UInt32 LzmaEnc_GetNumAvailableBytes(CLzmaEncHandle pp)
{
  const CLzmaEnc *p = (CLzmaEnc *)pp;
  return p->matchFinder.GetNumAvailableBytes(p->matchFinderObj);
}


const Byte *LzmaEnc_GetCurBuf(CLzmaEncHandle pp)
{
  const CLzmaEnc *p = (CLzmaEnc *)pp;
  return p->matchFinder.GetPointerToCurrentPos(p->matchFinderObj) - p->additionalOffset;
}


SRes LzmaEnc_WriteProperties(CLzmaEncHandle pp, Byte *props, SizeT *size)
{
  CLzmaEnc *p = (CLzmaEnc *)pp;
  unsigned i;
  UInt32 dictSize = p->dictSize;
  if (*size < LZMA_PROPS_SIZE)
    return SZ_ERROR_PARAM;
  *size = LZMA_PROPS_SIZE;
  props[0] = (Byte)((p->pb * 5 + p->lp) * 9 + p->lc);

  if (dictSize >= ((UInt32)1 << 22))
  {
    UInt32 kDictMask = ((UInt32)1 << 20) - 1;
    if (dictSize < (UInt32)0xFFFFFFFF - kDictMask)
      dictSize = (dictSize + kDictMask) & ~kDictMask;
  }
  else for (i = 11; i <= 30; i++)
  {
    if (dictSize <= ((UInt32)2 << i)) { dictSize = (2 << i); break; }
    if (dictSize <= ((UInt32)3 << i)) { dictSize = (3 << i); break; }
  }

  for (i = 0; i < 4; i++)
    props[1 + i] = (Byte)(dictSize >> (8 * i));
  return SZ_OK;
}




