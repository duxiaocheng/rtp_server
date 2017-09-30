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
// Purpose: AMRWB speech codec own header file
//
*/

#ifndef __OWNAMRWB_H__
#define __OWNAMRWB_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "amrwbapi.h"
#include "scratchmem.h"

#ifndef NULL
#define NULL ((void*)0)
#endif /* NULL */

extern CONST IppSpchBitRate Mode2RateTbl[NUM_OF_MODES];
extern CONST Ipp16s NumberPrmsTbl[NUM_OF_MODES];
extern CONST Ipp16s *pNumberBitsTbl[NUM_OF_MODES];

#define ENC_KEY 0xecd7222
#define DEC_KEY 0xdec7222

/* Codec constant parameters (coder and decoder) */

#define CODEC_VERSION "5.5.0"
#define FRAME_SIZE          256              /* Frame size */
#define SUBFRAME_SIZE_16k   (L_FRAME16k/NUMBER_SUBFRAME) /* Subframe size at 16kHz */
#define MAX_PARAM_SIZE      57               /* Maximum number of params */

#define SUBFRAME_SIZE      (FRAME_SIZE/NUMBER_SUBFRAME) /* Subframe size */
#define NUMBER_SUBFRAME     4                /* Number of subframe per frame */

#define WINDOW_SIZE         384              /* window size in LP analysis */
#define SPEECH_SIZE         WINDOW_SIZE      /* Total size of speech buffer */
#define LP_ORDER            16               /* Order of LP filter */
#define LP_ORDER_16K        20

#define UP_SAMPL_FILT_DELAY 12
#define DOWN_SAMPL_FILT_DELAY (UP_SAMPL_FILT_DELAY+3)

#define PITCH_GAIN_CLIP    15565              /* Pitch gain clipping */
#define PITCH_SHARP_FACTOR 27853              /* pitch sharpening factor */

#define PITCH_LAG_MAX      231                /* Maximum pitch lag */
#define OPL_DECIM          2                  /* Decimation in open-loop pitch analysis     */
#define INTERPOL_LEN       (LP_ORDER+1)       /* Length of filter for interpolation */

#define PREEMPH_FACTOR     22282              /* preemphasis factor */
#define TILT_FACTOR        22282              /* tilt factor (denominator) */
#define WEIGHT_FACTOR      30147              /* Weighting factor (numerator) */

#define MAX_SCALE_FACTOR   8                  /* scaling max for signal */
#define RND_INIT           21845              /* random init value */

/* Constants for Voice Activity Detection */

#define DTX_MAX_EMPTY_THRESH 50
#define DTX_HIST_SIZE       8
#define DTX_HIST_SIZE_MIN_ONE (DTX_HIST_SIZE-1)
#define DTX_ELAPSED_FRAMES_THRESH 30
#define DTX_HANG_CONST      7                  /* yields eight frames of SP HANGOVER */
#define INV_MED_THRESH      14564

#define ISF_GAP             128
#define ISF_FACTOR_LOW      256
#define ISF_DITHER_GAP      448
#define ISF_FACTOR_STEP     2

#define GAIN_FACTOR         75
#define GAIN_THRESH         180
#define THRESH_GAIN_PITCH   14746

#define DTX                 1
#define DTX_MUTE            2
#define SPEECH              0

#define DIST_ISF_MAX_IO     384                /* 150 Hz (6400Hz=16384) */
#define MAX_DISTANCE_ISF    307                /* 120 Hz (6400Hz=16384) */
#define THRESH_DISTANCE_ISF 154
#define MIN_GAIN_PITCH      9830

#define NB_COEF_DOWN        15
#define NB_COEF_UP          12
#define PRED_ORDER          4

/*
   Some arrays reference
 */
extern CONST Ipp32s   LagWindowTbl[LP_ORDER];
extern CONST Ipp16s LPWindowTbl[WINDOW_SIZE];
extern CONST Ipp16s FirUpSamplTbl[120];
extern CONST Ipp16s FirDownSamplTbl[120];
extern CONST Ipp16s IspInitTbl[LP_ORDER];
extern CONST Ipp16s IsfInitTbl[LP_ORDER];
extern CONST Ipp16s HPGainTbl[16];
extern CONST Ipp16s Fir6k_7kTbl[31];
extern CONST Ipp16s Fir7kTbl[31];
extern CONST Ipp16s InterpolFracTbl[NUMBER_SUBFRAME];
extern CONST Ipp16s BCoeffHP50Tbl[3];
extern CONST Ipp16s ACoeffHP50Tbl[3];
extern CONST Ipp16s BCoeffHP400Tbl[3];
extern CONST Ipp16s ACoeffHP400Tbl[3];
extern CONST Ipp16s PCoeffDownUnusableTbl[7];
extern CONST Ipp16s CCoeffDownUnusableTbl[7];
extern CONST Ipp16s PCoeffDownUsableTbl[7];
extern CONST Ipp16s CCoeffDownUsableTbl[7];
extern CONST Ipp16s ACoeffTbl[4];
extern CONST Ipp16s BCoeffTbl[4];

