/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2012 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: AMRWB speech codec: USC functions.
//
*/
#include "usc.h"
#include "ownamrwbe.h"

#define AMRWB_BITS_PER_SAMPLE 16
#define AMRWBE_MODE_MONO 1
#define AMRWBE_MODE_STEREO 2

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo);
static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_Modes *modes, USC_Handle handle);
static USC_Status Control(const USC_Modes *modes, USC_Handle handle );
static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out);
static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out);
static USC_Status GetOutStreamSize(const USC_Option *options, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst);
static USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize);

static Ipp32s IdxToRate (Ipp32s idxMode, Ipp32s* pRate);
static USC_Status RateToIdx (Ipp32s rate, Ipp32s nChannels, Ipp32s isf,
   Ipp32s* pIdxMode, Ipp32s* pIdxRateWB, Ipp32s* pIdxRateMono, Ipp32s* pIdxRateStereo);
static Ipp32s get_isf_index(Ipp32s *fscale);
static Ipp32s get_nb_bits(Ipp32s idxMode, Ipp32s idxRateWB, Ipp32s idxRateMono, Ipp32s idxRateStereo);
static Ipp32s WriteHeader_RAW(Ipp32s idxMode, Ipp32s idxIsf, Ipp32s offset, Ipp8u* pStream);
static Ipp32s Bin2Bitstream_AMRWBE(const Ipp16s* pSerial, Ipp32s length, Ipp32s offset, Ipp8u* pStream);
static USC_Status ownSetFrameSizeResampleFactor(Ipp32s pcmSampFreq, USC_Handle handle);
static USC_Status ownEncode_AMRWB(USC_Handle handle, Ipp32s* pIdxMode, Ipp16s *speech,
                                  Ipp8u *serial, Ipp32s dtx, Ipp32s* pFrameType);
static Ipp32s ownPackMMS_WB(Ipp32s mode, Ipp16s *param, Ipp8u *stream, Ipp32s frame_type, Ipp32s speech_mode);
static void ownCopyCoderState(USC_Handle handle, Ipp32s direction);
static Ipp32s ownDecode_AMRWB(USC_Handle handle, Ipp8u *serial, Ipp16s *speech, Ipp32s falgLostFrame);

static Ipp32s ownReadBitstream_RAW(const Ipp8u* pBitstream, Ipp16s* pSerial,
   Ipp32s *pIdxISF, Ipp32s* pIdxMode, Ipp32s* pIdxRateWB, Ipp32s* pIdxRateMono, Ipp32s* pIdxRateStereo, Ipp32s *pBFI);
static Ipp32s ownReadHeader_RAW(const Ipp8u* pStream, Ipp32s *pIdxMode, Ipp32s *pIdxISF, Ipp32s *pBFI);
static USC_Status ownCheckSamplFreq(int samplFreq);
static USC_Status ownSelectSamplFreq(USC_Handle handle);
static USC_Status ownSetFrameLength(USC_Handle handle);
static Ipp16s ownUnpackMMS_WB(const Ipp8u *pStream, Ipp16s *pParams, Ipp8u *frame_type,
   Ipp16s *speech_mode, Ipp16s *fqi);
static Ipp32s ownTestHomingFrame(Ipp16s* input_frame, Ipp16s mode);
static Ipp32s ownTestHomingFrameFirst(Ipp16s* input_frame, Ipp16s mode);

static Ipp32s ownFrameSize2ISFscale(Ipp32s frameSize);
static Ipp32s ownISFscale2FrameSize(Ipp32s isfScale);

void Simple_frame_limiter(Ipp16s *x, Ipp16s Qx, Ipp16s *mem, Ipp32s n);
static Ipp32s ownCheckBitrate2PCM (USC_Handle pHandle);

extern __ALIGN(16) CONST Ipp16s *tblHomingFrames_WB[9];

extern __ALIGN(16) CONST Ipp16s tblMode7k_idx[NUMBITS6600];
extern __ALIGN(16) CONST Ipp16s tblMode7k_mask[NUMBITS6600];
extern __ALIGN(16) CONST Ipp16s tblMode9k_idx[NUMBITS8850];
extern __ALIGN(16) CONST Ipp16s tblMode9k_mask[NUMBITS8850];
extern __ALIGN(16) CONST Ipp16s tblMode12k_idx[NUMBITS12650];
extern __ALIGN(16) CONST Ipp16s tblMode12k_mask[NUMBITS12650];
extern __ALIGN(16) CONST Ipp16s tblMode14k_idx[NUMBITS14250];
extern __ALIGN(16) CONST Ipp16s tblMode14k_mask[NUMBITS14250];
extern __ALIGN(16) CONST Ipp16s tblMode16k_idx[NUMBITS15850];
extern __ALIGN(16) CONST Ipp16s tblMode16k_mask[NUMBITS15850];
extern __ALIGN(16) CONST Ipp16s tblMode18k_idx[NUMBITS18250];
extern __ALIGN(16) CONST Ipp16s tblMode18k_mask[NUMBITS18250];
extern __ALIGN(16) CONST Ipp16s tblMode20k_idx[NUMBITS19850];
extern __ALIGN(16) CONST Ipp16s tblMode20k_mask[NUMBITS19850];
extern __ALIGN(16) CONST Ipp16s tblMode23k_idx[NUMBITS23050];
extern __ALIGN(16) CONST Ipp16s tblMode23k_mask[NUMBITS23050];
extern __ALIGN(16) CONST Ipp16s tblMode24k_idx[NUMBITS23850];
extern __ALIGN(16) CONST Ipp16s tblMode24k_mask[NUMBITS23850];
extern __ALIGN(16) CONST Ipp16s tblModeDTX_idx[NUMBITS_SID];
extern __ALIGN(16) CONST Ipp16s tblModeDTX_mask[NUMBITS_SID];

#define ISF_1_0 96
static __ALIGN(16) CONST Ipp32s isfIndex_fx[14] = {
  0, 48, 54, 60, 64, 72, 80, 90, ISF_1_0,108,120,128,135,144
};

#define AMRWBE_NUM_PCM_TYPES 16
static __ALIGN(16) CONST USC_PCMType pcmTypesTbl_AMRWBE[AMRWBE_NUM_PCM_TYPES]={
   {16000,16,1}, { 8000,16,1}, {24000,16,1}, {32000,16,1}, {48000,16,1}, {11025,16,1}, {22050,16,1}, {44100,16,1},
   { 8000,16,2}, {16000,16,2}, {24000,16,2}, {32000,16,2}, {48000,16,2}, {11025,16,2}, {22050,16,2}, {44100,16,2},
};

#define AMRWBE_NUM_RATES 38
static __ALIGN(16) CONST USC_Rates pTblRates_AMRWBE[AMRWBE_NUM_RATES]={
   { 6600}, { 8850}, {12650}, {14250}, /*WB rates*/
   {15850}, {18250}, {19850}, {23050},
   {23850},
   {10400}, {12000}, {12400}, {12800}, /*WB+ rates*/
   {13600}, {14000}, {14400}, {15200},
   {16000}, {16400}, {16800}, {17200},
   {18000}, {18400}, {19200}, {20000},
   {20400}, {20800}, {21200}, {22400},
   {23200}, {24000}, {25600}, {26000},
   {26800}, {28800}, {29600}, {30000},
   {32000}
};

typedef struct
{
    Ipp16s updateSidCount;
    Ipp16s sid_handover_debt;
    TXFrameType prevFrameType;
} SIDSyncState;

typedef struct {
   USC_Direction direction;
   Ipp32s vad;
   Ipp32s prevResetFlag;  /* previous was homing frame (for WB decoder) */
   Ipp32s prevMode;       /* previous mode             (for WB decoder) */
   Ipp32s prevFrameType;  /* previous frame type       (for WB decoder) */

   SIDSyncState sid_state;

   void* pObj_AMRWBE;
   void* pObj_AMRWB;
   Ipp16s *pBufChannelRight;
   Ipp16s *pBufChannelLeft;
   Ipp16s *pMemResampRight;
   Ipp16s *pMemResampLeft;
   Ipp16s fracLagRight;
   Ipp16s fracLagLeft;
   Ipp32s isFirstFrame;
   USC_OutputMode codecMode;
   USC_OutputMode codecModeFirst;

   Ipp32s isfFirst;
   Ipp32s isf;
   Ipp32s idxIsf;
   Ipp32s osf;
   Ipp32s frameSize;
   Ipp16s upFact;
   Ipp16s downFact;

   Ipp32s nSampToProceed;
   Ipp32s nSampProceeded;

   Ipp32s bitrateFirst;
   Ipp32s bitrate;
   Ipp32s idxMode;         // [0..47]
   Ipp32s idxRateWB;       // [0..8]
   Ipp32s idxRateMono;     // [0..7]
   Ipp32s idxRateStereo;   // [0..15]

   Ipp32s agcFlag;
   Ipp16s memAGCRight[2], memAGCLeft[2];
   Ipp32s frameScale;      // last frame scaling
   Ipp32s pcmSampFreq;
   Ipp32s bitPerSample;
   Ipp32s nChannels;
   Ipp32s reserved0; // for future extension
   Ipp32s reserved1; // for future extension
} HandleHeader_AMRWBE;

static Ipp32s ownSidSyncInit(SIDSyncState *st);


/* global usc vector table */
USCFUN USC_Fxns USC_AMRWBE_Fxns={
   {
      USC_Codec,
      (GetInfoSize_func) GetInfoSize,
      (GetInfo_func)     GetInfo,
      (NumAlloc_func)    NumAlloc,
      (MemAlloc_func)    MemAlloc,
      (Init_func)        Init,
      (Reinit_func)      Reinit,
      (Control_func)     Control
    },
    Encode,
    Decode,
    GetOutStreamSize,
    SetFrameSize
};

