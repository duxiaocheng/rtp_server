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
// Purpose: AMRWBE speech codec initialization API functions.
//
*/

#include "ownamrwbe.h"
#include "ownamrwb.h"

static void ownResetCoderAMRWBE(EncoderObj_AMRWBE *st, Ipp16s num_chan, Ipp16s fscale,
   Ipp32s use_case_mode, Ipp16s full_reset);
static void Init_coder_hf(EncoderOneChanelState_AMRWBE *st);
static void Init_coder_lf(EncoderObj_AMRWBE *st);
static void Cos_windowLP(Ipp16s *fh, Ipp32s len);

static void Reset_decoder_amrwb_plus(DecoderObj_AMRWBE *st, Ipp32s num_chan, Ipp32s full_reset);
static void Init_decoder_hf(DecoderOneChanelState_AMRWBE *st);
static void Init_decoder_lf(DecoderObj_AMRWBE *st);
static void Init_decoder_stereo_x(DecoderObj_AMRWBE *st);
void Init_dec_hi_stereo(DecoderObj_AMRWBE *st);

AMRWB_CODECFUN( APIAMRWB_Status, apiEncoderInit_AMRWBE,
         (EncoderObj_AMRWBE* pEncoderObj, Ipp32s isf, Ipp32s mode, Ipp32s fullReset))
{
   Ipp32s objSize;
   Ipp8u *ptr;

   if (fullReset) {
      /* arrange objects */
      objSize = sizeof (EncoderObj_AMRWBE);
      ptr = ((Ipp8u*)pEncoderObj) + objSize;
      ippsHighPassFilterGetSize_AMRWB_16s(2, &objSize); /* size for left/right memSigIn */
      pEncoderObj->right.memSigIn = (IppsHighPassFilterState_AMRWB_16s*)ptr;
      ptr += objSize;
      pEncoderObj->left.memSigIn = (IppsHighPassFilterState_AMRWB_16s*)ptr;
      ptr += objSize;
      ippsHighPassFilterGetSize_AMRWB_16s(3, &objSize);
      pEncoderObj->memHPOLLTP = (IppsHighPassFilterState_AMRWB_16s*)ptr;
      ptr += objSize;
      ippsVADGetSize_AMRWB_16s(&objSize);
      pEncoderObj->pVADState = (IppsVADState_AMRWB_16s*)ptr;
      ptr += objSize;
      pEncoderObj->_stClass = (NCLASSDATA_FX*)ptr;
   }
   /* init codec memory */
   ownResetCoderAMRWBE(pEncoderObj, 2/*max*/, (Ipp16s)isf, (Ipp16s)mode, (Ipp16s)fullReset);
   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiDecoderInit_AMRWBE,
         (DecoderObj_AMRWBE* pDecoderObj, Ipp32s isf, Ipp32s nChannels, Ipp32s fullReset))
{
   Ipp32s objSize;
   Ipp8u *ptr;

   if (fullReset) {
      /* arrange objects */
      objSize = sizeof (DecoderObj_AMRWBE);
      ptr = ((Ipp8u*)pDecoderObj) + objSize;
      ippsHighPassFilterGetSize_AMRWB_16s(2, &objSize); /* size for left/right/common memSigOut */
      pDecoderObj->right.wmemSigOut = (IppsHighPassFilterState_AMRWB_16s*)ptr;
      ptr += objSize;
      pDecoderObj->left.wmemSigOut = (IppsHighPassFilterState_AMRWB_16s*)ptr;
      ptr += objSize;
      pDecoderObj->wmemSigOut = (IppsHighPassFilterState_AMRWB_16s*)ptr;
   }

   Reset_decoder_amrwb_plus(pDecoderObj, nChannels, fullReset);

   return APIAMRWB_StsNoErr;
}

