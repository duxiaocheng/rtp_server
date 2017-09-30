/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: USC VAD G723 functions.
//
*/

#include <string.h>
#include "usc_filter.h"
#include "owng723.h"
#include "g723api.h"
#include "vadg723.h"

#define BITSTREAM_SIZE            24
#define G723_SPEECH_FRAME        240
#define G723_BITS_PER_SAMPLE      16
#define G723_UNTR_FRAMESIZE        1
#define G723_NUM_RATES             2
#define G723_SAMPLE_FREQUENCE   8000
#define G723_NCHANNELS             1
#define VAD_G723_LOCAL_MEMORY_SIZE (1536+40)

static USC_Status GetInfoSize_VAD(Ipp32s *pSize);
static USC_Status GetInfo_VAD_G723(USC_Handle handle, USC_FilterInfo *pInfo);
static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks);
static USC_Status MemAlloc_VAD_G723(const USC_FilterOption *options, USC_MemBank *pBanks);
static USC_Status Init_VAD_G723(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit_VAD_G723(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status Control_VAD_G723(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine);
static USC_Status VAD_G723(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision);

typedef struct {
    Ipp32s vad;
    Ipp32s hpf;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
} G723_Handle_Header;

USCFUN USC_Filter_Fxns USC_G723_VAD_Fxns=
{
    {
        USC_FilterVAD,
        (GetInfoSize_func) GetInfoSize_VAD,
        (GetInfo_func)     GetInfo_VAD_G723,
        (NumAlloc_func)    NumAlloc_VAD,
        (MemAlloc_func)    MemAlloc_VAD_G723,
        (Init_func)        Init_VAD_G723,
        (Reinit_func)      Reinit_VAD_G723,
        (Control_func)     Control_VAD_G723
    },
    SetDlyLine,
    VAD_G723
};

static Ipp32s vadObjSize(void){
   Ipp32s fltSize;
   Ipp32s objSize = sizeof(G723VADmemory);
   VoiceActivityDetectSize_G723(&fltSize);
   objSize += fltSize; /* VAD decision memory size*/
   return objSize;
}

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G723[1]={
   {G723_SAMPLE_FREQUENCE,G723_BITS_PER_SAMPLE}
};

static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine)
{
   if(pDlyLine==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   return USC_NoOperation;
}

int G723VAD(G723VADmemory* vadObj,const Ipp16s* src, Ipp8s* pDstBitStream )
{
   Ipp32s  i;

   LOCAL_ALIGN_ARRAY(16, Ipp16s, HPFltSignal,G723_FRM_LEN, vadObj);
   LOCAL_ALIGN_ARRAY(16, Ipp16s, AlignTmpVec,G723_MAX_PITCH+G723_FRM_LEN, vadObj);

   ParamStream_G723 CurrentParams;
   Ipp16s isNotSineWave;

   CurrentParams.FrameType = G723_ActiveFrm;

   if(NULL==vadObj || NULL==src || NULL ==pDstBitStream)
      return USC_BadArgument;
   if(vadObj->objPrm.objSize <= 0)
      return USC_NotInitialized;
   if(G723_ENC_KEY != vadObj->objPrm.key)
      return USC_UnsupportedVADType;

   if ( vadObj->objPrm.mode&G723Encode_HF_Enabled ) {
      /*    High-pass filtering.   Section 2.3 */
      ippsHighPassFilter_G723_16s(src,HPFltSignal,vadObj->HPFltMem);
   } else {
      ippsRShiftC_16s(src,1,HPFltSignal,G723_FRM_LEN);
   }

   UpdateSineDetector(&vadObj->SineWaveDetector, &isNotSineWave);

   /* Compute the Vad */
   CurrentParams.FrameType = G723_ActiveFrm;
   if( vadObj->objPrm.mode&G723Encode_VAD_Enabled){
      VoiceActivityDetect_G723(HPFltSignal, (Ipp16s*)&vadObj->prevSidLpc,(Ipp16s*)&vadObj->PrevOpenLoopLags,
         isNotSineWave,&i,&vadObj->AdaptEnableFlag,(Ipp8s *)vadObj,AlignTmpVec);
      CurrentParams.FrameType = (G723_FrameType)i;
   }

   if(CurrentParams.FrameType != G723_ActiveFrm) {
      vadObj->PastFrameType = CurrentParams.FrameType;
   }

   else {

      /* Active frame */
      vadObj->PastFrameType = G723_ActiveFrm;

   }

   CLEAR_SCRATCH_MEMORY(vadObj);

   return USC_NoError;

}

static USC_Status GetInfoSize_VAD(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_FilterInfo);
   return USC_NoError;
}

static USC_Status GetInfo_VAD_G723(USC_Handle handle, USC_FilterInfo *pInfo)
{
   G723_Handle_Header *g723_header;

   USC_CHECK_PTR(pInfo);

   pInfo->name = "IPP_VADG723";
   pInfo->nOptions = 1;
   pInfo->params[0].maxframesize = G723_SPEECH_FRAME*sizeof(Ipp16s);
   pInfo->params[0].minframesize = G723_SPEECH_FRAME*sizeof(Ipp16s);
   pInfo->params[0].framesize = G723_SPEECH_FRAME*sizeof(Ipp16s);

   if (handle == NULL) {
      pInfo->params[0].modes.vad = 1;
   } else {
      g723_header = (G723_Handle_Header*)handle;
      pInfo->params[0].modes.vad = g723_header->vad;
   }
   pInfo->params[0].pcmType.sample_frequency = pcmTypesTbl_G723[0].sample_frequency;
   pInfo->params[0].pcmType.bitPerSample = pcmTypesTbl_G723[0].bitPerSample;
   pInfo->params[0].pcmType.nChannels = 1;

   return USC_NoError;
}

static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 2;

   return USC_NoError;
}