static USC_Status GetInfoSize(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_CodecInfo);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_CodecInfo *pInfo)
{
   HandleHeader_AMRWBE *pHeader;

   USC_CHECK_PTR(pInfo);

   pInfo->name = "IPP_AMRWBE";
   if (handle == NULL) {
      pInfo->params.framesize= L_FRAME16k*sizeof(Ipp16s);
      pInfo->params.modes.bitrate = 6600;
      pInfo->params.direction = USC_DECODE;
      pInfo->params.modes.vad = 1;
      pInfo->params.modes.outMode = USC_OUT_MONO; /* mono */
      pInfo->params.modes.pf = 1; /* AGC on */
      pInfo->params.pcmType.sample_frequency = pcmTypesTbl_AMRWBE[0].sample_frequency;
      pInfo->params.pcmType.bitPerSample = pcmTypesTbl_AMRWBE[0].bitPerSample;
      pInfo->params.pcmType.nChannels = pcmTypesTbl_AMRWBE[0].nChannels;
   } else {
      pHeader = (HandleHeader_AMRWBE*)handle;
      if (pHeader->direction == USC_ENCODE) {
         /* return current work length */
         pInfo->params.framesize = pHeader->nSampToProceed * sizeof(Ipp16s) * pHeader->nChannels;
      } else if (pHeader->direction == USC_DECODE) {
         pInfo->params.framesize = (pHeader->frameSize*2) * sizeof(Ipp16s) * pHeader->nChannels/*codecMode*/;
      } else {
         return USC_NoOperation;
      }
      pInfo->params.modes.bitrate = pHeader->bitrate;
      pInfo->params.modes.outMode = pHeader->codecMode;
      pInfo->params.modes.pf = pHeader->agcFlag;
      pInfo->params.direction = pHeader->direction;
      pInfo->params.modes.vad = pHeader->vad;
      pInfo->params.pcmType.sample_frequency = pHeader->pcmSampFreq;
      pInfo->params.pcmType.bitPerSample = pHeader->bitPerSample;
      pInfo->params.pcmType.nChannels = pHeader->nChannels;
   }
   pInfo->maxbitsize = NBITS_MAX/8;
   pInfo->nPcmTypes = AMRWBE_NUM_PCM_TYPES;
   pInfo->pPcmTypesTbl = pcmTypesTbl_AMRWBE;

   pInfo->params.modes.hpf = 0;
   pInfo->params.law = 0;
   pInfo->params.modes.truncate = 0;
   pInfo->nRates = AMRWBE_NUM_RATES;
   pInfo->pRateTbl = (const USC_Rates *)&pTblRates_AMRWBE;
   pInfo->params.nModes = sizeof(USC_Modes)/sizeof(Ipp32s);
   return USC_NoError;
}

static USC_Status NumAlloc(const USC_Option *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_Option *options, USC_MemBank *pBanks)
{
   Ipp32u nbytes, objSize;
   Ipp32s tmp;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   if (options->direction == USC_ENCODE) { /* encode only */
      apiAMRWBEncoder_Alloc((const AMRWBEnc_Params*)&tmp, &objSize);
      nbytes = objSize;
      apiEncoderGetObjSize_AMRWBE(&objSize);
      nbytes += objSize;
      nbytes += 2*(2*L_FRAME_MAX_FS)*sizeof(Ipp16s); /* pBufChannelRight & pBufChannelLeft */
      nbytes += 2*((2*L_FILT_OVER_FS+7)&(~7))*sizeof(Ipp16s); /* pMemResampRight & pMemResampLeft */
   } else if (options->direction == USC_DECODE) { /* decode only */
      apiAMRWBDecoder_Alloc((AMRWBDec_Params*)&tmp, &objSize);
      nbytes = objSize;
      apiDecoderGetObjSize_AMRWBE(&objSize);
      nbytes += objSize;
      nbytes += 2*(2*L_FRAME_MAX_FS)*sizeof(Ipp16s); /* pBufChannelRight & pBufChannelLeft */
      nbytes += 2*(2*L_FILT_DECIM_FS)*sizeof(Ipp16s); /* pMemResampRight & pMemResampLeft */
   } else {
      return USC_NoOperation;
   }
   pBanks->nbytes = nbytes + sizeof(HandleHeader_AMRWBE); /* include header in handle */
   return USC_NoError;
}