void ownResetCoderAMRWBE(EncoderObj_AMRWBE *st, Ipp16s num_chan, Ipp16s fscale, Ipp32s use_case_mode, Ipp16s full_reset)
{
   if ((full_reset >0) && (use_case_mode==1)) {
      st->vad_hist = 0;
      ippsVADInit_AMRWB_16s(st->pVADState);
      st->tone_flag = 0;
      InitClassifyExcitation(st->_stClass);
   }

   st->right.decim_frac = 0;
   st->left.decim_frac = 0;

   if (full_reset > 0) {
      ippsZero_16s(st->lev_mem, 18);

      ippsZero_16s(st->right.mem_decim_hf, 2*L_FILT24k);
      ippsZero_16s(st->right.old_speech_hf, L_OLD_SPEECH_ST);
      ippsZero_16s(st->right.mem_decim, L_MEM_DECIM_SPLIT);
      st->right.mem_preemph = 0;
      Init_coder_hf(&(st->right));

      ippsZero_16s(st->left.mem_decim, L_MEM_DECIM_SPLIT);
      ippsZero_16s(st->left.mem_decim_hf, 2*L_FILT24k);
      ippsZero_16s(st->left.old_speech_hf, L_OLD_SPEECH_ST);
      st->left.mem_preemph = 0;
      Init_coder_hf(&(st->left));

      ippsZero_16s(st->old_speech, L_OLD_SPEECH_ST);
      ippsZero_16s(st->old_synth, M);

      if (fscale >= FSCALE_DENOM) {
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50_DENORM, (Ipp16s*)tblBCoeffHP50_DENORM,
            2, st->right.memSigIn);
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50_DENORM, (Ipp16s*)tblBCoeffHP50_DENORM,
            2, st->left.memSigIn);
      } else {
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50,
            2, st->right.memSigIn);
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50,
            2, st->left.memSigIn);
      }

      ippsZero_16s(st->old_chan, L_OLD_SPEECH_ST);
      ippsZero_16s(st->old_chan_2k, L_OLD_SPEECH_2k);
      ippsZero_16s(st->old_speech_2k, L_OLD_SPEECH_2k);
      ippsZero_16s(st->old_chan_hi, L_OLD_SPEECH_hi);
      ippsZero_16s(st->old_speech_hi, L_OLD_SPEECH_hi);

      Init_coder_lf(st);

      if(num_chan == 2) {
         ippsZero_16s(st->old_exc_mono,HI_FILT_ORDER);
         ippsZero_16s(st->old_wh,HI_FILT_ORDER);
         ippsZero_16s(st->old_wh_q,HI_FILT_ORDER);
         ippsZero_16s(st->old_gm_gain,2);
         st->filt_energy_threshold = 0;
         st->mem_stereo_ovlp_size = 0;
         ippsZero_16s(st->mem_stereo_ovlp,L_OVLP_2k);
      }

      ippsZero_16s(st->mem_gain_code, 4);
      st->SwitchFlagPlusToWB = 0;
      st->prev_mod = 0;

      st->Q_exc = 0;
      st->Old_Qexc = 0;

      st->Q_new = 0;
      st->OldQ_sp_deci[0] = 0;
      st->OldQ_sp_deci[1] = 0;
      st->Q_max[0] = 0;
      st->Q_max[1] = 0;

      st->Q_sp = 0;
      st->OldQ_sp = 0;
      st->scale_fac = 0;

      st->right.Q_sp_hf = 0;
      st->right.OldQ_sp_hf[0] = 0;
      st->right.OldQ_sp_hf[1] = 0;
      st->left.Q_sp_hf = 0;
      st->left.OldQ_sp_hf[0] = 0;
      st->left.OldQ_sp_hf[1] = 0;

      st->LastQMode = 0;

   } else {
      Ipp16s memHPRight[6], memHPLeft[6];
      ippsHighPassFilterGetDlyLine_AMRWB_16s(st->right.memSigIn, memHPRight, 2);
      ippsHighPassFilterGetDlyLine_AMRWB_16s(st->left.memSigIn, memHPLeft, 2);
      if (fscale >= FSCALE_DENOM) {
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50_DENORM, (Ipp16s*)tblBCoeffHP50_DENORM,
            2, st->right.memSigIn);
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50_DENORM, (Ipp16s*)tblBCoeffHP50_DENORM,
            2, st->left.memSigIn);
      } else {
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50, 2, st->right.memSigIn);
         ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50, 2, st->left.memSigIn);
      }
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPRight, st->right.memSigIn, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPLeft, st->left.memSigIn, 2);
   }
   if(fscale <= FSCALE_DENOM) {
      Cos_windowLP(st->window, L_WINDOW_WBP/2);
   } else {
      Cos_windowLP(st->window, L_WINDOW_HIGH_RATE/2);
   }
}

static void Init_coder_hf(EncoderOneChanelState_AMRWBE *st)
{
   ippsZero_16s(st->mem_hf1, MHF);
   ippsZero_16s(st->mem_hf2, MHF);
   ippsZero_16s(st->mem_hf3, MHF);
   ippsZero_16s(st->past_q_isf_hf, MHF);
   ippsZero_16s(st->mem_lev_hf, 18);
   st->old_gain = 256;
   ippsCopy_16s(tblISPhf_16s, st->ispold_hf, MHF);
   ippsCopy_16s(st->ispold_hf, st->ispold_q_hf, MHF);
}

static void Init_coder_lf(EncoderObj_AMRWBE *st)
{
   Ipp32s i;
   ippsZero_16s(st->old_exc, PIT_MAX_MAX+L_INTERPOL);
   ippsZero_16s(st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
   ippsZero_16s(st->mem_lp_decim2, 3);
   ippsZero_16s(st->past_isfq, M);
   ippsZero_16s(st->old_wovlp, 128);
   ippsZero_16s(st->hp_old_wsp, L_FRAME_PLUS/OPL_DECIM+(PIT_MAX_MAX/OPL_DECIM));

   for (i=0; i<5; i++)
      st->old_ol_lag[i] = 40;
   st->old_mem_wsyn = 0;
   st->old_mem_w0   = 0;
   st->old_mem_xnq  = 0;
   st->mem_wsp      = 0;

   st->old_T0_med = 0;
   st->ol_wght_flg = 0;
   st->ada_w = 0;

   st->old_wsp_max[0] = Q_MAX2;
   st->old_wsp_max[1] = Q_MAX2;
   st->old_wsp_max[2] = Q_MAX2;
   st->old_wsp_max[3] = Q_MAX2;
   st->old_ovlp_size = 0;
   st->old_wsp_shift = 0;

   ippsCopy_16s(tblISF_16s, st->isfold, M);
   ippsCopy_16s(tblISP_16s, st->ispold, M);
   ippsCopy_16s(st->ispold, st->ispold_q, M);

   ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeff, (Ipp16s*)tblBCoeff, 3, st->memHPOLLTP);
   st->scaleHPOLLTP = 0;
}

