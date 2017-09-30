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
// Purpose: AMRWB speech codec: common utilities.
//
*/

#include "ownamrwb.h"

__ALIGN32 CONST IppSpchBitRate Mode2RateTbl[10] = {
   IPP_SPCHBR_6600,      /* 6.60 kbps */
   IPP_SPCHBR_8850,      /* 8.85 kbps */
   IPP_SPCHBR_12650,     /* 12.65 kbps */
   IPP_SPCHBR_14250,     /* 14.25 kbps */
   IPP_SPCHBR_15850,     /* 15.85 kbps */
   IPP_SPCHBR_18250,     /* 18.25 kbps */
   IPP_SPCHBR_19850,     /* 19.85 kbps */
   IPP_SPCHBR_23050,     /* 23.05 kbps */
   IPP_SPCHBR_23850,     /* 23.85 kbps */
   IPP_SPCHBR_DTX
};

Ipp16s amrExtractBits( const Ipp8s **pBitStrm, Ipp32s *currBitstrmOffset, Ipp32s numParamBits )
{
    Ipp32s  i;
    Ipp32s  unpackedParam = 0;

    for ( i = 0; i < numParamBits; i ++ ){
        Ipp32s  lTemp;
        lTemp = ((*pBitStrm)[(i + *currBitstrmOffset)>>3] >> ((7 - i - *currBitstrmOffset) & 0x7)) & 1;
        unpackedParam <<= 1;
        unpackedParam +=  lTemp;
    }

    *pBitStrm += (numParamBits + *currBitstrmOffset)>>3;
    *currBitstrmOffset = (numParamBits + *currBitstrmOffset) & 0x7;

    return (Ipp16s)unpackedParam;
}

/* Find the voicing factor (1=voice to -1=unvoiced) */
Ipp16s ownVoiceFactor(Ipp16s *pPitchExc, Ipp16s valExcFmt, Ipp16s valPitchGain,
                      Ipp16s *pFixCdbkExc, Ipp16s valCodeGain, Ipp32s len)
{
   Ipp16s valExpDiff, sTmp, valExp, valEner, valExp1, valEner2, valExp2, valVoiceFac;
   Ipp32s s;

   ippsDotProd_16s32s_Sfs( pPitchExc, pPitchExc, len, &s, -1);
   s = Add_32s(s, 1);

   valExp1 = (Ipp16s)(30 - Norm_32s_I(&s));
   valEner = (Ipp16s)(s>>16);
   valExp1 = (Ipp16s)(valExp1 - (valExcFmt << 1));

   s = (valPitchGain * valPitchGain) << 1;
   valExp = Norm_32s_I(&s);
   sTmp = (Ipp16s)(s>>16);

   valEner = Mul_16s_Sfs(valEner, sTmp, 15);
   valExp1 = (Ipp16s)(valExp1 - (valExp + 10));

   ippsDotProd_16s32s_Sfs( pFixCdbkExc, pFixCdbkExc,len, &s, -1);
   s = Add_32s(s, 1);

   valExp2 = (Ipp16s)(30 - Norm_32s_I(&s));
   valEner2 = (Ipp16s)(s>>16);

   valExp = Exp_16s(valCodeGain);
   sTmp = (Ipp16s)(valCodeGain << valExp);
   sTmp = Mul_16s_Sfs(sTmp, sTmp, 15);
   valEner2 = Mul_16s_Sfs(valEner2, sTmp, 15);
   valExp2 = (Ipp16s)(valExp2 - (valExp << 1));

   valExpDiff = (Ipp16s)(valExp1 - valExp2);

   if (valExpDiff >= 0) {
      valEner  = (Ipp16s)(valEner >> 1);
      valEner2 = (Ipp16s)(valEner2 >> (valExpDiff + 1));
   } else {
      if ((1 - valExpDiff) >= 15) {
         valEner = 0;
      } else {
         valEner = (Ipp16s)(valEner >> (1 - valExpDiff));
      }
      valEner2 = (Ipp16s)(valEner2 >> 1);
   }

   valVoiceFac = (Ipp16s)(valEner - valEner2);
   valEner = (Ipp16s)(valEner + (valEner2 + 1));

   if (valVoiceFac >= 0) {
      valVoiceFac = (Ipp16s)((valVoiceFac << 15) / valEner);
   } else {
      valVoiceFac = (Ipp16s)(-(((-valVoiceFac) << 15) / valEner));
   }

   return (valVoiceFac);
}