static USC_Status MemAlloc_VAD_G723(const USC_FilterOption *options, USC_MemBank *pBanks)
{
    Ipp32s nbytes;
    Ipp32s scratchMemSize = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    nbytes =  vadObjSize();
    pBanks[0].nbytes = nbytes+sizeof(G723_Handle_Header);

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    scratchMemSize = VAD_G723_LOCAL_MEMORY_SIZE;

    pBanks[1].nbytes = scratchMemSize;

    return USC_NoError;
}

static USC_Status Init_VAD_G723(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   Ipp32s mode=0;
   G723_Handle_Header *g723_header;
   G723VADmemory *VadObj;
   Ipp8s* oldMemBuff;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_HANDLE(handle);
   USC_CHECK_PTR(pBanks[0].pMem);
   USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
   USC_CHECK_PTR(pBanks[1].pMem);
   USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);
   USC_BADARG(options->pcmType.sample_frequency < 1, USC_UnsupportedPCMType);
   USC_BADARG(options->pcmType.bitPerSample < 1, USC_UnsupportedPCMType);

   *handle = (USC_Handle*)pBanks[0].pMem;
   g723_header = (G723_Handle_Header*)*handle;

//   g723_header->hpf = options->modes.hpf;
   g723_header->vad = options->modes.vad;

   VadObj = (G723VADmemory *)((Ipp8s*)*handle + sizeof(G723_Handle_Header));
   if(NULL==VadObj) return USC_BadArgument;
   if(NULL==pBanks[1].pMem) return USC_BadArgument;

   if(pBanks[1].pMem)   VadObj->Mem.base = pBanks[1].pMem; // otherwise reinit
   else if (VadObj->Mem.base == NULL) return USC_NotInitialized;
   VadObj->Mem.CurPtr = VadObj->Mem.base;
   VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G723_LOCAL_MEMORY_SIZE);

//   if(options->modes.hpf) mode |= 2;
   if(options->modes.vad) mode |= 1;

   if(NULL==VadObj)
      return USC_BadArgument;

   oldMemBuff = VadObj->Mem.base; /* if Reinit */

   ippsZero_16s((Ipp16s*)VadObj,sizeof(G723VADmemory)>>1) ;

   VadObj->objPrm.objSize = vadObjSize();
   VadObj->objPrm.mode = mode;
   VadObj->objPrm.key = G723_ENC_KEY;

   /* Initialize the VAD */
   //if(VadObj->objPrm.mode & G723Encode_VAD_Enabled){
   VoiceActivityDetectInit_G723((Ipp8s *)VadObj/*->vadMem*/);
   /* Initialize the CNG */
   VadObj->PrevOpenLoopLags[0] = G723_SBFR_LEN;
   VadObj->PrevOpenLoopLags[1] = G723_SBFR_LEN;

   ippsZero_16s(VadObj->prevSidLpc,G723_LPC_ORDERP1);
   VadObj->prevSidLpc[0] = 0x2000;

   VadObj->PastFrameType = G723_ActiveFrm;
   //}

   if(NULL==VadObj) return USC_BadArgument;
   if(NULL==oldMemBuff) return USC_BadArgument;

   if(oldMemBuff)   VadObj->Mem.base = oldMemBuff; // otherwise reinit
   else if (VadObj->Mem.base == NULL) return USC_NotInitialized;
   VadObj->Mem.CurPtr = VadObj->Mem.base;
   VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G723_LOCAL_MEMORY_SIZE);

   return USC_NoError;
}

