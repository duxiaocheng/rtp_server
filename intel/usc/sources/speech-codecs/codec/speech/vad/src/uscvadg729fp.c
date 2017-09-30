/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2004-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: USC VAD G729 float functions.
//
*/

#include "usc_filter.h"
#include "vadg729fp.h"

#define  G729IFP_NUM_RATES   3
#define BITSTREAM_SIZE      15
#define G729_SPEECH_FRAME   80
#define G729_BITS_PER_SAMPLE 16
#define G729_SAMPLE_FREQUENCY 8000
#define G729_NCHANNELS        1
#define G729_SID_FRAMESIZE    2
#define G729_NUM_UNTR_FRAMES  2
#define VAD_G729FP_LOCAL_MEMORY_SIZE (1536+40)

int G729FPVAD(VADmemory* vadObj,const Ipp16s* src, Ipp16s* dst, Ipp32s *frametype );
static Ipp32s vadObjSize(void);

static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks);
static USC_Status MemAlloc_VAD_G729FP(const USC_FilterOption *options, USC_MemBank *pBanks);
static USC_Status GetInfoSize_VAD(Ipp32s *pSize);
static USC_Status Init_VAD_G729FP(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit_VAD_G729FP(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status Control_VAD_G729FP(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status GetInfo_VAD_G729FP(USC_Handle handle, USC_FilterInfo *pInfo);
static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine);
static USC_Status VAD_G729FP(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision);

/* global usc vector table */

USCFUN USC_Filter_Fxns USC_G729FP_VAD_Fxns=

{
    {
        USC_FilterVAD,
        (GetInfoSize_func) GetInfoSize_VAD,
        (GetInfo_func)     GetInfo_VAD_G729FP,
        (NumAlloc_func)    NumAlloc_VAD,
        (MemAlloc_func)    MemAlloc_VAD_G729FP,
        (Init_func)        Init_VAD_G729FP,
        (Reinit_func)      Reinit_VAD_G729FP,
        (Control_func)     Control_VAD_G729FP
    },
    SetDlyLine,
    VAD_G729FP,
};

typedef struct {
    Ipp32s vad;
    Ipp32s reserved; // for future extension
} G729_Handle_Header;

static __ALIGN32 CONST Ipp32f lwindow[LPC_ORDER+2] = {
   0.99879038f, 0.99546894f, 0.98995779f,
   0.98229335f, 0.97252620f, 0.96072035f,
   0.94695264f, 0.93131180f, 0.91389754f,
   0.89481964f, 0.87419660f, 0.85215437f
};
static Ipp32s vadObjSize()
{
   Ipp32s vadSize, fltsize;

   vadSize = sizeof(VADmemory);
   ippsIIRGetStateSize_32f(2,&fltsize);
   vadSize += fltsize;
   VADGetSize(&fltsize);
   vadSize += fltsize;
   ippsWinHybridGetStateSize_G729E_32f(&fltsize);
   vadSize += fltsize;

   return vadSize;
}

static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine)
{
   if(pDlyLine==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   return USC_NoOperation;
}

int G729FPVAD(VADmemory* vadObj,const Ipp16s* src, Ipp16s* dst, Ipp32s *frametype ){
   LOCAL_ALIGN_ARRAY(32, Ipp32f, forwardAutoCorr, (LPC_ORDERP2+1),vadObj);       /* Autocorrelations (forward) */
   LOCAL_ALIGN_ARRAY(32, Ipp32f, forwardLPC, LPC_ORDERP1*2,vadObj);      /* A(z) forward unquantized for the 2 subframes */
   LOCAL_ALIGN_ARRAY(32, Ipp32f, TmpAlignVec, WINDOW_LEN,vadObj);
   LOCAL_ARRAY(Ipp32f, forwardReflectCoeff, LPC_ORDER,vadObj);
   LOCAL_ARRAY(Ipp32f, CurrLSP, LPC_ORDER,vadObj);
   LOCAL_ARRAY(Ipp32f, CurrLSF, LPC_ORDER,vadObj);

   Ipp32f *pWindow;
   Ipp32f *newSpeech;
   /* Excitation vector */
   Ipp16s isVAD;
   Ipp32f tmp_lev;
   Ipp32s VADDecision;
   Ipp32f EnergydB;
   IppStatus sts;

   *frametype = 3;
   if(NULL==vadObj || NULL==src || NULL ==dst)
      return -10;
   if(vadObj->objPrm.objSize <= 0)
      return -8;
   if(ENC_KEY != vadObj->objPrm.key)
      return -5;
   isVAD = (Ipp16s)(vadObj->objPrm.mode == G729Encode_VAD_Enabled);

   if(!isVAD) return 0;

   newSpeech = vadObj->OldSpeechBuffer + SPEECH_BUFF_LEN - FRM_LEN;         /* New speech     */
   pWindow   = vadObj->OldSpeechBuffer + SPEECH_BUFF_LEN - WINDOW_LEN;        /* For LPC window */

   if (vadObj->sFrameCounter == 32767) vadObj->sFrameCounter = 256;
   else vadObj->sFrameCounter++;

   ippsConvert_16s32f(src,newSpeech,FRM_LEN);
   ownAutoCorr_G729_32f(pWindow, LPC_ORDERP2, forwardAutoCorr,TmpAlignVec);             /* Autocorrelations */

   /* Lag windowing    */
   ippsMul_32f(lwindow, &forwardAutoCorr[1], &forwardAutoCorr[1], LPC_ORDERP2);
   /* Levinson Durbin  */
   tmp_lev = 0;
   sts = ippsLevinsonDurbin_G729_32f(forwardAutoCorr, LPC_ORDER, &forwardLPC[LPC_ORDERP1], forwardReflectCoeff, &tmp_lev);
   if(sts == ippStsOverflow) {
      ippsCopy_32f(vadObj->OldForwardLPC,&forwardLPC[LPC_ORDERP1],LPC_ORDER+1);
      forwardReflectCoeff[0] = vadObj->OldForwardRC[0];
      forwardReflectCoeff[1] = vadObj->OldForwardRC[1];
   } else {
      ippsCopy_32f(&forwardLPC[LPC_ORDERP1],vadObj->OldForwardLPC,LPC_ORDER+1);
      vadObj->OldForwardRC[0] = forwardReflectCoeff[0];
      vadObj->OldForwardRC[1] = forwardReflectCoeff[1];
   }

   /* Convert A(z) to lsp */
   ippsLPCToLSP_G729_32f(&forwardLPC[LPC_ORDERP1], vadObj->OldLSP, CurrLSP);

   if (vadObj->objPrm.mode == G729Encode_VAD_Enabled) {
      ownACOS_G729_32f(CurrLSP, CurrLSF, LPC_ORDER);
      VoiceActivityDetect_G729_32f(forwardReflectCoeff[1], CurrLSF, forwardAutoCorr, pWindow, vadObj->sFrameCounter,
                  vadObj->prevVADDec, vadObj->prevPrevVADDec, &VADDecision, &EnergydB,(Ipp8s *)vadObj,TmpAlignVec);
   } else VADDecision = 1;

   if(VADDecision == 0) {
      /* Inactive frame */
      vadObj->prevVADDec = 0;
      *frametype=1; /* SID*/
   }
   else vadObj->prevVADDec = VADDecision;

   vadObj->prevPrevVADDec = vadObj->prevVADDec;
   ippsMove_32f(&vadObj->OldSpeechBuffer[FRM_LEN], &vadObj->OldSpeechBuffer[0], SPEECH_BUFF_LEN-FRM_LEN);
   CLEAR_SCRATCH_MEMORY(vadObj);
   return 0;
}

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G729FP[1]={
   {G729_SAMPLE_FREQUENCY,G729_BITS_PER_SAMPLE}
};

static USC_Status GetInfoSize_VAD(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_FilterInfo);
   return USC_NoError;
}

static USC_Status GetInfo_VAD_G729FP(USC_Handle handle, USC_FilterInfo *pInfo)
{
    G729_Handle_Header *g729_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_VADG729_FP";
    pInfo->nOptions = 1;
    pInfo->params[0].maxframesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    pInfo->params[0].minframesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    pInfo->params[0].framesize = G729_SPEECH_FRAME*sizeof(Ipp16s);

    if (handle == NULL) {
       pInfo->params[0].modes.vad = 1;
    } else {
       g729_header = (G729_Handle_Header*)handle;
       pInfo->params[0].modes.vad = g729_header->vad;
    }
    pInfo->params[0].pcmType.sample_frequency = pcmTypesTbl_G729FP[0].sample_frequency;
    pInfo->params[0].pcmType.bitPerSample = pcmTypesTbl_G729FP[0].bitPerSample;
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

static USC_Status MemAlloc_VAD_G729FP(const USC_FilterOption *options, USC_MemBank *pBanks)
{
    Ipp32u nbytes;
    Ipp32s srcatchMemSize = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    nbytes = vadObjSize();
    pBanks[0].nbytes = nbytes + sizeof(G729_Handle_Header); /* include header in handle */

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    srcatchMemSize = VAD_G729FP_LOCAL_MEMORY_SIZE;

    pBanks[1].nbytes = srcatchMemSize;

    return USC_NoError;
}

static USC_Status Init_VAD_G729FP(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
    G729_Handle_Header *g729_header;
    VADmemory *VadObj;

    Ipp32s i;
    Ipp32s fltsize;
    void* pBuf;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);
    USC_CHECK_PTR(pBanks[0].pMem);
    USC_BADARG(pBanks[0].nbytes<=0, USC_NotInitialized);
    USC_CHECK_PTR(pBanks[1].pMem);
    USC_BADARG(pBanks[1].nbytes<=0, USC_NotInitialized);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
    USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

    USC_BADARG(options->pcmType.sample_frequency < 1, USC_UnsupportedPCMType);
    USC_BADARG(options->pcmType.bitPerSample < 1, USC_UnsupportedPCMType);

    *handle = (USC_Handle*)pBanks[0].pMem;
    g729_header = (G729_Handle_Header*)*handle;

    g729_header->vad = options->modes.vad;
    VadObj = (VADmemory *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));
    ippsZero_16s((Ipp16s*)VadObj,sizeof(VADmemory)>>1) ;

    if(pBanks[1].pMem==NULL) return USC_BadArgument;
    if(VadObj==NULL) return USC_BadArgument;

    if(NULL==VadObj || NULL==pBanks[1].pMem)
       return USC_BadArgument;

    VadObj->Mem.base = pBanks[1].pMem;
    VadObj->Mem.CurPtr = VadObj->Mem.base;
    VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G729FP_LOCAL_MEMORY_SIZE);

   if(NULL==VadObj)
      return USC_BadArgument;

   ippsZero_32f(VadObj->OldSpeechBuffer, SPEECH_BUFF_LEN);

   VadObj->objPrm.objSize = vadObjSize();
   VadObj->objPrm.mode = g729_header->vad;
   VadObj->objPrm.key = ENC_KEY;

   pBuf = (Ipp8s*)VadObj + sizeof(VADmemory);
   ippsIIRGetStateSize_32f(2,&fltsize);
   VadObj = (VADmemory *)((Ipp8s*)pBuf + fltsize);
   VADGetSize(&fltsize);

   /* Static vectors to zero */
   for(i=0; i<MOVING_AVER_ORDER; i++)
   VadObj->sFrameCounter = 0;
   /* For G.729B */
   /* Initialize VAD/DTX parameters */
   //if(mode == G729Encode_VAD_Enabled) {
      VadObj->prevVADDec = 1;
      VadObj->prevPrevVADDec = 1;
      VADInit((Ipp8s*)VadObj);
   //}

    if(pBanks[1].pMem==NULL) return USC_BadArgument;
    if(VadObj==NULL) return USC_BadArgument;

    if(NULL==VadObj || NULL==pBanks[1].pMem)
       return USC_BadArgument;

    VadObj->Mem.base = pBanks[1].pMem;
    VadObj->Mem.CurPtr = VadObj->Mem.base;
    VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G729FP_LOCAL_MEMORY_SIZE);

    return USC_NoError;
}

