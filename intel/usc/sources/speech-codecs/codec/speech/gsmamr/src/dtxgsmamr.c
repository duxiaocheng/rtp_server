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
// Purpose: GSMAMR speech codec: DTX utilities.
//
*/

#include "owngsmamr.h"

#define  NB_PULSE 10 /* number of random pulses in DTX operation   */

/***************************************************************************
*  Function    : ownDecSidSyncReset_GSMAMR
***************************************************************************/
Ipp32s ownDecSidSyncReset_GSMAMR(sDTXDecoderSt *dtxState)
{

   dtxState->vSinceLastSid = 0;
   dtxState->vDTXHangCount = DTX_HANG_PERIOD;
   dtxState->vExpireCount = 32767;
   dtxState->vDataUpdated = 0;
   return 0;
}

enum enDTXStateType ownDecSidSync(sDTXDecoderSt *dtxState, RXFrameType frame_type)
{
   enum enDTXStateType newState;
   Ipp32s d_dtxHangoverAdded=0;

   /* set DTX state to: SPEECH, DTX or DTX_MUTE  */
   if ((frame_type == RX_SID_FIRST)   ||
       (frame_type == RX_SID_UPDATE)  ||
       (frame_type == RX_SID_BAD)     ||
       (((dtxState->eDTXPrevState == DTX) ||
         (dtxState->eDTXPrevState == DTX_MUTE)) &&
        ((frame_type == RX_NO_DATA) ||
         (frame_type == RX_SPEECH_BAD) ||
         (frame_type == RX_ONSET))))
   {
      newState = DTX;

      dtxState->vSinceLastSid++;

      if ((dtxState->eDTXPrevState == DTX_MUTE) &&
          ((frame_type == RX_SID_BAD) ||
           (frame_type == RX_SID_FIRST) ||
           (frame_type == RX_ONSET) ||
           (frame_type == RX_NO_DATA)))
      {
         newState = DTX_MUTE;
      }

      if( (frame_type != RX_SID_UPDATE) && (dtxState->vSinceLastSid > DTX_MAX_EMPTY_THRESH) ) {
         /* DTX_MUTE threshold exceeded*/
         newState = DTX_MUTE;
         dtxState->vSinceLastSid = 0;
      }
   } else {
      newState = SPEECH;
      dtxState->vSinceLastSid = 0;
   }

   if((dtxState->vDataUpdated == 0) && (frame_type == RX_SID_UPDATE))
      dtxState->vExpireCount = 0;

   if (dtxState->vExpireCount != 32767) dtxState->vExpireCount++;

   if ((frame_type == RX_SPEECH_GOOD)  ||
       (frame_type == RX_SPEECH_DEGRADED) ||
       (frame_type == RX_SPEECH_BAD) ||
       ((frame_type == RX_NO_DATA) && (newState == SPEECH)) /*REL-4 Version 4.1.0 */
      )
   {
      dtxState->vDTXHangCount = DTX_HANG_PERIOD;
   } else {

      if (dtxState->vExpireCount > DTX_ELAPSED_FRAMES_THRESH) {
         d_dtxHangoverAdded = 1;
         dtxState->vExpireCount = 0;
         dtxState->vDTXHangCount = 0;
      } else if (dtxState->vDTXHangCount == 0) {
         dtxState->vExpireCount = 0;
      } else {
         dtxState->vDTXHangCount--;
      }
   }

   if (newState != SPEECH) {
     /* DTX or DTX_MUTE */
      if (frame_type == RX_SID_FIRST) {
         dtxState->vSinceLastSid = 0;
         if(d_dtxHangoverAdded != 0)
            dtxState->vDataUpdated = 1;
      } else if (frame_type == RX_SID_UPDATE) {
         dtxState->vSinceLastSid = 0;
         dtxState->vDataUpdated = 1;
      } else if (frame_type == RX_SID_BAD) {
         dtxState->vSinceLastSid = 0;
	     d_dtxHangoverAdded = 0;
      }
   }

   dtxState->vDTXHangAdd = d_dtxHangoverAdded;
   
   return newState;
   /* newState is used by both SPEECH AND DTX synthesis routines */
}

/*************************************************************************
 *   Function: ownGenNoise_GSMAMR
 *************************************************************************/
