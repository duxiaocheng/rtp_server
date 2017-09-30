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
// Purpose: USC VAD G729 functions.
//
*/
#include <ippsc.h>
#include <ipps.h>
#include "usc_filter.h"
#include "vadg729.h"
#include "aux_fnxs.h"

#define BITSTREAM_SIZE       15
#define G729_SPEECH_FRAME    80
#define G729_BITS_PER_SAMPLE 16
#define G729_SAMPLE_FREQUENCY 8000
#define G729_NCHANNELS 1
#define G729_SID_FRAMESIZE    2
#define G729_NUM_UNTR_FRAMES  2
#define VAD_G729_LOCAL_MEMORY_SIZE (1024+40)

typedef struct {
    Ipp32s vad;
    Ipp32s reserved; // for future extension
} G729_Handle_Header;

int G729VAD(VADmemory_Obj* vadObj,const Ipp16s* src, Ipp16s* dst, Ipp32s *pVAD );
static Ipp32s vadObjSize(void);

static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks);
static USC_Status MemAlloc_VAD_G729(const USC_FilterOption *options, USC_MemBank *pBanks);
static USC_Status GetInfoSize_VAD(Ipp32s *pSize);
static USC_Status Init_VAD_G729(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status Reinit_VAD_G729(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status Control_VAD_G729(const USC_FilterModes *modes, USC_Handle handle );
static USC_Status GetInfo_VAD_G729(USC_Handle handle, USC_FilterInfo *pInfo);
static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine);
static USC_Status VAD_G729(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision);

/* global usc vector table */

USCFUN USC_Filter_Fxns USC_G729_VAD_Fxns=
{
    {
        USC_FilterVAD,
        (GetInfoSize_func) GetInfoSize_VAD,
        (GetInfo_func)     GetInfo_VAD_G729,
        (NumAlloc_func)    NumAlloc_VAD,
        (MemAlloc_func)    MemAlloc_VAD_G729,
        (Init_func)        Init_VAD_G729,
        (Reinit_func)      Reinit_VAD_G729,
        (Control_func)     Control_VAD_G729
    },
    SetDlyLine,
    VAD_G729,
};

static __ALIGN32 CONST USC_PCMType pcmTypesTbl_G729I[1]={
   {G729_SAMPLE_FREQUENCY,G729_BITS_PER_SAMPLE}
};

/*------------------------------------------------------------*/
static __ALIGN32 CONST Ipp16s tab1[3] = {
 (1<<10)+875,
 -3798,
 (1<<10)+875
};
static __ALIGN32 CONST Ipp16s tab2[3] = {
 (1<<12),
 (1<<12)+3711,
 -3733
};

static __ALIGN32 CONST Ipp16s hammingWin[LP_WINDOW_DIM] = {
    (1<<11)+573,  (1<<11)+575,(1<<11)+581,  2638,  2651,
    2668,  2689,
    2713,  2741,  2772,
    2808,  2847,  2890,
    2936,  2986,
    3040,  3097,  3158,  3223,  3291,  3363,  3438,  3517,  3599,  3685,  3774,  3867, 3963,
    4063,  4166,  4272,  4382,  4495,  4611,  4731,  4853,  4979,
    5108,  5240,  5376,  5514,  5655,  5800,  5947,
    6097,  6250,  6406,  6565,  6726,  6890,
    7057,  7227,  7399,  7573,  7750,  7930,
    8112,  8296,  8483,  8672,  8863,
    9057,  9252,  9450,  9650,  9852,
    10055, 10261, 10468, 10677, 10888,
    11101, 11315, 11531, 11748, 11967,
    12187, 12409, 12632, 12856,
    13082, 13308, 13536, 13764, 13994,
    14225, 14456, 14688, 14921,
    15155, 15389, 15624, 15859,
    16095, 16331, 16568, 16805,
    17042, 17279, 17516, 17754, 17991,
    18228, 18465, 18702, 18939,
    19175, 19411, 19647, 19882,
    20117, 20350, 20584, 20816, 21048,
    21279, 21509, 21738, 21967,
    22194, 22420, 22644, 22868,
    23090, 23311, 23531, 23749, 23965,
    24181, 24394, 24606, 24816,
    25024, 25231, 25435, 25638, 25839,
    26037, 26234, 26428, 26621, 26811, 26999,
    27184, 27368, 27548, 27727, 27903,
    28076, 28247, 28415, 28581, 28743, 28903,
    29061, 29215, 29367, 29515, 29661, 29804, 29944,
    30081, 30214, 30345, 30472, 30597, 30718, 30836, 30950,
    31062, 31170, 31274, 31376, 31474, 31568, 31659, 31747, 31831, 31911, 31988,
    32062, 32132, 32198, 32261, 32320, 32376, 32428, 32476, 32521, 32561, 32599,
    32632, 32662, 32688, 32711, 32729, 32744, 32755, 32763, IPP_MAX_16S, IPP_MAX_16S,
    32741, 32665, 32537, 32359, 32129,
    31850, 31521, 31143,
    30716, 30242,
    29720, 29151,
    28538,
    27879, 27177,
    26433,
    25647,
    24821,
    23957, 23055,
    22117,
    21145,
    20139,
    19102,
    18036,
    16941,
    15820,
    14674,
    13505,
    12315,
    11106,
    9879,
    8637,
    7381,
    6114,
    4838,
    3554,
    2264,
    971
};

static USC_Status SetDlyLine(USC_Handle handle, Ipp8s *pDlyLine)
{
   if(pDlyLine==NULL) return USC_BadDataPointer;
   if(handle==NULL) return USC_InvalidHandler;

   return USC_NoOperation;
}

static Ipp32s vadObjSize(void) {
    Ipp32s fltSize;
    Ipp32s objSize=sizeof(VADmemory_Obj);
    ippsHighPassFilterSize_G729(&fltSize);
    objSize += fltSize;
    objSize += 2 * fltSize;
    VoiceActivityDetectSize_G729(&fltSize);
    objSize += fltSize;
    objSize += 4*32;
    return objSize;
}

int G729VAD(VADmemory_Obj* vadObj,const Ipp16s *src, Ipp16s *dst, Ipp32s *frametype ) {
    LOCAL_ALIGN_ARRAY(32, Ipp16s, rCoeff, LPF_DIM,vadObj);
    LOCAL_ALIGN_ARRAY(32, Ipp16s, newLSP, LPF_DIM,vadObj);
    LOCAL_ALIGN_ARRAY(32, Ipp32s, autoR, VAD_LPC_DIM +2,vadObj);
    LOCAL_ALIGN_ARRAY(32, Ipp16s, newLSF, LPF_DIM,vadObj);
    LOCAL_ALIGN_ARRAY(32, Ipp16s, yVal, LP_WINDOW_DIM,vadObj);
    Ipp16s norma,Vad=1;
    Ipp16s *speechHistory = vadObj->speechHistory;
    Ipp16s vadEnable = (Ipp16s)((vadObj->objPrm.mode == G729Encode_VAD_Enabled));
    Ipp32s   i;

    if(NULL==vadObj || NULL==src || NULL ==dst)
        return -3;
    if(vadObj->preProc == NULL)
        return -4;
    if(vadObj->objPrm.objSize <= 0)
        return -4;
    if(ENC_KEY != vadObj->objPrm.key)
        return -5;

   *frametype = 3;

   if(!vadEnable) return 0;

    ippsCopy_16s(src,vadObj->speechHistory + SPEECH_BUF_DIM - LP_FRAME_DIM,LP_FRAME_DIM);
    ippsHighPassFilter_G729_16s_ISfs(
                                    vadObj->speechHistory + SPEECH_BUF_DIM - LP_FRAME_DIM,LP_FRAME_DIM,12,vadObj->preProc);
    norma=1;
    ippsMul_NR_16s_Sfs(LPC_WINDOW,hammingWin,yVal,LP_WINDOW_DIM,15);
    while(ippsAutoCorr_NormE_16s32s(yVal,LP_WINDOW_DIM,autoR,VAD_LPC_DIM +1,&i)!=ippStsNoErr) {
        ippsRShiftC_16s(yVal,2,yVal,LP_WINDOW_DIM);
        norma+=4;
    }
    norma = (Ipp16s)(norma - i);
    ippsLagWindow_G729_32s_I(autoR+1,VAD_LPC_DIM );

    ippsLSPToLSF_Norm_G729_16s(newLSP, newLSF);
    {
       LOCAL_ALIGN_ARRAY(32, Ipp16s, tmpVecVAD_LPC_DIM3, VAD_LPC_DIM +1, vadObj);
       VoiceActivityDetect_G729(LPC_WINDOW,newLSF,autoR,norma,rCoeff[1],&Vad,(Ipp8s*)vadObj, tmpVecVAD_LPC_DIM3);
       LOCAL_ALIGN_ARRAY_FREE(32, Ipp16s, tmpVecVAD_LPC_DIM3, VAD_LPC_DIM +1, vadObj);
    }

    if(Vad==0) {
       *frametype=1;
    }
    vadObj->VADPPrev = vadObj->VADPrev;
    vadObj->VADPrev = Vad;
    LOCAL_ARRAY_FREE(Ipp16s, yVal, LP_WINDOW_DIM,vadObj);
    LOCAL_ARRAY_FREE(Ipp16s, newLSF, LPF_DIM,vadObj);
    LOCAL_ARRAY_FREE(Ipp32s, autoR, VAD_LPC_DIM +2,vadObj);
    LOCAL_ARRAY_FREE(Ipp16s, newLSP, LPF_DIM,vadObj);
    LOCAL_ARRAY_FREE(Ipp16s, rCoeff, LPF_DIM,vadObj);
    CLEAR_SCRATCH_MEMORY(vadObj);
    return 0;
}


/*---------------------------------------------------------------*/

static USC_Status GetInfoSize_VAD(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_FilterInfo);
   return USC_NoError;
}
static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 2;

   return USC_NoError;
}