static USC_Status Reinit_VAD_G729FP(const USC_FilterModes *modes, USC_Handle handle )
{
    G729_Handle_Header *g729_header;
    VADmemory *VadObj;

    Ipp32s i;
    Ipp32s fltsize;
    void* pBuf;
    Ipp8s* oldMemBuff;

    USC_CHECK_PTR(modes);
    USC_CHECK_HANDLE(handle);
    USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
    USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

    g729_header = (G729_Handle_Header*)handle;

    g729_header->vad = modes->vad;
    VadObj = (VADmemory *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
   if(NULL==VadObj)
      return USC_BadArgument;

   oldMemBuff = VadObj->Mem.base; /* if Reinit */
   ippsZero_32f(VadObj->OldSpeechBuffer, SPEECH_BUFF_LEN);

   ippsZero_16s((Ipp16s*)VadObj,sizeof(VADmemory)>>1) ;

   VadObj->objPrm.objSize = vadObjSize();
   VadObj->objPrm.mode = g729_header->vad;
   VadObj->objPrm.key = ENC_KEY;

   pBuf = (Ipp8s*)VadObj + sizeof(VADmemory);
   ippsIIRGetStateSize_32f(2,&fltsize);
   VadObj = (VADmemory *)((Ipp8s*)pBuf + fltsize);
   VADGetSize(&fltsize);

   /* Static vectors to zero */
   for(i=0; i<MOVING_AVER_ORDER; i++)
   VadObj->sFrameCounter = 0;
   /* For G.729B */
   /* Initialize VAD/DTX parameters */
   //if(mode == G729Encode_VAD_Enabled) {
      VadObj->prevVADDec = 1;
      VadObj->prevPrevVADDec = 1;
      VADInit((Ipp8s*)VadObj);
   //}

    if(oldMemBuff==NULL) return USC_BadArgument;
    if(VadObj==NULL) return USC_BadArgument;

    if(NULL==VadObj || NULL==oldMemBuff)
       return USC_BadArgument;

    VadObj->Mem.base = oldMemBuff;
    VadObj->Mem.CurPtr = VadObj->Mem.base;
    VadObj->Mem.VecPtr = (Ipp32s *)(VadObj->Mem.base+VAD_G729FP_LOCAL_MEMORY_SIZE);

    return USC_NoError;
}

static USC_Status Control_VAD_G729FP(const USC_FilterModes *modes, USC_Handle handle )
{
   G729_Handle_Header *g729_header;
   VADmemory *VadObj;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g729_header = (G729_Handle_Header*)handle;

   g729_header->vad = modes->vad;
   VadObj = (VADmemory *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
   if(G729Encode_VAD_Enabled != modes->vad && G729Encode_VAD_Disabled != modes->vad){
      return USC_BadArgument;
   }
   VadObj->objPrm.mode = modes->vad;
   return USC_NoError;
}

static USC_Status VAD_G729FP(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision)
{
    VADmemory *VadObj;
    Ipp32s *pVD;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_PTR(pVADDecision);
    USC_CHECK_HANDLE(handle);
    USC_BADARG(in->nbytes<G729_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);
    USC_BADARG(in->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(in->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
    USC_BADARG(in->nbytes<G729_SPEECH_FRAME*sizeof(Ipp16s), USC_BadDataPointer);

    pVD = (Ipp32s*)pVADDecision;

    VadObj = (VADmemory *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

    if(G729FPVAD(VadObj,(const Ipp16s*)in->pBuffer,(Ipp16s*)out->pBuffer,pVD) != 0){
      return USC_NoOperation;
    }

   if (*pVD == 0){
      *pVADDecision = NODECISION;
       ippsZero_8u((Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
   } else if (*pVD == 1){
      *pVADDecision = INACTIVE;
      ippsZero_8u((Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
   } else if (*pVD == 3) {
      ippsCopy_8u((Ipp8u*)in->pBuffer,(Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
      *pVADDecision = ACTIVE;
   }

    out->bitrate = in->bitrate;

    in->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);
    out->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);

    return USC_NoError;
}
