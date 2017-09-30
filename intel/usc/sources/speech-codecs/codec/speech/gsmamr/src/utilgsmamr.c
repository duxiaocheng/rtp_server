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
// Purpose: GSMAMR speech codec: common utilities.
//
*/

#include "owngsmamr.h"

/****************************************************************************
 *  Function: ownVADPitchDetection_GSMAMR()
 ***************************************************************************/
void ownVADPitchDetection_GSMAMR(IppGSMAMRVad1State *st, Ipp16s *pTimeVec,
                                 Ipp16s *vLagCountOld, Ipp16s *vLagOld)
{
   Ipp16s lagcount, i;
   lagcount = 0;

   for (i = 0; i < 2; i++) {
      if(Abs_16s(*vLagOld - pTimeVec[i]) < LOW_THRESHOLD) lagcount++;
      *vLagOld = pTimeVec[i];
   }

   st->pitchFlag >>= 1;
   if (*vLagCountOld + lagcount >= NUM_THRESHOLD)
      st->pitchFlag = (Ipp16s)(st->pitchFlag | 0x4000);

   *vLagCountOld = lagcount;
}

/***************************************************************************
 *   Function: ownUpdateLTPFlag_GSMAMR()
 *************************************************************************/
void ownUpdateLTPFlag_GSMAMR(IppSpchBitRate rate, Ipp32s L_Rmax, Ipp32s L_R0, Ipp16s *vFlagLTP)
{
    Ipp16s thresh;
    Ipp16s hi1;
    Ipp16s lo1;
    Ipp32s Ltmp;

    if ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150)) thresh = 18022;
    else if (rate == IPP_SPCHBR_10200) thresh = 19660;
    else thresh = 21299;

    hi1 = (Ipp16s)(L_R0 >> 16);
    lo1 = (Ipp16s)((L_R0 >> 1) & 0x7fff);
    Ltmp = (hi1 * thresh +  ((lo1 * thresh) >> 15));
    if (L_Rmax > 2*Ltmp) *vFlagLTP = 1;
    else                 *vFlagLTP = 0;

    return;
    /* End of ownUpdateLTPFlag_GSMAMR() */
}

static __ALIGN32 CONST Ipp16s TableLog2[33] = {
     0,  1455,  2866,  4236,  5568,  6863,  8124,  9352,
 10549, 11716, 12855, 13967, 15054, 16117, 17156, 18172,
 19167, 20142, 21097, 22033, 22951, 23852, 24735, 25603,
 26455, 27291, 28113, 28922, 29716, 30497, 31266, 32023,
 32767 };

 /*************************************************************************
 * Function:   ownLog2_GSMAMR_norm
 *************************************************************************/
void ownLog2_GSMAMR_norm(Ipp32s inVal, Ipp16s exp, Ipp16s *expPart, Ipp16s *fracPart)
{
  Ipp16s idx, a, tmp;
  Ipp32s outVal;

  if( inVal <= 0 ) {
    *expPart = 0;
    *fracPart = 0;
    return;
  }

  *expPart = (Ipp16s)(30 - exp);
  idx = (Ipp16s)(inVal >> 25);
  a = (Ipp16s)((inVal >> 10) & 0x7fff);
  idx   -=  32;

  if (idx < 0)
  {
	  /* Add this because Klockwork complains*/
	  *expPart = 0;
	  *fracPart = 0;
	  return;
  }

  outVal = TableLog2[idx] << 16;
  tmp = (Ipp16s)(TableLog2[idx] - TableLog2[idx+1]);
  outVal -= 2 * tmp * a;

  *fracPart =  (Ipp16s)(outVal >> 16);

  return;
  /* End of ownLog2_GSMAMR_norm() */
}
/*************************************************************************
 *  Function:  ownLog2_GSMAMR()
 *************************************************************************/
void ownLog2_GSMAMR(Ipp32s inVal, Ipp16s *expPart, Ipp16s *fracPart)
{
  Ipp16s exp;

  exp = Norm_32s_I(&inVal);
  ownLog2_GSMAMR_norm (inVal, exp, expPart, fracPart);

  return;
  /* End of ownLog2_GSMAMR() */
}

/*************************************************************************
 *  Function:  ownPow2_GSMAMR
 *************************************************************************/
static __ALIGN32 CONST Ipp16s TablePow2[33] = {
 16384, 16743, 17109, 17484, 17867, 18258, 18658, 19066,
 19484, 19911, 20347, 20792, 21247, 21713, 22188, 22674,
 23170, 23678, 24196, 24726, 25268, 25821, 26386, 26964,
 27554, 28158, 28774, 29405, 30048, 30706, 31379, 32066,
 32767 };

Ipp32s ownPow2_GSMAMR(Ipp16s expPart, Ipp16s fracPart)
{
  Ipp16s exp, idx, a, tmp;
  Ipp32s inVal, outVal, temp;

  exp = (Ipp16s)(30 - expPart);
  if (exp > 31) return 0;
  idx = (Ipp16s)(fracPart >> 10);
  a   = (Ipp16s)((fracPart << 5) & 0x7fff);

  inVal = TablePow2[idx] << 16;
  tmp = (Ipp16s)(TablePow2[idx] - TablePow2[idx+1]);
  inVal = inVal - 2*tmp*a;

  if (exp >= 31) outVal = (inVal < 0) ? -1 : 0;
  else           outVal = inVal >> exp;

  temp = 1 << (exp-1);
  if(inVal & temp) outVal++;

  return(outVal);
  /* End of ownPow2_GSMAMR() */
}

/* table[i] = sqrt((i+16)*2^-6) * 2^15, i.e. sqrt(x) scaled Q15 */
static __ALIGN32 CONST Ipp16s TableSqrt[49] =
{
 16384, 16888, 17378, 17854, 18318, 18770, 19212, 19644,
 20066, 20480, 20886, 21283, 21674, 22058, 22435, 22806,
 23170, 23530, 23884, 24232, 24576, 24915, 25249, 25580,
 25905, 26227, 26545, 26859, 27170, 27477, 27780, 28081,
 28378, 28672, 28963, 29251, 29537, 29819, 30099, 30377,
 30652, 30924, 31194, 31462, 31727, 31991, 32252, 32511,
 32767};

/********************************************************************************
 *  Function: ownSqrt_Exp_GSMAMR()
 ********************************************************************************/

Ipp32s ownSqrt_Exp_GSMAMR (Ipp32s inVal, Ipp16s *exp)
{
    Ipp16s idx, a, tmp;
    Ipp32s outVal;

    if (inVal <= 0) { *exp = 0; return 0; }

    outVal = inVal;
    *exp = (Ipp16s)(Norm_32s_I(&outVal) & 0xFFFE);
    inVal <<= *exp;

    idx = (Ipp16s)(inVal >> 25);
    a = (Ipp16s)((inVal >> 10) & 0x7fff);

    idx -= 16;
    outVal = TableSqrt[idx] << 16;
    tmp = (Ipp16s)(TableSqrt[idx] - TableSqrt[idx+1]);
    outVal = outVal - 2*tmp*a;

    return (outVal);
    /* End of ownSqrt_Exp_GSMAMR() */
}

/*************************************************************************
 *  Function: Reorder_lsf()
 *************************************************************************/
void ownReorderLSFVec_GSMAMR(Ipp16s *lsf, Ipp16s minDistance, Ipp16s len)
{
    Ipp32s i;
    Ipp16s lsf_min;

    lsf_min = minDistance;

    for (i = 0; i < len; i++)  {
        if(lsf[i] < lsf_min) lsf[i] = lsf_min;
        lsf_min = (Ipp16s)(lsf[i] + minDistance);
    }
}
/*************************************************************************
 *  Function: ownGetMedianElements_GSMAMR()
 *************************************************************************/
