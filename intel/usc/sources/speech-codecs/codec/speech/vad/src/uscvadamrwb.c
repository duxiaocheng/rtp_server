/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2013 Intel Corporation. All Rights Reserved.
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
// Purpose: USC VAD AMRWB functions.
//
*/

#include <ipps.h>
#include <ippsc.h>
#include "usc_filter.h"
#include "aux_fnxs.h"


#define COMPLEN_AMRWB 12                         /* Number of sub-bands used by VAD              */

#define AMRWBE_BITS_PER_SAMPLE 16
//#define AMRWBE_SAMPLE_FREQUENCY 16000
//#define AMRWBE_NCHANNELS 1

#define L_FRAME16k   320                   /* Frame size at 16kHz                        */
#define AMRWBE_Frame   L_FRAME16k           /* Frame size at 16kHz                        */
#define DOWN_FAC  26215
#define NB_COEF_DOWN        15
#define FAC5 5
#define EHF_MASK (Ipp16s)0x0008            /* homing frame pattern                       */
#define WINDOW_SIZE         384              /* window size in LP analysis */
#define SPEECH_SIZE         WINDOW_SIZE      /* Total size of speech buffer */
#define FRAME_SIZE          256              /* Frame size */
#define PITCH_LAG_MAX      231                /* Maximum pitch lag */
#define OPL_DECIM          2                  /* Decimation in open-loop pitch analysis     */
#define WBP_OFFSET      (460)
#define INTERPOL_LEN       (LP_ORDER+1)       /* Length of filter for interpolation */
#define LP_ORDER            16               /* Order of LP filter */
#define SUBFRAME_SIZE      (FRAME_SIZE/NUMBER_SUBFRAME) /* Subframe size */
#define NUMBER_SUBFRAME     4                /* Number of subframe per frame */
#define MAX_PARAM_SIZE      57               /* Maximum number of params */
#define PREEMPH_FACTOR     22282              /* preemphasis factor */
#define MAX_SCALE_FACTOR   8                  /* scaling max for signal */
#define IO_MASK (Ipp16u)0xfffC             /* 14-bit input/output                        */
#define L_FRAME_PLUS    1024    /* 80ms frame size (long TCX frame)           */
#define L_FRAME48k      ((L_FRAME_PLUS/4)*15)      /* 3840 */

#define UP_SAMPL_FILT_DELAY 12
#define DOWN_SAMPL_FILT_DELAY (UP_SAMPL_FILT_DELAY+3)
#define HIGH_PASS_MEM_SIZE 31
#define L_FRAME_MAX_FS    (2*L_FRAME48k)
#define L_FILT_OVER_FS    12
#define WBP_OFFSET      (460)


typedef struct _HighPassFIRState_AMRWB_16s_ISfs{
   Ipp16s pFilterMemory[HIGH_PASS_MEM_SIZE-1];
   Ipp16s pFilterCoeff[HIGH_PASS_MEM_SIZE];
   Ipp32s ScaleFactor;
} HighPassFIRState_AMRWB_16s_ISfs;

typedef struct {
   IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnlIn;
   Ipp16s asiSpeechDecimate[2*DOWN_SAMPL_FILT_DELAY];
   Ipp16s siPreemph;
}TMPParam;

TMPParam *tmpPrm;

typedef struct _VADState_AMRWB_16s{
   Ipp16s asiBkgNoiseEstimate[COMPLEN_AMRWB];
   Ipp16s asiPrevAverageLevel[COMPLEN_AMRWB];     /* averaged input components for stationary */                                           /* estimation                               */
   Ipp16s asiPrevSignalLevel[COMPLEN_AMRWB];      /* input levels of the previous frame       */
   Ipp16s asiPrevSignalSublevel[COMPLEN_AMRWB];   /* input levels calculated at the end of a frame (lookahead)  */
   Ipp16s asiFifthFltState[5][2];                 /* memory for the filter bank               */
   Ipp16s asiThirdFltState[6];                    /* memory for the filter bank               */
   Ipp16s siBurstCount;                           /* counts length of a speech burst          */
   Ipp16s siHangCount;
   Ipp16s siStatCount;
   Ipp16s siVadReg;                               /* flags for intermediate VAD decisions     */
   Ipp16s siSpeechEstCnt;
   Ipp16s siSpeechMax;                            /* maximum level                            */
   Ipp16s siSpeechMaxCnt;                         /* counts frames that contains speech       */
   Ipp16s siSpeechEstLevel;
   Ipp32s iPowFramePrev;
   Ipp16s asiLevelWBE[COMPLEN_AMRWB];
   //Ipp16s siPreemph;
}VADState_AMRWB_16s;

