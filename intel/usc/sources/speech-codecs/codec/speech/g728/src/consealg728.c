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
// Purpose: G.728 speech codec: rame or packed loss concealment function for G728 decoder
//           (Annex I)
//
*/

#include "owng728.h"
#include "g728tables.h"

#define VTH           7022         /* Voicing threshold for frame erasures*/
#define DIMINV       13107         /* Inverse of vector dimension (=1/5 = 0.2)*/
#define FEGAINMAX     1024         /* Maximum gain DB increase after frame erasure*/

/* LinToDb constants*/

static __ALIGN32 CONST Ipp16s c_L[6] = {
-3400, 15758, -10505,
 9338, -9338,   9961};


#define FAC1     24660
#define GOFF_L  -16384
#define THRQTR   24576

/* On my opinion, these values can be divided into two for the best PESQ results (Igor S. Belyakov) */

static __ALIGN32 CONST Ipp16s voicedfegain[5] = {
   26214, 26214, 19661, 13107, 6554};

static __ALIGN32 CONST Ipp16s unvoicedfegain[6] = {
   32767, 32767, 26214, 19661, 13107, 6554};

static void LinToDB(Ipp16s x, Ipp16s nlsx, Ipp16s *ret);

static Ipp16s Rand_G728_16s( Ipp16s *seed ) /* G.723.1 */
{
   /* initial value 11111*/
   *seed = (Ipp16s)(*seed * 31821 + 13849);
   return (Ipp16s)((*seed>>9)+69);
}

static void LinToDB(Ipp16s x, Ipp16s nlsx, Ipp16s *ret) {
   Ipp32s aa0, aa1, p, i;

   aa0 = (x - THRQTR) << 1;
   aa1 = 0;
   for(i=5; i>0; i--) {
      aa1 = aa1 + c_L[i];
      p = aa1 * aa0;
      aa1 = p >> 16;
   }
   aa1 = aa1 + c_L[0];
   aa1 = aa1 >> 3;
   aa0 = 15 - nlsx;
   aa0 = aa0 << 10;
   aa1 = aa1 + aa0;
   aa1 = aa1 * FAC1;

   aa1 = aa1 >> 14;
   aa1 = aa1 - GOFF_L;
   *ret = (Ipp16s)aa1;

   return;
}

void Set_Flags_and_Scalin_Factor_for_Frame_Erasure(Ipp16s fecount, Ipp16s ptab, Ipp16s kp, Ipp16s* fedelay, Ipp16s* fescale,
                                                   Ipp16s* nlsfescale, Ipp16s* voiced, Ipp16s* etpast, Ipp16s *avmag, Ipp16s *nlsavmag)
{
   Ipp16s n10msec, nls;
   Ipp32s aa0, i;

   n10msec = (Ipp16s)(fecount >> 2);

   if(n10msec == 0) {
      if(ptab > VTH) {
         *fedelay = kp;
         *voiced = G728_TRUE;
      } else {
         *voiced = G728_FALSE;
         aa0 = 0;
         for(i = -40;i < 0; i++)
            aa0 = aa0 + Abs_32s(etpast[i]);
         VscaleOne_Range30_32s(&aa0, &aa0, &nls);
         *avmag = Cnvrt_NR_32s16s(aa0);
         *nlsavmag = (Ipp16s)(nls - 16 + 3);
      }
   }
   if(*voiced == G728_TRUE) {
      if(n10msec < 5) *fescale = voicedfegain[n10msec];
      else *fescale = 0;
   } else {
      if(n10msec < 6) {
         *fescale = unvoicedfegain[n10msec];
         aa0 = (*fescale) * (*avmag);
         VscaleOne_Range30_32s(&aa0, &aa0, nlsfescale);
         *fescale = Cnvrt_NR_32s16s(aa0);
         *nlsfescale = (Ipp16s)((*nlsfescale)+(*nlsavmag) - 1);
      } else {
         *fescale = 0;
      }
   }

   return;
}