Ipp16s ownGetMedianElements_GSMAMR (Ipp16s *pPastGainVal, Ipp16s num)
{
    Ipp16s i, j, idx = 0;
    Ipp16s max;
    Ipp16s medianIndex;
    Ipp16s tmp[MAX_MED_SIZE];
    Ipp16s tmp2[MAX_MED_SIZE];

    ippsCopy_16s(pPastGainVal, tmp2, num);

    for (i = 0; i < num; i++) {
        max = -32767;
        for (j = 0; j < num; j++)  {
            if (tmp2[j] >= max) {
                max = tmp2[j];
                idx = j;
            }
        }
        tmp2[idx] = -32768;
        tmp[i] = idx;
    }
    medianIndex = tmp[num >> 1];

    return (pPastGainVal[medianIndex]);
}

/*************************************************************************
 *  Function: ownComputeCodebookGain_GSMAMR
 *************************************************************************/
Ipp16s ownComputeCodebookGain_GSMAMR (Ipp16s *pTargetVec, Ipp16s *pFltVec)
{
    Ipp16s i;
    Ipp16s xy, yy, exp_xy, exp_yy, gain;
    IPP_ALIGNED_ARRAY(16, Ipp16s, pFltVecScale, SUBFR_SIZE_GSMAMR);
    Ipp32s s;

    ippsRShiftC_16s(pFltVec, 1, pFltVecScale, SUBFR_SIZE_GSMAMR);
    ippsDotProd_16s32s_Sfs(pTargetVec, pFltVecScale, SUBFR_SIZE_GSMAMR, &s, 0);

    if(s == 0) s = 1;
    s <<= 1;
    exp_xy = Norm_32s_I(&s);
    xy = (Ipp16s)(s >> 16);
    if(xy <= 0) return ((Ipp16s) 0);

    ippsDotProd_16s32s_Sfs(pFltVecScale, pFltVecScale, SUBFR_SIZE_GSMAMR, &s, -1);

    exp_yy = Norm_32s_I(&s);
    yy = (Ipp16s)(s >> 16);
    xy >>= 1;
    //gain = (yy > 0)? (xy<<15)/yy : IPP_MAX_16S;
    if(yy > 0) gain = (Ipp16s)((xy<<15)/yy);
    else gain = IPP_MAX_16S;

    i = (Ipp16s)(exp_xy + 5);
    i = (Ipp16s)(i - exp_yy);

    gain = ShiftL_16s(Shr_16s(gain,i), 1);

    return (gain);
}
/*************************************************************************
 *  Function: ownGainAdaptAlpha_GSMAMR
 *************************************************************************/
void ownGainAdaptAlpha_GSMAMR(Ipp16s *vOnSetQntGain, Ipp16s *vPrevAdaptOut, Ipp16s *vPrevGainZero,
                              Ipp16s *a_LTPHistoryGain, Ipp16s ltpg, Ipp16s gainCode, Ipp16s *alpha)
{
    Ipp16s adapt;
    Ipp16s result;
    Ipp16s filt;
    Ipp16s tmp, i;
    Ipp32s temp;

    if (ltpg <= LTP_GAIN_LOG10_1) adapt = 0;
    else if (ltpg <= LTP_GAIN_LOG10_2) adapt = 1;
    else adapt = 2;

    tmp = (Ipp16s)(gainCode >> 1);
    if (gainCode & 1) tmp++;

    if (tmp > *vPrevGainZero && gainCode > 200) *vOnSetQntGain = 8;
    else if (*vOnSetQntGain != 0) *vOnSetQntGain -= 1;

    if ((*vOnSetQntGain != 0) && (adapt < 2)) adapt += 1;

    a_LTPHistoryGain[0] = ltpg;
    filt = ownGetMedianElements_GSMAMR(a_LTPHistoryGain, 5);

    if(adapt == 0) {
       if (filt > 5443)   result = 0;
       else if (filt < 0)  result = 16384;
       else {
           temp = (24660 * filt)>>13;
           result = (Ipp16s)(16384 - temp);
       }
    } else result = 0;

    if (*vPrevAdaptOut == 0) result >>= 1;
    *alpha = result;

    *vPrevAdaptOut = result;
    *vPrevGainZero = gainCode;

    for (i = NUM_MEM_LTPG-1; i > 0; i--)
        a_LTPHistoryGain[i] = a_LTPHistoryGain[i-1];

}
/**************************************************************************
*  Function: ownCBGainAverage_GSMAMR()
**************************************************************************/
Ipp16s ownCBGainAverage_GSMAMR(Ipp16s *a_GainHistory, Ipp16s *vHgAverageVar, Ipp16s *vHgAverageCount,
                              IppSpchBitRate rate, Ipp16s gainCode, Ipp16s *lsp, Ipp16s *lspAver,
                              Ipp16s badFrame, Ipp16s vPrevBadFr, Ipp16s potDegBadFrame, Ipp16s vPrevDegBadFr,
                              Ipp16s vBackgroundNoise, Ipp16s vVoiceHangover)
{
   Ipp16s i;
   Ipp16s cbGainMix, diff, bgMix, cbGainMean;
   Ipp32s L_sum;
   Ipp16s tmp[LP_ORDER_SIZE], tmp1, tmp2, shift1, shift2, shift;

   cbGainMix = gainCode;
   for (i = 0; i < (CBGAIN_HIST_SIZE-1); i++)
      a_GainHistory[i] = a_GainHistory[i+1];

   a_GainHistory[CBGAIN_HIST_SIZE-1] = gainCode;

   for (i = 0; i < LP_ORDER_SIZE; i++) {
      tmp1 = (Ipp16s)Abs_16s(lspAver[i] - lsp[i]);
      shift1 = (Ipp16s)(Exp_16s(tmp1) - 1);
      tmp1 <<= shift1;
      shift2 = Exp_16s(lspAver[i]);
      tmp2 = (Ipp16s)(lspAver[i] << shift2);
      //tmp[i] = (tmp2>0)? (tmp1<<15)/tmp2 : IPP_MAX_16S;
      if(tmp2 > 0) tmp[i] = (Ipp16s)((tmp1<<15)/tmp2);
      else tmp[i] = IPP_MAX_16S;

      shift = (Ipp16s)(shift1 + 2 - shift2);
      if(shift >= 0) tmp[i] >>= shift;
      else           tmp[i] <<= -shift;
   }

   ippsSum_16s_Sfs(tmp, LP_ORDER_SIZE, &diff, 0);

   if(diff > 5325) {
      *vHgAverageVar = (Ipp16s)(*vHgAverageVar + 1);
      if(*vHgAverageVar > 10) *vHgAverageCount = 0;
   } else *vHgAverageVar = 0;

   /* Compute mix constant (bgMix) */
   if ((rate <= IPP_SPCHBR_6700) || (rate == IPP_SPCHBR_10200))
   {
      /* if errors and presumed noise make smoothing probability stronger */
      if (((((potDegBadFrame != 0) && (vPrevDegBadFr != 0)) || (badFrame != 0) || (vPrevBadFr != 0)) &&
          (vVoiceHangover > 1) && (vBackgroundNoise != 0) &&
          ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150) || (rate == IPP_SPCHBR_5900)) ))
      {
         if (diff > 4506) {
            if (6554 < diff) bgMix = 8192;
            else             bgMix = (Ipp16s)((diff - 4506) << 2);
         }
         else bgMix = 0;
      } else {
         if (diff > 3277) {
            if(5325 < diff)  bgMix = 8192;
            else             bgMix = (Ipp16s)((diff - 3277) << 2);
         }
         else bgMix = 0;
      }

      if((*vHgAverageCount < 40) || (diff > 5325)) bgMix = 8192;
  
	 L_sum = 6554 * a_GainHistory[2] * 2;
     for (i = 3; i < CBGAIN_HIST_SIZE; i++)
      {
        L_sum = Mac_16s32s_v2(L_sum, 6554, a_GainHistory[i]);        
      }
     cbGainMean = Round_32s16s(L_sum);       

      if (((badFrame != 0) || (vPrevBadFr != 0)) && (vBackgroundNoise != 0) &&
          ((rate == IPP_SPCHBR_4750) ||
           (rate == IPP_SPCHBR_5150) ||
           (rate == IPP_SPCHBR_5900)) )
      {
         ippsSum_16s32s_Sfs(a_GainHistory, CBGAIN_HIST_SIZE, &L_sum, 0);
         L_sum*=4681;
         cbGainMean = (Ipp16s)((L_sum + 0x4000) >> 15);
      }

      L_sum = bgMix*cbGainMix;
      L_sum += 8192*cbGainMean;
      L_sum -= bgMix*cbGainMean;
      cbGainMix = (Ipp16s)((L_sum + 0x1000) >> 13);
   }

   *vHgAverageCount = Add_16s(*vHgAverageCount, 1);
   return cbGainMix;
}
/***************************************************************************
 *  Function:  ownCheckLSPVec_GSMAMR()                                                     *
 ***************************************************************************/
