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
// Purpose: ALC: USC functions.
//
*/
#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <math.h>
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

#define ALC_FRAME_SIZE_SAMPLES  80
#define ALC_FRAME_SIZE          (ALC_FRAME_SIZE_SAMPLES*sizeof(Ipp16s))

typedef struct {
    Ipp32s sampleFrequence;
    Ipp32f reserved1;
    Ipp32s reserved2;
    Ipp32s reserved3;
} ALC_Handle_Header;

/* global usc vector table */
USCFUN USC_Filter_Fxns USC_ALC_Fxns=
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
    *pSize = sizeof(USC_FilterInfo) + sizeof(USC_FilterOption);
    return USC_NoError;
}

static USC_Status GetInfo(USC_Handle handle, USC_FilterInfo *pInfo)
{
    pInfo->name = "IPP_ALC";

    pInfo->nOptions = 2;
    pInfo->params[0].maxframesize = ALC_FRAME_SIZE;
    pInfo->params[0].minframesize = ALC_FRAME_SIZE;
    pInfo->params[0].framesize = ALC_FRAME_SIZE;
    pInfo->params[0].pcmType.bitPerSample = 16;
    pInfo->params[0].pcmType.sample_frequency = 8000;
    pInfo->params[0].pcmType.nChannels = 1;
    pInfo->params[0].modes.vad = 0;
    pInfo->params[0].modes.reserved2 = 0;
    pInfo->params[0].modes.reserved3 = 0;
    pInfo->params[0].modes.reserved4 = 0;

    pInfo->params[1].maxframesize = ALC_FRAME_SIZE*2;
    pInfo->params[1].minframesize = ALC_FRAME_SIZE*2;
    pInfo->params[1].framesize = ALC_FRAME_SIZE*2;
    pInfo->params[1].pcmType.bitPerSample = 16;
    pInfo->params[1].pcmType.sample_frequency = 16000;
    pInfo->params[0].pcmType.nChannels = 1;
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
   Ipp32s ALCAlgSize;

   if(options==NULL) return USC_BadDataPointer;
   if(pBanks==NULL) return USC_BadDataPointer;

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   ippsALCGetStateSize_G169_16s(&ALCAlgSize);

   pBanks->nbytes = ALCAlgSize + sizeof(ALC_Handle_Header);
   return USC_NoError;
}

static USC_Status Init(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   ALC_Handle_Header *alc_header;
   IppsALCState_G169_16s* pALCState;

   if(options==NULL) return USC_BadDataPointer;
   if(pBanks==NULL) return USC_BadDataPointer;
   if(pBanks->pMem==NULL) return USC_NotInitialized;
   if(pBanks->nbytes<=0) return USC_NotInitialized;
   if(handle==NULL) return USC_InvalidHandler;

   *handle = (USC_Handle*)pBanks->pMem;
   alc_header = (ALC_Handle_Header*)*handle;

   /* Init NR */
   pALCState = (IppsALCState_G169_16s*)((Ipp8s*)alc_header + sizeof(ALC_Handle_Header));

   ippsALCInit_G169_16s(pALCState);

   ippsALCSetLevel_G169_16s(6.f, 6.f, pALCState);

   alc_header->sampleFrequence = options->pcmType.sample_frequency;

   alc_header->reserved1 = 0;
   alc_header->reserved2 = 0;
   alc_header->reserved3 = 0;
   return USC_NoError;
}

static USC_Status Reinit(const USC_FilterModes *modes, USC_Handle handle )
{
   ALC_Handle_Header *alc_header;
   IppsALCState_G169_16s* pALCState;

   if(modes==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   alc_header = (ALC_Handle_Header*)handle;

   pALCState = (IppsALCState_G169_16s*)((Ipp8s*)alc_header + sizeof(ALC_Handle_Header));

   ippsALCInit_G169_16s(pALCState);

   ippsALCSetLevel_G169_16s(6.f, 6.f, pALCState);

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
   ALC_Handle_Header *alc_header;
   IppsALCState_G169_16s* pALCState;
   int frmSize = ALC_FRAME_SIZE;

   if(pIn==NULL) return USC_BadDataPointer;
   if(pOut==NULL) return USC_BadDataPointer;
   if(pIn->pBuffer==NULL) return USC_BadDataPointer;
   if(pOut->pBuffer==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   if(pIn->pcmType.sample_frequency == 16000) {
      frmSize = 2*ALC_FRAME_SIZE;
   }

   if(pIn->nbytes < frmSize) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }

   if(pOut->nbytes < frmSize) {
      pIn->nbytes = 0;
      pOut->nbytes = 0;
      pOut->pBuffer=NULL;
      if(pDecision) {
         *pDecision = NODECISION;
      }
      return USC_NoError;
   }

   alc_header = (ALC_Handle_Header*)handle;
   pALCState = (IppsALCState_G169_16s*)((Ipp8s*)alc_header + sizeof(ALC_Handle_Header));

   ippsALC_G169_16s((const Ipp16s*)pIn->pBuffer, (Ipp16s*)pOut->pBuffer, frmSize/2, pALCState);

   pIn->nbytes = frmSize;
   pOut->nbytes = frmSize;

   if(pDecision) {
      *pDecision = ACTIVE;
   }

   return USC_NoError;
}
