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
// Purpose: G.728 speech codec: 40 kbits bitrate.
//
*/

#include "owng728.h"
#include "g728tables.h"

#define ONE_Q11  (1 << 11)
#define ONE_Q13  (1 << 13)
#define ONE_Q14  (1 << 14)
#define ONE_Q15  (1 << 15)
#define TWO_Q14  (2 << 14)

#define COPY_POINTER(x)   \
  objJ->x = obj->x

#define GET_POINTER(x)    \
  objJ->p_##x = &(obj->x)



void Init_G728JDecoder(G728Decoder_Obj *obj) {
  G728J_Obj *objJ = &(obj->objJ);
  COPY_POINTER(vecSyntFltrCoeffs);
  COPY_POINTER(tmpSyntFltrCoeffs);
  COPY_POINTER(vecLGPredictorCoeffs);
  COPY_POINTER(tmpLGPredictorCoeffs);
  COPY_POINTER(vecLGPredictorState);
  GET_POINTER(illcond);
  GET_POINTER(illcondg);
  GET_POINTER(illcondp);
  GET_POINTER(scaleSyntFltrCoeffs);
  GET_POINTER(scaleLGPredictorCoeffs);
  COPY_POINTER(nlssttmp);
  COPY_POINTER(rexpMem);
  COPY_POINTER(rexplgMem);
  COPY_POINTER(sttmp);
  objJ->after_limiter_98 = -16384;
  objJ->ap_q11 = 0;
  objJ->dml = 0;
  objJ->dms = 0;
  objJ->GAIN_state = 0;
  objJ->DLQ_NLSGAIN = 14;
  objJ->G_CNT = 0;
  objJ->GC_CNT = 0;
  objJ->GC_FLAG = 0;
  objJ->TCQ_decoder_state=0;
  objJ->invGain.val = 16384;
  objJ->invGain.sf  =  -4;
  objJ->excGain.val =  16384;
  objJ->excGain.sf  =  -7;   ;

  ippsZero_16s(objJ->ET, RMS_BUF_LEN);
  ippsZero_16s(objJ->TCQ_STTMP_q2, NFRSZ + 1);
}

void Init_G728JEncoder(G728Encoder_Obj *obj) {
  G728J_Obj *objJ = &(obj->objJ);
  COPY_POINTER(vecSyntFltrCoeffs);
  COPY_POINTER(tmpSyntFltrCoeffs);
  COPY_POINTER(vecLGPredictorCoeffs);
  COPY_POINTER(tmpLGPredictorCoeffs);
  COPY_POINTER(vecLGPredictorState);
  GET_POINTER(illcond);
  GET_POINTER(illcondg);
  GET_POINTER(illcondp);
  GET_POINTER(scaleSyntFltrCoeffs);
  GET_POINTER(scaleLGPredictorCoeffs);
  COPY_POINTER(nlssttmp);
  COPY_POINTER(rexpMem);
  COPY_POINTER(rexplgMem);
  COPY_POINTER(sttmp);

  objJ->after_limiter_98 = -16384;
  objJ->ap_q11 = 0;
  objJ->dml = 0;
  objJ->dms = 0;
  objJ->GAIN_state = 0;
  objJ->DLQ_NLSGAIN = 14;
  objJ->G_CNT = 0;
  objJ->GC_CNT = 0;
  objJ->GC_FLAG = 0;
  objJ->distortion_metric = 0;

  objJ->invGain.val = 16384;
  objJ->invGain.sf  =  -4;
  objJ->excGain.val =  16384;
  objJ->excGain.sf  =  -7;

  ippsZero_16s(objJ->TCQ_STTMP_q2, NFRSZ + 1);
  ippsZero_16s(objJ->ET, RMS_BUF_LEN);
}




static const Ipp32s TCQ_next_state[N_STATES][N_BRANCHES] = {
  {0, 1},
  {2, 3},
  {0, 1},
  {2, 3}
};

static const Ipp32s TCQ_trans_from_src_to_dst [N_STATES][N_STATES] = {
  {0, 2, -1, -1},
  {-1, -1, 1, 3},
  {2, 0, -1, -1},
  {-1, -1, 3, 1}
};