static USC_Status Init(const USC_Option *pParams, const USC_MemBank *pBanks, USC_Handle *handle)
{
   HandleHeader_AMRWBE *pHeader;
   Ipp32u objSize;
   Ipp32s isf;
   USC_Status status;

   USC_CHECK_PTR(pParams);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(pParams->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(pParams->modes.vad < 0, USC_UnsupportedVADType);
   USC_BADARG(pParams->pcmType.bitPerSample!=AMRWB_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(((pParams->pcmType.nChannels!=AMRWBE_MODE_MONO)
      &&(pParams->pcmType.nChannels!=AMRWBE_MODE_STEREO)), USC_UnsupportedPCMType);

   *handle = (USC_Handle*)pBanks->pMem;
   pHeader= (HandleHeader_AMRWBE*)*handle;

   pHeader->direction = pParams->direction;
   pHeader->vad = pParams->modes.vad;
   pHeader->pcmSampFreq = pParams->pcmType.sample_frequency;
   pHeader->bitPerSample = pParams->pcmType.bitPerSample;
   pHeader->nChannels = pParams->pcmType.nChannels;
   pHeader->codecMode = pParams->modes.outMode;
   pHeader->codecModeFirst = pHeader->codecMode;

   pHeader->bitrate = pHeader->bitrateFirst = pParams->modes.bitrate;
   if (pParams->framesize != 0) { /* isf defined */
      isf = ownFrameSize2ISFscale(pParams->framesize/sizeof(Ipp16s));
      if (isf < 0) {
         *handle = NULL;
         return USC_BadArgument;
      }
      //check status
   } else { /* isf is not set (default isf=0)*/
      isf = 0;
   }
   status = RateToIdx(pHeader->bitrate, pHeader->codecMode, isf, &pHeader->idxMode,
      &pHeader->idxRateWB, &pHeader->idxRateMono, &pHeader->idxRateStereo);
   if (status) {
      *handle = NULL;
      return status;
   }

   if ((isf==0) && (16<=pHeader->idxMode && pHeader->idxMode<=47)) { /* WB+ modes, force to isf=1 by default */
      isf = ISF_1_0;
   }
   pHeader->isf = pHeader->isfFirst = isf;
   pHeader->idxIsf = get_isf_index(&pHeader->isf);

   status = ownSetFrameSizeResampleFactor(pHeader->pcmSampFreq, pHeader);
   if (status) {
      *handle = NULL;
      return status;
   }
   if ( ownCheckBitrate2PCM(pHeader) ) {
      *handle = NULL;
      return USC_UnsupportedBitRate;
   }
   pHeader->fracLagLeft = pHeader->fracLagRight = 0;

   pHeader->nSampToProceed = (pHeader->frameSize*pHeader->downFact) / pHeader->upFact;
   pHeader->isFirstFrame = 1;

   if (pParams->direction == USC_ENCODE)  { /* encode only */
      pHeader->pBufChannelRight = (Ipp16s*)( ((Ipp8u*)*handle) + sizeof(HandleHeader_AMRWBE) );
      ippsZero_16s(pHeader->pBufChannelRight, (2*L_FRAME_MAX_FS));
      pHeader->pBufChannelLeft = (pHeader->pBufChannelRight) + 2*L_FRAME_MAX_FS;
      ippsZero_16s(pHeader->pBufChannelLeft, (2*L_FRAME_MAX_FS));
      pHeader->pMemResampRight = (pHeader->pBufChannelLeft) + 2*L_FRAME_MAX_FS;
      ippsZero_16s(pHeader->pMemResampRight, (2*L_FILT_OVER_FS));
      pHeader->pMemResampLeft = (pHeader->pMemResampRight) + ((2*L_FILT_OVER_FS+7)&(~7));
      ippsZero_16s(pHeader->pMemResampLeft, (2*L_FILT_OVER_FS));
      pHeader->pObj_AMRWBE = (EncoderObj_AMRWBE*)( (pHeader->pMemResampLeft) + ((2*L_FILT_OVER_FS+7)&(~7)));
      apiEncoderInit_AMRWBE(pHeader->pObj_AMRWBE, pHeader->isf, pHeader->vad, 1);
      apiEncoderGetObjSize_AMRWBE(&objSize);
      pHeader->pObj_AMRWB = (AMRWBEncoder_Obj*)( ((Ipp8u*)pHeader->pObj_AMRWBE) + objSize);
      apiAMRWBEncoder_Init(pHeader->pObj_AMRWB, pHeader->vad);

   } else if (pParams->direction == USC_DECODE) { /* decode only */

      pHeader->agcFlag = pParams->modes.pf;
      pHeader->memAGCRight[0] = pHeader->memAGCRight[1] = 0;
      pHeader->memAGCLeft[0] = pHeader->memAGCLeft[1] = 0;
      pHeader->frameScale = 0;

      pHeader->pBufChannelRight = (Ipp16s*)( ((Ipp8u*)*handle) + sizeof(HandleHeader_AMRWBE) );
      ippsZero_16s(pHeader->pBufChannelRight, (2*L_FRAME_MAX_FS));
      pHeader->pBufChannelLeft = (pHeader->pBufChannelRight) + 2*L_FRAME_MAX_FS;
      ippsZero_16s(pHeader->pBufChannelLeft, (2*L_FRAME_MAX_FS));
      pHeader->pMemResampRight = (pHeader->pBufChannelLeft) + 2*L_FRAME_MAX_FS;
      ippsZero_16s(pHeader->pMemResampRight, (2*L_FILT_DECIM_FS));
      pHeader->pMemResampLeft = (pHeader->pMemResampRight) + (2*L_FILT_DECIM_FS);
      ippsZero_16s(pHeader->pMemResampLeft, (2*L_FILT_DECIM_FS));
      pHeader->pObj_AMRWBE = (DecoderObj_AMRWBE*)( (pHeader->pMemResampLeft) + 2*(L_FILT_DECIM_FS) );
      apiDecoderInit_AMRWBE(pHeader->pObj_AMRWBE, pHeader->isf, 2/*maxNChan*/, 1);
      apiDecoderGetObjSize_AMRWBE(&objSize);

      pHeader->pObj_AMRWB = (AMRWBDecoder_Obj*)( ((Ipp8u*)pHeader->pObj_AMRWBE) + objSize);
      apiAMRWBDecoder_Init(pHeader->pObj_AMRWB);
      pHeader->prevResetFlag = 1;
      pHeader->prevMode = AMRWB_RATE_6600;
      pHeader->prevFrameType = RX_SPEECH_GOOD;

   } else {
      *handle = NULL;
      return USC_NoOperation;
   }
   return USC_NoError;
}

static USC_Status Reinit(const USC_Modes *pModes, USC_Handle handle)
{
   HandleHeader_AMRWBE *pHeader;
   Ipp32s tmp;

   USC_CHECK_PTR(pModes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(pModes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(pModes->vad < 0, USC_UnsupportedVADType);

   pHeader = (HandleHeader_AMRWBE*)handle;
   USC_BADARG(RateToIdx(pModes->bitrate, pModes->outMode, pHeader->isfFirst, &tmp, &tmp, &tmp, &tmp),
      USC_UnsupportedBitRate);

   pHeader->vad = pModes->vad;
   pHeader->bitrate = pHeader->bitrateFirst;
   pHeader->codecMode = pHeader->codecModeFirst;//pModes->outMode;
   pHeader->isf = pHeader->isfFirst;
   pHeader->idxIsf = get_isf_index(&pHeader->isf);

   if (ownSetFrameSizeResampleFactor(pHeader->pcmSampFreq, pHeader)) {
      return USC_UnsupportedBitRate;
   }
   pHeader->fracLagLeft = pHeader->fracLagRight = 0;
   pHeader->nSampToProceed = (pHeader->frameSize*pHeader->downFact) / pHeader->upFact;
   pHeader->isFirstFrame = 1;

   if (pHeader->direction == USC_ENCODE) { /* encode only */
      /* no rearrange objects, just full data reset */
      /* WB+ */
      ippsZero_16s(pHeader->pBufChannelRight, 2*L_FRAME_MAX_FS);
      ippsZero_16s(pHeader->pBufChannelLeft, 2*L_FRAME_MAX_FS);
      ippsZero_16s(pHeader->pMemResampRight, 2*L_FILT_OVER_FS);
      ippsZero_16s(pHeader->pMemResampLeft, 2*L_FILT_OVER_FS);
      apiEncoderInit_AMRWBE(pHeader->pObj_AMRWBE, pHeader->isf, pHeader->vad, 1);

      /* WB */
      apiAMRWBEncoder_Init(pHeader->pObj_AMRWB, pHeader->vad);
      ownSidSyncInit(&pHeader->sid_state);

   } else if (pHeader->direction == USC_DECODE) { /* decode only */

      RateToIdx(pHeader->bitrate, pHeader->codecMode, pHeader->isf,
         &(pHeader->idxMode), &(pHeader->idxRateWB), &(pHeader->idxRateMono), &(pHeader->idxRateStereo));
      //pHeader->idxRateWB = (pHeader->codecMode) ? -1 : 0; /*for decoder selecting in case of first bad frame*/

      pHeader->frameScale = 0;
      pHeader->memAGCRight[0] = pHeader->memAGCRight[1] = 0;
      pHeader->memAGCLeft[0] = pHeader->memAGCLeft[1] = 0;

      ippsZero_16s(pHeader->pBufChannelRight, 2*L_FRAME_MAX_FS);
      ippsZero_16s(pHeader->pBufChannelLeft, 2*L_FRAME_MAX_FS);
      ippsZero_16s(pHeader->pMemResampRight, 2*L_FILT_DECIM_FS);
      ippsZero_16s(pHeader->pMemResampLeft, 2*L_FILT_DECIM_FS);
      /* WB+ */
      apiDecoderInit_AMRWBE(pHeader->pObj_AMRWBE, pHeader->isf, 2/*(Ipp32s)(pHeader->codecMode)*/, 1);
      /* WB */
      apiAMRWBDecoder_Init((AMRWBDecoder_Obj*)(pHeader->pObj_AMRWB));
      pHeader->prevResetFlag = 1;
      pHeader->prevMode = AMRWB_RATE_6600;
      pHeader->prevFrameType = RX_SPEECH_GOOD;
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}

USC_Status SetFrameSize(const USC_Option *options, USC_Handle handle, Ipp32s frameSize)
{
   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;
   Ipp32s isfScale;

   USC_CHECK_PTR(options);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(frameSize <= 0, USC_BadArgument);

   if (pHeader->direction == USC_ENCODE) { /* encoder */

      isfScale = ownFrameSize2ISFscale(frameSize/sizeof(Ipp16s));
      if (isfScale != pHeader->isf) {
         /*!!!! this switch allowed at 48kHz PCMs (wb+) !!!!*/
         pHeader->isf = isfScale;
         apiEncoderInit_AMRWBE(pHeader->pObj_AMRWBE, pHeader->isf, pHeader->vad, 0);
         if (ownSetFrameSizeResampleFactor(pHeader->pcmSampFreq, pHeader)) {
            return USC_UnsupportedBitRate;
         }
         pHeader->idxIsf = get_isf_index(&(pHeader->isf));
      }

   } else if (pHeader->direction == USC_DECODE) { /*decoder*/
      return USC_NoOperation;
   } else {
      return USC_NoOperation;
   }

   return USC_NoError;
}


static USC_Status Control(const USC_Modes *pModes, USC_Handle handle )
{
   Ipp32s idxMode, idxRateWB, idxRateMono, idxRateStereo;
   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;

   USC_CHECK_PTR(pModes);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(pModes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(pModes->vad < 0, USC_UnsupportedVADType);

   if (pHeader->direction == USC_ENCODE) { /* encoder */
      pHeader->codecMode = pModes->outMode;
      pHeader->vad = pModes->vad;

      if ( RateToIdx(pModes->bitrate, (Ipp32s)(pHeader->codecMode), pHeader->isf,
         &idxMode, &idxRateWB, &idxRateMono, &idxRateStereo) ) {
            return USC_UnsupportedBitRate;
      }

      if ( ((pHeader->idxMode<=8)&&(idxMode>=10)) || ((pHeader->idxMode>=10)&&(idxMode<=8)) ) {
         /*!!!! this switch allowed at 16kHz PCMs !!!!*/
         Ipp32s switchMode = 0;
         if (idxMode > 10) { /* WB -> WB+ */
            switchMode = 0;
         } else if (idxMode < 9) { /* WB+ -> WB */
            switchMode = 1;
         }
         apiCopyEncoderState_AMRWBE(pHeader->pObj_AMRWBE, pHeader->pObj_AMRWB, pHeader->vad, switchMode);
      }
      pHeader->bitrate = pModes->bitrate;
      pHeader->idxMode = idxMode;
      pHeader->idxRateWB = idxRateWB;
      pHeader->idxRateMono = idxRateMono;
      pHeader->idxRateStereo = idxRateStereo;
   } else if (pHeader->direction == USC_DECODE) { /*decoder*/
      return USC_NoOperation;
   } else {
      return USC_NoOperation;
   }

   return USC_NoError;
}



static USC_Status Encode(USC_Handle handle, USC_PCMStream *in, USC_Bitstream *out)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, serial, (NBITS_MAX));
   HandleHeader_AMRWBE *pHeader;
   AMRWBEncoder_Obj* pEncObj_WB;
   EncoderObj_AMRWBE* pEncObj_WBE;
   Ipp32s i, nSampToProceed, nSampProceeded, nSampHold, framesize;
   Ipp16s upFact, downFact;
   Ipp32s idxMode=-1, idxRateWB=-1, idxRateMono=-1, idxRateStereo=-1;
   Ipp32s nbits;
   Ipp8u* pStream;

   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(in);
   USC_BADARG((in->pBuffer==NULL), USC_NoOperation);
   USC_BADARG((in->pcmType.bitPerSample!=AMRWB_BITS_PER_SAMPLE), USC_UnsupportedPCMType);
   USC_BADARG(((in->pcmType.nChannels!=AMRWBE_MODE_MONO)
      &&(in->pcmType.nChannels!=AMRWBE_MODE_STEREO)), USC_UnsupportedPCMType);
   USC_BADARG(ownCheckSamplFreq(in->pcmType.sample_frequency), USC_UnsupportedPCMType);
   USC_CHECK_PTR(out);
   USC_BADARG((out->pBuffer==NULL), USC_NoOperation);


   pHeader = (HandleHeader_AMRWBE*)handle;
   USC_BADARG((pHeader->direction!=USC_ENCODE), USC_NoOperation);

   if ( RateToIdx(in->bitrate, (Ipp32s)(pHeader->codecMode), pHeader->isf,
      &idxMode, &idxRateWB, &idxRateMono, &idxRateStereo) ) {
         return USC_UnsupportedBitRate;
   }

   pEncObj_WB = (AMRWBEncoder_Obj*)pHeader->pObj_AMRWB;
   pEncObj_WBE = (EncoderObj_AMRWBE*)pHeader->pObj_AMRWBE;

   if (pHeader->isFirstFrame) { /* move to second branch for optimization */
      if (in->nbytes == 0) {
         return USC_NoOperation;
      }
      /* fillup lookahead buffers */
      nSampToProceed = pHeader->nSampToProceed;
      if (in->pcmType.nChannels == 2) { /* stereo pcm */
         /* Do the deinterleaving of channels*/
         ippsCplxToReal_16sc( (const Ipp16sc*) in->pBuffer,
                            (Ipp16s*) pHeader->pBufChannelLeft, (Ipp16s*) pHeader->pBufChannelRight, nSampToProceed);
         ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelLeft, nSampToProceed, pHeader->upFact, pHeader->downFact,
            &(pHeader->fracLagLeft), pHeader->pMemResampLeft, pHeader->pBufChannelLeft, pHeader->frameSize);
      } else {
         ippsCopy_16s((Ipp16s*)in->pBuffer, pHeader->pBufChannelRight, nSampToProceed);
      }
      ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelRight, nSampToProceed, pHeader->upFact, pHeader->downFact,
         &(pHeader->fracLagRight), pHeader->pMemResampRight, pHeader->pBufChannelRight, pHeader->frameSize);

      if (idxRateWB == -1) { /* AMRWB+ */
         apiEncoderProceedFirst_AMRWBE(pEncObj_WBE, pHeader->pBufChannelRight, pHeader->pBufChannelLeft,
            (Ipp16s)in->pcmType.nChannels, pHeader->frameSize, (Ipp16s)pHeader->isf, &nSampProceeded);
      } else {
         /* this branch is not affect on bit2bit test */
         /* Delete the 2 LSBs (14-bit input) */
         ippsAndC_16u((Ipp16u*)pHeader->pBufChannelRight, 0xfffC, (Ipp16u*)pHeader->pBufChannelRight, L_FRAME16k);
         apiAMRWBEncoderProceedFirst(pEncObj_WB, pHeader->pBufChannelRight);
         nSampProceeded = 320;
      }
      pHeader->isFirstFrame = 0;
      pHeader->nSampProceeded = nSampProceeded;
      /* number of sample per channel to read from file */
      pHeader->nSampToProceed = (nSampProceeded*pHeader->downFact + pHeader->fracLagRight) / pHeader->upFact;

      in->nbytes = nSampToProceed*sizeof(Ipp16s)*in->pcmType.nChannels;
      out->nbytes = 0;
      out->frametype = 0;
      out->bitrate = in->bitrate;

   } else {
      Ipp32s nBytes, cntBytes, srcSizePerChannel,wrkLen;
      /* regular branch */
      framesize = pHeader->frameSize;
      nSampProceeded = pHeader->nSampProceeded;
      nSampToProceed = pHeader->nSampToProceed;
      downFact = pHeader->downFact;
      upFact = pHeader->upFact;
      nSampHold = framesize - nSampProceeded;
      srcSizePerChannel = in->nbytes/(sizeof(Ipp16s)*(in->pcmType.nChannels));

      ippsMove_16s(pHeader->pBufChannelRight+nSampProceeded, pHeader->pBufChannelRight, nSampHold);
      ippsMove_16s(pHeader->pBufChannelLeft+nSampProceeded, pHeader->pBufChannelLeft, nSampHold);

      if (srcSizePerChannel < nSampToProceed) { /* not full frame came */
         wrkLen = srcSizePerChannel;
      } else {
         wrkLen = nSampToProceed;
      }

      if (in->pcmType.nChannels == 2) {
         /* Do the deinterleaving of channels*/
         ippsCplxToReal_16sc( (const Ipp16sc*)in->pBuffer, (Ipp16s*)pHeader->pBufChannelLeft+nSampHold,
            (Ipp16s*)pHeader->pBufChannelRight+nSampHold, wrkLen);
         if (srcSizePerChannel < nSampToProceed) { /* not full frame came */
            ippsZero_16s((pHeader->pBufChannelLeft)+nSampHold+srcSizePerChannel, nSampToProceed-srcSizePerChannel);
         }
         ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelLeft+nSampHold, nSampToProceed, upFact, downFact,
            &(pHeader->fracLagLeft), pHeader->pMemResampLeft, pHeader->pBufChannelLeft+nSampHold, nSampProceeded);
      } else {
         ippsCopy_16s((Ipp16s*)in->pBuffer, pHeader->pBufChannelRight+nSampHold, wrkLen);
      }
      if (srcSizePerChannel < nSampToProceed) { /* not full frame came */
         ippsZero_16s((pHeader->pBufChannelRight)+nSampHold+srcSizePerChannel, nSampToProceed-srcSizePerChannel);
      }
      ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelRight+nSampHold, nSampToProceed, upFact, downFact,
         &(pHeader->fracLagRight), pHeader->pMemResampRight, pHeader->pBufChannelRight+nSampHold, nSampProceeded);

      pHeader->idxMode = idxMode;
      pHeader->idxRateWB = idxRateWB;
      pHeader->idxRateMono = idxRateMono;
      pHeader->idxRateStereo = idxRateStereo;

      cntBytes = 0;
      pStream = (Ipp8u*)out->pBuffer;
      if (idxRateWB < 0) { /* AMRWB+ modes */
         nbits = get_nb_bits(idxMode, idxRateWB, idxRateMono, idxRateStereo);
         if (in->pcmType.nChannels == 2) { /* stereo */
            apiEncoderStereo_AMRWBE(pEncObj_WBE, pHeader->pBufChannelRight, pHeader->pBufChannelLeft, idxRateMono,
               framesize, serial, pHeader->vad, pHeader->isf, idxRateStereo, &nSampProceeded);
         } else { /* mono */
            apiEncoderMono_AMRWBE(pEncObj_WBE, pHeader->pBufChannelRight, idxRateMono, framesize,
               serial, pHeader->vad, pHeader->isf, &nSampProceeded);
         }
         for (i=0; i<4; i++) {
            nBytes = WriteHeader_RAW(idxMode, pHeader->idxIsf, i, pStream);
            pStream += nBytes;   cntBytes += nBytes;
            nBytes = Bin2Bitstream_AMRWBE(serial, nbits, i, pStream);
            pStream += nBytes;   cntBytes += nBytes;
         }
      } else { /* AMRWB */
         Ipp32s idxWorkRate;
         Ipp32s frameType = 0;
         for (i=0; i<4; i++) {
            idxWorkRate = idxRateWB;
            ownEncode_AMRWB(pHeader, &idxWorkRate, &(pHeader->pBufChannelRight[i*AMRWB_Frame]),
               (Ipp8u*)serial, pHeader->vad, &frameType);
            nBytes = WriteHeader_RAW(idxWorkRate, 0, i, pStream);
            pStream += nBytes;   cntBytes += nBytes;
            nBytes = ownPackMMS_WB(idxWorkRate, serial, pStream, frameType, idxRateWB);
            pStream += nBytes;   cntBytes += nBytes;
         }
         nSampProceeded = AMRWB_Frame << 2;
      }
      pHeader->nSampProceeded = nSampProceeded;
      /* number of sample per channel to read from file */
      pHeader->nSampToProceed = (nSampProceeded*pHeader->downFact + pHeader->fracLagRight) / pHeader->upFact;

      //in->nbytes = nSampToProceed*sizeof(Ipp16s) * in->pcmType.nChannels;
      in->nbytes = wrkLen*sizeof(Ipp16s) * in->pcmType.nChannels;
      out->bitrate = in->bitrate;
      out->nbytes = cntBytes;
      out->frametype = 0;
   }
   return USC_NoError;
}