/* 2.0 - 6.4 kHz phase dispersion */
static __ALIGN32 CONST Ipp16s impPhaseDispLowTbl[SUBFRAME_SIZE] =
{
    20182,  9693,  3270, -3437,  2864, -5240,  1589, -1357,   600,
     3893, -1497,  -698,  1203, -5249,  1199,  5371, -1488,  -705,
    -2887,  1976,   898,   721, -3876,  4227, -5112,  6400, -1032,
    -4725,  4093, -4352,  3205,  2130, -1996, -1835,  2648, -1786,
     -406,   573,  2484, -3608,  3139, -1363, -2566,  3808,  -639,
    -2051,  -541,  2376,  3932, -6262,  1432, -3601,  4889,   370,
      567, -1163, -2854,  1914,    39, -2418,  3454,  2975, -4021,
     3431
};

/* 3.2 - 6.4 kHz phase dispersion */
static __ALIGN32 CONST Ipp16s impPhaseDispMidTbl[SUBFRAME_SIZE] =
{
    24098, 10460, -5263,  -763,  2048,  -927, 1753, -3323,  2212,
      652, -2146,  2487, -3539,  4109, -2107,  -374, -626,  4270,
    -5485,  2235,  1858, -2769,   744,  1140,  -763, -1615, 4060,
    -4574,  2982, -1163,   731, -1098,   803,   167,  -714,  606,
     -560,   639,    43, -1766,  3228, -2782,   665,   763,  233,
    -2002,  1291,  1871, -3470,  1032,  2710, -4040,  3624,-4214,
     5292, -4270,  1563,   108,  -580,  1642, -2458,   957,  544,
     2540
};

void ownPhaseDispInit(Ipp16s *pDispvec)
{
    ippsZero_16s(pDispvec, 8);
}

void ownPhaseDisp(Ipp16s valCodeGain, Ipp16s valPitchGain, Ipp16s *pFixCdbkExc,
                  Ipp16s valLevelDisp, Ipp16s *pDispvec)
{
    Ipp16s valState;
    Ipp16s *pPrevPitchGain, *pPrevCodeGain, *pPrevState;
    Ipp32s i, j;
    IPP_ALIGNED_ARRAY (16, Ipp16s, pCodevec, SUBFRAME_SIZE*2);

    pPrevState = pDispvec;
    pPrevCodeGain = pDispvec + 1;
    pPrevPitchGain = pDispvec + 2;

    ippsZero_16s(pCodevec, SUBFRAME_SIZE*2);

    if (valPitchGain < MIN_GAIN_PITCH)
        valState = 0;
    else if (valPitchGain < THRESH_GAIN_PITCH)
        valState = 1;
    else
        valState = 2;

    ippsMove_16s(pPrevPitchGain, &pPrevPitchGain[1], 5);
    pPrevPitchGain[0] = valPitchGain;

    if ((valCodeGain - *pPrevCodeGain) > (*pPrevCodeGain << 1)) {
        if (valState < 2) valState += 1;
    } else {
        j = 0;
        for (i = 0; i < 6; i++)
            if (pPrevPitchGain[i] < MIN_GAIN_PITCH) j += 1;
        if (j > 2) valState = 0;
        if (valState > (*pPrevState + 1)) valState -= 1;
    }

    *pPrevCodeGain = valCodeGain;
    *pPrevState = valState;

    valState = (Ipp16s)(valState + valLevelDisp);
    if (valState == 0)
    {
        for (i = 0; i < SUBFRAME_SIZE; i++)
        {
            if (pFixCdbkExc[i] != 0)
            {
                for (j = 0; j < SUBFRAME_SIZE; j++)
                {
                    pCodevec[i + j] = (Ipp16s)(pCodevec[i + j] + ((pFixCdbkExc[i] * impPhaseDispLowTbl[j] + 0x00004000) >> 15));
                }
            }
        }
    } else if (valState == 1)
    {
        for (i = 0; i < SUBFRAME_SIZE; i++)
        {
            if (pFixCdbkExc[i] != 0)
            {
                for (j = 0; j < SUBFRAME_SIZE; j++)
                {
                    pCodevec[i + j] = (Ipp16s)(pCodevec[i + j] + ((pFixCdbkExc[i] * impPhaseDispMidTbl[j] + 0x00004000) >> 15));
                }
            }
        }
    }
    if (valState < 2)
        ippsAdd_16s(pCodevec, &pCodevec[SUBFRAME_SIZE], pFixCdbkExc, SUBFRAME_SIZE);
}