static USC_Status MemAlloc_VAD_G729(const USC_FilterOption *options, USC_MemBank *pBanks)
{
    Ipp32u nbytes;
    Ipp32s srcatchMemSize = 0;

    USC_CHECK_PTR(options);
    USC_CHECK_PTR(pBanks);

    pBanks[0].pMem = NULL;
    pBanks[0].align = 32;
    pBanks[0].memType = USC_OBJECT;
    pBanks[0].memSpaceType = USC_NORMAL;

    nbytes =  (Ipp32u)vadObjSize();

    pBanks[0].nbytes = nbytes + sizeof(G729_Handle_Header); /* include header in handle */

    pBanks[1].pMem = NULL;
    pBanks[1].align = 32;
    pBanks[1].memType = USC_BUFFER;
    pBanks[1].memSpaceType = USC_NORMAL;

    srcatchMemSize = VAD_G729_LOCAL_MEMORY_SIZE;

    pBanks[1].nbytes = srcatchMemSize;
    return USC_NoError;
}

static USC_Status Init_VAD_G729(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   VADmemory_Obj *VADObj;

   Ipp32s fltSize;
   Ipp16s abEnc[6];

   Ipp8s* oldMemBuff;

   G729_Handle_Header *g729_header;

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
   g729_header = (G729_Handle_Header*)*handle;

   g729_header->vad = options->modes.vad;

   VADObj = (VADmemory_Obj *)((Ipp8s*)*handle + sizeof(G729_Handle_Header));

   if(NULL==VADObj || NULL==pBanks[1].pMem)
      return USC_BadArgument;

   VADObj->Mem.base = pBanks[1].pMem;
   VADObj->Mem.CurPtr = VADObj->Mem.base;
   VADObj->Mem.VecPtr = (Ipp32s *)(VADObj->Mem.base+VAD_G729_LOCAL_MEMORY_SIZE);


   if(NULL==VADObj)
      return USC_BadArgument;

   oldMemBuff = VADObj->Mem.base; /* if Reinit */

   ippsZero_16s((Ipp16s*)VADObj,sizeof(*VADObj)>>1) ;

   VADObj->objPrm.objSize = vadObjSize();
   VADObj->objPrm.mode = g729_header->vad;
   VADObj->objPrm.key = ENC_KEY;

   VADObj->preProc = (Ipp8s*)VADObj + sizeof(VADmemory_Obj);
   VADObj->preProc = IPP_ALIGNED_PTR(VADObj->preProc, 16);
   ippsHighPassFilterSize_G729(&fltSize);
//   SynthesisFilterSize_G729(&fltSize);
   abEnc[0] = tab2[0]; abEnc[1] = tab2[1]; abEnc[2] = tab2[2];
   abEnc[3] = tab1[0]; abEnc[4] = tab1[1]; abEnc[5] = tab1[2];
   VoiceActivityDetectInit_G729((Ipp8s*)VADObj);
   ippsHighPassFilterInit_G729(abEnc,VADObj->preProc);

   ippsZero_16s(VADObj->speechHistory,SPEECH_BUF_DIM);
   ippsCopy_16s(presetOldA, VADObj->prevSubfrLPC, LPF_DIM+1);
   ippsZero_16s(VADObj->encPrm, PARM_DIM+1);
   ippsZero_16s(VADObj->BWDsynth, TBWD_DIM);
   ippsWinHybridGetStateSize_G729E_16s(&fltSize);
   if(fltSize > BWLPCF1_DIM*sizeof(Ipp32s)) {
      return USC_NotInitialized;
   }

   VADObj->Mem.base = oldMemBuff;
   VADObj->Mem.CurPtr = VADObj->Mem.base;
   VADObj->Mem.VecPtr = (Ipp32s *)(VADObj->Mem.base+VAD_G729_LOCAL_MEMORY_SIZE);

   return USC_NoError;
}