typedef struct {
    Ipp32s trunc;
    Ipp32s vad;
    Ipp32s pcmSampFreq;
    Ipp32s bitPerSample;
    Ipp32s reserved0; // for future extension
    Ipp32s reserved1; // for future extension
} Handle_Header_AMRWBE;

void  ownDecimation_AMRWB_16s(Ipp16s *pSrcSignal16k, Ipp16s len,
                              Ipp16s *pDstSignal12k8, Ipp16s *pMem);
extern void ownScaleSignal_AMRWB_16s_ISfs(Ipp16s *pSrcDstSignal, Ipp32s   len, Ipp16s ScaleFactor);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        HighPassFIRGetSize_AMRWB_16s_ISfs
//  Purpose:     Knowing of AMR WB high pass FIR filter size demand
//  Parameters:
//    pDstSize      Pointer to the output value of the memory size needed for filtering
//  Returns:  ippStsNoErr, if no errors
*/

void HighPassFIRGetSize_AMRWB_16s_ISfs(Ipp32s *pDstSize);

int AMRWBEVAD(IppsVADState_AMRWB_16s* VADObj, const Ipp16u* src, int flg, Ipp32s *pVAD );

static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks);
static USC_Status MemAlloc_VAD_AMRWBE(const USC_FilterOption *options, USC_MemBank *pBanks);
static USC_Status GetInfoSize_VAD(Ipp32s *pSize);
static USC_Status Init_VAD_AMRWBE(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle);
static USC_Status GetInfo_VAD_AMRWBE(USC_Handle handle, USC_FilterInfo *pInfo);
static USC_Status VAD_AMRWBE(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision);

/* global usc vector table */

USCFUN USC_Filter_Fxns USC_AMRWBE_VAD_Fxns=
{
    {
        USC_FilterVAD,
        (GetInfoSize_func) GetInfoSize_VAD,
        (GetInfo_func)     GetInfo_VAD_AMRWBE,
        (NumAlloc_func)    NumAlloc_VAD,
        (MemAlloc_func)    MemAlloc_VAD_AMRWBE,
        (Init_func)        Init_VAD_AMRWBE,
        NULL,
        NULL,
    },
    NULL,
    VAD_AMRWBE,
};

#define AMRWBE_NUM_PCM_TYPES 4
static __ALIGN32 CONST USC_PCMType pcmTypesTbl_AMRWBE[AMRWBE_NUM_PCM_TYPES]={
   {16000,16,1}, {24000,16,1}, {32000,16,1}, {48000,16,1}
};

__ALIGN32 CONST Ipp16s BvadCoeffHP50Tbl[3] = {4053, -8106, 4053};
__ALIGN32 CONST Ipp16s AvadCoeffHP50Tbl[3] = {8192, 16211, -8021};