static void Cos_windowLP(Ipp16s *pDst, Ipp32s len)
{
   Ipp32s i, j, dwTmp;
   Ipp16s wTmp;

   if(len == (L_WINDOW_WBP/2)) {
      for(i=0; i<len; i++) {
         pDst[i] = tblCosLF_16s[i];
      }
      for(i=0; i<len; i++) {
         pDst[len+i] = tblCosLF_16s[(len-1)-i];
      }
   } else {
      pDst[0] = pDst[2*len-1] = ShiftR_NR_16s(tblCosHF_16s[0],1);
      for(i=1,j=0; i<(len-1); i+=2,j++) {
         dwTmp = (tblCosHF_16s[j+1] - tblCosHF_16s[j])*8192*2;
         wTmp = Add_16s(Cnvrt_NR_32s16s(dwTmp), tblCosHF_16s[j]);
         pDst[i] = pDst[(2*len-1)-i] = wTmp;

         dwTmp = (tblCosHF_16s[j+1] - tblCosHF_16s[j])*24576*2;
         wTmp = Add_16s(Cnvrt_NR_32s16s(dwTmp), tblCosHF_16s[j]);
         pDst[i+1] = pDst[(2*len-1)-i-1] = wTmp;
      }
      pDst[len-1] = pDst[len] = tblCosHF_16s[(len>>1)-1];
   }
}