static USC_Status Reinit_VAD_G729(const USC_FilterModes *modes, USC_Handle handle )
{
   VADmemory_Obj *VADObj;
   Ipp32s fltSize;
   Ipp16s abEnc[6];

   Ipp8s* oldMemBuff;

   G729_Handle_Header *g729_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g729_header = (G729_Handle_Header*)handle;

   g729_header->vad = modes->vad;

   VADObj = (VADmemory_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

   if(NULL==VADObj)
      return USC_BadArgument;

   oldMemBuff = VADObj->Mem.base; /* if Reinit */

   ippsZero_16s((Ipp16s*)VADObj,sizeof(*VADObj)>>1) ;

   VADObj->objPrm.objSize = vadObjSize();
   VADObj->objPrm.mode = g729_header->vad;
   VADObj->objPrm.key = ENC_KEY;

   VADObj->preProc = (Ipp8s*)VADObj + sizeof(VADmemory_Obj);
   VADObj->preProc = IPP_ALIGNED_PTR(VADObj->preProc, 16);
   ippsHighPassFilterSize_G729(&fltSize);
//   SynthesisFilterSize_G729(&fltSize);
   abEnc[0] = tab2[0]; abEnc[1] = tab2[1]; abEnc[2] = tab2[2];
   abEnc[3] = tab1[0]; abEnc[4] = tab1[1]; abEnc[5] = tab1[2];
   VoiceActivityDetectInit_G729((Ipp8s*)VADObj);
   ippsHighPassFilterInit_G729(abEnc,VADObj->preProc);

   ippsZero_16s(VADObj->speechHistory,SPEECH_BUF_DIM);
   ippsCopy_16s(presetOldA, VADObj->prevSubfrLPC, LPF_DIM+1);
   ippsZero_16s(VADObj->encPrm, PARM_DIM+1);
   ippsZero_16s(VADObj->BWDsynth, TBWD_DIM);
   ippsWinHybridGetStateSize_G729E_16s(&fltSize);
   if(fltSize > BWLPCF1_DIM*sizeof(Ipp32s)) {
      return USC_NotInitialized;
   }

   VADObj->Mem.base = oldMemBuff;
   VADObj->Mem.CurPtr = VADObj->Mem.base;
   VADObj->Mem.VecPtr = (Ipp32s *)(VADObj->Mem.base+VAD_G729_LOCAL_MEMORY_SIZE);

   return USC_NoError;
}

static USC_Status Control_VAD_G729(const USC_FilterModes *modes, USC_Handle handle )
{
   VADmemory_Obj *VADObj;
   G729_Handle_Header *g729_header;

   USC_CHECK_PTR(modes);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(modes->vad > 1, USC_UnsupportedVADType);
   USC_BADARG(modes->vad < 0, USC_UnsupportedVADType);

   g729_header = (G729_Handle_Header*)handle;

   g729_header->vad = modes->vad;
   VADObj = (VADmemory_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));
   if(G729Encode_VAD_Enabled != modes->vad && G729Encode_VAD_Disabled != modes->vad) {
      return USC_BadArgument;
   }
   VADObj->objPrm.mode = modes->vad;

   return USC_NoError;
}

