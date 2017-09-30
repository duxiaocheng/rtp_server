/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2007-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: MS RTaudio Noise reduction: USC functions.
//
*/
#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "usc_filter.h"

static USC_Status GetInfoSize(Ipp32s *pSize);
static USC_Status GetInfo(USC_Handle handle, USC_FilterInfo *pInfo);
static USC_Status NumAlloc(const USC_FilterOption *options, Ipp32s *nbanks);
static USC_Status MemAlloc(const USC_FilterOption *options, USC_MemBank *pBanks);
static USC_Status Init(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status Control(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine);
static USC_Status Filter(USC_Handle handle, USC_PCMStream *pIn, USC_PCMStream *pOut, USC_FrameType *pDecision);

#define MSRTA_FRAME_SIZE_SAMPLES  160
#define MSRTA_FRAME_SIZE          (MSRTA_FRAME_SIZE_SAMPLES*sizeof(Ipp16s))

typedef struct {
    Ipp32s sampleFrequence;
    Ipp32s reserved1;
    Ipp32s reserved2;
    Ipp32s reserved3;
} ECNR_Handle_Header;

/* global usc vector table */
USCFUN USC_Filter_Fxns USC_RTANR_FP_Fxns=
{
    {
        USC_Filter,
        (GetInfoSize_func) GetInfoSize,
        (GetInfo_func)     GetInfo,
        (NumAlloc_func)    NumAlloc,
        (MemAlloc_func)    MemAlloc,
        (Init_func)        Init,
        (Reinit_func)      Reinit,
        (Control_func)     Control
    },
    SetDlyLine,
    Filter
};

static USC_Status GetInfoSize(Ipp32s *pSize)
{
    *pSize = sizeof(USC_FilterInfo) + 3*sizeof(USC_FilterOption);
    return USC_NoError;
}

static IppPCMFrequency Freq2IPPFreq(int frequency)
{
   if(frequency==8000) return IPP_PCM_FREQ_8000;
   else if(frequency==16000) return IPP_PCM_FREQ_16000;
   else if(frequency==22050) return IPP_PCM_FREQ_22050;
   else if(frequency==32000) return IPP_PCM_FREQ_32000;

   return IPP_PCM_FREQ_11025;
}
static USC_Status GetInfo(USC_Handle handle, USC_FilterInfo *pInfo)
{
    pInfo->name = "IPP_NR_RTA_FP";

    pInfo->nOptions = 4;
    pInfo->params[0].maxframesize = MSRTA_FRAME_SIZE;
    pInfo->params[0].minframesize = MSRTA_FRAME_SIZE;
    pInfo->params[0].framesize = MSRTA_FRAME_SIZE;
    pInfo->params[0].pcmType.bitPerSample = 16;
    pInfo->params[0].pcmType.sample_frequency = 8000;
    pInfo->params[0].pcmType.nChannels = 1;
    pInfo->params[0].modes.vad = 0;
    pInfo->params[0].modes.reserved2 = 0;
    pInfo->params[0].modes.reserved3 = 0;
    pInfo->params[0].modes.reserved4 = 0;

    pInfo->params[1].maxframesize = MSRTA_FRAME_SIZE;
    pInfo->params[1].minframesize = MSRTA_FRAME_SIZE;
    pInfo->params[1].framesize = MSRTA_FRAME_SIZE;
    pInfo->params[1].pcmType.bitPerSample = 16;
    pInfo->params[1].pcmType.sample_frequency = 16000;
    pInfo->params[1].pcmType.nChannels = 1;
    pInfo->params[1].modes.vad = 0;
    pInfo->params[1].modes.reserved2 = 0;
    pInfo->params[1].modes.reserved3 = 0;
    pInfo->params[1].modes.reserved4 = 0;

    pInfo->params[2].maxframesize = MSRTA_FRAME_SIZE;
    pInfo->params[2].minframesize = MSRTA_FRAME_SIZE;
    pInfo->params[2].framesize = MSRTA_FRAME_SIZE;
    pInfo->params[2].pcmType.bitPerSample = 16;
    pInfo->params[2].pcmType.sample_frequency = 22050;
    pInfo->params[2].pcmType.nChannels = 1;
    pInfo->params[2].modes.vad = 0;
    pInfo->params[2].modes.reserved2 = 0;
    pInfo->params[2].modes.reserved3 = 0;
    pInfo->params[2].modes.reserved4 = 0;

    pInfo->params[3].maxframesize = MSRTA_FRAME_SIZE;
    pInfo->params[3].minframesize = MSRTA_FRAME_SIZE;
    pInfo->params[3].framesize = MSRTA_FRAME_SIZE;
    pInfo->params[3].pcmType.bitPerSample = 16;
    pInfo->params[3].pcmType.sample_frequency = 32000;
    pInfo->params[3].pcmType.nChannels = 1;
    pInfo->params[3].modes.vad = 0;
    pInfo->params[3].modes.reserved2 = 0;
    pInfo->params[3].modes.reserved3 = 0;
    pInfo->params[3].modes.reserved4 = 0;

    return USC_NoError;
}

static USC_Status NumAlloc(const USC_FilterOption *options, Ipp32s *nbanks)
{
   if(options==NULL) return USC_BadDataPointer;
   if(nbanks==NULL) return USC_BadDataPointer;
   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc(const USC_FilterOption *options, USC_MemBank *pBanks)
{
   Ipp32s NRAlgSize;

   if(options==NULL) return USC_BadDataPointer;
   if(pBanks==NULL) return USC_BadDataPointer;

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   ippsFilterNoiseGetStateSize_RTA_32f(Freq2IPPFreq(options->pcmType.sample_frequency), &NRAlgSize);

   pBanks->nbytes = NRAlgSize + sizeof(ECNR_Handle_Header);
   return USC_NoError;
}

static USC_Status Init(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   ECNR_Handle_Header *nr_header;
   IppsFilterNoiseState_RTA_32f* pNRState;

   if(options==NULL) return USC_BadDataPointer;
   if(pBanks==NULL) return USC_BadDataPointer;
   if(pBanks->pMem==NULL) return USC_NotInitialized;
   if(pBanks->nbytes<=0) return USC_NotInitialized;
   if(handle==NULL) return USC_InvalidHandler;

   *handle = (USC_Handle*)pBanks->pMem;
   nr_header = (ECNR_Handle_Header*)*handle;

   /* Init NR */
   pNRState = (IppsFilterNoiseState_RTA_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsFilterNoiseInit_RTA_32f(Freq2IPPFreq(options->pcmType.sample_frequency), pNRState);

   nr_header->sampleFrequence = options->pcmType.sample_frequency;
   nr_header->reserved1 = 0;
   nr_header->reserved2 = 0;
   nr_header->reserved3 = 0;
   return USC_NoError;
}

static USC_Status Reinit(const USC_FilterModes *modes, USC_Handle handle )
{
   ECNR_Handle_Header *nr_header;
   IppsFilterNoiseState_RTA_32f* pNRState;

   if(modes==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   nr_header = (ECNR_Handle_Header*)handle;

   pNRState = (IppsFilterNoiseState_RTA_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsFilterNoiseInit_RTA_32f(Freq2IPPFreq(nr_header->sampleFrequence), pNRState);

   return USC_NoError;
}

static USC_Status Control(const USC_FilterModes *modes, USC_Handle handle )
{
   if(modes==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;
   return USC_NoError;
}

static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine)
{
   if(pDlyLine==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   return USC_NoOperation;
}
static USC_Status Filter(USC_Handle handle, USC_PCMStream *pIn, USC_PCMStream *pOut, USC_FrameType *pDecision)
{
   ECNR_Handle_Header *nr_header;
   IppsFilterNoiseState_RTA_32f* pNRState;
   Ipp32f cnvrtSrc[MSRTA_FRAME_SIZE_SAMPLES];
   Ipp32f cnvrtDst[MSRTA_FRAME_SIZE_SAMPLES];

   if(pIn==NULL) return USC_BadDataPointer;
   if(pOut==NULL) return USC_BadDataPointer;
   if(pIn->pBuffer==NULL) return USC_BadDataPointer;
   if(pOut->pBuffer==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   if(pIn->nbytes < MSRTA_FRAME_SIZE) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }

   if(pOut->nbytes < MSRTA_FRAME_SIZE) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }

   nr_header = (ECNR_Handle_Header*)handle;
   pNRState = (IppsFilterNoiseState_RTA_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsConvert_16s32f((Ipp16s*)pIn->pBuffer,cnvrtSrc,MSRTA_FRAME_SIZE_SAMPLES);
   ippsFilterNoise_RTA_32f(cnvrtSrc, cnvrtDst, pNRState);
   ippsConvert_32f16s_Sfs(cnvrtDst,(Ipp16s*)pOut->pBuffer,MSRTA_FRAME_SIZE_SAMPLES,ippRndNear,0);

   pIn->nbytes = MSRTA_FRAME_SIZE;
   pOut->nbytes = MSRTA_FRAME_SIZE;

   if(pDecision) {
      *pDecision = ACTIVE;
   }


   return USC_NoError;
}