/*-------------------------------------------------------------------*
 * Decimate a vector by 2 with 2nd order fir filter.                 *
 *-------------------------------------------------------------------*/

#define L_FIR  5
#define L_MEM  (L_FIR-2)
static __ALIGN32 CONST Ipp16s hFirTbl[L_FIR] = {4260, 7536, 9175, 7536, 4260};

void ownLPDecim2(Ipp16s *pSignal, Ipp32s len, Ipp16s *pMem)
{
    Ipp16s *pSignalPtr;
    Ipp32s i, j, s, k;
    IPP_ALIGNED_ARRAY (16, Ipp16s, pTmpvec, FRAME_SIZE + L_MEM);

    ippsCopy_16s(pMem, pTmpvec, L_MEM);
    ippsCopy_16s(pSignal, &pTmpvec[L_MEM], len);
    ippsCopy_16s(&pSignal[len-L_MEM], pMem, L_MEM);

    for (i = 0, j = 0; i < len; i += 2, j++)
    {
        pSignalPtr = &pTmpvec[i];
        //ippsDotProd_AMRWB_16s32s_Sfs(pSignalPtr, hFirTbl, L_FIR, &s, -1);
        s = 0;
        for (k=0; k<L_FIR; k++)
           s += pSignalPtr[k] * hFirTbl[k];
        pSignal[j] = Cnvrt_NR_32s16s(s<<1);
    }
}

/* Conversion of 16th-order 12.8kHz ISF vector into 20th-order 16kHz ISF vector */
#define INV_LENGTH 2731