Ipp16s ownCheckLSPVec_GSMAMR(Ipp16s *count, Ipp16s *lsp)
{
   Ipp16s i, dist, dist_min1, dist_min2, dist_thresh;

   dist_min1 = IPP_MAX_16S;
   for (i = 3; i < LP_ORDER_SIZE-2; i++) {
      dist = (Ipp16s)(lsp[i] - lsp[i+1]);
      if(dist < dist_min1) dist_min1 = dist;
   }

   dist_min2 = IPP_MAX_16S;
   for (i = 1; i < 3; i++) {
      dist = (Ipp16s)(lsp[i] - lsp[i+1]);
      if (dist < dist_min2) dist_min2 = dist;
   }

   if(lsp[1] > 32000) dist_thresh = 600;
   else if (lsp[1] > 30500) dist_thresh = 800;
   else dist_thresh = 1100;

   if (dist_min1 < 1500 || dist_min2 < dist_thresh) *count = (Ipp16s)(*count + 1);
   else                                             *count = 0;

   if (*count >= 12) {
      *count = 12;
      return 1;
   } else return 0;
   /* End of ownCheckLSPVec_GSMAMR() */
}
/***************************************************************************
 *  Function:  ownSubframePostProc_GSMAMR()
 ***************************************************************************/
Ipp32s ownSubframePostProc_GSMAMR(Ipp16s *pSpeechVec, IppSpchBitRate rate, Ipp16s numSubfr,
                               Ipp16s gainPitch, Ipp16s gainCode, Ipp16s *pAQnt, Ipp16s *pLocSynth,
                               Ipp16s *pTargetPitchVec, Ipp16s *code, Ipp16s *pFltAdaptExc, Ipp16s *pFltVec,
                               Ipp16s *a_MemorySyn, Ipp16s *a_MemoryErr, Ipp16s *a_Memory_W0, Ipp16s *pLTPExc,
                               Ipp16s *vFlagSharp)
{
   Ipp16s i, j, k;
   Ipp16s temp;
   Ipp32s TmpVal;
   Ipp16s tempShift;
   Ipp16s kShift;
   Ipp16s pitchFactor;

   if (rate != IPP_SPCHBR_12200) {
      tempShift = 1;
      kShift = 2;
      pitchFactor = gainPitch;
   } else {
      tempShift = 2;
      kShift = 4;
      pitchFactor = (Ipp16s)(gainPitch >> 1);
   }

   *vFlagSharp = gainPitch;
   if(*vFlagSharp > PITCH_SHARP_MAX) *vFlagSharp = PITCH_SHARP_MAX;

   for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {	   
	   TmpVal = 2*((pLTPExc[i + numSubfr]*pitchFactor) + (code[i]*gainCode));	   
	   TmpVal = ShiftL_32s(TmpVal, tempShift);
	   pLTPExc[i + numSubfr] = (Ipp16s)((Add_32s (TmpVal, (int)0x00008000L)) >> 16);
  }

   ippsSynthesisFilter_NR_16s_Sfs(pAQnt, &pLTPExc[numSubfr], &pLocSynth[numSubfr], SUBFR_SIZE_GSMAMR, 12, a_MemorySyn);
   ippsCopy_16s(&pLocSynth[numSubfr + SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE], a_MemorySyn, LP_ORDER_SIZE);

   for (i = SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE, j = 0; i < SUBFR_SIZE_GSMAMR; i++, j++) {
      a_MemoryErr[j] = Sub_16s(pSpeechVec[numSubfr + i],pLocSynth[numSubfr + i]);
	  temp = (Ipp16s)((pFltAdaptExc[i]*gainPitch) >> 14);
      k = (Ipp16s)((ShiftL_32s((2*pFltVec[i]*gainCode), kShift))>>16);
      a_Memory_W0[j] = (Ipp16s)(pTargetPitchVec[i] - temp - k);
   }

   return 0;
   /* End of ownSubframePostProc_GSMAMR() */
}

static __ALIGN32 CONST Ipp16s TableInterN6[LEN_FIRFLT] =
{
    29443, 28346, 25207, 20449, 14701,  8693, 3143, -1352, -4402, -5865,
    -5850, -4673, -2783,  -672,  1211,  2536, 3130,  2991,  2259,  1170,
        0, -1001, -1652, -1868, -1666, -1147, -464,   218,   756,  1060,
     1099,   904,   550,   135,  -245,  -514, -634,  -602,  -451,  -231,
        0,   191,   308,   340,   296,   198,   78,   -36,  -120,  -163,
     -165,  -132,   -79,   -19,    34,    73,   91,    89,    70,    38,
     0
};

/*************************************************************************
 *  Function:   ownPredExcMode3_6_GSMAMR()
 *************************************************************************/
void ownPredExcMode3_6_GSMAMR(Ipp16s *pLTPExc, Ipp16s T0, Ipp16s frac, Ipp16s lenSubfr, Ipp16s flag3)
{
    Ipp16s i, j, k;
    Ipp16s *x0, *x1, *x2;
    const Ipp16s *c1, *c2;
    Ipp32s s;

    x0 = &pLTPExc[-T0];

    frac = (Ipp16s)(-frac);
    if(flag3 != 0) frac <<= 1;

    if (frac < 0) {
        frac += MAX_UPSAMPLING;
        x0--;
    }

    for (j = 0; j < lenSubfr; j++) {
        x1 = x0++;
        x2 = x0;
        c1 = &TableInterN6[frac];
        c2 = &TableInterN6[MAX_UPSAMPLING - frac];

        s = 0;
        for (i = 0, k = 0; i < LEN_INTERPOL_10; i++, k += MAX_UPSAMPLING) {
            s += x1[-i]*c1[k];
            s += x2[i]*c2[k];
        }
        pLTPExc[j] = (Ipp16s)((s + 0x4000) >> 15);
    }

    return;
    /* End of ownPredExcMode3_6_GSMAMR() */
}

static __ALIGN32 CONST Ipp16s TblPhImpLow_M795[] =
{
  26777,    801,   2505,   -683,  -1382,    582,    604,  -1274,
   3511,  -5894,   4534,   -499,  -1940,   3011,  -5058,   5614,
  -1990,  -1061,  -1459,   4442,   -700,  -5335,   4609,    452,
   -589,  -3352,   2953,   1267,  -1212,  -2590,   1731,   3670,
  -4475,   -975,   4391,  -2537,    949,  -1363,   -979,   5734,
  26777,    801,   2505,   -683,  -1382,    582,    604,  -1274,
   3511,  -5894,   4534,   -499,  -1940,   3011,  -5058,   5614,
  -1990,  -1061,  -1459,   4442,   -700,  -5335,   4609,    452,
   -589,  -3352,   2953,   1267,  -1212,  -2590,   1731,   3670,
   -4475,   -975,   4391,  -2537,    949,  -1363,   -979,   5734
};

