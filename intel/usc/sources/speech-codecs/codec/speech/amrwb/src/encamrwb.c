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
// Purpose: AMRWB speech codec encoder API functions.
//
*/

#include "ownamrwb.h"

static Ipp16s ownSynthesis(Ipp16s *pLPCQuantvec, Ipp16s *pExc, Ipp16s valQNew,
                          const Ipp16u *pSynthSignal, AMRWBEncoder_Obj *st);
static void ownPrms2Bits(const Ipp16s* pPrms, Ipp8u *pBitstream, AMRWB_Rate_t rate);

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_GetSize,
         (AMRWBEncoder_Obj* encoderObj, Ipp32u *pCodecSize))
{
   if(NULL == encoderObj)
      return APIAMRWB_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIAMRWB_StsBadArgErr;
   if(encoderObj->objPrm.iKey != ENC_KEY)
      return APIAMRWB_StsNotInitialized;

   *pCodecSize = encoderObj->objPrm.iObjSize;
   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_Alloc,
         (const AMRWBEnc_Params *amrwb_Params, Ipp32u *pCodecSize))
{
   Ipp32s hpfltsize;
   if(NULL == amrwb_Params)
      return APIAMRWB_StsBadArgErr;
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize); // aligned by 8
   *pCodecSize = sizeof(AMRWBEncoder_Obj); // aligned by 8
   *pCodecSize += (2*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize); // aligned by 8
   *pCodecSize += (3*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(3, &hpfltsize); // aligned by 8
   *pCodecSize += hpfltsize;
   ippsVADGetSize_AMRWB_16s(&hpfltsize); // not aligned by 8
   *pCodecSize += (hpfltsize+7)&(~7);

   return APIAMRWB_StsNoErr;
}