void ownIsfExtrapolation(Ipp16s *pHfIsf)
{
    IPP_ALIGNED_ARRAY (16, Ipp16s, pIsfDiffvec, LP_ORDER - 2);
    Ipp32s i, s, pIsfCorrvec[3];
    Ipp16s valCoeff, valMean, valExp, valExp2, valHigh, valLow, valMaxCorr;
    Ipp16s sTmp, tmp2, tmp3;

    pHfIsf[LP_ORDER_16K - 1] = pHfIsf[LP_ORDER - 1];
    ippsSub_16s(pHfIsf, &pHfIsf[1], pIsfDiffvec, (LP_ORDER - 2));

    s = 0;
    for (i = 3; i < (LP_ORDER - 1); i++)
        s += pIsfDiffvec[i - 1] * INV_LENGTH;
    valMean = Cnvrt_NR_32s16s(s<<1);

    pIsfCorrvec[0] = 0;
    pIsfCorrvec[1] = 0;
    pIsfCorrvec[2] = 0;

    ippsMax_16s(pIsfDiffvec, (LP_ORDER - 2), &sTmp);
    valExp = Exp_16s(sTmp);
    ippsLShiftC_16s(pIsfDiffvec, valExp, pIsfDiffvec, (LP_ORDER - 2));
    valMean <<= valExp;
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = (Ipp16s)(pIsfDiffvec[i] - valMean);
        tmp3 = (Ipp16s)(pIsfDiffvec[i - 2] - valMean);
        //s = tmp2 * tmp3;
        s = (tmp2 * tmp3) << 1;
        //valHigh = (Ipp16s)(s >> 15);
        //valLow = (Ipp16s)(s & 0x7fff);
        Unpack_32s(s, &valHigh, &valLow);
        //s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15)) << 1;
        s = (valHigh*valHigh + (((valHigh*valLow)>>15)<<1) ) << 1;
        //s = Mpy_32(valHigh, valLow, valHigh, valLow);
        pIsfCorrvec[0] += s;
    }
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = (Ipp16s)(pIsfDiffvec[i] - valMean);
        tmp3 = (Ipp16s)(pIsfDiffvec[i - 3] - valMean);
        s = tmp2 * tmp3;
        valHigh = (Ipp16s)(s >> 15);
        valLow = (Ipp16s)(s & 0x7fff);
        //s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15)) << 1;
        s = (valHigh*valHigh + (((valHigh*valLow)>>15)<<1) ) << 1;
        pIsfCorrvec[1] += s;
    }
    for (i = 7; i < (LP_ORDER - 2); i++)
    {
        tmp2 = (Ipp16s)(pIsfDiffvec[i] - valMean);
        tmp3 = (Ipp16s)(pIsfDiffvec[i - 4] - valMean);
        s = tmp2 * tmp3;
        valHigh = (Ipp16s)(s >> 15);
        valLow = (Ipp16s)(s & 0x7fff);
        //s = (valHigh*valHigh + ((valHigh*valLow + valLow*valHigh)>>15) ) << 1;
        s = (valHigh*valHigh + (((valHigh*valLow)>>15)<<1) ) << 1;
        pIsfCorrvec[2] += s;
    }

    if (pIsfCorrvec[0] > pIsfCorrvec[1])
        valMaxCorr = 0;
    else
        valMaxCorr = 1;

    if (pIsfCorrvec[2] > pIsfCorrvec[valMaxCorr]) valMaxCorr = 2;
    valMaxCorr += 1;

    for (i = LP_ORDER - 1; i < (LP_ORDER_16K - 1); i++)
    {
        sTmp = (Ipp16s)(pHfIsf[i - 1 - valMaxCorr] - pHfIsf[i - 2 - valMaxCorr]);
        pHfIsf[i] = (Ipp16s)(pHfIsf[i - 1] + sTmp);
    }

    sTmp = (Ipp16s)(pHfIsf[4] + pHfIsf[3]);
    sTmp = (Ipp16s)(pHfIsf[2] - sTmp);
    sTmp = Mul_16s_Sfs(sTmp, 5461, 15);
    sTmp += 20390;
    if (sTmp > 19456) sTmp = 19456;
    sTmp = (Ipp16s)(sTmp - pHfIsf[LP_ORDER - 2]);
    tmp2 = (Ipp16s)(pHfIsf[LP_ORDER_16K - 2] - pHfIsf[LP_ORDER - 2]);

    valExp2 = Exp_16s(tmp2);
    valExp = Exp_16s(sTmp);
    valExp -= 1;
    sTmp <<= valExp;
    tmp2 <<= valExp2;
    valCoeff = (Ipp16s)((sTmp << 15) / tmp2);
    valExp = (Ipp16s)(valExp2 - valExp);

    for (i = LP_ORDER - 1; i < (LP_ORDER_16K - 1); i++)
    {
        sTmp = (Ipp16s)(((pHfIsf[i] - pHfIsf[i - 1]) * valCoeff) >> 15);
        if (valExp > 0)
            pIsfDiffvec[i - (LP_ORDER - 1)] = (Ipp16s)(sTmp << valExp);
        else
            pIsfDiffvec[i - (LP_ORDER - 1)] = (Ipp16s)(sTmp >> (-valExp));
    }

    for (i = LP_ORDER; i < (LP_ORDER_16K - 1); i++)
    {
        sTmp = (Ipp16s)((pIsfDiffvec[i - (LP_ORDER - 1)] + pIsfDiffvec[i - LP_ORDER]) - 1280);
        if (sTmp < 0)
        {
            if (pIsfDiffvec[i - (LP_ORDER - 1)] > pIsfDiffvec[i - LP_ORDER])
                pIsfDiffvec[i - LP_ORDER] = (Ipp16s)(1280 - pIsfDiffvec[i - (LP_ORDER - 1)]);
            else
                pIsfDiffvec[i - (LP_ORDER - 1)] = (Ipp16s)(1280 - pIsfDiffvec[i - LP_ORDER]);
        }
    }
    for (i = LP_ORDER-1; i < (LP_ORDER_16K-1); i++)
        pHfIsf[i] = (Ipp16s)(Add_16s(pHfIsf[i-1], pIsfDiffvec[i-(LP_ORDER-1)]));

    ownMulC_16s_ISfs(26214, pHfIsf, LP_ORDER_16K-1, 15);

    ippsISFToISP_AMRWB_16s(pHfIsf, pHfIsf, LP_ORDER_16K);

    return;
}

