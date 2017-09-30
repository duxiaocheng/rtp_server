/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2012 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: G.729/A/B/D/E/EV speech codec: Miscellaneous.
//
*/

#include <ipps.h>
#include "owng7291.h"
#include <ippdefs.h>

static int ownLag_max(const short *pSrc, short expHi, short expLo, short lagMax, short lagMin,
                        short *pCorrMax, short *pVoicing)
{
  int max32, acc32, tmp32, acc132, pitMax;
//  short expHi, expLo;
  short maxHi, maxLo, enerHi, enerLo;

  ippsAutoCorrLagMax_Inv_16s(pSrc, G7291_LFRAME4, lagMin, lagMax, &max32, &pitMax);
  ippsDotProd_16s32s_Sfs(&pSrc[-pitMax], &pSrc[-pitMax], G7291_LFRAME4, &acc32, -1);
  tmp32 = acc32;
  ippsInvSqrt_32s_I(&acc32, 1);

  L_Extract(max32, &maxHi, &maxLo);
  L_Extract(acc32, &enerHi, &enerLo);

  acc32 = Mpy_32(maxHi, maxLo, enerHi, enerLo);
  *pCorrMax = (short)acc32;
//  ippsDotProd_16s32s_Sfs(pSrc, pSrc, G7291_LFRAME4, &acc132, -1);

  L_Extract(tmp32, &enerHi, &enerLo);
//  L_Extract(acc132, &expHi, &expLo);
  acc132 = Mpy_32(expHi, expLo, enerHi, enerLo) >> 1;
  ippsInvSqrt_32s_I(&acc132, 1);
  L_Extract(acc132, &enerHi, &enerLo);
  acc32 = Mpy_32(maxHi, maxLo, enerHi, enerLo);
  *pVoicing = (short)acc32;

  return (pitMax);
}

void ownOpenLoopPitchSearch_G7291_16s(const Ipp16s *pSrc, Ipp16s *pVoicing, Ipp16s *pBestLag)
{
  short *pScaledBuf;
  int acc32, pitMax1, pitMax2, pitMax3;
  IPP_ALIGNED_ARRAY(16, short, scaledBuf, G7291_LFRAME4 + G7291_PITCH_LAG_MAX);
  short max1, max2, max3, thresh;
  short expHi, expLo;

//  IPP_BAD_PTR3_RET(pSrc, pVoicing, pBestLag);

  pScaledBuf = &scaledBuf[G7291_PITCH_LAG_MAX];
  ippsDotProd_16s32s_Sfs(pSrc-G7291_PITCH_LAG_MAX, pSrc-G7291_PITCH_LAG_MAX,
                           G7291_LFRAME4+G7291_PITCH_LAG_MAX, &acc32, 0);
  if (acc32 > IPP_MAX_32S/2) {
    ippsRShiftC_16s(pSrc-G7291_PITCH_LAG_MAX,3,(pScaledBuf-G7291_PITCH_LAG_MAX),G7291_LFRAME4+G7291_PITCH_LAG_MAX);
  }
  else {
       if ( acc32 < 524288 ) {
        ippsLShiftC_16s(pSrc-G7291_PITCH_LAG_MAX,3,(pScaledBuf-G7291_PITCH_LAG_MAX),G7291_LFRAME4+G7291_PITCH_LAG_MAX);
      }else{
        ippsCopy_16s(pSrc-G7291_PITCH_LAG_MAX,(pScaledBuf-G7291_PITCH_LAG_MAX),G7291_LFRAME4+G7291_PITCH_LAG_MAX);
      }
   }
  ippsDotProd_16s32s_Sfs(pScaledBuf, pScaledBuf, G7291_LFRAME4, &acc32, -1);
  L_Extract(acc32, &expHi, &expLo);

  pitMax1 = ownLag_max(pScaledBuf, expHi, expLo, G7291_PITCH_LAG_MAX, G7291_PITCH_LAG_MIN*4, &max1, &pVoicing[0]);
  pitMax2 = ownLag_max(pScaledBuf, expHi, expLo, G7291_PITCH_LAG_MIN*4-1, G7291_PITCH_LAG_MIN*2, &max2, &pVoicing[1]);
  pitMax3 = ownLag_max(pScaledBuf, expHi, expLo, G7291_PITCH_LAG_MIN*2-1, G7291_PITCH_LAG_MIN, &max3, &pVoicing[2]);

  if(Abs_16s((short)(*pBestLag - pitMax2)) < 10)
  {
    thresh = 22938;
  }
  else
  {
    thresh = 29491;
  }
  if (((max1 * thresh)>>15) < max2)
  {
    max1 = max2;
    pitMax1 = pitMax2;
  }

  if(Abs_16s((short)(*pBestLag - pitMax3)) < 5)
  {
    thresh = 22938;
  }
  else
  {
    thresh = 29491;
  }
  if (((max1 * thresh)>>15) < max3)
  {
    pitMax1 = pitMax3;
  }
  *pBestLag = (short)pitMax1;

  if (pVoicing[1] > pVoicing[0])
  {
    pVoicing[0] = pVoicing[1];
  }
  if (pVoicing[2] > pVoicing[0])
  {
    pVoicing[0] = pVoicing[2];
  }

   return;
}

 /*  Weighting filter of lower-band difference signal including an adaptive
     gain compensation */
void ownWeightingFilterFwd_G7291_16s(const short *pLPC, short *pSrcDst, short *pMem, short *pDiff)
{
  int num32, den32;
  IPP_ALIGNED_ARRAY(16, short, wspBuf, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, Ap1Buf, G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, Ap2Buf, G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, Wz_inBuf, G7291_LFRAME2 + G7291_LP_ORDER);
  short *pWz_inBuf;
  const short *pAz;
  int i, j;
  short indexSubfr, endLoop, fac;
  short e_den, fracDen, e_num, fracNum, e_fac;

//  IPP_BAD_PTR3_RET(pSrcDst, pMem, pDiff);

  pWz_inBuf = Wz_inBuf + G7291_LP_ORDER;
  for (i=0; i<G7291_LP_ORDER; i++) {
      pWz_inBuf[-G7291_LP_ORDER+i]=pSrcDst[i];
      pSrcDst[i]=pDiff[G7291_LFRAME2-G7291_LP_ORDER+i];
  }
  ippsCopy_16s(pDiff, pWz_inBuf, G7291_LFRAME2);

  pAz = pLPC;
  indexSubfr = (short) 0;
  for(i = 0; i < 4; i++)
  {
    /* weight A(z) */
    ippsMulPowerC_NR_16s_Sfs(pAz, G7291_GAMMA1, Ap1Buf, G7291_G729_MP1, 15);
    ippsMulPowerC_NR_16s_Sfs(pAz, G7291_GAMMA2, Ap2Buf, G7291_G729_MP1, 15);

    /* compute gain  */
    den32 = Ap1Buf[0] - Ap1Buf[1] + Ap1Buf[2] - Ap1Buf[3] + Ap1Buf[4] - Ap1Buf[5] +
            Ap1Buf[6] - Ap1Buf[7] + Ap1Buf[8] - Ap1Buf[9] + Ap1Buf[10];
    num32 = Ap2Buf[0] - Ap2Buf[1] + Ap2Buf[2] - Ap2Buf[3] + Ap2Buf[4] - Ap2Buf[5] +
            Ap2Buf[6] - Ap2Buf[7] + Ap2Buf[8] - Ap2Buf[9] + Ap2Buf[10];
 /* num32 = Ap2Buf[0];
    den32 = Ap1Buf[0];
    fac = (short) - 1;
    for (j = 1; j <= G7291_LP_ORDER; j++)
    {
      den32 = den32 + Ap1Buf[j] * fac;
      num32 = num32 + Ap2Buf[j] * fac;
      fac = Negate_16s(fac);
    }*/


    if (den32 < 0)       den32 = -den32;
    else if (den32 == 0) den32 = 1;

    e_den = Norm_32s_Pos_I(&den32);
    fracDen = (short)(den32>>16);

    if (num32 < 0)       num32 = -num32;
    else if (num32 == 0) num32 = 1;

    e_num = Norm_32s_Pos_I(&num32);
    fracNum = (short)(num32>>16);

    if (fracNum > fracDen)
    {
      fracNum = fracNum >> 1;
      e_num = e_num - 1;
    }
    fac = Div_16s(fracNum, fracDen);

    e_fac = e_num - e_den;
    e_fac = e_fac + 2;
    fac = fac >> e_fac;

    /* filter by A(z/gamma) * 1/A(z/gamma2) */
    ippsResidualFilter_G729_16s(&pWz_inBuf[indexSubfr], Ap1Buf, &wspBuf[indexSubfr]);
    ippsSynthesisFilterLow_NR_16s_ISfs(Ap2Buf,&wspBuf[indexSubfr],G7291_LSUBFR,12,pMem);

    for (j = 0; j < G7291_LP_ORDER; j++)
    {
      pMem[j] = wspBuf[indexSubfr + G7291_LSUBFR - G7291_LP_ORDER + j];
    }

    endLoop = indexSubfr + G7291_LSUBFR;
    ippsMulC_NR_16s_ISfs(fac, &wspBuf[indexSubfr], endLoop - indexSubfr, 13);

    pAz += G7291_G729_MP1;
    indexSubfr += G7291_LSUBFR;
  }
  ippsCopy_16s(wspBuf, pDiff, G7291_LFRAME2);
  return;
}
 /*  Inverse weighting filter of lower-band difference signal including
     an adaptive gain compensation */
