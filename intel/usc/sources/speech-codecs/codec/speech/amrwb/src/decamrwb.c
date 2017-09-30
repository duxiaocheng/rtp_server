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
// Purpose: AMRWB speech codec decoder API functions
//
*/

#include "ownamrwb.h"

/* extension */
#if(0) /* never used. Removed to improve BullsEye Coverage*/
extern __ALIGN32 CONST Ipp16s tblGainRampHF[64];

static void ownSynthesisDec_WBE(Ipp16s *pLPCvec, Ipp16s* pExcvec, Ipp16s valQNew,
   Ipp16u *pSynSignal, Ipp16s pPrmvec, Ipp16s *pHfIsfvec, IppSpchBitRate mode,
   Ipp16s valDTXState, AMRWBDecoder_Obj* st, Ipp16s bfi);
#endif

static void ownSynthesis(Ipp16s *pLPCvec, Ipp16s *pExcvec, Ipp16s valQNew,
   Ipp16u *pSynSignal, Ipp16s pPrmvec, Ipp16s *pHfIsfvec, IppSpchBitRate mode,
   Ipp16s valDTXState, AMRWBDecoder_Obj * st, Ipp16s bfi);
void (*ownSynthFun)(Ipp16s*, Ipp16s*, Ipp16s, Ipp16u*, Ipp16s, Ipp16s*,
   IppSpchBitRate, Ipp16s, AMRWBDecoder_Obj*, Ipp16s );

static void ownBits2Prms(const Ipp8u *pBitstream, Ipp16s *pPrms , AMRWB_Rate_t rate);

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBDecoder_GetSize,
         (AMRWBDecoder_Obj* decoderObj, Ipp32u *pCodecSize))
{
   if(NULL == decoderObj)
      return APIAMRWB_StsBadArgErr;
   if(NULL == pCodecSize)
      return APIAMRWB_StsBadArgErr;
   if(decoderObj->objPrm.iKey != DEC_KEY)
      return APIAMRWB_StsNotInitialized;

   *pCodecSize = decoderObj->objPrm.iObjSize;
   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBDecoder_Alloc,
         (const AMRWBDec_Params *amrwb_Params, Ipp32u *pCodecSize))
{
   Ipp32s hpfltsize;
   if(NULL == amrwb_Params) return APIAMRWB_StsBadArgErr;
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   *pCodecSize=sizeof(AMRWBDecoder_Obj);
   *pCodecSize += (2*hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize);
   *pCodecSize += (2*hpfltsize);
   ippsAdaptiveCodebookDecodeGetSize_AMRWB_16s(&hpfltsize);
   *pCodecSize += hpfltsize;

   return APIAMRWB_StsNoErr;
}