Ipp16s ownGpClip(IppSpchBitRate irate, Ipp16s *pMem)
{
    Ipp16s clip;
    Ipp16s thres;

    clip = 0;
    if( (irate == IPP_SPCHBR_6600) || (irate == IPP_SPCHBR_8850) )
   	{
   		/* clipping is activated when filtered pitch gain > threshold (0.94 to 1 in Q14) */
   		/* thres = 0.9f + (0.1f*mem[0]/DIST_ISF_MAX); */
   		thres = (Ipp16s)(pMem[0] * (Ipp16s)(16384/DIST_ISF_MAX_IO));
   		thres = ((Ipp32s)1638 * thres) >> 14;
   		thres = 14746 + thres;

   		if (pMem[1] > thres)
   			clip = 1;
   	}
   	else if ((pMem[0] < THRESH_DISTANCE_ISF) && (pMem[1] > THRESH_GAIN_PITCH))
        clip = 1;

    return (clip);
}

void ownCheckGpClipIsf(IppSpchBitRate irate, Ipp16s *pIsfvec, Ipp16s *pMem)
{
    Ipp16s valDist, valDistMin;
    Ipp32s i;

    valDistMin = (Ipp16s)(pIsfvec[1] - pIsfvec[0]);
    for (i = 2; i < LP_ORDER - 1; i++)
    {
        valDist = (Ipp16s)(pIsfvec[i] - pIsfvec[i - 1]);
        if (valDist < valDistMin) valDistMin = valDist;
    }

    valDist = (Ipp16s)((26214 * pMem[0] + 6554 * valDistMin) >> 15);
    if( (irate == IPP_SPCHBR_6600) || (irate == IPP_SPCHBR_8850) )
 	{
 		if (valDist > DIST_ISF_MAX_IO)
 		{
 			valDist = DIST_ISF_MAX_IO;
 		}
 	}
 	else
 	{
        if (valDist > MAX_DISTANCE_ISF) {
        	valDist = MAX_DISTANCE_ISF;
        }
 	}
    pMem[0] = valDist;
}

void ownCheckGpClipPitchGain(IppSpchBitRate irate, Ipp16s valPitchGain, Ipp16s *pMem)
{
    Ipp16s valGain;
    Ipp32s s;

    if( (irate == IPP_SPCHBR_6600) || (irate == IPP_SPCHBR_8850) )
 	{
 		/* long term LTP gain average (>250ms) */
 		/* gain = 0.98*mem[1] + 0.02*gain_pit; */
        s = 32113 * pMem[1];
        s += 655 * valPitchGain;
 	}
 	else
 	{
        s = 29491 * pMem[1];
        s += 3277 * valPitchGain;
 	}
    valGain = (Ipp16s)(s>>15);

    if (valGain < MIN_GAIN_PITCH) valGain = MIN_GAIN_PITCH;
    pMem[1] = valGain;
}