static USC_Status Decode(USC_Handle handle, USC_Bitstream *in, USC_PCMStream *out)
{
   HandleHeader_AMRWBE* pHeader;
   Ipp16s serial[NBITS_MAX]; /* serial parameters. */
   Ipp32s idxMode, idxRateWB, idxRateMono, idxRateStereo, idxISF;
   Ipp32s frameScale;
   Ipp32s bfi[4]={0,0,0,0};
   Ipp32s scaleISF;
   Ipp32s nSampProceeded, nSampToProceed, streamSize;
   Ipp32s frameType;
   Ipp32s modePLC=0;

   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(out);
   USC_BADARG((out->pBuffer==NULL), USC_NoOperation);

   pHeader = (HandleHeader_AMRWBE*)handle;
   if (pHeader->direction != USC_DECODE) {
      return USC_NoOperation;
   }

   if ((in!=NULL) && (in->pBuffer!=NULL)) { /*normal frame*/
      if (in->nbytes == 0 /*&& check frame type*/) {
         out->nbytes = 0;
         return USC_NoError;
      }

      idxRateWB = pHeader->idxRateWB;
      streamSize = ownReadBitstream_RAW((Ipp8u*)(in->pBuffer), serial, &idxISF,
         &idxMode, &idxRateWB, &idxRateMono, &idxRateStereo, bfi);
      scaleISF = isfIndex_fx[idxISF];

      if (pHeader->isFirstFrame) { /* initing by data */
         pHeader->isf = scaleISF;
         pHeader->idxIsf = idxISF;
         pHeader->idxMode = idxMode;
         pHeader->idxRateWB = idxRateWB;
         pHeader->idxRateMono = idxRateMono;
         pHeader->idxRateStereo = idxRateStereo;
         pHeader->isFirstFrame = 0;
      }
      frameType = in->frametype;
   } else { /* PLC */
      modePLC = 1;
      scaleISF = pHeader->isf;
      idxISF = pHeader->idxIsf;
      idxMode = pHeader->idxMode;
      idxRateWB = pHeader->idxRateWB;
      idxRateMono = pHeader->idxRateMono;
      idxRateStereo = pHeader->idxRateStereo;
      pHeader->isFirstFrame = 0;

      streamSize = ownReadBitstream_RAW((Ipp8u*)(NULL), serial, &idxISF,
         &idxMode, &idxRateWB, &idxRateMono, &idxRateStereo, bfi);
      ippsZero_16s(serial, streamSize*8);

      frameType = 15; /* 1111b */

   }

   if (frameType != 0) { /* SIMULATION of frame erasures */
      if (idxRateWB < 0) {
         bfi[0] = (frameType & 8) >> 3;
         bfi[1] = (frameType & 4) >> 2;
         bfi[2] = (frameType & 2) >> 1;
         bfi[3] = frameType & 1;
      } else {
         bfi[0] = frameType & 1;
      }
   }

   if (pHeader->idxIsf != idxISF) {
      scaleISF = isfIndex_fx[idxISF];
      //ownSelectSamplFreq(pHeader);
      //ownSetFrameLength(pHeader);
      apiDecoderInit_AMRWBE(pHeader->pObj_AMRWBE, scaleISF, 2/*(Ipp32s)(pHeader->codecMode)*/, 0);
   }

   if ( ((pHeader->idxRateWB>=0)&&(idxRateWB<0)) || ((pHeader->idxRateWB<0)&&(idxRateWB>=0)) ) {
      if (idxRateWB >= 0) { /* Switch from WB+ to WB*/
         apiCopyDecoderState_AMRWBE(pHeader->pObj_AMRWBE, pHeader->pObj_AMRWB, 1);
      } else if (idxRateWB < 0) { /* Switch from WB to WB+*/
         apiCopyDecoderState_AMRWBE(pHeader->pObj_AMRWBE, pHeader->pObj_AMRWB, 0);
      }
   }

   if (idxRateWB < 0) {
      apiDecoder_AMRWBE(pHeader->pObj_AMRWBE, serial, bfi, pHeader->frameSize, (Ipp32s)(pHeader->nChannels/*codecMode*/),
         pHeader->pBufChannelRight, pHeader->pBufChannelLeft, idxRateMono, idxRateStereo,
         (Ipp16s)scaleISF, (Ipp16s)pHeader->isf, &nSampProceeded, &frameScale);
   } else {
      Ipp32s i, nBytes=streamSize;
      const Ipp8u *pInBuff;
      if (!modePLC) {
         pInBuff = (Ipp8u*)in->pBuffer;
      } else {
         pInBuff = NULL;
      }
      ownDecode_AMRWB(pHeader, (Ipp8u*)(serial), pHeader->pBufChannelRight, bfi[0]);
      /*read & decode next three frames (will assume that no switches to WB+ through four successive frames)*/
      for (i=1; i<4; i++) {
         if (!modePLC) {
            pInBuff += nBytes;
         }
         nBytes = ownReadBitstream_RAW(pInBuff, serial, &idxISF,
            &idxMode, &idxRateWB, &idxRateMono, &idxRateStereo, bfi);
         streamSize += nBytes;
         frameType = frameType >> 1; /* SIMULATION of frame erasures */
         bfi[0] = (Ipp16s)(frameType & 1);
         ownDecode_AMRWB(pHeader, (Ipp8u*)(serial), (pHeader->pBufChannelRight)+i*L_FRAME16k, bfi[0]);
      }
      nSampProceeded = 4*L_FRAME16k;
      frameScale = 0;
   }

   nSampToProceed =
      ((nSampProceeded*pHeader->upFact)-(pHeader->fracLagRight)+(pHeader->downFact-1))/pHeader->downFact;

   if (pHeader->nChannels/*codecMode*/ == USC_OUT_STEREO) {
      /* Smooth the output so to avoid harsh clipping*/
      if (pHeader->agcFlag > 0) {
         Simple_frame_limiter(pHeader->pBufChannelRight, (Ipp16s)frameScale, pHeader->memAGCRight, nSampProceeded);
         Simple_frame_limiter(pHeader->pBufChannelLeft, (Ipp16s)frameScale, pHeader->memAGCLeft, nSampProceeded);
         frameScale = 0;
      }
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pMemResampRight, 2*L_FILT_DECIM_FS, (Ipp16s)(frameScale - pHeader->frameScale));
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pMemResampLeft, 2*L_FILT_DECIM_FS, (Ipp16s)(frameScale - pHeader->frameScale));

      ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelRight, nSampProceeded, pHeader->upFact, pHeader->downFact,
         &pHeader->fracLagRight, pHeader->pMemResampRight, pHeader->pBufChannelRight, nSampToProceed);
      ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelLeft, nSampProceeded, pHeader->upFact, pHeader->downFact,
         &pHeader->fracLagLeft, pHeader->pMemResampLeft, pHeader->pBufChannelLeft, nSampToProceed);

      /* interleave of left and right samples (stereo file format) */
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pBufChannelRight, nSampToProceed, (Ipp16s)(-frameScale));
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pBufChannelLeft, nSampToProceed, (Ipp16s)(-frameScale));
      /* Do the interleaving of channels*/
      ippsRealToCplx_16s( (const Ipp16s*) pHeader->pBufChannelLeft, (const Ipp16s*) pHeader->pBufChannelRight,
                          (Ipp16sc*) out->pBuffer, nSampToProceed);
   } else {
      if(pHeader->agcFlag > 0) {
         if (idxRateWB < 0) {
            Simple_frame_limiter(pHeader->pBufChannelRight, (Ipp16s)frameScale, pHeader->memAGCRight, nSampProceeded);
            frameScale = 0;
         } else {
            Ipp32s i;
            for (i=0; i<4; i++) {
               Simple_frame_limiter(&pHeader->pBufChannelRight[i*L_FRAME16k], 0, pHeader->memAGCRight, nSampProceeded/4);
            }
         }
      }
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pMemResampRight,2*L_FILT_DECIM_FS, (Ipp16s)(frameScale - pHeader->frameScale));

      ippsResamplePolyphase_AMRWBE_16s(pHeader->pBufChannelRight, nSampProceeded, pHeader->upFact, pHeader->downFact,
         &pHeader->fracLagRight, pHeader->pMemResampRight, pHeader->pBufChannelRight, nSampToProceed);
      ownScaleSignal_AMRWB_16s_ISfs(pHeader->pBufChannelRight, nSampToProceed, (Ipp16s)(-frameScale));
      ippsCopy_16s(pHeader->pBufChannelRight, (Ipp16s*)(out->pBuffer), nSampToProceed);
   }

   if (!modePLC) {
      in->nbytes = streamSize;
   }
   out->nbytes = nSampToProceed*((Ipp32s)(pHeader->nChannels/*codecMode*/))*sizeof(Ipp16s);

   pHeader->nSampToProceed = nSampToProceed;
   pHeader->frameScale = frameScale;
   pHeader->isf = scaleISF;
   pHeader->idxIsf = idxISF;
   pHeader->idxMode = idxMode;
   pHeader->idxRateWB = idxRateWB;
   pHeader->idxRateMono = idxRateMono;
   pHeader->idxRateStereo = idxRateStereo;

   return USC_NoError;
}