static void DecoderInit(AMRWBDecoder_Obj* decoderObj)
{
   ippsZero_16s(decoderObj->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);
   ippsZero_16s(decoderObj->asiIsfQuantPast, LP_ORDER);

   decoderObj->siFrameFirst = 1;
   decoderObj->iNoiseEnhancerThres = 0;
   decoderObj->siTiltCode = 0;

   ownPhaseDispInit(decoderObj->asiPhaseDisp);

   /* scaling memories for excitation */
   decoderObj->siScaleFactorOld = MAX_SCALE_FACTOR;
   decoderObj->asiScaleFactorMax[3] = MAX_SCALE_FACTOR;
   decoderObj->asiScaleFactorMax[2] = MAX_SCALE_FACTOR;
   decoderObj->asiScaleFactorMax[1] = MAX_SCALE_FACTOR;
   decoderObj->asiScaleFactorMax[0] = MAX_SCALE_FACTOR;

   /* extension */
   decoderObj->siOldPitchLag = 64;
   decoderObj->siOldPitchLagFrac = 0;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBDecoder_Init,
         (AMRWBDecoder_Obj* decoderObj))
{
   Ipp32s hpfltsize, i;

   DecoderInit(decoderObj);

   decoderObj->pSHighPassFIRState = (HighPassFIRState_AMRWB_16s_ISfs *)((Ipp8s*)decoderObj + sizeof(AMRWBDecoder_Obj));
   HighPassFIRGetSize_AMRWB_16s_ISfs(&hpfltsize);
   decoderObj->pSHighPassFIRState2 = (HighPassFIRState_AMRWB_16s_ISfs *)((Ipp8s*)decoderObj->pSHighPassFIRState + hpfltsize);
   decoderObj->pSHPFiltStateSgnlOut = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)decoderObj->pSHighPassFIRState2 + hpfltsize);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &hpfltsize);
   decoderObj->pSHPFiltStateSgnl400 = (IppsHighPassFilterState_AMRWB_16s *)((Ipp8s*)decoderObj->pSHPFiltStateSgnlOut + hpfltsize);
   decoderObj->pSAdaptCdbkDecState = (IppsAdaptiveCodebookDecodeState_AMRWB_16s *)((Ipp8s*)decoderObj->pSHPFiltStateSgnl400 + hpfltsize);

   /* routines initialization */

   ippsSet_16s(-14336, decoderObj->asiQuantEnerPast, 4);
   ippsZero_16s(decoderObj->asiCodeGainPast, 5);
   ippsZero_16s(decoderObj->asiQuantGainPast, 5);
   decoderObj->siCodeGainPrev = 0;
   ippsZero_16s(decoderObj->asiOversampFilt, 2 * NB_COEF_UP);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffHP50Tbl, (Ipp16s*)BCoeffHP50Tbl, 2, decoderObj->pSHPFiltStateSgnlOut);
   HighPassFIRInit_AMRWB_16s_ISfs((Ipp16s*)Fir6k_7kTbl, 2, decoderObj->pSHighPassFIRState);
   HighPassFIRInit_AMRWB_16s_ISfs((Ipp16s*)Fir7kTbl, 0, decoderObj->pSHighPassFIRState2);
   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)ACoeffHP400Tbl, (Ipp16s*)BCoeffHP400Tbl, 2, decoderObj->pSHPFiltStateSgnl400);

   /* isp initialization */

   ippsCopy_16s(IspInitTbl, decoderObj->asiIspOld, LP_ORDER);
   ippsCopy_16s(IsfInitTbl, decoderObj->asiIsfOld, LP_ORDER);
   for (i=0; i<3; i++)
       ippsCopy_16s(IsfInitTbl, &decoderObj->asiIsf[i * LP_ORDER], LP_ORDER);
   /* variable initialization */

   decoderObj->siDeemph = 0;

   decoderObj->siSeed = RND_INIT;             /* init random with 21845 */
   decoderObj->siHFSeed = RND_INIT;             /* init random with 21845 */
   ippsAdaptiveCodebookDecodeInit_AMRWB_16s(decoderObj->pSAdaptCdbkDecState);

   decoderObj->siBFHState = 0;
   decoderObj->siBfiPrev = 0;

   /* Static vectors to zero */

   ippsZero_16s(decoderObj->asiSynthHighFilt, LP_ORDER_16K);
   ippsZero_16s((Ipp16s*)decoderObj->asiSynthesis, LP_ORDER*2);

   ownDTXDecReset(&decoderObj->dtxDecState, (Ipp16s*)IsfInitTbl);
   decoderObj->siVadHist = 0;

   /* extension */
   decoderObj->siOldPitchLag = 64;
   decoderObj->siOldPitchLagFrac = 0;

   ippsZero_16s(decoderObj->mem_syn_out, (PITCH_LAG_MAX + SUBFRAME_SIZE));
   ippsZero_16s(decoderObj->mem_oversamp_hf_plus, (2 * UP_SAMPL_FILT_DELAY) );
   ippsZero_16s(decoderObj->mem_syn_hf_plus, 8);
   ippsZero_16s(decoderObj->lpc_hf_plus + 1, 8);
   decoderObj->lpc_hf_plus[0] = 4096;
   decoderObj->gain_hf_plus = 0;
   decoderObj->threshold_hf = 0;
   decoderObj->lp_amp_hf = 0;
   decoderObj->ramp_state = 0;
   
   decoderObj->objPrm.iKey =DEC_KEY;

   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiAMRWBDecode,
         (AMRWBDecoder_Obj* decoderObj, const Ipp8u* src, AMRWB_Rate_t rate,
          RXFrameType rx_type, Ipp16u* dst, Ipp32s offsetWBE))
{
    /* Decoder states */
    AMRWBDecoder_Obj *st;

    /* Excitation vector */
    IPP_ALIGNED_ARRAY(16, Ipp16s, pExcOld, (FRAME_SIZE+1)+PITCH_LAG_MAX+INTERPOL_LEN);
    Ipp16s *pExcvec;

    /* LPC coefficients */
    Ipp16s *pLPC;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCvec, NUMBER_SUBFRAME*(LP_ORDER+1));
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIspvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIsfvec, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFixedCodevec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFixedCodevec2, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pExcvec2, FRAME_SIZE);

    Ipp16s valFac, valStabFac, valVoiceFac, valQNew = 0;
    Ipp32s s, valCodeGain;

    /* Scalars */
    Ipp16s i, j, valSubFrame, valAdptIndex, valMax, tmp;
    Ipp16s valIntPitchLag, valFracPitchLag, valSelect;
    Ipp16s valPitchGain, valGainCode, valGainCodeLow;
    Ipp16s valDTXState, bfi, unusableFrame;
    Ipp16s vadFlag;
    Ipp16s valPitchSharp;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pExctmp, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pIsftmp, LP_ORDER);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pHfIsfvec, LP_ORDER_16K);
    Ipp16s pSrcCoeff[2];
    Ipp16s pPrevIntPitchLagBounds[2],subFrame;
    Ipp16s valPastPitchGain = 0, valPastCodeGain,exp;
    Ipp32s valInvEnergy;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pPrmvec, MAX_PARAM_SIZE);
    Ipp16s *pPrms = pPrmvec;

    Ipp16s valCorrGain = 0;
    IppSpchBitRate irate = Mode2RateTbl[rate];

   //if (offsetWBE == 0) 
   { /* old WB mode */
      ownSynthFun = ownSynthesis; /* select old Synthesis fun */
      if(rx_type != RX_NO_DATA) {
         if( rx_type == RX_SID_BAD || rx_type == RX_SID_UPDATE) {
            ownBits2Prms(src,pPrms,AMRWB_RATE_DTX);
         } else {
            ownBits2Prms(src,pPrms,rate);
         }
      }
   } 
#if(0) /* never used. Removed to improve BullsEye Coverage*/
   else 
   {
      ownSynthFun = ownSynthesisDec_WBE;
      ippsCopy_16s((const Ipp16s*)src, pPrms, MAX_PARAM_SIZE);
   }