static const Ipp16s Yk [Q_CELLS][N_STATES] = {
  {-11502, -9955, -8962, -8209},
  { -7592, -7062, -6595, -6174},
  { -5788, -5430, -5095, -4780},
  { -4480, -4194, -3919, -3655},
  { -3399, -3151, -2910, -2674},
  { -2444, -2218, -1996, -1777},
  { -1562, -1348, -1138,  -928},
  {  -721,  -514,  -308,  -103},
  {   103,   308,   514,   721},
  {   928,  1138,  1348,  1562},
  {  1777,  1996,  2218,  2444},
  {  2674,  2910,  3151,  3399},
  {  3655,  3919,  4194,  4480},
  {  4780,  5095,  5430,  5788},
  {  6174,  6595,  7062,  7592},
  {  8209,  8962,  9955, 11502},
};

static const Ipp16s log_pol[LOG_POL_ORDER] = {28437, -13945, 8131, -3787, 893};

static const Ipp16s FACV_vbd[PREDICTOR_ORDER] = {
  15360, 14400, 13500, 12656, 11865, 11124, 10428, 9777, 9166, 8593
};

void TCQ_quantize_resid(Ipp16s resid, Ipp16s state, Ipp16s *ch_index, Ipp16s *qerror);

void Vbd_log_calc_and_lim97( Ipp16s  *ET, Ipp16s  *vecLGPredictorState, Ipp16s  *speech_to_vbd_transition,
                             Ipp32s  *RMS_Q11, Ipp16s  after_limiter_98);

void LoggainLinearPrediction_G728J(Ipp16s *pVecLGPredictorCoeffs, Ipp16s *pLGPredictorState, Ipp32s *pGainLog);

void Decode_one_sample(const Ipp16s *vecSyntFltrCoeffs, const Ipp8u *pIdxs,  Ipp16s *TCQ_STTMP_q2,Ipp16s *ET,
                       Ipp16s *TCQ_decoder_st,Ipp16s *dst, Ipp32s_EC_Sfs excGain);
void G728J_adaptation(G728J_Obj *objJ,Ipp16s *icount,Ipp16s *DurbinFaultFlag);
void TCQ_backward_gain_adapter(G728J_Obj *objJ);

/* //////////////////Encode///////////////////// */

APIG728_Status apiG728JEncode(G728Encoder_Obj* encoderobj, Ipp16s *pSrc, Ipp8u *dst)
{
   Ipp8u idxs[G728_J_FRAMESIZE_SAMPLES], *pIdxs = idxs;
   G728J_Obj *objJ = &(encoderobj->objJ);
   int i,len=G728_J_FRAMESIZE_SAMPLES;
   Ipp16s icount = 0;
   Ipp16s DurbinFaultFlag = 0;

   /* For every Vector (5 samples) perform the following blocks. */
   for (; len >= IDIM; len -= IDIM) {
      ippsCodebookSearchTCQ_G728_16s8u((const Ipp16s *)pSrc, (const Ipp16s *)objJ->vecSyntFltrCoeffs,pIdxs,
                             objJ->TCQ_STTMP_q2, objJ->ET, &objJ->distortion_metric,
                             objJ->invGain, objJ->excGain );
// Estimate the gain for the next 5 samples.
      TCQ_backward_gain_adapter(objJ);
      G728J_adaptation(objJ,&icount, &DurbinFaultFlag);

      pIdxs += 5;
      pSrc += 5;
   }

   pIdxs = idxs;
   /*Pack bitstream*/
   for(i=0;i<G728_J_FRAMESIZE_SAMPLES;i+=8) {
      dst[0] =  ((pIdxs[1] & 0x7) << 5) | (pIdxs[0] & 0x1F);
      dst[1] =  ((pIdxs[3] & 0x1 )<< 7) | ((pIdxs[2] & 0x1F) << 2) | ((pIdxs[1] >> 3)& 0x3);
      dst[2] =  ((pIdxs[4] & 0xf )<< 4) | ((pIdxs[3] >> 1)& 0xf);
      dst[3] =  ((pIdxs[6] & 0x3 )<< 6) | ((pIdxs[5] & 0x1F) << 1) | ((pIdxs[4] >> 4)& 0x1);
      dst[4] =  ((pIdxs[7] & 0x1f )<< 3) | ((pIdxs[6] >> 2)& 0x7);
      dst += 5;
      pIdxs += 8;
   }

  return APIG728_StsNoErr;
}