static USC_Status GetOutStreamSize(const USC_Option *pOptions, Ipp32s bitrate, Ipp32s nbytesSrc, Ipp32s *nbytesDst)
{
   Ipp32s nbits, nBytes, nSamples, nBlocks;
   Ipp32s idxMode, idxModeWB, idxModeMono, idxModeStereo, isf;
   USC_Status status;

   USC_CHECK_PTR(pOptions);
   USC_CHECK_PTR(nbytesDst);
   USC_BADARG(nbytesSrc <= 0, USC_NoOperation);

   isf = 0; /* not significant */
   status = RateToIdx(bitrate, pOptions->pcmType.nChannels, isf,
      &idxMode, &idxModeWB, &idxModeMono, &idxModeStereo);
   if (status) {
      return status;
   }

   if (pOptions->direction == USC_ENCODE) {
      nbits = get_nb_bits(idxMode, idxModeWB, idxModeMono, idxModeStereo);
      nBytes = (nbits/4+7)/8;
      nSamples = nbytesSrc;// / sizeof(Ipp16s);
      nBlocks = nSamples / pOptions->framesize;
      if (nBlocks == 0) {
         return USC_NoOperation;
      }
      if (0 != nSamples % AMRWB_Frame) {
         nBlocks++;
      }
      *nbytesDst = nBlocks * nBytes;

   } else if(pOptions->direction==USC_DECODE) {

      if ((pOptions->modes.vad) && (idxModeWB>=0)) {
         idxModeWB = AMRWB_RATE_DTX;
      }

      nbits = get_nb_bits(idxMode, idxModeWB, idxModeMono, idxModeStereo);
      nBytes = (nbits/4+7)/8;
      nBlocks = nbytesSrc / nBytes;
      if (nBlocks == 0) {
         return USC_NoOperation;
      }

      nSamples = nBlocks * pOptions->framesize * pOptions->pcmType.nChannels/*modes.codecMode*/;
      *nbytesDst = nSamples * sizeof(Ipp16s);


   } else {
      return USC_NoOperation;
   }

   return USC_NoError;
}


static USC_Status ownSetFrameSizeResampleFactor(Ipp32s pcmSampFreq, USC_Handle handle)
{
   Ipp16s resampFact;
   HandleHeader_AMRWBE *pHeader = (HandleHeader_AMRWBE*)handle;

   pHeader->frameSize = L_FRAME48k;
   pHeader->upFact = pHeader->downFact = resampFact = 12;

   if (pHeader->isf != 0) {
      switch (pcmSampFreq) {
         case  8000 : resampFact = 2;  break;
         case 16000 : resampFact = 4;  break;
         case 24000 : resampFact = 6;  break;
         case 32000 : resampFact = 8;  break;
         case 48000 : break;
         case 11025 : resampFact = 3;  pHeader->frameSize = L_FRAME44k;  break;
         case 22050 : resampFact = 6;  pHeader->frameSize = L_FRAME44k;  break;
         case 44100 : pHeader->frameSize = L_FRAME44k;  break;
         default:
            //error in sampling frequency.  choice of filter are:
            //11, 22, 44 kHz (FILTER_44kHz)
            //8, 16, 24, 32, 48 kHz (FILTER_48kHz)
            return USC_UnsupportedPCMType;
      }
      if (pHeader->direction == USC_ENCODE) { /*encoder*/
         pHeader->frameSize *= 2;
      }
   } else {
      switch (pcmSampFreq) {
         case  8000 : pHeader->frameSize = (L_FRAME8k);  break;
         case 16000 : pHeader->frameSize = (L_FRAME16kPLUS);  break;
         case 24000 : pHeader->frameSize = (L_FRAME24k);  break;
         default :
            //error in sampling freq: without fsratio(isf) only 8, 16 or 24 kHz are allowed
            return USC_UnsupportedPCMType;
      }
   }
   if (pHeader->direction == USC_ENCODE) { /* encoder */
      pHeader->downFact = resampFact;
   } else if (pHeader->direction == USC_DECODE) { /* decoder */
      pHeader->upFact = resampFact;
   } else {
      return USC_NoOperation;
   }
   return USC_NoError;
}