void ownSynthesisFilter_AMRWB_16s32s_ISfs
       ( const Ipp16s *pSrcLpc, Ipp32s valLpcOrder, const Ipp16s *pSrcExcitation,
         Ipp32s *pSrcDstSignal, Ipp32s len, Ipp32s scaleFactor )
{
//    IPP_BAD_PTR3_RET(pSrcLpc, pSrcExcitation, pSrcDstSignal);
//    IPP_BAD_SIZE_RET(valLpcOrder);
//    IPP_BAD_SIZE_RET(len);

#if(ForBullsEyeOnly)  /* Move calling ippsSynthesisFilter_AMRWB_16s32s_I here for better BullsEye coverage*/
	if (scaleFactor == 3)
	{
		ippsSynthesisFilter_AMRWB_16s32s_I(pSrcLpc, valLpcOrder, pSrcExcitation, pSrcDstSignal, len);
	}
	else
#endif
    {
      Ipp32s i, j, s, s1;
      Ipp16s sTmp;
      for (i = 0; i < len; i++)
      {
        s = 0;
        for (j = 1; j <= valLpcOrder; j++) {
           sTmp = (Ipp16s)((pSrcDstSignal[i - j]>>4)&0x0FFF);
           s -= sTmp * pSrcLpc[j];
        }

        s >>= 11;

        s1 = pSrcExcitation[i] * pSrcLpc[0];

        for (j = 1; j <= valLpcOrder; j++)
            s1 -= (pSrcDstSignal[i - j]>>16) * pSrcLpc[j];

        s = Add_32s(s, Mul2_32s(s1));
        s = ShiftL_32s(s, (Ipp16u)scaleFactor);

        pSrcDstSignal[i] = s;
      }
    }
}

#define NC16k_AMRWB 10
#define ownLSPPolinomials(pLSP,shift, pPolinom,len,ScaleFactor)\
{\
    Ipp16s ownHigh, ownLow;\
    Ipp32s ownI, ownJ, n, k, valTmp;\
\
    pPolinom[0] = 1 << (21 + ScaleFactor);\
    pPolinom[1] = (-pLSP[0+shift]) << (7 + ScaleFactor);\
\
    for(ownI=2,k=2; ownI<=len; ownI++,k+=2){\
        pPolinom[ownI] = pPolinom[ownI-2];\
        for(ownJ=1,n=ownI; ownJ<ownI; ownJ++,n--) {\
            ownHigh = (Ipp16s)(pPolinom[n-1] >> 16);\
            ownLow = (Ipp16s)((pPolinom[n-1] >> 1) & 0x7fff);\
            valTmp = ownHigh * pLSP[k+shift] + ((ownLow * pLSP[k+shift])>>15);\
            pPolinom[n] -= 4*valTmp;\
            pPolinom[n] += pPolinom[n-2];\
        }\
        pPolinom[n] -=  pLSP[k+shift] << (7 + ScaleFactor);\
    }\
}

#define GetLPCFromPoli(pDstLpc,len,lenBy2,f1,f2,scaleFactor,RoundOperation,tmax)\
{\
   Ipp32s i_GetLPCFromPoli,j_GetLPCFromPoli;\
   Ipp32s lTemporaryVar;\
\
   pDstLpc[0] = 4096;\
   tmax = 1;\
   for (i_GetLPCFromPoli = 1, j_GetLPCFromPoli = len - 1; i_GetLPCFromPoli < lenBy2; i_GetLPCFromPoli++, j_GetLPCFromPoli--) {\
      lTemporaryVar = Add_32s(f1[i_GetLPCFromPoli],f2[i_GetLPCFromPoli]);\
      tmax |= lTemporaryVar;\
      pDstLpc[i_GetLPCFromPoli] = RoundOperation((lTemporaryVar + (1 << scaleFactor)) >> (scaleFactor+1));\
\
      lTemporaryVar = f1[i_GetLPCFromPoli] - f2[i_GetLPCFromPoli];\
      tmax |= Abs_32s(lTemporaryVar);\
      pDstLpc[j_GetLPCFromPoli] = RoundOperation((lTemporaryVar + (1 << scaleFactor)) >> (scaleFactor+1));\
   }\
}