#endif

    st = decoderObj;
    pPrevIntPitchLagBounds[0] = 0;

    valDTXState = ownRXDTXHandler(&st->dtxDecState, (Ipp16s)rx_type);

    if (valDTXState != SPEECH)
    {
        ownDTXDec(&st->dtxDecState, pExcvec2, valDTXState, pIsfvec, (const Ipp16u*)pPrms);
    }
    /* SPEECH action state machine  */

    if ((rx_type == RX_SPEECH_BAD) ||
        (rx_type == RX_SPEECH_PROBABLY_DEGRADED))
    {
        /* bfi for all valAdptIndex, bits are not usable */
        bfi = 1;
        unusableFrame = 0;
    } else if ((rx_type == RX_SPEECH_LOST) ||
               (rx_type == RX_NO_DATA))
    {
        /* bfi only for lsf, gains and pitch period */
        bfi = 1;
        unusableFrame = 1;
    } else
    {
        bfi = 0;
        unusableFrame = 0;
    }

    if (bfi != 0)
    {
        st->siBFHState += 1;
        if (st->siBFHState > 6) st->siBFHState = 6;
    } else
    {
        st->siBFHState >>= 1;
    }

    if (st->dtxDecState.siGlobalState == DTX)
    {
        st->siBFHState = 5;
        st->siBfiPrev = 0;
    } else if (st->dtxDecState.siGlobalState == DTX_MUTE)
    {
        st->siBFHState = 5;
        st->siBfiPrev = 1;
    }

    if (valDTXState == SPEECH)
    {
        vadFlag = *(pPrms)++;
        if (bfi == 0)
        {
            if (vadFlag == 0) {
                st->siVadHist += 1;
				st->dtxDecState.sidtxVadHist=Add_16s(st->dtxDecState.sidtxVadHist,1);/*added for CR15 chnage:Robust operation of AMRWB-decoder*/
			}
            else {
				st->siVadHist = 0;
				st->dtxDecState.sidtxVadHist=0;/*added for CR15 chnage:Robust operation of AMRWB-decoder*/
			}
        }
		else if (st->dtxDecState.sidtxVadHist>0){    /*added for CR15 chnage:Robust operation of AMRWB-decoder*/
			st->dtxDecState.sidtxVadHist=Add_16s(st->dtxDecState.sidtxVadHist,1);
		}
    }

    if (valDTXState != SPEECH)     /* CNG mode */
    {
        ippsISFToISP_AMRWB_16s(pIsfvec, pIspvec, LP_ORDER);
//        ippsISPToLPC_AMRWB_16s(pIspvec, pLPCvec, LP_ORDER);
        ownISPToLPC_AMRWB_16s(pIspvec, pLPCvec, LP_ORDER, 1);
        ippsCopy_16s(st->asiIsfOld, pIsftmp, LP_ORDER);

        for (valSubFrame = 0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE)
        {
            j = (Ipp16s)(valSubFrame >> 6);
            ippsInterpolateC_NR_G729_16s_Sfs(pIsftmp, (Ipp16s)(IPP_MAX_16S - InterpolFracTbl[j]),
                pIsfvec,InterpolFracTbl[j], pHfIsfvec, LP_ORDER, 15);
            ownSynthFun(pLPCvec, &pExcvec2[valSubFrame], 0, &dst[valSubFrame * 5 / 4], (Ipp16s)1,
                pHfIsfvec, irate, valDTXState, st, bfi);
        }

        DecoderInit(decoderObj);
        ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);

        st->siBfiPrev = bfi;
        st->dtxDecState.siGlobalState = valDTXState;

        return APIAMRWB_StsNoErr;
    }

    ippsCopy_16s(st->asiExcOld, pExcOld, PITCH_LAG_MAX + INTERPOL_LEN);
    pExcvec = pExcOld + PITCH_LAG_MAX + INTERPOL_LEN;

    ippsISFQuantDecode_AMRWB_16s((Ipp16s*)pPrms, pIsfvec, st->asiIsfQuantPast, st->asiIsfOld, st->asiIsf, bfi, irate);
    if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_DTX))
        pPrms += 5;
    else
        pPrms += 7;

    ippsISFToISP_AMRWB_16s(pIsfvec, pIspvec, LP_ORDER);

    if (st->siFrameFirst != 0)
    {
        st->siFrameFirst = 0;
        ippsCopy_16s(pIspvec, st->asiIspOld, LP_ORDER);
    }
    /* Find the interpolated ISPs and convert to pLPCvec for all subframes */
    {
      IPP_ALIGNED_ARRAY(16, Ipp16s, pIspTmp, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld, 18022, pIspvec,14746, pIspTmp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(pIspTmp, pLPCvec, LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  6554, pIspvec,26214, pIspTmp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(pIspTmp, &pLPCvec[LP_ORDER + 1], LP_ORDER);

      ippsInterpolateC_NR_G729_16s_Sfs(st->asiIspOld,  1311, pIspvec,31457, pIspTmp, LP_ORDER, 15);
      ippsISPToLPC_AMRWB_16s(pIspTmp, &pLPCvec[2*(LP_ORDER + 1)], LP_ORDER);

      /* 4th subframe: pLPCvec (frac=1.0) */

      ippsISPToLPC_AMRWB_16s(pIspvec, &pLPCvec[3*(LP_ORDER + 1)], LP_ORDER);
      ippsCopy_16s(pIspvec, st->asiIspOld, LP_ORDER);
    }

    /* Check stability on pIsfvec : distance between old st->asiIsfOld and current pIsfvec */
    valStabFac = ownChkStab(pIsfvec, st->asiIsfOld, LP_ORDER-1);
    ippsCopy_16s(st->asiIsfOld, pIsftmp, LP_ORDER);
    ippsCopy_16s(pIsfvec, st->asiIsfOld, LP_ORDER);

    pLPC = pLPCvec;                               /* pointer to interpolated LPC parameters */

    for (valSubFrame = 0,subFrame=0; valSubFrame < FRAME_SIZE; valSubFrame += SUBFRAME_SIZE,subFrame++)
    {
        valAdptIndex = *(pPrms)++;
        ippsAdaptiveCodebookDecode_AMRWB_16s(valAdptIndex, &valFracPitchLag, &pExcvec[valSubFrame],
            &valIntPitchLag, pPrevIntPitchLagBounds, subFrame, bfi, unusableFrame, irate,st->pSAdaptCdbkDecState);

        if (unusableFrame)
        {
            valSelect = 1;
        } else
        {
            if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_8850)||(irate == IPP_SPCHBR_DTX))
                valSelect = 0;
            else
                valSelect = *(pPrms)++;
        }

        if (valSelect == 0)
        {
            /* find pitch excitation with lp filter */
            pSrcCoeff[0] = 20972;
            pSrcCoeff[1] = -5898;
            ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, &pExcvec[valSubFrame], pFixedCodevec, SUBFRAME_SIZE, 1);
            ippsCopy_16s(pFixedCodevec, &pExcvec[valSubFrame], SUBFRAME_SIZE);
        }

        if (unusableFrame != 0)
        {
            for (i = 0; i < SUBFRAME_SIZE; i++)
            {
                pFixedCodevec[i] = (Ipp16s)(Rand_16s(&(st->siSeed)) >> 3);
            }
        } else {
           ippsAlgebraicCodebookDecode_AMRWB_16s((Ipp16s*)pPrms, pFixedCodevec, irate);
           if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_DTX))
               pPrms += 1;
           else if ((irate == IPP_SPCHBR_8850) || (irate == IPP_SPCHBR_12650) ||
               (irate == IPP_SPCHBR_14250) || (irate == IPP_SPCHBR_15850))
               pPrms += 4;
           else
               pPrms += 8;
        }

        tmp = 0;
        ippsPreemphasize_AMRWB_16s_ISfs (st->siTiltCode, pFixedCodevec, SUBFRAME_SIZE, 14, &tmp);

        tmp = valIntPitchLag;
        if (valFracPitchLag > 2) tmp += 1;

        if (tmp < SUBFRAME_SIZE) {
            ippsHarmonicFilter_NR_16s(PITCH_SHARP_FACTOR, tmp, &pFixedCodevec[tmp], &pFixedCodevec[tmp], SUBFRAME_SIZE-tmp);
        }

        ippsDotProd_16s32s_Sfs(pFixedCodevec, pFixedCodevec,SUBFRAME_SIZE, &s, -1);
        s = Add_32s(s, 1);

        exp = (Ipp16s)(6 - Norm_32s_I(&s));  /* exponent = 0..30 */
        InvSqrt_32s16s_I(&s, &exp);

        if ((exp - 3) > 0)
            valInvEnergy = (Ipp16s)(ShiftL_32s(s,(Ipp16s)(exp - 3)) >> 16);
        else
            valInvEnergy = (Ipp16s)(s >> (16 + 3 - exp));

        if (bfi != 0)
        {
            tmp = ownMedian5(&st->asiCodeGainPast[2]);
            valPastPitchGain = tmp;

            if (valPastPitchGain > 15565) valPastPitchGain = 15565;

            if (unusableFrame != 0)
               valPitchGain = Mul_16s_Sfs(PCoeffDownUnusableTbl[st->siBFHState], valPastPitchGain, 15);
            else
               valPitchGain = Mul_16s_Sfs(PCoeffDownUsableTbl[st->siBFHState], valPastPitchGain, 15);

            tmp = ownMedian5(&st->asiQuantGainPast[2]);

            if (st->siVadHist > 2)
            {
               valPastCodeGain = tmp;
            } else
            {
               if (unusableFrame != 0)
                  valPastCodeGain = Mul_16s_Sfs(CCoeffDownUnusableTbl[st->siBFHState], tmp, 15);
               else
                  valPastCodeGain = Mul_16s_Sfs(CCoeffDownUsableTbl[st->siBFHState], tmp, 15);
            }
        }

        valAdptIndex = *(pPrms)++;
        ippsDecodeGain_AMRWB_16s(valAdptIndex, valInvEnergy, &valPitchGain, &valCodeGain, bfi,
            st->siBfiPrev, st->asiQuantEnerPast, &st->siCodeGainPrev, &valPastCodeGain, irate);

        if (bfi == 0) {
            valPastPitchGain = valPitchGain;
            ippsAdaptiveCodebookDecodeUpdate_AMRWB_16s(valPitchGain, valIntPitchLag, st->pSAdaptCdbkDecState);
            st->siOldPitchLag = valIntPitchLag;       /* save duplicate for WB<->WB+ */
            st->siOldPitchLagFrac = 0;    /* Remove fraction in case of BFI */
        }

        ippsMove_16s(&st->asiQuantGainPast[1],st->asiQuantGainPast, 4);
        st->asiQuantGainPast[4] = valPastCodeGain;

        ippsMove_16s(&st->asiCodeGainPast[1],st->asiCodeGainPast, 4);
        st->asiCodeGainPast[4] = valPastPitchGain;

        ippsMin_16s(st->asiScaleFactorMax, 4, &tmp);
        if (tmp > MAX_SCALE_FACTOR) tmp = MAX_SCALE_FACTOR;

        valQNew = 0;
        s = valCodeGain;

        while ((s < 0x08000000) && (valQNew < tmp))
        {
            s <<= 1;
            valQNew += 1;
        }
        valGainCode = Cnvrt_NR_32s16s(s);

        ownScaleSignal_AMRWB_16s_ISfs(pExcvec + valSubFrame - (PITCH_LAG_MAX + INTERPOL_LEN),
            PITCH_LAG_MAX + INTERPOL_LEN + SUBFRAME_SIZE, (Ipp16s)(valQNew - st->siScaleFactorOld));
        st->siScaleFactorOld = valQNew;

        ippsCopy_16s(&pExcvec[valSubFrame], pExcvec2, SUBFRAME_SIZE);
        ownScaleSignal_AMRWB_16s_ISfs(pExcvec2, SUBFRAME_SIZE, -3);

        /* post processing of excitation elements */

        if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_8850)||(irate == IPP_SPCHBR_DTX))
        {
            valPitchSharp = Cnvrt_32s16s(valPitchGain << 1);
            if (valPitchSharp > 16384)
            {
                for (i = 0; i < SUBFRAME_SIZE; i++)
                {
                    tmp = Mul_16s_Sfs(pExcvec2[i], valPitchSharp, 15);
                    pExctmp[i] = Cnvrt_NR_32s16s(tmp * valPitchGain);
                }
            }
        } else
            valPitchSharp = 0;

        valVoiceFac = ownVoiceFactor(pExcvec2, -3, valPitchGain, pFixedCodevec, valGainCode, SUBFRAME_SIZE);

        /* tilt of pFixedCodevec for next subframe: 0.5=voiced, 0=unvoiced */

        st->siTiltCode = (Ipp16s)((valVoiceFac >> 2) + 8192);

        ippsCopy_16s(&pExcvec[valSubFrame], pExcvec2, SUBFRAME_SIZE);
        ippsInterpolateC_NR_16s(pFixedCodevec, valGainCode, 7,
            &pExcvec[valSubFrame], valPitchGain, 2, &pExcvec[valSubFrame],SUBFRAME_SIZE);

        /* find maximum value of excitation for next scaling */
        valMax = 1;
        for (i = 0; i < SUBFRAME_SIZE; i++)
        {
            tmp = Abs_16s(pExcvec[i + valSubFrame]);
            if (tmp > valMax) valMax = tmp;
        }

        /* tmp = scaling possible according to valMax value of excitation */
        tmp = (Ipp16s)(Exp_16s(valMax) + valQNew - 1);

        st->asiScaleFactorMax[3] = st->asiScaleFactorMax[2];
        st->asiScaleFactorMax[2] = st->asiScaleFactorMax[1];
        st->asiScaleFactorMax[1] = st->asiScaleFactorMax[0];
        st->asiScaleFactorMax[0] = tmp;

        Unpack_32s(valCodeGain, &valGainCode, &valGainCodeLow);
        if ((irate == IPP_SPCHBR_6600) || (irate == IPP_SPCHBR_DTX))
            j = 0;
        else if (irate == IPP_SPCHBR_8850)
            j = 1;
        else
            j = 2;

        ownPhaseDisp(valGainCode, valPitchGain, pFixedCodevec, j, st->asiPhaseDisp);

        tmp = (Ipp16s)(16384 - (valVoiceFac >> 1));    /* 1=unvoiced, 0=voiced */
        valFac = Mul_16s_Sfs(valStabFac, tmp, 15);

        s = valCodeGain;
        if (s < st->iNoiseEnhancerThres)
        {
            s += Mpy_32_16(valGainCode, valGainCodeLow, 6226);
            if (s > st->iNoiseEnhancerThres) s = st->iNoiseEnhancerThres;
        } else
        {
            s = Mpy_32_16(valGainCode, valGainCodeLow, 27536);
            if (s < st->iNoiseEnhancerThres) s = st->iNoiseEnhancerThres;
        }
        st->iNoiseEnhancerThres = s;

        valCodeGain = Mpy_32_16(valGainCode, valGainCodeLow, (Ipp16s)(IPP_MAX_16S - valFac));
        Unpack_32s(s, &valGainCode, &valGainCodeLow);
        valCodeGain += Mpy_32_16(valGainCode, valGainCodeLow, valFac);

        tmp = (Ipp16s)((valVoiceFac >> 3) + 4096);/* 0.25=voiced, 0=unvoiced */

        pSrcCoeff[0] = 1;
        pSrcCoeff[1] = tmp;
        ippsHighPassFilter_Direct_AMRWB_16s(pSrcCoeff, pFixedCodevec, pFixedCodevec2, SUBFRAME_SIZE, 0);

        /* build excitation */

        valGainCode = Cnvrt_NR_32s16s(valCodeGain << valQNew);
        ippsInterpolateC_NR_16s(pFixedCodevec2, valGainCode, 7,
            pExcvec2, valPitchGain, 2, pExcvec2,SUBFRAME_SIZE);

        if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_8850)||(irate == IPP_SPCHBR_DTX))
        {
            if (valPitchSharp > 16384)
            {
                ippsAdd_16s(pExcvec2, pExctmp, pExctmp, SUBFRAME_SIZE);
                ownagc2(pExcvec2, pExctmp, SUBFRAME_SIZE);
                ippsCopy_16s(pExctmp, pExcvec2, SUBFRAME_SIZE);
            }
        }
        if ((irate == IPP_SPCHBR_6600)||(irate == IPP_SPCHBR_DTX))
        {
            j = (Ipp16s)(valSubFrame >> 6);
            ippsInterpolateC_NR_G729_16s_Sfs(pIsftmp, (Ipp16s)(IPP_MAX_16S - InterpolFracTbl[j]),
                pIsfvec,InterpolFracTbl[j], pHfIsfvec, LP_ORDER, 15);
        } else
            ippsZero_16s(st->asiSynthHighFilt, LP_ORDER_16K - LP_ORDER);

        if (irate == IPP_SPCHBR_23850)
        {
            valCorrGain = *(pPrms)++;
            ownSynthFun(pLPC, pExcvec2, valQNew, &dst[valSubFrame * 5 / 4], valCorrGain, pHfIsfvec,
                irate, valDTXState, st, bfi);
        } else
            ownSynthFun(pLPC, pExcvec2, valQNew, &dst[valSubFrame * 5 / 4], 0, pHfIsfvec,
                irate, valDTXState, st, bfi);
        pLPC += (LP_ORDER + 1);                   /* interpolated LPC parameters for next subframe */
    }

    ippsCopy_16s(&pExcOld[FRAME_SIZE], st->asiExcOld, PITCH_LAG_MAX + INTERPOL_LEN);

    ownScaleSignal_AMRWB_16s_ISfs(pExcvec, FRAME_SIZE, (Ipp16s)(-valQNew));
    ippsDecDTXBuffer_AMRWB_16s(pExcvec, pIsfvec, &st->dtxDecState.siHistPtr,
        st->dtxDecState.asiIsfHistory, st->dtxDecState.siLogEnerHist);

    st->dtxDecState.siGlobalState = valDTXState;

    st->siBfiPrev = bfi;

    return APIAMRWB_StsNoErr;
}