Ipp16s ownGenNoise_GSMAMR (Ipp32s *pShiftReg, Ipp16s numBits)
{
   Ipp16s noise_bits, State, i;

   noise_bits = 0;
   for (i = 0; i < numBits; i++)  {
      /* State n == 31 */
      if ((*pShiftReg & 0x00000001) != 0) State = 1;
      else                                State = 0;
      /* State n == 3 */
      if ((*pShiftReg & 0x10000000) != 0) State = (Ipp16s)(State ^ 1);
      else                                State = (Ipp16s)(State ^ 0);

      noise_bits <<= 1;
      noise_bits = (Ipp16s)(noise_bits | ((Ipp16s)*pShiftReg & 1));
      *pShiftReg = *pShiftReg >> 1;

      if(State & 1) *pShiftReg = *pShiftReg | 0x40000000;
   }

   return noise_bits;
}

/***************************************************************************
*  Function : ownBuildCNCode_GSMAMR
***************************************************************************/
void ownBuildCNCode_GSMAMR (Ipp32s *seed, Ipp16s *pCNVec)
{
   Ipp16s i, j, k;
   ippsZero_16s(pCNVec, SUBFR_SIZE_GSMAMR);

   for (k = 0; k < NB_PULSE; k++) {
      i = ownGenNoise_GSMAMR(seed, 2);      /* generate pulse position */
      i = (Ipp16s)(i * 10 + k);
      j = ownGenNoise_GSMAMR(seed, 1);      /* generate sign           */
      if (j > 0) pCNVec[i] = 4096;
      else       pCNVec[i] = -4096;
   }
   return;
}

/*************************************************************************
 *   Function: ownBuildCNParam_GSMAMR
 *************************************************************************/
void ownBuildCNParam_GSMAMR(Ipp16s *seed, const Ipp16s numParam, const Ipp16s *pTableSizeParam,
                            Ipp16s *pCNParam)
{
   Ipp16s i;
   const Ipp16s *ptr_wnd;

   *seed = (Ipp16s)(*seed * 31821 + 13849L);
   ptr_wnd = &TableHammingWindow[*seed & 0x7F];
   for(i=0; i< numParam;i++){
     pCNParam[i] = (Ipp16s)(*ptr_wnd++ & ~(0xFFFF<<pTableSizeParam[i]));
   }
}

/*************************************************************************
 *  Function:   ownDecLSPQuantDTX_GSMAMR()
 *************************************************************************/
void ownDecLSPQuantDTX_GSMAMR(Ipp16s *a_PastQntPredErr, Ipp16s *a_PastLSFQnt, Ipp16s BadFrInd,
                              Ipp16s *indice, Ipp16s *lsp1_q)
{
    Ipp16s i, index;
    Ipp16s *p_dico, temp;
    IPP_ALIGNED_ARRAY(16, Ipp16s, lsf1_r, LP_ORDER_SIZE);
    IPP_ALIGNED_ARRAY(16, Ipp16s, lsf1_q, LP_ORDER_SIZE);

    if (BadFrInd != 0) {  /* if bad frame */
        for(i = 0; i < LP_ORDER_SIZE; i++)
          lsf1_q[i] = (Ipp16s)(((a_PastLSFQnt[i] * ALPHA_09) >> 15) + ((TableMeanLSF_3[i] * ONE_ALPHA) >> 15));

        for (i = 0; i < LP_ORDER_SIZE; i++) {
          temp = (Ipp16s)(TableMeanLSF_3[i] + a_PastQntPredErr[i]);
          a_PastQntPredErr[i] = (Ipp16s)(lsf1_q[i] - temp);
        }
    } else {
       /* MR59, MR67, MR74, MR102, MRDTX */
        index = *indice++;
        p_dico = &TableDecCode1LSF_3[3*index];
        lsf1_r[0] = *p_dico++;
        lsf1_r[1] = *p_dico++;
        lsf1_r[2] = *p_dico;

        index = *indice++;
        p_dico = &TableDecCode2LSF_3[3*index];
        lsf1_r[3] = *p_dico++;
        lsf1_r[4] = *p_dico++;
        lsf1_r[5] = *p_dico;

        index = *indice;
        p_dico = &TableDecCode3LSF_3[index << 2];
        lsf1_r[6] = *p_dico++;
        lsf1_r[7] = *p_dico++;
        lsf1_r[8] = *p_dico++;
        lsf1_r[9] = *p_dico;

        for (i = 0; i < LP_ORDER_SIZE; i++) {
           temp = (Ipp16s)(TableMeanLSF_3[i] + a_PastQntPredErr[i]);
           lsf1_q[i] = (Ipp16s)(lsf1_r[i] + temp);
           a_PastQntPredErr[i] = lsf1_r[i];
        }
    }
    ownReorderLSFVec_GSMAMR(lsf1_q, LSF_GAP, LP_ORDER_SIZE);
    ippsCopy_16s(lsf1_q, a_PastLSFQnt, LP_ORDER_SIZE);
    ippsLSFToLSP_GSMAMR_16s(lsf1_q, lsp1_q);

    return;
}