AMRWB_CODECFUN( APIAMRWB_Status, apiCopyEncoderState_AMRWBE,
         (EncoderObj_AMRWBE* pObjWBE, AMRWBEncoder_Obj* pObjWB, Ipp32s vad, Ipp32s direction))
{
   Ipp32s i;
   Ipp16s memHPFilter[9];

   if (direction == 0) { /* Copy data from WB to WB+ structure */
      /* Full reset for WB+ encoder*/
      ownResetCoderAMRWBE(pObjWBE, 1, 0, vad, 1);
      for (i = 0; i < M; i++) {
         pObjWBE->isfold[i] = Cnvrt_NR_32s16s(Mul2_Low_32s(ShiftL_16s(pObjWB->asiIsfOld[i],2) * 20972));
         pObjWBE->past_isfq[i] = Cnvrt_NR_32s16s(Mul2_Low_32s(ShiftL_16s(pObjWB->asiIsfQuantPast[i],2) * 20972));
      }

      ippsCopy_16s(pObjWB->asiIspOld, pObjWBE->ispold, M);
      ippsCopy_16s(pObjWB->asiIspQuantOld,pObjWBE->ispold_q, M);

      for (i=0; i<5; i++) {
         pObjWBE->old_ol_lag[i] = pObjWB->asiPitchLagOld[i];
      }
      pObjWBE->old_T0_med = pObjWB->siMedianOld;

      ippsCopy_16s(pObjWB->asiSpeechDecimate, pObjWBE->right.mem_decim, 30);

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWB->pSHPFiltStateSgnlIn, memHPFilter, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWBE->right.memSigIn, 2);

      ippsCopy_16s(pObjWB->asiSpeechOld + 48, pObjWBE->old_speech_pe, 16 + 512);
      ownScaleSignal_AMRWB_16s_ISfs(pObjWBE->old_speech_pe, 16 + 512, (Ipp16s)(-pObjWB->siScaleFactorOld));
      ippsCopy_16s(pObjWB->asiWspOld, pObjWBE->old_d_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM), PIT_MAX/OPL_DECIM);
      ippsCopy_16s(pObjWB->asiHypassFiltWspOld, pObjWBE->hp_old_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM), PIT_MAX/OPL_DECIM);
      ownScaleSignal_AMRWB_16s_ISfs(pObjWBE->old_d_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM),
         PIT_MAX/OPL_DECIM, (Ipp16s)(-pObjWB->siScaleFactorOld));
      ownScaleSignal_AMRWB_16s_ISfs(pObjWBE->hp_old_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM),
         PIT_MAX/OPL_DECIM, (Ipp16s)(-pObjWB->siScaleFactorOld));

      if (pObjWB->siScaleFactorOld >= 0) {
         pObjWBE->mem_wsp = (Ipp16s)(pObjWB->siWsp >> pObjWB->siScaleFactorOld);
      } else {
         pObjWBE->mem_wsp = ShiftL_16s(pObjWB->siWsp, (Ipp16s)(-pObjWB->siScaleFactorOld));
      }
      ippsCopy_16s(pObjWB->asiWspDecimate, pObjWBE->mem_lp_decim2, 3);
      pObjWBE->ol_gain = pObjWB->siOpenLoopGain;

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWB->pSHPFiltStateWsp, memHPFilter, 3);
      for (i=0; i<7; i++)
         memHPFilter[i] = memHPFilter[i+2];
      memHPFilter[7] = 0;
      memHPFilter[8] = 0;
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWBE->memHPOLLTP, 3);
      pObjWBE->scaleHPOLLTP=0;

      pObjWBE->ada_w = pObjWB->siAdaptiveParam;
      pObjWBE->ol_wght_flg = pObjWB->siWeightFlag;
      pObjWBE->old_mem_w0 = pObjWB->siSpeechWgt;

      ippsCopy_16s(pObjWB->asiSynthesis, pObjWBE->old_synth, M);
      ownScaleSignal_AMRWB_16s_ISfs(pObjWBE->old_synth,M, (Ipp16s)(-pObjWB->siScaleFactorOld));
      if (pObjWB->siScaleFactorOld >= 0) {
         pObjWBE->right.mem_preemph = (Ipp16s)(pObjWB->siPreemph >> pObjWB->siScaleFactorOld);
      } else {
         pObjWBE->right.mem_preemph = ShiftL_16s(pObjWB->siPreemph, (Ipp16s)(-pObjWB->siScaleFactorOld));
      }

      ippsCopy_16s(pObjWB->asiExcOld, pObjWBE->old_exc + PIT_MAX_MAX-PIT_MAX ,PIT_MAX + L_INTERPOL);
      ownScaleSignal_AMRWB_16s_ISfs(pObjWBE->old_exc + PIT_MAX_MAX-PIT_MAX,
         (PIT_MAX + L_INTERPOL), (Ipp16s)(-pObjWB->siScaleFactorOld));

      pObjWBE->old_ovlp_size = 0;

      pObjWBE->SwitchFlagPlusToWB = 1;

      if (vad == USE_CASE_B) {
         Ipp32s objSize;
         ippsVADGetSize_AMRWB_16s(&objSize);
         /* Note: iPowFramePrev{==prev_pow_sum}is coped, but in refcode sets to 0 while VAD reset */
         ippsCopy_8u((Ipp8u*)pObjWB->pSVadState, (Ipp8u*)pObjWBE->pVADState, objSize);

         pObjWBE->tone_flag = pObjWB->siToneFlag;
         if (pObjWBE->_stClass->StatClassCount == 0)
            pObjWBE->_stClass->StatClassCount = 15;
      }

      for (i = 0; i < 2; i++) {
         pObjWBE->Q_max[i] = pObjWB->asiScaleFactorMax[i];
         pObjWBE->OldQ_sp_deci[i] = 0;
      }

      pObjWBE->Q_new = 0;
      pObjWBE->Q_exc = 0;
      pObjWBE->Old_Qexc = 0;
      pObjWBE->LastQMode = 0;
      pObjWBE->OldQ_sp = 0;

   } else if (direction == 1) { /* copy data from WB+ to WB */
      Ipp32s Ltmp;
      apiAMRWBEncoder_Init(pObjWB, vad);

      for (i=0; i<M; i++) {
         pObjWB->asiIsfOld[i] = (Ipp16s)((pObjWBE->isfold[i]*12800)>>15);
         pObjWB->asiIsfQuantPast[i] = (Ipp16s)((pObjWBE->past_isfq[i]*12800)>>15);
      }
      ippsCopy_16s(pObjWBE->ispold, pObjWB->asiIspOld, M);
      ippsCopy_16s(pObjWBE->ispold_q, pObjWB->asiIspQuantOld, M);
      ippsCopy_16s(pObjWBE->old_ol_lag, pObjWB->asiPitchLagOld, 5);

      pObjWB->siMedianOld = pObjWBE->old_T0_med;

      ippsCopy_16s(pObjWBE->right.mem_decim, pObjWB->asiSpeechDecimate, 30);
      ownScaleSignal_AMRWB_16s_ISfs(pObjWB->asiSpeechDecimate, 30, (Ipp16s)(-pObjWBE->Q_new));


      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWBE->right.memSigIn, memHPFilter, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWB->pSHPFiltStateSgnlIn, 2);

      if ((-pObjWBE->Q_new+pObjWBE->Q_sp-1) > 0) {
         pObjWB->siPreemph = ShiftL_16s(pObjWBE->right.mem_preemph, (Ipp16u)(-pObjWBE->Q_new+pObjWBE->Q_sp-1));
      } else {
         pObjWB->siPreemph = (Ipp16s)(pObjWBE->right.mem_preemph >> (pObjWBE->Q_new-pObjWBE->Q_sp+1));
      }

      ippsCopy_16s(pObjWBE->old_speech_pe, pObjWB->asiSpeechOld + 48, 16 + 512);

      for (i = 0; i < 48; i++) {
         pObjWB->asiSpeechOld[i] = 0;
      }

      ippsCopy_16s(pObjWBE->old_d_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM), pObjWB->asiWspOld, PIT_MAX / OPL_DECIM);
      ippsCopy_16s(pObjWBE->hp_old_wsp+(PIT_MAX_MAX/OPL_DECIM)-(PIT_MAX/OPL_DECIM), pObjWB->asiHypassFiltWspOld, PIT_MAX / OPL_DECIM);

      pObjWB->siWsp = pObjWBE->mem_wsp;
      ippsCopy_16s(pObjWBE->mem_lp_decim2, pObjWB->asiWspDecimate, 3);

      pObjWB->siOpenLoopGain = pObjWBE->ol_gain;

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWBE->memHPOLLTP, memHPFilter, 3);
      for (i=8; i>1; i--)
         memHPFilter[i] = memHPFilter[i-2];
      memHPFilter[0] = 0;
      memHPFilter[1] = 0;
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWB->pSHPFiltStateWsp, 3);
      pObjWB->siScaleExp = 0;//=pObjWBE->scaleHPOLLTP;????

      pObjWB->siAdaptiveParam = pObjWBE->ada_w;
      pObjWB->siWeightFlag = pObjWBE->ol_wght_flg;

      pObjWB->siSpeechWgt = pObjWBE->old_mem_w0;

      if (pObjWBE->prev_mod == 0) {
         for (i=0; i<4; i++) {
            pObjWB->asiGainQuant[i] = pObjWBE->mem_gain_code[3-i];
         }
      }

      ippsCopy_16s(pObjWBE->old_synth, pObjWB->asiSynthesis, M);

      for (i=0; i<M; i++) {
         Ipp16s tmp_lo, tmp_hi;
         Ltmp = Mul2_Low_32s(pObjWB->asiSynthesis[i] * 128);
         tmp_hi = (Ipp16s)(Ltmp>>12);
         tmp_lo = (Ipp16s)(Ltmp&0x00000FFF);
         pObjWB->aiSynthMem[i] = (tmp_hi<<16) | (tmp_lo<<4);
      }

      for (i=0; i<(PIT_MAX+L_INTERPOL); i++) {
         pObjWB->asiExcOld[i] = pObjWBE->old_exc[i+PIT_MAX_MAX-PIT_MAX];
      }
      ownScaleSignal_AMRWB_16s_ISfs(pObjWB->asiExcOld, PIT_MAX+L_INTERPOL, (Ipp16s)(-pObjWBE->Old_Qexc+pObjWBE->Q_sp-1));
      pObjWB->siScaleFactorOld = Add_16s(1,pObjWBE->Q_sp);
      for (i=0; i<2; i++) {
         pObjWB->asiScaleFactorMax[i] = pObjWBE->Q_max[i];
      }

      if (vad == USE_CASE_B) {
         Ipp32s objSize;
         ippsVADGetSize_AMRWB_16s(&objSize);
         /* Note: iPowFramePrev{==prev_pow_sum}is coped, but in refcode set to 0 while VAD reset */
         ippsCopy_8u((Ipp8u*)pObjWBE->pVADState, (Ipp8u*)pObjWB->pSVadState, objSize);
         pObjWB->siToneFlag = pObjWBE->tone_flag;
      }
   } else {
        return APIAMRWB_StsBadArgErr;
   }

   return APIAMRWB_StsNoErr;
}