int calc_10log10(int AA0) {
   int i;
  long int T;
  short int exp;

  if (AA0 < 1) {
    AA0 = 1; //        illegal input number clipping to 0dB.
  } else {
    // Scale the input number to the range 1Q14..2Q14
    exp = 3; // Q3 is the difference Q14 - Q11
     while (AA0 < ONE_Q14) {
       AA0 = (AA0 << 1);
       exp -= 1;
     }
     while (TWO_Q14 <= AA0) {
       AA0 = (AA0 >> 1L);
       exp += 1;
     }
     T = AA0 - ONE_Q14;
     T <<= 1; // Translate T from Q14 to Q15
     AA0 = log_pol[LOG_POL_ORDER-1];
     for (i = (LOG_POL_ORDER-1); i >= 1; i--) {
       AA0 = ((T*AA0)*2+(log_pol[i-1] << 16) + ONE_Q15) >> 16;
     }
     AA0 = ((AA0*T)*2+ONE_Q15) >> 16;
     AA0 = AA0 >> 4; // Translate AA0 to Q11.
     AA0 = AA0*5;    // Divide AA0 by 2 and multiply by 10 to get dB units.
     // Add the mantisa, and calculate the 10 log dB value of the input
     AA0 = AA0 + ((LOG_2 * exp) >> 2);
  }
  return (AA0);
}



void Vbd_log_calc_and_lim97( Ipp16s  *ET, Ipp16s  *vecLGPredictorState,Ipp16s  *speech_to_vbd_transition,
                             Ipp32s  *RMS_Q11, Ipp16s  after_limiter_98) {
   int i;
  long int AA0;

  AA0 = 0;
  for (i = 0; i < RMS_BUF_LEN; i++) {
    AA0 = AA0 +ET[i] * ET[i];
  }
  // Divide by RMS_BUF_LEN, and scale AA0 from Q22 to Q11.
  AA0 = (AA0 + ONE_Q13) >> 14;
  *RMS_Q11 = AA0; // 32 bit operation.
  AA0 = calc_10log10(AA0); // BLOCK #J.16
  AA0 = AA0 + (after_limiter_98 << 2); // 32 bit operation.
  AA0 = AA0 >> 2; // scale to Q9 format.
  // block 97: limit GSTATE to -32dB.
  if (AA0 < -16384) {
    AA0 = -16384;
  }
  // If immediately after transition, than use the GSTATE of the speech mode.
  if (*speech_to_vbd_transition == 1) {
    *speech_to_vbd_transition = 0;
  } else {
    for(i = LPCLG - 1; i>0; i--) {
      vecLGPredictorState[i] = vecLGPredictorState[i-1];  // or above ?????? (RD)
    }
    vecLGPredictorState[0] = (Ipp16s)AA0;
  }
}




void Log_gain_weighting(Ipp16s *GAIN_state, Ipp16s *after_limiter_98, Ipp16s al_q11) {
  long int AA0;

  AA0 = *GAIN_state;
  AA0 = (((AA0 << 6) - AA0) + *after_limiter_98) >> 6; // 63/64 iir.
  AA0 = (AA0*(ONE_Q11 - al_q11) + *after_limiter_98*al_q11) >> 11;
  *after_limiter_98 = (Ipp16s)AA0;
  *GAIN_state = (Ipp16s)AA0;
}

  void Gain_inverse(Ipp32s_EC_Sfs *excGain, Ipp32s_EC_Sfs *invGain, Ipp16s DLQ_NLSGAIN){

  short inv_nls;
  Ipp16s DLQ_GAIN = (Ipp16s)(excGain[0].val);
  Ipp16s DLQ_inv_GAIN = (Ipp16s)(invGain[0].val);

  excGain[0].sf = (Ipp32s)(18 - CODEBOOK_Q - DLQ_NLSGAIN);

  // Initialize the numerator for inversion.
  Divide_16s(16384, 14, DLQ_GAIN, DLQ_NLSGAIN, &DLQ_inv_GAIN, & inv_nls);
  // The divide is function Annex G/G.728.
  invGain[0].sf = (Ipp32s)(-2 - inv_nls + CODEBOOK_Q + 1);
  invGain[0].val = DLQ_inv_GAIN;

}