static void InitEncoder(AMRWBEncoder_Obj* encoderObj)
{
   ippsZero_16s(encoderObj->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);
   ippsZero_16s(encoderObj->asiSynthesis, LP_ORDER);
   ippsZero_16s(encoderObj->asiIsfQuantPast, LP_ORDER);

   encoderObj->siSpeechWgt = 0;
   encoderObj->siTiltCode = 0;
   encoderObj->siFrameFirst = 1;

   encoderObj->asiGainPitchClip[0] = MAX_DISTANCE_ISF;
   encoderObj->asiGainPitchClip[1] = MIN_GAIN_PITCH;

   encoderObj->iNoiseEnhancerThres = 0;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_Init,
         (AMRWBEncoder_Obj* encoderObj, Ipp32s mode))
{
   Ipp32s hpfltsize;

   InitEncoder(encoderObj);

   encoderObj->pSHPFIRState = (HighPassFIRState_AMRWB_16s_ISfs *)((Ipp8s*)encoderObj + sizeof(AMRWBEncoder_Obj));
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   encoderObj->pSHPFIRState2 = (HighPassFIRState_AMRWB_16s_ISfs *)((Ipp8s*)encoderObj->pSHPFIRState + hpfltsize);
   encoderObj->pSHPFiltStateSgnlIn = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)encoderObj->pSHPFIRState2 + hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize);
   encoderObj->pSHPFiltStateSgnlOut = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)encoderObj->pSHPFiltStateSgnlIn + hpfltsize);
   encoderObj->pSHPFiltStateSgnl400 = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)encoderObj->pSHPFiltStateSgnlOut + hpfltsize);
   encoderObj->pSHPFiltStateWsp = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)encoderObj->pSHPFiltStateSgnl400 + hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(3, &hpfltsize);
   encoderObj->pSVadState = (IppsVADState_AMRWB_16s *)((Ipp8s*)encoderObj->pSHPFiltStateWsp + hpfltsize);

   /* Static vectors to zero */

   ippsZero_16s(encoderObj->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE + WBP_OFFSET);
   ippsZero_16s(encoderObj->asiWspOld, (PITCH_LAG_MAX / OPL_DECIM));
   ippsZero_16s(encoderObj->asiWspDecimate, 3);

   /* routines initialization */

   ippsZero_16s(encoderObj->asiSpeechDecimate, 2 * NB_COEF_DOWN);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffHP50Tbl, (Ipp16s*)BCoeffHP50Tbl, 2, encoderObj->pSHPFiltStateSgnlIn);
   ippsZero_16s(encoderObj->asiLevinsonMem, 32);
   ippsSet_16s(-14336, encoderObj->asiGainQuant, PRED_ORDER);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffTbl, (Ipp16s*)BCoeffTbl, 3, encoderObj->pSHPFiltStateWsp);

   /* isp initialization */

   ippsCopy_16s(IspInitTbl, encoderObj->asiIspOld, LP_ORDER);
   ippsCopy_16s(IspInitTbl, encoderObj->asiIspQuantOld, LP_ORDER);

   /* variable initialization */

   encoderObj->siPreemph = 0;
   encoderObj->siWsp = 0;
   encoderObj->siScaleFactorOld = 15;
   encoderObj->asiScaleFactorMax[0] = 15;
   encoderObj->asiScaleFactorMax[1] = 15;
   encoderObj->siWspOldMax = 0;
   encoderObj->siWspOldShift = 0;

   /* pitch ol initialization */

   encoderObj->siMedianOld = 40;
   encoderObj->siOpenLoopGain = 0;
   encoderObj->siAdaptiveParam = 0;
   encoderObj->siWeightFlag = 0;
   ippsSet_16s(40, encoderObj->asiPitchLagOld, 5);
   ippsZero_16s(encoderObj->asiHypassFiltWspOld, (FRAME_SIZE / 2) / OPL_DECIM + (PITCH_LAG_MAX / OPL_DECIM));

   ippsZero_16s(encoderObj->asiSynthHighFilt, LP_ORDER);
   ippsZero_16s((Ipp16s*)encoderObj->aiSynthMem, LP_ORDER * 2);

   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffHP50Tbl, (Ipp16s*)BCoeffHP50Tbl, 2, encoderObj->pSHPFiltStateSgnlOut);
   HighPassFIRInit_AMRWB_16s_ISfs((Ipp16s*)Fir6k_7kTbl, 2, encoderObj->pSHPFIRState);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffHP400Tbl, (Ipp16s*)BCoeffHP400Tbl, 2, encoderObj->pSHPFiltStateSgnl400);

   ippsCopy_16s(IsfInitTbl, encoderObj->asiIsfOld, LP_ORDER);

   encoderObj->siDeemph = 0;

   encoderObj->siHFSeed = RND_INIT;

   HighPassFIRInit_AMRWB_16s_ISfs((Ipp16s*)Fir6k_7kTbl, 2, encoderObj->pSHPFIRState2);
   encoderObj->siAlphaGain = IPP_MAX_16S;

   encoderObj->siVadHist = 0;
   encoderObj->iDtx = mode;

   ippsVADInit_AMRWB_16s(encoderObj->pSVadState);
   encoderObj->siToneFlag = 0;
   ownDTXEncReset(&encoderObj->dtxEncState, (Ipp16s*)IsfInitTbl);
   encoderObj->siScaleExp = 0;

   
   encoderObj->objPrm.iKey = ENC_KEY;

   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoder_Mode,
               (AMRWBEncoder_Obj* encoderObj, Ipp32s mode)) {
    if(encoderObj->iDtx != mode) encoderObj->iDtx = mode;
    return APIAMRWB_StsNoErr;
}
#if(0) /* never used. Removed to improve BullsEye Coverage*/
AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncoderProceedFirst,
         (AMRWBEncoder_Obj* pEncObj, const Ipp16s* pSrc))
{
   Ipp16s error[LP_ORDER + SUBFRAME_SIZE]; /* error of quantization                  */
   Ipp16s code[SUBFRAME_SIZE];             /* Fixed codebook excitation              */
   Ipp16s tmp;
   Ipp32s dwTmp;
   Ipp16s *new_speech;                     /* Speech vector                          */

   new_speech = pEncObj->asiSpeechOld + SPEECH_SIZE - 2*FRAME_SIZE - UP_SAMPL_FILT_DELAY + WBP_OFFSET;
   ippsZero_16s(pEncObj->asiSpeechDecimate, 2*DOWN_SAMPL_FILT_DELAY);
   /* Down sampling signal from 16kHz to 12.8kHz */
   ownDecimation_AMRWB_16s((Ipp16s*)pSrc, DOWN_SAMPL_FILT_DELAY, new_speech, pEncObj->asiSpeechDecimate);
   /* decimate with zero-padding to avoid delay of filter */
   ippsCopy_16s(pEncObj->asiSpeechDecimate, code, 2 * DOWN_SAMPL_FILT_DELAY);
   ippsZero_16s(error, DOWN_SAMPL_FILT_DELAY);
   ownDecimation_AMRWB_16s(error, DOWN_SAMPL_FILT_DELAY, new_speech + FRAME_SIZE, code);
   /*
    * Perform 50Hz HP filtering of input signal.
    * Perform fixed preemphasis through 1 - g z^-1
    */
   ippsHighPassFilter_AMRWB_16s_ISfs(new_speech, FRAME_SIZE, pEncObj->pSHPFiltStateSgnlIn, 14);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &dwTmp);
   ippsCopy_8u((const Ipp8u*)pEncObj->pSHPFiltStateSgnlIn, (Ipp8u*)code, dwTmp);
   ippsHighPassFilter_AMRWB_16s_ISfs(new_speech+FRAME_SIZE, DOWN_SAMPL_FILT_DELAY,
      (IppsHighPassFilterState_AMRWB_16s *)code, 14);
   ippsPreemphasize_AMRWB_16s_ISfs(PREEMPH_FACTOR, new_speech, FRAME_SIZE, 14, &(pEncObj->siPreemph));
   /* last L_FILT samples for autocorrelation window */
   tmp = pEncObj->siPreemph;
   ippsPreemphasize_AMRWB_16s_ISfs(PREEMPH_FACTOR, (new_speech+FRAME_SIZE), DOWN_SAMPL_FILT_DELAY, 14, &tmp);
   return APIAMRWB_StsNoErr;
}
#endif /* never used. Removed to improve BullsEye Coverage*/

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBEncode,
         (AMRWBEncoder_Obj* encoderObj, const Ipp16u* src,
          Ipp8u* dst, AMRWB_Rate_t *rate, Ipp32s Vad, Ipp32s offsetWBE ))
{
    /* Coder states */
    AMRWBEncoder_Obj *st;

    /* Speech vector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pSpeechOldvec, SPEECH_SIZE + WBP_OFFSET);
    Ipp16s *pSpeechNew, *pSpeech, *pWindow;

    /* Weighted speech vector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pWgtSpchOldvec, FRAME_SIZE + (PITCH_LAG_MAX / OPL_DECIM));
    Ipp16s *pWgtSpch;

    /* Excitation vector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pExcOldvec, (FRAME_SIZE + 1) + PITCH_LAG_MAX + INTERPOL_LEN);
    Ipp16s *pExc;

    /* LPC coefficients */

    IPP_ALIGNED_ARRAY(16, Ipp32s, pAutoCorrvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pRCvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIspvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIspQuantvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIsfvec, LP_ORDER);
    Ipp16s *pLPCUnQuant, *pLPCQuant;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCUnQuantvec, NUMBER_SUBFRAME*(LP_ORDER+1));
    IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCQuantvec, NUMBER_SUBFRAME*(LP_ORDER+1));

    /* Other vectors */

    IPP_ALIGNED_ARRAY(16, Ipp16s, pAdptTgtPitch, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pAdptTgtCdbk, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pCorrvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pResidvec, SUBFRAME_SIZE);

    IPP_ALIGNED_ARRAY(16, Ipp16s, pImpRespPitchvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pImpRespCdbkvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFixCdbkvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFixCdbkExcvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFltAdptvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFltAdpt2vec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pErrQuant, LP_ORDER + SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pSynthvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pExc2vec, FRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pVadvec, FRAME_SIZE);

    /* Scalars */

    Ipp16s valSubFrame, valSelect, valClipFlag, valVadFlag;
    Ipp16s valPitchLag, valFracPitchLag;
    Ipp16s pOpenLoopLag[2],pPitchLagBounds[2];
    Ipp16s valPitchGain, valScaleCodeGain, pGainCoeff[4], pGainCoeff2[4];
    Ipp16s tmp, valGainCoeff, valAdptGain, valExp, valQNew, valShift, max;
    Ipp16s valVoiceFac;

    Ipp32s i, z, valCodeGain, valMax;

    Ipp16s valStabFac = 0, valFac, valCodeGainLow;
    Ipp16s pSrcCoeff[2];

    Ipp16s valCorrGain;
    Ipp32s valSize, subFrame;
    IppSpchBitRate irate = Mode2RateTbl[*rate];
    IppSpchBitRate valCodecMode = irate;
    Ipp16s *pHPWgtSpch;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pPrmvec, MAX_PARAM_SIZE);
    Ipp16s *pPrm;

    if (offsetWBE == 0)
       pPrm = pPrmvec;
    else
       pPrm = (Ipp16s*)dst;

    st = encoderObj;

    pSpeechNew = pSpeechOldvec + SPEECH_SIZE - FRAME_SIZE - UP_SAMPL_FILT_DELAY + offsetWBE;
    pSpeech = pSpeechOldvec + SPEECH_SIZE - FRAME_SIZE - SUBFRAME_SIZE;
    pWindow = pSpeechOldvec + SPEECH_SIZE - WINDOW_SIZE;

    pExc = pExcOldvec + PITCH_LAG_MAX + INTERPOL_LEN;
    pWgtSpch = pWgtSpchOldvec + (PITCH_LAG_MAX / OPL_DECIM);

    /* copy coder memory state into working space (internal memory for DSP) */

    ippsCopy_16s(st->asiSpeechOld, pSpeechOldvec, SPEECH_SIZE - FRAME_SIZE + offsetWBE);
    ippsCopy_16s(st->asiWspOld, pWgtSpchOldvec, PITCH_LAG_MAX / OPL_DECIM);
    ippsCopy_16s(st->asiExcOld, pExcOldvec, PITCH_LAG_MAX + INTERPOL_LEN);

    /* Down sampling signal from 16kHz to 12.8kHz */

    ownDecimation_AMRWB_16s((Ipp16s*)src, L_FRAME16k, pSpeechNew, st->asiSpeechDecimate);

    /* last UP_SAMPL_FILT_DELAY samples for autocorrelation window */
    ippsCopy_16s(st->asiSpeechDecimate, pFixCdbkvec, 2*DOWN_SAMPL_FILT_DELAY);
    ippsZero_16s(pErrQuant, DOWN_SAMPL_FILT_DELAY);            /* set next sample to zero */
    ownDecimation_AMRWB_16s(pErrQuant, DOWN_SAMPL_FILT_DELAY, pSpeechNew + FRAME_SIZE, pFixCdbkvec);

    /* Perform 50Hz HP filtering of input signal */

    ippsHighPassFilter_AMRWB_16s_ISfs(pSpeechNew, FRAME_SIZE, st->pSHPFiltStateSgnlIn, 14);

    /* last UP_SAMPL_FILT_DELAY samples for autocorrelation window */
    ippsHighPassFilterGetSize_AMRWB_16s(2, &valSize);
    ippsCopy_8u((const Ipp8u*)st->pSHPFiltStateSgnlIn, (Ipp8u*)pFixCdbkvec, valSize);
    ippsHighPassFilter_AMRWB_16s_ISfs(pSpeechNew + FRAME_SIZE, UP_SAMPL_FILT_DELAY, (IppsHighPassFilterState_AMRWB_16s *)pFixCdbkvec, 14);

    /* get max of new preemphased samples (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    z = pSpeechNew[0] << 15;
    z -= st->siPreemph * PREEMPH_FACTOR;
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

    if (valQNew > st->asiScaleFactorMax[0]) valQNew = st->asiScaleFactorMax[0];
    if (valQNew > st->asiScaleFactorMax[1]) valQNew = st->asiScaleFactorMax[1];
    valExp = (Ipp16s)(valQNew - st->siScaleFactorOld);
    st->siScaleFactorOld = valQNew;
    st->asiScaleFactorMax[1] = st->asiScaleFactorMax[0];
    st->asiScaleFactorMax[0] = valShift;

    /* preemphasis with scaling (FRAME_SIZE+UP_SAMPL_FILT_DELAY) */

    tmp = pSpeechNew[FRAME_SIZE - 1];

    ippsPreemphasize_AMRWB_16s_ISfs(PREEMPH_FACTOR, pSpeechNew, FRAME_SIZE + UP_SAMPL_FILT_DELAY, 15-valQNew, &st->siPreemph);

    st->siPreemph = tmp;

    /* scale previous samples and memory */

    ownScaleSignal_AMRWB_16s_ISfs(pSpeechOldvec, SPEECH_SIZE - FRAME_SIZE - UP_SAMPL_FILT_DELAY + offsetWBE, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(pExcOldvec, PITCH_LAG_MAX + INTERPOL_LEN, valExp);

    ownScaleSignal_AMRWB_16s_ISfs(st->asiSynthesis, LP_ORDER, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(st->asiWspDecimate, 3, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(&(st->siWsp), 1, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(&(st->siSpeechWgt), 1, valExp);

     /*  Call VAD */
    if (offsetWBE == 0) {
       ippsCopy_16s(pSpeechNew, pVadvec, FRAME_SIZE);
    } else {
       ippsCopy_16s(pSpeech + SUBFRAME_SIZE - UP_SAMPL_FILT_DELAY, pVadvec, FRAME_SIZE);
    }

    ownScaleSignal_AMRWB_16s_ISfs(pVadvec, FRAME_SIZE, (Ipp16s)(1 - valQNew));
    ippsVAD_AMRWB_16s(pVadvec, st->pSVadState, &encoderObj->siToneFlag, &valVadFlag);
    if (valVadFlag == 0)
        st->siVadHist += 1;
    else
        st->siVadHist = 0;

    /* DTX processing */

    if (Vad != 0)
    {
       Ipp16s foo;
       Ipp16s dtxMode = ~IPP_SPCHBR_DTX;
       ippsEncDTXHandler_GSMAMR_16s(&st->dtxEncState.siHangoverCount,
                  &st->dtxEncState.siAnaElapsedCount, &dtxMode, &foo, valVadFlag);
       if(dtxMode==IPP_SPCHBR_DTX) {
          *rate = AMRWB_RATE_DTX;
          irate = IPP_SPCHBR_DTX;
       }
    }

    if (*rate != AMRWB_RATE_DTX)
    {
        *(pPrm)++ = valVadFlag;
    }
     /* Perform LPC analysis */

    ownAutoCorr_AMRWB_16s32s(pWindow, LP_ORDER, pAutoCorrvec, LPWindowTbl, WINDOW_SIZE);
    ownLagWindow_AMRWB_32s_I(pAutoCorrvec, LP_ORDER);
    if(ippsLevinsonDurbin_G729_32s16s(pAutoCorrvec, LP_ORDER, pLPCUnQuantvec, pRCvec, &tmp) == ippStsOverflow){
         pLPCUnQuantvec[0] = 4096;
         ippsCopy_16s(st->asiLevinsonMem, &pLPCUnQuantvec[1], LP_ORDER);
         pRCvec[0] = st->asiLevinsonMem[LP_ORDER]; /* only two pRCvec coefficients are needed */
         pRCvec[1] = st->asiLevinsonMem[LP_ORDER+1];
    }else{
         ippsCopy_16s(pLPCUnQuantvec, st->asiLevinsonMem, LP_ORDER+1);
         st->asiLevinsonMem[LP_ORDER] = pRCvec[0];
         st->asiLevinsonMem[LP_ORDER+1] = pRCvec[1];
    }
    ippsLPCToISP_AMRWB_16s(pLPCUnQuantvec, pIspvec, st->asiIspOld);

    /* Find the interpolated ISPs and convert to pLPCUnQuantvec for all subframes */
    {
      IPP_ALIGNED_ARRAY(16, Ipp16s, isp, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld, 18022, pIspvec,14746, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, pLPCUnQuantvec, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  6554, pIspvec,26214, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCUnQuantvec[LP_ORDER + 1], LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  1311, pIspvec,31457, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCUnQuantvec[2*(LP_ORDER + 1)], LP_ORDER);

      /* 4th subframe: pIspvec (frac=1.0) */

      ippsISPToLPC_AMRWB_16s(pIspvec, &pLPCUnQuantvec[3*(LP_ORDER + 1)], LP_ORDER);

      /* update asiIspOld[] for the next frame */
      ippsCopy_16s(pIspvec, st->asiIspOld, LP_ORDER);
    }

    /* Convert ISPs to frequency domain 0..6400 */
    ippsISPToISF_Norm_AMRWB_16s(pIspvec, pIsfvec, LP_ORDER);

    /* check resonance for pitch clipping algorithm */
    ownCheckGpClipIsf(irate, pIsfvec, st->asiGainPitchClip);

     /* Perform PITCH_OL analysis */

    pLPCUnQuant = pLPCUnQuantvec;
    for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
    {
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        //ippsResidualFilter_AMRWB_16s_Sfs(pLPCvec, LP_ORDER, &pSpeech[valSubFrame], &pWgtSpch[valSubFrame], SUBFRAME_SIZE,10);
        ippsResidualFilter_Low_16s_Sfs(pLPCvec, LP_ORDER, &pSpeech[valSubFrame], &pWgtSpch[valSubFrame], SUBFRAME_SIZE, 11);
        pLPCUnQuant += (LP_ORDER + 1);
    }
    ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pWgtSpch, FRAME_SIZE, &(st->siWsp));

    /* find maximum value on pWgtSpch[] for 12 bits scaling */
    ippsMinMax_16s(pWgtSpch, FRAME_SIZE, &tmp, &max);

    max = Abs_16s(max);
    tmp = Abs_16s(tmp);

    if(tmp > max) max = tmp;
    tmp = st->siWspOldMax;

    if (max > tmp) tmp = max;
    st->siWspOldMax = max;

    valShift = (Ipp16s)(Exp_16s(tmp) - 3);
    if (valShift > 0) valShift = 0;               /* valShift = 0..-3 */

    /* decimation of pWgtSpch[] to search pitch in LF and to reduce complexity */
    ownLPDecim2(pWgtSpch, FRAME_SIZE, st->asiWspDecimate);

    /* scale pWgtSpch[] in 12 bits to avoid overflow */
    ownScaleSignal_AMRWB_16s_ISfs(pWgtSpch, FRAME_SIZE / OPL_DECIM, valShift);

    /* scale asiWspOld */
    valExp = (Ipp16s)(valExp + (valShift - st->siWspOldShift));
    st->siWspOldShift = valShift;
    ownScaleSignal_AMRWB_16s_ISfs(pWgtSpchOldvec, PITCH_LAG_MAX / OPL_DECIM, valExp);
    ownScaleSignal_AMRWB_16s_ISfs(st->asiHypassFiltWspOld, PITCH_LAG_MAX / OPL_DECIM, valExp);
    st->siScaleExp = (Ipp16s)(st->siScaleExp - valExp);

    /* Find open loop pitch lag for whole speech frame */

    pHPWgtSpch = st->asiHypassFiltWspOld + PITCH_LAG_MAX / OPL_DECIM;

    if(irate == IPP_SPCHBR_6600) {
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch, pHPWgtSpch, (FRAME_SIZE) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch, pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[0],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE) / OPL_DECIM);
       ippsMove_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
       pOpenLoopLag[1] = pOpenLoopLag[0];
    } else {
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch, pHPWgtSpch, (FRAME_SIZE / 2) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch, pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[0],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE / 2) / OPL_DECIM);
       ippsMove_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE / 2) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
       ippsHighPassFilter_AMRWB_16s_Sfs(pWgtSpch + ((FRAME_SIZE / 2) / OPL_DECIM), pHPWgtSpch,
                              (FRAME_SIZE / 2) / OPL_DECIM, st->pSHPFiltStateWsp,st->siScaleExp);
       ippsOpenLoopPitchSearch_AMRWB_16s(pWgtSpch + ((FRAME_SIZE / 2) / OPL_DECIM), pHPWgtSpch, &st->siMedianOld, &st->siAdaptiveParam, &pOpenLoopLag[1],
                              &encoderObj->siToneFlag, &st->siOpenLoopGain,
                              st->asiPitchLagOld, &st->siWeightFlag, (FRAME_SIZE / 2) / OPL_DECIM);
       ippsMove_16s(&st->asiHypassFiltWspOld[(FRAME_SIZE / 2) / OPL_DECIM],st->asiHypassFiltWspOld,PITCH_LAG_MAX / OPL_DECIM);
    }

    if (irate == IPP_SPCHBR_DTX)            /* CNG mode */
    {
        //ippsResidualFilter_AMRWB_16s_Sfs(&pLPCUnQuantvec[3 * (LP_ORDER + 1)], LP_ORDER, pSpeech, pExc, FRAME_SIZE,10);
        ippsResidualFilter_Low_16s_Sfs(&pLPCUnQuantvec[3 * (LP_ORDER + 1)], LP_ORDER, pSpeech, pExc, FRAME_SIZE, 11);
        ippsRShiftC_16s(pExc, valQNew, pExc2vec, FRAME_SIZE);
        ippsEncDTXBuffer_AMRWB_16s(pExc2vec, pIsfvec, &st->dtxEncState.siHistPtr,
                  st->dtxEncState.asiIsfHistory, st->dtxEncState.siLogEnerHist, valCodecMode);
        ownDTXEnc(&st->dtxEncState, pIsfvec, pExc2vec, (Ipp16u*)pPrm);

        /* Convert ISFs to the cosine domain */
        ippsISFToISP_AMRWB_16s(pIsfvec, pIspQuantvec, LP_ORDER);
        ippsISPToLPC_AMRWB_16s(pIspQuantvec, pLPCQuantvec, LP_ORDER);

        for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
        {
            ownSynthesis(pLPCQuantvec, &pExc2vec[valSubFrame], 0, &src[valSubFrame * 5 / 4], st);
        }
        ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);
        InitEncoder(encoderObj);

        if (offsetWBE == 0)
           ownPrms2Bits(pPrmvec,dst,AMRWB_RATE_DTX);

        ippsCopy_16s(&pSpeechOldvec[FRAME_SIZE], st->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE + offsetWBE);
        ippsCopy_16s(&pWgtSpchOldvec[FRAME_SIZE / OPL_DECIM], st->asiWspOld, PITCH_LAG_MAX / OPL_DECIM);

        return APIAMRWB_StsNoErr;
    }

    ippsISFQuant_AMRWB_16s(pIsfvec, st->asiIsfQuantPast, pIsfvec, (Ipp16s*)pPrm, irate);

    if (irate == IPP_SPCHBR_6600)
        pPrm += 5;
    else
        pPrm += 7;

    /* Check stability on pIsfvec : distance between old pIsfvec and current isf */
    if (irate == IPP_SPCHBR_23850 || offsetWBE!=0) {
        valStabFac = ownChkStab(pIsfvec, st->asiIsfOld, LP_ORDER-1);
    }
    ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);

    /* Convert ISFs to the cosine domain */
    ippsISFToISP_AMRWB_16s(pIsfvec, pIspQuantvec, LP_ORDER);

    if (st->siFrameFirst != 0)
    {
        st->siFrameFirst = 0;
        ippsCopy_16s(pIspQuantvec, st->asiIspQuantOld, LP_ORDER);
    }

    /* Find the interpolated ISPs and convert to pIspQuantvec[] for all subframes */

    {
      IPP_ALIGNED_ARRAY( 16, Ipp16s, isp, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld, 18022, pIspQuantvec,14746, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, pLPCQuantvec, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld,  6554, pIspQuantvec,26214, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCQuantvec[LP_ORDER + 1], LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspQuantOld,  1311, pIspQuantvec,31457, isp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(isp, &pLPCQuantvec[2*(LP_ORDER + 1)], LP_ORDER);

      /* 4th subframe: pIspQuantvec (frac=1.0) */

      ippsISPToLPC_AMRWB_16s(pIspQuantvec, &pLPCQuantvec[3*(LP_ORDER + 1)], LP_ORDER);

      ippsCopy_16s(pIspQuantvec, st->asiIspQuantOld, LP_ORDER);
    }

    pLPCQuant = pLPCQuantvec;
    for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
    {
        //ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE,10);
        ippsResidualFilter_Low_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE, 11);
        pLPCQuant += (LP_ORDER + 1);
    }

    /* Buffer isf's and energy for dtx on non-speech frame */

    if (valVadFlag == 0)
    {
        ippsRShiftC_16s(pExc, valQNew, pExc2vec, FRAME_SIZE);
        ippsEncDTXBuffer_AMRWB_16s(pExc2vec, pIsfvec, &st->dtxEncState.siHistPtr,
                  st->dtxEncState.asiIsfHistory, st->dtxEncState.siLogEnerHist, valCodecMode);
    }

    /* Loop for every subframe in the analysis frame */

    pLPCUnQuant = pLPCUnQuantvec;
    pLPCQuant = pLPCQuantvec;

    for (valSubFrame = 0,subFrame=0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE, subFrame++)
    {
        ippsSub_16s(st->asiSynthesis, &pSpeech[valSubFrame - LP_ORDER], pErrQuant, LP_ORDER);
        //ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE,10);
        ippsResidualFilter_Low_16s_Sfs(pLPCQuant, LP_ORDER, &pSpeech[valSubFrame], &pExc[valSubFrame], SUBFRAME_SIZE, 11);
        {
           Ipp16s tmpLPCQuant;
           tmpLPCQuant = pLPCQuant[0];
           pLPCQuant[0] >>= 1;
           ippsSynthesisFilter_G729E_16s(pLPCQuant, LP_ORDER,  &pExc[valSubFrame], pErrQuant + LP_ORDER, SUBFRAME_SIZE, pErrQuant);
           pLPCQuant[0] = tmpLPCQuant;
        }

        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        //ippsResidualFilter_AMRWB_16s_Sfs(pLPCvec, LP_ORDER, pErrQuant + LP_ORDER, pAdptTgtPitch, SUBFRAME_SIZE,10);
        ippsResidualFilter_Low_16s_Sfs(pLPCvec, LP_ORDER, pErrQuant + LP_ORDER, pAdptTgtPitch, SUBFRAME_SIZE, 11);

        ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pAdptTgtPitch, SUBFRAME_SIZE, &(st->siSpeechWgt));

        ippsZero_16s(pFixCdbkvec, LP_ORDER);
        ippsCopy_16s(pAdptTgtPitch, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2);

        tmp = 0;
        ippsPreemphasize_AMRWB_16s_ISfs (TILT_FACTOR, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2, 13, &tmp);
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pLPCvec, LP_ORDER+1, 15);
        {
           Ipp16s tmpLPCVec;
           tmpLPCVec = pLPCvec[0];
           pLPCvec[0] >>= 1;
           ippsSynthesisFilter_G729E_16s_I(pLPCvec, LP_ORDER, pFixCdbkvec + LP_ORDER, SUBFRAME_SIZE / 2, pFixCdbkvec);
           pLPCvec[0] = tmpLPCVec;
        }
        //ippsResidualFilter_AMRWB_16s_Sfs(pLPCQuant, LP_ORDER, pFixCdbkvec + LP_ORDER, pResidvec, SUBFRAME_SIZE / 2,10);
        ippsResidualFilter_Low_16s_Sfs(pLPCQuant, LP_ORDER, pFixCdbkvec + LP_ORDER, pResidvec, SUBFRAME_SIZE/2, 11);

        ippsCopy_16s(&pExc[valSubFrame + (SUBFRAME_SIZE / 2)], pResidvec + (SUBFRAME_SIZE / 2), SUBFRAME_SIZE / 2);

        ippsZero_16s(pErrQuant, LP_ORDER + SUBFRAME_SIZE);
        ippsMulPowerC_NR_16s_Sfs(pLPCUnQuant, WEIGHT_FACTOR, pErrQuant + LP_ORDER, LP_ORDER+1, 15);
        {
           Ipp16s tmpLPCQuant;
           tmpLPCQuant = pLPCQuant[0];
           pLPCQuant[0] = 16384;
           ippsSynthesisFilter_G729E_16s_I(pLPCQuant, LP_ORDER, pErrQuant + LP_ORDER, SUBFRAME_SIZE, pErrQuant);
           pLPCQuant[0] = tmpLPCQuant;
           ippsCopy_16s(pErrQuant + LP_ORDER,pImpRespPitchvec,SUBFRAME_SIZE);
        }
        tmp = 0;
        ippsDeemphasize_AMRWB_NR_16s_I(TILT_FACTOR, pImpRespPitchvec, SUBFRAME_SIZE, &tmp);

        ippsCopy_16s(pImpRespPitchvec, pImpRespCdbkvec, SUBFRAME_SIZE);
        ownScaleSignal_AMRWB_16s_ISfs(pImpRespCdbkvec, SUBFRAME_SIZE, -2);

        ownScaleSignal_AMRWB_16s_ISfs(pAdptTgtPitch, SUBFRAME_SIZE, valShift);
        ownScaleSignal_AMRWB_16s_ISfs(pImpRespPitchvec, SUBFRAME_SIZE, (Ipp16s)(1 + valShift));

        /* find closed loop fractional pitch  lag */
        ippsAdaptiveCodebookSearch_AMRWB_16s(pAdptTgtPitch, pImpRespPitchvec, pOpenLoopLag, &valPitchLag, pPitchLagBounds, &pExc[valSubFrame], &valFracPitchLag,
                                 (Ipp16s*)pPrm, subFrame, irate);
        pPrm += 1;

        /* Gain clipping test to avoid unstable synthesis on frame erasure */
        valClipFlag = ownGpClip(irate, st->asiGainPitchClip);

        if ((irate != IPP_SPCHBR_6600)&&(irate != IPP_SPCHBR_8850))
        {
            ippsConvPartial_NR_Low_16s(&pExc[valSubFrame], pImpRespPitchvec, pFltAdptvec, SUBFRAME_SIZE);
            ippsAdaptiveCodebookGainCoeff_AMRWB_16s(pAdptTgtPitch, pFltAdptvec, pGainCoeff, &valGainCoeff, SUBFRAME_SIZE);
            if ((valClipFlag != 0) && (valGainCoeff > PITCH_GAIN_CLIP))
                valGainCoeff = PITCH_GAIN_CLIP;
            ippsInterpolateC_G729_16s_Sfs(pAdptTgtPitch, 16384, pFltAdptvec,(Ipp16s)(-valGainCoeff), pCorrvec, SUBFRAME_SIZE, 14); /* pCorrvec used temporary */
        } else
        {
            valGainCoeff = 0;
        }

        /* find pitch excitation with lp filter */
        pSrcCoeff[0] = 20972;
        pSrcCoeff[1] = -5898;
        ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, &pExc[valSubFrame], pFixCdbkvec, SUBFRAME_SIZE, 1);

        //ippsConvPartial_NR_Low_16s(pFixCdbkvec, pImpRespPitchvec, pFltAdpt2vec, SUBFRAME_SIZE);
        ippsConvPartial_NR_16s(pFixCdbkvec, pImpRespPitchvec, pFltAdpt2vec, SUBFRAME_SIZE);
        ippsAdaptiveCodebookGainCoeff_AMRWB_16s(pAdptTgtPitch, pFltAdpt2vec, pGainCoeff2, &valAdptGain, SUBFRAME_SIZE);

        if ((valClipFlag != 0) && (valAdptGain > PITCH_GAIN_CLIP))
            valAdptGain = PITCH_GAIN_CLIP;
        ippsInterpolateC_G729_16s_Sfs(pAdptTgtPitch, 16384, pFltAdpt2vec,(Ipp16s)(-valAdptGain), pAdptTgtCdbk, SUBFRAME_SIZE, 14);

        valSelect = 0;
        if ((irate != IPP_SPCHBR_6600)&&(irate != IPP_SPCHBR_8850))
        {
           //Ipp32s valCorrSum, valCdbkSum;
           //ippsDotProd_16s32s_Sfs( pCorrvec, pCorrvec,SUBFRAME_SIZE, &valCorrSum, 0);
           //ippsDotProd_16s32s_Sfs( pAdptTgtCdbk, pAdptTgtCdbk,SUBFRAME_SIZE, &valCdbkSum, 0);
           //if (valCorrSum <= valCdbkSum) valSelect = 1;
           /*^^^^ overflows on new WB+ test sets ^^^^*/
           Ipp32s dwTmp = 0;
           for (i=0; i<SUBFRAME_SIZE; i++)
              dwTmp = Add_32s(dwTmp, (pCorrvec[i]*pCorrvec[i])<<1);
           for (i=0; i<SUBFRAME_SIZE; i++)
              dwTmp = Add_32s(dwTmp, Negate_32s((pAdptTgtCdbk[i]*pAdptTgtCdbk[i])<<1));
           if (dwTmp <= 0)
              valSelect = 1;
           *(pPrm)++ = valSelect;
        }

        if (valSelect == 0)
        {
            /* use the lp filter for pitch excitation prediction */
            valPitchGain = valAdptGain;
            ippsCopy_16s(pFixCdbkvec, &pExc[valSubFrame], SUBFRAME_SIZE);
            ippsCopy_16s(pFltAdpt2vec, pFltAdptvec, SUBFRAME_SIZE);
            ippsCopy_16s(pGainCoeff2, pGainCoeff, 4);
        } else
        {
            /* no filter used for pitch excitation prediction */
            valPitchGain = valGainCoeff;
            ippsCopy_16s(pCorrvec, pAdptTgtCdbk, SUBFRAME_SIZE);
        }

        ippsInterpolateC_G729_16s_Sfs(pResidvec, 16384, &pExc[valSubFrame],(Ipp16s)(-valPitchGain), pResidvec, SUBFRAME_SIZE, 14);
        ownScaleSignal_AMRWB_16s_ISfs(pResidvec, SUBFRAME_SIZE, valShift);

        tmp = 0;
        ippsPreemphasize_AMRWB_16s_ISfs (st->siTiltCode, pImpRespCdbkvec, SUBFRAME_SIZE, 14, &tmp);

        if (valFracPitchLag > 2) valPitchLag += 1;

        if (valPitchLag < SUBFRAME_SIZE) {
            ippsHarmonicFilter_NR_16s(PITCH_SHARP_FACTOR, valPitchLag, &pImpRespCdbkvec[valPitchLag], &pImpRespCdbkvec[valPitchLag], SUBFRAME_SIZE-valPitchLag);
        }

        ippsAlgebraicCodebookSearch_AMRWB_16s(pAdptTgtCdbk, pResidvec, pImpRespCdbkvec, pFixCdbkvec, pFltAdpt2vec, irate, (Ipp16s*)pPrm);

        if (irate == IPP_SPCHBR_6600)
        {
            pPrm += 1;
        } else if ((irate == IPP_SPCHBR_8850) || (irate == IPP_SPCHBR_12650) ||
            (irate == IPP_SPCHBR_14250) || (irate == IPP_SPCHBR_15850))
        {
            pPrm += 4;
        } else
        {
            pPrm += 8;
        }

        tmp = 0;
        ippsPreemphasize_AMRWB_16s_ISfs (st->siTiltCode, pFixCdbkvec, SUBFRAME_SIZE, 14,  &tmp);

        if (valPitchLag < SUBFRAME_SIZE) {
            ippsHarmonicFilter_NR_16s(PITCH_SHARP_FACTOR, valPitchLag, &pFixCdbkvec[valPitchLag], &pFixCdbkvec[valPitchLag], SUBFRAME_SIZE-valPitchLag);
        }

        ippsGainQuant_AMRWB_16s(pAdptTgtPitch, pFltAdptvec, (valQNew + valShift), pFixCdbkvec, pFltAdpt2vec, pGainCoeff,
                  st->asiGainQuant, &valPitchGain, &valCodeGain, valClipFlag, (Ipp16s*)pPrm, SUBFRAME_SIZE, irate);
        pPrm += 1;

        /* test quantized gain of pitch for pitch clipping algorithm */
        ownCheckGpClipPitchGain(irate, valPitchGain, st->asiGainPitchClip);

        z = ShiftL_32s(valCodeGain, valQNew);
        valScaleCodeGain = Cnvrt_NR_32s16s(z);

        /* find voice factor (1=voiced, -1=unvoiced) */

        ippsCopy_16s(&pExc[valSubFrame], pExc2vec, SUBFRAME_SIZE);
        ownScaleSignal_AMRWB_16s_ISfs(pExc2vec, SUBFRAME_SIZE, valShift);

        valVoiceFac = ownVoiceFactor(pExc2vec, valShift, valPitchGain, pFixCdbkvec, valScaleCodeGain, SUBFRAME_SIZE);

        /* tilt of code for next subframe: 0.5=voiced, 0=unvoiced */

        st->siTiltCode = (Ipp16s)((valVoiceFac >> 2) + 8192);

        z = Mul2_32s(valScaleCodeGain * pFltAdpt2vec[SUBFRAME_SIZE - 1]);
        if ((5+valShift) > 0)
           z = ShiftL_32s(z, (Ipp16u)(5 + valShift));
        else
           z >>= -(5+valShift);
        z = Negate_32s(z);
        z = Add_32s(z, Mul2_32s(pAdptTgtPitch[SUBFRAME_SIZE - 1] * 16384));
        z = Add_32s(z, Negate_32s(Mul2_32s(pFltAdptvec[SUBFRAME_SIZE - 1] * valPitchGain)));
        if ((1-valShift) > 0)
           z = ShiftL_32s(z, (Ipp16u)(1-valShift));
        else
           z >>= -(1 - valShift);
        st->siSpeechWgt = Cnvrt_NR_32s16s(z);

        if (irate == IPP_SPCHBR_23850)
            ippsCopy_16s(&pExc[valSubFrame], pExc2vec, SUBFRAME_SIZE);

        ippsInterpolateC_NR_16s(pFixCdbkvec, valScaleCodeGain, 7,
            &pExc[valSubFrame], valPitchGain, 2, &pExc[valSubFrame], SUBFRAME_SIZE);

        {
           Ipp16s tmpLPCQnt;
           tmpLPCQnt = pLPCQuant[0];
           pLPCQuant[0] >>= 1;
           ippsSynthesisFilter_G729E_16s(pLPCQuant, LP_ORDER,  &pExc[valSubFrame], pSynthvec, SUBFRAME_SIZE, st->asiSynthesis);
           ippsCopy_16s(&pSynthvec[SUBFRAME_SIZE-LP_ORDER],st->asiSynthesis,LP_ORDER);
           pLPCQuant[0] = tmpLPCQnt;
        }

        if (irate == IPP_SPCHBR_23850)
        {
            Unpack_32s(valCodeGain, &valScaleCodeGain, &valCodeGainLow);

            /* noise enhancer */
            tmp = (Ipp16s)(16384 - (valVoiceFac >> 1));        /* 1=unvoiced, 0=voiced */
            valFac = Mul_16s_Sfs(valStabFac, tmp, 15);

            z = valCodeGain;

            if (z < st->iNoiseEnhancerThres)
            {
                z += Mpy_32_16(valScaleCodeGain, valCodeGainLow, 6226);
                if (z > st->iNoiseEnhancerThres) z = st->iNoiseEnhancerThres;
            } else
            {
                z = Mpy_32_16(valScaleCodeGain, valCodeGainLow, 27536);
                if (z < st->iNoiseEnhancerThres) z = st->iNoiseEnhancerThres;
            }
            st->iNoiseEnhancerThres = z;

            valCodeGain = Mpy_32_16(valScaleCodeGain, valCodeGainLow, (Ipp16s)(IPP_MAX_16S - valFac));
            Unpack_32s(z, &valScaleCodeGain, &valCodeGainLow);
            valCodeGain += Mpy_32_16(valScaleCodeGain, valCodeGainLow, valFac);

            /* pitch enhancer */
            tmp = (Ipp16s)((valVoiceFac >> 3) + 4096); /* 0.25=voiced, 0=unvoiced */

            pSrcCoeff[0] = 1;
            pSrcCoeff[1] = tmp;
            ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, pFixCdbkvec, pFixCdbkExcvec, SUBFRAME_SIZE, 0);

            /* build excitation */

            valScaleCodeGain = Cnvrt_NR_32s16s(valCodeGain << valQNew);
            ippsInterpolateC_NR_16s(pFixCdbkExcvec, valScaleCodeGain, 7,
                pExc2vec, valPitchGain, 2, pExc2vec,SUBFRAME_SIZE);
            valCorrGain = ownSynthesis(pLPCQuant, pExc2vec, valQNew, &src[valSubFrame * 5 / 4], st);
            *(pPrm)++ = valCorrGain;
        }
        pLPCUnQuant += (LP_ORDER + 1);
        pLPCQuant += (LP_ORDER + 1);
    }                                      /* end of subframe loop */

    if (offsetWBE == 0)
       ownPrms2Bits(pPrmvec,dst,*rate);

    ippsCopy_16s(&pSpeechOldvec[FRAME_SIZE], st->asiSpeechOld, SPEECH_SIZE - FRAME_SIZE + offsetWBE);
    ippsCopy_16s(&pWgtSpchOldvec[FRAME_SIZE / OPL_DECIM], st->asiWspOld, PITCH_LAG_MAX / OPL_DECIM);
    ippsCopy_16s(&pExcOldvec[FRAME_SIZE], st->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);

    return APIAMRWB_StsNoErr;
}

