/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2012 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// Purpose: ANS tones family detector: USC functions.
//
*/
#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <math.h>
#include <stdio.h>
#include "usc_td.h"
#include <ipps.h>
#include <ippsc.h>

static USC_Status GetInfoSize(int *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_TD_Info *pInfo);
static USC_Status NumAlloc(const USC_TD_Option *options, int *nbanks);
static USC_Status MemAlloc(const USC_TD_Option *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_TD_Option *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_TD_Modes *modes, USC_Handle handle );
static USC_Status Control(const USC_TD_Modes *modes, USC_Handle handle );

static USC_Status DetectTone(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID);
static USC_Status GenerateTone(USC_Handle handle, USC_ToneID ToneID, int volume, int durationMS, USC_PCMStream *pOut);

#define ANS_DETECTOR_FRAME_SIZE_SAMPLES  205
#define ANS_DETECTOR_FRAME_SIZE          (ANS_DETECTOR_FRAME_SIZE_SAMPLES*sizeof(short))
#define ANS_NUM_TONE_IDS                   1

#define TONE_LEN 7200

typedef struct {
   int toneLen;
   float tmpVec[TONE_LEN+ANS_DETECTOR_FRAME_SIZE_SAMPLES];
   Ipp32fc tmpVecH[TONE_LEN+ANS_DETECTOR_FRAME_SIZE_SAMPLES];
   int reserved3;
   int reserved4;
} ANS_Handle_Header;

/* global usc vector table */
USCFUN USC_TD_Fxns USC_ANSam_Fxns=
{
    {
        USC_TD,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    DetectTone,
    GenerateTone
};

static const USC_ToneID pToneIdsTbl_ANS[ANS_NUM_TONE_IDS]={
    USC_ANSam
};

static USC_Status GetInfoSize(int *pSize)
{
   USC_CHECK_PTR(pSize);

   *pSize = sizeof(USC_TD_Info) + sizeof(USC_TD_Option);
   return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_TD_Info *pInfo)
{
   USC_CHECK_PTR(pInfo);
   pInfo->name = "IPP_TDANSam";

   pInfo->nOptions = 1;
   pInfo->params[0].framesize = ANS_DETECTOR_FRAME_SIZE;
   pInfo->params[0].minframesize = ANS_DETECTOR_FRAME_SIZE;
   pInfo->params[0].maxframesize = ANS_DETECTOR_FRAME_SIZE;
   pInfo->params[0].pcmType.bitPerSample = 16;
   pInfo->params[0].pcmType.sample_frequency = 8000;
   pInfo->params[0].modes.reserved1 = 0;
   pInfo->params[0].modes.reserved2 = 0;
   pInfo->params[0].modes.reserved3 = 0;
   pInfo->params[0].modes.reserved4 = 0;

   pInfo->nToneIDs = ANS_NUM_TONE_IDS;
   pInfo->pToneIDsTbl = (const USC_ToneID*)pToneIdsTbl_ANS;

   return USC_NoError;
}

static IppPCMFrequency USC2IPP_Freq(int frequency)
{
   if(frequency==8000) return IPP_PCM_FREQ_8000;
   return IPP_PCM_FREQ_16000;
}
static USC_Status NumAlloc(const USC_TD_Option *options, int *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);
   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_TD_Option *options, USC_MemBank *pBanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   pBanks->nbytes = sizeof(ANS_Handle_Header);
   return USC_NoError;
}

static USC_Status Init(const USC_TD_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   ANS_Handle_Header *ANS_header;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0,USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   *handle = (USC_Handle*)pBanks->pMem;
   ANS_header = (ANS_Handle_Header*)*handle;

   ANS_header->toneLen = 0;
   ANS_header->reserved4 = 0;

   return USC_NoError;
}

static USC_Status Reinit(const USC_TD_Modes *modes, USC_Handle handle )
{
   ANS_Handle_Header *ANS_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   ANS_header = (ANS_Handle_Header*)handle;

   ANS_header->toneLen = 0;
   ANS_header->reserved4 = 0;

   return USC_NoError;
}

static USC_Status Control(const USC_TD_Modes *modes, USC_Handle handle )
{
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   return USC_NoError;
}

#define ANS_TONE_FREQ 2100
#define ANS_AM_TONE_FREQ 15
#define TONE_LEVEL 80000000000.f