void Signal_classifier(Ipp16s *tmpSyntFltrCoeffs,Ipp32s *GC_ATMP_SUM) {
  int i;

  *GC_ATMP_SUM = 0;
  for (i = 1; i < LPC-1/*+1*/; i++) {
    *GC_ATMP_SUM = *GC_ATMP_SUM + Abs_16s(tmpSyntFltrCoeffs[i]);
  }
}


void Gain_compensation_decision(Ipp16s *vecLGPredictorState, Ipp16s *GC_CNT,Ipp16s *G_CNT, Ipp16s *GC_FLAG,
                                Ipp16s  *GC_ATMP1, Ipp32s *GDIFF, Ipp32s *G_AVE, Ipp32s *GC_ATMP_SUM) {
  *GDIFF = vecLGPredictorState[0] - *G_AVE;

  if(*GDIFF < G_TRS) {
     *G_AVE = ((*G_AVE << G_CONST) - *G_AVE + vecLGPredictorState[0]) >> G_CONST;
     (*G_CNT)++;
  } else {
     if(*G_CNT > G_LEN) {
        if (((*GC_ATMP_SUM * ATMP_CONST) >> 3) < Abs_16s(*GC_ATMP1)) {
           *GC_FLAG = 1;
           *GC_CNT = GC_CNT_INIT;
        }
     }
     *G_AVE = vecLGPredictorState[0];
     *G_CNT = 0;
  }
}


void Gain_compensation(Ipp16s *DLQ_NLSGAIN,Ipp16s *GC_CNT,Ipp16s *GC_FLAG) {
   if (*GC_FLAG == 1) {
      if (*DLQ_NLSGAIN > GC_NLS_LIMIT_INIT-1) {
         if (*DLQ_NLSGAIN > GC_NLS_LIMIT_INIT) {
            (*GC_CNT)--;
         }
         *DLQ_NLSGAIN = *DLQ_NLSGAIN -GC_COMPENSATION_INIT;
         if (*DLQ_NLSGAIN < GC_NLS_LIMIT_INIT) {
            (*DLQ_NLSGAIN) = GC_NLS_LIMIT_INIT;
         }
         if (*GC_CNT == 0) {
            *GC_FLAG = 0;
         }
      }
   }
}




/* /////////////Common/////////////////////// */