static USC_Status RateToIdx (Ipp32s rate, Ipp32s nChannels, Ipp32s isf,
                      Ipp32s* pIdxMode, Ipp32s* pIdxRateWB, Ipp32s* pIdxRateMono, Ipp32s* pIdxRateStereo)
{
   switch (rate) {
      /* AMRWB modes*/
      case 6600 :
         *pIdxMode= 0;  *pIdxRateWB= 0;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 8850 :
         *pIdxMode= 1;  *pIdxRateWB= 1;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 12650 :
         *pIdxMode= 2;  *pIdxRateWB= 2;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 14250 :
         *pIdxMode= 3;  *pIdxRateWB= 3;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 15850 :
         *pIdxMode= 4;  *pIdxRateWB= 4;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 18250 :
         *pIdxMode= 5;  *pIdxRateWB= 5;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 19850 :
         *pIdxMode= 6;  *pIdxRateWB= 6;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 23050 :
         *pIdxMode= 7;  *pIdxRateWB= 7;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      case 23850 :
         *pIdxMode= 8;  *pIdxRateWB= 8;  *pIdxRateMono=-1;  *pIdxRateStereo=-1;  break;
      /* AMRWB+ mono modes */
      case 10400 :
         *pIdxMode=16;  *pIdxRateWB=-1;  *pIdxRateMono= 0;  *pIdxRateStereo=-1;  break;
      case 12000 :
         *pIdxMode=17;  *pIdxRateWB=-1;  *pIdxRateMono= 1;  *pIdxRateStereo=-1;  break;
      case 13600 :
         *pIdxMode = (isf == 0) ? 10 : 18;
         *pIdxRateWB=-1;  *pIdxRateMono= 2;  *pIdxRateStereo=-1;  break;
      case 15200 :
         if (nChannels == 1) { /* mono mode */
            *pIdxMode=19;  *pIdxRateWB=-1;  *pIdxRateMono= 3;  *pIdxRateStereo=-1;  break;
         } else if (nChannels == 2) { /* stereo mode */
            *pIdxMode=28;  *pIdxRateWB=-1;  *pIdxRateMono= 1;  *pIdxRateStereo= 3;  break;
         } else {
            return USC_UnsupportedBitRate;
         }
      case 16800 :
         *pIdxMode=20;  *pIdxRateWB=-1;  *pIdxRateMono= 4;  *pIdxRateStereo=-1;  break;
      case 19200 :
         if (nChannels == 1) {
            *pIdxMode=21;  *pIdxRateWB=-1;  *pIdxRateMono= 5;  *pIdxRateStereo=-1;  break;
         } else if (nChannels == 2) {
            *pIdxMode=34;  *pIdxRateWB=-1;  *pIdxRateMono= 3;  *pIdxRateStereo= 5;  break;
         } else {
            return USC_UnsupportedBitRate;
         }
      case 20800 :
         *pIdxMode=22;  *pIdxRateWB=-1;  *pIdxRateMono= 6;  *pIdxRateStereo=-1;  break;
      case 24000 :
         if (nChannels == 1) {
            *pIdxMode = (isf == 0) ? 12 : 23;
            *pIdxRateWB=-1;  *pIdxRateMono= 7;  *pIdxRateStereo=-1;  break;
         } else if (nChannels == 2) {
            *pIdxMode = (isf == 0) ? 13 : 40;
            *pIdxRateWB=-1;  *pIdxRateMono= 5;  *pIdxRateStereo= 7;  break;
         } else {
            return USC_UnsupportedBitRate;
         }
      /* AMRWB+ stereo modes */
      case 12400 :
         *pIdxMode=24;  *pIdxRateWB=-1;  *pIdxRateMono= 0;  *pIdxRateStereo= 0;  break;
      case 12800 :
         *pIdxMode=25;  *pIdxRateWB=-1;  *pIdxRateMono= 0;  *pIdxRateStereo= 1;  break;
      case 14000 :
         *pIdxMode=26;  *pIdxRateWB=-1;  *pIdxRateMono= 0;  *pIdxRateStereo= 4;  break;
      case 14400 :
         *pIdxMode=27;  *pIdxRateWB=-1;  *pIdxRateMono= 1;  *pIdxRateStereo= 1;  break;
      case 16000 :
         *pIdxMode=29;  *pIdxRateWB=-1;  *pIdxRateMono= 1;  *pIdxRateStereo= 5;  break;
      case 16400 :
         *pIdxMode=30;  *pIdxRateWB=-1;  *pIdxRateMono= 2;  *pIdxRateStereo= 2;  break;
      case 17200 :
         *pIdxMode=31;  *pIdxRateWB=-1;  *pIdxRateMono= 2;  *pIdxRateStereo= 4;  break;
      case 18000 :
         *pIdxMode = (isf == 0) ? 11 : 32;
         *pIdxRateWB=-1;  *pIdxRateMono= 2;  *pIdxRateStereo= 6;  break;
      case 18400 :
         *pIdxMode=33;  *pIdxRateWB=-1;  *pIdxRateMono= 3;  *pIdxRateStereo= 3;  break;
      case 20000 :
         *pIdxMode=35;  *pIdxRateWB=-1;  *pIdxRateMono= 3;  *pIdxRateStereo= 7;  break;
      case 20400 :
         *pIdxMode=36;  *pIdxRateWB=-1;  *pIdxRateMono= 4;  *pIdxRateStereo= 4;  break;
      case 21200 :
         *pIdxMode=37;  *pIdxRateWB=-1;  *pIdxRateMono= 4;  *pIdxRateStereo= 6;  break;
      case 22400 :
         *pIdxMode=38;  *pIdxRateWB=-1;  *pIdxRateMono= 4;  *pIdxRateStereo= 9;  break;
      case 23200 :
         *pIdxMode=39;  *pIdxRateWB=-1;  *pIdxRateMono= 5;  *pIdxRateStereo= 5;  break;
      case 25600 :
         *pIdxMode=41;  *pIdxRateWB=-1;  *pIdxRateMono= 5;  *pIdxRateStereo=11;  break;
      case 26000 :
         *pIdxMode=42;  *pIdxRateWB=-1;  *pIdxRateMono= 6;  *pIdxRateStereo= 8;  break;
      case 26800 :
         *pIdxMode=43;  *pIdxRateWB=-1;  *pIdxRateMono= 6;  *pIdxRateStereo=10;  break;
      case 28800 :
         *pIdxMode=44;  *pIdxRateWB=-1;  *pIdxRateMono= 6;  *pIdxRateStereo=15;  break;
      case 29600 :
         *pIdxMode=45;  *pIdxRateWB=-1;  *pIdxRateMono= 7;  *pIdxRateStereo= 9;  break;
      case 30000 :
         *pIdxMode=46;  *pIdxRateWB=-1;  *pIdxRateMono= 7;  *pIdxRateStereo=10;  break;
      case 32000 :
         *pIdxMode=47;  *pIdxRateWB=-1;  *pIdxRateMono= 7;  *pIdxRateStereo=15;  break;
      default :
         return USC_UnsupportedBitRate;
   }
   return USC_NoError;
}
static Ipp32s IdxToRate (Ipp32s idxMode, Ipp32s* pRate)
{
   switch (idxMode) {
      /* AMRWB modes*/
      case 0 : *pRate=  6600;  break;
      case 1 : *pRate=  8850;  break;
      case 2 : *pRate= 12650;  break;
      case 3 : *pRate= 14250;  break;
      case 4 : *pRate= 15850;  break;
      case 5 : *pRate= 18250;  break;
      case 6 : *pRate= 19850;  break;
      case 7 : *pRate= 23050;  break;
      case 8 : *pRate= 23850;  break;
      /* ARMWB character modes (for WB <-> WB+) */
      case 10 : *pRate=13600;  break;
      case 11 : *pRate=18000;  break;
      case 12 : *pRate=24000;  break;
      case 13 : *pRate=24000;  break;
      /* AMRWB+ mono modes */
      case 16 : *pRate=10400;  break;
      case 17 : *pRate=12000;  break;
      case 18 : *pRate=13600;  break;
      case 19 : *pRate=15200;  break;
      case 20 : *pRate=16800;  break;
      case 21 : *pRate=19200;  break;
      case 22 : *pRate=20800;  break;
      case 23 : *pRate=24000;  break;
      /* AMRWB+ stereo modes */
      case 24 : *pRate=12400;  break;
      case 25 : *pRate=12800;  break;
      case 26 : *pRate=14000;  break;
      case 27 : *pRate=14400;  break;
      case 28 : *pRate=15200;  break;
      case 29 : *pRate=16000;  break;
      case 30 : *pRate=16400;  break;
      case 31 : *pRate=17200;  break;
      case 32 : *pRate=18000;  break;
      case 33 : *pRate=18400;  break;
      case 34 : *pRate=19200;  break;
      case 35 : *pRate=20000;  break;
      case 36 : *pRate=20400;  break;
      case 37 : *pRate=21200;  break;
      case 38 : *pRate=22400;  break;
      case 39 : *pRate=23200;  break;
      case 40 : *pRate=24000;  break;
      case 41 : *pRate=25600;  break;
      case 42 : *pRate=26000;  break;
      case 43 : *pRate=26800;  break;
      case 44 : *pRate=28800;  break;
      case 45 : *pRate=29600;  break;
      case 46 : *pRate=30000;  break;
      case 47 : *pRate=32000;  break;
      default :
         return -1;
   }
   return 0;
}
static __ALIGN(16) CONST Ipp16s miMode_fx[3 * 48] = {
/*WB|mono|stereo*/
  0, -1, -1, //0
  1, -1, -1, //1
  2, -1, -1, //2
  3, -1, -1, //3
  4, -1, -1, //4
  5, -1, -1, //5
  6, -1, -1, //6
  7, -1, -1, //7
  8, -1, -1, //8
  9, -1, -1, //WB SID (==DTX?)
 -1,  2, -1, //10
 -1,  2,  6, //11
 -1,  7, -1, //12
 -1,  5,  7, //13
 -2, -2, -2, //14 - FRAME_ERASURE
 -3, -3, -3, //15 - NO_DATA
 -1,  0, -1, //16
 -1,  1, -1, //17
 -1,  2, -1, //18
 -1,  3, -1, //19
 -1,  4, -1, //20
 -1,  5, -1, //21
 -1,  6, -1, //22
 -1,  7, -1, //23
 -1,  0,  0, //24
 -1,  0,  1, //25
 -1,  0,  4, //26
 -1,  1,  1, //27
 -1,  1,  3, //28
 -1,  1,  5, //29
 -1,  2,  2, //30
 -1,  2,  4, //31
 -1,  2,  6, //32
 -1,  3,  3, //33
 -1,  3,  5, //34
 -1,  3,  7, //35
 -1,  4,  4, //36
 -1,  4,  6, //37
 -1,  4,  9, //38
 -1,  5,  5, //39
 -1,  5,  7, //40
 -1,  5, 11, //41
 -1,  6,  8, //42
 -1,  6, 10, //43
 -1,  6, 15, //44
 -1,  7,  9, //45
 -1,  7, 10, //46
 -1,  7, 15  //47
};

static Ipp32s get_isf_index(Ipp32s *fscale)
{
   Ipp32s index, i;
   Ipp32s dist, tmp16;
   index = 0;
   dist = 512;
   /* Mono mode only */
   for (i=0; i<14; i++) {
      tmp16 = Abs_32s((*fscale) - isfIndex_fx[i]);
      if(tmp16 < dist) {
         dist = tmp16;
         index = i;
      }
   }
   *fscale = isfIndex_fx[index];
   return index;
}

static Ipp32s get_nb_bits(Ipp32s idxMode, Ipp32s idxRateWB, Ipp32s idxRateMono, Ipp32s idxRateStereo)
{
   Ipp32s nb_bits;

   if ((idxMode!=14) && (idxMode!=15)) {
      if (idxRateWB < 0) {
         nb_bits = tblNBitsMonoCore_16s[idxRateMono] + NBITS_BWE;
         if (idxRateStereo >= 0) {
            nb_bits = nb_bits + (tblNBitsStereoCore_16s[idxRateStereo] + NBITS_BWE);
         }
      } else {
         nb_bits = tblNBitsAMRWBCore_16s[idxRateWB];
      }
   } else {
      nb_bits = 0;
   }
   return (Ipp32s)nb_bits;
}

static Ipp32s WriteHeader_RAW(Ipp32s idxMode, Ipp32s idxIsf, Ipp32s offset, Ipp8u* pStream)
{
   Ipp8u byte;

   byte = (Ipp8u)idxMode;
   pStream[0] = byte;  /*write mode index information */
   byte = (Ipp8u)(offset<<6);
   byte = (Ipp8u)(byte + (Ipp8u)(idxIsf & 0x1F));
   pStream[1] = byte;
   return 2; /* 2 bytes written */
}

static Ipp32s Bin2Bitstream_AMRWBE(const Ipp16s* pSerial, Ipp32s length, Ipp32s offset, Ipp8u* pStream)
{
   Ipp32s j, k, nb_byte;
   const Ipp16s *ptr;
   Ipp8u byte;

   nb_byte = ((length / 4) + 7) / 8;
   ptr = &pSerial[offset * (length / 4)];

   for (j=0; j<nb_byte; j++) {
      byte = 0;
      for (k=0; k<8; k++,ptr++) {
         byte <<= 1;
         if (*ptr != 0)
            byte += 1;
      }
      pStream[j] = byte;
   }
   return nb_byte;
}

static Ipp32s ownSidSyncInit(SIDSyncState *st)
{
    st->updateSidCount = 3;
    st->sid_handover_debt = 0;
    st->prevFrameType = TX_SPEECH;
    return 0;
}
static Ipp32s ownTestPCMFrameHoming(const Ipp16s *pSrc)
{
   Ipp32s i, fl = 0;
   for (i = 0; i < AMRWB_Frame; i++) {
      fl = pSrc[i] ^ EHF_MASK;
      if (fl) break;
   }
   return (!fl);
}

static USC_Status ownEncode_AMRWB(USC_Handle handle, Ipp32s* pIdxWorkMode,
                           Ipp16s *speech, Ipp8u *prms, Ipp32s dtx, Ipp32s* pFrameType)
{
   Ipp32s reset_flag;
   IPP_ALIGNED_ARRAY(16, Ipp16u, work_buf, (AMRWB_Frame));
   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;

   /* check for homing frame */
   reset_flag = ownTestPCMFrameHoming(speech);
   if (!reset_flag) {

      ippsAndC_16u((Ipp16u*)speech, (Ipp16u)IO_MASK, work_buf, AMRWB_Frame);

      if (apiAMRWBEncode(pHeader->pObj_AMRWB, work_buf, (Ipp8u*)prms, (AMRWB_Rate_t*)pIdxWorkMode, dtx, WBP_OFFSET)) {
         return USC_BadArgument;
      }

      if ((*pIdxWorkMode) == AMRWB_RATE_DTX) {
         pHeader->sid_state.updateSidCount--;
         if (pHeader->sid_state.prevFrameType == TX_SPEECH) {
            *pFrameType = TX_SID_FIRST;
            pHeader->sid_state.updateSidCount = 3;
         } else {
            if ((pHeader->sid_state.sid_handover_debt>0) && (pHeader->sid_state.updateSidCount>2)) {
               /* ensure extra updates are properly delayed after a possible SID_FIRST */
               *pFrameType = TX_SID_UPDATE;
               pHeader->sid_state.sid_handover_debt--;
            } else {
               if (pHeader->sid_state.updateSidCount == 0) {
                  *pFrameType = TX_SID_UPDATE;
                  pHeader->sid_state.updateSidCount = 8;
               } else {
                  *pFrameType = TX_NO_DATA;
                  (*pIdxWorkMode) = AMRWB_RATE_NO_DATA;
               }
            }
         }
      } else {
         pHeader->sid_state.updateSidCount = 8;
         *pFrameType = TX_SPEECH;
      }
      pHeader->sid_state.prevFrameType = (TXFrameType)*pFrameType;
   } else { /* perform homing if homing frame was detected at encoder input */
      apiAMRWBEncoder_Init(pHeader->pObj_AMRWB, dtx);
      ownSidSyncInit(&(pHeader->sid_state));
      ippsCopy_16s(tblHomingFrames_WB[*pIdxWorkMode], (Ipp16s*)prms, NumberPrmsTbl[*pIdxWorkMode]);
      *pFrameType = TX_SPEECH;
   }

   return USC_NoError;
}