static __ALIGN32 CONST Ipp16s lsf_hist_mean_scale[LP_ORDER_SIZE] = {
   20000, 20000, 20000, 20000, 20000, 18000, 16384, 8192,  0,  0
};
static __ALIGN32 CONST Ipp16s dtx_log_en_adjust[9] =
{

/* MR475  MR515   MR59   MR67   MR74  MR795  MR102  MR122  MRDTX */
  -1023,   -878,  -732,  -586,  -440,  -294,  -148,     0,     0
};

/***************************************************************************
*  Function    : ownDTXDecoder_GSMAMR
***************************************************************************/
Ipp32s ownDTXDecoder_GSMAMR(sDTXDecoderSt *dtxState, Ipp16s *a_MemorySyn, Ipp16s *a_PastQntPredErr,
                         Ipp16s *a_PastLSFQnt, Ipp16s *a_PastQntEnergy, Ipp16s *a_PastQntEnergy_M122,
                         Ipp16s *vHgAverageVar, Ipp16s *vHgAverageCount, enum enDTXStateType newState,
                         GSMAMR_Rate_t rate, Ipp16s *pParmVec, Ipp16s *pSynthSpeechVec, Ipp16s *pA_LP)
{
   Ipp16s logEnergyIdx;
   Ipp16s i, j;
   Ipp16s int_fac;
   Ipp32s logEnergy_long;
   Ipp16s logEnergyE, logEnergyM;
   Ipp16s level;
   Ipp16s predErr, predInit_ma;
   Ipp16s logPredE, logPredM, logPred;
   Ipp32s   negative;
   Ipp16s lsfMean;
   Ipp32s lsfMean_long;
   Ipp16s lsfIndex, lsfFactor;
   Ipp16s ptr;
   Ipp16s tmpLength;
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLspBuf, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pCoef_A, LP_ORDER_SIZE + 1);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pCoef_Refl, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pExt, SUBFR_SIZE_GSMAMR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLsfBuf, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLsfVarBuf, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLspVarBuf, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pCoeffABuf, LP_ORDER_SIZE + 1);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pLsf, LP_ORDER_SIZE);
   IPP_ALIGNED_ARRAY(16, Ipp32s, pLsf_long, LP_ORDER_SIZE);


   if ((dtxState->vDTXHangAdd != 0) && (dtxState->vFlagSidFrame != 0)) {

      dtxState->vLogEnergyCorrect = dtx_log_en_adjust[rate];
      ptr = (Ipp16s)(dtxState->vLogEnergyHistory + 1);
      if (ptr == DTX_HIST_SIZE)
         ptr = 0;

      ippsCopy_16s(&dtxState->a_LSFHistory[dtxState->vLogEnergyHistory*LP_ORDER_SIZE],
                   &dtxState->a_LSFHistory[ptr*LP_ORDER_SIZE], LP_ORDER_SIZE);
      dtxState->a_LogEnergyHistory[ptr] = dtxState->a_LogEnergyHistory[dtxState->vLogEnergyHistory];
      dtxState->vLogEnergy = 0;
      ippsSet_32s(0, pLsf_long, LP_ORDER_SIZE);
      /* average energy and lsp */
      for (i = 0; i < DTX_HIST_SIZE; i++) {
         dtxState->vLogEnergy = (Ipp16s)(dtxState->vLogEnergy + (dtxState->a_LogEnergyHistory[i] >> 3));
         for (j = 0; j < LP_ORDER_SIZE; j++)
            pLsf_long[j] += dtxState->a_LSFHistory[i * LP_ORDER_SIZE + j];
      }
      for (j = 0; j < LP_ORDER_SIZE; j++)
         pLsf[j] = (Ipp16s)(pLsf_long[j] >> 3); /* divide by 8 */

      ippsLSFToLSP_GSMAMR_16s(pLsf, dtxState->a_LSP);
      dtxState->vLogEnergy = (Ipp16s)(dtxState->vLogEnergy - dtxState->vLogEnergyCorrect);
      ippsCopy_16s(dtxState->a_LSFHistory, dtxState->a_LSFHistoryMean, 80);
      for (i = 0; i < LP_ORDER_SIZE; i++) {
         lsfMean_long = 0;
         /* compute mean pLsf */
         for (j = 0; j < 8; j++)
            lsfMean_long += dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE];

         lsfMean = (Ipp16s)(lsfMean_long >> 3);

         for (j = 0; j < 8; j++) {
            dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = (Ipp16s)(dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] - lsfMean);
            dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = (Ipp16s)((dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] *
                lsf_hist_mean_scale[i]) >> 15);
            if(dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] < 0) negative = 1;
            else negative = 0;

            dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = Abs_16s(dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE]);

            if (dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] > 655)
               dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = (Ipp16s)(655 + ((dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] - 655) >> 2));

            if (dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] > 1310)
               dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = 1310;

            if (negative != 0)
               dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE] = (Ipp16s)(-dtxState->a_LSFHistoryMean[i+j*LP_ORDER_SIZE]);

         }
      }
   }
   if (dtxState->vFlagSidFrame != 0 ) {
      ippsCopy_16s(dtxState->a_LSP, dtxState->a_LSP_Old, LP_ORDER_SIZE);
      dtxState->vLogEnergyOld = dtxState->vLogEnergy;
      if (dtxState->vFlagValidData != 0 )
      {
         /* Compute interpolation factor  */
         tmpLength = dtxState->vLastSidFrame;
         dtxState->vLastSidFrame = 0;
         if (tmpLength > 32) tmpLength = 32;

         if (tmpLength >= 2)
            dtxState->vSidPeriodInv = (Ipp16s)((1 << 25)/Cnvrt_32s16s(tmpLength << 10));
         else
            dtxState->vSidPeriodInv = 1 << 14;

         ippsCopy_16s(&TablePastLSFQnt[pParmVec[0] * LP_ORDER_SIZE], a_PastQntPredErr, LP_ORDER_SIZE);
         ownDecLSPQuantDTX_GSMAMR(a_PastQntPredErr, a_PastLSFQnt, 0, &pParmVec[1], dtxState->a_LSP);
         ippsZero_16s(a_PastQntPredErr, LP_ORDER_SIZE);   /* reset for next speech frame */
         logEnergyIdx = pParmVec[4];
         dtxState->vLogEnergy = Cnvrt_32s16s(logEnergyIdx << (11 - 2));
         dtxState->vLogEnergy -= (2560 * 2);
         if (logEnergyIdx == 0) dtxState->vLogEnergy = IPP_MIN_16S;
         if ((dtxState->vFlagDataUpdate == 0) || (dtxState->eDTXPrevState == SPEECH)) {
            ippsCopy_16s(dtxState->a_LSP, dtxState->a_LSP_Old, LP_ORDER_SIZE);
            dtxState->vLogEnergyOld = dtxState->vLogEnergy;
         }
      }
      predInit_ma = (Ipp16s)((dtxState->vLogEnergy >> 1) - 9000);
      if (predInit_ma > 0) predInit_ma = 0;
      if (predInit_ma < -14436) predInit_ma = -14436;

      a_PastQntEnergy[0] = predInit_ma;
      a_PastQntEnergy[1] = predInit_ma;
      a_PastQntEnergy[2] = predInit_ma;
      a_PastQntEnergy[3] = predInit_ma;
      predInit_ma = (Ipp16s)((5443 * predInit_ma) >> 15);

      a_PastQntEnergy_M122[0] = predInit_ma;
      a_PastQntEnergy_M122[1] = predInit_ma;
      a_PastQntEnergy_M122[2] = predInit_ma;
      a_PastQntEnergy_M122[3] = predInit_ma;
   }
   /* CNG */
   dtxState->vLogEnergyCorrect = (Ipp16s)((Ipp16s)((dtxState->vLogEnergyCorrect * 29491) >> 15) +
           ((Ipp16s)((Cnvrt_32s16s(dtx_log_en_adjust[rate] << 5) * 3277)>>15) >> 5));
   /* Interpolate SID info */
   int_fac = Cnvrt_32s16s((1 + dtxState->vLastSidFrame) << 10);
   int_fac = (Ipp16s)((int_fac * dtxState->vSidPeriodInv) >> 15);
   if (int_fac > 1024) int_fac = 1024;

   int_fac <<= 4;
   logEnergy_long = 2 * int_fac * dtxState->vLogEnergy;

   for(i = 0; i < LP_ORDER_SIZE; i++)
      pLspBuf[i] = (Ipp16s)((int_fac * dtxState->a_LSP[i]) >> 15);

   int_fac = (Ipp16s)(16384 - int_fac);
   logEnergy_long += (int_fac * dtxState->vLogEnergyOld) << 1;

   for(i = 0; i < LP_ORDER_SIZE; i++) {
      pLspBuf[i] = (Ipp16s)(pLspBuf[i] + (Ipp16s)((int_fac * dtxState->a_LSP_Old[i]) >> 15));
      pLspBuf[i] = Cnvrt_32s16s(pLspBuf[i] << 1); /* Q14 -> Q15 */
   }
   lsfFactor = (Ipp16s)(dtxState->vLogMean - 2457);
   lsfFactor = (Ipp16s)(4096 - (Ipp16s)((lsfFactor * 9830) >> 15));
   /* limit to values between 0..1 in Q12 */
   if (lsfFactor > 4096) lsfFactor = 4096;
   if (lsfFactor < 0)    lsfFactor = 0;

   lsfFactor = Cnvrt_32s16s(lsfFactor << 3); /* -> Q15 */
   lsfIndex = ownGenNoise_GSMAMR(&dtxState->vPerfSeedDTX_long, 3);
   ippsLSPToLSF_Norm_G729_16s(pLspBuf,pLsfBuf);
   ippsCopy_16s(pLsfBuf, pLsfVarBuf, LP_ORDER_SIZE);

   for(i = 0; i < LP_ORDER_SIZE; i++)
      pLsfVarBuf[i] = (Ipp16s)(pLsfVarBuf[i] + (Ipp16s)((lsfFactor * dtxState->a_LSFHistoryMean[i+lsfIndex*LP_ORDER_SIZE]) >> 15));

   ownReorderLSFVec_GSMAMR(pLsfBuf, LSF_GAP, LP_ORDER_SIZE);
   ownReorderLSFVec_GSMAMR(pLsfVarBuf, LSF_GAP, LP_ORDER_SIZE);
   /* copy pLsf to speech decoders pLsf state */
   ippsCopy_16s(pLsfBuf, a_PastLSFQnt, LP_ORDER_SIZE);

   ippsLSFToLSP_GSMAMR_16s(pLsfBuf, pLspBuf);
   ippsLSFToLSP_GSMAMR_16s(pLsfVarBuf, pLspVarBuf);
   ippsLSPToLPC_GSMAMR_16s(pLspBuf, pCoef_A);
   ippsLSPToLPC_GSMAMR_16s(pLspVarBuf, pCoeffABuf);

   ippsCopy_16s(pCoef_A, &pA_LP[0],           LP_ORDER_SIZE + 1);
   ippsCopy_16s(pCoef_A, &pA_LP[LP_ORDER_SIZE + 1],       LP_ORDER_SIZE + 1);
   ippsCopy_16s(pCoef_A, &pA_LP[2 * (LP_ORDER_SIZE + 1)], LP_ORDER_SIZE + 1);
   ippsCopy_16s(pCoef_A, &pA_LP[3 * (LP_ORDER_SIZE + 1)], LP_ORDER_SIZE + 1);

   ownConvertDirectCoeffToReflCoeff_GSMAMR(&pCoef_A[1], pCoef_Refl);
   predErr = IPP_MAX_16S;

   for (i = 0; i < LP_ORDER_SIZE; i++)
      predErr = (Ipp16s)((predErr * (IPP_MAX_16S - (Ipp16s)((pCoef_Refl[i] * pCoef_Refl[i]) >> 15))) >> 15);

   ownLog2_GSMAMR(predErr, &logPredE, &logPredM);
   
   logPred = Cnvrt_32s16s((logPredE -15) << 12);  /* Q12 */
   logPred = Sub_16s(0,(logPred + (logPredM >> 3)))>>1;
 
   dtxState->vLogMean = (Ipp16s)((Ipp16s)((29491 * dtxState->vLogMean) >> 15) + (Ipp16s)((3277 * logPred) >> 15));

   logEnergy_long >>= 10;
   logEnergy_long += 4 * 65536L;
   logEnergy_long -= ShiftL_32s(logPred, 4);

   logEnergy_long += ShiftL_32s(dtxState->vLogEnergyCorrect, 5);
   logEnergyE = (Ipp16s)(logEnergy_long >> 16);
   logEnergyM = (Ipp16s)((logEnergy_long - (logEnergyE << 16)) >> 1);
   level = (Ipp16s)(ownPow2_GSMAMR(logEnergyE, logEnergyM)); /* Q4 */

   for (i = 0; i < 4; i++) {
      ownBuildCNCode_GSMAMR(&dtxState->vPerfSeedDTX_long, pExt);
      for (j = 0; j < SUBFR_SIZE_GSMAMR; j++)
         pExt[j] = (Ipp16s)((level * pExt[j]) >> 15);

      ippsSynthesisFilter_NR_16s_Sfs(pCoeffABuf, pExt, &pSynthSpeechVec[i * SUBFR_SIZE_GSMAMR], SUBFR_SIZE_GSMAMR, 12, a_MemorySyn);
      ippsCopy_16s(&pSynthSpeechVec[i * SUBFR_SIZE_GSMAMR + SUBFR_SIZE_GSMAMR - LP_ORDER_SIZE], a_MemorySyn, LP_ORDER_SIZE);
   }
   /* reset codebook averaging variables */
   *vHgAverageVar = 20;
   *vHgAverageCount = 0;
   if (newState == DTX_MUTE) {

      tmpLength = dtxState->vLastSidFrame;
      if (tmpLength > 32)     tmpLength = 32;
      else if(tmpLength <= 0) tmpLength = 8; /* avoid division by zero */

      dtxState->vSidPeriodInv = (Ipp16s)((1 << 20)/Cnvrt_32s16s(tmpLength << 10)); /* div: den # 0*/
      dtxState->vLastSidFrame = 0;
      ippsCopy_16s(dtxState->a_LSP, dtxState->a_LSP_Old, LP_ORDER_SIZE);
      dtxState->vLogEnergyOld = dtxState->vLogEnergy;
      dtxState->vLogEnergy -= 256;
   }
   /* reset interpolation length timer if data has been updated. */
   if ((dtxState->vFlagSidFrame != 0) && ((dtxState->vFlagValidData != 0) ||
      ((dtxState->vFlagValidData == 0) &&  (dtxState->vDTXHangAdd) != 0)))
   {
      dtxState->vLastSidFrame =  0;
      dtxState->vFlagDataUpdate = 1;
   }
   return 0;
}