/* Synthesis of signal at 16kHz with pHighFreqvec extension */
static Ipp16s ownSynthesis(Ipp16s *pLPCQuantvec, Ipp16s *pExc, Ipp16s valQNew,
                          const Ipp16u *pSynthSignal, AMRWBEncoder_Obj *st)
{
    Ipp16s valFac, tmp, valExp;
    Ipp16s valEner, valExpEner;
    Ipp32s i, z, sfs;

    IPP_ALIGNED_ARRAY( 16, Ipp32s, pSynthHighvec, LP_ORDER + SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY( 16, Ipp16s, pSynthvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY( 16, Ipp16s, pHighFreqvec, SUBFRAME_SIZE_16k);
    IPP_ALIGNED_ARRAY( 16, Ipp16s, pLPCvec, LP_ORDER + 1);
    IPP_ALIGNED_ARRAY( 16, Ipp16s, pHighFreqSpchvec, SUBFRAME_SIZE_16k);

    Ipp16s valHPEstGain, valHPCalcGain, valHPCorrGain;
    Ipp16s valDistMin, valDist;
    Ipp16s valHPGainIndex = 0;
    Ipp16s valGainCoeff, valAdptGain;
    Ipp16s valWeight, valWeight2;

    /* speech synthesis */
    ippsCopy_16s((Ipp16s*)st->aiSynthMem, (Ipp16s*)pSynthHighvec, LP_ORDER*2);
    tmp = pLPCQuantvec[0];
    sfs = Exp_16s(tmp) - 2;
    pLPCQuantvec[0] >>= (4 + valQNew);

#if(!ForBullsEyeOnly)
    if (sfs)
        ownSynthesisFilter_AMRWB_16s32s_ISfs(pLPCQuantvec, LP_ORDER, pExc, pSynthHighvec + LP_ORDER, SUBFRAME_SIZE, sfs + 3);
    else
        ippsSynthesisFilter_AMRWB_16s32s_I(pLPCQuantvec, LP_ORDER, pExc, pSynthHighvec + LP_ORDER, SUBFRAME_SIZE);
#else
	/* Move calling ippsSynthesisFilter_AMRWB_16s32s_I to ownSynthesisFilter_AMRWB_16s32s_ISfs for better BullsEye coverage*/
	ownSynthesisFilter_AMRWB_16s32s_ISfs(pLPCQuantvec, LP_ORDER, pExc, pSynthHighvec + LP_ORDER, SUBFRAME_SIZE, sfs + 3);
#endif

    pLPCQuantvec[0] = tmp;

    ippsCopy_16s((Ipp16s*)(&pSynthHighvec[SUBFRAME_SIZE]), (Ipp16s*)st->aiSynthMem, LP_ORDER*2);
    ippsDeemphasize_AMRWB_32s16s(PREEMPH_FACTOR>>1, pSynthHighvec + LP_ORDER, pSynthvec, SUBFRAME_SIZE, &(st->siDeemph));
    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnlOut, 14);

    /* Original speech signal as reference for high band gain quantisation */
    ippsCopy_16s((Ipp16s*)pSynthSignal,pHighFreqSpchvec,SUBFRAME_SIZE_16k);

    /* pHighFreqvec noise synthesis */

    for (i = 0; i < SUBFRAME_SIZE_16k; i++)
        pHighFreqvec[i] = (Ipp16s)(Rand_16s(&(st->siHFSeed)) >> 3);

    /* energy of excitation */

    ownScaleSignal_AMRWB_16s_ISfs(pExc, SUBFRAME_SIZE, -3);
    valQNew -= 3;

    ippsDotProd_16s32s_Sfs( pExc, pExc,SUBFRAME_SIZE, &z, -1);
    z = Add_32s(z, 1);

    valExpEner = (Ipp16s)(30 - Norm_32s_I(&z));
    valEner = (Ipp16s)(z >> 16);

    valExpEner = (Ipp16s)(valExpEner - (valQNew + valQNew));

    /* set energy of white noise to energy of excitation */

    ippsDotProd_16s32s_Sfs( pHighFreqvec, pHighFreqvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExp = (Ipp16s)(30 - Norm_32s_I(&z));
    tmp = (Ipp16s)(z >> 16);

    if (tmp > valEner)
    {
        tmp >>= 1;
        valExp += 1;
    }
    if (tmp == valEner)
        z = (Ipp32s)IPP_MAX_16S << 16;
    else
        z = (Ipp32s)(((Ipp32s)tmp << 15) / (Ipp32s)valEner) << 16;
    valExp = (Ipp16s)(valExp - valExpEner);
    InvSqrt_32s16s_I(&z, &valExp);
    if ((valExp+1) >= 0)
        z = ShiftL_32s(z, (Ipp16s)(valExp + 1));
    else
        z >>= (-(valExp + 1));
    tmp = (Ipp16s)(z >> 16);

    ownMulC_16s_ISfs(tmp, pHighFreqvec, SUBFRAME_SIZE_16k, 15);

    /* find tilt of synthesis speech (tilt: 1=voiced, -1=unvoiced) */

    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnl400, 15);
    ippsDotProd_16s32s_Sfs( pSynthvec, pSynthvec,SUBFRAME_SIZE, &z, -1);
    z = Add_32s(z, 1);

    valExp = Norm_32s_I(&z);
    valEner = (Ipp16s)(z>>16);

    ippsDotProd_16s32s_Sfs( pSynthvec, &pSynthvec[1],SUBFRAME_SIZE-1, &z, -1);
    z = Add_32s(z, 1);

    tmp = (Ipp16s)(ShiftL_32s(z, valExp) >> 16);
    if (tmp > 0)
    {
        if (tmp == valEner)
            valFac = IPP_MAX_16S;
        else
            valFac = (Ipp16s)(((Ipp32s)tmp << 15) / (Ipp32s)valEner);
    } else
        valFac = 0;

    /* modify energy of white noise according to synthesis tilt */
    valGainCoeff = (Ipp16s)(IPP_MAX_16S - valFac);
    valAdptGain = Cnvrt_32s16s((valGainCoeff * 5) >> 2);

    if (st->siVadHist > 0)
    {
        valWeight = 0;
        valWeight2 = IPP_MAX_16S;
    } else
    {
        valWeight = IPP_MAX_16S;
        valWeight2 = 0;
    }
    tmp = Mul_16s_Sfs(valWeight, valGainCoeff, 15);
    tmp = (Ipp16s)(tmp + Mul_16s_Sfs(valWeight2, valAdptGain, 15));

    if (tmp != 0) tmp += 1;
    valHPEstGain = tmp;

    if (valHPEstGain < 3277) valHPEstGain = 3277;

    /* synthesis of noise: 4.8kHz..5.6kHz --> 6kHz..7kHz */
    ippsMulPowerC_NR_16s_Sfs(pLPCQuantvec, 19661, pLPCvec, LP_ORDER+1, 15); /* valFac=0.6 */
    {
      Ipp16s tmpLPCVec;
      tmpLPCVec = pLPCvec[0];
      pLPCvec[0] >>= 1;
      ippsSynthesisFilter_G729E_16s_I(pLPCvec, LP_ORDER, pHighFreqvec, SUBFRAME_SIZE_16k, st->asiSynthHighFilt);
      ippsCopy_16s(&pHighFreqvec[SUBFRAME_SIZE_16k-LP_ORDER],st->asiSynthHighFilt,LP_ORDER);
      pLPCvec[0] = tmpLPCVec;
    }

    /* noise High Pass filtering (1ms of delay) */
    HighPassFIR_AMRWB_16s_ISfs(pHighFreqvec, SUBFRAME_SIZE_16k, st->pSHPFIRState);

    /* filtering of the original signal */
    HighPassFIR_AMRWB_16s_ISfs(pHighFreqSpchvec, SUBFRAME_SIZE_16k, st->pSHPFIRState2);

    /* check the gain difference */
    ownScaleSignal_AMRWB_16s_ISfs(pHighFreqSpchvec, SUBFRAME_SIZE_16k, -1);

    ippsDotProd_16s32s_Sfs( pHighFreqSpchvec, pHighFreqSpchvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExpEner = (Ipp16s)(30 - Norm_32s_I(&z));
    valEner = (Ipp16s)(z >> 16);

    /* set energy of white noise to energy of excitation */
    ippsDotProd_16s32s_Sfs( pHighFreqvec, pHighFreqvec,SUBFRAME_SIZE_16k, &z, -1);
    z = Add_32s(z, 1);

    valExp = (Ipp16s)(30 - Norm_32s_I(&z));
    tmp = (Ipp16s)(z >> 16);

    if (tmp > valEner)
    {
        tmp >>= 1;
        valExp += 1;
    }
    if (tmp == valEner)
        z = (Ipp32s)IPP_MAX_16S << 16;
    else
        z = (Ipp32s)(((Ipp32s)tmp << 15) / (Ipp32s)valEner) << 16;
    valExp = (Ipp16s)(valExp - valExpEner);
    InvSqrt_32s16s_I(&z, &valExp);
    if (valExp > 0)
        z = ShiftL_32s(z, valExp);
    else
        z >>= (-valExp);
    valHPCalcGain = (Ipp16s)(z >> 16);

    z = st->dtxEncState.siHangoverCount * 4681;
    st->siAlphaGain = Mul_16s_Sfs(st->siAlphaGain, (Ipp16s)z, 15);

    if (st->dtxEncState.siHangoverCount > 6)
        st->siAlphaGain = IPP_MAX_16S;
    valHPEstGain >>= 1;
    valHPCorrGain  = Mul_16s_Sfs(valHPCalcGain, st->siAlphaGain, 15);
    valHPCorrGain = (Ipp16s)(valHPCorrGain + Mul_16s_Sfs((Ipp16s)(IPP_MAX_16S - st->siAlphaGain), valHPEstGain, 15));

    /* Quantise the correction gain */
    valDistMin = IPP_MAX_16S;
    for (i = 0; i < 16; i++)
    {
        valDist = Mul_16s_Sfs((Ipp16s)(valHPCorrGain - HPGainTbl[i]), (Ipp16s)(valHPCorrGain - HPGainTbl[i]), 15);
        if (valDistMin > valDist)
        {
            valDistMin = valDist;
            valHPGainIndex = (Ipp16s)i;
        }
    }

    /* return the quantised gain index when using the highest mode, otherwise zero */
    return (valHPGainIndex);
}
static Ipp8s* ownPar2Ser( Ipp32s valPrm, Ipp8s *pBitStreamvec, Ipp32s valNumBits )
{
    Ipp32s i;
    Ipp16s tmp;
    Ipp8s *TempPnt = pBitStreamvec + valNumBits -1;

    for ( i = 0 ; i < valNumBits ; i ++ ) {
        tmp = (Ipp16s) (valPrm & 0x1);
        valPrm >>= 1;
        *TempPnt-- = (Ipp8s)tmp;
    }

    return (pBitStreamvec + valNumBits);
}

static void ownPrms2Bits(const Ipp16s* pPrms, Ipp8u *pBitstream, AMRWB_Rate_t rate)
{
    Ipp8s *pBsp;
    Ipp32s i, tmp, valSht, valBitCnt = 0;
    IPP_ALIGNED_ARRAY(16, Ipp8s, pBitStreamvec, NB_BITS_MAX);
    Ipp8u *pBits = (Ipp8u*)pBitstream;

    pBsp = pBitStreamvec ;

    for( i = 0; i < NumberPrmsTbl[rate]; i++) {
       valBitCnt += pNumberBitsTbl[rate][i];
       tmp = pPrms[i];
       pBsp = ownPar2Ser( tmp, pBsp, pNumberBitsTbl[rate][i] );
    }

    /* Clear the output vector */
    ippsZero_8u((Ipp8u*)pBitstream, (valBitCnt+7)/8);
    valSht  = (valBitCnt + 7) / 8 * 8 - valBitCnt;

    for ( i = 0 ; i < valBitCnt; i ++ ) {
        pBits[i/8] <<= 1;
        if (pBitStreamvec[i]) pBits[i/8]++;
    }
    pBits[(valBitCnt-1)/8] <<= valSht;
}
