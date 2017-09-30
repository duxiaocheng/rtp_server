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
// Purpose: G.722.1 speech codec: auxiliaries.
//
*/

#include "owng722.h"

__ALIGN32 CONST Ipp16s cnstDiffRegionPowerBits_G722[REG_NUM_MAX][DIFF_REG_POW_LEVELS] = {
{
  99,99,99,99,99,99,99,99,99,99,99,99,
  99,99,99,99,99,99,99,99,99,99,99,99
},
{
   4, 6, 5, 5, 4, 4, 4, 4, 4, 4, 3, 3,
   3, 4, 5, 7, 8, 9,11,11,12,12,12,12
},
{
  10, 8, 6, 5, 5, 4, 3, 3, 3, 3, 3, 3,
   4, 5, 7, 9,11,12,13,15,15,15,16,16
},
{
  12,10, 8, 6, 5, 4, 4, 4, 4, 4, 4, 3,
   3, 3, 4, 4, 5, 5, 7, 9,11,13,14,14},
{
  13,10, 9, 9, 7, 7, 5, 5, 4, 3, 3, 3,
   3, 3, 4, 4, 4, 5, 7, 9,11,13,13,13
},
{
  12,13,10, 8, 6, 6, 5, 5, 4, 4, 3, 3,
   3, 3, 3, 4, 5, 5, 6, 7, 9,11,14,14
},
{
  12,11, 9, 8, 8, 7, 5, 4, 4, 3, 3, 3,
   3, 3, 4, 4, 5, 5, 7, 8,10,13,14,14
},
{
  15,16,15,12,10, 8, 6, 5, 4, 3, 3, 3,
   2, 3, 4, 5, 5, 7, 9,11,13,16,16,16
},
{
  14,14,11,10, 9, 7, 7, 5, 5, 4, 3, 3,
   2, 3, 3, 4, 5, 7, 9, 9,12,14,15,15
},
{
   9, 9, 9, 8, 7, 6, 5, 4, 3, 3, 3, 3,
   3, 3, 4, 5, 6, 7, 8,10,11,12,13,13
},
{
  14,12,10, 8, 6, 6, 5, 4, 3, 3, 3, 3,
   3, 3, 4, 5, 6, 8, 8, 9,11,14,14,14
},
{
  13,10, 9, 8, 6, 6, 5, 4, 4, 4, 3, 3,
   2, 3, 4, 5, 6, 8, 9, 9,11,12,14,14
},
{
  16,13,12,11, 9, 6, 5, 5, 4, 4, 4, 3,
   2, 3, 3, 4, 5, 7, 8,10,14,16,16,16},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3, 2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14},
{
  13,14,14,14,10, 8, 7, 7, 5, 4, 3, 3,
   2, 3, 3, 4, 5, 5, 7, 9,11,14,14,14
}};
__ALIGN32 CONST Ipp16s cnstVectorDimentions_G722[NUM_CATEGORIES] =  { 2, 2, 2, 4, 4, 5, 5, 1};
__ALIGN32 CONST Ipp16s cnstNumberOfVectors_G722[NUM_CATEGORIES] = {10,10,10, 5, 5, 4, 4,20};
__ALIGN32 CONST Ipp16s cnstMaxBin_G722[NUM_CATEGORIES] = {13, 9, 6, 4, 3, 2, 1, 1};
__ALIGN32 CONST Ipp16s cnstMaxBinInverse_G722[NUM_CATEGORIES] = {
    2341,3277,4682,6554,8193,10923,16385,16385
};

__ALIGN32 CONST Ipp16s cnstExpectedBits_G722[NUM_CATEGORIES] = {52, 47, 43, 37, 29, 22, 16,   0};