/***************************************************************************
*  Function    : ownRX_DTX_Handler_GSMAMR
***************************************************************************/
enum enDTXStateType ownRX_DTX_Handler_GSMAMR(GSMAMRDecoder_Obj* decoderObj, RXFrameType frame_type)
{
   sDTXDecoderSt *dtxState = &decoderObj->stDecState.dtxDecoderState;
   enum enDTXStateType newDTXState = ownDecSidSync(&decoderObj->stDecState.dtxDecoderState, frame_type);

   if (newDTXState != SPEECH) {
      dtxState->vSidUpdateCt--;
      if (dtxState->eDTXPrevState == SPEECH) dtxState->vSidUpdateCt = 3;
      dtxState->vLastSidFrame++;
   } else dtxState->vLastSidFrame = 0;

   
   if (newDTXState != SPEECH) {
      dtxState->vFlagSidFrame = 0;
      dtxState->vFlagValidData = 0;
      if (newDTXState == DTX) dtxState->vFlagSidFrame = 1;
      if (dtxState->vSidUpdateCt == 0 && newDTXState != DTX_NODATA) {
         dtxState->vSidUpdateCt = 8;
         dtxState->vFlagSidFrame = 1;
         dtxState->vFlagValidData = 1;
      }

	  if (frame_type == RX_SID_FIRST)
      {
           dtxState->vFlagSidFrame = 1;                                        
      }
      else if (frame_type == RX_SID_UPDATE)
      {
         dtxState->vFlagSidFrame = 1;                                        
         dtxState->vFlagValidData = 1;                                       
      }
      else if (frame_type == RX_SID_BAD)
      {
         dtxState->vFlagSidFrame = 1;                                         
         dtxState->vDTXHangAdd = 0; /* use old data */              
      } 

	  if (frame_type == RX_NO_DATA)
		dtxState->vFlagSidFrame = 0; 
   }

   return newDTXState;
}