static void Reset_decoder_amrwb_plus(DecoderObj_AMRWBE *st, Ipp32s num_chan, Ipp32s full_reset)
{
   st->left.wover_frac = 0;
   st->right.wover_frac = 0;

   if (full_reset>0) {
      ippsZero_16s(st->wold_synth_pf, PIT_MAX_MAX+(2*L_SUBFR));
      ippsZero_16s(st->wold_noise_pf, 2*L_FILT);
      st->wold_gain_pf[0] = 0;
      st->wold_T_pf[0] = 64;
      st->wold_gain_pf[1] = 0;
      st->wold_T_pf[1] = 64;
      st->Old_bpf_scale = 0;

      ippsZero_16s(st->left.wmem_oversamp, L_MEM_JOIN_OVER);
      ippsZero_16s(st->right.wmem_oversamp, L_MEM_JOIN_OVER);
      ippsZero_16s(st->left.wmem_oversamp_hf, 2*L_FILT);
      ippsZero_16s(st->right.wmem_oversamp_hf, 2*L_FILT);

      ippsZero_16s(st->left.wold_synth_hf, (D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5));
      ippsZero_16s(st->right.wold_synth_hf, (D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5));

      Init_decoder_hf(&(st->left));
      Init_decoder_hf(&(st->right));

      ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50, 2, st->wmemSigOut);
      ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50, 2, st->left.wmemSigOut);
      ippsHighPassFilterInit_AMRWB_16s((Ipp16s*)tblACoeffHP50, (Ipp16s*)tblBCoeffHP50, 2, st->right.wmemSigOut);

      st->last_mode = 0;
      st->wmem_deemph = 0;

      ippsZero_16s(st->h_fx,ECU_WIEN_ORD+1);

      Init_decoder_lf(st);

      if(num_chan == 2) {
         Init_decoder_stereo_x(st);
      }
      ippsZero_16s(st->wold_xri,L_TCX);

      ippsZero_16s(st->wmem_gain_code, 4);
      ippsZero_16s(st->wmem_lpc_hf + 1, MHF);
      st->wmem_lpc_hf[0] = 4096;
      st->wmem_gain_hf = 0;
      st->wramp_state = 64;
      st->L_gc_thres = 0;
   }
}