void TCQ_backward_gain_adapter(G728J_Obj *objJ)
{
  int diff;
  Ipp32s loggain;
  Ipp32s RMS_Q11 = 0;
  Ipp16s gain, nlsgain;
  Ipp16s al_q11 = 0;

  // Calculate the log gain RMS value for the last selected path segment.
  Vbd_log_calc_and_lim97( objJ->ET, objJ->vecLGPredictorState,&(objJ->speech_to_vbd_transition),
                             &RMS_Q11, objJ->after_limiter_98);

  // Calculate the dms and dml DLQ decision parameters.
  objJ->dms = (objJ->dms << 5) - objJ->dms;
  objJ->dml = (objJ->dml << 7) - objJ->dml;
  objJ->dms += RMS_Q11;
  objJ->dml += RMS_Q11;
  objJ->dms >>= 5;
  objJ->dml >>= 7;
  diff = Abs_32s(objJ->dms - objJ->dml);
  if (diff >= (objJ->dml >> 3L)) {
    // Average Power is variable, non stationary signals such
    // as speech. ap -> 2, and unlock the quantizer.
    objJ->ap_q11 = (objJ->ap_q11 * 15 ) >> 4; // ap = ap*15.0 /16.0;
    objJ->ap_q11 += (ONE_Q11 ) >> 3;
  } else {
    if ((objJ->excGain.val >> objJ->DLQ_NLSGAIN) < 10) {
      // Idle Channel Conditions, ap -> 2, and unlock the quantizer.
      objJ->ap_q11 = (objJ->ap_q11 * 15 ) >> 4;
      objJ->ap_q11 += (ONE_Q11 ) >> 3;
    } else {
      // Average Power is constant, stationary signals, such as VBD
      // ap -> 2, and lock the quantizer.
      objJ->ap_q11 = (objJ->ap_q11 * 15 ) >> 4;
    }
  }
  if (ONE_Q11 < objJ->ap_q11) {
    al_q11 = ONE_Q11;
  } else {
    al_q11 = objJ->ap_q11;
  }

  // Log-gain predictor
  LoggainLinearPrediction_G728J(objJ->vecLGPredictorCoeffs, objJ->vecLGPredictorState, &loggain);
  // log-gain limiter 98, part of blocks 46 AND 47, P. 39.
  if(loggain > 14336) loggain = 14336;
  if(loggain < -16384) loggain = -16384;

  objJ->after_limiter_98 = (Ipp16s)loggain;
  Log_gain_weighting(&(objJ->GAIN_state), &(objJ->after_limiter_98), al_q11);
  loggain = objJ->after_limiter_98; // GAIN is the averaged log-gain.

  // add log offset, part of blocks 46 AND 47, P. 39.
  // BlockG48 - Inverse logarithmic Calculator.
  InverseLogarithmCalc(loggain, &gain, &nlsgain);

  objJ->DLQ_NLSGAIN = nlsgain;
  objJ->excGain.val = gain;         //objJ->DLQ_GAIN

  Gain_compensation_decision(objJ->vecLGPredictorState, &(objJ->GC_CNT),&(objJ->G_CNT), &(objJ->GC_FLAG),
                                 &(objJ->GC_ATMP1), &(objJ->GDIFF), &(objJ->G_AVE), &(objJ->GC_ATMP_SUM));
  Gain_compensation(&(objJ->DLQ_NLSGAIN),&(objJ->GC_CNT),&(objJ->GC_FLAG));

  Gain_inverse(&(objJ->excGain), &(objJ->invGain),objJ->DLQ_NLSGAIN) ;

}


