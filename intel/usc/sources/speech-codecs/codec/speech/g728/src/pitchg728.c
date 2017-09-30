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
// Purpose: G.728 speech codec: pitch search functions.
//
*/

#include "owng728.h"

void LTPCoeffsCalc(const Ipp16s *sst, Ipp32s kp, Ipp16s *gl, Ipp16s *glb, Ipp16s *pTab){
   Ipp32s aa0, aa1;
   Ipp16s den, num;
   Ipp16s nlsden, nlsnum, nlsptab, nls, b;

   /* Pitch_predictor_tab_calc*/
   ippsDotProd_16s32s_Sfs(sst-NPWSZ-kp, sst-NPWSZ-kp, NPWSZ,&aa0, 0);
   ippsDotProd_16s32s_Sfs(sst-NPWSZ, sst-NPWSZ-kp, NPWSZ,&aa1, 0);
   if(aa0 == 0) {
      *pTab = 0;
   } else if(aa1 <= 0) {
      *pTab = 0;
   } else {
      if(aa1 >= aa0) *pTab = 16384;
      else {
         VscaleOne_Range30_32s(&aa0, &aa0, &nlsden);
         VscaleOne_Range30_32s(&aa1, &aa1, &nlsnum);
         num = Cnvrt_NR_32s16s(aa1);
         den = Cnvrt_NR_32s16s(aa0);
         Divide_16s(num, nlsnum, den, nlsden, pTab, &nlsptab);
         *pTab = (Ipp16s)((*pTab)>>(nlsptab - 14));
      }
   }
   /* Update long-term postfilter coefficients*/
   if(*pTab < PPFTH) aa0 = 0;
   else              aa0 = PPFZCF * (*pTab);
   b = (Ipp16s)(aa0 >> 14);
   aa0 = aa0 >> 16;
   aa0 = aa0 + 16384;
   den = (Ipp16s)aa0;
   Divide_16s(16384, 14, den, 14, gl, &nls);
   aa0 = *gl * b;
   *glb = (Ipp16s)(aa0 >> nls);
   if(nls > 14) *gl = (Ipp16s)((*gl)>>(nls-14));
   return;
}