static USC_Status DetectTone(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID)
{
   ANS_Handle_Header *ANS_header;
   Ipp32fc gVal;
   Ipp32f rFreq,sig[ANS_DETECTOR_FRAME_SIZE_SAMPLES],res;

   USC_CHECK_PTR(pIn);
   USC_CHECK_PTR(pIn->pBuffer);
   USC_CHECK_PTR(pDetectedToneID);
   USC_CHECK_HANDLE(handle);

   if(pIn->nbytes <= 0) {
      pIn->nbytes = 0;
      *pDetectedToneID = USC_NoTone;
      return USC_NoError;
   }

   ANS_header = (ANS_Handle_Header*)handle;

   ippsConvert_16s32f((Ipp16s*)pIn->pBuffer,sig,ANS_DETECTOR_FRAME_SIZE_SAMPLES);

   rFreq = (Ipp32f)((0.5 + (Ipp32f)(ANS_TONE_FREQ*ANS_DETECTOR_FRAME_SIZE_SAMPLES/8000.))/ANS_DETECTOR_FRAME_SIZE_SAMPLES);

   ippsGoertz_32f(sig, ANS_DETECTOR_FRAME_SIZE_SAMPLES, &gVal, rFreq);

   res = gVal.im*gVal.im + gVal.re*gVal.re;

   if(res > TONE_LEVEL) {
      ippsCopy_32f(sig,&ANS_header->tmpVec[ANS_header->toneLen],ANS_DETECTOR_FRAME_SIZE_SAMPLES);
      ANS_header->toneLen += ANS_DETECTOR_FRAME_SIZE_SAMPLES;
   } else {
      ANS_header->toneLen = 0;
   }

   if(ANS_header->toneLen >= TONE_LEN) {
      Ipp32f m;
      IppsHilbertSpec_32f32fc *pHilbSpec;
      ippsHilbertInitAlloc_32f32fc(&pHilbSpec,ANS_header->toneLen, ippAlgHintAccurate);

      ippsHilbert_32f32fc(ANS_header->tmpVec, ANS_header->tmpVecH, pHilbSpec);

      ippsMagnitude_32fc(ANS_header->tmpVecH,ANS_header->tmpVec,ANS_header->toneLen);

      ippsMean_32f(ANS_header->tmpVec,ANS_header->toneLen,&m,ippAlgHintAccurate);

      ippsSubC_32f(ANS_header->tmpVec, m, ANS_header->tmpVec,ANS_header->toneLen);

      rFreq = (Ipp32f)((0.5 + (Ipp32f)(ANS_AM_TONE_FREQ*ANS_header->toneLen/8000.))/ANS_header->toneLen);

      ippsGoertz_32f(ANS_header->tmpVec, ANS_header->toneLen, &gVal, rFreq);

      res = (Ipp32f)sqrt(gVal.im*gVal.im + gVal.re*gVal.re);

      ippsHilbertFree_32f32fc(pHilbSpec);

      if(res > 3000000.f) {
         *pDetectedToneID = USC_ANSam;
      } else {
         *pDetectedToneID = USC_ANS;
      }
      ANS_header->toneLen = 0;
   } else {
      *pDetectedToneID = USC_NoTone;
   }

   return USC_NoError;
}

#define Cnvrt_64f16s(src, dst, fScale) \
   src = src * fScale;\
   if(src < 0) {\
      src = src - 0.5f;\
   } else {\
      src = src + 0.5f;\
   }\
   dst = (short)src


static USC_Status GenerateTone(USC_Handle handle, USC_ToneID ToneID, int volume, int durationMS, USC_PCMStream *pOut)
{
   short *ansTone;
   int i;
   double dToneSample;
   int numSamples;
   double dSampleFrequensy;
   double Amp;

   USC_CHECK_PTR(pOut);
   USC_CHECK_HANDLE(handle);
   USC_BADARG((volume < 1) || (volume > 63),USC_BadArgument);
   USC_BADARG(durationMS <= 0,USC_BadArgument);

   ansTone = (short *)pOut->pBuffer;

   numSamples = (8000 / 1000) * durationMS;
   dSampleFrequensy = 8000.;

   Amp = (double)IPP_MAX_16S / (pow(10,(double)volume/10.));

   if(ToneID==USC_ANSam) {
      for(i=0;i<numSamples;i++) {
         dToneSample = (1-0.2*sin(2*IPP_PI*i*(double)ANS_AM_TONE_FREQ/dSampleFrequensy)) *
                      sin(2*IPP_PI*(double)ANS_TONE_FREQ*i/dSampleFrequensy);

         Cnvrt_64f16s(dToneSample, ansTone[i], Amp);
      }
      pOut->nbytes = numSamples*sizeof(short);
      return USC_NoError;
   } else if(ToneID==USC_ANS) {
      for(i=0;i<numSamples;i++) {
         dToneSample = sin(2*IPP_PI*(double)ANS_TONE_FREQ*i/dSampleFrequensy);

         Cnvrt_64f16s(dToneSample, ansTone[i], Amp);
      }
      pOut->nbytes = numSamples*sizeof(short);
      return USC_NoError;
   }
   return USC_NoOperation;
}
