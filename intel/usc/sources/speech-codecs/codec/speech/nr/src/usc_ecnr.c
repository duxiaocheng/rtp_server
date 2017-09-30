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
// Purpose: Echo Canceller Noise reduction: USC functions.
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

#define ECNR_FRAME_SIZE          160

typedef struct {
    Ipp32s sampleFrequence;
    Ipp32s reserved1;
    Ipp32s reserved2;
    Ipp32s reserved3;
} ECNR_Handle_Header;

/* global usc vector table */
USCFUN USC_Filter_Fxns USC_ECNR_Fxns=
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
unsigned int usc_nr_seed;
Ipp64f usc_nr_apwr;
Ipp32f usc_nr_spwr;
static USC_Status GetInfoSize(Ipp32s *pSize)
{
    *pSize = sizeof(USC_FilterInfo) + sizeof(USC_FilterOption);
    return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_FilterInfo *pInfo)
{
    pInfo->name = "IPP_NR_EC";

    pInfo->nOptions = 2;
    pInfo->params[0].maxframesize = ECNR_FRAME_SIZE;
    pInfo->params[0].minframesize = ECNR_FRAME_SIZE;
    pInfo->params[0].framesize = ECNR_FRAME_SIZE;
    pInfo->params[0].pcmType.bitPerSample = 16;
    pInfo->params[0].pcmType.sample_frequency = 8000;
    pInfo->params[0].pcmType.nChannels = 1;
    pInfo->params[0].modes.vad = 0;
    pInfo->params[0].modes.reserved2 = 0;
    pInfo->params[0].modes.reserved3 = 0;
    pInfo->params[0].modes.reserved4 = 0;

    pInfo->params[1].maxframesize = ECNR_FRAME_SIZE;
    pInfo->params[1].minframesize = ECNR_FRAME_SIZE;
    pInfo->params[1].framesize = ECNR_FRAME_SIZE;
    pInfo->params[1].pcmType.bitPerSample = 16;
    pInfo->params[1].pcmType.sample_frequency = 16000;
    pInfo->params[1].pcmType.nChannels = 1;
    pInfo->params[1].modes.vad = 0;
    pInfo->params[1].modes.reserved2 = 0;
    pInfo->params[1].modes.reserved3 = 0;
    pInfo->params[1].modes.reserved4 = 0;

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

   ippsFilterNoiseGetStateSize_EC_32f(options->pcmType.sample_frequency,&NRAlgSize);

   pBanks->nbytes = NRAlgSize + sizeof(ECNR_Handle_Header);
   return USC_NoError;
}

static USC_Status Init(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   ECNR_Handle_Header *nr_header;
   IppsFilterNoiseState_EC_32f* pNRState;

   if(options==NULL) return USC_BadDataPointer;
   if(pBanks==NULL) return USC_BadDataPointer;
   if(pBanks->pMem==NULL) return USC_NotInitialized;
   if(pBanks->nbytes<=0) return USC_NotInitialized;
   if(handle==NULL) return USC_InvalidHandler;

   *handle = (USC_Handle*)pBanks->pMem;
   nr_header = (ECNR_Handle_Header*)*handle;

   /* Init NR */
   pNRState = (IppsFilterNoiseState_EC_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsFilterNoiseInit_EC_32f(options->pcmType.sample_frequency,pNRState);
   ippsFilterNoiseLevel_EC_32f(ippsNrHigh, pNRState);
   ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, pNRState);

   nr_header->sampleFrequence = options->pcmType.sample_frequency;
   nr_header->reserved1 = 0;
   nr_header->reserved2 = 0;
   usc_nr_seed=1;
   usc_nr_apwr=0;
   usc_nr_spwr=0;
   nr_header->reserved3 = 0;
   return USC_NoError;
}

static USC_Status Reinit(const USC_FilterModes *modes, USC_Handle handle )
{
   ECNR_Handle_Header *nr_header;
   IppsFilterNoiseState_EC_32f* pNRState;

   if(modes==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   nr_header = (ECNR_Handle_Header*)handle;

   pNRState = (IppsFilterNoiseState_EC_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsFilterNoiseInit_EC_32f(nr_header->sampleFrequence,pNRState);
   ippsFilterNoiseLevel_EC_32f(ippsNrHigh, pNRState);
   ippsFilterNoiseSetMode_EC_32f(ippsNrSmoothOff, pNRState);
   usc_nr_seed=1;
   usc_nr_apwr=0;
   usc_nr_spwr=0;

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
   IppsFilterNoiseState_EC_32f* pNRState;
   Ipp64f pwr;
   int    i;
   int    flag;
   Ipp32f wBuf[16];
   Ipp16s *pShort;
   Ipp32f foo;
//   Ipp32f pRnd[16];

   if(pIn==NULL) return USC_BadDataPointer;
   if(pOut==NULL) return USC_BadDataPointer;
   if(pIn->pBuffer==NULL) return USC_BadDataPointer;
   if(pOut->pBuffer==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   if(pIn->nbytes < ECNR_FRAME_SIZE) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }

   if(pOut->nbytes < ECNR_FRAME_SIZE) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }
   nr_header = (ECNR_Handle_Header*)handle;
   pNRState = (IppsFilterNoiseState_EC_32f*)((Ipp8s*)nr_header + sizeof(ECNR_Handle_Header));

   ippsCopy_8u((Ipp8u*)pIn->pBuffer,(Ipp8u*)pOut->pBuffer,ECNR_FRAME_SIZE);

   pShort=(Ipp16s *)pOut->pBuffer;
   for (i=0;i<(ECNR_FRAME_SIZE>>1);i+=16) {
        ippsConvert_16s32f_Sfs(&pShort[i], wBuf, 16, 0);
        ippsFilterNoiseDetect_EC_32f64f(wBuf,&pwr,&foo,&flag,pNRState);
        ippsFilterNoise_EC_32f_I(wBuf,ippsNrNoUpdate,pNRState);
        ippsConvert_32f16s_Sfs(wBuf, &pShort[i],16,ippRndZero, 0);
   }
   pIn->nbytes = ECNR_FRAME_SIZE;
   pOut->nbytes = ECNR_FRAME_SIZE;

   if(pDecision) {
      *pDecision = ACTIVE;
   }


   return USC_NoError;
}