void G728J_adaptation(G728J_Obj *objJ,Ipp16s *icount,Ipp16s *DurbinFaultFlag) {
  Ipp16s r[11];
  Ipp16s foo;
  int i, k;

  icount[0]++;
  if (icount[0] > NUPDATE) {
    icount[0] = 1;
  }
  if (icount[0] == 3) {
    for (k = 0; k <= (NUPDATE - 1); k++) {
      // Prepare STTMP & NLSSTTMP for autocore routine - HW_s.
      VscaleFive_16s(& objJ->TCQ_STTMP_q2[k * IDIM], &objJ->sttmp[k * IDIM], 12, &objJ->nlssttmp[k]);
      objJ->nlssttmp[k] += 2; // Q2 correction.
    }
    // Block G49 - Hybrid window module for synthesis filter (HW_s)
    *objJ->p_illcond = 0;
    if(ippsWinHybridBlock_G728_16s(0, objJ->sttmp, objJ->nlssttmp, objJ->rtmp, objJ->rexpMem) != ippStsNoErr){
       *objJ->p_illcond = 1;
    }
  }
  if (icount[0] == 4) {
    if (*objJ->p_illcond == G728_FALSE) {
      LevinsonDurbin(objJ->rtmp, 0, 10, objJ->tmpSyntFltrCoeffs, &foo, &foo,
         objJ->p_scaleSyntFltrCoeffs, DurbinFaultFlag, objJ->p_illcond);
      if (*objJ->p_illcond == G728_FALSE) {
        objJ->GC_ATMP1 = objJ->tmpSyntFltrCoeffs[0];
        Signal_classifier(objJ->tmpSyntFltrCoeffs,&(objJ->GC_ATMP_SUM));
        BandwidthExpansionModul(FACV_vbd, objJ->tmpSyntFltrCoeffs, *objJ->p_scaleSyntFltrCoeffs, NULL, PREDICTOR_ORDER);
      } else {

        DurbinFaultFlag[0] = 0;
        ippsCopy_16s((const Ipp16s *)objJ->vecSyntFltrCoeffs,objJ->tmpSyntFltrCoeffs,LPC);
      }
    } else {

        DurbinFaultFlag[0] = 0;
        ippsCopy_16s((const Ipp16s *)objJ->vecSyntFltrCoeffs,objJ->tmpSyntFltrCoeffs,LPC);
    }
  }
  if (icount[0] == 4) {
    Ipp16s gtmp[4];
    gtmp[0] = objJ->vecLGPredictorState[3];
    gtmp[1] = objJ->vecLGPredictorState[2];
    gtmp[2] = objJ->vecLGPredictorState[1];
    gtmp[3] = objJ->vecLGPredictorState[0];
    //BlockG43 - HW_gain
    *objJ->p_illcondg=0;
    if(ippsWinHybrid_G728_16s(0, gtmp, r, objJ->rexplgMem) != ippStsNoErr){
       *objJ->p_illcondg=1;
    };
    if (*objJ->p_illcondg == G728_FALSE) {
       LevinsonDurbin(r, 0, LPCLG, objJ->tmpLGPredictorCoeffs, &foo, &foo,
         objJ->p_scaleLGPredictorCoeffs, objJ->p_illcondp, DurbinFaultFlag);
    }
  }
  if (icount[0] == 4) {
    if (*objJ->p_illcondg == G728_FALSE) {
      if(*objJ->p_illcondg != 1) {
         BandwidthExpansionModul(cnstGainPreditorBroadenVector, objJ->tmpLGPredictorCoeffs,
                             *objJ->p_scaleLGPredictorCoeffs, objJ->vecLGPredictorCoeffs, LPCLG+1);
      }
    } else {
      DurbinFaultFlag[0] = 0;
    }
  }
  if (icount[0] == 2) {
     for (i = 0; i < PREDICTOR_ORDER; i++) {
      objJ->vecSyntFltrCoeffs[i] = objJ->tmpSyntFltrCoeffs[i];
    }
  }
}



/* ////////////////DECODER//////////////////// */


APIG728_Status apiG728JDecode(G728Decoder_Obj* decoderobj, Ipp8u *src, Ipp16s *dst)
{
  G728J_Obj *objJ = &(decoderobj->objJ);
  Ipp8u idxs[G728_J_FRAMESIZE_SAMPLES], *pIdxs = idxs;
  int i,len=G728_J_FRAMESIZE_SAMPLES;

  Ipp16s icount = 0;
  Ipp16s DurbinFaultFlag = 0;
  Ipp16s tmpvecSyntFltrCoeffs[PREDICTOR_ORDER];


   for (i = 0; i < PREDICTOR_ORDER; i++) {
     tmpvecSyntFltrCoeffs[i] = objJ->vecSyntFltrCoeffs[PREDICTOR_ORDER -i-1];
   }

   /*UnPack bitstream*/
   for(i=0;i<G728_J_FRAMESIZE_SAMPLES;i+=8) {
      pIdxs[0] = src[0] & 0x1f;
      pIdxs[1] = ((src[1] & 0x3) << 3 ) | ((src[0] >> 5 ) & 0x7);
      pIdxs[2] = (src[1] >> 2) & 0x1f;
      pIdxs[3] = ((src[2] & 0xf) << 1 ) | ((src[1] >> 7 ) & 0x1);
      pIdxs[4] = ((src[3] & 0x1) << 4 ) | ((src[2] >> 4 ) & 0xf);
      pIdxs[5] = (src[3] >> 1) & 0x1f;
      pIdxs[6] = ((src[4] & 0x7) << 2 ) | ((src[3] >> 6 ) & 0x3);
      pIdxs[7] = (src[4] >> 3) & 0x1f;

      src += 5;
      pIdxs += 8;
   }

   pIdxs = idxs;

   for (; len >= IDIM; len -= IDIM) {
      if(icount==2){
         for (i = 0; i <= (PREDICTOR_ORDER-1); i++) {
            tmpvecSyntFltrCoeffs[i] = objJ->vecSyntFltrCoeffs[PREDICTOR_ORDER -i-1];
         }
      }

      Decode_one_sample((const Ipp16s *)tmpvecSyntFltrCoeffs, pIdxs, objJ->TCQ_STTMP_q2,objJ->ET,
          &objJ->TCQ_decoder_state,dst,objJ->excGain);

      TCQ_backward_gain_adapter(objJ);
      G728J_adaptation(objJ,&icount,&DurbinFaultFlag);

      dst += 5;
      pIdxs += 5;
  }
  return APIG728_StsNoErr;
}