/* Synthesis of signal at 16kHz with HF extension */
static void ownSynthesis(Ipp16s *pLPCvec, Ipp16s *pExcvec, Ipp16s valQNew,
     Ipp16u *pSynSignal, Ipp16s pPrmvec, Ipp16s *pHfIsfvec,
     IppSpchBitRate mode, Ipp16s valDTXState, AMRWBDecoder_Obj * st, Ipp16s bfi)
{
    Ipp16s i, valFac, tmp, exp;
    Ipp16s ener, exp_ener;
    Ipp32s s;

    IPP_ALIGNED_ARRAY(16, Ipp32s, pSynthHi, LP_ORDER + SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pSynthvec, SUBFRAME_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pHFvec, SUBFRAME_SIZE_16k);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pLPCtmp, LP_ORDER_16K + 1);
    IPP_ALIGNED_ARRAY(16, Ipp16s, pHFLCPvec, LP_ORDER_16K + 1);
    Ipp16s valHFCorrGain;
    Ipp16s valHFGainIndex;
    Ipp16s gain1, gain2;
    Ipp16s weight1, weight2;
    Ipp16s tmp_aq0;
    Ipp32s tmpSum, sfs;

    ippsCopy_16s((Ipp16s*)st->asiSynthesis, (Ipp16s*)pSynthHi, LP_ORDER*2);

    tmp_aq0 = pLPCvec[0];
    sfs = Exp_16s(tmp_aq0) - 2;
    pLPCvec[0] >>= (4 + valQNew);