static Ipp32s ownPackMMS_WB(Ipp32s mode, Ipp16s *param, Ipp8u *stream, Ipp32s frame_type, Ipp32s speech_mode)
{
   Ipp32s j=0, nBits;
   const Ipp16s* pIdx;
   const Ipp16s* pMask;

   for(j=0; j<(tblPacketSizeMMS_WB[mode]-1); j++) {
      stream[j] = 0;
   }

   switch(mode) {
   case AMRWB_RATE_6600:
      nBits = NUMBITS6600;
      pIdx = tblMode7k_idx;
      pMask = tblMode7k_mask;
      break;
   case AMRWB_RATE_8850:
      nBits = NUMBITS8850;
      pIdx = tblMode9k_idx;
      pMask = tblMode9k_mask;
      break;
   case AMRWB_RATE_12650:
      nBits = NUMBITS12650;
      pIdx = tblMode12k_idx;
      pMask = tblMode12k_mask;
      break;
   case AMRWB_RATE_14250:
      nBits = NUMBITS14250;
      pIdx = tblMode14k_idx;
      pMask = tblMode14k_mask;
      break;
   case AMRWB_RATE_15850:
      nBits = NUMBITS15850;
      pIdx = tblMode16k_idx;
      pMask = tblMode16k_mask;
      break;
   case AMRWB_RATE_18250:
      nBits = NUMBITS18250;
      pIdx = tblMode18k_idx;
      pMask = tblMode18k_mask;
      break;
   case AMRWB_RATE_19850:
      nBits = NUMBITS19850;
      pIdx = tblMode20k_idx;
      pMask = tblMode20k_mask;
      break;
   case AMRWB_RATE_23050:
      nBits = NUMBITS23050;
      pIdx = tblMode23k_idx;
      pMask = tblMode23k_mask;
      break;
   case AMRWB_RATE_23850:
      nBits = NUMBITS23850;
      pIdx = tblMode24k_idx;
      pMask = tblMode24k_mask;
      break;
   case AMRWB_RATE_DTX:
      nBits = NUMBITS_SID;
      pIdx = tblModeDTX_idx;
      pMask = tblModeDTX_mask;
      break;
   case AMRWB_RATE_NO_DATA:
      return 0;
   default:
      return 0;
   }

   for (j=1; j<=nBits; j++) {
      if (param[pIdx[j-1]] & pMask[j-1]) {
         *stream = (Ipp8u)((*stream) + 1);
      }
      if (j & 7) {
         *stream = (Ipp8u)((*stream)<<1);
      } else {
         stream++;
      }
   }
   if (mode != AMRWB_RATE_DTX) {
      while (j & 7) {
         *stream = (Ipp8u)((*stream)<<1);
         j = j + 1;
      }
   } else {
      /* sid type */
      if (frame_type == TX_SID_UPDATE) { /* sid update */
         *stream = (Ipp8u)((*stream) + 1);
      }
      /* speech mode indicator */
      *stream  = (Ipp8u)((*stream) << 4);
      *stream = (Ipp8u)((*stream) + speech_mode);
      j = 40;
   }
   return (j>>3);
}



static void ownBitstream2Bin_AMRWBE(const Ipp8u* pStream, Ipp16s* pSerial, Ipp32s nBytes)
{
   Ipp32s i, j;
   Ipp8u byte;
   for (i=0; i<nBytes; i++) {
      byte = pStream[i];
      for (j=0; j<8; j++,pSerial++) {
         *pSerial = (Ipp16s)((byte&128)>>7);
         byte = (Ipp8u)(byte << 1);
      }
   }
}

static Ipp32s ownReadHeader_RAW(const Ipp8u* pStream, Ipp32s *pIdxMode, Ipp32s *pIdxISF, Ipp32s *pBFI)
{
   // pIdxMode bring the previous WB mode index (if <0, then WB+ mode)
   // pIdxMode take off new mode index
   Ipp8u byte;
   Ipp32s idxMode, /*idxTF,*/ idxISF;

   byte = pStream[0];
   idxMode = byte & 127;

   byte = pStream[1]; /* read one more byte to ensure empty header */
   /*idxTF = (byte & 0xc0)>>6;*/ /* tfi extrapolated by RTP packetizer */
   idxISF = (byte & 0x1F);
   *pIdxISF = idxISF;

   if ( (idxMode>47) || (idxMode<0) /* mode unknown */
      || (idxMode==14) || (idxMode==15) /* Frame lost or erased */
      || ((idxMode==9) && (*pIdxMode<0)) ) { /* WB SID in WB+ frame  not supported case so declare a NO_DATA*/

      if(idxMode == 14) { /* frame lost WB or WB+*/
         *pBFI = 1;
      } else if (idxMode == 15) { /* DTX in WB reset BFI vector (???? in WB DTX==9 !!!!)*/
         *pBFI = 0;
      }
      *pIdxMode = idxMode;
      return -1;
   }
   *pIdxMode = idxMode;
   *pBFI = 0; /* good frame */
   return 2; /* two bytes read */
}

static Ipp32s ownReadBitstream_RAW(const Ipp8u* pStream, Ipp16s* pSerial,
   Ipp32s *pIdxISF, Ipp32s* pIdxMode, Ipp32s* pIdxRateWB, Ipp32s* pIdxRateMono, Ipp32s* pIdxRateStereo, Ipp32s *pBFI)
{
   Ipp32s i, nBytes=0, streamSize=0, nbits, res;
   Ipp16s *pSer = pSerial;
   Ipp8u byte;
   Ipp32s idxMode;

   idxMode = *pIdxRateWB; /* as previous decoder mode (<0 - wb+, >=0 - wb)*/
   pSer = pSerial;

   if (pStream != NULL) {

      /* assume there is no amrwb -> wb+ switching inside superframe */
      for (i=0; i<4; i++) {
         res = ownReadHeader_RAW(pStream, &idxMode, pIdxISF, &pBFI[i]);
         pStream += 2; // 2 bytes header is read
         streamSize += 2;

         if (res > 0) { /* normal data frame */
            *pIdxMode = idxMode;
            *pIdxRateWB = miMode_fx[idxMode*3];
            *pIdxRateMono = miMode_fx[idxMode*3+1];
            *pIdxRateStereo = miMode_fx[idxMode*3+2];
            if (*pIdxRateWB < 0) { /* WB+ mode */
               nbits = get_nb_bits(idxMode, *pIdxRateWB, *pIdxRateMono, *pIdxRateStereo);
               nBytes = (nbits/4+7)/8;
               streamSize += nBytes;
               ownBitstream2Bin_AMRWBE(pStream, pSer, nBytes);
               pStream += nBytes; /* Ipp8u */
               pSer += (nbits/4); /* Ipp16s */
            } else { /* WB modes, read 20ms frame and return */
               /* just write IF2 header and copy data */
               /* write IF2 Header */
               nBytes = tblPacketSizeMMS_WB[*pIdxRateWB];
               streamSize = nBytes + 1;
               byte = 4; /* FQI = good frame */
               byte = (Ipp8u)(byte + (Ipp8u)((*pIdxRateWB)<<3));
               ((Ipp8u*)pSer)[0] = byte; /* Frame Type */
               /* get data */
               ippsCopy_8u(pStream, &(((Ipp8u*)pSer)[1]), (nBytes-1));
               return streamSize;
            }
         } else if (*pIdxRateWB >= 0) { /* if previous mode was WB_mode */
            if (idxMode == 15) { /* DTX FRAME (NO DATA)*/
               ((Ipp8u*)pSerial)[0] = 0x7C; /* need in AMR WB */
            } else if (idxMode == 14) { /* Frame lost */
               ((Ipp8u*)pSerial)[0] = 0x74; /* need in AMR WB */
            }
            return 2;
         }
      }
   } else { //PLC mode, just return streamSize
      if (*pIdxRateWB < 0) { /* WB+ mode */
         nbits = get_nb_bits(idxMode, *pIdxRateWB, *pIdxRateMono, *pIdxRateStereo);
         nBytes = (nbits/4+7)/8;
         streamSize = 4*(2+nBytes);
      } else { /* WB modes, read 20ms frame and return */
         nBytes = tblPacketSizeMMS_WB[*pIdxRateWB];
         streamSize = nBytes + 1;
      }
   }
   return streamSize;
}


static USC_Status ownCheckSamplFreq(int samplFreq)
{
   int i;
   for (i=0; i<AMRWBE_NUM_PCM_TYPES; i++) {
      if (samplFreq == pcmTypesTbl_AMRWBE[i].sample_frequency) {
         return USC_NoError;
      }
   }
   return USC_UnsupportedPCMType;
}

static USC_Status ownSelectSamplFreq(USC_Handle handle)
{
   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;
   if (pHeader->osf == 0) { /* default sampling rate if undefined */
      if (pHeader->idxRateWB == 0) {
         pHeader->osf=16000; /* 16khz for AMRWB */
      } else if (pHeader->isf == 0) {
         pHeader->osf = 24000; /* 24khz */
      } else {
         pHeader->osf = 48000; /* 48kHz */
      }
   } else { /* user specified sampling rate */
      if(pHeader->idxRateWB == 0) {
         if(pHeader->osf != 16000) {
            return USC_UnsupportedPCMType;
         }
      } else {
         if (pHeader->isf == 0) {
            if ((pHeader->osf!=16000) && (pHeader->osf!=24000) && (pHeader->osf!=8000)) {
               return USC_UnsupportedPCMType;
            }
         } else {
            if ((pHeader->osf!=44100) && (pHeader->osf!=48000)) {
               return USC_UnsupportedPCMType;
            }
         }
      }
   }
   return USC_NoError;
}
static USC_Status ownSetFrameLength(USC_Handle handle)
{
   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;
   switch (pHeader->osf) {
      case  8000: pHeader->frameSize = (L_FRAME8k)*sizeof(Ipp16s);  break;
      case 16000: pHeader->frameSize = L_FRAME16kPLUS;  break;
      case 24000: pHeader->frameSize = L_FRAME24k;  break;
      case 32000: pHeader->frameSize = L_FRAME32k;  break;
      case 44100: pHeader->frameSize = L_FRAME44k;  break;
      case 48000: pHeader->frameSize = L_FRAME48k;  break;
      default:
         return USC_UnsupportedPCMType;
   }
   return USC_NoError;
}