__ALIGN32 CONST Ipp16s vadFirDownSamplTbl[120] =
{            /* table x4/5 */
      -1,     0,    18,   -58,//k=0
      99,   -95,     0,   198,
    -441,   601,  -507,     0,
    1030, -2746,  6575, 12254,
       0, -1293,  1366,  -964,
     434,     0,  -233,   270,
    -188,    77,     0,   -26,
      19,    -6,
      -3,     9,     0,   -41,//k=1
     111,  -170,   153,     0,
    -295,   649,  -888,   770,
       0, -1997,  9894,  9894,
   -1997,     0,   770,  -888,
     649,  -295,     0,   153,
    -170,   111,   -41,     0,
       9,    -3,
      -6,    19,   -26,     0,//k=2
      77,  -188,   270,  -233,
       0,   434,  -964,  1366,
   -1293,     0, 12254,  6575,
   -2746,  1030,     0,  -507,
     601,  -441,   198,     0,
     -95,    99,   -58,    18,
       0,    -1,
      -5,    24,   -50,    54,//k=3
       0,  -128,   294,  -408,
     344,     0,  -647,  1505,
   -2379,  3034, 13107,  3034,
   -2379,  1505,  -647,     0,
     344,  -408,   294,  -128,
       0,    54,   -50,    24,
      -5,     0
};
static Ipp32s ownTestPCMFrameHoming(const Ipp16s *pSrc)
{
   Ipp32s i, fl = 0;
   for (i = 0; i < AMRWBE_Frame; i++) {
      fl = pSrc[i] ^ EHF_MASK;
      if (fl) break;
   }
   return (!fl);
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
static void ownDownSampling(Ipp16s * pSrcSignal, Ipp16s * pDstSignal, Ipp16s len)
{
   Ipp16s intPart, fracPart, pos;
   Ipp32s j, valSum;

   pos = 0;
   for (j = 0; j < len; j++) {
      intPart = (Ipp16s)(pos >> 2);
      fracPart = (Ipp16s)(pos & 3);
      ippsDotProd_16s32s_Sfs( &pSrcSignal[intPart- NB_COEF_DOWN + 1], &vadFirDownSamplTbl[(3-fracPart)*30],2*NB_COEF_DOWN, &valSum, -2);
      pDstSignal[j] = Cnvrt_NR_32s16s(valSum);
      pos += FAC5;
   }
}
int AMRWBEVAD
        (IppsVADState_AMRWB_16s* VADObj, const Ipp16u* src, int flg, Ipp32s *pVAD)
{
    /* Speech vector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pSpeechOldvec, SPEECH_SIZE + WBP_OFFSET);
    Ipp16s *pSpeechNew;//, *pSpeech, *pWindow;

    /* Weighted speech vector */
/*    IPP_ALIGNED_ARRAY(16, Ipp16s, pWgtSpchOldvec, FRAME_SIZE + (PITCH_LAG_MAX / OPL_DECIM)); */

    /* Excitation vector */
/*    IPP_ALIGNED_ARRAY(16, Ipp16s, pExcOldvec, (FRAME_SIZE + 1) + PITCH_LAG_MAX + INTERPOL_LEN); */

    /* Other vectors */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pVadvec, FRAME_SIZE);

    /* Scalars */

    Ipp16s valVadFlag;
    Ipp16s tmp,  valQNew, valShift;
    Ipp32s i, z, valMax;
    Ipp16s pToneFlag;

/*    IPP_ALIGNED_ARRAY(16, Ipp16s, pPrmvec, MAX_PARAM_SIZE); */


    pSpeechNew = pSpeechOldvec + SPEECH_SIZE - FRAME_SIZE - UP_SAMPL_FILT_DELAY/* + offsetWBE*/;

    //ippsZero_16s(tmpPrm->asiSpeechDecimate, 2 * NB_COEF_DOWN);

    if (flg == 24000){
        /* Down sampling signal from 16kHz to 12.8kHz */
        ownDecimation_AMRWB_16s((Ipp16s*)src, L_FRAME16k, pSpeechNew, tmpPrm->asiSpeechDecimate);

        /* Perform 50Hz HP filtering of input signal */
        ippsHighPassFilter_AMRWB_16s_ISfs(pSpeechNew, FRAME_SIZE, tmpPrm->pSHPFiltStateSgnlIn, 14);
    }
    else{
        ippsCopy_16s((Ipp16s*)src, pSpeechNew, FRAME_SIZE);
    }

    /* get max of new preemphased samples (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    z = pSpeechNew[0] << 15;
    z -= tmpPrm->siPreemph * PREEMPH_FACTOR;
    valMax = Abs_32s(z);

    for (i = 1; i < FRAME_SIZE + UP_SAMPL_FILT_DELAY; i++)
    {
        z  = pSpeechNew[i] << 15;
        z -= pSpeechNew[i - 1] * PREEMPH_FACTOR;
        z  = Abs_32s(z);
        if (z > valMax) valMax = z;
    }

    /* get scaling factor for new and previous samples */
    tmp = (Ipp16s)(valMax >> 16);

    if (tmp == 0)
    {
        valShift = MAX_SCALE_FACTOR;
    } else
    {
        valShift = (Ipp16s)(Exp_16s(tmp) - 1);
        if (valShift < 0) valShift = 0;
        if (valShift > MAX_SCALE_FACTOR) valShift = MAX_SCALE_FACTOR;
    }
    valQNew = valShift;

    /* preemphasis with scaling (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    tmp = pSpeechNew[FRAME_SIZE - 1];

    ippsPreemphasize_AMRWB_16s_ISfs(PREEMPH_FACTOR, pSpeechNew, FRAME_SIZE + UP_SAMPL_FILT_DELAY, 15-valQNew, &tmpPrm->siPreemph);

    tmpPrm->siPreemph = tmp;

    /*  Call VAD */
    ippsCopy_16s(pSpeechNew, pVadvec, FRAME_SIZE);
    pToneFlag = 0;
    ownScaleSignal_AMRWB_16s_ISfs(pVadvec, FRAME_SIZE, (Ipp16s)(1 - valQNew));
    ippsVAD_AMRWB_16s(pVadvec, VADObj, &pToneFlag, &valVadFlag);

    *pVAD = valVadFlag;

    return 0;
}