/*
   Procedure definitions
 */
Ipp32s   ownPow2_AMRWB(Ipp16s valExp, Ipp16s valFrac);
void  ownLagWindow_AMRWB_32s_I(Ipp32s *pSrcDst, Ipp32s len);
void  ownAutoCorr_AMRWB_16s32s(Ipp16s *pSrcSignal, Ipp32s valLPCOrder, Ipp32s *pDst,
                               const Ipp16s* tblWindow, Ipp32s windowLen);
void  ownScaleSignal_AMRWB_16s_ISfs(Ipp16s *pSrcDstSignal, Ipp32s   len, Ipp16s ScaleFactor);
Ipp16s ownChkStab(Ipp16s *pSrc1, Ipp16s *pSrc2, Ipp32s len);
void  ownDecimation_AMRWB_16s(Ipp16s *pSrcSignal16k, Ipp16s len,
                              Ipp16s *pDstSignal12k8, Ipp16s *pMem);
void  ownOversampling_AMRWB_16s(Ipp16s *pSrcSignal12k8, Ipp16s len,
                                Ipp16u *pDstSignal16k,Ipp16s *pMem);
Ipp16s ownMedian5(Ipp16s *x);
void  ownagc2(Ipp16s *pSrcPFSignal, Ipp16s *pDstPFSignal, Ipp16s len);
void  ownIsfExtrapolation(Ipp16s *pHfIsf);
void  ownLPDecim2(Ipp16s *pSignal, Ipp32s len, Ipp16s *pMem);
Ipp16s ownVoiceFactor(Ipp16s *pPitchExc, Ipp16s valExcFmt, Ipp16s valPitchGain,
                      Ipp16s *pFixCdbkExc, Ipp16s valCodeGain, Ipp32s len);
Ipp16s ownGpClip(IppSpchBitRate irate, Ipp16s *pMem);
void  ownCheckGpClipIsf(IppSpchBitRate irate, Ipp16s *pIsfvec, Ipp16s *pMem);
void  ownCheckGpClipPitchGain(IppSpchBitRate irate, Ipp16s valPitchGain, Ipp16s *pMem);
void  ownPhaseDispInit(Ipp16s *pDispvec);
void  ownPhaseDisp(Ipp16s valCodeGain, Ipp16s valPitchGain, Ipp16s *pFixCdbkExc,
                   Ipp16s valLevelDisp, Ipp16s *pDispvec);
Ipp16s amrExtractBits( const Ipp8s **pBitStrm, Ipp32s *currBitstrmOffset, Ipp32s numParamBits );
void ownSynthesisFilter_AMRWB_16s32s_ISfs
       ( const Ipp16s *pSrcLpc, Ipp32s valLpcOrder, const Ipp16s *pSrcExcitation,
         Ipp32s *pSrcDstSignal, Ipp32s len, Ipp32s scaleFactor );
void ownISPToLPC_AMRWB_16s
       ( const Ipp16s *pSrcIsp, Ipp16s *pDstLpc, Ipp32s len, Ipp32s adaptive_scaling );
#if(0)
/* extention */
void ownSoftExcitationHF(Ipp16s *exc_hf, Ipp32s *mem);
void ownSmoothEnergyHF(Ipp16s *HF, Ipp32s *threshold);
#endif

#define HIGH_PASS_MEM_SIZE 31

typedef struct _HighPassFIRState_AMRWB_16s_ISfs{
   Ipp16s pFilterMemory[HIGH_PASS_MEM_SIZE-1];
   Ipp16s pFilterCoeff[HIGH_PASS_MEM_SIZE];
   Ipp32s ScaleFactor;
} HighPassFIRState_AMRWB_16s_ISfs;

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        HighPassFIRGetSize_AMRWB_16s_ISfs
//  Purpose:     Knowing of AMR WB high pass FIR filter size demand
//  Parameters:
//    pDstSize      Pointer to the output value of the memory size needed for filtering
//  Returns:  ippStsNoErr, if no errors
*/

void HighPassFIRGetSize_AMRWB_16s_ISfs(Ipp32s *pDstSize);

/* /////////////////////////////////////////////////////////////////////////////
//  Name:        HighPassFIRInit_AMRWB_16s_ISfs
//  Purpose:     Initialization of the memory allocated for AMR WB high pass FIR filter
//  Parameters:
//    pSrcFilterCoeff    pointer to the filter coefficienst vector of lenght 31
//    ScaleFactor        scale factor value
//    pState             pointer to the memory supplied for filtering
//  Returns:  ippStsNoErr, if no errors
//
*/