void Decode_one_sample(const Ipp16s *vecSyntFltrCoeffs, const Ipp8u *pIdxs, Ipp16s *TCQ_STTMP_q2,Ipp16s *ET,
                       Ipp16s *TCQ_decoder_st,Ipp16s *dst, Ipp32s_EC_Sfs excGain) {

  Ipp64s AA0;
  int i,j;
  int TCQ_decoder_state = (int)TCQ_decoder_st[0];
  int next_state;
  int subset, Flg =0;
  Ipp32s tmp, tmplabel;
  Ipp32s next_state_label[IDIM];
  Ipp32s level_selector[IDIM];
  Ipp16s codebook_level[IDIM];
  Ipp16s DLQ_nls_p_cb_q_m_18 = (Ipp16s)excGain.sf;
  Ipp16s DLQ_GAIN =  (Ipp16s)excGain.val;

   if (DLQ_nls_p_cb_q_m_18 < 0 ) {
      Flg = 1;
      DLQ_nls_p_cb_q_m_18 = -DLQ_nls_p_cb_q_m_18;
   }

   for (j = 0; j < IDIM; j++) {
      tmplabel = ((Ipp32s)pIdxs[j] >> BITS_PER_LEVEL);
      next_state_label[j] = tmplabel & 1;
      level_selector[j] = ((Ipp32s)pIdxs[j] & LEVEL_MASK);
   }

   for (j = 0; j < IDIM; j++) {      // Decoder transition module
      tmp = 0; // Use the Encoder node 0 space.
      for (i = 0; i <= (PREDICTOR_ORDER-1); i++) {
         tmp -= TCQ_STTMP_q2[(NFRSZ -PREDICTOR_ORDER) + i+j] * vecSyntFltrCoeffs[i];
      }
      tmp = Cnvrt_NR_32s16s(tmp << 2);

      next_state = TCQ_next_state [TCQ_decoder_state] [next_state_label[j]];
      subset = TCQ_trans_from_src_to_dst [TCQ_decoder_state] [next_state];
      TCQ_decoder_state = next_state;
      codebook_level[j] = Yk[level_selector[j]][subset]; // Get the codebook level

      AA0 = codebook_level[j] * DLQ_GAIN;

      if (Flg) {
        AA0 >>= DLQ_nls_p_cb_q_m_18;
      } else {
        AA0 <<= DLQ_nls_p_cb_q_m_18;
      }
      AA0 += (tmp << 16);

      AA0 >>= 16;
      TCQ_STTMP_q2 [(NFRSZ-1)+j+1] = (Ipp16s)AA0;
      dst[j] = ShiftL_16s((Ipp16s)AA0, 1);

   }

   for (i = 0; i < (RMS_BUF_LEN -IDIM); i++) {
      ET[i] = ET[IDIM+i];
   }

   for (j = 0; j < IDIM; j++) {
      ET[RMS_BUF_LEN -IDIM +j] = codebook_level[j];
   }

   ippsMove_16s((const Ipp16s *)&TCQ_STTMP_q2[IDIM],TCQ_STTMP_q2,NFRSZ);

   TCQ_decoder_st[0] = (Ipp16s)TCQ_decoder_state ;
}





void LoggainLinearPrediction_G728J(Ipp16s *pVecLGPredictorCoeffs, Ipp16s *pLGPredictorState, Ipp32s *pGainLog){
   Ipp32s dwSum, i;

   /* predict log gain */
   dwSum = 0;
   for (i=0; i<LPCLG; i++) {
      dwSum = dwSum - pLGPredictorState[i]*pVecLGPredictorCoeffs[i];
   }
   *pGainLog = dwSum >> 14;

   return;
}

