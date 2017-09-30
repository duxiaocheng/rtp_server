/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2011 Intel Corporation. All Rights Reserved.
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

#define ANS_DETECTOR_FRAME_SIZE          160
#define ANS_NUM_TONE_IDS                   1

typedef struct {
    int bitPerSample;
    int sample_frequency;
    int reserved3;
    int reserved4;
} ANS_Handle_Header;

/* global usc vector table */
USCFUN USC_TD_Fxns USC_ANSs_Fxns=
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
    USC_slashANS
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
   pInfo->name = "IPP_TDANS";

   if(handle==NULL) {
      pInfo->nOptions = 2;
      pInfo->params[0].framesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].minframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].maxframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].pcmType.bitPerSample = 16;
      pInfo->params[0].pcmType.sample_frequency = 8000;
      pInfo->params[0].modes.reserved1 = 0;
      pInfo->params[0].modes.reserved2 = 0;
      pInfo->params[0].modes.reserved3 = 0;
      pInfo->params[0].modes.reserved4 = 0;

      pInfo->params[1].framesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[1].minframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[1].maxframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[1].pcmType.bitPerSample = 16;
      pInfo->params[1].pcmType.sample_frequency = 16000;
      pInfo->params[1].modes.reserved1 = 0;
      pInfo->params[1].modes.reserved2 = 0;
      pInfo->params[1].modes.reserved3 = 0;
      pInfo->params[1].modes.reserved4 = 0;
   } else {
      ANS_Handle_Header *ANS_header;
      ANS_header = (ANS_Handle_Header*)handle;

      pInfo->nOptions = 1;
      pInfo->params[0].framesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].minframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].maxframesize = ANS_DETECTOR_FRAME_SIZE;
      pInfo->params[0].pcmType.bitPerSample = ANS_header->bitPerSample;
      pInfo->params[0].pcmType.sample_frequency = ANS_header->sample_frequency;
      pInfo->params[0].modes.reserved1 = 0;
      pInfo->params[0].modes.reserved2 = 0;
      pInfo->params[0].modes.reserved3 = 0;
      pInfo->params[0].modes.reserved4 = 0;
   }

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
   int size = 0;
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   ippsToneDetectGetStateSize_EC_16s(USC2IPP_Freq(options->pcmType.sample_frequency), &size);

   pBanks->nbytes = sizeof(ANS_Handle_Header) + size;
   return USC_NoError;
}

static USC_Status Init(const USC_TD_Option *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   ANS_Handle_Header *ANS_header;
   IppsToneDetectState_EC_16s *pIppTDParams;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0,USC_NotInitialized);
   USC_CHECK_HANDLE(handle);

   *handle = (USC_Handle*)pBanks->pMem;
   ANS_header = (ANS_Handle_Header*)*handle;

   pIppTDParams = (IppsToneDetectState_EC_16s *)((char*)*handle + sizeof(ANS_Handle_Header));

   ANS_header->bitPerSample = options->pcmType.bitPerSample;
   ANS_header->sample_frequency = options->pcmType.sample_frequency;
   ANS_header->reserved4 = 0;



   ippsToneDetectInit_EC_16s((IppsToneDetectState_EC_16s *)pIppTDParams, USC2IPP_Freq(ANS_header->sample_frequency));

   return USC_NoError;
}

static USC_Status Reinit(const USC_TD_Modes *modes, USC_Handle handle )
{
   ANS_Handle_Header *ANS_header;
   IppsToneDetectState_EC_16s *pIppTDParams;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   ANS_header = (ANS_Handle_Header*)handle;

   pIppTDParams = (IppsToneDetectState_EC_16s *)((char*)handle + sizeof(ANS_Handle_Header));

   ippsToneDetectInit_EC_16s((IppsToneDetectState_EC_16s *)pIppTDParams, USC2IPP_Freq(ANS_header->sample_frequency));

   ANS_header->reserved4 = 0;

   return USC_NoError;
}

static USC_Status Control(const USC_TD_Modes *modes, USC_Handle handle )
{
   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);

   return USC_NoError;
}

static USC_Status DetectTone(USC_Handle handle, USC_PCMStream *pIn, USC_ToneID *pDetectedToneID)
{
   ANS_Handle_Header *ANS_header;
   int toneRes=0;
   IppsToneDetectState_EC_16s *pIppTDParams;

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

   USC_BADARG(ANS_header->bitPerSample != pIn->pcmType.bitPerSample,USC_UnsupportedPCMType);
   USC_BADARG(ANS_header->sample_frequency != pIn->pcmType.sample_frequency,USC_UnsupportedPCMType);

   pIppTDParams = (IppsToneDetectState_EC_16s *)((char*)handle + sizeof(ANS_Handle_Header));

   ippsToneDetect_EC_16s((const Ipp16s *)pIn->pBuffer, pIn->nbytes>>1, &toneRes, pIppTDParams);

   if(toneRes != 0) {
      *pDetectedToneID = USC_slashANS;
   } else {
      *pDetectedToneID = USC_NoTone;
   }

   return USC_NoError;
}

#define ANS_PHASE_SHIFT_ANGLE IPP_PI
#define ANS_TONE_FREQ 2100
#define ANS_AM_TONE_FREQ 15
#define ANS_PHASE_SHIFT_POSITION (double)0.450

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
   ANS_Handle_Header *ANS_header;
   short *ansTone;
   int i, lModule;
   double dToneSample;
   double dPhaseShiftAngle = ANS_PHASE_SHIFT_ANGLE;
   int numSamples;
   double dSampleFrequency;
   double Amp;

   USC_CHECK_PTR(pOut);
   USC_CHECK_HANDLE(handle);
   USC_BADARG((volume < 1) || (volume > 63),USC_BadArgument);
   USC_BADARG(durationMS <= 0,USC_BadArgument);

   ansTone = (short *)pOut->pBuffer;

   ANS_header = (ANS_Handle_Header*)handle;

   numSamples = (ANS_header->sample_frequency / 1000) * durationMS;
   dSampleFrequency = (double)ANS_header->sample_frequency;

   Amp = (double)IPP_MAX_16S / (pow(10,(double)volume/10.));

   if(ToneID==USC_slashANS) {
      for(i=0;i<numSamples;i++) {
         lModule = (int)(i/(ANS_PHASE_SHIFT_POSITION*dSampleFrequency))%2;
         dToneSample = sin(2*IPP_PI*(double)ANS_TONE_FREQ*i/dSampleFrequency + dPhaseShiftAngle*lModule);
         Cnvrt_64f16s(dToneSample, ansTone[i], Amp);
      }
      pOut->nbytes = numSamples*sizeof(short);
      return USC_NoError;
   } else if(ToneID==USC_slashANSam) {
      for(i=0;i<numSamples;i++) {
         lModule = (int)(i/(ANS_PHASE_SHIFT_POSITION*dSampleFrequency))%2;
         dToneSample = (1-0.2*sin(2*IPP_PI*i*(double)ANS_AM_TONE_FREQ/dSampleFrequency)) *
                      sin(2*IPP_PI*(double)ANS_TONE_FREQ*i/dSampleFrequency + dPhaseShiftAngle*lModule);

         Cnvrt_64f16s(dToneSample, ansTone[i], Amp);
      }
      pOut->nbytes = numSamples*sizeof(short);
      return USC_NoError;
   }
   return USC_NoOperation;
}