void ownISPToLPC_AMRWB_16s
       ( const Ipp16s *pSrcIsp, Ipp16s *pDstLpc, Ipp32s len, Ipp32s adaptive_scaling )
{
    Ipp16s sHighPart, sLowPart, sq, sq_sug;
    Ipp32s i, j, nc, iTmp, tmax;
    IPP_ALIGNED_ARRAY(16, Ipp32s, f1, NC16k_AMRWB + 1);
    IPP_ALIGNED_ARRAY(16, Ipp32s, f2, NC16k_AMRWB);

//    IPP_BAD_PTR2_RET(pSrcIsp, pDstLpc)
//    IPP_BAD_SIZE_RET(len);
//    IPP_BADARG_RET( (len > 20), ippStsSizeErr )

    nc = len >> 1;
    if (nc > 8) {
        ownLSPPolinomials(pSrcIsp,0,f1,nc,0);
        ippsLShiftC_32s(f1, 2, f1, nc);
        f1[nc] = ShiftL_32s(f1[nc], 2);
        ownLSPPolinomials(pSrcIsp,1,f2,nc-1,0);
        ippsLShiftC_32s(f2, 2, f2, nc);
    } else {
        ownLSPPolinomials(pSrcIsp,0,f1,nc,2);
        ownLSPPolinomials(pSrcIsp,1,f2,nc-1,2);
    }

    for (i = nc - 1; i > 1; i--) {
        f2[i] -= f2[i - 2];
    }

    for (i = 0; i < nc; i++) {
        Unpack_32s(f1[i], &sHighPart, &sLowPart);
        iTmp = Mpy_32_16(sHighPart, sLowPart, pSrcIsp[len - 1]);
        f1[i] = Add_32s(f1[i], iTmp);

        Unpack_32s(f2[i], &sHighPart, &sLowPart);
        iTmp = Mpy_32_16(sHighPart, sLowPart, pSrcIsp[len - 1]);
        f2[i] -= iTmp;
    }
    GetLPCFromPoli(pDstLpc,len,nc,f1,f2,11,(Ipp16s),tmax);

    /* rescale data if overflow has occurred and reprocess the loop */

    if ( adaptive_scaling == 1 )
       sq = (Ipp16s)(4 - Exp_32s(tmax));
    else
       sq = 0;

    if (sq > 0)
    {
      sq_sug = (Ipp16s)(sq + 12);
      for (i = 1, j = (len - 1); i < nc; i++, j--)
        {
          iTmp = Add_32s(f1[i], f2[i]);
          pDstLpc[i] = (Ipp16s)((iTmp + (1 << (sq_sug-1))) >> sq_sug);

          iTmp = f1[i] - f2[i];
          pDstLpc[j] = (Ipp16s)((iTmp + (1 << (sq_sug-1))) >> sq_sug);
        }
      pDstLpc[0] >>= sq;
    }
    else
    {
      sq_sug = 12;
      sq     = 0;
    }

    Unpack_32s(f1[nc], &sHighPart, &sLowPart);
    iTmp = Mpy_32_16(sHighPart, sLowPart, pSrcIsp[len - 1]);

    iTmp = Add_32s(f1[nc], iTmp);
    pDstLpc[nc] = (Ipp16s)((iTmp + (1 << 11)) >> sq_sug);

    pDstLpc[len] = ShiftR_NR_16s(pSrcIsp[len - 1], (Ipp16s)(sq+3));
}
#if(0) /* never used. Removed to improve BullsEye Coverage*/
void ownSoftExcitationHF(Ipp16s *exc_hf, Ipp32s *mem)
{
   Ipp32s lp_amp, Ltmp, Ltmp1;
   Ipp16s i, hi, lo;

   lp_amp = *mem;

   for (i=0; i<SUBFRAME_SIZE; i++) {
      Ltmp = ShiftL_32s(Abs_16s(exc_hf[i]),15);/* clearing sign bit*/

      Unpack_32s(Ltmp, &hi, &lo);
      Ltmp1 = Mpy_32_16(hi, lo, 655);

      Unpack_32s(lp_amp, &hi, &lo);
      lp_amp = Add_32s(Ltmp1, Mpy_32_16(hi, lo, 32112));

      Ltmp = Ltmp - ShiftL_32s(lp_amp,1);

      if (Ltmp <= 0) {
         ;
      } else {
         lp_amp = Add_32s(lp_amp, (Ltmp>>1));
         if (exc_hf[i] >= 0) {
            exc_hf[i]= Sub_16s(exc_hf[i], Mul2_16s(Cnvrt_NR_32s16s(Ltmp)));
         } else {
            exc_hf[i] = Add_16s(exc_hf[i], Mul2_16s(Cnvrt_NR_32s16s(Ltmp)));
         }
      }
   }
   *mem = lp_amp;
}