/*F*
//  Name:      CategorizeFrame
//  Purpose:   Computes a series of categorizations
*F*/
void CategorizeFrame(Ipp16s nAccessibleBits, Ipp32s nRegs, Ipp32s nCatCtrlPossibs,
         Ipp16s* pRmsIndices, Ipp16s* pPowerCtgs, Ipp16s* pCtgBalances)
{
   Ipp16s offset, frameSize;

   if (nRegs == REG_NUM) {
      frameSize = G722_DCT_LENGTH_7kHz;
   } else {
      frameSize = G722_DCT_LENGTH_14kHz;
   }
   /* Compensation accessible bits at higher bit rates */
   if (nAccessibleBits > frameSize){
      nAccessibleBits = (Ipp16s)(nAccessibleBits - frameSize);
      nAccessibleBits = (Ipp16s)(nAccessibleBits * 5);
      nAccessibleBits = (Ipp16s)(nAccessibleBits >> 3);
      nAccessibleBits = (Ipp16s)(nAccessibleBits + frameSize);
   }
   /* calculate the offset */
   GetPowerCategories(pRmsIndices, nAccessibleBits, nRegs, pPowerCtgs, &offset);
   /* Getting poweres and balancing categories */
   GetCtgPoweresBalances(pPowerCtgs, pCtgBalances, pRmsIndices, nAccessibleBits,
      nRegs, nCatCtrlPossibs, offset);
}

/*F*
//  Name:      GetCtgPoweresBalances
//  Purpose:   Computes the poweres balances of categories.
*F*/
void GetCtgPoweresBalances(Ipp16s *pPowerCtgs, Ipp16s *pCtgBalances, Ipp16s* pRmsIndices,
         Ipp16s nBitsAvailable, Ipp32s nRegs, Ipp32s nCatCtrlPossibs, Ipp16s offset)
{
   Ipp16s nCodeBits;
   Ipp16s regNum, j;
   Ipp16s ctgMaxRate[REG_NUM_MAX], ctgMinRate[REG_NUM_MAX];
   Ipp16s tmpCtgBalances[2*CAT_CONTROL_NUM_MAX];
   Ipp16s rawMax, rawMin;
   Ipp16s rawMaxIndx=0, rawMinIndx=0;
   Ipp16s maxRateIndx, minRateIndx;
   Ipp16s max, min;
   Ipp16s tmp0, tmp1;

   nCodeBits = 0;
   for (regNum=0; regNum<nRegs; regNum++)
      nCodeBits = (Ipp16s)(nCodeBits + cnstExpectedBits_G722[pPowerCtgs[regNum]]);

   for (regNum=0; regNum<nRegs; regNum++){
      ctgMaxRate[regNum] = pPowerCtgs[regNum];
      ctgMinRate[regNum] = pPowerCtgs[regNum];
   }

   max = nCodeBits;
   min = nCodeBits;
   maxRateIndx = (Ipp16s)nCatCtrlPossibs;
   minRateIndx = (Ipp16s)nCatCtrlPossibs;

   for (j=0; j<nCatCtrlPossibs-1; j++) {
      if ((max + min) <= (nBitsAvailable << 1)) {
         rawMin = 99;
         /* Search for best region for high bit rate */
         for (regNum=0; regNum<nRegs; regNum++) {
            if (ctgMaxRate[regNum] > 0) {
               tmp0 = (Ipp16s)(ctgMaxRate[regNum] << 1);
               tmp1 = (Ipp16s)(offset - pRmsIndices[regNum]);
               tmp0 = (Ipp16s)(tmp1 - tmp0);
               if (tmp0 < rawMin) {
                  rawMin = tmp0;
                  rawMinIndx = regNum;
               }
            }
         }
         maxRateIndx--;
         tmpCtgBalances[maxRateIndx] = rawMinIndx;
            max = (Ipp16s)(max - cnstExpectedBits_G722[ctgMaxRate[rawMinIndx]]);
         ctgMaxRate[rawMinIndx]--;
         max = (Ipp16s)(max + cnstExpectedBits_G722[ctgMaxRate[rawMinIndx]]);
      } else {
         rawMax = -99;
         /* Search for best region for low bit rate */
         for (regNum=(Ipp16s)(nRegs-1); regNum >= 0; regNum--) {
            if (ctgMinRate[regNum] < NUM_CATEGORIES-1){
               tmp0 = (Ipp16s)(ctgMinRate[regNum]<<1);
               tmp1 = (Ipp16s)(offset - pRmsIndices[regNum]);
               tmp0 = (Ipp16s)(tmp1 - tmp0);
               if (tmp0 > rawMax) {
                  rawMax = tmp0;
                  rawMaxIndx = regNum;
               }
            }
         }
         tmpCtgBalances[minRateIndx] = rawMaxIndx;
         minRateIndx++;
         min = (Ipp16s)(min - cnstExpectedBits_G722[ctgMinRate[rawMaxIndx]]);
         ctgMinRate[rawMaxIndx]++;
         min = (Ipp16s)(min + cnstExpectedBits_G722[ctgMinRate[rawMaxIndx]]);
      }
   }
   for (regNum=0; regNum<nRegs; regNum++)
      pPowerCtgs[regNum] = ctgMaxRate[regNum];
   ippsCopy_16s(&tmpCtgBalances[maxRateIndx], pCtgBalances,
      nCatCtrlPossibs);
}