static void Init_decoder_hf(DecoderOneChanelState_AMRWBE *st)
{
   ippsZero_16s(st->wmem_syn_hf, MHF);
   ippsZero_16s(st->wpast_q_isf_hf, MHF);
   ippsZero_16s(st->wpast_q_isf_hf_other, MHF);
   st->wpast_q_gain_hf = 0;
   st->wpast_q_gain_hf_other = 0;
   st->wold_gain = 256;
   st->Lp_amp = 0;
   st->Lthreshold = 0;
   st->Q_synHF = 6;
   ippsCopy_16s(tblISPhf_16s, st->wispold_hf, MHF);
}

static void Init_decoder_lf(DecoderObj_AMRWBE *st)
{
   Ipp32s i;
   ippsZero_16s(st->wold_exc, PIT_MAX_MAX+L_INTERPOL);
   ippsZero_16s(st->wold_synth, M);
   ippsZero_16s(st->cp_old_synth, M);
   ippsZero_16s(st->wpast_isfq, M);
   ippsZero_16s(st->wwovlp, L_OVLP);
   ippsCopy_16s(tblISF_16s, st->wisfold, M);
   for (i = 0; i < L_MEANBUF; i++) {
      ippsCopy_16s(tblISF_16s, &(st->wisf_buf[i*M]), M);
   }

   ippsCopy_16s(tblISP_16s, st->wispold, M);

   st->mem_subfr_q[6] = Q_MAX;
   st->mem_subfr_q[5] = Q_MAX;
   st->mem_subfr_q[4] = Q_MAX;
   st->mem_subfr_q[3] = Q_MAX;
   st->mem_subfr_q[2] = Q_MAX;
   st->mem_subfr_q[1] = Q_MAX;
   st->mem_subfr_q[0] = Q_MAX;

   ippsZero_16s(st->old_subfr_q, 4*NB_DIV);
   ippsZero_16s(st->old_syn_q, 4*NB_DIV);

   st->prev_lpc_lost = 0;
   st->seed_ace = 0;
   st->seed_tcx = 21845;
   st->pitch_tcx = L_DIV;

   st->wmem_wsyn = 0;
   st->wwsyn_rms = 0;
   st->wpast_gpit = 0;
   st->Lpast_gcode = 0;

   st->Q_exc = 0;
   st->Q_syn = -1;
   st->Old_Q_syn = -1;

   st->prev_Q_syn = 6;
   st->Old_Q_exc = Q_MAX;
   st->Old_Qxri = 6;
   st->Old_Qxnq = Q_MAX;
   st->last_mode = 0;
   st->Old_QxnqMax = Q_MAX;

   st->ovlp_size = 0;
   st->i_offset = 0;

   st->siOldPitchLag = 64;
   st->siOldPitchLagFrac = 0;

   ippsZero_16s(st->mem_syn2,M);
}

static void Init_decoder_stereo_x(DecoderObj_AMRWBE *st)
{
   ippsZero_16s(st->my_old_synth_2k_fx,L_FDEL_2k + D_STEREO_TCX +  2*(D_NC*5)/32);
   ippsZero_16s(st->my_old_synth_hi_fx,2*D_NC);
   ippsZero_16s(st->my_old_synth_fx,2*L_FDEL+20);
   ippsZero_16s(st->mem_left_2k_fx,2*L_FDEL_2k);
   ippsZero_16s(st->mem_right_2k_fx,2*L_FDEL_2k);
   ippsZero_16s(st->mem_left_hi_fx,L_FDEL);
   ippsZero_16s(st->mem_right_hi_fx,L_FDEL);
   ippsZero_16s(st->left.mem_d_tcx_fx,D_NC + (D_STEREO_TCX*32/5));
   ippsZero_16s(st->right.mem_d_tcx_fx,D_NC + (D_STEREO_TCX*32/5));

   Init_dec_hi_stereo(st);

   ippsZero_16s(st->right.wmem_d_nonc,D_NC);
   ippsZero_16s(st->left.wmem_d_nonc,D_NC);

   st->mem_stereo_ovlp_size_fx = 0;
   ippsZero_16s(st->mem_stereo_ovlp_fx,L_OVLP_2k);
   st->mem_balance_fx = 0;

   st->last_stereo_mode =0;
   st->side_rms_fx = 0;
}