void HighPassFIRInit_AMRWB_16s_ISfs(Ipp16s *pSrcFilterCoeff, Ipp32s ScaleFactor,
                                    HighPassFIRState_AMRWB_16s_ISfs *pState);
/* /////////////////////////////////////////////////////////////////////////////
//  Name:        HighPassFIR_AMRWB_16s_ISfs
//  Purpose:     High-pass FIR filtering
//  Parameters:
//    pSrcDstSignal        pointer to the vector for inplace operation
//    pState    pointer to the memory supplied for filtering
//  Returns:  ippStsNoErr, if no errors
*/

void HighPassFIR_AMRWB_16s_ISfs(Ipp16s *pSrcDstSignal,
                                Ipp32s len,
                                HighPassFIRState_AMRWB_16s_ISfs *pState);

/*
   The encoder and decoder objects structures definitions
*/
typedef struct _SDtxEncoderState
{
    Ipp16s asiIsfHistory[LP_ORDER * DTX_HIST_SIZE];
    Ipp16s siLogEnerHist[DTX_HIST_SIZE];
    Ipp16s siHistPtr;
    Ipp16s siLogEnerIndex;
    Ipp16s siCngSeed;
    Ipp16s siHangoverCount;
    Ipp16s siAnaElapsedCount;
    Ipp32s   aiDistMatrix[28];
    Ipp32s   aiSumDist[DTX_HIST_SIZE];
} SDtxEncoderState;

typedef struct _SDtxDecoderState
{
    Ipp16s siSidLast;
    Ipp16s siSidPeriodInv;
    Ipp16s siLogEnergy;
    Ipp16s siLogEnergyOld;
    Ipp16s asiIsf[LP_ORDER];
    Ipp16s asiIsfOld[LP_ORDER];
    Ipp16s siCngSeed;
    Ipp16s asiIsfHistory[LP_ORDER * DTX_HIST_SIZE];
    Ipp16s siLogEnerHist[DTX_HIST_SIZE];
    Ipp16s siHistPtr;
    Ipp16s siHangoverCount;
    Ipp16s siAnaElapsedCount;
    Ipp16s siSidFrame;
    Ipp16s siValidData;
    Ipp16s siHangoverAdded;
    Ipp16s siGlobalState;
    Ipp16s siDataUpdated;
    Ipp16s siDitherSeed;
    Ipp16s siComfortNoiseDith;
    Ipp16s sidtxVadHist;
} SDtxDecoderState;

typedef struct _SAMRWBCoderObj
{
   Ipp32s iObjSize;
   Ipp32s iKey;
   Ipp32u uiMode;         /* coder mode's */
   Ipp32u uiRez;          /* rezerved */
} SAMRWBCoderObj;

struct _AMRWBEncoder_Obj {
   SAMRWBCoderObj objPrm;
   Ipp16s asiSpeechDecimate[2*DOWN_SAMPL_FILT_DELAY];
   Ipp16s siPreemph;
   Ipp16s asiSpeechOld[SPEECH_SIZE - FRAME_SIZE + WBP_OFFSET];
   Ipp16s asiWspOld[PITCH_LAG_MAX / OPL_DECIM];
   Ipp16s asiExcOld[PITCH_LAG_MAX + INTERPOL_LEN];
   Ipp16s asiLevinsonMem[2*LP_ORDER];
   Ipp16s asiIspOld[LP_ORDER];
   Ipp16s asiIspQuantOld[LP_ORDER];
   Ipp16s asiIsfQuantPast[LP_ORDER];
   Ipp16s siWsp;
   Ipp16s asiWspDecimate[3];
   Ipp16s siSpeechWgt;
   Ipp16s asiSynthesis[LP_ORDER];
   Ipp16s siTiltCode;
   Ipp16s siWspOldMax;
   Ipp16s siWspOldShift;
   Ipp16s siScaleFactorOld;
   Ipp16s asiScaleFactorMax[2];
   Ipp16s asiGainPitchClip[2];
   Ipp16s asiGainQuant[4];
   Ipp16s siMedianOld;
   Ipp16s siOpenLoopGain;
   Ipp16s siAdaptiveParam;
   Ipp16s siWeightFlag;
   Ipp16s asiPitchLagOld[5];
   IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateWsp;
   Ipp16s asiHypassFiltWspOld[FRAME_SIZE / OPL_DECIM + (PITCH_LAG_MAX / OPL_DECIM)];
   IppsVADState_AMRWB_16s *pSVadState;
   Ipp32s iDtx;
   SDtxEncoderState dtxEncState;
   Ipp16s siFrameFirst;
   Ipp16s asiIsfOld[LP_ORDER];
   Ipp32s iNoiseEnhancerThres;
   Ipp32s aiSynthMem[LP_ORDER];
   Ipp16s siDeemph;
   Ipp16s asiSynthHighFilt[LP_ORDER];
   HighPassFIRState_AMRWB_16s_ISfs *pSHPFIRState;
   HighPassFIRState_AMRWB_16s_ISfs *pSHPFIRState2;
   IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnlIn;
   IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnlOut;
   IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnl400;
   Ipp16s siHFSeed;
   Ipp16s siVadHist;
   Ipp16s siAlphaGain;
   Ipp16s siScaleExp;
   Ipp16s siToneFlag;
};