#if(!ForBullsEyeOnly)
    if (sfs)
        ownSynthesisFilter_AMRWB_16s32s_ISfs(pLPCvec, LP_ORDER, pExcvec, pSynthHi + LP_ORDER, SUBFRAME_SIZE, sfs + 3);
    else
        ippsSynthesisFilter_AMRWB_16s32s_I(pLPCvec, LP_ORDER, pExcvec, pSynthHi + LP_ORDER, SUBFRAME_SIZE);
#else
	/* Move calling ippsSynthesisFilter_AMRWB_16s32s_I to ownSynthesisFilter_AMRWB_16s32s_ISfs for better BullsEye coverage*/
	ownSynthesisFilter_AMRWB_16s32s_ISfs(pLPCvec, LP_ORDER, pExcvec, pSynthHi + LP_ORDER, SUBFRAME_SIZE, sfs + 3);
#endif
    pLPCvec[0] = tmp_aq0;

    ippsCopy_16s((Ipp16s*)(&pSynthHi[SUBFRAME_SIZE]), (Ipp16s*)st->asiSynthesis, LP_ORDER*2);
    ippsDeemphasize_AMRWB_32s16s(PREEMPH_FACTOR>>1, pSynthHi + LP_ORDER, pSynthvec, SUBFRAME_SIZE, &(st->siDeemph));
    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnlOut, 14);
    ownOversampling_AMRWB_16s(pSynthvec, SUBFRAME_SIZE, pSynSignal, st->asiOversampFilt);

    /* generate white noise vector */
    for (i = 0; i < SUBFRAME_SIZE_16k; i++)
        pHFvec[i] = (Ipp16s)(Rand_16s(&(st->siHFSeed)) >> 3);

    ownScaleSignal_AMRWB_16s_ISfs(pExcvec, SUBFRAME_SIZE, -3);
    valQNew -= 3;

    ippsDotProd_16s32s_Sfs( pExcvec, pExcvec,SUBFRAME_SIZE, &tmpSum, -1);
    tmpSum = Add_32s(tmpSum, 1);

    exp_ener = (Ipp16s)(30 - Norm_32s_I(&tmpSum));
    ener = (Ipp16s)(tmpSum >> 16);

    exp_ener = (Ipp16s)(exp_ener - (valQNew + valQNew));

    /* set energy of white noise to energy of excitation */
    ippsDotProd_16s32s_Sfs( pHFvec, pHFvec,SUBFRAME_SIZE_16k, &tmpSum, -1);
    tmpSum = Add_32s(tmpSum, 1);

    exp = (Ipp16s)(30 - Norm_32s_I(&tmpSum));
    tmp = (Ipp16s)(tmpSum >> 16);

    if (tmp > ener)
    {
        tmp >>= 1;
        exp += 1;
    }
    if (tmp == ener)
        s = (Ipp32s)IPP_MAX_16S << 16;
    else
        s = (Ipp32s)(((Ipp32s)tmp << 15) / (Ipp32s)ener) << 16;
    exp = (Ipp16s)(exp - exp_ener);
    InvSqrt_32s16s_I(&s, &exp);
    if ((exp+1) > 0)
        s = ShiftL_32s(s, (Ipp16s)(exp + 1));
    else
        s >>= (-(exp + 1));
    tmp = (Ipp16s)(s >> 16);
    ownMulC_16s_ISfs(tmp, pHFvec, SUBFRAME_SIZE_16k, 15);

    /* find tilt of synthesis speech (tilt: 1=voiced, -1=unvoiced) */
    ippsHighPassFilter_AMRWB_16s_ISfs(pSynthvec, SUBFRAME_SIZE, st->pSHPFiltStateSgnl400, 15);
    ippsDotProd_16s32s_Sfs( pSynthvec, pSynthvec,SUBFRAME_SIZE, &s, -1);
    s = Add_32s(s, 1);

    exp = Norm_32s_I(&s);
    ener = (Ipp16s)(s >> 16);

    ippsDotProd_16s32s_Sfs( pSynthvec, &pSynthvec[1],SUBFRAME_SIZE-1, &s, -1);
    s = Add_32s(s, 1);

    tmp = (Ipp16s)(ShiftL_32s(s, exp)>>16);

    if (tmp > 0)
    {
        if (tmp == ener)
            valFac = IPP_MAX_16S;
        else
            valFac = (Ipp16s)((tmp << 15) / ener);
    } else
        valFac = 0;

    /* modify energy of white noise according to synthesis tilt */
    gain1 = (Ipp16s)(IPP_MAX_16S - valFac);
    gain2 = (Ipp16s)((gain1 * 5) >> 3);
    gain2 = Cnvrt_32s16s(gain2 << 1);

    if (st->siVadHist > 0)
    {
        weight1 = 0;
        weight2 = IPP_MAX_16S;
    } else
    {
        weight1 = IPP_MAX_16S;
        weight2 = 0;
    }
    tmp = Mul_16s_Sfs(weight1, gain1, 15);
    tmp = (Ipp16s)(tmp + Mul_16s_Sfs(weight2, gain2, 15));

    if (tmp) tmp += 1;
    if (tmp < 3277) tmp = 3277;

    if ((mode == IPP_SPCHBR_23850) && (bfi == 0))
    {
        /* HF correction gain */
        valHFGainIndex = pPrmvec;
        valHFCorrGain = HPGainTbl[valHFGainIndex];

        /* HF gain */
        for (i = 0; i < SUBFRAME_SIZE_16k; i++)
            pHFvec[i] = (Ipp16s)(Mul_16s_Sfs(pHFvec[i], valHFCorrGain, 15) << 1);
    } else
        ownMulC_16s_ISfs(tmp, pHFvec, SUBFRAME_SIZE_16k, 15);

    if (((mode == IPP_SPCHBR_6600)||(mode == IPP_SPCHBR_DTX)) && (valDTXState == SPEECH))
    {
        ownIsfExtrapolation(pHfIsfvec);
        ippsISPToLPC_AMRWB_16s(pHfIsfvec, pHFLCPvec, LP_ORDER_16K);

        ippsMulPowerC_NR_16s_Sfs(pHFLCPvec, 29491, pLPCtmp, LP_ORDER_16K+1, 15); /* valFac=0.9 */
        {
            tmp = pLPCtmp[0];
            pLPCtmp[0] >>= 1;
            ippsSynthesisFilter_G729E_16s_I(pLPCtmp, LP_ORDER_16K, pHFvec, SUBFRAME_SIZE_16k, st->asiSynthHighFilt);
            ippsCopy_16s(&pHFvec[SUBFRAME_SIZE_16k-LP_ORDER_16K],st->asiSynthHighFilt,LP_ORDER_16K);
            pLPCtmp[0] = tmp;
        }
    } else
    {
        /* synthesis of noise: 4.8kHz..5.6kHz --> 6kHz..7kHz */
        ippsMulPowerC_NR_16s_Sfs(pLPCvec, 19661, pLPCtmp, LP_ORDER+1, 15); /* valFac=0.6 */
        {
            tmp = pLPCtmp[0];
            pLPCtmp[0] >>= 1;
            ippsSynthesisFilter_G729E_16s_I(pLPCtmp, LP_ORDER, pHFvec, SUBFRAME_SIZE_16k, st->asiSynthHighFilt + (LP_ORDER_16K - LP_ORDER));
            ippsCopy_16s(&pHFvec[SUBFRAME_SIZE_16k-LP_ORDER],st->asiSynthHighFilt + (LP_ORDER_16K - LP_ORDER),LP_ORDER);
            pLPCtmp[0] = tmp;
        }
    }

    /* noise High Pass filtering (1ms of delay) */
    HighPassFIR_AMRWB_16s_ISfs(pHFvec, SUBFRAME_SIZE_16k, st->pSHighPassFIRState);

    if (mode == IPP_SPCHBR_23850)
    {
        /* Low Pass filtering (7 kHz) */
       HighPassFIR_AMRWB_16s_ISfs(pHFvec, SUBFRAME_SIZE_16k, st->pSHighPassFIRState2);
    }
    /* add filtered pHFvec noise to speech synthesis */
    ippsAdd_16s(pHFvec, (Ipp16s*)pSynSignal, (Ipp16s*)pSynSignal, SUBFRAME_SIZE_16k);

    return;
}
static void ownBits2Prms(const Ipp8u *pBitstream, Ipp16s *pPrms , AMRWB_Rate_t rate)
{
    Ipp32s i;
    Ipp32s nBit = 0;

    for( i = 0; i < NumberPrmsTbl[rate]; i++) {
       pPrms[i] = amrExtractBits( (const Ipp8s **)&pBitstream, &nBit, pNumberBitsTbl[rate][i] );
    }
}
#if(0) /* never used. Removed to improve BullsEye Coverage*/
static void ownSynthesisDec_WBE(Ipp16s *pLPCvec, Ipp16s *pExcvec, Ipp16s valQNew,
   Ipp16u *pSynSignal, Ipp16s pPrmvec, Ipp16s *pHfIsfvec, IppSpchBitRate mode,
   Ipp16s valDTXState, AMRWBDecoder_Obj* st, Ipp16s bfi)
{
   IPP_ALIGNED_ARRAY(16, Ipp32s, synHiLo, (LP_ORDER + SUBFRAME_SIZE));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth, (SUBFRAME_SIZE));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_buff, (PITCH_LAG_MAX+2*SUBFRAME_SIZE));
   IPP_ALIGNED_ARRAY(16, Ipp16s, HF, (SUBFRAME_SIZE_16k)); /* High Frequency vector */
   IPP_ALIGNED_ARRAY(16, Ipp16s, Ap, (LP_ORDER_16K+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, HfA, (LP_ORDER_16K+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, exc_p, (SUBFRAME_SIZE));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_p, (SUBFRAME_SIZE));

   Ipp16s *synth_new, *synth_out;

   Ipp16s HF_corr_gain;
   Ipp16s HF_gain_ind;
   Ipp16s gain1, gain2;
   Ipp16s weight1, weight2;
   Ipp16s gain_hf_p;

   Ipp16s fac, wTmp, exp;
   Ipp16s ener, exp_ener;
   Ipp32s i, dwTmp;

   synth_new = synth_buff + PITCH_LAG_MAX + SUBFRAME_SIZE;
   synth_out = synth_buff + PITCH_LAG_MAX - UP_SAMPL_FILT_DELAY;
   ippsCopy_16s(st->mem_syn_out, synth_buff, (PITCH_LAG_MAX + SUBFRAME_SIZE));


   ippsCopy_32s(st->asiSynthesis, synHiLo, LP_ORDER);
   wTmp = pLPCvec[0];
   pLPCvec[0] = (Ipp16s)(pLPCvec[0] >> (4 + valQNew));
   //ippsSynthesisFilter_AMRWBE_16s32s_I(pLPCvec, LP_ORDER, pExcvec, synHiLo+LP_ORDER, SUBFRAME_SIZE);
   ippsSynthesisFilter_AMRWB_16s32s_I(pLPCvec, LP_ORDER, pExcvec, synHiLo+LP_ORDER, SUBFRAME_SIZE);
   pLPCvec[0] = wTmp;
   ippsCopy_32s(synHiLo+SUBFRAME_SIZE, st->asiSynthesis, LP_ORDER);

   ippsDeemphasize_AMRWB_32s16s(PREEMPH_FACTOR>>1, synHiLo+LP_ORDER, synth_new, SUBFRAME_SIZE, &(st->siDeemph));

   ippsHighPassFilter_AMRWB_16s_ISfs(synth_out, SUBFRAME_SIZE, st->pSHPFiltStateSgnlOut, 14);
   //ippsHighPassFilter_AMRWB_16s_ISfs(synth_new, SUBFRAME_SIZE, st->memSigOut, 14);

   ownOversampling_AMRWB_16s(synth_out, SUBFRAME_SIZE, pSynSignal, st->asiOversampFilt);
   //ownOversamp_16k(synth_new, SUBFRAME_SIZE, pSynSignal, st->mem_oversamp);

   if (st->ramp_state > 0) {
      st->ramp_state  = (Ipp16s)(st->ramp_state - 1);

      gain_hf_p = Cnvrt_NR_32s16s((st->gain_hf_plus * tblGainRampHF[st->ramp_state])<<1);

      for (i = 0; i < SUBFRAME_SIZE; i++) {
         exc_p[i] = Cnvrt_NR_32s16s(ShiftL_32s(gain_hf_p*pExcvec[i], 5));
      }

      ownSoftExcitationHF(exc_p, &(st->lp_amp_hf));

      wTmp = st->lpc_hf_plus[0];
      if (valQNew >= 0)
         st->lpc_hf_plus[0] = (Ipp16s)( st->lpc_hf_plus[0] >> valQNew );
      else
         st->lpc_hf_plus[0] = (Ipp16s)( st->lpc_hf_plus[0] << (-valQNew) );
      ippsSynthesisFilter_G729E_16s(st->lpc_hf_plus, 8, exc_p, synth_p, SUBFRAME_SIZE, st->mem_syn_hf_plus);
      ippsCopy_16s(&synth_p[SUBFRAME_SIZE-8], st->mem_syn_hf_plus, 8);
      st->lpc_hf_plus[0] = wTmp;

      ownSmoothEnergyHF(synth_p, &(st->threshold_hf));

      ippsUpsample_AMRWBE_16s(synth_p, HF, SUBFRAME_SIZE_16k, st->mem_oversamp_hf_plus, 1, 0);
   } else {
      ippsCopy_16s(synth_out, synth, SUBFRAME_SIZE);
      //ippsCopy_16s(synth_new, synth, SUBFRAME_SIZE);

      /* generate white noise vector */
      for (i=0; i<SUBFRAME_SIZE_16k; i++)
         HF[i] = (Ipp16s)(Rand_16s(&(st->siHFSeed)) >> 3);

      /* energy of excitation */
      ownScaleSignal_AMRWB_16s_ISfs(pExcvec, SUBFRAME_SIZE, -3);
      valQNew = (Ipp16s)(valQNew - 3);

      ippsDotProd_16s32s_Sfs(pExcvec, pExcvec, SUBFRAME_SIZE, &dwTmp, -1);
      dwTmp = Add_32s(dwTmp, 1);
      exp_ener = (Ipp16s)(30 - Norm_32s_I(&dwTmp));
      ener = (Ipp16s)(dwTmp >> 16);
      exp_ener = (Ipp16s)(exp_ener - (valQNew + valQNew) );

      /* set energy of white noise to energy of excitation */
      ippsDotProd_16s32s_Sfs(HF, HF, SUBFRAME_SIZE_16k, &dwTmp, -1);
      dwTmp = Add_32s(dwTmp, 1);
      exp = (Ipp16s)(30 - Norm_32s_I(&dwTmp));
      wTmp = (Ipp16s)(dwTmp>>16);

      if (wTmp > ener) {
         wTmp = (Ipp16s)(wTmp >> 1);
         exp = (Ipp16s)(exp + 1);
      }
      if (wTmp == ener) {
         dwTmp = (Ipp32s)IPP_MAX_16S << 16;
      } else {
         dwTmp = (Ipp32s)(((Ipp32s)wTmp << 15) / (Ipp32s)ener) << 16;
      }

      exp = (Ipp16s)(exp - exp_ener);
      InvSqrt_32s16s_I(&dwTmp, &exp);
      if ((exp+1)>0) {
         dwTmp = ShiftL_32s(dwTmp, (Ipp16u)(exp+1));
      } else {
         dwTmp = dwTmp >> (-(exp+1));
      }
      wTmp = (Ipp16s)(dwTmp>>16);

      for (i = 0; i < SUBFRAME_SIZE_16k; i++)
         HF[i] = (Ipp16s)((HF[i] * wTmp) >> 15);

      /* find tilt of synthesis speech (tilt: 1=voiced, -1=unvoiced) */
      ippsHighPassFilter_AMRWB_16s_ISfs(synth, SUBFRAME_SIZE, st->pSHPFiltStateSgnl400, 15);

      ippsDotProd_16s32s_Sfs(synth, synth, SUBFRAME_SIZE, &dwTmp, -1);
      dwTmp = Add_32s(dwTmp, 1);
      exp = Norm_32s_I(&dwTmp);
      ener = (Ipp16s)(dwTmp >> 16);

      ippsDotProd_16s32s_Sfs(synth, &synth[1], SUBFRAME_SIZE-1, &dwTmp, -1);
      dwTmp = Add_32s(dwTmp, 1);
      wTmp = (Ipp16s)( ShiftL_32s(dwTmp, exp)>>16 );
      if (wTmp > 0) {
         if (wTmp == ener)
            fac = IPP_MAX_16S;
         else
            fac = (Ipp16s)((wTmp << 15) / ener);
      } else {
         fac = 0;
      }

      /* modify energy of white noise according to synthesis tilt */
      gain1 = (Ipp16s)(32767 - fac);
      gain2 = (Ipp16s)((gain1 * 5) >> 3);
      gain2 = Cnvrt_32s16s(gain2 << 1);

      if (st->siVadHist > 0) {
         weight1 = 0;
         weight2 = IPP_MAX_16S;
      } else {
         weight1 = IPP_MAX_16S;
         weight2 = 0;
      }
      wTmp = (Ipp16s)((weight1 * gain1) >> 15);
      wTmp = (Ipp16s)(wTmp + ((weight2 * gain2) >> 15));

      if (wTmp != 0)
         wTmp = (Ipp16s)(wTmp + 1);
      if (wTmp < 3277)
         wTmp = 3277; /* 0.1 in Q15 */

      if ((mode == IPP_SPCHBR_23850) && (bfi == 0)) {
         /* HF correction gain */
         HF_gain_ind = pPrmvec;
         HF_corr_gain = HPGainTbl[HF_gain_ind];

         /* HF gain */
         for (i=0; i<SUBFRAME_SIZE_16k; i++)
            HF[i] = (Ipp16s)(((HF[i] * HF_corr_gain) >> 14 ) & 0xFFFE);
      } else {
         for (i=0; i<SUBFRAME_SIZE_16k; i++)
            HF[i] = (Ipp16s)((HF[i] * wTmp) >> 15);
      }

      if (((mode == IPP_SPCHBR_6600)||(mode == IPP_SPCHBR_DTX)) && (valDTXState == SPEECH)) {
         ownIsfExtrapolation(pHfIsfvec);
         ippsISPToLPC_AMRWB_16s(pHfIsfvec, HfA, LP_ORDER_16K);

         ippsMulPowerC_NR_16s_Sfs(HfA, 29491, Ap, LP_ORDER_16K+1, 15);
         wTmp = Ap[0];
         Ap[0] = (Ipp16s)(Ap[0] >> 1);
         ippsSynthesisFilter_G729E_16s_I(Ap, LP_ORDER_16K, HF, SUBFRAME_SIZE_16k, st->asiSynthHighFilt);
         ippsCopy_16s(&HF[SUBFRAME_SIZE_16k-LP_ORDER_16K], st->asiSynthHighFilt, LP_ORDER_16K);
         Ap[0] = wTmp;
      } else {
         /* synthesis of noise: 4.8kHz..5.6kHz --> 6kHz..7kHz */
         ippsMulPowerC_NR_16s_Sfs(pLPCvec, 19661, Ap, (LP_ORDER+1), 15);
         wTmp = Ap[0];
         Ap[0] = (Ipp16s)( Ap[0] >> 1);
         ippsSynthesisFilter_G729E_16s_I(Ap, LP_ORDER, HF, SUBFRAME_SIZE_16k, st->asiSynthHighFilt+(LP_ORDER_16K-LP_ORDER));
         ippsCopy_16s(&HF[SUBFRAME_SIZE_16k-LP_ORDER], st->asiSynthHighFilt+(LP_ORDER_16K-LP_ORDER), LP_ORDER);
         Ap[0] = wTmp;
      }

      /* noise High Pass filtering (1ms of delay) */
      HighPassFIR_AMRWB_16s_ISfs(HF, SUBFRAME_SIZE_16k, st->pSHighPassFIRState);

      if (mode == IPP_SPCHBR_23850) /* Low Pass filtering (7 kHz) {nb_bits >= NUMBITS23850} */
         HighPassFIR_AMRWB_16s_ISfs(HF, SUBFRAME_SIZE_16k, st->pSHighPassFIRState2);
   }

   /* add filtered HF noise to speech synthesis */
   ippsAdd_16s(HF, (Ipp16s*)pSynSignal, (Ipp16s*)pSynSignal, SUBFRAME_SIZE_16k);

   ippsCopy_16s(synth_buff+SUBFRAME_SIZE, st->mem_syn_out, (PITCH_LAG_MAX + SUBFRAME_SIZE));
}
#endif  /*Removed to improve BullsEye Coverage*/