void ownSmoothEnergyHF(Ipp16s *HF, Ipp32s *threshold)
{
   Ipp32s Ltmp, Lener;
   Ipp16s i, HF_tmp[SUBFRAME_SIZE], hi, lo;
   Ipp16s exp_ener, exp_tmp, frac_ener, frac_tmp;

   ippsCopy_16s(HF, HF_tmp, SUBFRAME_SIZE);
   ownScaleSignal_AMRWB_16s_ISfs(HF_tmp, SUBFRAME_SIZE, -2); /* <14 bits -> <12 bits */

   Lener = 1;
   for (i=0; i<SUBFRAME_SIZE; i++) {
      Ltmp = Mul2_32s(HF_tmp[i] * HF_tmp[i]);
      Lener = Add_32s(Lener, Ltmp);
   }

   Ltmp = Lener;

   if (Ltmp < (*threshold)) { /* add 1.5 dB and saturate to threshold */
      Unpack_32s(Ltmp, &hi, &lo);
      Ltmp = ShiftL_32s(Mpy_32_16(hi, lo, 23169),1);
      if (Ltmp > (*threshold) ) {
         Ltmp = *threshold;
      }
   } else { /* substract 1.5 dB and saturate to threshold */
      Unpack_32s(Ltmp, &hi, &lo);
      Ltmp = Mpy_32_16(hi, lo, 23174);
      if (Ltmp < (*threshold)) {
         Ltmp = *threshold;
      }
   }
   /* set the threshold for next subframer to the current modified energy */
   *threshold = Ltmp;

   if(Ltmp == 0) {
      Ltmp = 1;
   }

   /* apply correction scale factor to HF signal */
   exp_ener = Norm_32s_I(&Lener);
   frac_ener = (Ipp16s)(Lener >> 16);

   exp_tmp = Norm_32s_I(&Ltmp);
   frac_tmp = (Ipp16s)(Ltmp >> 16);

   if(frac_ener > frac_tmp) {
      frac_ener =  (Ipp16s)(frac_ener >> 1);
      exp_ener = (Ipp16s)(exp_ener - 1);
   }

   if (frac_ener == frac_tmp) {
      frac_tmp = IPP_MAX_16S;
   } else {
      frac_tmp = (Ipp16s)((frac_ener << 15) / frac_tmp);
   }
   exp_tmp = (Ipp16s)(exp_ener - exp_tmp);

   exp_ener = (Ipp16s)(exp_tmp - 1);
   Ltmp = ((Ipp32s)frac_tmp)<<16;
   InvSqrt_32s16s_I(&Ltmp, &exp_tmp);

   frac_tmp = Cnvrt_NR_32s16s(Ltmp);
   exp_tmp = (Ipp16s)(1 + exp_tmp + exp_ener);

   if (exp_tmp <= 0) {
      for (i=0; i<SUBFRAME_SIZE; i++) {
         Ltmp  = Mul2_32s(HF[i] * frac_tmp);
         HF[i] = Cnvrt_NR_32s16s(Ltmp >> (-exp_tmp));
      }
   } else {
      for (i=0; i<SUBFRAME_SIZE; i++) {
         Ltmp  = Mul2_32s(HF[i] * frac_tmp);
         HF[i] = Cnvrt_NR_32s16s(ShiftL_32s(Ltmp, exp_tmp));
      }
   }
}
#endif /* never used. Removed to improve BullsEye Coverage*/