static __ALIGN32 CONST Ipp16s TblPhImpLow[] =
{
  14690,  11518,   1268,  -2761,  -5671,   7514,    -35,  -2807,
  -3040,   4823,   2952,  -8424,   3785,   1455,   2179,  -8637,
   8051,  -2103,  -1454,    777,   1108,  -2385,   2254,   -363,
   -674,  -2103,   6046,  -5681,   1072,   3123,  -5058,   5312,
  -2329,  -3728,   6924,  -3889,    675,  -1775,     29,  10145,
  14690,  11518,   1268,  -2761,  -5671,   7514,    -35,  -2807,
  -3040,   4823,   2952,  -8424,   3785,   1455,   2179,  -8637,
   8051,  -2103,  -1454,    777,   1108,  -2385,   2254,   -363,
   -674,  -2103,   6046,  -5681,   1072,   3123,  -5058,   5312,
  -2329,  -3728,   6924,  -3889,    675,  -1775,     29,  10145
};
static __ALIGN32 CONST Ipp16s TblPhImpMid[] =
{
  30274,   3831,  -4036,   2972,  -1048,  -1002,   2477,  -3043,
   2815,  -2231,   1753,  -1611,   1714,  -1775,   1543,  -1008,
    429,   -169,    472,  -1264,   2176,  -2706,   2523,  -1621,
    344,    826,  -1529,   1724,  -1657,   1701,  -2063,   2644,
  -3060,   2897,  -1978,    557,    780,  -1369,    842,    655,
  30274,   3831,  -4036,   2972,  -1048,  -1002,   2477,  -3043,
   2815,  -2231,   1753,  -1611,   1714,  -1775,   1543,  -1008,
    429,   -169,    472,  -1264,   2176,  -2706,   2523,  -1621,
    344,    826,  -1529,   1724,  -1657,   1701,  -2063,   2644,
  -3060,   2897,  -1978,    557,    780,  -1369,    842,    655
};

/*************************************************************************
*  Proc:   ownPhaseDispersion_GSMAMR
**************************************************************************/
void ownPhaseDispersion_GSMAMR(sPhaseDispSt *state, IppSpchBitRate rate, Ipp16s *pLTPExcSignal,
                               Ipp16s cbGain, Ipp16s ltpGain, Ipp16s *innovVec, Ipp16s pitchFactor,
                               Ipp16s tmpShift)
{
   Ipp16s i, i1;
   Ipp16s tmp1;
   Ipp16s impNr;

   IPP_ALIGNED_ARRAY(16, Ipp16s, inno_sav, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, ps_poss, SUBFR_SIZE_GSMAMR);
   Ipp16s nze, nPulse;
   const Ipp16s *ph_imp;

   for(i = LTP_GAIN_MEM_SIZE-1; i > 0; i--)
       state->a_GainMemory[i] = state->a_GainMemory[i-1];

   state->a_GainMemory[0] = ltpGain;

   if(ltpGain < LTP_THRESH2) {    /* if (ltpGain < 0.9) */
       if(ltpGain > LTP_THRESH1) impNr = 1;
       else                      impNr = 0;
   } else impNr = 2;

   tmp1 = ShiftL_16s(state->vPrevGain, 1);
   if(cbGain > tmp1) state->vOnSetGain = 2;
   else {
      if (state->vOnSetGain > 0) state->vOnSetGain -= 1;
   }

   if(state->vOnSetGain == 0) {
       i1 = 0;
       for(i = 0; i < LTP_GAIN_MEM_SIZE; i++) {
           if(state->a_GainMemory[i] < LTP_THRESH1) i1 ++;
       }
       if(i1 > 2) impNr = 0;
   }

   if((impNr > state->vPrevState + 1) && (state->vOnSetGain == 0)) impNr -= 1;

   if((impNr < 2) && (state->vOnSetGain > 0)) impNr += 1;

   if(cbGain < 10) impNr = 2;

   if(state->vFlagLockGain == 1) impNr = 0;

   state->vPrevState = impNr;
   state->vPrevGain = cbGain;

   if((rate != IPP_SPCHBR_12200) && (rate != IPP_SPCHBR_10200) && (rate != IPP_SPCHBR_7400) && (impNr < 2)) {
       nze = 0;
       for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {
           if (innovVec[i] != 0) {
               ps_poss[nze] = i;
               nze++;
           }
           inno_sav[i] = innovVec[i];
           innovVec[i] = 0;
       }

       if(rate == IPP_SPCHBR_7950) {
          if(impNr == 0) ph_imp = TblPhImpLow_M795;
          else           ph_imp = TblPhImpMid;
       } else {
          if(impNr == 0) ph_imp = TblPhImpLow;
          else           ph_imp = TblPhImpMid;
       }

       for (nPulse = 0; nPulse < nze; nPulse++) {
           Ipp16s amp = inno_sav[ps_poss[nPulse]];
           const Ipp16s *posImp = &ph_imp[SUBFR_SIZE_GSMAMR-ps_poss[nPulse]];

           /*ippsMulC_16s_Sfs(&ph_imp[SUBFR_SIZE_GSMAMR-ps_poss[nPulse]],inno_sav[ps_poss[nPulse]],tmpV,L_SUBFR,15);
           ippsAdd_16s_I(tmpV,innovVec,SUBFR_SIZE_GSMAMR);*/
           for(i = 0; i < SUBFR_SIZE_GSMAMR; i++)
              innovVec[i] = (Ipp16s)(innovVec[i] + ((amp * posImp[i]) >> 15));
       }
   }

   ippsInterpolateC_NR_G729_16s_Sfs(innovVec, cbGain, pLTPExcSignal, pitchFactor,
                                    pLTPExcSignal, SUBFR_SIZE_GSMAMR, (15-tmpShift));
   return;
   /* End of ownPhaseDispersion_GSMAMR() */
}
/*************************************************************************
*  Function    : ownSourceChDetectorUpdate_GSMAMR
**************************************************************************/
Ipp16s ownSourceChDetectorUpdate_GSMAMR(Ipp16s *a_EnergyHistVector, Ipp16s *vCountHangover,
                                       Ipp16s *ltpGainHist, Ipp16s *pSpeechVec, Ipp16s *vVoiceHangover)
{
   Ipp16s i;
   Ipp16s prevVoiced, inbgNoise;
   Ipp16s temp;
   Ipp16s ltpLimit, frameEnergyMin;
   Ipp16s currEnergy, noiseFloor, maxEnergy, maxEnergyLastPart;
   Ipp32s s;

   s =  0;

   ippsDotProd_16s32s_Sfs(pSpeechVec, pSpeechVec, FRAME_SIZE_GSMAMR, &s, 0);

   s = ShiftL_32s(s,3);
   currEnergy = Extract_h_32s16s(s);
   frameEnergyMin = 32767;

   for(i = 0; i < ENERGY_HIST_SIZE; i++) {
      if(a_EnergyHistVector[i] < frameEnergyMin)
         frameEnergyMin = a_EnergyHistVector[i];
   }

   noiseFloor = (Ipp16s)Mul16_32s(frameEnergyMin);
   maxEnergy = a_EnergyHistVector[0];

   for(i = 1; i < ENERGY_HIST_SIZE-4; i++) {
      if(maxEnergy < a_EnergyHistVector[i])
         maxEnergy = a_EnergyHistVector[i];
   }

   maxEnergyLastPart = a_EnergyHistVector[2*ENERGY_HIST_SIZE/3];
   for(i = 2*ENERGY_HIST_SIZE/3+1; i < ENERGY_HIST_SIZE; i++) {
      if(maxEnergyLastPart < a_EnergyHistVector[i])
         maxEnergyLastPart = a_EnergyHistVector[i];
   }

   inbgNoise = 0;
   if((maxEnergy > LOW_NOISE_LIMIT) &&
      (currEnergy < THRESH_ENERGY_LIMIT) &&
      (currEnergy > LOW_NOISE_LIMIT) &&
      ((currEnergy < noiseFloor) || (maxEnergyLastPart < UPP_NOISE_LIMIT)))
   {
      if (*vCountHangover + 1 > 30) *vCountHangover = 30;
      else                          *vCountHangover = (Ipp16s)(*vCountHangover + 1);
   } else {
      *vCountHangover = 0;
   }

   if(*vCountHangover > 1) inbgNoise = 1;

   for (i = 0; i < ENERGY_HIST_SIZE-1; i++)
      a_EnergyHistVector[i] = a_EnergyHistVector[i+1];

   a_EnergyHistVector[ENERGY_HIST_SIZE-1] = currEnergy;

   ltpLimit = 13926;
   if(*vCountHangover > 8)  ltpLimit = 15565;
   if(*vCountHangover > 15) ltpLimit = 16383;

   prevVoiced = 0;
   if(ownGetMedianElements_GSMAMR(&ltpGainHist[4], 5) > ltpLimit) prevVoiced = 1;
   if(*vCountHangover > 20) {
      if(ownGetMedianElements_GSMAMR(ltpGainHist, 9) > ltpLimit)  prevVoiced = 1;
      else                                                        prevVoiced = 0;
   }

   if(prevVoiced) *vVoiceHangover = 0;
   else {
      temp = (Ipp16s)(*vVoiceHangover + 1);
      if(temp > 10) *vVoiceHangover = 10;
      else          *vVoiceHangover = temp;
   }

   return inbgNoise;
   /* End of ownSourceChDetectorUpdate_GSMAMR() */
}
/*************************************************************************
 Function: ownCalcUnFiltEnergy_GSMAMR
 *************************************************************************/