static USC_Status GetInfoSize_VAD(Ipp32s *pSize)
{
   USC_CHECK_PTR(pSize);
   *pSize = sizeof(USC_FilterInfo) + 3*sizeof(USC_FilterOption);

   return USC_NoError;
}
static USC_Status GetInfo_VAD_AMRWBE(USC_Handle handle, USC_FilterInfo *pInfo)
{
    USC_CHECK_PTR(pInfo);

    pInfo->name = "IPP_VADAMRWBE";
    pInfo->params[0].pcmType.sample_frequency = pcmTypesTbl_AMRWBE[0].sample_frequency;
    pInfo->params[0].pcmType.bitPerSample     = pcmTypesTbl_AMRWBE[0].bitPerSample;
    pInfo->params[0].pcmType.nChannels        = 1;
    pInfo->params[0].modes.vad = 1;
    pInfo->params[0].modes.reserved3 = 1;
    pInfo->params[0].framesize = AMRWBE_Frame*sizeof(Ipp16s);

    pInfo->params[1].pcmType.sample_frequency = pcmTypesTbl_AMRWBE[1].sample_frequency;
    pInfo->params[1].pcmType.bitPerSample     = pcmTypesTbl_AMRWBE[1].bitPerSample;
    pInfo->params[1].pcmType.nChannels        = 1;
    pInfo->params[1].modes.vad = 1;
    pInfo->params[1].modes.reserved3 = 1;
    pInfo->params[1].framesize = AMRWBE_Frame*sizeof(Ipp16s);

    pInfo->params[2].pcmType.sample_frequency = pcmTypesTbl_AMRWBE[2].sample_frequency;
    pInfo->params[2].pcmType.bitPerSample     = pcmTypesTbl_AMRWBE[2].bitPerSample;
    pInfo->params[2].pcmType.nChannels        = 1;
    pInfo->params[2].modes.vad = 1;
    pInfo->params[2].modes.reserved3 = 1;
    pInfo->params[2].framesize = AMRWBE_Frame*sizeof(Ipp16s);

    pInfo->params[3].pcmType.sample_frequency = pcmTypesTbl_AMRWBE[3].sample_frequency;
    pInfo->params[3].pcmType.bitPerSample     = pcmTypesTbl_AMRWBE[3].bitPerSample;
    pInfo->params[3].pcmType.nChannels        = 1;
    pInfo->params[3].modes.vad = 1;
    pInfo->params[3].modes.reserved3 = 1;
    pInfo->params[3].framesize = AMRWBE_Frame*sizeof(Ipp16s);

    pInfo->nOptions = AMRWBE_NUM_PCM_TYPES;

    return USC_NoError;
}
static USC_Status NumAlloc_VAD(const USC_FilterOption *options, Ipp32s *nbanks)
{
   USC_CHECK_PTR(options);
   USC_CHECK_PTR(nbanks);

   *nbanks = 1;
   return USC_NoError;
}

static USC_Status MemAlloc_VAD_AMRWBE(const USC_FilterOption *options, USC_MemBank *pBanks)
{
   Ipp32u nbytes;
   Ipp32s hpfltsize;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);

   pBanks->pMem = NULL;
   pBanks->align = 32;
   pBanks->memType = USC_OBJECT;
   pBanks->memSpaceType = USC_NORMAL;

   hpfltsize = sizeof(HighPassFIRState_AMRWB_16s_ISfs);
   hpfltsize = (hpfltsize+7)&(~7);
   nbytes = sizeof(IppsVADState_AMRWB_16s); // aligned by 8
   nbytes += (2*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize); // aligned by 8
   nbytes += (3*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(3, &hpfltsize); // aligned by 8
   nbytes += hpfltsize;
   ippsVADGetSize_AMRWB_16s(&hpfltsize); // not aligned by 8
   nbytes += (hpfltsize+7)&(~7);
   nbytes += 2*(2*L_FRAME_MAX_FS)*sizeof(Ipp16s); /* pBufChannelRight & pBufChannelLeft */
   nbytes += 2*((2*L_FILT_OVER_FS+7)&(~7))*sizeof(Ipp16s); /* pMemResampRight & pMemResampLeft */

   pBanks->nbytes = nbytes + sizeof(Handle_Header_AMRWBE); /* include header in handle */
   return USC_NoError;
}