/*F*
//  Name:      GetPowerCategories
//  Purpose:   Calculates the category offset.
*F*/
void GetPowerCategories(Ipp16s* pRmsIndices, Ipp16s nAccessibleBits, Ipp32s nRegs,
         Ipp16s* pPowerCtgs, Ipp16s* pOffset)
{
   Ipp16s step, testOffset;
   Ipp16s i;
   Ipp16s powerCat;
   Ipp16s nBits;

   *pOffset = -32;
   step = 32;
   do {
      testOffset = (Ipp16s)((*pOffset) + step);
      nBits = 0;
      for (i=0; i<nRegs; i++){
         powerCat = (Ipp16s)((testOffset - pRmsIndices[i])>>1);
         powerCat = (Ipp16s)(IPP_MAX(powerCat, 0));
         powerCat = (Ipp16s)(IPP_MIN(powerCat, (NUM_CATEGORIES-1)));
         nBits = (Ipp16s)(nBits + cnstExpectedBits_G722[powerCat]);
      }
      if (nBits >= nAccessibleBits - 32)
         *pOffset = testOffset;
      step >>= 1;
   } while (step > 0);

   for (i=0; i<nRegs; i++) {
      pPowerCtgs[i] = (Ipp16s)(((*pOffset) - pRmsIndices[i])>>1);
      pPowerCtgs[i] = (Ipp16s)(IPP_MAX(pPowerCtgs[i], 0));
      pPowerCtgs[i] = (Ipp16s)(IPP_MIN(pPowerCtgs[i], (NUM_CATEGORIES-1)));
   }
}

/*F*
//  Name:      NormalizeWndData
//  Purpose:   Normalize input for DCT
*F*/
void NormalizeWndData(Ipp16s* pWndData, Ipp16s* pScale, Ipp32s dctSize){
   IPP_ALIGNED_ARRAY(16, Ipp16s, vecAbsWnd, G722_DCT_LENGTH_MAX);
   Ipp32s enrgL1;
   Ipp16s maxWndData, tmp, scale;

   ippsAbs_16s(pWndData, vecAbsWnd, dctSize);
   ippsMax_16s(vecAbsWnd, dctSize, &maxWndData);
   *pScale = 0;
   scale = 0;
   if (maxWndData < 14000) { /* low amplitude input shall be shifted up*/
      tmp = maxWndData;
      if(maxWndData < 438) tmp++;
      tmp = (Ipp16s)((tmp * 9587) >> 19);
      scale = 9;
      if (tmp > 0)
         scale = (Ipp16s)(Exp_16s_Pos(tmp) - 6);
   }
   ippsSum_16s32s_Sfs(vecAbsWnd, dctSize, &enrgL1, 0);
   enrgL1 >>= 7;  /* L1 energy * 1/128 */
   if (maxWndData < enrgL1) scale--;
   if (scale > 0)
      ippsLShiftC_16s(pWndData, scale, pWndData, dctSize);
   else if (scale < 0)
      ippsRShiftC_16s(pWndData, -scale, pWndData, dctSize);
   *pScale = scale;
}
