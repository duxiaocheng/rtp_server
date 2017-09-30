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
// Purpose: G.728 speech codec: internal functions.
//
*/

#include <ipps.h>
#include "owng728.h"


void VscaleFive_16s(const Ipp16s *pSrc, Ipp16s *pDst, Ipp16s rangeScale, Ipp16s *pScale) {
   Ipp16s wVal0, wVal1, maxi, mini, scale;
   Ipp32s i;

   wVal0 = wVal1 = pSrc[0];
   for( i=0; i<5; i++) {
      if(pSrc[i] > wVal0) wVal0 = pSrc[i];
      if(pSrc[i] < wVal1) wVal1 = pSrc[i];
   }


   if (!(wVal0 | wVal1)) {
      for (i=0; i<5; i++) pDst[i] = 0;
      *pScale = (Ipp16s)(rangeScale + 1);
      return;
   }

   scale = 0;

   if ( (wVal0<0) || (wVal1<(-wVal0)) ) {
      maxi = (Ipp16s)(-(1 << rangeScale));
      mini = (Ipp16s)(-(1 << (rangeScale+1)));
      if (wVal1 < mini) {
         while(wVal1 < mini) {
            wVal1 = (Ipp16s)(wVal1 >> 1);
            scale += 1;
         }
         for(i=0; i<5; i++)
            pDst[i] = (Ipp16s)(pSrc[i] >> scale);
         *pScale = (Ipp16s)(-scale);
      } else {
         while (wVal1 >= maxi) {
            wVal1 = (Ipp16s)(wVal1 << 1);
            scale += 1;
         }
         for(i=0; i<5; i++)
            pDst[i] = (Ipp16s)(pSrc[i] << scale);
         *pScale = scale;
      }
   } else {
      mini = (Ipp16s)(1 << rangeScale);
      maxi = (Ipp16s)((mini - 1) + mini);
      if (wVal0 > maxi) {
         while (wVal0 > maxi) {
            wVal0 = (Ipp16s)(wVal0 >> 1);
            scale += 1;
         }
         for (i=0; i<5; i++)
            pDst[i] = (Ipp16s)(pSrc[i] >> scale);
         *pScale = (Ipp16s)(-scale);
      } else {
         while (wVal0 < mini) {
            wVal0 = (Ipp16s)(wVal0 << 1);
            scale += 1;
         }
         for (i=0; i<5; i++)
            pDst[i] = (Ipp16s)(pSrc[i] << scale);
         *pScale = scale;
      }
   }
   return;
}

void VscaleOne_Range30_32s(const Ipp32s *pSrc, Ipp32s *pDst, Ipp16s *pScale) {
   Ipp32s dwVal;

   if(!(*pSrc)) {
      *pDst = 0;
      *pScale = 31;
      return;
   }
   dwVal = *pSrc;
   *pScale = 0;
   if(dwVal < 0) {
      while(dwVal >= (Ipp32s)0xC0000000) {
         dwVal = dwVal << 1;
         *pScale = (Ipp16s)((*pScale) + 1);
      }
   } else {
      while(dwVal < 0x40000000) {
         dwVal = dwVal << 1;
         *pScale = (Ipp16s)((*pScale) + 1);
      }
   }
   *pDst = dwVal;
   return;
}

void Divide_16s(Ipp16s numarator, Ipp16s scaleNum, Ipp16s denominator, Ipp16s scaleDen,
                Ipp16s *pQuotient, Ipp16s *pScaleQuot)
{
   Ipp16s sign = 1;
   Ipp32s prod, val0, val1, i;

   prod = numarator * denominator;
   if(prod < 0) sign = -1;

   *pScaleQuot = (Ipp16s)(scaleNum - scaleDen + 14);

   val0 = Abs_16s(numarator);
   val1 = Abs_16s(denominator);

   if(val0 < val1) {
      *pScaleQuot = (Ipp16s)((*pScaleQuot) + 1);
      val0 = val0 << 1;
   }

   *pQuotient = 0;
   for(i = 0; i < 15; i++) {
      *pQuotient = ShiftL_16s(*pQuotient, 1);
      if(val0 >= val1) {
         *pQuotient = Add_16s(*pQuotient, 1);
         val0 = val0 - val1;
      }
      val0 = val0 << 1;
   }

   if(val0 >= val1) *pQuotient = Add_16s(*pQuotient, 1);
   if(sign < 0) *pQuotient = (Ipp16s)(-(*pQuotient));
   return;
}

void LevinsonDurbin(const Ipp16s *rtmp, Ipp32s ind1, Ipp32s ind2, Ipp16s *pTmpSyntFltrCoeffs, Ipp16s *rc1,
         Ipp16s *alphatmp, Ipp16s *pScaleSyntFltrCoeffs, Ipp16s *illcondp, Ipp16s *illcond) {

   if ((*illcond==1) || (rtmp[0]<=0)) {
      *illcond = 1;
   } else {
      IppStatus sts;
      if(ind1 > 1)
         sts = ippsLevinsonDurbin_G728_16s_ISfs(rtmp, ind1, ind2, pTmpSyntFltrCoeffs, alphatmp, pScaleSyntFltrCoeffs);
      else
         sts = ippsLevinsonDurbin_G728_16s_Sfs(rtmp, ind2, pTmpSyntFltrCoeffs, rc1, alphatmp, pScaleSyntFltrCoeffs);
      if(sts != ippStsNoErr){
         *illcond = 1;
         if(sts == ippStsLPCCalcErr)
            *illcondp = 1;
      }
   }
}

void GetShapeGainIdxs(Ipp16s codebookIdx, Ipp32s *pShapeIdx, Ipp32s *pGainIdx, Ipp16s rate) {

   if(rate==G728_Rate_12800){
      *pShapeIdx = codebookIdx >> 2,
      *pGainIdx = codebookIdx - ((*pShapeIdx)*NG_128);
      *pShapeIdx += NCWD >> 1;
   } else if(rate==G728_Rate_9600) {
      Ipp32s idx;
      idx = codebookIdx >> 2,
      *pGainIdx = codebookIdx - (idx * NG_96);
      idx = ((idx>>2)<<3) + (idx&0x3);
      *pShapeIdx = idx + NCWD3_4;
   } else if (rate==G728_Rate_16000){
      *pShapeIdx = codebookIdx >> 3,
      *pGainIdx = codebookIdx & 7;
   }
}