static USC_Status GetInfo_VAD_G729(USC_Handle handle, USC_FilterInfo *pInfo)
{
    G729_Handle_Header *g729_header;

    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_VADG729";
    pInfo->params[0].maxframesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    pInfo->params[0].minframesize = G729_SPEECH_FRAME*sizeof(Ipp16s);
    pInfo->params[0].framesize = G729_SPEECH_FRAME*sizeof(Ipp16s);

    if (handle == NULL) {
       pInfo->params[0].modes.vad = 1;
    } else {
       g729_header = (G729_Handle_Header*)handle;
       pInfo->params[0].modes.vad = g729_header->vad;
    }
    pInfo->params[0].pcmType.sample_frequency = pcmTypesTbl_G729I[0].sample_frequency;
    pInfo->params[0].pcmType.bitPerSample = pcmTypesTbl_G729I[0].bitPerSample;
    pInfo->params[0].pcmType.nChannels = 1;
    pInfo->nOptions = 1;

   return USC_NoError;
}

static USC_Status VAD_G729(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision)
{
   VADmemory_Obj *VADObj;

   Ipp32s *pVD;
   pVD = (Ipp32s*)pVADDecision;

   USC_CHECK_PTR(in);
   USC_CHECK_PTR(out);
   USC_CHECK_PTR(pVD);
   USC_CHECK_HANDLE(handle);

   USC_BADARG(in->nbytes<G729_SPEECH_FRAME*sizeof(Ipp16s), USC_NoOperation);

   USC_BADARG(in->pcmType.bitPerSample!=G729_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   USC_BADARG(in->pcmType.sample_frequency!=G729_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);

   VADObj = (VADmemory_Obj *)((Ipp8s*)handle + sizeof(G729_Handle_Header));

   if(G729VAD(VADObj, (const Ipp16s*)in->pBuffer, (Ipp16s*)out->pBuffer, pVD) != USC_NoError){//Ipp8u
      return USC_NoOperation;
   }

   if (*pVD == 0){
      *pVADDecision = NODECISION;
       ippsZero_8u((Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
   } else if (*pVD == 1){
      *pVADDecision = INACTIVE;
      ippsZero_8u((Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
   } else if (*pVD == 3) {
      *pVADDecision = ACTIVE;
      ippsCopy_8u((Ipp8u*)in->pBuffer,(Ipp8u*)out->pBuffer, G729_SPEECH_FRAME*sizeof(Ipp16s));
   }

   out->bitrate = in->bitrate;

   in->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);
   out->nbytes = G729_SPEECH_FRAME*sizeof(Ipp16s);

   return USC_NoError;
}