void ExcitationSignalExtrapolation(Ipp16s voiced, Ipp16s *fedelay, Ipp16s fescale,
         Ipp16s nlsfescale, Ipp16s* etpast, Ipp16s *et, Ipp16s *nlset, Ipp16s *seed) {
   Ipp16s temp = fescale, nlstemp = 15,den,nlsden,nls;
   Ipp32s i, aa0, aa1;

   if(voiced == G728_TRUE) {
      for(i=0; i<IDIM; i++)
         etpast[i] = etpast[i-(*fedelay)];
     /* nlstemp = 15;*/
   }
   if(voiced == G728_FALSE) {
      *fedelay = Rand_G728_16s(seed);
      aa1 = 0;
      for(i=0; i<IDIM; i++) {
         etpast[i] = etpast[i-(*fedelay)];
         aa1 = aa1 + Abs_32s(etpast[i]);
      }
      if((aa1==0)||(fescale==0)) {
         temp = 0;
         nlstemp = 15;
      } else {
         VscaleOne_Range30_32s(&aa1, &aa1, &nlsden);
         den = Cnvrt_NR_32s16s(aa1);
         nlsden = (Ipp16s)(nlsden - 16);
         Divide_16s(fescale, nlsfescale, den, nlsden, &temp, &nlstemp);
      }
   }
   for(i=0; i<IDIM; i++) {
      aa0 = temp * etpast[i];
      aa0 = aa0 >> nlstemp;
      aa0 = Cnvrt_32s16s(aa0);
      etpast[i] = (Ipp16s)aa0;
   }
   VscaleFive_16s(etpast, et, 13, &nls);
   *nlset = (Ipp16s)(nls + 2);

   return;
}

void UpdateExcitationSignal(Ipp16s *etpast, Ipp16s *et, Ipp16s nlset) {
   Ipp32s i, aa0;
   Ipp16s scale;

   for(i=-KPMAX; i<-IDIM; i++)
      etpast[i] = etpast[i+IDIM];

   scale = (Ipp16s)(nlset - 2);

   if (scale >= 0) {
      for(i=0; i<IDIM; i++) {
         etpast[i-IDIM] = (Ipp16s)(et[i] >> scale);
      }
   } else {
      scale = (Ipp16s)(-scale);
      for(i=0; i<IDIM; i++) {
         aa0 = et[i];
         aa0 = aa0 << scale;
         etpast[i-IDIM] = Cnvrt_32s16s(aa0);
      }
   }

   return;
}


void UpdateLGPredictorState(Ipp16s *et, Ipp16s *pLGPredictorState) {
   Ipp32s aa0, k;
   Ipp16s nls;
   Ipp16s etrms;      /* Energy in dB of current vector*/
   Ipp16s nlsetrms;   /* NLS for etrms*/

   aa0 = 0;
   for(k=0; k<IDIM; k++)
      aa0 = aa0 + et[k]*et[k];

   VscaleOne_Range30_32s(&aa0, &aa0, &nls);
   etrms = (Ipp16s)(aa0 >> 16);
   nlsetrms = (Ipp16s)((4 + nls) -  16);
   aa0 = etrms * DIMINV;

   VscaleOne_Range30_32s(&aa0, &aa0, &nls);
   etrms = (Ipp16s)(aa0 >> 16);
   nlsetrms = (Ipp16s)(nlsetrms + nls);
   if(nlsetrms > 14) {
      etrms = 16384;
      nlsetrms = 14;
   }

   LinToDB(etrms, nlsetrms, pLGPredictorState);

   return;
}

void Log_gain_Limiter_after_erasure(Ipp32s *pGainLog, Ipp32s ogaindb, Ipp16s afterfe)
{
   Ipp32s tmp;

   if(*pGainLog > 14336) *pGainLog = 14336;
   if(*pGainLog < -16384) *pGainLog = -16384;

   tmp = (*pGainLog) - ogaindb;
   if((afterfe > 0)&&(tmp > FEGAINMAX))
      *pGainLog = ogaindb + FEGAINMAX;

   return;
}