void ownWeightingFilterInv_G7291_16s(const short *pLPC, short *pSrcDst, short *pMem, short *pDiff)
{
  int num32, den32;
  IPP_ALIGNED_ARRAY(16, short, wspBuf, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, Ap1Buf, G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, Ap2Buf, G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, invWz_inBuf, G7291_LFRAME2 + G7291_LP_ORDER);
  short   *pInvWz_in;
  const short *pAz;
  int i, j;
  short    indexSubfr, endLoop;
  short    fac, e_den, fracDen, e_num, fracNum, e_fac;

//  IPP_BAD_PTR3_RET(pSrcDst, pMem, pDiff);

  pInvWz_in = invWz_inBuf + G7291_LP_ORDER;
  for (i=0; i<G7291_LP_ORDER; i++) {
      pInvWz_in[-G7291_LP_ORDER+i]=pSrcDst[i];
      pSrcDst[i]=pDiff[G7291_LFRAME2-G7291_LP_ORDER+i];
  }
  ippsCopy_16s(pDiff, pInvWz_in, G7291_LFRAME2);

  /* apply W(z)= fac*A(z/gamma2)/A(z/gamma1) */
  pAz = pLPC;
  indexSubfr = (short) 0;
  for(i = 0; i < 4; i++)
  {
    /* weight A(z) */
    ippsMulPowerC_NR_16s_Sfs(pAz, G7291_GAMMA1, Ap1Buf, G7291_G729_MP1, 15);
    ippsMulPowerC_NR_16s_Sfs(pAz, G7291_GAMMA2, Ap2Buf, G7291_G729_MP1, 15);

    /* compute gain */
    num32 = Ap1Buf[0] - Ap1Buf[1] + Ap1Buf[2] - Ap1Buf[3] + Ap1Buf[4] - Ap1Buf[5] +
            Ap1Buf[6] - Ap1Buf[7] + Ap1Buf[8] - Ap1Buf[9] + Ap1Buf[10];
    den32 = Ap2Buf[0] - Ap2Buf[1] + Ap2Buf[2] - Ap2Buf[3] + Ap2Buf[4] - Ap2Buf[5] +
            Ap2Buf[6] - Ap2Buf[7] + Ap2Buf[8] - Ap2Buf[9] + Ap2Buf[10];

    /*den32 = Ap2Buf[0];
    num32 = Ap1Buf[0];
    fac = (short) - 1;
    for(j = 1; j <= G7291_LP_ORDER; j++)
    {
      den32 = den32 + Ap2Buf[j] * fac;
      num32 = num32 + Ap1Buf[j] * fac;
      fac = Negate_16s(fac);
    }*/

    if (den32 < 0)       den32 = -den32;  /* force den > 0 (needed by Div_16s) */
    else if (den32 == 0) den32 = 1;

    e_den = Norm_32s_Pos_I(&den32);
    fracDen = (short)(den32>>16);

    if (num32 < 0)       num32 = -num32;
    else if (num32 == 0) num32 =  1;

    e_num = Norm_32s_Pos_I(&num32);
    fracNum = (short)(num32>>16);

    if (fracNum > fracDen)
    {
      fracNum = fracNum >> 1;
      e_num = e_num - 1;
    }
    fac = Div_16s(fracNum, fracDen);

    e_fac = e_num - e_den;
    e_fac = e_fac + 2;
    fac = fac >> e_fac;

    /* filter by A(z/gamma2) * 1/A(z/gamma1) */
    ippsResidualFilter_G729_16s(&pInvWz_in[indexSubfr], Ap2Buf, &wspBuf[indexSubfr]);
    ippsSynthesisFilterLow_NR_16s_ISfs(Ap1Buf,&wspBuf[indexSubfr],G7291_LSUBFR,12,pMem);

    for (j = 0; j < G7291_LP_ORDER; j++)
    {
      pMem[j] = wspBuf[indexSubfr + G7291_LSUBFR - G7291_LP_ORDER + j];
    }

    endLoop = indexSubfr + G7291_LSUBFR;
    ippsMulC_NR_16s_ISfs(fac, &wspBuf[indexSubfr], endLoop - indexSubfr, 13);
    pAz += G7291_G729_MP1;
    indexSubfr += G7291_LSUBFR;
  }
  ippsCopy_16s(wspBuf, pDiff, G7291_LFRAME2);
  return;
}

 /* Arranging the 4 tri-pulse patterns */
void ownArrangePatterns(
const short *pPatternPos,  /* (i) : positions of the 4 patterns */
const short *pPatternSign, /* (i) : signs of the 4 patterns */
short AmplSidePulse,       /* (i) : amplitude of the side pulses */
short *pFCBvector)         /* (o) : decoded FCB vector */
{
  int i;
  short sp_p, sp_m;

  ippsZero_16s(pFCBvector, G7291_LSUBFR);
  sp_m = (AmplSidePulse * 32766 + 0x4000) >> 15;
  sp_p = -AmplSidePulse;
  {
    if (pPatternSign[0])
    {
      if (pPatternPos[0])
      {
        pFCBvector[pPatternPos[0] - 1] = sp_m;
      }
      pFCBvector[pPatternPos[0] + 1] = sp_m;
    }
    else
    {
      if (pPatternPos[0])
      {
        pFCBvector[pPatternPos[0] - 1] = sp_p;
      }
      pFCBvector[pPatternPos[0] + 1] = sp_p;
    }
  }
  for (i = 1; i < 3; i++)
  {
    if(pPatternSign[i])
    {
      pFCBvector[pPatternPos[i] - 1] = pFCBvector[pPatternPos[i] - 1] + sp_m;
      pFCBvector[pPatternPos[i] + 1] = pFCBvector[pPatternPos[i] + 1] + sp_m;
    }
    else
    {
      pFCBvector[pPatternPos[i] - 1] = pFCBvector[pPatternPos[i] - 1] + sp_p;
      pFCBvector[pPatternPos[i] + 1] = pFCBvector[pPatternPos[i] + 1] + sp_p;
    }
  }
  {
    if (pPatternSign[3])
    {
      pFCBvector[pPatternPos[3] - 1] = pFCBvector[pPatternPos[3] - 1] + sp_m;
      if (pPatternPos[3] != 39)
      {
        pFCBvector[pPatternPos[3] + 1] = pFCBvector[pPatternPos[3] + 1] + sp_m;
      }
    }
    else
    {
      pFCBvector[pPatternPos[3] - 1] = pFCBvector[pPatternPos[3] - 1] + sp_p;
      if (pPatternPos[3] != 39)
      {
        pFCBvector[pPatternPos[3] + 1] = pFCBvector[pPatternPos[3] + 1] + sp_p;
      }
    }
  }
  for (i = 0; i < G7291_LSUBFR; i++)
  {
    pFCBvector[i] = (pFCBvector[i] + 2) >> 2;
  }
  for(i = 0; i < 4; i++)
  {
    if(pPatternSign[i])
    {
      pFCBvector[pPatternPos[i]] = pFCBvector[pPatternPos[i]] + 8191;
    }
    else
    {
      pFCBvector[pPatternPos[i]] = pFCBvector[pPatternPos[i]] - 8192;
    }
  }
  return;
}

 /* Synthesys of g7291 core */