void ownCalcUnFiltEnergy_GSMAMR(Ipp16s *pLPResVec, Ipp16s *pLTPExc, Ipp16s *code, Ipp16s gainPitch,
                                Ipp16s lenSubfr, Ipp16s *fracEnergyCoeff, Ipp16s *expEnergyCoeff, Ipp16s *ltpg)
{
    Ipp32s s, TmpVal;
    Ipp16s i, exp, tmp;
    Ipp16s ltp_res_en, pred_gain;
    Ipp16s ltpg_exp, ltpg_frac;

    ippsDotProd_16s32s_Sfs(pLPResVec, pLPResVec, lenSubfr, &s, -1);

    if (s < 400) {
        fracEnergyCoeff[0] = 0;
        expEnergyCoeff[0] = -15;
    } else {
        exp = Norm_32s_I(&s);
        fracEnergyCoeff[0] = (Ipp16s)(s >> 16);
        expEnergyCoeff[0] = (Ipp16s)(15 - exp);
    }

    ippsDotProd_16s32s_Sfs(pLTPExc, pLTPExc, lenSubfr, &s, -1);
    exp = Normalization_32s(s);
	fracEnergyCoeff[1] = (Ipp16s)((s<<exp)>>16);
	expEnergyCoeff[1] = (Ipp16s)(15 - exp);

    ippsDotProd_16s32s_Sfs(pLTPExc, code, lenSubfr, &s, -1);
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[2] = (Ipp16s)(s >> 16);
    expEnergyCoeff[2] = (Ipp16s)(2 - exp);

    s = 0;
    for (i = 0; i < lenSubfr; i++) {        
		TmpVal = 4* pLTPExc[i] * gainPitch;
        tmp = (Ipp16s)(pLPResVec[i] - (Ipp16s)((Add_32s(TmpVal, (int) 0x00008000L))>>16));
        s = Mac_16s32s_v2(s, tmp, tmp); 
    }

    exp = Normalization_32s(s);
    ltp_res_en = (Ipp16s)((s<<exp)>>16);
    exp = (Ipp16s)(15 - exp);

    fracEnergyCoeff[3] = ltp_res_en;
    expEnergyCoeff[3] = exp;

    if(ltp_res_en > 0 && fracEnergyCoeff[0] != 0) {
        pred_gain = (Ipp16s)(((fracEnergyCoeff[0] >> 1) << 15) / ltp_res_en);
        exp = (Ipp16s)(exp - expEnergyCoeff[0]);
        TmpVal = pred_gain << 16;

        if(exp < -3) ShiftL_32s(TmpVal, (Ipp16s)(-(exp + 3)));
        else         TmpVal >>= (exp + 3);

        ownLog2_GSMAMR(TmpVal, &ltpg_exp, &ltpg_frac);

         TmpVal = 2*(((ltpg_exp - 27) << 15) + ltpg_frac);
		*ltpg = Cnvrt_NR_32s16s(ShiftL_32s(TmpVal, 13));
    }
    else *ltpg = 0;

    return;
    /* End of ownCalcUnFiltEnergy_GSMAMR() */
}

/*************************************************************************
 * Function: ownCalcFiltEnergy_GSMAMR
 *************************************************************************/