static USC_Status Reinit_VAD_G723(const USC_FilterModes *modes, USC_Handle handle )
{
   Ipp32s mode=0;
   G723_Handle_Header *g723_header;
   G723VADmemory *VadObj;
   Ipp8s* oldMemBuff;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g723_header = (G723_Handle_Header*)handle;

//   g723_header->hpf = modes->hpf;
   g723_header->vad = modes->vad;

   VadObj = (G723VADmemory *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
//   if(modes->hpf) mode |= 2;
   if(modes->vad) mode |= 1;

   if(NULL==VadObj)
      return USC_BadArgument;

   oldMemBuff = VadObj->Mem.base; /* if Reinit */

   ippsZero_16s((Ipp16s*)VadObj,sizeof(G723VADmemory)>>1) ;

   VadObj->objPrm.objSize = vadObjSize();
   VadObj->objPrm.mode = mode;
   VadObj->objPrm.key = G723_ENC_KEY;

   /* Initialize the VAD */
   //if(VadObj->objPrm.mode & G723Encode_VAD_Enabled){
   VoiceActivityDetectInit_G723((Ipp8s *)VadObj/*->vadMem*/);
   /* Initialize the CNG */
   VadObj->PrevOpenLoopLags[0] = G723_SBFR_LEN;
   VadObj->PrevOpenLoopLags[1] = G723_SBFR_LEN;

   ippsZero_16s(VadObj->prevSidLpc,G723_LPC_ORDERP1);
   VadObj->prevSidLpc[0] = 0x2000;

   VadObj->PastFrameType = G723_ActiveFrm;
   //}

   if(NULL==VadObj) return USC_BadArgument;
   if(NULL==oldMemBuff) return USC_BadArgument;

   if(oldMemBuff)   VadObj->Mem.base = oldMemBuff; // otherwise reinit
   else if (VadObj->Mem.base == NULL) return USC_NotInitialized;
   VadObj->Mem.CurPtr = VadObj->Mem.base;
   VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G723_LOCAL_MEMORY_SIZE);
   return USC_NoError;
}

static USC_Status Control_VAD_G723(const USC_FilterModes *modes, USC_Handle handle)
{
   Ipp32s mode=0;
   G723_Handle_Header *g723_header;
   G723VADmemory *VadObj;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g723_header = (G723_Handle_Header*)handle;

//   g723_header->hpf = modes->hpf;
   g723_header->vad = modes->vad;

   VadObj = (G723VADmemory *)((Ipp8s*)handle + sizeof(G723_Handle_Header));
//   if(modes->hpf) mode |= 2;
   if(modes->vad) mode |= 1;
   VadObj->objPrm.mode = mode;
   return USC_NoError;
}

static USC_Status VAD_G723(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision)
{
    G723VADmemory *VadObj;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_PTR(pVADDecision);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(in->pcmType.bitPerSample!=G723_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G723_SAMPLE_FREQUENCE, USC_UnsupportedPCMType);
    USC_BADARG(in->nbytes < G723_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);

    VadObj = (G723VADmemory *)((Ipp8s*)handle + sizeof(G723_Handle_Header));

    if(G723VAD(VadObj,(const Ipp16s*)in->pBuffer, out->pBuffer) != USC_NoError){
       return USC_NoOperation;
    }

    if (VadObj->PastFrameType == G723_UntransmittedFrm){
       *pVADDecision = NODECISION;
       ippsZero_8u((Ipp8u*)out->pBuffer, G723_SPEECH_FRAME*sizeof(Ipp16s));
    } else if(VadObj->PastFrameType == G723_ActiveFrm) {
       *pVADDecision = ACTIVE;
       ippsCopy_8u((Ipp8u*)in->pBuffer,(Ipp8u*)out->pBuffer, G723_SPEECH_FRAME*sizeof(Ipp16s));
    } else if(VadObj->PastFrameType == G723_SIDFrm){
       *pVADDecision = INACTIVE;
       ippsZero_8u((Ipp8u*)out->pBuffer, G723_SPEECH_FRAME*sizeof(Ipp16s));
    }

    out->bitrate = in->bitrate;

    in->nbytes = G723_SPEECH_FRAME*sizeof(Ipp16s);
    out->nbytes = G723_SPEECH_FRAME*sizeof(Ipp16s);

    return USC_NoError;
}