static USC_Status Init_VAD_AMRWBE(const USC_FilterOption *options, const USC_MemBank *pBanks, USC_Handle *handle)
{
   Handle_Header_AMRWBE *amrwbe_header;
   Ipp32s hpfltsize;
   IppsVADState_AMRWB_16s *VADObj;

   USC_CHECK_PTR(options);
   USC_CHECK_PTR(pBanks);
   USC_CHECK_PTR(pBanks->pMem);
   USC_BADARG(pBanks->nbytes<=0, USC_NotInitialized);
   USC_CHECK_HANDLE(handle);
   USC_BADARG(options->modes.vad > 1, USC_UnsupportedVADType);
   USC_BADARG(options->modes.vad < 0, USC_UnsupportedVADType);

   USC_BADARG(options->pcmType.bitPerSample!=AMRWBE_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
   //USC_BADARG(options->pcmType.sample_frequency!=AMRWB_SAMPLE_FREQUENCY, USC_UnsupportedPCMType);
   //USC_BADARG(options->pcmType.nChannels!=AMRWB_NCHANNELS, USC_UnsupportedPCMType);

   *handle = (USC_Handle*)pBanks->pMem;
   amrwbe_header = (Handle_Header_AMRWBE*)*handle;

   amrwbe_header->pcmSampFreq = options->pcmType.sample_frequency;
   amrwbe_header->bitPerSample = options->pcmType.bitPerSample;

   amrwbe_header->trunc = options->modes.reserved3;
   amrwbe_header->vad = options->modes.vad;

   VADObj = (IppsVADState_AMRWB_16s *)((Ipp8s*)*handle + sizeof(Handle_Header_AMRWBE));
   tmpPrm = (TMPParam *) ((Ipp8s*)VADObj + sizeof(IppsVADState_AMRWB_16s));

   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   tmpPrm->pSHPFiltStateSgnlIn = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)tmpPrm + sizeof(TMPParam) + 2*hpfltsize);

   ///* routines initialization */

   ippsZero_16s(tmpPrm->asiSpeechDecimate, 2 * NB_COEF_DOWN);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)AvadCoeffHP50Tbl, (Ipp16s*)BvadCoeffHP50Tbl, 2, tmpPrm->pSHPFiltStateSgnlIn);

   ///* variable initialization */

   tmpPrm->siPreemph = 0;
   ippsVADInit_AMRWB_16s(VADObj);

   return USC_NoError;
}

static USC_Status VAD_AMRWBE(USC_Handle handle, USC_PCMStream *in, USC_PCMStream *out, USC_FrameType *pVADDecision)
{
    Handle_Header_AMRWBE *amrwbe_header;
    IppsVADState_AMRWB_16s *VADObj;
    Ipp32s *pVD;
    pVD = (Ipp32s*)pVADDecision;

    USC_CHECK_PTR(in);
    USC_CHECK_PTR(out);
    USC_CHECK_HANDLE(handle);

    USC_BADARG(NULL == in->pBuffer, USC_NoOperation);
    USC_BADARG(NULL == out->pBuffer, USC_NoOperation);

    USC_BADARG(in->nbytes<AMRWBE_Frame*sizeof(Ipp16s), USC_NoOperation);

    USC_BADARG(in->pcmType.bitPerSample!=AMRWBE_BITS_PER_SAMPLE, USC_UnsupportedPCMType);
    USC_BADARG(ownCheckSamplFreq(in->pcmType.sample_frequency), USC_UnsupportedPCMType);
    //USC_BADARG(in->pcmType.nChannels!=AMRWB_NCHANNELS, USC_UnsupportedPCMType);

    amrwbe_header = (Handle_Header_AMRWBE*)handle;

    if(amrwbe_header == NULL) return USC_InvalidHandler;

    VADObj = (IppsVADState_AMRWB_16s *)((Ipp8s*)handle + sizeof(Handle_Header_AMRWBE));

    /* check for homing frame */

    if(AMRWBEVAD(VADObj,(const Ipp16u*)in->pBuffer,in->pcmType.sample_frequency, pVD) != USC_NoError){
            return USC_NoOperation;
    }
    if (*pVD == 1){
        *pVADDecision = ACTIVE;
        ippsCopy_8u((Ipp8u*)in->pBuffer,(Ipp8u*)out->pBuffer, AMRWBE_Frame*sizeof(Ipp16s));
    }
    else
    {
        *pVADDecision = INACTIVE;
        ippsZero_8u((Ipp8u*)out->pBuffer, AMRWBE_Frame*sizeof(Ipp16s));
    }

    out->bitrate = in->bitrate;
    in->nbytes = AMRWBE_Frame*sizeof(Ipp16s);
    out->nbytes = AMRWBE_Frame*sizeof(Ipp16s);
    return USC_NoError;
} 