void ownCalcFiltEnergy_GSMAMR(IppSpchBitRate rate, Ipp16s *pTargetPitchVec, Ipp16s *pTargetVec,
                              Ipp16s *pFltAdaptExc, Ipp16s *pFltVec, Ipp16s *fracEnergyCoeff,
                              Ipp16s *expEnergyCoeff, Ipp16s *optFracCodeGain, Ipp16s *optExpCodeGain)
{

    Ipp16s g_coeff[4];
    Ipp32s s, ener_init=1;
    Ipp16s exp, frac;
    Ipp16s pFltVecTmp[SUBFR_SIZE_GSMAMR];
    Ipp16s gain;
		
    

    if(rate == IPP_SPCHBR_7950 || rate == IPP_SPCHBR_4750) ener_init = 0;

    ippsRShiftC_16s(pFltVec, 3, pFltVecTmp, SUBFR_SIZE_GSMAMR);
    ippsAdaptiveCodebookGainCoeffs_GSMAMR_16s(pTargetPitchVec, pFltAdaptExc, &gain, g_coeff);

    fracEnergyCoeff[0] = g_coeff[0];
    expEnergyCoeff[0]  = g_coeff[1];
    //fracEnergyCoeff[1] = (g_coeff[2] == IPP_MIN_16S) ? IPP_MAX_16S : -g_coeff[2];/* coeff[1] = -2 xn y1 */
    if(g_coeff[2] == IPP_MIN_16S) fracEnergyCoeff[1] = IPP_MAX_16S; /* coeff[1] = -2 xn y1 */
    else fracEnergyCoeff[1] = (Ipp16s)(-g_coeff[2]);
    expEnergyCoeff[1]  = (Ipp16s)(g_coeff[3] + 1);	
		
    ippsDotProd_16s32s_Sfs(pFltVecTmp, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[2] = (Ipp16s)(s >> 16);
    expEnergyCoeff[2] = (Ipp16s)(-3 - exp);

    ippsDotProd_16s32s_Sfs(pTargetPitchVec, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    s >>= 16;
    //fracEnergyCoeff[3] = (s != IPP_MIN_16S)? -s : IPP_MAX_16S;
    if(s != IPP_MIN_16S) fracEnergyCoeff[3] = (Ipp16s)(-s);
    else fracEnergyCoeff[3] = IPP_MAX_16S;
    expEnergyCoeff[3] = (Ipp16s)(7 - exp);

    ippsDotProd_16s32s_Sfs(pFltAdaptExc, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
    s += ener_init;
    exp = Norm_32s_I(&s);
    fracEnergyCoeff[4] = (Ipp16s)(s >> 16);
    expEnergyCoeff[4] = (Ipp16s)(7 - exp);

    if(rate == IPP_SPCHBR_4750 || rate == IPP_SPCHBR_7950) {
        ippsDotProd_16s32s_Sfs( pTargetVec, pFltVecTmp, SUBFR_SIZE_GSMAMR, &s, -1);
        s += ener_init;
        exp = Norm_32s_I(&s);
        frac = (Ipp16s)(s >> 16);
        exp = (Ipp16s)(6 - exp);

        if (frac <= 0) {
            *optFracCodeGain = 0;
            *optExpCodeGain = 0;
        } else {
            *optFracCodeGain = (Ipp16s)(((frac >> 1) << 15) / fracEnergyCoeff[2]);
            *optExpCodeGain = (Ipp16s)(exp - expEnergyCoeff[2] - 14);
        }
    }
    return;
    /* End of ownCalcFiltEnergy_GSMAMR() */
}

/*************************************************************************
 * Function: ownCalcTargetEnergy_GSMAMR
 *************************************************************************/
void ownCalcTargetEnergy_GSMAMR(Ipp16s *pTargetPitchVec, Ipp16s *optExpCodeGain, Ipp16s *optFracCodeGain)
{
    Ipp32s s;
    Ipp16s exp;

    ippsDotProd_16s32s_Sfs(pTargetPitchVec, pTargetPitchVec, SUBFR_SIZE_GSMAMR, &s, -1);
    exp = Norm_32s_I(&s);
    *optFracCodeGain = (Ipp16s)(s >> 16);
    *optExpCodeGain = (Ipp16s)(16 - exp);

    return;
    /* End of ownCalcTargetEnergy_GSMAMR() */
}
/**************************************************************************
*  Function:    : A_Refl
*  Convert from direct form coefficients to reflection coefficients
**************************************************************************/
void ownConvertDirectCoeffToReflCoeff_GSMAMR(Ipp16s *pDirectCoeff, Ipp16s *pReflCoeff)
{

   Ipp16s i,j;
   Ipp16s aState[LP_ORDER_SIZE];
   Ipp16s bState[LP_ORDER_SIZE];
   Ipp16s normShift, normProd;
   Ipp16s scale, temp, mult;
   Ipp32s const1;
   Ipp32s TmpVal, TmpVal1, TmpVal2;

   ippsCopy_16s(pDirectCoeff, aState, LP_ORDER_SIZE);
   for (i = LP_ORDER_SIZE-1; i >= 0; i--) {
      if(Abs_16s(aState[i]) >= 4096) goto ExitRefl;

      pReflCoeff[i] = (Ipp16s)(aState[i] << 3);
      TmpVal1 = 2 * pReflCoeff[i] * pReflCoeff[i];
      TmpVal = IPP_MAX_32S - TmpVal1;
      normShift = Norm_32s_Pos_I(&TmpVal);
      scale = (Ipp16s)(14 - normShift);
      normProd = Cnvrt_NR_32s16s(TmpVal);
      mult = (Ipp16s)((16384 << 15) / normProd);
      const1 = 1<<(scale - 1);

      for (j = 0; j < i; j++) {
         TmpVal = (aState[j] << 15) - (pReflCoeff[i] * aState[i-j-1]);
         temp = (Ipp16s)((TmpVal + 0x4000) >> 15);
         TmpVal1 = mult * temp;
         TmpVal2 = (TmpVal1 + const1) >> scale;

         if(TmpVal2 > 32767) goto ExitRefl;
         bState[j] = (Ipp16s)TmpVal2;
      }

      ippsCopy_16s(bState, aState, i);
   }
   return;

ExitRefl:
   ippsZero_16s(pReflCoeff, LP_ORDER_SIZE);
}
/**************************************************************************
*  Function    : ownScaleExcitation_GSMAMR
***************************************************************************/
void ownScaleExcitation_GSMAMR(Ipp16s *pInputSignal, Ipp16s *pOutputSignal)
{
    Ipp16s exp;
    Ipp16s gain_in, gain_out, g0;
    Ipp32s i, s;
    Ipp16s temp[SUBFR_SIZE_GSMAMR];

    ippsDotProd_16s32s_Sfs(pOutputSignal, pOutputSignal, SUBFR_SIZE_GSMAMR, &s, 0);
    if (s >= IPP_MAX_32S/2) {
        ippsRShiftC_16s(pOutputSignal, 2, temp, SUBFR_SIZE_GSMAMR);
        ippsDotProd_16s32s_Sfs(temp, temp, SUBFR_SIZE_GSMAMR, &s, -1);
    } else s >>= 3;

    if(s == 0) return;

    exp = (Ipp16s)(Exp_32s_Pos(s) - 1);
	gain_out = Round_32s16s(Shl_32s(s, exp));

    ippsDotProd_16s32s_Sfs(pInputSignal, pInputSignal, SUBFR_SIZE_GSMAMR, &s, 0);
    if (s >= IPP_MAX_32S/2) {
        ippsRShiftC_16s(pInputSignal, 2, temp, SUBFR_SIZE_GSMAMR);
        ippsDotProd_16s32s_Sfs(temp, temp, SUBFR_SIZE_GSMAMR, &s, -1);
    } else s >>= 3;

    if(s == 0) g0 = 0;
    else {
        i = Norm_32s_I(&s);
        gain_in = Cnvrt_NR_32s16s(s);
        exp = (Ipp16s)(exp - i);
        s = ((gain_out << 15) / gain_in);
        s = ShiftL_32s(s, 7);
        if (exp < 0) s = ShiftL_32s(s, (Ipp16u)(-exp));
        else         s >>= exp;
        ippsInvSqrt_32s_I(&s, 1);
        g0 = Cnvrt_NR_32s16s(ShiftL_32s(s, 9));
    }

    for (i = 0; i < SUBFR_SIZE_GSMAMR; i++)
        pOutputSignal[i] = (Ipp16s)(Mul16_32s(pOutputSignal[i] * g0) >> 16);

    return;
    /* End of ownScaleExcitation_GSMAMR() */
}

static const Ipp16s TablePredCoeff[NUM_PRED_TAPS] = {5571, 4751, 2785, 1556};
/* MEAN_ENER  = 36.0/constant, constant = 20*Log10(2)       */
#define MEAN_ENER_MR122  783741
static const Ipp16s TablePredCoeff_M122[NUM_PRED_TAPS] = {44, 37, 22, 12};

/*************************************************************************
 * Function:  ownPredEnergyMA_GSMAMR()
 *************************************************************************/
void ownPredEnergyMA_GSMAMR(Ipp16s *a_PastQntEnergy, Ipp16s *a_PastQntEnergy_M122, IppSpchBitRate rate,
                            Ipp16s *code, Ipp16s *expGainCodeCB, Ipp16s *fracGainCodeCB, Ipp16s *expEnergyCoeff,
                            Ipp16s *fracEnergyCoeff)
{
    Ipp32s ener_code, ener;
    Ipp16s exp, frac;
    ippsDotProd_16s32s_Sfs( code, code, SUBFR_SIZE_GSMAMR, &ener_code, -1);

    if (rate == IPP_SPCHBR_12200) {
        ener_code = 2 *((ener_code + 0x8000) >> 16) * 26214;
        ownLog2_GSMAMR(ener_code, &exp, &frac);
        ener_code = ((exp - 30) << 16) + (frac << 1);
        ippsDotProd_16s32s_Sfs(a_PastQntEnergy_M122, TablePredCoeff_M122, NUM_PRED_TAPS, &ener, -1);
        ener += MEAN_ENER_MR122;
        ener = (ener - ener_code) >> 1;                /* Q16 */
        Unpack_32s(ener, expGainCodeCB, fracGainCodeCB);
    } else {
        Ipp32s TmpVal;
        Ipp16s exp_code, predGainCB;

        exp_code = Norm_32s_I(&ener_code);
        ownLog2_GSMAMR_norm (ener_code, exp_code, &exp, &frac);
        TmpVal = Mpy_32_16(exp, frac, -24660);
        if(rate == IPP_SPCHBR_10200) TmpVal += 2 * 16678 * 64;
        else if (rate == IPP_SPCHBR_7950) {
           *fracEnergyCoeff = (Ipp16s) (ener_code>>16);
           *expEnergyCoeff = (Ipp16s)(-11 - exp_code);
           TmpVal += 2 * 17062 * 64;
        } else if (rate == IPP_SPCHBR_7400) {
            TmpVal += 2 * 32588 * 32;
        } else if (rate == IPP_SPCHBR_6700) {
            TmpVal += 2 * 32268 * 32;
        } else { /* MR59, MR515, MR475 */
            TmpVal += 2 * 16678 * 64;
        }

        TmpVal <<= 10;
        ippsDotProd_16s32s_Sfs(a_PastQntEnergy, TablePredCoeff, NUM_PRED_TAPS, &ener, -1);
        TmpVal += ener;
        predGainCB = (Ipp16s)(TmpVal>>16);
        if(rate == IPP_SPCHBR_7400) TmpVal = predGainCB * 5439;
        else                        TmpVal = predGainCB * 5443;
        TmpVal >>= 7;
        Unpack_32s(TmpVal, expGainCodeCB, fracGainCodeCB);
    }
    return;
    /* End of ownPredEnergyMA_GSMAMR() */
}
/*************************************************************************
*  Function:   ownCloseLoopFracPitchSearch_GSMAMR
***************************************************************************/
Ipp32s ownCloseLoopFracPitchSearch_GSMAMR(Ipp16s *vTimePrevSubframe, Ipp16s *a_GainHistory, IppSpchBitRate rate,
                                       Ipp16s frameOffset, Ipp16s *pLoopPitchLags, Ipp16s *pImpResVec,
                                       Ipp16s *pLTPExc, Ipp16s *pPredRes, Ipp16s *pTargetPitchVec, Ipp16s lspFlag,
                                       Ipp16s *pTargetVec, Ipp16s *pFltAdaptExc, Ipp16s *pExpPitchDel,
                                       Ipp16s *pFracPitchDel, Ipp16s *gainPitch, Ipp16s **ppAnalysisParam,
                                       Ipp16s *gainPitchLimit)
{
    Ipp16s i;
    Ipp16s index;
    Ipp32s TmpVal;
    Ipp16s subfrNum;
    Ipp16s sum = 0;

    subfrNum = (Ipp16s)(frameOffset / SUBFR_SIZE_GSMAMR);
    ippsAdaptiveCodebookSearch_GSMAMR_16s(pTargetPitchVec, pImpResVec, pLoopPitchLags,
                                          vTimePrevSubframe, (pLTPExc - MAX_OFFSET),
                                          pFracPitchDel, &index, pFltAdaptExc, subfrNum, rate);

	ippsConvPartial_16s_Sfs_amr_sat(pLTPExc, pImpResVec, pFltAdaptExc, SUBFR_SIZE_GSMAMR, 12);

    *pExpPitchDel = *vTimePrevSubframe;
    *(*ppAnalysisParam)++ = index;

    ippsAdaptiveCodebookGain_GSMAMR_16s(pTargetPitchVec, pFltAdaptExc, gainPitch);

    if(rate == IPP_SPCHBR_12200) *gainPitch &= 0xfffC;

    *gainPitchLimit = IPP_MAX_16S;
    if((lspFlag != 0) && (*gainPitch > PITCH_GAIN_CLIP)) {
       ippsSum_16s_Sfs(a_GainHistory, PG_NUM_FRAME, &sum, 0);
       sum = (Ipp16s)(sum + (*gainPitch >> 3));          /* Division by 8 */
       if(sum > PITCH_GAIN_CLIP) *gainPitchLimit = PITCH_GAIN_CLIP;
    }

   if ((rate == IPP_SPCHBR_4750) || (rate == IPP_SPCHBR_5150)) {
      if(*gainPitch > 13926) *gainPitch = 13926;
   } else {
       if(sum > PITCH_GAIN_CLIP) *gainPitch = PITCH_GAIN_CLIP;

       if(rate == IPP_SPCHBR_12200) {
           index = ownQntGainPitch_M122_GSMAMR(*gainPitchLimit, *gainPitch);
           *gainPitch = (Ipp16s)(TableQuantGainPitch[index] & 0xFFFC);
           *(*ppAnalysisParam)++ = index;
       }
   }

   /* update target vector und evaluate LTP residual

      Algorithmically is equivalent to :

      ippsMulC_16s_Sfs(pFltAdaptExc,*gainPitch,y1Tmp,SUBFR_SIZE_GSMAMR,14);
      ippsMulC_16s_Sfs(pLTPExc,*gainPitch,excTmp,SUBFR_SIZE_GSMAMR,14);
      ippsSub_16s(y1Tmp,pTargetPitchVec,pTargetVec,SUBFR_SIZE_GSMAMR);
      ippsSub_16s_I(excTmp,pPredRes,SUBFR_SIZE_GSMAMR);

      But !!! could not be used due to bitexactness.
   */

   for (i = 0; i < SUBFR_SIZE_GSMAMR; i++) {
       TmpVal = 4 * pFltAdaptExc[i] * *gainPitch;
       pTargetVec[i] = Sub_16s(pTargetPitchVec[i], TmpVal>>16);
       TmpVal = 4*pLTPExc[i] * *gainPitch;
       pPredRes[i] = Sub_16s(pPredRes[i], TmpVal>>16);
   }

   return 0;
   /* End of ownCloseLoopFracPitchSearch_GSMAMR() */
}

/*************************************************************************
 *
 *   Title: ef_update_lsf_p_CN
 *
 *   General Description: Update the reference LSF parameter vector. The reference
 *            vector is computed by averaging the quantized LSF parameter
 *            vectors which exist in the LSF parameter history.
 *
 *            inputs:      lsf_old[0..EF_DTX_HANGOVER-1][0..LPCORDER-1]
 *                                 LSF parameter history
 *
 *            outputs:     lsf_p_CN[0..9]   Computed reference LSF parameter vector
 *
 *  Input variables:
 *  Output variables:
 *  Local Variables:
 *  Global Variables:
 *
 *  Function Specifics:
 *************************************************************************/

void ef_update_lsf_p_CN(
    Ipp16s ef_lsf_old[EF_DTX_HANGOVER][EF_LPCORDER],
    Ipp16s ef_lsf_p_CN[EF_LPCORDER]
)
{
    Ipp16s i, j;
    Ipp32s L_temp;

    
    for (j = 0; j < EF_LPCORDER; j++)
    {
        L_temp = EF_INV_DTX_HANGOVER * ef_lsf_old[0][j] * 2;
        for (i = 1; i < EF_DTX_HANGOVER; i++)
        {
            L_temp += EF_INV_DTX_HANGOVER *ef_lsf_old[i][j] * 2;
        }
        ef_lsf_p_CN[j] = (L_temp + 0x00008000L)>>16;
    }
    
}

/*************************************************************************
 *
 *   Title: ef_update_lsf_history
 *
 *   General Description: Update the LSF parameter history. The LSF
 *            parameters kept in the buffer are used later for computing
 *            the reference LSF parameter vector and the averaged LSF
 *            parameter vector.
 *
 *   Input variables:  ef_lsf1[0..9] LSF vector of the 1st half of the frame
 *                     ef_lsf2[0..9]    LSF vector of the 2nd half of the frame
 *                     ef_lsf_old[0..EF_DTX_HANGOVER-1][0..LPCORDER-1]
 *                              Old LSF history
 *
 *   Output variables: ef_lsf_old[0..EF_DTX_HANGOVER-1][0..LPCORDER-1]
 *                              Updated LSF history
 *  Local Variables:
 *  Global Variables:
 *
 *  Function Specifics: *
 *************************************************************************/

void ef_update_lsf_history(
    const Ipp16s ef_lsf1[EF_LPCORDER],
    const Ipp16s ef_lsf2[EF_LPCORDER],
    Ipp16s ef_lsf_old[EF_DTX_HANGOVER][EF_LPCORDER],
    Ipp16s *next_lsf_old
)
{
    Ipp16s i, temp;    

    /* Store new LSF data to lsf_old buffer */
    for (i = 0; i < EF_LPCORDER; i++)
    {
        temp = (Ipp16s)((ef_lsf1[i] >> 1) + (ef_lsf2[i] >> 1));
        ef_lsf_old[*next_lsf_old][i] = temp;
    }

    *next_lsf_old = *next_lsf_old + 1;
    if((*next_lsf_old - EF_DTX_HANGOVER) >= 0)
        *next_lsf_old = 0;
    
}

/*************************************************************************
*  Title: amr_Int_lpc_1and3
*  General Description: Interpolates the LSPs and converts to LPC
*               parameters to get a different LP filter in each subframe.
*  Input variables:
*  Output variables:
*  Return value:
*  Local Variables:
*  Global Variables:
*
*  Function Specifics: The 20 ms speech frame is divided into 4 subframes.
*                The LSPs are quantized and transmitted at the 2nd and
*                4th subframes (twice per frame) and interpolated at the
*                1st and 3rd subframe.
*
*                      |------|------|------|------|
*                         sf1    sf2    sf3    sf4
*                   F0            Fm            F1
*
*                 sf1:   1/2 Fm + 1/2 F0         sf3:   1/2 F1 + 1/2 Fm
*                 sf2:       Fm                  sf4:       F1
*  Returns     : void
*
*
**************************************************************************/

void amr_Int_lpc_1and3(
    const Ipp16s lsp_old[],   /* i : LSP vector at the 4th subfr. of past frame (AMR_M)     */
    const Ipp16s lsp_mid[],   /* i : LSP vector at the 2nd subfr. of present frame (AMR_M)  */
    const Ipp16s lsp_new[],   /* i : LSP vector at the 4th subfr. of present frame (AMR_M)  */
    Ipp16s Az[]                 /* o : interpolated LP parameters in all subfr. (AMR_AZ_SIZE) */
)
{
    Ipp16s i;
    Ipp16s lsp[LP_ORDER_SIZE];

    
    /* lsp[i] = lsp_mid[i] * 0.5 + lsp_old[i] * 0.5 */

    for (i = 0; i < LP_ORDER_SIZE; i++)
    {
        lsp[i] = (lsp_mid[i] >> 1) + (lsp_old[i] >> 1);
    }

    ippsLSPToLPC_GSMAMR_16s(lsp, Az);        /* Subframe 1 */
    Az += LP1_ORDER_SIZE;

    ippsLSPToLPC_GSMAMR_16s(lsp_mid, Az);    /* Subframe 2 */
    Az += LP1_ORDER_SIZE;

    for (i = 0; i < LP_ORDER_SIZE; i++)
    {
        lsp[i] = (lsp_mid[i] >> 1) + (lsp_new[i] >> 1);
    }

    ippsLSPToLPC_GSMAMR_16s(lsp, Az);	/* Subframe 3 */
    Az += LP1_ORDER_SIZE;

    ippsLSPToLPC_GSMAMR_16s(lsp_new, Az);    /* Subframe 4 */
}

/*************************************************************************
 *
 *   Title: ef_pseudonoise
 *
 *   General Description: Generate a random integer value to use in comfort noise
 *            generation. The algorithm uses polynomial x^31 + x^3 + 1
 *            (length of PN sequence is 2^31 - 1).
 *
 *           NOTES:  For EFR, no_bits is either 1 or 2.
 *
 *           inputs:      *shift_reg    Old CN generator shift register state
 *           outputs:     *shift_reg    Updated CN generator shift register state
 *           return value: Generated random integer value
 *
 *  Input variables:
 *  Output variables:
 *  Local Variables:
 *  Global Variables:
 *
 *  Function Specifics:*
 *************************************************************************/

Ipp16s ef_pseudonoise( /* (o) : result */
    Ipp32s shift_reg[1],   /* (i) : pointer to shift egister */
    Ipp16s no_bits         /* (i) : number of bits to get */
)
{
    Ipp16s noise_bits, Sn, i;

    
    noise_bits = 0;
    for (i = 0; i < no_bits; i++)
    {
        /* State n == 31 */

        if ((*shift_reg & 0x00000001L) != 0)
        {
            Sn = 1;
        }
        else
        {
            Sn = 0;
        }

        /* State n == 3 */

        if ((*shift_reg & 0x10000000L) != 0)
        {
            Sn = Sn ^ 1;
        }
        else
        {
            Sn = Sn ^ 0;
        }

        noise_bits = noise_bits << 1;
        noise_bits = noise_bits | (Extract_l_32s16s(*shift_reg) & 1);


        *shift_reg = *shift_reg >> 1;

        if (Sn & 1)
        {
            *shift_reg = *shift_reg | 0x40000000L;
        }
    }

    return noise_bits;
}



/*************************************************************************
 *
 *   Title: ef_build_CN_code
 *
 *   General Description: Compute the comfort noise fixed codebook excitation. The
 *            gains of the pulses are always +/-1.
 *
 *   Input variables:      *seed         Old CN generator shift register state
 *
 *   Output variables:     ef_cod[0..39]    Generated comfort noise fixed codebook vector
 *                *seed         Updated CN generator shift register state
 *
 *      Local Variables:
 *      Global Variables:
 *
 *  Function Specifics :
 *************************************************************************/

void ef_build_CN_code(
    Ipp16s ef_cod[EF_L_SUBFR],
    Ipp32s seed[1]
)
{
    Ipp16s i, j, k;

    for (i = 0; i < EF_L_SUBFR; i++)
    {
        ef_cod[i] = 0;
    }

    for (k = 0; k < EF_NB_PULSE; k++)
    {
        i = ef_pseudonoise(seed, 2);       /* generate pulse position */
        i = Extract_l_32s16s((i * 10 * 2)) >> 1;
        i = i + k;

        j = ef_pseudonoise(seed, 1);       /* generate sign           */


        if (j > 0)
        {
            ef_cod[i] = 4096;
        }
        else
        {
            ef_cod[i] = -4096;
        }
    }    
}

/*************************************************************************
 *
 *   Title: ef_update_gcode0_CN
 *
 *   General Description: Update the reference fixed codebook gain parameter value.
 *            The reference value is computed by averaging the quantized
 *            fixed codebook gain parameter values which exist in the
 *            fixed codebook gain parameter history.
 *
 *   Input variables:      ef_gain_code_old[0..4*EF_DTX_HANGOVER-1]
 *                              fixed codebook gain parameter history
 *
 *   Output variables:     return Computed reference fixed codebook gain
 *   Local Variables:
 *   Global Variables:
 *
 *  Function Specifics:
 *************************************************************************/

Ipp16s ef_update_gcode0_CN(
    const Ipp16s ef_gain_code_old[4 * EF_DTX_HANGOVER]
)
{
    Ipp16s i, j;
    Ipp32s L_temp, L_ret;

    L_ret = 0L;
    for (i = 0; i < EF_DTX_HANGOVER; i++)
    {
        L_temp = 0x1fff * ef_gain_code_old[4 * i] * 2;
        for (j = 1; j < 4; j++)
        {
             L_temp +=  0x1fff * ef_gain_code_old[4 * i + j] * 2;
        }
        L_ret += EF_INV_DTX_HANGOVER * Extract_h_32s16s(L_temp) * 2;
    }

    return Extract_h_32s16s(L_ret);
}


/*************************************************************************
 *
 *   Title: ef_update_gain_code_history
 *
 *   General Description: Update the fixed codebook gain parameter history of the
 *            encoder. The fixed codebook gain parameters kept in the buffer
 *            are used later for computing the reference fixed codebook
 *            gain parameter value and the averaged fixed codebook gain
 *            parameter value.
 *
 *   Input variables:      new_gain_code   New fixed codebook gain value
 *
 *                         ef_gain_code_old[0..4*EF_DTX_HANGOVER-1]
 *                                Old fixed codebook gain history of encoder
 *
 *   Output variables:     gain_code_old[0..4*EF_DTX_HANGOVER-1]
 *                                Updated fixed codebk gain history of encoder
 *   Local Variables:
 *   Global Variables:
 *
 *   Function Specifics: *
 *************************************************************************/

void ef_update_gain_code_history(
    Ipp16s new_gain_code,
    Ipp16s ef_gain_code_old[4 * EF_DTX_HANGOVER],
    Ipp16s *buf_p_p      /* pointer for gain code history update in tx */
)
{

    /* Circular buffer */
    ef_gain_code_old[*buf_p_p] = new_gain_code;


    if (Sub_16s(*buf_p_p, (4 * EF_DTX_HANGOVER - 1)) == 0)
    {
        *buf_p_p = 0;
    }
    else
    {
        *buf_p_p = Add_16s(*buf_p_p, 1);
    }

}