void Init_dec_hi_stereo(DecoderObj_AMRWBE* st)
{
   Ipp32s i;

   Cos_window(st->W_window, 0, L_SUBFR, 2);
   for(i=0; i<L_SUBFR; i++) {
      st->W_window[i] = MulHR_16s(st->W_window[i],st->W_window[i]);
   }

   ippsZero_16s(st->old_wh_fx,HI_FILT_ORDER);
   ippsZero_16s(st->old_wh2_fx,HI_FILT_ORDER);
   ippsZero_16s(st->old_exc_mono_fx,HI_FILT_ORDER);
   ippsZero_16s(st->old_AqLF_fx,5*(M+1));

   for(i=0;i<5;i++) {
      st->old_AqLF_fx[i*(M+1)]=4096;
   }

   ippsZero_16s(st->left.mem_synth_hi_fx,M);
   ippsZero_16s(st->right.mem_synth_hi_fx,M);
   ippsZero_16s(st->old_wh_q_fx,HI_FILT_ORDER);
   ippsZero_16s(st->old_gm_gain_fx,2);

   for(i=0;i<4;i++) {
      st->old_gain_left_fx[i]=8192;
      st->old_gain_right_fx[i]=8192;
   }
}

AMRWB_CODECFUN( APIAMRWB_Status, apiCopyDecoderState_AMRWBE,
         (DecoderObj_AMRWBE* pObjWBE, AMRWBDecoder_Obj* pObjWB, Ipp32s direction))
{
   Ipp32s i, Ltmp;
   Ipp16s memHPFilter[9];
   Ipp16s *pBase;

   if (direction == 0) { /* Copy data from WB to WB+ structure*/
      Reset_decoder_amrwb_plus(pObjWBE, 1, 1);

      ippsCopy_16s(pObjWB->asiIspOld, pObjWBE->wispold, M);

      for (i=0; i<M; i++) {
         pObjWBE->wisfold[i] = Cnvrt_NR_32s16s(Mul2_Low_32s(ShiftL_16s(pObjWB->asiIsfOld[i],2) * 20972));
         pObjWBE->wpast_isfq[i] = Cnvrt_NR_32s16s(Mul2_Low_32s(ShiftL_16s(pObjWB->asiIsfQuantPast[i],2) * 20972));
      }

      for (i=0; i<L_MEANBUF; i++)
         ippsCopy_16s(pObjWBE->wisfold, &(pObjWBE->wisf_buf[i*M]), M);

      pObjWBE->siOldPitchLag = pObjWB->siOldPitchLag;
      pObjWBE->siOldPitchLagFrac = pObjWB->siOldPitchLagFrac;

      pObjWBE->wold_T_pf[0] = pObjWBE->wold_T_pf[1] = pObjWB->siOldPitchLag;
      pObjWBE->wold_gain_pf[0] = pObjWBE->wold_gain_pf[1] = 0;

      for (i=0; i<M; i++) {
         Ltmp = ShiftL_32s(pObjWB->asiSynthesis[i], 3);
         pObjWBE->wold_synth[i] = Cnvrt_NR_32s16s(Ltmp);
      }

      for(i=0; i<(PIT_MAX+L_INTERPOL); i++)
         pObjWBE->wold_exc[i + PIT_MAX_MAX - PIT_MAX] = pObjWB->asiExcOld[i];

      pObjWBE->wmem_deemph = pObjWB->siDeemph;

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWB->pSHPFiltStateSgnlOut, memHPFilter, 2);
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWBE->wmemSigOut, 2);

      for (i=0; i<(2*L_FILT); i++)
         pObjWBE->right.wmem_oversamp[i] = (Ipp16s)((pObjWB->asiOversampFilt[i]*20480)>>15);

      pObjWBE->L_gc_thres = pObjWB->iNoiseEnhancerThres;

      for (i=0; i<(PIT_MAX+L_SUBFR); i++)
         pObjWBE->wold_synth_pf[PIT_MAX_MAX - PIT_MAX+i] = pObjWB->mem_syn_out[i];

      pObjWBE->wramp_state = 0;
      for (i=0; i<6; i++) {
         pObjWBE->mem_subfr_q[i] = pObjWB->siScaleFactorOld;
         pObjWBE->old_subfr_q[i] = pObjWB->siScaleFactorOld;
         pObjWBE->old_syn_q[i] = -1;
      }

      pObjWBE->mem_subfr_q[i] = pObjWB->siScaleFactorOld;

      pObjWBE->Q_exc = pObjWB->siScaleFactorOld;
      pObjWBE->Old_Q_syn = 0;
      pObjWBE->right.Q_synHF = Add_16s(pObjWBE->Old_Q_syn, 1);

   } else { /* Copy data from WB+ to WB structure*/
      apiAMRWBDecoder_Init(pObjWB);
      ippsCopy_16s(pObjWBE->wispold, pObjWB->asiIspOld, M);

      for (i = 0; i < M; i++) {
         pObjWB->asiIsfOld[i] = (Ipp16s)((pObjWBE->wisfold[i]*12800)>>15);
         pObjWB->asiIsfQuantPast[i] = (Ipp16s)((pObjWBE->wpast_isfq[i]*12800)>>15);
      }

      pObjWB->siOldPitchLag = pObjWBE->siOldPitchLag;
      pObjWB->siOldPitchLagFrac = pObjWBE->siOldPitchLagFrac;

      for (i = 0; i < M; i++) {
         pObjWB->asiSynthesis[i] = ShiftL_32s(pObjWBE->cp_old_synth[i], (Ipp16s)(12-pObjWBE->Old_Q_syn));
      }

      if (pObjWBE->last_mode == 0) {
         Ipp16s frac, exp1;
         for (i=0; i<4; i++) {
            ownLog2_AMRWBE((pObjWBE->wmem_gain_code[3-i]), &exp1, &frac);
            exp1 = (Ipp16s)(exp1 - 11);
            Ltmp = Mpy_32_16(exp1, frac, LG10);
            pObjWB->asiQuantEnerPast[i] = (Ipp16s)(Ltmp>>(13-10));
         }
      }

      for (i=0; i<(PIT_MAX+L_INTERPOL); i++)
         pObjWB->asiExcOld[i] = pObjWBE->wold_exc[i + PIT_MAX_MAX- PIT_MAX];

      pObjWB->siScaleFactorOld = pObjWBE->Q_exc;

      for (i=0; i<4; i++)
         pObjWB->asiScaleFactorMax[i] = pObjWBE->mem_subfr_q[i];

      if (pObjWBE->Old_Q_syn >= 0) {
         pObjWB->siDeemph = (Ipp16s)(pObjWBE->wmem_deemph >> pObjWBE->Old_Q_syn);
      } else {
         pObjWB->siDeemph = ShiftL_16s(pObjWBE->wmem_deemph, (Ipp16u)(-pObjWBE->Old_Q_syn));
      }

      ippsHighPassFilterGetDlyLine_AMRWB_16s(pObjWBE->wmemSigOut, memHPFilter, 2);
      if (pObjWBE->Old_Q_syn >= 0) {
         for (i=0; i<6; i++)
            memHPFilter[i] = (Ipp16s)(memHPFilter[i] >> pObjWBE->Old_Q_syn);
      } else {
         for (i=0; i<6; i++)
            memHPFilter[i] = ShiftL_16s(memHPFilter[i], (Ipp16u)(-pObjWBE->Old_Q_syn));
      }
      ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, pObjWB->pSHPFiltStateSgnlOut, 2);

      if ( (pObjWBE->Old_Q_syn-1) >= 0) {
         for (i=0; i<(2*L_FILT); i++) {
            Ltmp  = Mul2_Low_32s((pObjWBE->right.wmem_oversamp[i]>>(pObjWBE->Old_Q_syn-1)) * 26214);
            pObjWB->asiOversampFilt[i] = Cnvrt_NR_32s16s(Ltmp);
         }
      } else {
         for (i=0; i<(2*L_FILT); i++) {
            Ltmp  = Mul2_Low_32s(ShiftL_16s(pObjWBE->right.wmem_oversamp[i],(Ipp16u)(1-pObjWBE->Old_Q_syn)) * 26214);
            pObjWB->asiOversampFilt[i] = Cnvrt_NR_32s16s(Ltmp);
         }
      }

      pObjWB->iNoiseEnhancerThres = pObjWBE->L_gc_thres;

      pBase = &(pObjWBE->wold_synth_pf[PIT_MAX_MAX-PIT_MAX]);
      if (pObjWBE->Old_Q_syn >= 0) {
         for (i=0; i<(PIT_MAX+L_SUBFR); i++) {
            pObjWB->mem_syn_out[i] = Cnvrt_NR_32s16s((((Ipp32s)pBase[i])<<16) >> pObjWBE->Old_Q_syn);
         }
      } else {
         for (i=0; i<(PIT_MAX+L_SUBFR); i++) {
            pObjWB->mem_syn_out[i] =
               Cnvrt_NR_32s16s(ShiftL_32s(((Ipp32s)pBase[i])<<16, (Ipp16u)(-pObjWBE->Old_Q_syn)));
         }
      }

      ippsCopy_16s(pObjWBE->wmem_lpc_hf,pObjWB->lpc_hf_plus, 9);

      pObjWB->gain_hf_plus = pObjWBE->wmem_gain_hf;

      if (pObjWBE->right.Q_synHF >= 0) {
         for (i=0; i<(2*L_FILT); i++) {
            pObjWB->mem_oversamp_hf_plus[i] = (Ipp16s)(pObjWBE->right.wmem_oversamp_hf[i] >> pObjWBE->right.Q_synHF);
         }
         for (i = 0; i < 8; i++) {
            pObjWB->mem_syn_hf_plus[i] = (Ipp16s)(pObjWBE->right.wmem_syn_hf[i] >> pObjWBE->right.Q_synHF);
         }
      } else {
         for (i=0; i<(2*L_FILT); i++) {
            pObjWB->mem_oversamp_hf_plus[i] =
               ShiftL_16s(pObjWBE->right.wmem_oversamp_hf[i], (Ipp16u)(-pObjWBE->right.Q_synHF));
         }
         for (i = 0; i < 8; i++) {
            pObjWB->mem_syn_hf_plus[i] =
               ShiftL_16s(pObjWBE->right.wmem_syn_hf[i], (Ipp16u)(-pObjWBE->right.Q_synHF));
         }
      }

      pObjWB->threshold_hf = pObjWBE->right.Lthreshold;
      pObjWB->lp_amp_hf = pObjWBE->right.Lp_amp;
      pObjWB->ramp_state = 64;
   }

   return APIAMRWB_StsNoErr;
}