struct _AMRWBDecoder_Obj
{
    SAMRWBCoderObj objPrm;
    Ipp16s asiExcOld[PITCH_LAG_MAX + INTERPOL_LEN];
    Ipp16s asiIspOld[LP_ORDER];
    Ipp16s asiIsfOld[LP_ORDER];
    Ipp16s asiIsf[3*LP_ORDER];
    Ipp16s asiIsfQuantPast[LP_ORDER];
    Ipp16s siTiltCode;
    Ipp16s siScaleFactorOld;
    Ipp16s asiScaleFactorMax[4];
    Ipp32s iNoiseEnhancerThres;
    Ipp32s asiSynthesis[LP_ORDER];
    Ipp16s siDeemph;
    Ipp16s asiOversampFilt[2*UP_SAMPL_FILT_DELAY];
    Ipp16s asiSynthHighFilt[LP_ORDER_16K];
    HighPassFIRState_AMRWB_16s_ISfs *pSHighPassFIRState;
    HighPassFIRState_AMRWB_16s_ISfs *pSHighPassFIRState2;
    IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnlOut;
    IppsHighPassFilterState_AMRWB_16s *pSHPFiltStateSgnl400;
    Ipp16s siSeed;
    Ipp16s siHFSeed;
    Ipp16s asiQuantEnerPast[4];
    Ipp16s asiCodeGainPast[5];
    Ipp16s asiQuantGainPast[5];
    Ipp16s siCodeGainPrev;
    Ipp16s asiPhaseDisp[8];
    Ipp16s siBfiPrev;
    Ipp16s siBFHState;
    Ipp16s siFrameFirst;
    SDtxDecoderState dtxDecState;
    Ipp16s siVadHist;
    IppsAdaptiveCodebookDecodeState_AMRWB_16s *pSAdaptCdbkDecState;

    /* extention */
    Ipp16s siOldPitchLag; /* old pitch lag    { moved from pAdaptCdbkDecState for WB<->WB+} */
    Ipp16s siOldPitchLagFrac;

    Ipp16s mem_syn_out[PITCH_LAG_MAX + SUBFRAME_SIZE];
    Ipp16s mem_oversamp_hf_plus[2 * UP_SAMPL_FILT_DELAY];
    Ipp16s mem_syn_hf_plus[8];
    Ipp16s lpc_hf_plus[9];
    Ipp16s gain_hf_plus;
    Ipp32s threshold_hf;
    Ipp32s lp_amp_hf;
    Ipp16s ramp_state;
};

#define   AMRWB_CODECFUN(type,name,arg)                extern type name arg

Ipp16s ownDTXEncReset(SDtxEncoderState *st, Ipp16s *pIsfInit);
Ipp16s ownDTXEnc(SDtxEncoderState *st, Ipp16s *pIsfvec, Ipp16s *pExc2vec, Ipp16u *pPrmsvec);
Ipp16s ownDTXDecReset(SDtxDecoderState * st, Ipp16s *pIsfInit);
Ipp16s ownDTXDec(SDtxDecoderState *st, Ipp16s *pExc2vec, Ipp16s valDTXState, Ipp16s *pIsfvec,
                 const Ipp16u *pPrmsvec);
Ipp16s ownRXDTXHandler(SDtxDecoderState * st, Ipp16s frameType);

/********************************************************
*      auxiliary inline functions declarations
*********************************************************/
__INLINE void ownMulC_16s_ISfs(Ipp16s val, Ipp16s *pSrcDst, Ipp32s len, Ipp32s scaleFactor)
{
    Ipp32s i;
    for (i = 0; i < len; i++)
    {
        pSrcDst[i] = (Ipp16s)((pSrcDst[i] * val) >> scaleFactor);
    }
}

__INLINE void ownMulC_16s_Sfs(Ipp16s *pSrc, Ipp16s val, Ipp16s *pDst, Ipp32s len, Ipp32s scaleFactor)
{
    Ipp32s i;
    for (i = 0; i < len; i++)
    {
        pDst[i] = (Ipp16s)((pSrc[i] * val) >> scaleFactor);
    }
}

#include "aux_fnxs.h"

#endif /* __OWNAMRWB_H__ */
