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

#ifndef __OWNEVS_H__
#define __OWNEVS_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include <ippsc.h>
#include "evsapi.h"
#include "scratchmem.h"


/* Codec constant parameters (coder and decoder) */

#define CODEC_VERSION "13.1.0"
#if(0)
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
#endif

#define   EVS_CODECFUN(type,name,arg)                extern type name arg


#include "aux_fnxs.h"

#endif /* __OWNAMRWB_H__ */