void ownSynthesys_G7291(
short *pExcCore,  /* (i)  : 8kbits core excitation                 (Q1)       */
short *pExcExt,   /* (i)  : 12kbits  core excitation  extension    (Q1)       */
short *pExcMem,   /* (i)  : 12kbits  core excitation  extension    (Q1)       */
short *BfAq,      /* (i)  : LPC coefficient filter                 (Q12)      */
short *pSynth,    /* (o)  : Synthesis buffer                       (Q0)       */
short *pSynthMem, /* (i/o): synthesis memory buffer                (Q0)       */
int   extRate)    /* (i)  : Extension bit rate                                */
{
  short indexSubfr;
  short *pBfAq;
  int Ovf = 0, i;

  pBfAq = BfAq;
  for(indexSubfr = 0; indexSubfr < 2 * G7291_LFRAME4; indexSubfr += G7291_LSUBFR)
  {
    pBfAq[0] >>= 1;
    if (extRate >= 12000) {
        Ovf = ippsSynthesisFilter_NR_16s_Sfs(pBfAq, &pExcExt[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pSynthMem);
    }
    else {
        Ovf = ippsSynthesisFilter_NR_16s_Sfs(pBfAq, &pExcCore[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pSynthMem);
    }
    if (Ovf != 0)
    {
      ippsRShiftC_16s(pExcMem, 2, pExcMem, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
      ippsRShiftC_16s(pExcCore, 2, pExcCore, 2 * G7291_LFRAME4);
      ippsRShiftC_16s(pExcExt, 2, pExcExt, 2 * G7291_LFRAME4);
      if (extRate >= 12000) {
          ippsSynthesisFilter_NR_16s_Sfs(pBfAq, &pExcExt[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pSynthMem);
          for (i=0; i<G7291_LP_ORDER; i++) pSynthMem[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
      }
      else {
          ippsSynthesisFilter_NR_16s_Sfs(pBfAq, &pExcCore[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pSynthMem);
          for (i=0; i<G7291_LP_ORDER; i++) pSynthMem[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
      }
    }
    else
    {
      for (i=0; i<G7291_LP_ORDER; i++) pSynthMem[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
    }
    pBfAq[0] <<= 1;
    pBfAq += G7291_G729_MP1;
  }
  return;
}

/*    ownTamFer   called for  a not erased frame
1 tap LTP filtering of 1 as input value for one g729 subframe
last G7291_MEM_LEN_TAM samples are saved  (no fractional pitch for taming -> no interpolation) */
short ownTamFer(short *pMemTam, short CoefLtp, short Pitch)
{
  int i, j;
  int somme = 0;
  IPP_ALIGNED_ARRAY(16, short, tamBuf, G7291_LSUBFR);

  if(Pitch < G7291_LSUBFR)
  {
    for(j = 0, i = (G7291_MEM_LEN_TAM - Pitch); i < G7291_MEM_LEN_TAM; j++, i++)
    {
      tamBuf[j] = ((CoefLtp * pMemTam[i] + 0x4000) >> 15) << 1;
      somme += tamBuf[j];
    }
    for(j = Pitch; j < G7291_LSUBFR; j++)
    {
      tamBuf[j] = ((CoefLtp * tamBuf[j - Pitch] + 0x4000) >> 15) << 1;
      somme += tamBuf[j];
    }
    /*calcul of the weighting factor MultGainBk to use in the LTP synthesis */
    if (somme > G7291_MAXTAMLIM)
    {
      somme = G7291_MAXTAMLIM;
    }

    if((CoefLtp > 16384) && (somme > G7291_MINTAMLIM))
    {
      CoefLtp = (short)((0x20004000 +
          Div_16s((short)(G7291_MAXTAMLIM - somme), G7291_DIFF_MAXMINTAMLIM) *
          (CoefLtp - 0x4000)) >> 15);
    }
    /*filtering using the modified correction */
    for(j = 0, i = (G7291_MEM_LEN_TAM - Pitch); i < G7291_MEM_LEN_TAM; j++, i++)
    {
      tamBuf[j] = ((CoefLtp * pMemTam[i] + 0x4000) >> 15) << 1;
    }
    for(j = Pitch; j < G7291_LSUBFR; j++)
    {
      tamBuf[j] = ((CoefLtp * tamBuf[j - Pitch] + 0x4000) >> 15) << 1;
    }
  }
  else
  {
    for(j = 0, i = G7291_MEM_LEN_TAM - Pitch; j < G7291_LSUBFR; j++, i++) /*memory from the end of pMemTam */
    {
      tamBuf[j] = ((CoefLtp * pMemTam[i] + 0x4000) >> 15) << 1;
      somme += tamBuf[j];
    }
    /*calcul of the weighting factor MultGainBk to use in the LTP synthesis */
    if (somme > G7291_MAXTAMLIM)
    {
      somme = G7291_MAXTAMLIM;
    }

    if((CoefLtp > 0x4000) && (somme > G7291_MINTAMLIM))
    {
      CoefLtp = (short)((0x20004000 +
          Div_16s((short)(G7291_MAXTAMLIM - somme), G7291_DIFF_MAXMINTAMLIM) *
          (CoefLtp - 0x4000)) >> 15);
    }
    /*filtering using the modified correction */
    for(j = 0, i = G7291_MEM_LEN_TAM - Pitch; j < G7291_LSUBFR; j++, i++) /*memory from the end of pMemTam */
    {
      tamBuf[j] = ((CoefLtp * pMemTam[i] + 0x4000) >> 15) << 1;
    }
  }

  /*memory update */
  ippsMove_16s(&pMemTam[G7291_LSUBFR], pMemTam, G7291_LEN_MEMTAM_UPDATE_TAM);
  ippsCopy_16s(tamBuf, &pMemTam[G7291_LEN_MEMTAM_UPDATE_TAM], G7291_LSUBFR);

  return (CoefLtp);
}
 /* Find the voicing factor (1=voice to -1=unvoiced) */
short ownFindVoiceFactor(const short *pSrcPitchExc, const short *pSrcGains, const short *pFixedCdbkExc)
{
  IPP_ALIGNED_ARRAY(16, short, excBuf, G7291_LSUBFR);
  int i, tmp32;
  short tmp16, exp, ener1, exp1, ener2, exp2, Q_exc;

  ippsDotProd_16s32s_Sfs(pSrcPitchExc, pSrcPitchExc, G7291_LSUBFR, &tmp32, 0);
  if(tmp32 > IPP_MAX_32S/2)
  {
    ippsRShiftC_16s(pSrcPitchExc, 3, excBuf, G7291_LSUBFR);
    Q_exc = (short) - 4;
  }
  else
  {
    ippsCopy_16s(pSrcPitchExc, excBuf, G7291_LSUBFR);
    Q_exc = (short) 2;

  }
  ippsDotProd_16s32s_Sfs(excBuf, excBuf, G7291_LSUBFR, &tmp32, -1);
  tmp32 += 1;
  exp1 = 30 - Norm_32s_I(&tmp32);
  ener1 = (short)(tmp32 >> 16);

  exp1 = exp1 - Q_exc;
  tmp32 = (pSrcGains[0] * pSrcGains[0]) << 1;
  exp = Norm_32s_I(&tmp32);
  tmp16 = (short)(tmp32>>16);
  ener1 = (ener1 * tmp16)>>15;
  exp1 = exp1 - exp - 2;

  ippsDotProd_16s32s_Sfs(pFixedCdbkExc, pFixedCdbkExc, G7291_LSUBFR, &tmp32, -1);
  tmp32 += 1;
  exp2 = 30 - Norm_32s_I(&tmp32);
  ener2 = (short)(tmp32 >> 16);

  tmp16 = pSrcGains[1];
  exp = Norm_16s_I(&tmp16) + 1;
  tmp16 = (tmp16 * tmp16)>>15;
  ener2 = (ener2 * tmp16)>>15;
  exp2 = exp2 - (exp << 1);

  i = exp1 - exp2;
  if(i > 15)
  {
    return (short) 32767;
  }
  else if (i < -15)
  {
    return (short) - 32768;
  }
  else
  {
    int L_ener1, L_ener2;
    if(i >= 0)
    {
      L_ener1 = ener1 << i;
      L_ener2 = ener2;
    }
    else
    {
      L_ener1 = ener1;
      L_ener2 = ener2 << (-i);
    }
    tmp32 = L_ener1 - L_ener2;
    L_ener1 = L_ener1 + L_ener2 + 1;
    exp1 = Norm_32s_I(&L_ener1);
    ener1 = (short)(L_ener1>>16);
    tmp16 = (short)((tmp32 << exp1)>>16);

    if(tmp32 >= 0)
    {
      tmp16 = Div_16s(tmp16, ener1);
    }
    else
    {
      tmp16 = Negate_16s(Div_16s(Negate_16s(tmp16), ener1));
    }
  }
  return (tmp16);
}


 /* TDBWE parameter lookup and mean value reconstruction */
void ownDeQuantization_TDBWE_16s(short *pCdbkIndices, short *pDst)
{
  int i;
  short mu_q;
  unsigned short vqIndex;
  short *parameters, *pParam;
  const short *pTbl;

  pParam = pCdbkIndices;
  /* retrieve the time envelope's mean from the ppBitstream */
  mu_q = G7291_TDBWE_MEAN_TIME_ENV_cbTbl[*pParam++];
  /* retrieve the time envelope from the ppBitstream and de-normalize */
  for(i = 0; i < G7291_TDBWE_TIME_ENV_SPLIT_VQ_NUMBER_SPLITS; i++)
  {
    vqIndex = *pParam++;
    pDst[i*8+0] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+0];
    pDst[i*8+1] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+1];
    pDst[i*8+2] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+2];
    pDst[i*8+3] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+3];
    pDst[i*8+4] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+4];
    pDst[i*8+5] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+5];
    pDst[i*8+6] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+6];
    pDst[i*8+7] = G7291_TDBWE_TIME_ENV_cbTbl[vqIndex*8+7];
  }

  for(i = 0; i < G7291_TDBWE_NB_SUBFRAMES; i++)
  {
    pDst[i] = Add_16s(pDst[i], mu_q);
  }
  /* retrieve the frequency envelope from the bitstream and de-normalize */
  parameters = pDst + G7291_TDBWE_PAROFS_FREQ_ENVELOPE;
  for(i = 0; i < G7291_TDBWE_FREQ_ENV_SPLIT_VQ_NUMBER_SPLITS; i++)
  {
    pTbl = G7291_TDBWE_FREQ_ENV_cbTbl[i];
    vqIndex = *pParam++;
    parameters[i*4+0] = pTbl[vqIndex*4+0];
    parameters[i*4+1] = pTbl[vqIndex*4+1];
    parameters[i*4+2] = pTbl[vqIndex*4+2];
    parameters[i*4+3] = pTbl[vqIndex*4+3];
  }
  parameters = pDst + G7291_TDBWE_PAROFS_FREQ_ENVELOPE;
  for(i = 0; i < G7291_TDBWE_NB_SUBBANDS; i++)
  {
    parameters[i] = Add_16s(parameters[i], mu_q);
  }
  return;
}
 /* Compute log2 rms for each subband */
void ownCalcSpecEnv_TDAC(
short *pTransformCoeffs,          /* (i)    transform coefficients of the difference           */
short *pLogRms,    /* (o)    log2 of the spectrum envelope                      */
int numSubBands, /* (i)    number of subbands                                 */
int normShiftLo, /* (i)    norm_shift of low bands                            */
int normShiftHi, /* (i)    norm_shift of high bands                           */
int normMDCT)   /* (i)    norm_shift transmitted to decoder                  */
{
  int j, ener;
  short logInt, logFrac;
  int norm;

  for(j = 0; j < numSubBands; j++)
  {
    ippsDotProd_16s32s_Sfs(&pTransformCoeffs[G7291_TDAC_sb_boundTbl[j]], &pTransformCoeffs[G7291_TDAC_sb_boundTbl[j]],
        G7291_TDAC_sb_boundTbl[j + 1] - G7291_TDAC_sb_boundTbl[j], &ener, -1);

    /* divide energy by number of coef in each band : taken into account in the norm value
       normalize result before log2 operation */
    norm = Norm_32s_Pos_I(&ener);
    norm += G7291_TDAC_nb_coef_divTbl[j] + 1;

    if (j < G7291_TDAC_NB_SB_NB)
    {
      norm = norm + ((normShiftLo - normMDCT) << 1);
    }
    else
    {
      norm = norm + ((normShiftHi - normMDCT) << 1);
    }

    if (ener == 0)
    {
      pLogRms[j] = (short) - 24576;
    }
    else
    {
      Log2(ener, &logInt, &logFrac);
      ener = ((logInt - norm) << 16) + (logFrac << 1);
      pLogRms[j] = (short)(ener>>6);
    }
  }
  return;
}
 /* Quantize with 3dB step */
static void ownQuant3dB(
short *pIndex,     /* (o)    code words number              */
short *pLogRms,   /* (i)    log2 of the spectrum envelope  */
int normMDCT)     /* (i)    MDCT norm value                */
{
  short tmp16;
  int minRMS, maxRMS;

  if (normMDCT < G7291_TDAC_NORM_MDCT_TRESH)
  {
    minRMS = G7291_TDAC_MIN_RMS + (normMDCT << 1);
    maxRMS = G7291_TDAC_MAX_RMS + (normMDCT << 1);
  }
  else
  {
    minRMS = G7291_TDAC_MIN_RMS + G7291_TDAC_RMS_OFFSET;
    maxRMS = G7291_TDAC_MAX_RMS + G7291_TDAC_RMS_OFFSET;
  }

  tmp16 = (*pLogRms * G7291_TDAC_Q3db_INV)>>15;
  tmp16 = tmp16 + 0x100;
  *pIndex = tmp16 >> 9;

  if (*pIndex < minRMS)
  {
    *pIndex = (short)minRMS;
  }

  if (*pIndex > maxRMS)
  {
    *pIndex = (short)maxRMS;
  }
  return;
}
 /* Compute differential indices and detect saturation */
static short ownTDAC_compute_diff_index(
short *pRmsIndex,  /* (i)    indices of quantized rms */
short numSubBands, /* (i)    number of subbands */
short *pDiffIndex) /* (o)    differential indices */
{
  int j;
  short tmp16;

  for (j = 0; j < numSubBands; j++)
  {
    tmp16 = pRmsIndex[j] - pRmsIndex[j - 1];
    if (tmp16 < -12 || tmp16 > 12)
    {
      return (1);
    }
    pDiffIndex[j] = tmp16;

    pRmsIndex[j] = pRmsIndex[j - 1] + tmp16;
  }
  return (0);
}
static __ALIGN32 CONST short len_huff_diffTbl[25] = {
  11, 11, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13, 14, 14,
};
static __ALIGN32 CONST unsigned short code_huff_diffTbl[25] = {
  0x0006, 0x0007, 0x0012, 0x0008, 0x0005, 0x0003, 0x0003, 0x0002, 0x0002, 0x000c, 0x0001, 0x0001, 0x0002, 0x0007,
      0x000d, 0x0003, 0x0003, 0x0002, 0x0001, 000000, 0x0002, 0x0027, 0x004c, 0x009b, 0x009a,
};

 /* encode spectral envelope */
static void ownTDAC_encode_spenv(
short *pRmsIndex,   /* (i)    indices of quantized rms */
short *pDiffIndex,  /* (i)    differential indices */
short *pPrms,       /* (o)    parameters */
short *numBitEnv,   /* (o)    number of bits used for spectral envelope */
short numSubBands,  /* (i)    number of subbands */
short saturFlg,     /* (i)    saturation flag */
int normMDCT)       /* (i)    MDCT normalization factor */
{
  short numBitEnvEq[G7291_TDAC_NB_SB_NB+1];
  short prmEq[G7291_TDAC_NB_SB_NB+1];
  short numBitEq, nbit, k;
  short minRMS, len, tmp16;
  int j;

  if (normMDCT < G7291_TDAC_NORM_MDCT_TRESH)
  {
    minRMS = (short)(G7291_TDAC_MIN_RMS + normMDCT * 2);
  }
  else
  {
    minRMS = (short)(G7291_TDAC_MIN_RMS + G7291_TDAC_RMS_OFFSET);
  }

  /* mode 1: equiprobable */
  prmEq[0] = 1;
  numBitEq = (short) 1;
  numBitEnvEq[0] = (short) 1;

  for(j = 0; j < numSubBands; j++)
  {
    tmp16 = pRmsIndex[j] - minRMS;
    prmEq[j+1] = tmp16;
    numBitEq = numBitEq + 5;
    numBitEnvEq[j+1] = (short) 5;
  }

  if (saturFlg != 1)
  {
    /* mode 0: differential Huffman */
    /* first band */
    pPrms[0] = 0;
    numBitEnv[0] = (short) 1;
    tmp16 = pRmsIndex[0] - minRMS;
    pPrms[1] = tmp16;
    nbit = 5 + 1;
    numBitEnv[1] = (short) 5;

    /* next bands */
    for(j = 1; j < numSubBands; j++)
    {
      k = pDiffIndex[j] + 12;
      len = len_huff_diffTbl[k];
      pPrms[j+1] = code_huff_diffTbl[k];
      nbit = nbit + len;
      numBitEnv[j+1] = len;
    }
  }
  else
  {
    nbit = (short) 0;
  }

  if ((saturFlg == 1) || (numBitEq < nbit))
  {
    for(j = 0; j < numSubBands+1; j++)
    {
      pPrms[j] = prmEq[j];
      numBitEnv[j] = numBitEnvEq[j];
    }
  }
  return;
}

 /* Encode spectral envelope */
void ownEncodeSpectralEnvelope_TDAC(
short *pLogRms,       /* (i)     log2 of the spectrum envelope */
short *pRmsIndex,     /* (o)     decoded spectrum envelope */
short *pCoderParam,
short *numBitEnv,
int *pBitCount,       /* (o)     number of bits written to bitstream  */
int numBitToEncode,   /* (i)     maximal number of bits for TDAC */
int normMDCT)       /* (i)     MDCT normalization factor */
{
  int j, i;
  int tmp32 = 0;
  short pDiffIndex[G7291_TDAC_NB_SB];
  short saturFlg;
  short prmNB[1 + G7291_TDAC_NB_SB_NB], prmWB[1 + G7291_TDAC_NB_SB_WB];

  ippsZero_16s(numBitEnv, G7291_TDAC_NB_SB+2);

  for (j = 0; j < G7291_TDAC_NB_SB; j++)
  {
    ownQuant3dB(&pRmsIndex[j], &pLogRms[j], normMDCT);
  }

  /* compute spectral envelope parameters in lower band */
  saturFlg = ownTDAC_compute_diff_index(&pRmsIndex[1], G7291_TDAC_NB_SB_NB - 1, &pDiffIndex[1]);
  ownTDAC_encode_spenv(pRmsIndex, pDiffIndex, prmNB, numBitEnv, G7291_TDAC_NB_SB_NB, saturFlg, normMDCT);

  /* compute spectral envelope parameters in higher band */
  saturFlg = ownTDAC_compute_diff_index(&pRmsIndex[G7291_TDAC_NB_SB_NB + 1], G7291_TDAC_NB_SB_WB - 1,
                                         &pDiffIndex[G7291_TDAC_NB_SB_NB + 1]);
  ownTDAC_encode_spenv(pRmsIndex + G7291_TDAC_NB_SB_NB, pDiffIndex + G7291_TDAC_NB_SB_NB,
                           prmWB, numBitEnv + G7291_TDAC_NB_SB_NB+1, G7291_TDAC_NB_SB_WB, saturFlg, normMDCT);

  /* write higher band parameters to bitstream */
  for (i=0; i<G7291_TDAC_NB_SB_NB+1; i++) {
    pCoderParam[i] = prmNB[i];
  }
  for (i=0; i<G7291_TDAC_NB_SB_WB+1; i++) {
    pCoderParam[i+G7291_TDAC_NB_SB_NB+1] = prmWB[i];
  }
  for (i=0; i<G7291_TDAC_NB_SB_WB+1; i++) {
      tmp32 += numBitEnv[i+G7291_TDAC_NB_SB_NB+1];
      if (tmp32 <= numBitToEncode) {
          *pBitCount = tmp32;
      } else {
          *pBitCount = numBitToEncode;
          break;
      }
  }
  for (i=0; i<G7291_TDAC_NB_SB_NB+1; i++) {
      tmp32 += numBitEnv[i];
      if (tmp32 <= numBitToEncode) {
          *pBitCount = tmp32;
      } else {
          *pBitCount = numBitToEncode;
          break;
      }
  }
  return;
}
 /* Order subbands by decreasing perceptual importance */
void ownSort_TDAC(
short *pPerceptImport,    /* (i) Q1  perceptual importance per subband */
short *pSubBandOrder)     /* (o)     subband order */
{
  short pPerceptImportTbl[G7291_TDAC_NB_SB], max;
  short i, j, jmax, k;

  /* copy pPerceptImport */
  for (i=0; i<G7291_TDAC_NB_SB; i++) pPerceptImportTbl[i]=pPerceptImport[i];

  /* sort pPerceptImport in decreasing order and save positions */
  k = 0;
  for (i = 0; i < G7291_TDAC_NB_SB; i++)
  {
    jmax = 0;          /* to avoid warning */
    max = G7291_TDAC_SORTED_IP;
    for (j = 0; j < G7291_TDAC_NB_SB; j++)
    {
      if (pPerceptImportTbl[j] > max)
      {
        jmax = j;
        max = pPerceptImportTbl[j];
      }
    }

    pSubBandOrder[k] = jmax;
    pPerceptImportTbl[jmax] = G7291_TDAC_SORTED_IP;
    k = k + 1;
  }
  return;
}
/* Compute pBitAlloc(j)=SelectCbk(pNumCoeffs(j)*(pPerceptImport(j)-lam)) and
    nbit_total = sum_j pBitAlloc(j) */
static void ownTDAC_calc_nbit_total(
short *pPerceptImport,    /* (i) Q1  perceptual importance per subband */
short lam,                /* (i) Q6  water level */
short *nbit_total,        /* (o)     total number of bits */
short *pBitAlloc)         /* (o)     number of bits per subband */
{
  int j, n;
  short nunBitEstim;

  /* compute bit allocation for each subband */
  *nbit_total = (short) 0;
  for (j = 0; j < G7291_TDAC_NB_SB-1; j++)
  {
    /* compute estimated bit allocation = estimated rate per sample * dimension */
    nunBitEstim = (pPerceptImport[j] << 5) - lam;
    nunBitEstim = nunBitEstim << G7291_TDAC_nb_coef_divTbl[j];

    /* find codebook rate which is closest to target rate */
    n = (nunBitEstim + 0x20) >> 6;
    if(n <= 0)       n = 0;
    else if(n >= 32) n = 32;
    else             n = G7291_Rate16Tbl_ind[n];

    pBitAlloc[j] = (short)n;
    *nbit_total = (short)(*nbit_total + n);
  }
  // j = numSubBand-1
  nunBitEstim = (pPerceptImport[G7291_TDAC_NB_SB-1] << 5) - lam;
  nunBitEstim = nunBitEstim << G7291_TDAC_nb_coef_divTbl[G7291_TDAC_NB_SB-1];

  /* find codebook rate which is closest to target rate */
  n = (nunBitEstim + 0x20) >> 6;
  if(n <= 0)        n = 0;
  else if(n >= 16)  n = 16;
  else              n = G7291_Rate8Tbl_ind[n];

  pBitAlloc[G7291_TDAC_NB_SB-1] = (short)n;
  *nbit_total = (short)(*nbit_total + n);
  return;
}
 /* Search for codebook in dimension dim with rate < n bits */
static short ownTDAC_SearchPrev(
short numBits, /* (i)     number of bits */
short dim)     /* (i)     dimension */
{
  const short *ptr;
  short i, idic_sup, tmp;

  ptr = G7291_adRateTbl[dim];
  idic_sup = G7291_NbDicTbl[dim] - 1;

  for(i = idic_sup; i > 0; i--)
  {
    tmp = ptr[i] - numBits;
    if (tmp < 0)
    {
      return (ptr[i]);
    }
  }
  return (ptr[0]);
}

 /* Compute pBitAlloc(j)=SelectCbk(pNumCoeffs(j)*(pPerceptImport(j)-lam)) and
    nbit_total = sum_j pBitAlloc(j) */
void ownBitAllocation_TDAC(
short *pBitAlloc,      /* (o)     number of bits per subband */
int nbit_max,        /* (i)     bit budget to TDAC vq */
short *pPerceptImport, /* (i) Q1  perceptual importance per subband */
short *ord_b)          /* (i)     ordering of subbands */
{
  short new_bit_alloc[G7291_TDAC_NB_SB];
  short i, j, n;
  short bitCount, bit_cnt0, bit_cnt1;
  short lam, lam0, lam1;
  short max_ip, min_ip;
  short iter, diff, new_bi;
  int addBits, addBitsMax, tmp;

  for (j = 0; j < G7291_TDAC_NB_SB; j++)
  {
    pBitAlloc[j] = (short) 0;
  }

  max_ip = pPerceptImport[ord_b[0]];
  tmp = G7291_TDAC_NB_SB - 1;
  min_ip = pPerceptImport[ord_b[tmp]];

  lam0 = max_ip << 5;
  bit_cnt0 = (short) 0;

  lam1 = (min_ip - 8) << 5;
  ownTDAC_calc_nbit_total(pPerceptImport, lam1, &bit_cnt1, new_bit_alloc);

  for(iter=1; iter < 11; iter++)
  {

    /* try middle value */
    lam = (lam0 + lam1) >> 1;
    ownTDAC_calc_nbit_total(pPerceptImport, lam, &bitCount, new_bit_alloc);

    /* update solution if total bit allocation is inferior and closer to bit budget */
    if (bitCount <= nbit_max)
    {
      lam0 = lam;
      for (j = 0; j < G7291_TDAC_NB_SB; j++)
      {
        pBitAlloc[j] = new_bit_alloc[j];
      }
      bit_cnt0 = bitCount;
    }
    else
    {
      lam1 = lam;
    }
  }
  if (bit_cnt0 == nbit_max) return;
  /* add bits */
  for (j = 0; j < G7291_TDAC_NB_SB; j++)
  {
    i = ord_b[j];
    n = G7291_TDAC_nb_coefTbl[i];

    /* add bits to subband i (try to sature bit allocation) */
    addBits = G7291_NbDicTbl[n] - 1;
    addBits = *(G7291_adRateTbl[n] + addBits);
    addBits = addBits - pBitAlloc[i];
    addBitsMax = nbit_max - bit_cnt0;

    if (addBits > addBitsMax)
    {
      addBits = addBitsMax;
    }

    tmp = pBitAlloc[i] + addBits;
    if(tmp <= 0) new_bi =  0;
    else if(tmp >= G7291_adRateTbl[n][G7291_NbDicTbl[n]-1])
       new_bi = G7291_adRateTbl[n][G7291_NbDicTbl[n]-1];
    else
       new_bi = G7291_adRateTbl_ind[n][tmp];

    diff = new_bi - pBitAlloc[i];

    tmp = bit_cnt0 + diff;
    tmp -= nbit_max;

    if (tmp > 0)
    {
      new_bi = ownTDAC_SearchPrev(new_bi, n);
      diff = new_bi - pBitAlloc[i];
    }

    bit_cnt0 = bit_cnt0 + diff;
    pBitAlloc[i] = pBitAlloc[i] + diff;
  }
  return;
}
/* Masks for bitstream packing */
static __ALIGN32 CONST unsigned short maskBitTbl[G7291_TDAC_MAX_LONG_HUFF + 1] =
    { 0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383,
  32767, 0xFFFF
};

 /* Read bits from bitstream */
static unsigned short ownTDAC_read_bits(
unsigned short *pBitBuffer, /* (i/o)   bit buffer */
short len,              /* (i)     number of bits to read */
short **ppBitstream,    /* (i/o)   bitstream pointer */
int   *valBitCnt,
short *pBitCount)       /* (i/o)   total number of bits read from bitstream */
{
  int buffer_32;
  int i;
  unsigned short temp;

  i = G7291_TDAC_MAX_LONG_HUFF - len;
  temp = (unsigned short)(*pBitBuffer >> i);
  temp = (temp & maskBitTbl[len]); /* clean MSBs */
  *pBitCount = *pBitCount + len;

  buffer_32 = (int) * pBitBuffer;
  buffer_32 = buffer_32 << len;
  *pBitBuffer = (short)buffer_32;

  *pBitBuffer = (short)(*pBitBuffer | ownExtractBits((const unsigned char **)ppBitstream, valBitCnt, len));

  return temp;
}

 /* Decode rms index (natural binary coding) */
static void ownTDAC_dec(
unsigned short *pBitBuffer, /* (i/o)   bit buffer */
short **ppBitstream,    /* (i/o)   bitstream pointer */
int   *valBitCnt,
short *pBitCount,       /* (i/o)   total number of bits read from ppBitstream */
short *pRmsIndex,       /* (o)     decoded rms index */
int normMDCT)           /* (i)     MDCT normalization factor */
{
  unsigned short temp;
  short minRMS;

  if (normMDCT < G7291_TDAC_NORM_MDCT_TRESH)
  {
    minRMS = (short)(G7291_TDAC_MIN_RMS + (normMDCT << 1));
  }
  else
  {
    minRMS = (short)(G7291_TDAC_MIN_RMS + G7291_TDAC_RMS_OFFSET);
  }

  temp = ownTDAC_read_bits(pBitBuffer, 5, ppBitstream, valBitCnt, pBitCount);

  /* set index defined in interval [-8,23] */
  *pRmsIndex = temp + minRMS;
  return;
}

/*  Read and decode Huffman code */
static void TDAC_find_huffman(
unsigned short *pBitBuffer,     /* (i/o)   bit buffer */
const short *pCodeLenghtTbl,    /* (i)     table of code lengths */
const unsigned short *pCodeTbl, /* (i)     code table */
short numHuffCode,              /* (i)     number of Huffman codes */
short *pIndexHuffCode)          /* (o)     index of Huffman code */
{
  short i, j;
  unsigned short temp;

  *pIndexHuffCode = (short) - 1;
  for(j = 0; j < numHuffCode; j++)
  {
    /* read the number of bits corresponding to the length of the pIndexHuffCode-th codevector */
    i = G7291_TDAC_MAX_LONG_HUFF - pCodeLenghtTbl[j];
    temp = *pBitBuffer >> i;
    temp = temp & maskBitTbl[pCodeLenghtTbl[j]];

    /* check if these bits match the codevector */
    if (temp == pCodeTbl[j])
    {
      *pIndexHuffCode = j;
      break;
    }
  }
  return;
}

/*  Flush bit buffer */
static void TDAC_flush_buffer(
unsigned short *pBitBuffer,  /* (i/o)   bit buffer */
short len,                   /* (i)     number of bits to flush */
short **ppBitstream,         /* (i/o)   bitstream pointer */
int   *valBitCnt,
short *pBitCount)            /* (i/o)   total number of bits read from bitstream */
{
  int buffer_32;

  *pBitCount = *pBitCount + len;
  buffer_32 = (int) * pBitBuffer;
  buffer_32 = buffer_32 << len;
  *pBitBuffer = (short)buffer_32;
  *pBitBuffer = (short)(*pBitBuffer | ownExtractBits((const unsigned char **)ppBitstream, valBitCnt, len));
  return;
}

/*  Decode rms indices  */
static short TDAC_decode_rms_index(
short **ppBitstream,         /* (i/o)   ppBitstream pointer */
int   *valBitCnt,
short *pRmsIndex,            /* (o)     decoded rms indices */
short *pBitCount,            /* (o)     total number of bits read from ppBitstream */
int numBitToDecode,          /* (i)     maximal number of bits for TDAC */
short numSubBands,           /* (i)     number of subbands */
unsigned short *pBitBuffer,  /* (i/o)   bit buffer */
int normMDCT)                /* (i)     MDCT normalization factor */
{
  short eq_switch, j, k, nb_dec;
  short tmp;

  nb_dec = (short) 0;

  /* read switch between normal and equiprobable mode (1 bit) and first subband */
  tmp = *pBitCount + 6;
  if (tmp <= numBitToDecode)
  {
    eq_switch = ownTDAC_read_bits(pBitBuffer, 1, ppBitstream, valBitCnt, pBitCount);
    ownTDAC_dec(pBitBuffer, ppBitstream, valBitCnt, pBitCount, &pRmsIndex[0], normMDCT);

    nb_dec = nb_dec + 1;
  }
  else
  {
    return (nb_dec);
  }

  /* switch between normal and equiprobable decoding */
  if (eq_switch == 0)            /* 0: normal; 1: equiprobrable */
  {
    /* decode other subbands */
    for(j = 1; j < numSubBands; j++)
    {
      /* try to find a Huffman codevector */
      TDAC_find_huffman(pBitBuffer, len_huff_diffTbl, code_huff_diffTbl, 25, &k);

      /* Test if word recovered */
      tmp = *pBitCount + len_huff_diffTbl[k];
      if (k == -1 || tmp > numBitToDecode)
      {
        return (nb_dec);
      }
      else
      {
        /* flush pBitBuffer */
        TDAC_flush_buffer(pBitBuffer, len_huff_diffTbl[k], ppBitstream, valBitCnt, pBitCount);

        /* add diff_rms defined in interval [-12,12] to previous pRmsIndex */
        tmp = k - 12;
        pRmsIndex[j] = pRmsIndex[j - 1] + tmp;

        nb_dec = nb_dec + 1;
      }
    }
  }
  else                        /* if (eq_switch == 0) */
  {
    for (j = 1; j < numSubBands; j++)
    {
      tmp = *pBitCount + 5;
      if (tmp <= numBitToDecode)
      {
        ownTDAC_dec(pBitBuffer, ppBitstream, valBitCnt, pBitCount, &pRmsIndex[j], normMDCT);
        nb_dec = nb_dec + 1;
      }
      else
      {
        return (nb_dec);
      }
    }
  }
  return (nb_dec);
}

/*  Compute rms_q(j) = 2^(pRmsIndex(j)/2) */
static void TDAC_calc_rms_q(
short *pRmsIndex,   /* (i)   decoded rms indices */
short *pRmsDecoded, /* (o)   decoded rms */
int normMDCT,       /* (i)   MDCT normalization factor */
short *ndec)        /* (i)   number of spectral envelope parameters */
{
  short maxRMS, minRMS;
  short v_exp, v_frac;
  short bad_rms;
  int j;

  if (normMDCT < G7291_TDAC_NORM_MDCT_TRESH)
  {
    minRMS = (short)(G7291_TDAC_MIN_RMS + (normMDCT << 1));
    maxRMS = (short)(G7291_TDAC_MAX_RMS + (normMDCT << 1));
  }
  else
  {
    minRMS = G7291_TDAC_MIN_RMS + G7291_TDAC_RMS_OFFSET;
    maxRMS = G7291_TDAC_MAX_RMS + G7291_TDAC_RMS_OFFSET;
  }

  /* verify that pRmsIndex is in [G7291_TDAC_MIN_RMS,G7291_TDAC_MAX_RMS] interval */
  bad_rms = (short) 0;
  for (j = 0; j < *ndec; j++)
  {
    if (pRmsIndex[j] < minRMS)
    {
      bad_rms = (short) 1;
    }

    if (pRmsIndex[j] > maxRMS)
    {
      bad_rms = (short) 1;
    }
  }

  /* reconstruct rms */
  if (bad_rms)
  {
    *ndec = (short) 0;
  }
  else
  {
    for (j = 0; j < *ndec; j++)
    {
      v_exp = pRmsIndex[j] >> 1; /* pRmsIndex * G7291_TDAC_Q3db */
      if (pRmsIndex[j] & 0x01)
      {
        v_frac = (short) 16384;
      }
      else
      {
        v_frac = (short) 0;
      }

      pRmsDecoded[j] = (short)Pow2(v_exp, v_frac);
    }
  }
  return;
}

/*  Decode spectral envelope */
void ownDecodeSpectralEnvelope_TDAC(
short **ppBitstream,   /* (i/o)   bitstream pointer */
int   *valBitCnt,
short *pRmsIndex,      /* (o)     decoded rms indices */
short *pRmsDecoded,    /* (o)     decoded rms */
short *pBitCount,      /* (o)     total number of bits read from ppBitstream */
int numBitToDecode,    /* (i)     maximal number of bits for TDAC */
short *numNBparams,    /* (i)     number of lower-band spectral envelope parameters */
short *numWBparams,    /* (i)     number of higher-band spectral envelope parameters */
int normMDCT)        /* (i)     MDCT normalization factor */
{
  unsigned short bit_buffer;
  char *pBit;

  /* initialize decoded spectral envelope */
  ippsZero_16s(pRmsDecoded, G7291_TDAC_NB_SB);
  *numNBparams = (short) 0;
  *numWBparams = (short) 0;

  /* fill buffer which contains MAX_LONG_HUF new bits */
  if (numBitToDecode >= G7291_TDAC_MAX_LONG_HUFF)
  {
    bit_buffer = (unsigned short)ownExtractBits((const unsigned char **)ppBitstream, valBitCnt, G7291_TDAC_MAX_LONG_HUFF);
  }
  else
  {
    return;
  }

  /* decode spectral envelope in higher band */
  *numWBparams = TDAC_decode_rms_index(ppBitstream, valBitCnt, pRmsIndex + G7291_TDAC_NB_SB_NB,
                                          pBitCount, numBitToDecode, G7291_TDAC_NB_SB_WB, &bit_buffer, normMDCT);
  TDAC_calc_rms_q(pRmsIndex + G7291_TDAC_NB_SB_NB, pRmsDecoded + G7291_TDAC_NB_SB_NB, normMDCT, numWBparams);

  /* decode spectral envelope in lower band */
  *numNBparams = TDAC_decode_rms_index(ppBitstream, valBitCnt, pRmsIndex, pBitCount, numBitToDecode,
                                          G7291_TDAC_NB_SB_NB, &bit_buffer, normMDCT);
  TDAC_calc_rms_q(pRmsIndex, pRmsDecoded, normMDCT, numNBparams);

  /* flush buffer */
  pBit = (char*)(*ppBitstream);
  pBit -= (G7291_TDAC_MAX_LONG_HUFF >> 3);
  *ppBitstream = (short*)pBit;
  return;
}
/*  Median filter for sequence of odd length */
static short gmed_n(/* (o) median value  */
short *ind,  /* (i) Current and past values */
short n)     /* (i) number of values (valid for an odd number of gains <= G7291_NMAX) */
{
  short  tmp[G7291_NMAX], tmp2[G7291_NMAX];
  short  i, j, ix;
  short  max, medianIndex;

  ix = (short) 0;

  for(i = 0; i < n; i++)
  {
    tmp2[i] = ind[i];
  }

  for(i = 0; i < n; i++)
  {
    max = (short) - 1;
    for(j = 0; j < n; j++)
    {
      if (tmp2[j] >= max)
      {
        max = tmp2[j];
        ix = j;
      }
    }
    tmp2[ix] = (short) - 1;
    tmp[i] = ix;
  }

  medianIndex = tmp[n >> 1];
  return (ind[medianIndex]);
}

/*  Set adaptive gamma parameters of lower-band postfilter */
void ownSetAdaptGamma(
short *pSynth,   /* (i)    lower-band synthesis prior to postfiltering */
short *pMemFlt,  /* (i/o)  median filter memory */
int   rate,      /* (i)    decoder bit rate */
short *pGamma1,  /* (o)    gamma parameter */
short *pGamma2)  /* (o)    gamma parameter */
{
  /* Variables declarations */
  int   acc32;
  int   i, j;
  short Th;

  /* set gammas */
  if (rate >= 14000)     /* >= 14 kbit/s */
  {
    *pGamma1 = G7291_GAMMA1_PST12K;
    *pGamma2 = G7291_GAMMA2_PST12K;
  }
  else
  {
    if (rate == 12000)   /* == 12 kbit/s */
    {
      /* compute frame energy */
      ippsDotProd_16s32s_Sfs(pSynth, pSynth, G7291_LFRAME2, &acc32, -1);
      acc32 = ShiftL_32s(acc32, 16);
      Th = (short)(acc32>>16);

      /* sSmooth Th */
      j = G7291_NMAX - 1;
      for (i = 0; i < j; i++)
      {
        pMemFlt[i] = pMemFlt[i + 1];
      }
      pMemFlt[j] = Th;
      Th = gmed_n(pMemFlt, G7291_NMAX);

      acc32 = Th * G7291_GAMMA1_PST12K;
      acc32 += (IPP_MAX_16S - Th) * G7291_GAMMA1_PST;
      *pGamma1 = Cnvrt_NR_32s16s(acc32 << 1);

      acc32 = Th * G7291_GAMMA2_PST12K;
      acc32 += (IPP_MAX_16S - Th) * G7291_GAMMA2_PST;
      *pGamma2 = Cnvrt_NR_32s16s(acc32 << 1);
    }
    else                        /* == 8 kbit/s */
    {
      *pGamma1 = G7291_GAMMA1_PST;
      *pGamma2 = G7291_GAMMA2_PST;
    }
  }
  return;
}
/*  Compute attenuation factor e_num/e_den */
static short getV(
int e_num, /* (i)   numerator */
int e_den)  /* (i)   denominator */
{
  short v, n1, n2, d, n;
  int   v32, e_den32;
  short e_num16;

  n1 = Exp_32s(e_num);
  n2 = Exp_32s(e_den);
  e_den32 = (e_den << n2) >> 1;
  e_num16 = (short)((e_num << n1) >> 16);
  d = Div_32s16s(e_den32, e_num16);
  v32 = d;
  ippsInvSqrt_32s_I(&v32, 1);
  n = n2 - n1 + 14;
  v = (short)(v32 >> (23 - (n >> 1)));
  if((n & 1) != 0)
  {
    v32 = (v * 0x5a82) << 1;
    v = (short)(v32>>16) << 1;
  }
  v = v << 8;
  return (v);
}

/*  Reduce pre/postecho in lower and higher band
    Add CELP and TDAC synthesis in lower-band  */
void ownEchoReduction(
short *sOutLow,       /* (o)   output samples lowband */
short *sBasicLow,     /* (i)   12k layer contribution (lowband)  */
short *sTdacAddLow,   /* (i)   TDAC layer add-on for lowand  */
short *vmlOld,        /* (i)   needed for lowband gain smoothing  */
short *sOutHi,        /* (o)   output samples highband */
short *sBasicHi,      /* (i)   14k layer contribution (highband) */
short *sTdacIncHi,    /* (i)   highband signal TDBWE combined with TDAC */
short *vmhOld,        /* (i)   needed for highband gain smoothing */
short work,           /* (i)   if zero ->do not cancel echo (rate<=14000) */
short beginIndexLow,  /* (i)   start index for false alarm zone in lower-band  */
short endIndexLow,    /* (i)   stop index for false alarm zone in lower-band */
short beginIndexHigh, /* (i)   start index for false alarm zone in higher-band */
short endIndexHigh)   /* (i)   stop index for false alarm zone in higher-band */
{
  int i, j, acc32;
  int   EsBasicLow, EsTdacAddLow, EsBasicHi, EsTdacIncHi, EsTdacIncHi081;
  short vl, vh, Hi, Lo;
  IPP_ALIGNED_ARRAY(16, short, vml, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, vmh, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, sTdacAddLow_tilt, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, sBasicLow_tilt, G7291_LFRAME2);

  /* apply tilt on lowband before comparing energies */
  ippsSub_16s(&sBasicLow[1], sBasicLow, &sBasicLow_tilt[1], G7291_LFRAME2-1);
  ippsSub_16s(&sTdacAddLow[1], sTdacAddLow, &sTdacAddLow_tilt[1], G7291_LFRAME2-1);
  sBasicLow_tilt[0] = 0;
  sTdacAddLow_tilt[0] = 0;

  if(!work) {
    ippsSet_16s(0x7fff, vml, G7291_LFRAME2);
    ippsSet_16s(0x7fff, vmh, G7291_LFRAME2);
  } else
  {
    if (beginIndexLow == 0 && endIndexLow == 159)
        ippsSet_16s(0x7fff, vml, G7291_LFRAME2);
    else
    {
      /* compare energies, get scale factors */
      for(j=0; j<G7291_LFRAME2; j+=G7291_PRE_SUBFRAME)
      {
          /* get energies */
          ippsDotProd_16s32s_Sfs(&sBasicLow_tilt[j], &sBasicLow_tilt[j], G7291_PRE_SUBFRAME, &EsBasicLow, -1);
          ippsDotProd_16s32s_Sfs(&sTdacAddLow_tilt[j], &sTdacAddLow_tilt[j], G7291_PRE_SUBFRAME, &EsTdacAddLow, -1);
          EsBasicLow += 100;
          EsTdacAddLow += 100;

          if(EsBasicLow >= EsTdacAddLow)
              vl = 0x7fff;
          else
              vl = getV(EsBasicLow, EsTdacAddLow);

          ippsSet_16s(vl, &vml[j], G7291_PRE_SUBFRAME);
      }
      /* beginIndexLow = 0; endIndexLow = 159; no preecho supression */
      ippsSet_16s(0x7fff, &vml[beginIndexLow], endIndexLow-beginIndexLow+1);
    }
    if (beginIndexHigh == 0 && endIndexHigh == 159)
        ippsSet_16s(0x7fff, vmh, G7291_LFRAME2);
    else
    {
      /* compare energies, get scale factors */
      for(j=0; j<G7291_LFRAME2; j+=G7291_PRE_SUBFRAME)
      {
          /* get energies */
          ippsDotProd_16s32s_Sfs(&sBasicHi[j], &sBasicHi[j], G7291_PRE_SUBFRAME, &EsBasicHi, -1);
          EsBasicHi += 100;
          ippsDotProd_16s32s_Sfs(&sTdacIncHi[j], &sTdacIncHi[j], G7291_PRE_SUBFRAME, &EsTdacIncHi, -1);
          EsTdacIncHi += 100;

          L_Extract(EsTdacIncHi, &Hi, &Lo);
          EsTdacIncHi081 = Mpy_32_16(Hi, Lo, 0x67ae);

          if(EsBasicHi >= EsTdacIncHi081)
              vh = 0x7fff;
          else
              vh = getV(EsBasicHi, EsTdacIncHi);

          ippsSet_16s(vh, &vmh[j], G7291_PRE_SUBFRAME);
      }
      /* beginIndexLow = 0; endIndexLow = 159; no preecho supression */
      ippsSet_16s(0x7fff, &vmh[beginIndexHigh], endIndexHigh-beginIndexHigh+1);
    }

    /* EnvAdaption - smoothing of gains */
    acc32 = *vmhOld * 0x6ccd + vmh[0] * 0x1333;
    vmh[0] = Cnvrt_NR_32s16s(acc32 << 1);
    acc32 = *vmlOld * 0x6ccd + vml[0] * 0x1333;
    vml[0] = Cnvrt_NR_32s16s(acc32 << 1);

    for(i = 1; i < G7291_LFRAME2; i++)
    {
        acc32 = vmh[i-1] * 0x6ccd + vmh[i] * 0x1333;
        vmh[i] = Cnvrt_NR_32s16s(acc32 << 1);
        acc32 = vml[i-1] * 0x6ccd + vml[i] * 0x1333;
        vml[i] = Cnvrt_NR_32s16s(acc32 << 1);
    }

    /* update vmh vml memory */
    *vmlOld = vml[(G7291_LFRAME2) - 1];
    *vmhOld = vmh[(G7291_LFRAME2) - 1];
  }

  ippsMul_NR_16s_Sfs(sTdacAddLow, vml, sOutLow, G7291_LFRAME2, 15);
  ippsAdd_16s(sBasicLow, sOutLow, sOutLow, G7291_LFRAME2);
  ippsMul_NR_16s_Sfs(sTdacIncHi, vmh, sOutHi, G7291_LFRAME2, 15);
  return;
}

/*  Determine start/stop indices for false alarm zone in lower- and higher- band */
void ownDiscriminationEcho(
short *pReconstrSignal,/* (i)   reconstructed signal, output of the imdct transform */
short *pMem,           /* (i)   memory of the imdct transform, used in the next frame */
int *pMaxPrev,         /* (i/o) maximum energy in the previous reconstructed frame */
short *pBeginIdx,      /* (o)   starting index of the eventual false alarm zone */
short *pEndIdx)        /* (o)   ending index of the eventual false alarm zone */
{
  int i, j;
  int es_tdac[6]; /* 0..3: energy of the 4 subframes, 4..5: energy of the future subframes */
  int minEs, maxEs, max_rec = 0;
  short *ptr;

  /*energy of low bands 4 present and 2 future sub-frames */
  ptr = pReconstrSignal;
  for(j = 0; j < 4; j++)       /*4 present subframes */
  {
    ippsDotProd_16s32s_Sfs(ptr, ptr, G7291_PRE_SUBFRAME, &es_tdac[j], 0);
    es_tdac[j] = Add_32s(es_tdac[j], 100);
    ptr += G7291_PRE_SUBFRAME;
  }

  ptr = pMem;
  for(j = 4; j < 6; j++)       /*2 future subframes */
  {
    ippsDotProd_16s32s_Sfs(ptr, ptr, G7291_PRE_SUBFRAME, &es_tdac[j], -1);
    es_tdac[j] = Add_32s(es_tdac[j], 100);
    ptr += G7291_PRE_SUBFRAME;
  }

  minEs = es_tdac[0];          /* for memorising the min energy */
  maxEs = es_tdac[0];          /* for memorising the max energy */
  *pBeginIdx = 0;

  for(i = 1; i < 6; i++)
  {
    if (es_tdac[i] > maxEs)
    {
      maxEs = es_tdac[i];      /* max energy low band, 4 present and 2 future subframes */
      *pBeginIdx = (short)(i * G7291_PRE_SUBFRAME);
    }
    if (i == 3)
    {
      max_rec = maxEs;         /*save partial max result for the 4 present subframes */
    }
  }

  if((*pMaxPrev >> 5) >= max_rec)  /*post-echo situation, preecho false alarm detection desactivated */
  {
    *pBeginIdx = G7291_LFRAME2;
    *pEndIdx = G7291_LFRAME2 - 1;  /* begin > end --> nothing is done */
    *pMaxPrev = max_rec;
  }
  else
  {
    *pMaxPrev = max_rec;
    for(i = 1; i < 4; i++)
    {
      if (es_tdac[i] < minEs)
      {
        minEs = es_tdac[i];    /* min energy low band, 4 present subframes only */
      }
    }
    if ((maxEs >> 4) < minEs) /* no big energy change --> no preecho ->false alarm zone:the whole frame */
    {
      *pBeginIdx = 0;
      *pEndIdx = G7291_LFRAME2 - 1;
    }
    else
    {
      *pEndIdx = *pBeginIdx + 2 * G7291_PRE_SUBFRAME - 1; /*length of the false alarm zone is 2 subframes */
      if ((*pEndIdx - G7291_LFRAME2 - 1) > 0)
      {
        *pEndIdx = G7291_LFRAME2 - 1;
      }
    }
  }
  return;
}