static Ipp16s ownUnpackMMS_WB(const Ipp8u *pStream, Ipp16s *pParams,
      Ipp8u *frame_type, Ipp16s *speech_mode, Ipp16s *fqi)
{
   Ipp16s mode;
   Ipp32s i, j, nBits;
   Ipp32u tmp;
   const Ipp16s* pIdx;
   const Ipp16s* pMask;

   *fqi = (Ipp16s)(((pStream[0]) >> 2) & 0x01);
   mode = (Ipp16s)(((pStream[0]) >> 3) & 0x0F);
   switch (mode) {
   case AMRWB_RATE_6600:
      nBits = NUMBITS6600;
      pIdx = tblMode7k_idx;
      pMask = tblMode7k_mask;
      break;
   case AMRWB_RATE_8850:
      nBits = NUMBITS8850;
      pIdx = tblMode9k_idx;
      pMask = tblMode9k_mask;
      break;
   case AMRWB_RATE_12650:
      nBits = NUMBITS12650;
      pIdx = tblMode12k_idx;
      pMask = tblMode12k_mask;
      break;
   case AMRWB_RATE_14250:
      nBits = NUMBITS14250;
      pIdx = tblMode14k_idx;
      pMask = tblMode14k_mask;
      break;
   case AMRWB_RATE_15850:
      nBits = NUMBITS15850;
      pIdx = tblMode16k_idx;
      pMask = tblMode16k_mask;
      break;
   case AMRWB_RATE_18250:
      nBits = NUMBITS18250;
      pIdx = tblMode18k_idx;
      pMask = tblMode18k_mask;
      break;
   case AMRWB_RATE_19850:
      nBits = NUMBITS19850;
      pIdx = tblMode20k_idx;
      pMask = tblMode20k_mask;
      break;
   case AMRWB_RATE_23050:
      nBits = NUMBITS23050;
      pIdx = tblMode23k_idx;
      pMask = tblMode23k_mask;
      break;
   case AMRWB_RATE_23850:
      nBits = NUMBITS23850;
      pIdx = tblMode24k_idx;
      pMask = tblMode24k_mask;
      break;
   case AMRWB_RATE_DTX:
      nBits = NUMBITS_SID;
      pIdx = tblModeDTX_idx;
      pMask = tblModeDTX_mask;
      break;
   case AMRWB_RATE_NO_DATA:
      *frame_type = RX_NO_DATA;
      return mode;
   case AMRWB_RATE_LOST_FRAME:
      *frame_type = RX_SPEECH_LOST;
      return mode;
   default:
      *frame_type = RX_SPEECH_LOST;
      *fqi = 0;
      return mode;
   }

   *frame_type = RX_SPEECH_GOOD;

   tmp = (Ipp32u)pStream[1];
   for (i=2,j=1; j<=nBits; j++) {
      if (tmp & 0x80) {
         pParams[pIdx[j-1]] = (Ipp16s)(pParams[pIdx[j-1]] + pMask[j-1]);
      }
      if ( j & 0x07) {
         tmp = tmp<<1;
      } else {
         tmp = (Ipp32u)pStream[i];
         i++;
      }
   }

   if (mode == AMRWB_RATE_DTX) {
      /* get SID type bit */
      *frame_type = RX_SID_FIRST;
      if (tmp & 0x80) {
         *frame_type = RX_SID_UPDATE;
      }
      tmp &= 127;
      /* speech mode indicator */
      *speech_mode = (Ipp16s)(tmp >> 3);
   }

   if (*fqi == 0) {
      if (*frame_type == RX_SPEECH_GOOD) {
         *frame_type = RX_SPEECH_BAD;
      }
      if ((*frame_type==RX_SID_FIRST) | (*frame_type==RX_SID_UPDATE) ) {
         *frame_type = RX_SID_BAD;
      }
   }

   return mode;
}

#define _good_frame  0
#define _bad_frame   1
#define _lost_frame  2
#define _no_frame    3
static Ipp32s ownDecode_AMRWB(USC_Handle handle, Ipp8u *serial, Ipp16s *speech, Ipp32s bfi)
{
   Ipp8u frame_type;
   Ipp16s prm[PRMNO_MR23850 + 1];
   Ipp16s mode = 0;
   Ipp16s speech_mode = 0;
   Ipp16s fqi;
   Ipp32s reset_flag=0;
   Ipp32s i;

   HandleHeader_AMRWBE* pHeader = (HandleHeader_AMRWBE*)handle;
   ippsZero_16s(prm, PRMNO_MR23850+1);

   if ((bfi == _good_frame) || (bfi == _bad_frame)) {
      *serial = (Ipp8u)((Ipp16s)*serial & ~(bfi<<2));
      mode = ownUnpackMMS_WB(serial, prm, &frame_type, &speech_mode, &fqi);
   } else if (bfi == _no_frame) {
      frame_type = RX_NO_DATA;
   } else {
      frame_type = RX_SPEECH_LOST;
   }

   /* if no mode information guess one from the previous frame */
   if ((frame_type==RX_SPEECH_LOST) | (frame_type==RX_NO_DATA) ) {
      mode = (Ipp16s)pHeader->prevMode;
   }
   if (mode == AMRWB_RATE_DTX) {
      mode = speech_mode;
   }
   /* if homed: check if this frame is another homing frame */
   if (pHeader->prevResetFlag == 1) {
      /* only check until end of first subframe */
      reset_flag = ownTestHomingFrameFirst(prm, mode);
   }

   /* produce encoder homing frame if homed & input=decoder homing frame */
   if ((reset_flag != 0) && (pHeader->prevResetFlag != 0)) {
      for (i = 0; i < L_FRAME16k; i++) {
         speech[i] = EHF_MASK;
      }
   } else {
      apiAMRWBDecode(pHeader->pObj_AMRWB, (const Ipp8u*)prm, mode, frame_type, (Ipp16u*)speech, WBP_OFFSET);
   }

   /* Delete the 2 LSBs (14-bit input) */
   ippsAndC_16u((Ipp16u*)speech, 0xfffC, (Ipp16u*)speech, L_FRAME16k);

   /* if not homed: check whether current frame is a homing frame */
   if ((pHeader->prevResetFlag==0) & (mode < 9)) {
      /* check whole frame */
      reset_flag = ownTestHomingFrame(prm, mode);
   }
   /* reset decoder if current frame is a homing frame */
   if (reset_flag != 0) {
      apiAMRWBDecoder_Init(pHeader->pObj_AMRWB);
   }
   pHeader->prevResetFlag = (Ipp16s)reset_flag;
   pHeader->prevFrameType = frame_type;
   pHeader->prevMode = mode;
   return 0;
}

static __ALIGN(16) CONST Ipp16s Nb_of_param_first[9] = {
   9,  14, 15, 15, 15, 19, 19, 19, 19
};
static Ipp32s dhf_test(Ipp16s input_frame[], const Ipp16s mdhf[], Ipp32s nparms)
{
   Ipp32s i, j;
   /* check if the parameters matches the parameters of the corresponding decoder homing frame */
   j = 0;
   for (i = 0; i < nparms; i++) {
      j = (Ipp32s)(input_frame[i]^mdhf[i]);
      if (j)
         break;
   }
   return (!j);
}
static Ipp32s ownTestHomingFrame(Ipp16s* input_frame, Ipp16s mode)
{
   /* perform test for COMPLETE parameter frame */
   if (mode != AMRWB_RATE_23850) {
      return dhf_test(input_frame, &tblHomingFrames_WB[mode][0], NumberPrmsTbl[mode]);
   } else {
      Ipp32s i = 0;
      i = dhf_test(input_frame,  &tblHomingFrames_WB[AMRWB_RATE_23850][0], 19)|
         dhf_test(input_frame+20, &tblHomingFrames_WB[AMRWB_RATE_23850][20], 11) |
         dhf_test(input_frame+32, &tblHomingFrames_WB[AMRWB_RATE_23850][32], 11)|
         dhf_test(input_frame+44, &tblHomingFrames_WB[AMRWB_RATE_23850][44], 11);
      return i;
   }
}
static Ipp32s ownTestHomingFrameFirst(Ipp16s* input_frame, Ipp16s mode)
{
   /* perform test for FIRST SUBFRAME of parameter frame ONLY */
   return dhf_test(input_frame, &tblHomingFrames_WB[mode][0], Nb_of_param_first[mode]);
}

static Ipp32s ownFrameSize2ISFscale(Ipp32s frameSize)
{
   Ipp32s isfScale;
   switch (frameSize) {
      case L_FRAME16k:   isfScale = 0; break; /* WB modes & (10..13)*/
      case  2560: isfScale = 48;   break;
      case  2275: isfScale = 54;   break;
      case  2048: isfScale = 60;   break;
      case  1920: isfScale = 64;   break;
      case  1707: isfScale = 72;   break;
      case  1536: isfScale = 80;   break;
      case  1365: isfScale = 90;   break;
      case  L_FRAME16kPLUS: isfScale = 96;   break; /* 1.0 */
      case 1137: isfScale = 108;   break;
      case 1024: isfScale = 120;   break;
      case 960: isfScale =  128;   break;
      case 910: isfScale =  135;   break;
      case 853: isfScale =  144;   break;
      default :
         /* Unknown internal sampling frequency! */
         return -1;
   }
   return isfScale;
}

static Ipp32s ownISFscale2FrameSize(Ipp32s isfScale)
{
   Ipp32s frameSize;
   switch (isfScale) {
      case   0: frameSize = L_FRAME16kPLUS;   break;/* frameWB*4 */
      case  48: frameSize = 2560;   break;
      case  54: frameSize = 2275;   break;
      case  60: frameSize = 2048;   break;
      case  64: frameSize = 1920;   break;
      case  72: frameSize = 1707;   break;
      case  80: frameSize = 1536;   break;
      case  90: frameSize = 1365;   break;
      case  96: frameSize = L_FRAME16kPLUS;   break;
      case 108: frameSize = 1137;   break;
      case 120: frameSize = 1024;   break;
      case 128: frameSize =  960;   break;
      case 135: frameSize =  910;   break;
      case 144: frameSize =  853;   break;
      default :
         /* Unknown internal sampling frequency! */
         return -1;
   }
   return frameSize;
}

static Ipp32s ownCheckBitrate2PCM (USC_Handle pHandle)
{
   HandleHeader_AMRWBE *pHeader = (HandleHeader_AMRWBE*)pHandle;
   Ipp32s idxMode = pHeader->idxMode;
   Ipp32s pcmFreq = pHeader->pcmSampFreq;

   if ( pHeader->direction==USC_ENCODE &&
      (pHeader->nChannels==1 && pHeader->codecMode==USC_OUT_STEREO)) {
         return -1;
   }

   if (0<=idxMode && idxMode<=8) {
      if ( (pHeader->isf!=0) ||
         (pcmFreq!=16000) ||
         (pHeader->nChannels!=1) ||
         (pHeader->codecMode!=USC_OUT_MONO) ) {
            return -1;
      }
   } else if (10<=idxMode && idxMode<=13) {
      if ( (pHeader->isf!=0) ||
         (pcmFreq!=8000 && pcmFreq!=16000 && pcmFreq!=24000) ||
         ( pHeader->codecMode!=USC_OUT_MONO && (idxMode==10 || idxMode==12)) ) {
            return -1;
      }
   } else if (16<=idxMode && idxMode<=23) {
      if ( (pHeader->isf<48 || 144<pHeader->isf) ||
         (pHeader->codecMode!=USC_OUT_MONO) ) {
            return -1;
      }
   } else if (24<=idxMode && idxMode<=47) {
      if ( (pHeader->isf<48 || 144<pHeader->isf) ||
         (pHeader->nChannels==1) ) {
            return -1;
      }
   } else { /*unknown mode*/
      return -1;
   }
   return 0;
}

