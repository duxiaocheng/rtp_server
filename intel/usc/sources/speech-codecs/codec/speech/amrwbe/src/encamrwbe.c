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
// Purpose: AMRWBE speech codec encoder API functions.
//
*/
#include "ownamrwbe.h"

#define REMOVED_BITS 1

static APIAMRWB_Status Coder_lf_b(Ipp32s codec_mode, Ipp16s *speech, Ipp16s *synth, Ipp16s *mod, Ipp16s *AqLF,
   Ipp16s *wwindow, Ipp16s *param, Ipp16s *ol_gain, Ipp16s *ave_T_out, Ipp16s *ave_p_out,
   Ipp16s *coding_mod, Ipp32s pit_adj, EncoderObj_AMRWBE *st);

static APIAMRWB_Status Coder_lf(Ipp32s codec_mode, Ipp16s *speech, Ipp16s *synth, Ipp16s *mod, Ipp16s *AqLF,
   Ipp16s *window, Ipp16s *param, Ipp16s *ol_gain, Ipp16s *ave_T_out, Ipp16s *ave_p_out,
   Ipp16s *coding_mod, Ipp32s pit_adj, EncoderObj_AMRWBE *st);

static void Coder_hf(Ipp16s* mod, Ipp16s* AqLF, Ipp16s* speech, Ipp16s* speech_hf, Ipp16s* window,
   Ipp16s* param, Ipp32s fscale, EncoderOneChanelState_AMRWBE *st, Ipp16s Q_new, Ipp16s Q_speech);

static void Coder_stereo_x(Ipp16s* AqLF, Ipp32s brMode, Ipp16s* param, Ipp16s fscale,
   EncoderObj_AMRWBE *st, Ipp16s* wspeech_hi, Ipp16s* wchan_hi, Ipp16s* wspeech_2k, Ipp16s* wchan_2k);

static APIAMRWB_Status Coder_acelp(Ipp16s* Az, Ipp16s* Azq, Ipp16s* speech, Ipp16s *mem_wsyn_, Ipp16s* synth_,
   Ipp16s* exc, Ipp16s* wovlp_, Ipp32s len, Ipp32s codec_mode, Ipp16s norm_corr, Ipp16s norm_corr2,
   Ipp16s T_op, Ipp16s T_op2, Ipp16s* T_out, Ipp16s* p_out, Ipp16s* c_out_, Ipp16s* sprm, Ipp16s* xn_in,
   EncoderObj_AMRWBE* st);

static void Try_tcx(Ipp32s k, Ipp32s mode, Ipp16s *snr, Ipp16s* A, Ipp16s* wsp, Ipp16s* mod,
   Ipp16s* coding_mod, Ipp16s* isp, Ipp16s* isp_q, Ipp16s* AqLF, Ipp16s* speech, Ipp16s* mem_w0,
   Ipp16s* mem_xnq, Ipp16s* mem_wsyn, Ipp16s* old_exc, Ipp16s* mem_syn, Ipp16s* wovlp,
   Ipp16s* ovlp_size, Ipp16s* past_isfq, Ipp32s nbits, Ipp32s nprm_tcx, Ipp16s* prm, EncoderObj_AMRWBE *st);

static Ipp16s Coder_tcx(Ipp16s* A, Ipp16s* speech, Ipp16s* mem_wsp, Ipp16s* mem_wsyn,
   Ipp16s* synth, Ipp16s* exc_, Ipp16s* wovlp, Ipp32s ovlp_size, Ipp32s frameSize,
   Ipp32s nb_bits, Ipp16s* prm, EncoderObj_AMRWBE *st);

static void Cod_hi_stereo(Ipp16s* wAqLF, Ipp16s* param, EncoderObj_AMRWBE *st,
   Ipp16s* speech_hi_, Ipp16s* right_hi_);

static void Cod_tcx_stereo(Ipp16s* wmono_2k, Ipp16s* wright_2k, Ipp16s* param,
   Ipp32s brMode, Ipp16s* mod, Ipp16s fscale, EncoderObj_AMRWBE *st);

static APIAMRWB_Status Ctcx_stereo(Ipp16s* wside, Ipp16s* wmono, Ipp16s* wsynth, Ipp16s* wwovlp,
   Ipp32s ovlp_size, Ipp32s frameSize, Ipp32s nb_bits, Ipp16s* sprm, Ipp16s pre_echo, Ipp16s Q_in);

AMRWB_CODECFUN( APIAMRWB_Status, apiEncoderGetObjSize_AMRWBE,
         (Ipp32u *pCodecSize))
{
   Ipp32s sizeObj;
   if(NULL == pCodecSize)
      return APIAMRWB_StsBadArgErr;

   *pCodecSize=sizeof(EncoderObj_AMRWBE); // aligned by 8
   ippsHighPassFilterGetSize_AMRWB_16s(2, &sizeObj); // aligned by 8
   *pCodecSize += (2*sizeObj); /* left/right memSigIn */
   ippsHighPassFilterGetSize_AMRWB_16s(3, &sizeObj); // aligned by 8
   *pCodecSize += sizeObj; /* memHPOLLTP */
   ippsVADGetSize_AMRWB_16s(&sizeObj); // not aligned by 8
   *pCodecSize += (sizeObj+7)&(~7);
   *pCodecSize += sizeof(NCLASSDATA_FX); // aligned by 8

   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiEncoderProceedFirst_AMRWBE,
         (EncoderObj_AMRWBE* pEncObj,
         Ipp16s* channel_right,
         Ipp16s* channel_left,
         Ipp16s n_channel,
         Ipp32s frameSize,
         Ipp16s fscale,
         Ipp32s* pCountSamplesProceeded))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, pChannelRight, (2*L_FRAME_MAX_FS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pChannelLeft, (2*L_FRAME_MAX_FS));

   IPP_ALIGNED_ARRAY(16, Ipp16s, buffer, (L_FRAME_MAX_FS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSignalRight, (L_FRAME_PLUS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSignalLeft, (L_FRAME_PLUS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pSpeechBuff, (L_TOTAL_ST));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pMono2kBuff, (L_TOTAL_ST_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pMonoHFBuff, (L_TOTAL_ST_hi));

   Ipp16s* pNewSpeech = pSpeechBuff + L_OLD_SPEECH_ST;
   Ipp16s *pNewMono2k = pMono2kBuff + L_OLD_SPEECH_2k;
   Ipp16s *pNewMonoHF = pMonoHFBuff + L_OLD_SPEECH_hi;
   Ipp32s offsetSave = L_FRAME_PLUS-L_OLD_SPEECH_ST;
   Ipp32s i, cntSamples, dwTmp;

   Ipp32s nBitToRemove;
   Ipp16s scale_fac, fac_fs;
   Ipp16s fac_up,fac_down, exp_u, m_fu, tmp_len;
   Ipp32s WorkLen, L_next=0;

   switch (frameSize) {
   case L_FRAME8k:
      L_next = (n_channel==2) ? L_NEXT_ST8k : L_NEXT8k;
      break;
   case L_FRAME16kPLUS:
      L_next = (n_channel==2) ? L_NEXT_ST16k : L_NEXT16k;
      break;
   case L_FRAME24k:
      L_next = (n_channel==2) ? L_NEXT_ST24k : L_NEXT24k;
      break;
   }

   if (frameSize == L_FRAME32k) {
      fac_fs = FSCALE_DENOM*3/2;
   } else {
      fac_fs = fscale;
   }
   fac_up = (Ipp16s)(fac_fs << 3);
   fac_down = 180*8;

   if (fscale != 0) {
      if (frameSize == (2*L_FRAME44k)) {
         fac_up = (Ipp16s)(fac_fs << 3);
         fac_down = 3*441;
      }
      if (n_channel == 2) {
         tmp_len = L_NEXT_ST;
      } else {
         tmp_len = L_NEXT_WBP;
      }
      exp_u = Exp_16s(fac_up);
      m_fu = ShiftL_16s(fac_up, exp_u);

      dwTmp = ShiftL_16s(tmp_len,1) * fac_down;
      dwTmp = dwTmp + pEncObj->right.decim_frac;
      WorkLen = Div_16s(ExtractHigh(ShiftL_32s(dwTmp, (Ipp16u)(exp_u+1))), m_fu);

      if (dwTmp >= ((WorkLen+1)*fac_up)) {
         WorkLen = WorkLen + 1;
      }
   } else {
      WorkLen = L_next;
   }

   ippsCopy_16s(channel_right, pChannelRight, WorkLen);
   ippsCopy_16s(channel_left, pChannelLeft, WorkLen);

   ippsZero_16s(buffer, L_FRAME_MAX_FS);
   ippsZero_16s(pSpeechBuff, L_TOTAL_ST);

   ippsZero_16s(pSignalLeft, L_FRAME_PLUS);
   ippsZero_16s(pSignalRight, L_FRAME_PLUS);

   if((fscale<72) && (fscale != 0)) {
      nBitToRemove  = 2;
   } else {
      nBitToRemove = 1;
   }

   if (n_channel == 2) { /* stereo branch */

      ippsZero_16s(pMono2kBuff, L_OLD_SPEECH_2k);
      ippsZero_16s(pMonoHFBuff, L_OLD_SPEECH_hi);

      scale_fac = ownScaleTwoChannels_16s(pChannelLeft, pChannelRight, WorkLen, &(pEncObj->Q_new),
         pEncObj->OldQ_sp_deci, nBitToRemove);
      ownRescaleEncoderStereo(pEncObj, scale_fac, scale_fac);

      if (fscale == 0) {
         ippsCopy_16s(pChannelLeft, buffer+L_next, L_next);
         ippsDownsample_AMRWBE_16s(buffer, frameSize, pSignalLeft, pEncObj->left.mem_decim, 0);

         ippsCopy_16s(pChannelRight, buffer+L_next, L_next);
         ippsDownsample_AMRWBE_16s(buffer, frameSize, pSignalRight, pEncObj->right.mem_decim, 0);
         cntSamples = L_next;
      } else {
         ippsBandSplitDownsample_AMRWBE_16s(pChannelLeft, frameSize, pSignalLeft+L_FRAME_PLUS-L_NEXT_ST,
            pChannelLeft, L_NEXT_ST, pEncObj->left.mem_decim, &(pEncObj->left.decim_frac), &cntSamples, fac_fs);

         ippsBandSplitDownsample_AMRWBE_16s(pChannelRight, frameSize, pSignalRight+L_FRAME_PLUS-L_NEXT_ST,
            pChannelRight, L_NEXT_ST, pEncObj->right.mem_decim, &(pEncObj->right.decim_frac), &cntSamples, fac_fs);
      }

      ippsHighPassFilter_AMRWB_16s_ISfs(pSignalLeft, L_FRAME_PLUS, pEncObj->left.memSigIn, 14);
      ippsHighPassFilter_AMRWB_16s_ISfs(pSignalRight, L_FRAME_PLUS, pEncObj->right.memSigIn, 14);

      ownMixChennels_16s(pSignalLeft, pSignalRight, pNewSpeech, L_FRAME_PLUS, 16384, 16384);

      ippsBandSplit_AMRWBE_16s(pNewSpeech-2*L_FDEL, pNewMono2k-2*L_FDEL_2k, pNewMonoHF, L_FRAME_PLUS);

      ippsCopy_16s(&pNewSpeech[offsetSave], pEncObj->old_speech, L_OLD_SPEECH_ST);
      ippsCopy_16s(pMono2kBuff+L_FRAME_2k, pEncObj->old_speech_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(pMonoHFBuff+L_FRAME_PLUS, pEncObj->old_speech_hi, L_OLD_SPEECH_hi);

   } else { /* mono branch */

      scale_fac = ownScaleOneChannel_16s(pChannelRight, WorkLen, &(pEncObj->Q_new), pEncObj->OldQ_sp_deci, nBitToRemove);
      ownRescaleEncoderMono(pEncObj, scale_fac, scale_fac);

      if (fscale == 0) {
         ippsCopy_16s(pChannelRight, buffer+(L_next*3), L_next);
         ippsDownsample_AMRWBE_16s(buffer, frameSize, pNewSpeech, pEncObj->right.mem_decim, 0);
         cntSamples = L_next;
      } else {
         ippsBandSplitDownsample_AMRWBE_16s(pChannelRight, frameSize, pNewSpeech+L_FRAME_PLUS-L_NEXT_WBP,
            pChannelRight, L_NEXT_WBP, pEncObj->right.mem_decim, &(pEncObj->right.decim_frac), &cntSamples, fac_fs);
      }

      ippsHighPassFilter_AMRWB_16s_ISfs(pNewSpeech, L_FRAME_PLUS, pEncObj->right.memSigIn, 14);
      ippsCopy_16s(&pNewSpeech[offsetSave], pEncObj->old_speech, L_OLD_SPEECH_ST);
   }

   Preemph_scaled(pNewSpeech, &(pEncObj->Q_sp), &(pEncObj->scale_fac), &(pEncObj->right.mem_preemph),
      pEncObj->Q_max, &(pEncObj->OldQ_sp),PREEMPH_FAC_FX, REMOVED_BITS, L_FRAME_PLUS);

   pEncObj->Q_sp = (Ipp16s)((pEncObj->Q_sp + pEncObj->Q_new) - REMOVED_BITS);
   ownRescaleEncoderMono(pEncObj, 0, pEncObj->scale_fac);
   pEncObj->scale_fac = (Ipp16s)(pEncObj->scale_fac + scale_fac);

   ippsCopy_16s(&pNewSpeech[offsetSave], pEncObj->old_speech_pe, L_OLD_SPEECH_ST);

   if (n_channel == 2) {
      ippsCopy_16s(pSignalRight, pNewSpeech, L_FRAME_PLUS);

      ippsBandSplit_AMRWBE_16s(&pNewSpeech[-2*L_FDEL] , &pNewMono2k[-2*L_FDEL_2k],
         pNewMonoHF, L_FRAME_PLUS);

      ippsCopy_16s(&pNewSpeech[offsetSave], pEncObj->old_chan, L_OLD_SPEECH_ST);
      ippsCopy_16s(&pMono2kBuff[L_FRAME_2k], pEncObj->old_chan_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(&pMonoHFBuff[L_FRAME_PLUS], pEncObj->old_chan_hi, L_OLD_SPEECH_hi);
   }

   if (frameSize > L_FRAME8k) {
      if (n_channel == 2) { /* streo mode */
         if (fscale == 0) {
            ippsCopy_16s(pChannelLeft, buffer+L_next, L_next);
            ippsDownsample_AMRWBE_16s(buffer, frameSize, pSignalLeft, pEncObj->left.mem_decim_hf, 1);
            ippsCopy_16s(pChannelRight, buffer+L_next, L_next);
            ippsDownsample_AMRWBE_16s(buffer, frameSize, pSignalRight, pEncObj->right.mem_decim_hf, 1);

            ippsCopy_16s(&pSignalLeft[offsetSave], pEncObj->left.old_speech_hf, L_OLD_SPEECH_ST);
            ippsCopy_16s(&pSignalRight[offsetSave], pEncObj->right.old_speech_hf, L_OLD_SPEECH_ST);
         } else {
            for (i=0; i<M; i++) {
               pEncObj->left.old_speech_hf[i] = pNewSpeech[offsetSave+i];
               pEncObj->right.old_speech_hf[i] = pNewSpeech[offsetSave+i];
            }
            ippsCopy_16s(pChannelLeft, &pEncObj->left.old_speech_hf[M], L_NEXT_ST);
            ippsCopy_16s(pChannelRight, &pEncObj->right.old_speech_hf[M], L_NEXT_ST);
         }

      } else { /* mono mode */
         if (fscale == 0) {
            ippsCopy_16s(pChannelRight, buffer+(3*L_next), L_next);
            ippsDownsample_AMRWBE_16s(buffer, frameSize, pSignalRight, pEncObj->right.mem_decim_hf, 1);
            ippsCopy_16s(&pSignalRight[offsetSave], pEncObj->right.old_speech_hf, L_OLD_SPEECH_ST);
         } else {
            for (i=0; i<M; i++) {
               pEncObj->right.old_speech_hf[i] = pNewSpeech[offsetSave+i];
            }
            ippsCopy_16s(pChannelRight, &pEncObj->right.old_speech_hf[M], L_NEXT_WBP);
         }
      }
   }
   *pCountSamplesProceeded = cntSamples;
   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiEncoderStereo_AMRWBE,
   (EncoderObj_AMRWBE *st,
   Ipp16s* channel_right,
   Ipp16s* channel_left,
   Ipp32s codec_mode,
   Ipp32s frameSize,
   Ipp16s* serial,
   Ipp32s use_case_mode,
   Ipp32s fscale,
   Ipp32s StbrMode,
   Ipp32s* pCountSamplesProceeded))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, wAqLF, ((NB_SUBFR_WBP+1)*(M+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sparam, (NB_DIV*NPRM_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sprm_stereo, (MAX_NPRM_STEREO_DIV*NB_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sprm_hf_left, (NB_DIV*NPRM_BWE_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sprm_hf_right, (NB_DIV*NPRM_BWE_DIV));

   IPP_ALIGNED_ARRAY(16, Ipp16s, wsig_left, (L_FRAME_PLUS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sig_right, (L_FRAME_PLUS));

   IPP_ALIGNED_ARRAY(16, Ipp16s, old_speech, (L_TOTAL_ST));
   Ipp16s *speech, *new_speech;
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_synth, (M+L_FRAME_PLUS));
   Ipp16s *wsynth;

   IPP_ALIGNED_ARRAY(16, Ipp16s, pChannelRight,(2*L_FRAME_MAX_FS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, pChannelLeft, (2*L_FRAME_MAX_FS));

   Ipp32s i, WorkLen, nBitToRemove, nbits_pack;
   Ipp16s mod[NB_DIV];
   Ipp32s cntSamples;
   Ipp16s fac_fs;
   Ipp16s excType[4];

   Ipp16s wol_gain[NB_DIV];
   Ipp16s wT_out[NB_DIV];
   Ipp16s wp_out[NB_DIV], scale_fac, scale_fac_hf;
   Ipp16s fac_up,fac_down, exp_u, m_fu;
   Ipp32s dwTmp;

   if (frameSize == L_FRAME32k) {
      fac_fs = FSCALE_DENOM*3/2;
   } else {
      fac_fs = (Ipp16s)fscale;
   }
   fac_up = (Ipp16s)(fac_fs << 3);
   fac_down = 180*8;

   if (fscale != 0) {
      if (frameSize == (2*L_FRAME44k)) {
         fac_up = (Ipp16s)(fac_fs << 3);
         fac_down = 3*441;
      }
      exp_u = Exp_16s(fac_up);
      m_fu = ShiftL_16s(fac_up, exp_u);
      dwTmp = (L_FRAME_PLUS*2 * fac_down);
      dwTmp = dwTmp + (st->right.decim_frac);
      WorkLen = Div_16s(ExtractHigh(ShiftL_32s(dwTmp, (Ipp16u)(exp_u+1))), m_fu);
      if (dwTmp >= (WorkLen+1)*fac_up) {
         WorkLen = WorkLen + 1;
      }
   } else {
      WorkLen = frameSize;
  }

   ippsCopy_16s(channel_right, pChannelRight, WorkLen);
   ippsCopy_16s(channel_left, pChannelLeft, WorkLen);

   new_speech = old_speech + L_OLD_SPEECH_ST;
   speech     = old_speech + L_TOTAL_ST - L_FRAME_PLUS - L_A_MAX - L_BSP;
   wsynth = old_synth + M;

   if((fscale<72) && (fscale != 0))
      nBitToRemove  = 2;
   else
      nBitToRemove = 1;

   scale_fac = ownScaleTwoChannels_16s(pChannelLeft, pChannelRight, WorkLen, &(st->Q_new), st->OldQ_sp_deci, nBitToRemove);
   ownRescaleEncoderStereo(st, scale_fac, scale_fac);

   if (fscale == 0) {
      ippsDownsample_AMRWBE_16s(pChannelLeft, frameSize, wsig_left, st->left.mem_decim, 0);
      ippsDownsample_AMRWBE_16s(pChannelRight, frameSize, sig_right, st->right.mem_decim, 0);
      cntSamples = frameSize;
   } else {
      ippsBandSplitDownsample_AMRWBE_16s(pChannelLeft, frameSize, wsig_left, pChannelLeft,
         L_FRAME_PLUS, st->left.mem_decim, &(st->left.decim_frac), &cntSamples, fac_fs);
      ippsBandSplitDownsample_AMRWBE_16s(pChannelRight, frameSize, sig_right, pChannelRight,
         L_FRAME_PLUS, st->right.mem_decim, &(st->right.decim_frac), &cntSamples, fac_fs);
   }

   ippsHighPassFilter_AMRWB_16s_ISfs(wsig_left, L_FRAME_PLUS, st->left.memSigIn, 14);
   ippsHighPassFilter_AMRWB_16s_ISfs(sig_right, L_FRAME_PLUS, st->right.memSigIn, 14);

   ownMixChennels_16s(wsig_left,sig_right,new_speech,L_FRAME_PLUS, 16384, 16384);

   Preemph_scaled(new_speech, &(st->Q_sp), &(st->scale_fac),&(st->right.mem_preemph),st->Q_max, &(st->OldQ_sp),PREEMPH_FAC_FX, REMOVED_BITS, L_FRAME_PLUS);
   st->Q_sp = (Ipp16s)( st->Q_sp + st->Q_new - REMOVED_BITS);
   ownRescaleEncoderStereo(st, 0, st->scale_fac);
   st->scale_fac = Add_16s(st->scale_fac, scale_fac);

   ippsCopy_16s(st->old_speech_pe, old_speech, L_OLD_SPEECH_ST);
   ippsCopy_16s(st->old_synth, old_synth, M);

   if (use_case_mode == USE_CASE_B) {
      for(i=0; i<4; i++) {
         {
            IPP_ALIGNED_ARRAY(16, Ipp16s, tmps, (256));
            ippsCopy_16s(&new_speech[256*i],tmps, 256);
            ownScaleSignal_AMRWB_16s_ISfs(tmps, 256, (Ipp16s)(-st->Q_sp));
            ippsVAD_AMRWB_16s(tmps, st->pVADState, &(st->tone_flag), &st->_stClass->vadFlag[i]);
         }
         if (st->_stClass->vadFlag[i] == 0) {
            st->vad_hist = Add_16s(st->vad_hist, 1);
         } else {
            st->vad_hist = 0;
         }
         {
            Ipp16s vadLevel[COMPLEN];
            ippsVADGetEnergyLevel_AMRWB_16s(st->pVADState, vadLevel);
            excType[i] = ClassifyExcitation(st->_stClass, vadLevel, i);
         }
      }
      if (Coder_lf_b(codec_mode, speech, wsynth, mod, wAqLF, st->window, sparam,
         wol_gain, wT_out, wp_out, excType, fscale, st)) {
            return APIAMRWB_StsErr;
      }
   } else {
      for (i=0; i<4; i++) {
         excType[i] = 0;
      }
      if ( Coder_lf(codec_mode, speech, wsynth, mod, wAqLF, st->window,
         sparam, wol_gain, wT_out, wp_out, excType, fscale, st) ) {
            return APIAMRWB_StsErr;
      }
   }

   for(i=0; i<4; i++) {
      mod[i] = excType[i];
   }
   ippsCopy_16s(&old_speech[L_FRAME_PLUS], st->old_speech_pe, L_OLD_SPEECH_ST);
   ippsCopy_16s(&old_synth[L_FRAME_PLUS], st->old_synth, M);

   if (frameSize > L_FRAME8k) {
      IPP_ALIGNED_ARRAY(16, Ipp16s, old_speech_hf, (L_TOTAL_ST));
      Ipp16s *new_speech_hf, *speech_hf_;

      new_speech_hf = old_speech_hf + L_OLD_SPEECH_ST;
      speech_hf_     = old_speech_hf + L_TOTAL_ST - L_FRAME_PLUS - L_A_MAX - L_BSP;
      ippsCopy_16s(st->left.old_speech_hf, old_speech_hf, L_OLD_SPEECH_ST);

      if (fscale == 0) {
         ippsDownsample_AMRWBE_16s(pChannelLeft, frameSize, new_speech_hf, st->left.mem_decim_hf, 1);
      } else {
         ippsCopy_16s(pChannelLeft, new_speech_hf, L_FRAME_PLUS);
      }
      ippsCopy_16s(&old_speech_hf[L_FRAME_PLUS], st->left.old_speech_hf, L_OLD_SPEECH_ST);

      if (StbrMode < 0) {
         ippsCopy_16s(speech_hf_, pChannelLeft, L_FRAME_PLUS+L_A_MAX+L_BSP);
      } else {
         scale_fac_hf = ownScaleOneChannel_16s(old_speech_hf, L_TOTAL_ST, &(st->left.Q_sp_hf), st->left.OldQ_sp_hf, 1);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.mem_hf3, MHF, scale_fac_hf);
         Coder_hf(mod, wAqLF, speech, speech_hf_, st->window, sprm_hf_left, fscale, &(st->left),st->Q_new, st->Q_sp);
      }
      ippsCopy_16s(st->right.old_speech_hf, old_speech_hf, L_OLD_SPEECH_ST);

      if (fscale == 0) {
         ippsDownsample_AMRWBE_16s(pChannelRight, frameSize, new_speech_hf, st->right.mem_decim_hf, 1);
      } else {
         ippsCopy_16s(pChannelRight, new_speech_hf, L_FRAME_PLUS);
      }

      if (StbrMode < 0) {
         for (i=0; i<L_FRAME_PLUS+L_A_MAX+L_BSP; i++) {
            speech_hf_[i] =
               Cnvrt_NR_32s16s(Add_32s(Mul2_Low_32s(speech_hf_[i]*16384), Mul2_Low_32s(pChannelLeft[i]*16384)));
         }
      }
      ippsCopy_16s(&old_speech_hf[L_FRAME_PLUS], st->right.old_speech_hf, L_OLD_SPEECH_ST);
      scale_fac_hf = ownScaleOneChannel_16s(old_speech_hf, L_TOTAL_ST, &(st->right.Q_sp_hf), st->right.OldQ_sp_hf, 1);
      ownScaleSignal_AMRWB_16s_ISfs(st->right.mem_hf3, MHF, scale_fac_hf);
      Coder_hf(mod, wAqLF, speech, speech_hf_, st->window, sprm_hf_right, fscale, &(st->right),st->Q_new, st->Q_sp);
   } else {
      ippsZero_16s(sprm_hf_right, (NB_DIV*NPRM_BWE_DIV));
      ippsZero_16s(sprm_hf_left, (NB_DIV*NPRM_BWE_DIV));
   }

   {
      IPP_ALIGNED_ARRAY(16, Ipp16s, old_mono_2k, (L_TOTAL_ST_2k));
      Ipp16s *new_mono_2k = old_mono_2k + L_OLD_SPEECH_2k;
      Ipp16s *wmono_2k = old_mono_2k+L_TOTAL_ST_2k-L_FRAME_2k-L_A_2k-L_FDEL_2k;

      IPP_ALIGNED_ARRAY(16, Ipp16s, old_chan_2k, (L_TOTAL_ST_2k));
      Ipp16s *wnew_chan_2k = old_chan_2k + L_OLD_SPEECH_2k;
      Ipp16s *wchan_2k = old_chan_2k+L_TOTAL_ST_2k-L_FRAME_2k-L_A_2k-L_FDEL_2k;

      IPP_ALIGNED_ARRAY(16, Ipp16s, old_mono_hi, (L_TOTAL_ST_hi));
      Ipp16s *new_mono_hi = old_mono_hi + L_OLD_SPEECH_hi;
      Ipp16s *wmono_hi = old_mono_hi + L_TOTAL_ST_hi - L_FRAME_PLUS-L_A_MAX;

      IPP_ALIGNED_ARRAY(16, Ipp16s, old_chan_hi, (L_TOTAL_ST_hi));
      Ipp16s *wnew_chan_hi = old_chan_hi + L_OLD_SPEECH_hi;
      Ipp16s *wchan_hi = old_chan_hi + L_TOTAL_ST_hi - L_FRAME_PLUS-L_A_MAX;

      ippsCopy_16s(st->old_speech, old_speech, L_OLD_SPEECH_ST);
      ippsCopy_16s(st->old_speech_2k, old_mono_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(st->old_speech_hi, old_mono_hi, L_OLD_SPEECH_hi);
      ownMixChennels_16s(wsig_left,sig_right,new_speech,L_FRAME_PLUS, 16384, 16384);

      ippsBandSplit_AMRWBE_16s(&new_speech[-2*L_FDEL], &new_mono_2k[-2*L_FDEL_2k], new_mono_hi, L_FRAME_PLUS);
      ippsCopy_16s(&old_speech[L_FRAME_PLUS], st->old_speech, L_OLD_SPEECH_ST);
      ippsCopy_16s(&old_mono_2k[L_FRAME_2k], st->old_speech_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(&old_mono_hi[L_FRAME_PLUS], st->old_speech_hi, L_OLD_SPEECH_hi);

      ippsCopy_16s(st->old_chan, old_speech, L_OLD_SPEECH_ST);
      ippsCopy_16s(st->old_chan_2k, old_chan_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(st->old_chan_hi, old_chan_hi, L_OLD_SPEECH_hi);

      ippsCopy_16s(sig_right, new_speech, L_FRAME_PLUS);

      ippsBandSplit_AMRWBE_16s(&new_speech[-2*L_FDEL], &wnew_chan_2k[-2*L_FDEL_2k], wnew_chan_hi, L_FRAME_PLUS);
      ippsCopy_16s(&old_speech[L_FRAME_PLUS], st->old_chan, L_OLD_SPEECH_ST);
      ippsCopy_16s(&old_chan_2k[L_FRAME_2k], st->old_chan_2k, L_OLD_SPEECH_2k);
      ippsCopy_16s(&old_chan_hi[L_FRAME_PLUS], st->old_chan_hi, L_OLD_SPEECH_hi);

      if (StbrMode < 0) {
         ippsZero_16s(st->old_exc_mono,HI_FILT_ORDER);
         ippsZero_16s(st->old_wh,HI_FILT_ORDER);
         ippsZero_16s(st->old_wh_q,HI_FILT_ORDER);
         ippsZero_16s(st->old_gm_gain,2);
         st->filt_energy_threshold = 0;
         ippsZero_16s(st->mem_stereo_ovlp,L_OVLP_2k);
         st->mem_stereo_ovlp_size= L_OVLP_2k;
      } else {
         Coder_stereo_x(wAqLF, StbrMode, sprm_stereo, (Ipp16s)fscale, st, wmono_hi, wchan_hi, wmono_2k, wchan_2k);
      }
   }

   nbits_pack = (tblNBitsMonoCore_16s[codec_mode] + NBITS_BWE) >> 2;

   if (StbrMode >= 0) {
      nbits_pack = nbits_pack + ((tblNBitsStereoCore_16s[StbrMode] + NBITS_BWE) >> 2);
   }

   for (i=0; i<NB_DIV; i++) {
      Int2bin(mod[i], 2, &serial[i*nbits_pack]);
   }

   Enc_prm(mod, (Ipp16s)codec_mode, sparam, serial, nbits_pack);
   if (StbrMode >= 0) {
      Enc_prm_stereo_x(sprm_stereo, serial, nbits_pack, NBITS_BWE, StbrMode);
      Enc_prm_hf(mod, sprm_hf_left, serial-NBITS_BWE/4, nbits_pack);
   }
   Enc_prm_hf(mod, sprm_hf_right, serial, nbits_pack);
   *pCountSamplesProceeded = cntSamples;
   return APIAMRWB_StsNoErr;
}


AMRWB_CODECFUN( APIAMRWB_Status, apiEncoderMono_AMRWBE,
      (EncoderObj_AMRWBE *st, Ipp16s* channel_right, Ipp32s codec_mode, Ipp32s frameSize,
      Ipp16s* serial, Ipp32s use_case_mode, Ipp32s fscale, Ipp32s *pCountSamplesProceeded))
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, pChannelRight, (2*L_FRAME_MAX_FS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, AqLF, ((NB_SUBFR_WBP+1)*(M+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, param, (NB_DIV*NPRM_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_hf_right, (NB_DIV*NPRM_BWE_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_speech, (L_TOTAL_ST));
   Ipp16s *speech, *new_speech;
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_synth, (M+L_FRAME_PLUS));
   Ipp16s *wsynth;
   Ipp32s i, k, nbits_pack, WorkLen;
   Ipp16s mod[NB_DIV];
   Ipp16s ol_gain[NB_DIV];
   Ipp16s T_out[NB_DIV];
   Ipp16s p_out[NB_DIV];
   Ipp32s cntSamples;
   Ipp16s fac_fs;
   Ipp16s excType[4];
   Ipp16s scale_fac_hf;
   Ipp32s nBitToRemove;
   Ipp16s scale_fac = 0;
   Ipp16s fac_up,fac_down, exp_u, m_fu;
   Ipp32s dwTmp;

   if (frameSize == L_FRAME32k) {
      fac_fs = FSCALE_DENOM*3/2;
   } else {
      fac_fs = (Ipp16s)fscale;
   }
   fac_up = (Ipp16s)(fac_fs << 3);
   fac_down = 180*8;

   if (fscale != 0) {
      if (frameSize == (2*L_FRAME44k)) {
         fac_up = (Ipp16s)(fac_fs << 3);
         fac_down = 3*441;
      }
      exp_u = Exp_16s(fac_up);
      m_fu = ShiftL_16s(fac_up, exp_u);
      /*frameSize = ((L_frame_int*fac_down)+(*frac_mem))/fac_up;*/
      dwTmp = Mul2_Low_32s(L_FRAME_PLUS*2 * fac_down);
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(st->right.decim_frac));
      WorkLen = Div_16s(ExtractHigh(ShiftL_32s(dwTmp, exp_u)), m_fu);
      dwTmp = Sub_32s(dwTmp, Mul2_Low_32s((WorkLen + 1)*fac_up));
      if (dwTmp >= 0) {
        WorkLen = (WorkLen + 1);
      }
   } else {
      WorkLen = frameSize;
   }

   ippsCopy_16s(channel_right, pChannelRight, WorkLen);



  new_speech = old_speech + L_OLD_SPEECH_ST;
  speech     = old_speech + L_TOTAL_ST - L_FRAME_PLUS - L_A_MAX - L_BSP;
  wsynth      = old_synth + M;

   if ((fscale<72) && (fscale != 0)) {
      nBitToRemove = 2;
   } else {
      nBitToRemove = 1;
   }

   scale_fac = ownScaleOneChannel_16s(pChannelRight, WorkLen, &(st->Q_new), st->OldQ_sp_deci, nBitToRemove);
   ownRescaleEncoderMono(st, scale_fac, scale_fac);

   if (fscale == 0) {
      ippsDownsample_AMRWBE_16s(pChannelRight, frameSize, new_speech, st->right.mem_decim, 0);
      cntSamples = frameSize;
   } else {
      ippsBandSplitDownsample_AMRWBE_16s(pChannelRight, frameSize, new_speech, pChannelRight,
         L_FRAME_PLUS, st->right.mem_decim, &(st->right.decim_frac), &cntSamples, fac_fs);
   }

   ippsHighPassFilter_AMRWB_16s_ISfs(new_speech, L_FRAME_PLUS, st->right.memSigIn, 14);

   Preemph_scaled(new_speech, &(st->Q_sp), &(st->scale_fac),&(st->right.mem_preemph), st->Q_max,
      &(st->OldQ_sp),PREEMPH_FAC_FX, REMOVED_BITS, L_FRAME_PLUS);

   st->Q_sp = (Ipp16s)(st->Q_sp + st->Q_new - REMOVED_BITS);
   ownRescaleEncoderMono(st, 0, st->scale_fac);
   st->scale_fac = Add_16s(st->scale_fac, scale_fac);

   ippsCopy_16s(st->old_speech_pe, old_speech, L_OLD_SPEECH_ST);
   ippsCopy_16s(st->old_synth, old_synth, M);

   if (use_case_mode == USE_CASE_B) {
      for(i=0; i<4; i++) {
         {
            IPP_ALIGNED_ARRAY(16, Ipp16s, tmps, 256);
            ippsCopy_16s(&new_speech[256*i], tmps, 256);
            ownScaleSignal_AMRWB_16s_ISfs(tmps, 256, (Ipp16s)(-st->Q_sp));
            ippsVAD_AMRWB_16s(tmps, st->pVADState, &(st->tone_flag), &st->_stClass->vadFlag[i]);
         }
         if (st->_stClass->vadFlag[i] == 0) {
            st->vad_hist = (Ipp16s)(st->vad_hist + 1);
         } else {
            st->vad_hist = 0;
         }
         {
            Ipp16s vadLevel[COMPLEN];
            ippsVADGetEnergyLevel_AMRWB_16s(st->pVADState, vadLevel);
            excType[i] = ClassifyExcitation(st->_stClass, vadLevel, i);
         }
      }

      if (Coder_lf_b(codec_mode, speech, wsynth, mod, AqLF, st->window, param,
         ol_gain, T_out, p_out, excType, fscale, st)) {
            return APIAMRWB_StsErr;
      }
   } else {
      for (i=0; i<4; i++) {
         excType[i] = 0;
      }

      if (Coder_lf(codec_mode, speech, wsynth, mod, AqLF, st->window, param,
         ol_gain, T_out, p_out, excType, fscale, st)) {
            return APIAMRWB_StsErr;
      }
   }

   for(i=0; i<4; i++) {
      mod[i] = excType[i];
   }
   st->prev_mod = mod[3];

   ippsCopy_16s(&old_speech[L_FRAME_PLUS], st->old_speech_pe, L_OLD_SPEECH_ST);
   ippsCopy_16s(&old_synth[L_FRAME_PLUS], st->old_synth, M);

   if (frameSize > L_FRAME8k) {
      Ipp16s old_speech_hf[L_TOTAL_ST];
      Ipp16s *new_speech_hf, *speech_hf_;

      new_speech_hf = old_speech_hf + L_OLD_SPEECH_ST;
      speech_hf_     = old_speech_hf + L_TOTAL_ST - L_FRAME_PLUS - L_A_MAX - L_BSP;

      ippsCopy_16s(st->right.old_speech_hf, old_speech_hf, L_OLD_SPEECH_ST);

      if (fscale == 0) {
         ippsDownsample_AMRWBE_16s(pChannelRight, frameSize, new_speech_hf, st->right.mem_decim_hf, 1);
      } else {
         ippsCopy_16s(pChannelRight, new_speech_hf, L_FRAME_PLUS);
      }

      ippsCopy_16s(&old_speech_hf[L_FRAME_PLUS], st->right.old_speech_hf, L_OLD_SPEECH_ST);
      scale_fac_hf = ownScaleOneChannel_16s(old_speech_hf, L_TOTAL_ST, &(st->right.Q_sp_hf), st->right.OldQ_sp_hf, 1);
      ownScaleSignal_AMRWB_16s_ISfs(st->right.mem_hf3, MHF, scale_fac_hf);

      Coder_hf(mod, AqLF, speech, speech_hf_, st->window, prm_hf_right, fscale, &(st->right),st->Q_new, st->Q_sp);
   } else {
      ippsZero_16s(prm_hf_right, (NB_DIV*NPRM_BWE_DIV));
   }

   nbits_pack = (tblNBitsMonoCore_16s[codec_mode] + NBITS_BWE) >> 2;

   for (k=0; k<NB_DIV; k++) {
    Int2bin(mod[k], 2, &serial[k*nbits_pack]);
   }
   Enc_prm(mod, codec_mode, param, serial, nbits_pack);
   Enc_prm_hf(mod, prm_hf_right, serial, nbits_pack);
   *pCountSamplesProceeded = cntSamples;
   return APIAMRWB_StsNoErr;
}


static APIAMRWB_Status Coder_lf_b(Ipp32s codec_mode, Ipp16s* speech, Ipp16s* synth,
      Ipp16s* mod, Ipp16s* AqLF, Ipp16s* wwindow, Ipp16s* param, Ipp16s* ol_gain,
      Ipp16s* ave_T_out, Ipp16s* ave_p_out, Ipp16s* coding_mod, Ipp32s pit_adj, EncoderObj_AMRWBE *st)
{
   IPP_ALIGNED_ARRAY(16, Ipp32s, pAutoCorr, (M+1)); /* Autocorrelations of windowed speech {r_h & r_l} */
   IPP_ALIGNED_ARRAY(16, Ipp16s, rc, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, A, ((NB_SUBFR_WBP+1)*(M+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Ap_, (M+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isp, ((NB_DIV+1)*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isp_q, ((NB_DIV+1)*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, past_isfq, (3*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, mem_syn, ((NB_DIV+1)*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wovlp, ((NB_DIV+1)*128));

   Ipp16s mem_w0[NB_DIV+1], mem_wsyn[NB_DIV+1];
   Ipp16s mem_xnq[NB_DIV+1];

   Ipp16s *prm;
   Ipp16s ovlp_size[NB_DIV+1];
   Ipp32s i, j, k, i2, i1, nbits;
   Ipp16s *wsp;
   Ipp16s snr, snr1, snr2;
   Ipp32s Lener, Lcor_max, Lt0;
   Ipp16s *p, *p1;
   Ipp16s norm_corr[4], norm_corr2[4];
   Ipp16s T_op[NB_DIV], T_op2[NB_DIV];
   Ipp16s T_out[4];
   Ipp16s p_out[4];
   Ipp16s LTPGain[2];
   Ipp16s PIT_min, PIT_max;
   Ipp16s tmp16;
   Ipp32s dwTmp;
   Ipp16s shift;
   Ipp16s max;

   if(pit_adj == 0) {
      st->i_offset = 0;
      PIT_min = PIT_MIN_12k8;
      PIT_max = PIT_MAX_12k8;
   } else {
      /*i = (((pit_adj*PIT_MIN_12k8)+(FSCALE_DENOM/2))/FSCALE_DENOM)-PIT_MIN_12k8;*/
      dwTmp = Add_32s(FSCALE_DENOM, Mul2_Low_32s(pit_adj*PIT_MIN_12k8));
      tmp16 = Div_16s((Ipp16s)(ShiftL_32s(dwTmp, 8)>>16), 24576);
      dwTmp = Sub_32s(dwTmp, Mul2_Low_32s((tmp16+1)*FSCALE_DENOM));
      tmp16 = (Ipp16s)(tmp16 - PIT_MIN_12k8);
      if (dwTmp >= 0) {
         tmp16 = (Ipp16s)(tmp16 + 1);
      }
      st->i_offset = tmp16;
      PIT_min = (Ipp16s)(PIT_MIN_12k8 + tmp16);
      PIT_max = (Ipp16s)(PIT_MAX_12k8 + (tmp16*6));
   }

   nbits = tblNBitsMonoCore_16s[codec_mode];
   nbits = nbits - NBITS_MODE;

   ippsCopy_16s(st->ispold, isp, M);
   for (i=0; i<NB_DIV; i++) {
      if (pit_adj <= FSCALE_DENOM) {
         ownAutoCorr_AMRWB_16s32s(&speech[(i*L_DIV)+L_SUBFR], M, pAutoCorr, wwindow, L_WINDOW_WBP);
      } else {
         ownAutoCorr_AMRWB_16s32s(&speech[(i*L_DIV)+(L_SUBFR/2)], M, pAutoCorr, wwindow, L_WINDOW_HIGH_RATE);
      }
      ownLagWindow_AMRWB_32s_I(pAutoCorr, M);
      if(ippsLevinsonDurbin_G729_32s16s(pAutoCorr, M, Ap_, rc, &tmp16) == ippStsOverflow){
         Ap_[0] = 4096;
         ippsCopy_16s(st->lev_mem, &Ap_[1], M);
         rc[0] = st->lev_mem[M];
         rc[1] = st->lev_mem[M+1];
      } else {
         ippsCopy_16s(Ap_, st->lev_mem, M+1);
         st->lev_mem[M] = rc[0];
         st->lev_mem[M+1] = rc[1];
      }

      ippsLPCToISP_AMRWBE_16s(Ap_, st->ispold, ispnew, M);

      for (j=0; j<M; j++) {
         st->_stClass->ApBuf[i*M+j] = Ap_[j];
      }

      ippsCopy_16s(ispnew, &isp[(i+1)*M], M);
      ownIntLPC(st->ispold, ispnew, tblInterpolTCX20_16s, &A[i*4*(M+1)], 4, M);
      ippsCopy_16s(ispnew, st->ispold, M);
   }

   ippsCopy_16s(&synth[-M], mem_syn, M);
   wsp = synth;
   {
      Ipp16s tmp_max, i_fr, exp_wsp, exp_t0, exp_cm, exp_ener;
      IPP_ALIGNED_ARRAY(16, Ipp16s, old_d_wsp, ((PIT_MAX_MAX/OPL_DECIM)+L_DIV));
      Ipp16s *d_wsp_;

      d_wsp_ = old_d_wsp + PIT_MAX_MAX/OPL_DECIM;
      ippsCopy_16s(st->old_d_wsp, old_d_wsp, PIT_MAX_MAX/OPL_DECIM);

      for (i = 0; i < NB_DIV; i++) {

         ownFindWSP(&A[i*(NB_SUBFR_WBP/4)*(M+1)], &speech[i*L_DIV], &wsp[i*L_DIV], &(st->mem_wsp), L_DIV);

         mem_w0[i+1] = st->mem_wsp;
         ippsCopy_16s(&wsp[i*L_DIV], d_wsp_, L_DIV);

         max = 0;
         ippsMaxAbs_16s(d_wsp_, L_DIV, &max);
         max = IPP_MAX(0, max);

         tmp_max = max;
         for (i_fr=0; i_fr<3; i_fr++) {
            tmp16 = st->old_wsp_max[i_fr];
            if (tmp_max > tmp16) {
               ;
            } else {
               tmp_max  = tmp16;
            }
            st->old_wsp_max[i_fr] = st->old_wsp_max[i_fr+1];
         }
         tmp16 = st->old_wsp_max[3];

         if (tmp_max > tmp16) {
            tmp16 = tmp_max;
         }

         st->old_wsp_max[3] = max;

         shift = (Ipp16s)(Exp_16s(tmp16) - 3);

         if (shift > 0) {
            shift = 0;
         }

         ownLPDecim2(d_wsp_, L_DIV, st->mem_lp_decim2);
         ownScaleSignal_AMRWB_16s_ISfs(d_wsp_, L_DIV / OPL_DECIM, shift);
         if (i == 0) {
            exp_wsp = (Ipp16s)(st->scale_fac + (shift - st->old_wsp_shift));
         } else {
            exp_wsp = (Ipp16s)(shift - st->old_wsp_shift);
         }

         st->old_wsp_shift = shift;

         if(exp_wsp != 0) {
            ownScaleSignal_AMRWB_16s_ISfs(old_d_wsp, PIT_MAX_MAX / OPL_DECIM, exp_wsp);
            ownScaleSignal_AMRWB_16s_ISfs(st->hp_old_wsp, PIT_MAX_MAX / OPL_DECIM, exp_wsp);
         }

         st->scaleHPOLLTP = (Ipp16s)(st->scaleHPOLLTP - exp_wsp);

         {
            Ipp16s* pHPWgtSpch = st->hp_old_wsp + (PIT_max/OPL_DECIM);
            ippsHighPassFilter_AMRWB_16s_Sfs(d_wsp_, pHPWgtSpch, (2*L_SUBFR)/OPL_DECIM, st->memHPOLLTP, st->scaleHPOLLTP);
            ippsOpenLoopPitchSearch_AMRWBE_16s(d_wsp_, pHPWgtSpch, &st->old_T0_med, &st->ada_w, &T_op[i],
               &tmp16, &(st->ol_gain), st->old_ol_lag, &st->ol_wght_flg,
               (2*L_SUBFR)/OPL_DECIM, (Ipp16s)((PIT_min/OPL_DECIM)+1), (Ipp16s)(PIT_max/OPL_DECIM));
            ippsMove_16s(&(st->hp_old_wsp[(2*L_SUBFR)/OPL_DECIM]), st->hp_old_wsp, (PIT_max/OPL_DECIM));
         }
         T_op[i] = (Ipp16s)(T_op[i] >> 1);
         LTPGain[1] = st->ol_gain;

         Lcor_max =0;
         Lt0 = 0;
         Lener = 0;

         p = &d_wsp_[0];
         p1 = d_wsp_ - T_op[i];
         for(j=0; j<L_SUBFR; j++) {
            Lcor_max  = Add_32s(Lcor_max, Mul2_Low_32s(p[j] * p1[j]));
            Lt0 = Add_32s(Lt0, Mul2_Low_32s(p1[j] * p1[j]));
            Lener = Add_32s(Lener, Mul2_Low_32s(p[j] * p[j]));
         }

         if (Lt0 == 0) {
            Lt0 = 1;
         }
         if (Lener == 0) {
            Lener = 1;
         }

         exp_cm = Norm_32s_I(&Lcor_max);
         exp_t0 = Norm_32s_I(&Lt0);
         exp_ener = Norm_32s_I(&Lener);
         Lt0 = Mul2_Low_32s(Cnvrt_NR_32s16s(Lt0) * Cnvrt_NR_32s16s(Lener));
         exp_t0 = (Ipp16s)(62 - (exp_t0 + exp_ener + Norm_32s_I(&Lt0)));

         InvSqrt_32s16s_I(&Lt0, &exp_t0);

         Lcor_max = Mul2_Low_32s(Cnvrt_NR_32s16s(Lcor_max) * Cnvrt_NR_32s16s(Lt0));
         exp_cm = (Ipp16s)(31 - exp_cm + exp_t0);

         if (exp_cm > 0) {
            norm_corr[i]  = Cnvrt_NR_32s16s(ShiftL_32s(Lcor_max, exp_cm));
         } else {
            norm_corr[i]  = Cnvrt_NR_32s16s(Lcor_max >> (-exp_cm));
         }

         {
            Ipp16s* pHPWgtSpch = st->hp_old_wsp + (PIT_max/OPL_DECIM);
            ippsHighPassFilter_AMRWB_16s_Sfs(d_wsp_+((2*L_SUBFR)/OPL_DECIM), pHPWgtSpch,
               (2*L_SUBFR)/OPL_DECIM, st->memHPOLLTP, st->scaleHPOLLTP);
            ippsOpenLoopPitchSearch_AMRWBE_16s(d_wsp_+((2*L_SUBFR)/OPL_DECIM), pHPWgtSpch, &st->old_T0_med,
               &st->ada_w, &T_op2[i], &tmp16, &(st->ol_gain), st->old_ol_lag, &st->ol_wght_flg,
               (2*L_SUBFR)/OPL_DECIM, (Ipp16s)((PIT_min/OPL_DECIM)+1), (Ipp16s)(PIT_max/OPL_DECIM));
            ippsMove_16s(&(st->hp_old_wsp[(2*L_SUBFR)/OPL_DECIM]), st->hp_old_wsp, (PIT_max/OPL_DECIM));
         }
         T_op2[i] = (Ipp16s)(T_op2[i] >> 1);
         LTPGain[0] = st->ol_gain;

         Lcor_max =0;
         Lt0 = 0;
         Lener = 0;

         p = d_wsp_ + L_SUBFR;
         p1 = d_wsp_ + L_SUBFR - T_op2[i];
         for(j=0; j<L_SUBFR; j++) {
            Lcor_max  = Add_32s(Lcor_max, Mul2_Low_32s(p[j] * p1[j]));
            Lt0 = Add_32s(Lt0, Mul2_Low_32s(p1[j] * p1[j]));
            Lener = Add_32s(Lener, Mul2_Low_32s(p[j] * p[j]));
         }

         if (Lt0 == 0)
            Lt0 = 1;

         if (Lener == 0)
            Lener = 1;

         exp_cm = Norm_32s_I(&Lcor_max);
         exp_t0 = Norm_32s_I(&Lt0);
         exp_ener = Norm_32s_I(&Lener);
         Lt0 = Mul2_Low_32s(Cnvrt_NR_32s16s(Lt0) * Cnvrt_NR_32s16s(Lener));
         exp_t0 = (Ipp16s)(62 - (exp_t0 + exp_ener + Norm_32s_I(&Lt0)));

         InvSqrt_32s16s_I(&Lt0, &exp_t0);

         Lcor_max = Mul2_Low_32s(Cnvrt_NR_32s16s(Lcor_max) * Cnvrt_NR_32s16s(Lt0));
         exp_cm = (Ipp16s)(31 - exp_cm + exp_t0);

         if (exp_cm > 0) {
            norm_corr2[i]  = Cnvrt_NR_32s16s(ShiftL_32s(Lcor_max, exp_cm));
         } else {
            norm_corr2[i]  = Cnvrt_NR_32s16s(Lcor_max >> (-exp_cm));
         }

         ol_gain[i] = st->ol_gain;

         ippsMove_16s(&old_d_wsp[L_DIV/OPL_DECIM], old_d_wsp, PIT_MAX_MAX/OPL_DECIM);

         st->_stClass->LTPGain[(2*i+2)] = LTPGain[1];
         st->_stClass->LTPGain[(2*i+2)+1] = LTPGain[0];
         st->_stClass->LTPLag[(2*i)+2] = T_op[i];
         st->_stClass->LTPLag[(2*i+2)+1] = T_op2[i];
         st->_stClass->NormCorr[(2*i+2)] = norm_corr[i];
         st->_stClass->NormCorr[(2*i+2)+1] = norm_corr2[i];
      }

      ippsCopy_16s(old_d_wsp, st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
   }

   ovlp_size[0] = st->old_ovlp_size;
   ClassifyExcitationRef(st->_stClass, isp, coding_mod);

   if (st->SwitchFlagPlusToWB && coding_mod[0] != 0) {
      coding_mod[0] = 0;
      st->_stClass->NbOfAcelps = Add_16s(st->_stClass->NbOfAcelps,1);
      st->SwitchFlagPlusToWB = 0;
   }

   ovlp_size[1] = st->old_ovlp_size;
   ovlp_size[2] = st->old_ovlp_size;
   ovlp_size[3] = st->old_ovlp_size;
   ovlp_size[4] = st->old_ovlp_size;

   mem_w0[0]   = st->old_mem_w0;
   mem_xnq[0]   = st->old_mem_xnq;
   mem_wsyn[0] = st->old_mem_wsyn;

   ippsCopy_16s(st->old_wovlp, wovlp, 128);
   ippsCopy_16s(st->past_isfq, &past_isfq[0], M);
   ippsCopy_16s(st->ispold_q, isp_q, M);

   snr2 = 0;
   for (i1=0; i1<2; i1++) {
      ippsMove_16s(&past_isfq[i1*M], &past_isfq[(i1+1)*M], M);

      snr1 = 0;
      for (i2=0; i2<2; i2++) {
         k = (i1<<1) + i2;

         if ((coding_mod[k]==0) || ((coding_mod[k]==1) && (st->_stClass->NbOfAcelps!=0))
            || ((st->_stClass->NoMtcx[i1]!=0) && (st->_stClass->NbOfAcelps==0))  ) {
            prm = param + (k*NPRM_DIV);

            ippsISPToISF_Norm_AMRWB_16s(&isp[(k+1)*M], isfnew, M);

            {
               IppSpchBitRate ippTmpRate = IPP_SPCHBR_8850;/* NOT (6600 & DTX)*/
               ippsISFQuant_AMRWB_16s(isfnew, &past_isfq[(i1+1)*M], isfnew, prm, ippTmpRate);
            }
            prm += NPRM_LPC;

            ippsISFToISP_AMRWB_16s(isfnew, &isp_q[(k+1)*M], M);
            ownIntLPC(&isp_q[k*M], &isp_q[(k+1)*M], tblInterpolTCX20_16s, &AqLF[k*4*(M+1)], 4, M);

            if (coding_mod[k] == 0) {

               mem_xnq[k+1] = mem_xnq[k];
               ovlp_size[k+1] = 0;
               {
                  IPP_ALIGNED_ARRAY(16, Ipp16s, old_exc, (PIT_MAX_MAX+L_INTERPOL+L_DIV+1));
                  IPP_ALIGNED_ARRAY(16, Ipp16s, old_syn, (M+L_DIV));
                  IPP_ALIGNED_ARRAY(16, Ipp16s, buf, (L_DIV));

                  ippsCopy_16s(st->old_exc, old_exc, PIT_MAX_MAX+L_INTERPOL);
                  ippsCopy_16s(&mem_syn[k*M], old_syn, M);

                  if (Coder_acelp(&A[k*(NB_SUBFR_WBP/4)*(M+1)], &AqLF[k*(NB_SUBFR_WBP/4)*(M+1)],
                        &speech[k*L_DIV], &mem_xnq[k+1], old_syn+M, old_exc+PIT_MAX_MAX+L_INTERPOL,
                        &wovlp[(k+1)*128], L_DIV, codec_mode, norm_corr[k], norm_corr2[k],
                        T_op[k], T_op2[k], T_out, p_out, st->mem_gain_code, prm, &wsp[k*L_DIV], st)) {
                           return APIAMRWB_StsErr;
                  }
                  ippsCopy_16s(&old_exc[L_DIV], st->old_exc, PIT_MAX_MAX+L_INTERPOL);
                  ippsCopy_16s(&old_syn[L_DIV], &mem_syn[(k+1)*M], M);

                  ave_T_out[k] = Add_16s(T_op[k],T_op2[k]);

                  dwTmp = Mul2_Low_32s(p_out[0] * 8192);
                  dwTmp = Add_32s(dwTmp, Mul2_Low_32s(p_out[1]*8192));
                  dwTmp = Add_32s(dwTmp, Mul2_Low_32s(p_out[2]*8192));
                  ave_p_out[k] = Cnvrt_NR_32s16s(Add_32s(dwTmp, Mul2_Low_32s(p_out[3]*8192)));
                  mem_wsyn[k+1] = mem_wsyn[k];
                  ownFindWSP(&A[k*(NB_SUBFR_WBP/4)*(M+1)], &old_syn[M], buf, &mem_wsyn[k+1], L_DIV);
                  st->LastQMode = 0;
               }
               mod[k] = 0;
               coding_mod[k] = 0;
            }

            if (coding_mod[k] != 0 ) {
               snr = -32760;
               Try_tcx(k, 1, &snr, A, wsp, mod, coding_mod, isp, isp_q,
                  AqLF, speech, mem_w0, mem_xnq, mem_wsyn, st->old_exc,
                  mem_syn, wovlp, ovlp_size, past_isfq,
                  ((nbits>>2)-NBITS_LPC), NPRM_TCX20, prm, st);
               snr1 = Add_16s(snr1, (Ipp16s)(snr>>1));
            }
         }
      }

      if (coding_mod[i1*2] != 0 && coding_mod[i1*2+1] != 0) {

         if (st->_stClass->NbOfAcelps == 0)
            snr1 = -32760;

         k = i1 << 1;

         prm = param + (k*NPRM_DIV);

         Try_tcx(k, 2, &snr1, A, wsp, mod, coding_mod, isp, isp_q,
            AqLF, speech, mem_w0, mem_xnq, mem_wsyn, st->old_exc,
            mem_syn, wovlp, ovlp_size, past_isfq,
            ((nbits>>1)-NBITS_LPC), NPRM_LPC+NPRM_TCX40, prm, st);

         snr2 = Add_16s(snr2, (Ipp16s)(snr1>>1));
      }
   }

   if (coding_mod[0] != 0 &&  coding_mod[1] != 0 &&  coding_mod[2] != 0 &&  coding_mod[3] != 0 &&
            st->_stClass->NoMtcx[0] == 0 &&  st->_stClass->NoMtcx[1] == 0) {
      k = 0;
      prm = param + (k*NPRM_DIV);
      Try_tcx(k, 3, &snr2, A, wsp, mod, coding_mod, isp, isp_q,
         AqLF, speech, mem_w0, mem_xnq, mem_wsyn, st->old_exc,
         mem_syn, wovlp, ovlp_size, past_isfq,
         (nbits-NBITS_LPC), NPRM_LPC+NPRM_TCX80, prm, st);
   }

   for(i=0; i<4; i++)
      st->_stClass->prevModes[i] = coding_mod[i];

   if (st->_stClass->NoMtcx[0] != 0)
      st->_stClass->NoMtcx[0] = 0;

   if (st->_stClass->NoMtcx[1] != 0)
      st->_stClass->NoMtcx[1] = 0;

   st->old_ovlp_size = ovlp_size[NB_DIV];

   st->old_mem_w0 = mem_w0[NB_DIV];
   st->old_mem_xnq = mem_xnq[NB_DIV];
   st->old_mem_wsyn = mem_wsyn[NB_DIV];

   ippsCopy_16s(&wovlp[NB_DIV*128], st->old_wovlp, 128);
   ippsCopy_16s(&past_isfq[2*M], st->past_isfq, M);
   ippsCopy_16s(&isp_q[NB_DIV*M], st->ispold_q, M);
   ippsCopy_16s(&mem_syn[NB_DIV*M], synth+L_FRAME_PLUS-M, M);

   return APIAMRWB_StsNoErr;
}

static APIAMRWB_Status Coder_lf(Ipp32s codec_mode, Ipp16s* speech, Ipp16s* synth, Ipp16s* mod,
      Ipp16s* AqLF, Ipp16s* window, Ipp16s* param, Ipp16s* ol_gain, Ipp16s* ave_T_out, Ipp16s* ave_p_out,
      Ipp16s* coding_mod, Ipp32s pit_adj, EncoderObj_AMRWBE *st)
{
   IPP_ALIGNED_ARRAY(16, Ipp32s, pAutoCorr, (M+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, A, ((NB_SUBFR_WBP+1)*(M+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Aq, ((NB_SUBFR_WBP+1)*(M+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, rc, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew_q, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isp, ((NB_DIV+1)*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isp_q, ((NB_DIV+1)*M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, past_isfq_1, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, past_isfq_2, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Ap, (M+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_tcx, (NPRM_LPC+NPRM_TCX80));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_tcx, (M+L_TCX));
   IPP_ALIGNED_ARRAY(16, Ipp16s, exc_tcx, (L_TCX));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wsp, (L_FRAME_PLUS));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wovlp, ((NB_DIV+1)*128));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wovlp_tcx, (128));
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_d_wsp, ((PIT_MAX_MAX/OPL_DECIM)+L_DIV));
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_exc, (PIT_MAX_MAX+L_INTERPOL+L_FRAME_PLUS+1));

   Ipp16s mem_w0[NB_DIV+1], mem_wsyn[NB_DIV+1];
   Ipp16s mem_xnq[NB_DIV+1];
   Ipp16s ovlp_size[NB_DIV+1];


   Ipp16s mem_w0_tcx, mem_xnq_tcx, mem_wsyn_tcx;
   Ipp16s *d_wsp, *exc, *prm;

   Ipp32s i, j, k, i2, i1, nbits, i_fr;
   Ipp16s tmp_max, exp_wsp, exp_t0, exp_cm, exp_ener;
   Ipp16s snr, snr1, snr2;
   Ipp16s tmp;

   Ipp16s *p, *p1,exp_dwn;
   Ipp16s norm_corr[4], norm_corr2[4];
   Ipp16s T_op[NB_DIV], T_op2[NB_DIV];
   Ipp16s T_out[4];
   Ipp16s p_out[4];
   Ipp16s PIT_min, PIT_max;
   Ipp32s Lener, Lcor_max, Lt0, dwTmp;
   Ipp16s tmp16, shift, max;

   if(pit_adj == 0) {
      st->i_offset = 0;
      PIT_min = PIT_MIN_12k8;
      PIT_max = PIT_MAX_12k8;
   } else {
      dwTmp = Add_32s(FSCALE_DENOM, Mul2_Low_32s(pit_adj*PIT_MIN_12k8));
      tmp16 = Div_16s((Ipp16s)(ShiftL_32s(dwTmp, 8)>>16), 24576);
      dwTmp = Sub_32s(dwTmp, Mul2_Low_32s((tmp16+1)*FSCALE_DENOM));
      tmp16 = (Ipp16s)(tmp16 - PIT_MIN_12k8);
      if (dwTmp >= 0) {
         tmp16 = (Ipp16s)(tmp16 + 1);
      }
      st->i_offset = tmp16;

      PIT_min = (Ipp16s)(PIT_MIN_12k8 + tmp16);
      PIT_max = (Ipp16s)(PIT_MAX_12k8 + (tmp16*6));
   }

    d_wsp = old_d_wsp + PIT_MAX_MAX/OPL_DECIM;
   exc   = old_exc   + PIT_MAX_MAX + L_INTERPOL;

   ippsCopy_16s(st->old_d_wsp, old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
   ippsCopy_16s(st->old_exc, old_exc, PIT_MAX_MAX+L_INTERPOL);

   nbits = tblNBitsMonoCore_16s[codec_mode];
   nbits = nbits - NBITS_MODE;

   ippsCopy_16s(st->ispold, isp, M);
   for (i=0; i<NB_DIV; i++) {
      if (pit_adj <= FSCALE_DENOM) {
         ownAutoCorr_AMRWB_16s32s(&speech[(i*L_DIV)+L_SUBFR], M, pAutoCorr, window, L_WINDOW_WBP);
      } else {
         ownAutoCorr_AMRWB_16s32s(&speech[(i*L_DIV)+(L_SUBFR/2)], M, pAutoCorr, window, L_WINDOW_HIGH_RATE);
      }
      ownLagWindow_AMRWB_32s_I(pAutoCorr, M);
      if(ippsLevinsonDurbin_G729_32s16s(pAutoCorr, M, Ap, rc, &tmp) == ippStsOverflow) {
         Ap[0] = 4096;
         ippsCopy_16s(st->lev_mem, &Ap[1], M);
         rc[0] = st->lev_mem[M];
         rc[1] = st->lev_mem[M+1];
      } else {
         ippsCopy_16s(Ap, st->lev_mem, M+1);
         st->lev_mem[M] = rc[0];
         st->lev_mem[M+1] = rc[1];
      }

      ippsLPCToISP_AMRWBE_16s(Ap, st->ispold, ispnew, M);
      ippsCopy_16s(ispnew, &isp[(i+1)*M], M);
      ownIntLPC(st->ispold, ispnew, tblInterpolTCX20_16s, &A[i*4*(M+1)], 4, M);
      ippsCopy_16s(ispnew, st->ispold, M);
   }

   for (i = 0; i < NB_DIV; i++) {

      ownFindWSP(&A[i*(NB_SUBFR_WBP/4)*(M+1)], &speech[i*L_DIV], &wsp[i*L_DIV], &(st->mem_wsp), L_DIV);
      mem_w0[i+1] = st->mem_wsp;

      ippsCopy_16s(&wsp[i*L_DIV], d_wsp, L_DIV);

      max = 0;
      ippsMaxAbs_16s(d_wsp, L_DIV, &max);
      max = IPP_MAX(0, max);

      tmp_max = max;
      for(i_fr=0; i_fr<3; i_fr++) {
         tmp16 = st->old_wsp_max[i_fr];
         if (tmp_max > tmp16)
            ;
         else
            tmp_max  = tmp16;
         st->old_wsp_max[i_fr] = st->old_wsp_max[i_fr+1] ;
      }

      tmp16 = st->old_wsp_max[3];

      if (tmp_max > tmp16)
         tmp16 = tmp_max ;

      st->old_wsp_max[3] = max;
      shift = (Ipp16s)(Exp_16s(tmp16) - 3);

      if (shift > 0)
         shift = 0;

      ownLPDecim2(d_wsp, L_DIV, st->mem_lp_decim2);
      ownScaleSignal_AMRWB_16s_ISfs(d_wsp, L_DIV / OPL_DECIM, shift);

      if (i == 0)
         exp_wsp = (Ipp16s)(st->scale_fac + (shift - st->old_wsp_shift));
      else
         exp_wsp = (Ipp16s)(shift - st->old_wsp_shift);

      st->old_wsp_shift = shift;

      if(exp_wsp != 0) {
         ownScaleSignal_AMRWB_16s_ISfs(old_d_wsp, PIT_MAX_MAX / OPL_DECIM, exp_wsp);
         ownScaleSignal_AMRWB_16s_ISfs(st->hp_old_wsp, PIT_MAX_MAX / OPL_DECIM, exp_wsp);
      }

      st->scaleHPOLLTP = (Ipp16s)(st->scaleHPOLLTP - exp_wsp);

      {
         Ipp16s* pHPWgtSpch = st->hp_old_wsp + (PIT_max>>1);
         ippsHighPassFilter_AMRWB_16s_Sfs(d_wsp, pHPWgtSpch, (2*L_SUBFR)/OPL_DECIM, st->memHPOLLTP, st->scaleHPOLLTP);
         ippsOpenLoopPitchSearch_AMRWBE_16s(d_wsp, pHPWgtSpch, &st->old_T0_med,
            &st->ada_w, &T_op[i], &tmp16, &(st->ol_gain), st->old_ol_lag, &st->ol_wght_flg,
            (2*L_SUBFR)/OPL_DECIM, (Ipp16s)((PIT_min/OPL_DECIM)+1), (Ipp16s)(PIT_max/OPL_DECIM));
         ippsMove_16s(&(st->hp_old_wsp[(2*L_SUBFR)/OPL_DECIM]), st->hp_old_wsp, (PIT_max>>1));
      }
      T_op[i] = (Ipp16s)(T_op[i] >> 1);

      Lcor_max =0;
      Lt0 = 0;
      Lener = 0;

      p = &d_wsp[0];
      p1 = d_wsp - T_op[i];
      for(j=0; j<L_SUBFR; j++) {
         Lcor_max  = Add_32s(Lcor_max, Mul2_Low_32s(p[j] * p1[j]));
         Lt0 = Add_32s(Lt0, Mul2_Low_32s(p1[j] * p1[j]));
         Lener = Add_32s(Lener, Mul2_Low_32s(p[j] * p[j]));
      }
      if (Lt0 == 0) {
         Lt0 = 1;
      }
      if (Lener == 0) {
         Lener = 1;
      }

      exp_cm = Norm_32s_I(&Lcor_max);
      exp_t0 = Norm_32s_I(&Lt0);
      exp_ener = Norm_32s_I(&Lener);
      Lt0 = Mul2_Low_32s(Cnvrt_NR_32s16s(Lt0) * Cnvrt_NR_32s16s(Lener));
      exp_t0 = (Ipp16s)(62 - (exp_t0 + exp_ener + Norm_32s_I(&Lt0)));

      InvSqrt_32s16s_I(&Lt0, &exp_t0);

      Lcor_max = Mul2_Low_32s(Cnvrt_NR_32s16s(Lcor_max) * Cnvrt_NR_32s16s(Lt0));
      exp_cm = (Ipp16s)(31 - exp_cm + exp_t0);

      if (exp_cm > 0) {
         norm_corr[i]  = Cnvrt_NR_32s16s(ShiftL_32s(Lcor_max, exp_cm));
      } else {
         norm_corr[i]  = Cnvrt_NR_32s16s(Lcor_max >> (-exp_cm));
      }

      {
         Ipp16s* pHPWgtSpch = st->hp_old_wsp + (PIT_max>>1);
         ippsHighPassFilter_AMRWB_16s_Sfs(d_wsp+((2*L_SUBFR)/OPL_DECIM), pHPWgtSpch,
            (2*L_SUBFR)/OPL_DECIM, st->memHPOLLTP, st->scaleHPOLLTP);
         ippsOpenLoopPitchSearch_AMRWBE_16s(d_wsp+((2*L_SUBFR)/OPL_DECIM), pHPWgtSpch, &st->old_T0_med,
            &st->ada_w, &T_op2[i], &tmp16, &(st->ol_gain), st->old_ol_lag, &st->ol_wght_flg,
            (2*L_SUBFR)/OPL_DECIM, (Ipp16s)((PIT_min/OPL_DECIM)+1), (Ipp16s)(PIT_max/OPL_DECIM));
         ippsMove_16s(&(st->hp_old_wsp[(2*L_SUBFR)/OPL_DECIM]), st->hp_old_wsp, (PIT_max>>1));
      }
      T_op2[i] = (Ipp16s)(T_op2[i] >> 1);

      Lcor_max =0;
      Lt0 = 0;
      Lener = 0;

      p = d_wsp + L_SUBFR;
      p1 = d_wsp + L_SUBFR - T_op2[i];
      for(j=0; j<L_SUBFR; j++) {
         Lcor_max  = Add_32s(Lcor_max, Mul2_Low_32s(p[j] * p1[j]));
         Lt0 = Add_32s(Lt0, Mul2_Low_32s(p1[j] * p1[j]));
         Lener = Add_32s(Lener, Mul2_Low_32s(p[j] * p[j]));
      }

      if (Lt0 == 0)
         Lt0 = 1;
      if (Lener == 0)
         Lener = 1;

      exp_cm = Norm_32s_I(&Lcor_max);
      exp_t0 = Norm_32s_I(&Lt0);
      exp_ener = Norm_32s_I(&Lener);
      Lt0 = Mul2_Low_32s(Cnvrt_NR_32s16s(Lt0) * Cnvrt_NR_32s16s(Lener));
      exp_t0 = (Ipp16s)(62 - (exp_t0 + exp_ener + Norm_32s_I(&Lt0)));

      InvSqrt_32s16s_I(&Lt0, &exp_t0);

      Lcor_max = Mul2_Low_32s(Cnvrt_NR_32s16s(Lcor_max) * Cnvrt_NR_32s16s(Lt0));
      exp_cm = (Ipp16s)(31 - exp_cm + exp_t0);

      if (exp_cm > 0) {
         norm_corr2[i]  = Cnvrt_NR_32s16s(ShiftL_32s(Lcor_max, exp_cm));
      } else {
         norm_corr2[i]  = Cnvrt_NR_32s16s(Lcor_max >> (-exp_cm));
      }

      ol_gain[i] = st->ol_gain;

      ippsMove_16s(&old_d_wsp[L_DIV/OPL_DECIM], old_d_wsp, PIT_MAX_MAX/OPL_DECIM);
   }
   ippsCopy_16s(old_d_wsp, st->old_d_wsp, PIT_MAX_MAX/OPL_DECIM);

   ovlp_size[0] = st->old_ovlp_size;
   mem_w0[0]   = st->old_mem_w0;
   mem_xnq[0]   = st->old_mem_xnq;
   mem_wsyn[0] = st->old_mem_wsyn;

   ippsCopy_16s(st->old_wovlp, wovlp, 128);
   ippsCopy_16s(st->past_isfq, past_isfq_1, M);
   ippsCopy_16s(st->ispold_q, isp_q, M);

   snr2 = 0;

   for (i1=0; i1<2; i1++) {
      ippsCopy_16s(past_isfq_1, past_isfq_2, M);
      snr1 = 0;

      for (i2=0; i2<2; i2++) {
         k = (i1<<1) + i2;
         prm = param + (k*NPRM_DIV);
         ippsISPToISF_Norm_AMRWB_16s(&isp[(k+1)*M], isfnew, M);

         {
            IppSpchBitRate ippTmpRate = IPP_SPCHBR_8850;/* NOT (6600 & DTX)*/
            ippsISFQuant_AMRWB_16s(isfnew, past_isfq_1, isfnew, prm, ippTmpRate);
         }
         prm += NPRM_LPC;

         ippsISFToISP_AMRWB_16s(isfnew, &isp_q[(k+1)*M], M);
         ownIntLPC(&isp_q[k*M], &isp_q[(k+1)*M], tblInterpolTCX20_16s, Aq, 4, M);
         ippsCopy_16s(Aq, &AqLF[k*4*(M+1)], 5*(M+1));


         mem_xnq[k+1] = mem_xnq[k];
         ovlp_size[k+1] = 0;

         if (Coder_acelp(&A[k*(NB_SUBFR_WBP/4)*(M+1)], Aq, &speech[k*L_DIV],
               &mem_xnq[k+1], &synth[k*L_DIV], &exc[k*L_DIV], &wovlp[(k+1)*128],
               L_DIV, codec_mode, norm_corr[k], norm_corr2[k], T_op[k], T_op2[k],
               T_out, p_out, st->mem_gain_code, prm, &wsp[k*L_DIV], st) ) {
                  return APIAMRWB_StsErr;
         }

         ave_T_out[k] = Add_16s(T_op[k],T_op2[k]);
         dwTmp = Mul2_Low_32s(p_out[0] * 8192);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(p_out[1]*8192));
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(p_out[2]*8192));
         ave_p_out[k] = Cnvrt_NR_32s16s(Add_32s(dwTmp, Mul2_Low_32s(p_out[3]*8192)));
         mem_wsyn[k+1] = mem_wsyn[k];
         {
            IPP_ALIGNED_ARRAY(16, Ipp16s, buf, L_FRAME_PLUS);

            ownFindWSP(&A[k*(NB_SUBFR_WBP/4)*(M+1)], &synth[k*L_DIV], buf, &mem_wsyn[k+1], L_DIV);
            ippsSNR_AMRWBE_16s(&wsp[k*L_DIV], buf, L_DIV, L_SUBFR, &snr);

            st->LastQMode = 0;

            if (st->SwitchFlagPlusToWB > 0) {
                snr = 32767;
                st->SwitchFlagPlusToWB = 0;
             }
         }

         mod[k] = 0;
         coding_mod[k] = 0;

         ippsCopy_16s(&synth[(k*L_DIV)-M], synth_tcx, M);
         mem_w0_tcx = mem_w0[k];
         mem_xnq_tcx = mem_xnq[k];

         ippsCopy_16s(&wovlp[k*128], wovlp_tcx, 128);

         exp_dwn = Coder_tcx(Aq, &speech[k*L_DIV], &mem_w0_tcx, &mem_xnq_tcx,
            &synth_tcx[M], exc_tcx, wovlp_tcx, ovlp_size[k], L_DIV,
            ((nbits>>2)-NBITS_LPC), prm_tcx, st);

         mem_wsyn_tcx = mem_wsyn[k];

         {
             IPP_ALIGNED_ARRAY(16, Ipp16s, buf, L_FRAME_PLUS);
             ownFindWSP(&A[k*(NB_SUBFR_WBP/4)*(M+1)], &synth_tcx[M], buf, &mem_wsyn_tcx, L_DIV);
             ippsSNR_AMRWBE_16s(&wsp[k*L_DIV], buf, L_DIV, L_SUBFR, &tmp);
         }

         if (tmp > snr) {
           st->Q_exc = Add_16s(st->Q_sp,exp_dwn);
           st->LastQMode = 1;

           snr = tmp;
           mod[k] = 1;
           coding_mod[k] = 1;

           mem_w0[k+1] = mem_w0_tcx;
           mem_xnq[k+1] = mem_xnq_tcx;
           mem_wsyn[k+1] = mem_wsyn_tcx;

           ovlp_size[k+1] = 32;

           ippsCopy_16s(wovlp_tcx, &wovlp[(k+1)*128], 128);
           ippsCopy_16s(&synth_tcx[M], &synth[k*L_DIV], L_DIV);
           ippsCopy_16s(exc_tcx, &exc[k*L_DIV], L_DIV);
           ippsCopy_16s(prm_tcx, prm, NPRM_TCX20);

         }
         snr1 = Add_16s(snr1, (Ipp16s)(snr>>1));
      }

      k = i1 << 1;
      prm = param + (k*NPRM_DIV);

      ippsISPToISF_Norm_AMRWB_16s(&isp[(k+2)*M], isfnew, M);
      {
         IppSpchBitRate ippTmpRate = IPP_SPCHBR_8850; /* NOT (6600 & DTX)*/
         ippsISFQuant_AMRWB_16s(isfnew, past_isfq_2, isfnew, prm_tcx, ippTmpRate);
      }
      ippsISFToISP_AMRWB_16s(isfnew, ispnew_q, M);
      ownIntLPC(&isp_q[k*M], ispnew_q, tblInterpolTCX40_16s, Aq, (NB_SUBFR_WBP/2), M);

      ippsCopy_16s(&synth[(k*L_DIV)-M], synth_tcx, M);
      mem_w0_tcx = mem_w0[k];
      mem_xnq_tcx = mem_xnq[k];
      ippsCopy_16s(&wovlp[k*128], wovlp_tcx, 128);

       exp_dwn = Coder_tcx(Aq, &speech[k*L_DIV], &mem_w0_tcx, &mem_xnq_tcx,
          &synth_tcx[M], exc_tcx, wovlp_tcx, ovlp_size[k], 2*L_DIV,
          ((nbits>>1)-NBITS_LPC), prm_tcx+NPRM_LPC, st);

       mem_wsyn_tcx = mem_wsyn[k];

       {
          IPP_ALIGNED_ARRAY(16, Ipp16s, buf, L_FRAME_PLUS);
          ownFindWSP(&A[i1*(NB_SUBFR_WBP/2)*(M+1)], &synth_tcx[M], buf, &mem_wsyn_tcx, 2*L_DIV);
          ippsSNR_AMRWBE_16s(&wsp[k*L_DIV], buf, 2*L_DIV, L_SUBFR, &tmp);
       }

      if (tmp > snr1) {
         st->Q_exc = Add_16s(st->Q_sp,exp_dwn);
         st->LastQMode = 1;
         snr1 = tmp;

         mod[k] = mod[k+1] = 2;
         coding_mod[k] = coding_mod[k+1] = 2;

         ippsCopy_16s(ispnew_q, &isp_q[(k+2)*M], M);

         mem_w0[k+2] = mem_w0_tcx;
         mem_xnq[k+2] = mem_xnq_tcx;
         mem_wsyn[k+2] = mem_wsyn_tcx;

         ovlp_size[k+2] = 64;

         ippsCopy_16s(past_isfq_2, past_isfq_1, M);
         ippsCopy_16s(wovlp_tcx, &wovlp[(k+2)*128], 128);
         ippsCopy_16s(&synth_tcx[M], &synth[k*L_DIV], 2*L_DIV);
         ippsCopy_16s(exc_tcx, &exc[k*L_DIV], 2*L_DIV);
         ippsCopy_16s(prm_tcx, prm, NPRM_LPC+NPRM_TCX40);
         ippsCopy_16s(Aq, &AqLF[k*4*(M+1)], 9*(M+1));
      }
      snr2 = Add_16s(snr2, (Ipp16s)(snr1>>1));
   }

   k = 0;
   prm = param + (k*NPRM_DIV);
   ippsCopy_16s(st->past_isfq, past_isfq_2, M);

   ippsISPToISF_Norm_AMRWB_16s(&isp[(k+4)*M], isfnew, M);
   {
      IppSpchBitRate ippTmpRate = IPP_SPCHBR_8850;/* NOT (6600 & DTX)*/
      ippsISFQuant_AMRWB_16s(isfnew, past_isfq_2, isfnew, prm_tcx, ippTmpRate);
   }

   ippsISFToISP_AMRWB_16s(isfnew, ispnew_q, M);
   ownIntLPC(&isp_q[k*M], ispnew_q, tblInterpolTCX80_16s, Aq, NB_SUBFR_WBP, M);

   ippsCopy_16s(&synth[(k*L_DIV)-M], synth_tcx, M);
   mem_w0_tcx = mem_w0[k];
   mem_xnq_tcx = mem_xnq[k];
   ippsCopy_16s(&wovlp[k*128], wovlp_tcx, 128);

   exp_dwn = Coder_tcx(Aq, &speech[k*L_DIV], &mem_w0_tcx, &mem_xnq_tcx,
      &synth_tcx[M], exc_tcx, wovlp_tcx, ovlp_size[k], 4*L_DIV,
      (nbits-NBITS_LPC), prm_tcx+NPRM_LPC, st);

   mem_wsyn_tcx = mem_wsyn[k];
   {
      IPP_ALIGNED_ARRAY(16, Ipp16s, buf, (L_FRAME_PLUS));
      ownFindWSP(&A[0*(NB_SUBFR_WBP/2)*(M+1)], &synth_tcx[M], buf, &mem_wsyn_tcx, 4*L_DIV);
      ippsSNR_AMRWBE_16s(&wsp[k*L_DIV], buf, 4*L_DIV, L_SUBFR, &tmp);
   }

   if (tmp > snr2) {
      st->Q_exc = Add_16s(st->Q_sp,exp_dwn);
      st->LastQMode = 1;
      for (i=0; i<4; i++) {
         mod[k+i] = 3;
         coding_mod[k+i] = 3;
      }

      ippsCopy_16s(ispnew_q, &isp_q[(k+4)*M], M);

      mem_w0[k+4] = mem_w0_tcx;
      mem_xnq[k+4] = mem_xnq_tcx;
      mem_wsyn[k+4] = mem_wsyn_tcx;

      ovlp_size[k+4] = 128;

      ippsCopy_16s(past_isfq_2, past_isfq_1, M);
      ippsCopy_16s(wovlp_tcx, &wovlp[(k+4)*128], 128);
      ippsCopy_16s(&synth_tcx[M], &synth[k*L_DIV], 4*L_DIV);
      ippsCopy_16s(exc_tcx, &exc[k*L_DIV], 4*L_DIV);
      ippsCopy_16s(prm_tcx, prm, NPRM_LPC+NPRM_TCX80);
      ippsCopy_16s(Aq, &AqLF[k*4*(M+1)], 17*(M+1));
   }
   st->old_ovlp_size = ovlp_size[NB_DIV];
   st->old_mem_w0 = mem_w0[NB_DIV];
   st->old_mem_xnq = mem_xnq[NB_DIV];
   st->old_mem_wsyn = mem_wsyn[NB_DIV];

   ippsCopy_16s(&wovlp[4*128], st->old_wovlp, 128);
   ippsCopy_16s(past_isfq_1, st->past_isfq, M);
   ippsCopy_16s(&isp_q[NB_DIV*M], st->ispold_q, M);
   ippsCopy_16s(&old_exc[L_FRAME_PLUS], st->old_exc, PIT_MAX_MAX+L_INTERPOL);
   return APIAMRWB_StsNoErr;
}

void Coder_hf(Ipp16s* mod, Ipp16s* AqLF, Ipp16s* speech, Ipp16s* speech_hf, Ipp16s* window,
      Ipp16s* param, Ipp32s fscale, EncoderOneChanelState_AMRWBE *st, Ipp16s Q_new, Ipp16s Q_speech)
{
   IPP_ALIGNED_ARRAY(16, Ipp32s, pAutoCorr, (MHF+1)); /* Autocorrelations of windowed speech {r_h & r_l} */
   Ipp16s *prm;

   IPP_ALIGNED_ARRAY(16, Ipp16s, A, ((NB_SUBFR_WBP+1)*(MHF+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Aq, ((NB_SUBFR_WBP+1)*(MHF+1)));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Ap, (MHF+1));
   Ipp16s *p_A, *p_Aq;

   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew, (NB_DIV*MHF));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew_q, (MHF));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew, (MHF));

   IPP_ALIGNED_ARRAY(16, Ipp16s, gain_hf, (NB_SUBFR_WBP));
   Ipp16s gain4_[4];
   IPP_ALIGNED_ARRAY(16, Ipp16s, HF, (L_SUBFR));
   IPP_ALIGNED_ARRAY(16, Ipp16s, buf, (MHF+L_SUBFR));
   IPP_ALIGNED_ARRAY(16, Ipp16s, rc, (M+2));
   Ipp16s gain, m_tmp, e_tmp, tmp16;
   const Ipp16s *interpol_wind;

   Ipp32s i, j, k, i_subfr, sf, nsf, mode;
   Ipp16s index;
   Ipp32s dwTmp, Lener;

   for (k=0; k<NB_DIV; k++) {
      if(fscale <= FSCALE_DENOM) {
         ownAutoCorr_AMRWB_16s32s(&speech_hf[(k*L_DIV)+L_SUBFR], MHF, pAutoCorr, window, L_WINDOW_WBP);
      } else {
         ownAutoCorr_AMRWB_16s32s(&speech_hf[(k*L_DIV)+(L_SUBFR/2)], MHF, pAutoCorr, window, L_WINDOW_HIGH_RATE);
      }
      ownLagWindow_AMRWB_32s_I(pAutoCorr, MHF);
      if(ippsLevinsonDurbin_G729_32s16s(pAutoCorr, MHF, Ap, rc, &tmp16) == ippStsOverflow) {
         Ap[0] = 4096;
         ippsCopy_16s(st->mem_lev_hf, &Ap[1], MHF);
         rc[0] = st->mem_lev_hf[MHF];
         rc[1] = st->mem_lev_hf[MHF+1];
      } else {
         ippsCopy_16s(Ap, st->mem_lev_hf, MHF+1);
         st->mem_lev_hf[MHF] = rc[0];
         st->mem_lev_hf[MHF+1] = rc[1];
      }

      ippsLPCToISP_AMRWBE_16s(Ap, st->ispold_hf, &ispnew[k*MHF], MHF);
      ownIntLPC(st->ispold_hf, &ispnew[k*MHF], tblInterpolTCX20_16s, &A[k*4*(MHF+1)], 4, MHF);
      ippsCopy_16s(&ispnew[k*MHF], st->ispold_hf, MHF);
   }

   p_A = A;
   for (k=0; k<NB_DIV; k++) {
      mode = mod[k];

      if ((mode == 0) || (mode==1)) {
         nsf = 4;
         interpol_wind = tblInterpolTCX20_16s;
      } else if (mode == 2) {
         nsf = 8;
         interpol_wind = tblInterpolTCX40_16s;
      } else {
         nsf = 16;
         interpol_wind = tblInterpolTCX80_16s;
      }

      prm = param + (k*NPRM_BWE_DIV);
      ippsISPToISF_Norm_AMRWB_16s(&ispnew[(k+((nsf>>2)-1))*MHF], isfnew, MHF);
      ippsISFQuantHighBand_AMRWBE_16s(isfnew, st->past_q_isf_hf, isfnew, prm, fscale);
      prm += 2;

      ippsISFToISP_AMRWB_16s(isfnew, ispnew_q, MHF);
      ownIntLPC(st->ispold_q_hf, ispnew_q, interpol_wind, Aq, nsf, MHF);
      ippsCopy_16s(ispnew_q, st->ispold_q_hf, MHF);

      gain = Match_gain_6k4(&AqLF[((k*4)+nsf)*(M+1)], &Aq[nsf*(MHF+1)]);
      Int_gain(st->old_gain, gain, interpol_wind, gain_hf, nsf);
      st->old_gain = gain;

      p_Aq = Aq;
      for (sf=0; sf<nsf; sf++) {
         i_subfr = (((k<<2) + sf) << 6);
         ippsResidualFilter_Low_16s_Sfs(&AqLF[((k*4)+sf)*(M+1)], M, &speech[i_subfr], buf+MHF, L_SUBFR, 12);
         ippsCopy_16s(st->mem_hf1, buf, MHF);

         tmp16 = p_Aq[0];
         p_Aq[0] >>= 3;
         ippsSynthesisFilter_G729E_16s_I(p_Aq, MHF, buf+MHF, L_SUBFR, st->mem_hf1);
         ippsCopy_16s(&buf[L_SUBFR], st->mem_hf1, MHF);
         p_Aq[0] = tmp16;

         ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA_HF_FX, Ap, (MHF+1), 15);
         ippsResidualFilter_Low_16s_Sfs(p_A, MHF, buf+MHF, HF, L_SUBFR, 11);

         tmp16 = Ap[0];
         Ap[0] >>= 1;
         ippsSynthesisFilter_G729E_16s_I(Ap, MHF, HF, L_SUBFR, st->mem_hf2);
         ippsCopy_16s(&HF[L_SUBFR-MHF], st->mem_hf2, MHF);
         Ap[0] = tmp16;

         ippsResidualFilter_Low_16s_Sfs(p_A, MHF, &speech_hf[i_subfr], buf, L_SUBFR, 12);

         tmp16 = Ap[0];
         Ap[0] >>= 3;
         ippsSynthesisFilter_G729E_16s_I(Ap, MHF, buf, L_SUBFR, st->mem_hf3);
         ippsCopy_16s(&buf[L_SUBFR-MHF], st->mem_hf3, MHF);
         Ap[0] = tmp16;

         dwTmp = 0;
         Lener = 0;
         for (i=0; i<L_SUBFR; i++) {
            Lener = Add_32s(Lener, Mul2_Low_32s(buf[i]*buf[i]));
            dwTmp = Add_32s(dwTmp, Mul2_Low_32s(HF[i]*HF[i]));
         }

         if(dwTmp == 0)
            dwTmp = 1;

         if(Lener== 0)
            Lener= 1;

         ownLog2_AMRWBE(Lener,&e_tmp,&m_tmp);
         e_tmp = (Ipp16s)(e_tmp - ((st->Q_sp_hf + Q_new) << 1));
         Lener = Mpy_32_16(e_tmp, m_tmp, LG10);

         ownLog2_AMRWBE(dwTmp,&e_tmp,&m_tmp);
         e_tmp = (Ipp16s)(e_tmp - (Q_speech<<1));
         dwTmp = Mpy_32_16(e_tmp, m_tmp, LG10);
         Lener = Sub_32s(Lener, dwTmp);

         gain = (Ipp16s)(ShiftL_32s(Lener, 10)>>16);
         gain_hf[sf] = Sub_16s(gain,gain_hf[sf]);

         p_A += (MHF+1);
         p_Aq += (MHF+1);
      }

      if (mode == 3) {
         for (i=0; i<4; i++) {
            dwTmp = 0;
            for (j=0; j<4; j++) {
               dwTmp = Add_32s(dwTmp, Mul2_Low_32s(gain_hf[(4*i)+j]*8192));
            }
            gain4_[i] = Cnvrt_NR_32s16s(dwTmp);
         }
         Q_gain_hf(gain4_, gain4_, prm);
         prm++;

         for (i=0; i<nsf; i++) {
            gain = gain4_[i>>2];

            tmp16 = Sub_16s(gain_hf[i],gain);
            dwTmp  = Add_32s(8388608, Mul2_Low_32s(Add_16s(2688,tmp16)*10923));
            index = (Ipp16s)((dwTmp>>8) >> 16);

            if (index > 7) {
               index = 7;
            } else if (index < 0) {
               index = 0;
            }
            prm[i] = index;

            gain_hf[i] = Sub_16s(Add_16s(gain, (Ipp16s)(index + (index<<1))),2688);
         }
      } else if (mode == 2) {

         for (i=0; i<4; i++) {
            dwTmp = 0;
            for (j=0; j<2; j++) {
               dwTmp = Add_32s(dwTmp, Mul2_Low_32s(gain_hf[(2*i)+j]*16384));
            }
            gain4_[i] = Cnvrt_NR_32s16s(dwTmp);
         }

         Q_gain_hf(gain4_, gain4_, prm);
         prm++;
         for (i=0; i<nsf; i++) {
            gain = gain4_[i>>1];

            tmp16 = Sub_16s(gain_hf[i], gain);
            dwTmp  = Add_32s(8388608, Mul2_Low_32s(Add_16s(1152,tmp16)*10923));
            index = (Ipp16s)((dwTmp>>8) >> 16);

            if (index > 3) {
               index = 3;
            } else if (index < 0) {
               index = 0;
            }
            prm[i] = index;
            gain_hf[i] = Sub_16s(Add_16s(gain, (Ipp16s)(index + (index<<1))),1152);
         }
      } else {
         Q_gain_hf(gain_hf, gain_hf,prm);
      }

      if (mode == 2) {
         k = k + 1;
      } else if (mode == 3) {
         k = k + 3;
      }
   }
}

static void Coder_stereo_x(Ipp16s *AqLF, Ipp32s brMode, Ipp16s *param, Ipp16s fscale,
      EncoderObj_AMRWBE *st, Ipp16s *wspeech_hi, Ipp16s *wchan_hi, Ipp16s *wspeech_2k, Ipp16s *wchan_2k)
{
   Ipp16s mod[4];

   if (tblNBitsStereoCore_16s[brMode] > (300+4) ) {
      st->filt_hi_pmsvq = &mspqFiltHighBand;
      st->gain_hi_pmsvq = &mspqGainHighBand;
   } else {
      st->filt_hi_pmsvq = &mspqFiltLowBand;
      st->gain_hi_pmsvq = &mspqGainLowBand;
   }

   Cod_hi_stereo(AqLF, param, st, wspeech_hi, wchan_hi);
   Cod_tcx_stereo(wspeech_2k, wchan_2k, &param[4+NPRM_STEREO_HI_X*NB_DIV], brMode, mod, fscale, st);

   param[NPRM_STEREO_HI_X*NB_DIV + 0] = mod[0];
   param[NPRM_STEREO_HI_X*NB_DIV + 1] = mod[1];
   param[NPRM_STEREO_HI_X*NB_DIV + 2] = mod[2];
   param[NPRM_STEREO_HI_X*NB_DIV + 3] = mod[3];
}

static APIAMRWB_Status Coder_acelp(Ipp16s* Az, Ipp16s* Azq, Ipp16s* speech, Ipp16s* mem_wsyn_,
      Ipp16s* synth_, Ipp16s* exc, Ipp16s* wovlp_, Ipp32s len, Ipp32s codec_mode,
      Ipp16s norm_corr, Ipp16s norm_corr2, Ipp16s T_op, Ipp16s T_op2, Ipp16s* T_out,
      Ipp16s* p_out, Ipp16s* c_out_, Ipp16s* sprm, Ipp16s* xn_in, EncoderObj_AMRWBE* st)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, code, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, cn, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, xn, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, xn2, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, dn, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, y1, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, y2, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, error, (M+L_SUBFR));
   IPP_ALIGNED_ARRAY(16, Ipp16s, h1, (L_SUBFR+M+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, h2, L_SUBFR);

   Ipp16s *p_A, *p_Aq, Ap[M+1];
   Ipp16s g_corr[10], g_corr2[4];

   Ipp32s i, i_subfr, j;
   Ipp16s T0, T0_frac, index, select;

   Ipp32s ener, mean_ener_code, gain_code;
   Ipp16s gain_pit, max, gain1, gain2;

   Ipp16s exp1, m1, exp2;
   Ipp16s ener_dB, mean_ener_code_dB, tmp16, tmpMemWsyn;
   Ipp32s dwTmp;
   Ipp16s g_code;

   Ipp32s n_subfr;
   Ipp16s openLoopLag[2], pitchLagBounds[2];
   Ipp16s hpCoeffs[2] = {20972, -5898};
   IppSpchBitRate ippRate = tblMonoMode2MonoRate_WBE[codec_mode];

   openLoopLag[0] = (Ipp16s)(T_op << 1);
   openLoopLag[1] = (Ipp16s)(T_op2 << 1);

   max =0;
   mean_ener_code = 0;
   p_Aq = Azq;

   tmp16 = (Ipp16s)(st->Q_sp - st->Q_exc);
   if(tmp16 != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(&exc[-(PIT_MAX_MAX+L_INTERPOL)], PIT_MAX_MAX+L_INTERPOL,  tmp16);
   }

   for (i_subfr=0; i_subfr<len; i_subfr+=L_SUBFR) {
      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &speech[i_subfr], &exc[i_subfr], L_SUBFR, 11);

      ener = 1;
      for (i=0; i<L_SUBFR; i++) {
         tmp16 = (Ipp16s)((exc[i+i_subfr]*8192)>>15);
         ener = Add_32s(ener, Mul2_Low_32s(tmp16*tmp16));
      }

      exp2 = (Ipp16s)(30 - Norm_32s_I(&ener));

      _Log2_norm(ener, exp2, &exp1, &m1);
      exp2 = (Ipp16s)(25 - (exp1 + (st->Q_sp<<1)));

      dwTmp    = Mpy_32_16(exp2, m1, LG10);
      ener_dB = (Ipp16s)(dwTmp>>(14-7));

      if (ener_dB < 0)
         ener_dB = 0;
      if (ener_dB > max)
         max = ener_dB;

      mean_ener_code = Add_32s(mean_ener_code, Mul2_Low_32s(ener_dB*8192));
      p_Aq += (M+1);
   }
   Cnvrt_NR_32s16s(mean_ener_code);
   mean_ener_code = Sub_32s(mean_ener_code, Mul2_Low_32s(norm_corr*5*128));
   mean_ener_code = Sub_32s(mean_ener_code, Mul2_Low_32s(norm_corr2*5*128));

   mean_ener_code = Sub_32s(mean_ener_code, 150994944);
   mean_ener_code_dB = Cnvrt_NR_32s16s(mean_ener_code);
   dwTmp = Sub_32s(mean_ener_code, Mul2_Low_32s(mean_ener_code_dB*30037));

   dwTmp = Add_32s(dwTmp, 4194304);
   dwTmp = dwTmp>>7;
   index = (Ipp16s)(dwTmp>>16);

   if (index < 0)
      index = 0;
   if (index >3)
      index = 3;

   mean_ener_code = Add_32s(18, Mul2_Low_32s(index*12/2));
   mean_ener_code_dB = ShiftL_16s((Ipp16s)(mean_ener_code),7);
   tmp16 = Sub_16s(max,27*128);
   while ((mean_ener_code_dB<tmp16) && (index < 3)) {
      index += 1;
      mean_ener_code_dB = Add_16s(mean_ener_code_dB, 12*128);
   }
   *sprm = index;
   sprm++;

   p_A = Az;
   p_Aq = Azq;

   for (i_subfr=0,n_subfr=0; i_subfr<len; i_subfr+=L_SUBFR,n_subfr++) {
      ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);
      ippsCopy_16s(&xn_in[i_subfr], xn, L_SUBFR);

      ippsCopy_16s(&synth_[i_subfr-M], error, M);
      ippsZero_16s(&error[M], L_SUBFR);

      tmp16 = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s_I(p_Aq, M, &error[M], L_SUBFR, error);
      p_Aq[0] = tmp16;

      ippsResidualFilter_Low_16s_Sfs(Ap, M, &error[M], xn2, L_SUBFR, 11);

      ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 14, xn2, L_SUBFR, mem_wsyn_);

      ippsSub_16s(xn2, xn, xn, L_SUBFR);

      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &speech[i_subfr], &exc[i_subfr], L_SUBFR, 11);

      ippsZero_16s(code, M);
      ippsCopy_16s(xn, &code[M], L_SUBFR/2);

      tmp16 = 0;
      ippsPreemphasize_AMRWB_16s_ISfs(TILT_FAC_FX, &code[M], L_SUBFR/2, 14, &tmp16);

      ippsSynthesisFilter_G729E_16s_I(Ap, M, &code[M], L_SUBFR/2, code);

      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &code[M], cn, L_SUBFR/2, 11);

      ippsCopy_16s(&exc[i_subfr+(L_SUBFR/2)], cn+(L_SUBFR/2), L_SUBFR/2);

      ippsZero_16s(h1, L_SUBFR+M+1);
      ippsCopy_16s(Ap, &h1[M], M+1);

      for (i = 0; i < L_SUBFR; i++)
      {
         dwTmp = Mul2_Low_32s(h1[i + M] * 16384);
         for (j = 1; j <= M; j++)
               dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(p_Aq[j]*h1[i + M - j]));

         h1[i + M] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp, 3));
         h1[i] = h1[i + M];
      }

      tmp16 = 0;
      ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 14, h1, L_SUBFR, &tmp16);

      ippsCopy_16s(h1, h2, L_SUBFR);
      ownScaleSignal_AMRWB_16s_ISfs(h2, L_SUBFR, -2);

      if(st->old_wsp_shift != 0)
         ownScaleSignal_AMRWB_16s_ISfs(xn, L_SUBFR, st->old_wsp_shift);

      if(st->old_wsp_shift != 1)
         ownScaleSignal_AMRWB_16s_ISfs(h1, L_SUBFR, (Ipp16s)(st->old_wsp_shift+1));

      ippsAdaptiveCodebookSearch_AMRWBE_16s(xn, h1, openLoopLag, &T0, pitchLagBounds, &exc[i_subfr], &T0_frac,
         sprm, n_subfr, ippRate, st->i_offset);
      sprm += 1;

      T_out[n_subfr] = T0;

      ippsConvPartial_NR_Low_16s(&exc[i_subfr], h1, y1, L_SUBFR);
      ippsAdaptiveCodebookGainCoeff_AMRWB_16s(xn, y1, g_corr, &gain1, L_SUBFR);
      ippsInterpolateC_G729_16s_Sfs(xn, 16384, y1, (Ipp16s)(-gain1), dn, L_SUBFR, 14);

      ippsHighPassFilter_Direct_AMRWB_16s(hpCoeffs, &exc[i_subfr], code, L_SUBFR, 1);

      ippsConvPartial_NR_16s(code, h1, y2, L_SUBFR);
      ippsAdaptiveCodebookGainCoeff_AMRWB_16s(xn, y2, g_corr2, &gain2, L_SUBFR);

      ippsInterpolateC_G729_16s_Sfs(xn, 16384, y2, (Ipp16s)(-gain2), xn2, L_SUBFR, 14);

      dwTmp = 0;
      for (i=0; i<L_SUBFR; i++)
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(dn[i]*dn[i]));
      for (i=0; i<L_SUBFR; i++)
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(xn2[i]*xn2[i]));

      if(dwTmp > 0) {
         select = 0;
         ippsCopy_16s(code, &exc[i_subfr], L_SUBFR);
         ippsCopy_16s(y2, y1, L_SUBFR);
         gain_pit = gain2;
         g_corr[0] = g_corr2[0];
         g_corr[1] = g_corr2[1];
         g_corr[2] = g_corr2[2];
         g_corr[3] = g_corr2[3];
      } else {
         select = 1;
         gain_pit = gain1;
         ippsCopy_16s(dn, xn2, L_SUBFR);
      }
      *sprm = select;  sprm++;

      ippsInterpolateC_G729_16s_Sfs(xn, 16384, y1, (Ipp16s)(-gain_pit), xn2, L_SUBFR, 14);
      /*implace op. allowed???*/
      ippsInterpolateC_G729_16s_Sfs(cn, 16384, &exc[i_subfr], (Ipp16s)(-gain_pit), cn, L_SUBFR, 14);
      ownScaleSignal_AMRWB_16s_ISfs(cn, L_SUBFR, st->old_wsp_shift);

      tmp16 = 0;
      ippsPreemphasize_AMRWB_16s_ISfs(TILT_CODE_FX, h2, L_SUBFR, 14, &tmp16);

      if (T0_frac > 2)
         T0 = (Ipp16s)(T0 + 1);
      if (T0 < L_SUBFR)
         ippsHarmonicFilter_NR_16s(PIT_SHARP_FX, T0, &h2[T0], &h2[T0], L_SUBFR-T0);

      ippsAlgebraicCodebookSearch_AMRWB_16s(xn2, cn, h2, code, y2, ippRate, sprm);
      if (codec_mode==MODE_9k6 || codec_mode==MODE_11k2 || codec_mode==MODE_12k8 ||
          codec_mode==MODE_14k4 || codec_mode==MODE_16k)
         sprm += 4;
      else if (codec_mode==MODE_18k4 || codec_mode==MODE_20k || codec_mode==MODE_23k2)
         sprm += 8;
      else {
         return APIAMRWB_StsErr;/* invalid mode for acelp frame! */
      }

      tmp16 = 0;
      ippsPreemphasize_AMRWB_16s_ISfs(TILT_CODE_FX, code, L_SUBFR, 14, &tmp16);
      ippsHarmonicFilter_NR_16s(PIT_SHARP_FX, T0, &code[T0], &code[T0], L_SUBFR-T0);

      ippsGainQuant_AMRWBE_16s(xn, y1, (st->Q_sp + st->old_wsp_shift + 1),
         code, y2, L_SUBFR, g_corr, mean_ener_code_dB, &gain_pit, &c_out_[i_subfr/L_SUBFR], &gain_code, &index);
      p_out[i_subfr/L_SUBFR] = gain_pit;

      *sprm = index; sprm++;

      if (st->Q_sp > 0) {
         dwTmp = ShiftL_32s(gain_code, st->Q_sp);
      } else {
         dwTmp = gain_code >> (-st->Q_sp);
      }
      g_code = Cnvrt_NR_32s16s(dwTmp);

      dwTmp = Mul2_Low_32s(g_code * y2[L_SUBFR - 1]);
      if ((5+st->old_wsp_shift) > 0) {
         dwTmp = ShiftL_32s(dwTmp, (Ipp16u)(5+st->old_wsp_shift));
      } else {
         dwTmp = dwTmp >> (-(5+st->old_wsp_shift));
      }
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(y1[L_SUBFR - 1]*gain_pit));
      if (1 > st->old_wsp_shift) {
         dwTmp = ShiftL_32s(dwTmp, (Ipp16s)(1 - st->old_wsp_shift));
      } else {
         dwTmp = dwTmp >> (st->old_wsp_shift-1);
      }
      *mem_wsyn_ = Cnvrt_NR_32s16s(dwTmp);

      for (i = 0; i < L_SUBFR; i++) {
        dwTmp = Mul2_Low_32s(g_code * code[i]);
        dwTmp = ShiftL_32s(dwTmp, 6);
        dwTmp = Add_32s(dwTmp, Mul2_Low_32s(exc[i + i_subfr]*gain_pit));
        dwTmp = ShiftL_32s(dwTmp, 1);
        exc[i + i_subfr] = Cnvrt_NR_32s16s(dwTmp);
      }
      //^^^^ overflows?? ^^^^
      //ippsInterpolateC_NR_16s(code, g_code, 7, &exc[i_subfr], gain_pit, 2, &exc[i_subfr], L_SUBFR);

      tmp16 = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &exc[i_subfr], &synth_[i_subfr], L_SUBFR, &synth_[i_subfr-M]);
      p_Aq[0] = tmp16;

      p_A += (M+1);
      p_Aq += (M+1);
   }

   ippsMulPowerC_NR_16s_Sfs(p_Aq, GAMMA1_FX, Ap, (M+1), 15);

   ippsCopy_16s(&synth_[len-M], error, M);
   tmpMemWsyn = *mem_wsyn_;
   for (i_subfr=0; i_subfr<(2*L_SUBFR); i_subfr+=L_SUBFR) {
      ippsZero_16s(&error[M], L_SUBFR);

      tmp16 = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s_I(p_Aq, M, &error[M], L_SUBFR, error);
      p_Aq[0] = tmp16;

      ippsResidualFilter_Low_16s_Sfs(Ap, M, &error[M], &wovlp_[i_subfr], L_SUBFR, 11);
      ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 14, &wovlp_[i_subfr], L_SUBFR, &tmpMemWsyn);
      ippsMove_16s(&error[L_SUBFR], error, M);
   }

   for (i=1; i<L_SUBFR; i++) {
      wovlp_[L_SUBFR+i] = MulHR_16s(wovlp_[L_SUBFR+i], tblOverlap5ms_16s[L_SUBFR-1-i]);
   }

   st->Q_exc = st->Q_sp;
   return APIAMRWB_StsNoErr;
}

static void Try_tcx(Ipp32s k, Ipp32s mode, Ipp16s* snr, Ipp16s* A, Ipp16s* wsp, Ipp16s* mod,
   Ipp16s* coding_mod, Ipp16s* isp, Ipp16s* isp_q, Ipp16s* AqLF, Ipp16s* speech, Ipp16s* mem_w0,
   Ipp16s* mem_xnq, Ipp16s* mem_wsyn, Ipp16s* old_exc, Ipp16s* mem_syn, Ipp16s* wovlp,
   Ipp16s* ovlp_size, Ipp16s* past_isfq, Ipp32s nbits, Ipp32s nprm_tcx,
   Ipp16s* prm, EncoderObj_AMRWBE *st)
{
   Ipp32s i, ndiv;
   Ipp16s tmp_snr;
   Ipp32s ndiv_l;
   Ipp16s mem_w0_tcx, mem_xnq_tcx, mem_wsyn_tcx;

   IPP_ALIGNED_ARRAY(16, Ipp16s, Aq, ((NB_SUBFR_WBP+1)*(M+1)));
   Ipp16s* p_Aq;
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_tcx, (M+L_TCX));
   IPP_ALIGNED_ARRAY(16, Ipp16s, exc_tcx, (L_TCX));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wovlp_tcx, (128));
   IPP_ALIGNED_ARRAY(16, Ipp16s, past_isfq_tcx, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew_q, (M));
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew, (M));
   Ipp16s exp_dwn;
   const Ipp16s *wind_frac;

   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_tcx_buf, (NPRM_LPC+NPRM_TCX80));
   Ipp16s *prm_tcx;

   prm_tcx = prm_tcx_buf;
   ndiv = mode;
   if(ndiv == 1) {
      wind_frac = tblInterpolTCX20_16s;
   } else if (ndiv == 2) {
      wind_frac = tblInterpolTCX40_16s;
   } else {
      ndiv = ndiv + 1;
      wind_frac = tblInterpolTCX80_16s;
   }
   ndiv_l = ndiv * L_DIV;

   if (mode > 1) {
      ippsISPToISF_Norm_AMRWB_16s(&isp[(k+ndiv)*M], isfnew, M);
      ippsCopy_16s(&past_isfq[(k/2)*M], past_isfq_tcx, M);
      {
         IppSpchBitRate ippTmpRate = IPP_SPCHBR_8850;/* NOT (6600 & DTX)*/
         ippsISFQuant_AMRWB_16s(isfnew, past_isfq_tcx, isfnew, prm_tcx, ippTmpRate);
      }
      prm_tcx += NPRM_LPC;

      ippsISFToISP_AMRWB_16s(isfnew, ispnew_q, M);
      ownIntLPC(&isp_q[k*M], ispnew_q, wind_frac , Aq, (ndiv<<2), M);
      p_Aq = Aq;
   } else  {
      p_Aq = &AqLF[k*4*(M+1)];
   }

   ippsCopy_16s(&mem_syn[k*M], synth_tcx, M);

   mem_w0_tcx = mem_w0[k];
   mem_xnq_tcx = mem_xnq[k];
   ippsCopy_16s(&wovlp[k*128], wovlp_tcx, 128);

   exp_dwn = Coder_tcx(p_Aq, &speech[k*L_DIV], &mem_w0_tcx, &mem_xnq_tcx, &synth_tcx[M],
      exc_tcx, wovlp_tcx, ovlp_size[k], ndiv_l, nbits, prm_tcx, st);

   mem_wsyn_tcx = mem_wsyn[k];
   {
      IPP_ALIGNED_ARRAY(16, Ipp16s, buf, (L_FRAME_PLUS));
      ownFindWSP(&A[k*4*(M+1)], &synth_tcx[M], buf, &mem_wsyn_tcx, ndiv_l);
      ippsSNR_AMRWBE_16s(&wsp[k*L_DIV], buf, ndiv_l, L_SUBFR, &tmp_snr);
   }

   st->Q_exc = Add_16s(st->Q_sp,exp_dwn);

   if (tmp_snr > (*snr)) {
      st->LastQMode = 1;
      *snr = tmp_snr;
      for (i=0; i<ndiv; i++) {
         mod[k+i] = (Ipp16s)mode;
         coding_mod[k+i] = (Ipp16s)mode;
      }

      mem_w0[k+ndiv] = mem_w0_tcx;
      mem_xnq[k+ndiv] = mem_xnq_tcx;
      mem_wsyn[k+ndiv] = mem_wsyn_tcx;

      ovlp_size[k+ndiv] = (Ipp16s)(ndiv<<5);

      if (mode > 1) {
         ippsCopy_16s(ispnew_q, &isp_q[(k+ndiv)*M], M);
         ippsCopy_16s(past_isfq_tcx, &past_isfq[((k+ndiv)/2)*M], M);
         ippsCopy_16s(Aq, &AqLF[k*4*(M+1)], ((ndiv*4)+1)*(M+1));
      }

      if (ndiv == 1) {
         ippsMove_16s(&old_exc[L_DIV], old_exc, (PIT_MAX_MAX + L_INTERPOL)- L_DIV );
         ippsCopy_16s(exc_tcx, old_exc+PIT_MAX_MAX+L_INTERPOL - L_DIV , L_DIV);
      } else {
         ippsCopy_16s(&exc_tcx[(ndiv_l)-(PIT_MAX_MAX+L_INTERPOL)], old_exc, PIT_MAX_MAX+L_INTERPOL);
      }

      ippsCopy_16s(prm_tcx_buf, prm, nprm_tcx);
      ippsCopy_16s(wovlp_tcx, &wovlp[(k+ndiv)*128], 128);
      ippsCopy_16s(&synth_tcx[ndiv_l], &mem_syn[(k+ndiv)*M], M);
   }
}

#if defined(_WIN32_WCE)
#pragma optimize( "", off )
#endif
static Ipp16s Coder_tcx(Ipp16s* A, Ipp16s* speech, Ipp16s* mem_wsp, Ipp16s* mem_wsyn,
      Ipp16s* synth, Ipp16s* exc_, Ipp16s* wovlp, Ipp32s ovlp_size, Ipp32s frameSize,
      Ipp32s nb_bits, Ipp16s* prm, EncoderObj_AMRWBE *st  )
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, window, (256));
   IPP_ALIGNED_ARRAY(16, Ipp16s, Ap, (M+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, temp, (M));
   Ipp16s *xri, *xn, *xnq, *p_A;
   Ipp32s i, i_subfr, lext, lg;
   Ipp16s tmp16, fac_ns_, inc2, index;
   Ipp16s Q_tcx, e_tmp, max16s, max1;
   Ipp32s dwTmp, Lgain;

   xn = synth;
   xnq = xri = exc_;

   if (frameSize == (L_FRAME_PLUS/2)) {
      lext = L_OVLP/2;
      inc2 = 2;
   } else if (frameSize == (L_FRAME_PLUS/4)) {
      lext = L_OVLP/4;
      inc2 = 4;
   } else {
      lext = L_OVLP;
      inc2 = 1;
   }

   lg = frameSize + lext;
   Cos_window(window, ovlp_size, lext, inc2);

   p_A = A;
   for (i_subfr=0; i_subfr<frameSize; i_subfr+=L_SUBFR) {
      ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);
      ippsResidualFilter_Low_16s_Sfs(Ap, M, &speech[i_subfr], &xn[i_subfr], L_SUBFR, 12);
      p_A += (M+1);
   }
   ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 15, xn, frameSize, mem_wsp);

   ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);
   ippsResidualFilter_Low_16s_Sfs(Ap, M, &speech[frameSize], &xn[frameSize], lext, 12);

   tmp16 = *mem_wsp;
   ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 15, &xn[frameSize], lext, &tmp16);

   if (ovlp_size == 0) {
      ippsSub_16s(wovlp, xn, xn, (2*L_SUBFR));
   }

   ownWindowing(window, xn, ovlp_size);
   ownWindowing(&window[ovlp_size], &xn[frameSize], lext);

   ippsFFTFwd_RToPerm_AMRWBE_16s(xn, xri, lg);
   xri[1] = 0;

   Adap_low_freq_emph(xri, lg);

   nb_bits = nb_bits - 10;
   if (frameSize == (L_FRAME_PLUS/2)) {
      nb_bits = nb_bits - 6;
   } else if (frameSize == L_FRAME_PLUS) {
      nb_bits = nb_bits - 9;
   }

   ippsQuantTCX_AMRWBE_16s(xri, &prm[2], (lg>>3), nb_bits, &fac_ns_);

   ippsCopy_16s(&prm[2], xri, lg);

   Scale_tcx_ifft(xri, lg, &Q_tcx);
   Adap_low_freq_deemph(xri, lg);

   dwTmp = Sub_32s(524288, Mul2_Low_32s(10*fac_ns_));
   index = Cnvrt_NR_32s16s(dwTmp);

   if (index < 0) {
      index = 0;
   } else if (index > 7) {
      index = 7;
   }

   prm[0] = index;

   xri[1] = 0;
   ippsFFTInv_PermToR_AMRWBE_16s(xri, xnq, lg);

   ippsGainQuantTCX_AMRWBE_16s(xn, st->Q_sp, xnq, lg, 1, &Lgain, &prm[1]);

   e_tmp = Norm_32s_I(&Lgain);
   tmp16  = (Ipp16s)(Lgain>>16);

   e_tmp = (Ipp16s)(e_tmp - (st->Q_sp + 15));
   if (e_tmp >= 0) {
      for (i=0; i<lg; i++) {
         dwTmp = Mul2_Low_32s(xnq[i] * tmp16);
         xnq[i] = Cnvrt_NR_32s16s(dwTmp >> e_tmp);
      }
   } else {
      for (i=0; i<lg; i++) {
         dwTmp = Mul2_Low_32s(xnq[i] * tmp16);
         xnq[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp, (Ipp16u)(-e_tmp)));
      }
   }

   ownWindowing(window, xnq, ovlp_size);
   ownWindowing(&window[ovlp_size], &xnq[frameSize], lext);

   ippsAdd_16s(wovlp, xnq, xnq, L_OVLP);
   ippsCopy_16s(&xnq[frameSize], wovlp, lext);
   ippsZero_16s(&wovlp[lext], (L_OVLP-lext));

   ippsPreemphasize_AMRWB_16s_ISfs(TILT_FAC_FX, xnq, frameSize, 14, mem_wsyn);

   ippsCopy_16s(&synth[-M], temp, M);

   max16s = 0;
   ippsMaxAbs_16s(xnq, frameSize, &max16s);
   max16s = (Ipp16s)(IPP_MAX(0, max16s));

   max16s = (Ipp16s)(Exp_16s(max16s) - 2);
   if (max16s > 0) {
      max16s = 0;
   }
   max1 = (Ipp16s)(max16s - 1);

   ownScaleSignal_AMRWB_16s_ISfs(xnq, frameSize, max16s);
   ownScaleSignal_AMRWB_16s_ISfs(&synth[-M], M, max1);

   p_A = A;
   for (i_subfr=0; i_subfr<frameSize; i_subfr+=L_SUBFR) {
      ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);

      tmp16 = Ap[0];
      Ap[0] = (Ipp16s)(Ap[0] >> 1);
      ippsSynthesisFilter_G729E_16s(Ap, M, &xnq[i_subfr], &synth[i_subfr], L_SUBFR, &synth[i_subfr-M]);
      Ap[0] = tmp16;

      ippsResidualFilter_Low_16s_Sfs(p_A, M, &synth[i_subfr], &exc_[i_subfr], L_SUBFR, 11);
      p_A += (M+1);
   }
   ownScaleSignal_AMRWB_16s_ISfs(synth, frameSize, (Ipp16s)(-max1));
   ippsCopy_16s(temp, &synth[-M], M);

   return max1;
}
#if defined(_WIN32_WCE)
#pragma optimize( "", on )
#endif

static void Cod_hi_stereo(Ipp16s* wAqLF, Ipp16s* param, EncoderObj_AMRWBE *st,
      Ipp16s* speech_hi_, Ipp16s* right_hi_)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, wbuf, (L_DIV+L_SUBFR));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wgain_left, (NB_SUBFR_WBP));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wgain_right, (NB_SUBFR_WBP));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wwh, (NB_DIV*HI_FILT_ORDER));
   Ipp16s *wexc_mono = speech_hi_-M;
   Ipp16s *wexc_side = right_hi_-M;
   Ipp16s *wp_Aq;
   Ipp16s *wp_h;
   Ipp16s *wx,*wy;
   Ipp16s *prm;
   Ipp32s i_subfr, i;

   ippsCopy_16s(st->old_exc_mono,wexc_mono-HI_FILT_ORDER,HI_FILT_ORDER);

   wp_Aq = wAqLF;
   for (i_subfr=0; i_subfr<L_FRAME_PLUS; i_subfr+=L_SUBFR) {
      ippsResidualFilter_Low_16s_Sfs(wp_Aq, M, &speech_hi_[i_subfr], &wexc_mono[i_subfr], L_SUBFR, 12);
      ippsResidualFilter_Low_16s_Sfs(wp_Aq, M, &right_hi_[i_subfr], &wexc_side[i_subfr], L_SUBFR, 12);
      wp_Aq += (M+1);
   }
   ippsResidualFilter_Low_16s_Sfs(wp_Aq, M, &speech_hi_[i_subfr], &wexc_mono[i_subfr], L_SUBFR, 12);
   ippsResidualFilter_Low_16s_Sfs(wp_Aq, M, &right_hi_[i_subfr], &wexc_side[i_subfr], L_SUBFR, 12);

   Compute_exc_side(wexc_side, wexc_mono);

   ippsCopy_16s(wexc_mono+L_FRAME_PLUS-HI_FILT_ORDER, st->old_exc_mono, HI_FILT_ORDER);

   wp_h = wwh;
   for(i=0; i<NB_DIV; i++) {
      prm = param + i*NPRM_STEREO_HI_X;
      wx = wexc_mono + i*L_DIV;
      wy = wexc_side + i*L_DIV;

      ippsFIRGenMidBand_AMRWBE_16s(wx, wy, wp_h);

      Smooth_ener_filter(wp_h, &(st->filt_energy_threshold));

      Quant_filt(wp_h, st->old_wh_q, &prm, st->filt_hi_pmsvq);
      Fir_filt(wp_h, HI_FILT_ORDER, wx, wbuf, L_DIV+L_SUBFR);

      Compute_gain_match(&wgain_left[i], &wgain_right[i], wx, wy, wbuf);

      Quant_gain(wgain_left[i], wgain_right[i], st->old_gm_gain, &prm, st->gain_hi_pmsvq);

      wp_h += HI_FILT_ORDER;
   }
   ippsCopy_16s(&wwh[(NB_DIV-1)*HI_FILT_ORDER], st->old_wh, HI_FILT_ORDER);
}

static void Cod_tcx_stereo(
  Ipp16s* wmono_2k,
  Ipp16s* wright_2k,
  Ipp16s* param,
  Ipp32s brMode,
  Ipp16s* mod,
  Ipp16s fscale,
  EncoderObj_AMRWBE *st
  )
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_tcx, (NPRM_TCX80_D));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sig_buf_mono, (TCX_L_FFT_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sig_buf_right, (TCX_L_FFT_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, sig_buf_side, (TCX_L_FFT_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_tcx, (L_FRAME_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth, (L_FRAME_2k));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ovlp, (L_OVLP_2k*5));
   IPP_ALIGNED_ARRAY(16, Ipp16s, ovlp_tcx, (L_OVLP_2k));
   Ipp32s i, k, i2, i1, nbits;
   Ipp16s ovlp_size[4+1];
   Ipp16s *prm;
   Ipp16s snr, snr1, snr2, tmp16;

   nbits = tblNBitsStereoCore_16s[brMode] - (24+4);
   if (tblNBitsStereoCore_16s[brMode] > (300+4)) {
      nbits = nbits - 24;
   }

   ippsZero_16s(sig_buf_mono,TCX_L_FFT_2k);

   if(fscale == 0) {
      ippsCopy_16s(&wmono_2k[-TCX_STEREO_DELAY_2k], sig_buf_mono, L_FRAME_2k+L_OVLP_2k);
   } else {
      ippsCopy_16s(&wmono_2k[-TCX_STEREO_DELAY_2k-L_SUBFR_2k], sig_buf_mono, L_FRAME_2k+L_OVLP_2k);
   }

   ippsZero_16s(sig_buf_right,TCX_L_FFT_2k);
   if(fscale == 0) {
      ippsCopy_16s(&wright_2k[-TCX_STEREO_DELAY_2k], sig_buf_right, L_FRAME_2k+L_OVLP_2k);
   } else {
      ippsCopy_16s(&wright_2k[-TCX_STEREO_DELAY_2k-L_SUBFR_2k], sig_buf_right, L_FRAME_2k+L_OVLP_2k);
   }

   ippsZero_16s(sig_buf_side,TCX_L_FFT_2k);

   ippsSub_16s(sig_buf_right, sig_buf_mono, sig_buf_side, TCX_L_FFT_2k);

   ovlp_size[0] = st->mem_stereo_ovlp_size;
   ippsCopy_16s(st->mem_stereo_ovlp, ovlp, L_OVLP_2k);

   snr2 = 0;
   for (i1=0; i1<2; i1++) {
      snr1 = 0;
      for (i2=0; i2<2; i2++) {
         k = (i1<<1) + i2;

         prm = param + (k*NPRM_DIV_TCX_STEREO);

         ippsCopy_16s(&ovlp[k*L_OVLP_2k], ovlp_tcx, L_OVLP_2k);
         Ctcx_stereo(&sig_buf_side[k*L_DIV_2k], &sig_buf_mono[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k],
            ovlp_tcx, ovlp_size[k], L_FRAME_2k/4, ((nbits>>2)-2), prm_tcx, 1, st->Q_new);

         ippsSNR_AMRWBE_16s(&sig_buf_side[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k], L_FRAME_2k/4,L_DIV_2k, &snr);

         mod[k] = 0;

         ovlp_size[k+1] = L_OVLP_2k/4;

         ippsCopy_16s(ovlp_tcx, &ovlp[(k+1)*L_OVLP_2k], L_OVLP_2k);
         ippsCopy_16s(&synth_tcx[k*L_DIV_2k], &synth[k*L_DIV_2k], L_FRAME_2k/4);
         ippsCopy_16s(prm_tcx, prm, NPRM_TCX20_D);
         ippsCopy_16s(&ovlp[k*L_OVLP_2k], ovlp_tcx, L_OVLP_2k);

         Ctcx_stereo(&sig_buf_side[k*L_DIV_2k], &sig_buf_mono[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k],
            ovlp_tcx, ovlp_size[k], L_FRAME_2k/4, ((nbits>>2)-2), prm_tcx, 0, st->Q_new);

         ippsSNR_AMRWBE_16s(&sig_buf_side[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k], L_FRAME_2k/4,L_DIV_2k, &tmp16);

         if(tmp16 > snr) {
            snr = tmp16;
            mod[k] = 1;
            ovlp_size[k+1] = L_OVLP_2k/4;
            ippsCopy_16s(ovlp_tcx, &ovlp[(k+1)*L_OVLP_2k], L_OVLP_2k);
            ippsCopy_16s(&synth_tcx[k*L_DIV_2k], &synth[k*L_DIV_2k], L_FRAME_2k/4);
            ippsCopy_16s(prm_tcx, prm, NPRM_TCX20_D);
         }
         snr1  = Add_16s(snr1, MulHR_16s(snr,16384));
      }
      k = i1 << 1;
      prm = param + (k*NPRM_DIV_TCX_STEREO);

      ippsCopy_16s(&ovlp[k*L_OVLP_2k], ovlp_tcx, L_OVLP_2k);

      Ctcx_stereo(&sig_buf_side[k*L_DIV_2k], &sig_buf_mono[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k], ovlp_tcx,
         ovlp_size[k], L_FRAME_2k/2, ((nbits>>1)-4), prm_tcx, 0, st->Q_new);
      ippsSNR_AMRWBE_16s(&sig_buf_side[k*L_DIV_2k], &synth_tcx[k*L_DIV_2k], L_FRAME_2k/2, L_DIV_2k, &tmp16);

      if (tmp16 > snr1) {
         snr1 = tmp16;
         for (i=0; i<2; i++) {
            mod[k+i] = 2;
         }
         ovlp_size[k+2] = L_OVLP_2k/2;
         ippsCopy_16s(ovlp_tcx, &ovlp[(k+2)*L_OVLP_2k], L_OVLP_2k);
         ippsCopy_16s(&synth_tcx[k*L_DIV_2k], &synth[k*L_DIV_2k], L_FRAME_2k/2);
         ippsCopy_16s(prm_tcx, prm, NPRM_TCX40_D);
      }
      snr2 =Add_16s(snr2, MulHR_16s(snr1,16384));
   }

   k = 0;
   prm = param + (k*NPRM_DIV_TCX_STEREO);

   ippsCopy_16s(&ovlp[k*L_OVLP_2k], ovlp_tcx, L_OVLP_2k);
   Ctcx_stereo(sig_buf_side, sig_buf_mono, synth_tcx, ovlp_tcx, ovlp_size[k], L_FRAME_2k,
      (nbits-8), prm_tcx, 0, st->Q_new);

   ippsSNR_AMRWBE_16s(sig_buf_side, synth_tcx, L_FRAME_2k, L_DIV_2k, &tmp16);

   if (tmp16 > snr2) {
      for (i=0; i<4; i++) {
         mod[k+i] = 3;
      }
      ovlp_size[k+4] = L_OVLP_2k;
      ippsCopy_16s(ovlp_tcx, &ovlp[(k+4)*L_OVLP_2k], L_OVLP_2k);
      ippsCopy_16s(synth_tcx, &synth[k*L_DIV_2k], L_FRAME_2k);
      ippsCopy_16s(prm_tcx, prm, NPRM_TCX80_D);
   }

   st->mem_stereo_ovlp_size = ovlp_size[4];
   ippsCopy_16s(&ovlp[4*L_OVLP_2k], st->mem_stereo_ovlp, L_OVLP_2k);
}

static APIAMRWB_Status Ctcx_stereo(Ipp16s* wside, Ipp16s* wmono, Ipp16s* wsynth,
   Ipp16s* wwovlp, Ipp32s ovlp_size, Ipp32s frameSize, Ipp32s nb_bits, Ipp16s* sprm,
   Ipp16s pre_echo, Ipp16s Q_in)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, xri, (L_TCX_LB));
   IPP_ALIGNED_ARRAY(16, Ipp16s, xn, (L_TCX_LB));
   IPP_ALIGNED_ARRAY(16, Ipp16s, wm, (L_TCX_LB));
   IPP_ALIGNED_ARRAY(16, Ipp16s, xnq, (L_TCX_LB));
   IPP_ALIGNED_ARRAY(16, Ipp16s, window, (L_TCX_LB));
   Ipp16s gain_shap_cod[8];
   Ipp16s gain_shap_dec[8];

   Ipp16s xn_scl;
   Ipp32s i, lg, lext;
   Ipp32s dwTmp, Lgain;
   Ipp16s tmp16, fac_ns;
   Ipp16s gain_pan, inc2,Q_tcx, e_tmp;

   switch (frameSize) {
      case 40:  lext = 8;  inc2 = 16;  break;
      case 80:  lext = 16;  inc2 = 8;  break;
      case 160:  lext = 32;  inc2 = 4;  break;
      default :  return APIAMRWB_StsBadArgErr;
   };
   lg = frameSize + lext;

   Cos_window(window, ovlp_size, lext, inc2);

   ippsCopy_16s(wside, xn, lg);
   ippsCopy_16s(wmono, wm, lg);

   ownWindowing(window, xn, ovlp_size);
   ownWindowing(window, wm, ovlp_size);

   ownWindowing(&window[ovlp_size], &xn[frameSize], lext);
   ownWindowing(&window[ovlp_size], &wm[frameSize], lext);

   if(pre_echo>0) {
      Comp_gain_shap_cod(wm,gain_shap_cod,gain_shap_dec ,lg, Q_in);
      Apply_gain_shap(lg, xn, gain_shap_cod);
      Apply_gain_shap(lg, wm, gain_shap_cod);
   }

   ippsGainQuantTCX_AMRWBE_16s(xn, 1, wm, lg, 0, &Lgain, &tmp16);
   gain_pan = (Ipp16s)(ShiftL_32s(Lgain, 15)>>16);

   nb_bits = nb_bits - 7;
   sprm[0] = Q_gain_pan(&gain_pan);

   xn_scl = Compute_xn_target(xn, wm, gain_pan, lg);

   ippsFFTFwd_RToPerm_AMRWBE_16s(xn, xri, lg);
   xri[1] = 0;

   Adap_low_freq_emph(xri, (lg<<2));

   nb_bits = nb_bits - 7;
   ippsQuantTCX_AMRWBE_16s(xri, sprm+2, (lg>>3), nb_bits, &fac_ns);

   ippsCopy_16s(&sprm[2], xri, lg);

   Scale_tcx_ifft(xri, lg ,&Q_tcx);
   Adap_low_freq_deemph(xri, (lg<<2));

   xri[1] = 0;
   ippsFFTInv_PermToR_AMRWBE_16s(xri, xnq, lg);

   ownScaleSignal_AMRWB_16s_ISfs(xnq, lg, (Ipp16s)(-xn_scl));
   ownScaleSignal_AMRWB_16s_ISfs(xn, lg, (Ipp16s)(-xn_scl));

   if (pre_echo > 0) {
      Apply_gain_shap(lg, xnq, gain_shap_dec );
      Apply_gain_shap(lg, xn, gain_shap_dec );
   }

   ippsGainQuantTCX_AMRWBE_16s(xn, Q_in, xnq, lg, 1, &Lgain, &sprm[1]);

   e_tmp = Norm_32s_I(&Lgain);
   tmp16  = (Ipp16s)(Lgain>>16);

   e_tmp = (Ipp16s)(e_tmp - (Q_in + 14));
   if (e_tmp >= 0) {
      for (i=0; i<lg; i++) {
         dwTmp = Mul2_Low_32s(xnq[i] * tmp16);
         dwTmp = Add_32s((dwTmp>>e_tmp), Mul2_Low_32s(wm[i]*gain_pan));
         xnq[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,1));
      }
   } else {
      for (i=0; i<lg; i++) {
         dwTmp = Mul2_Low_32s(xnq[i] * tmp16);
         dwTmp = Add_32s(ShiftL_32s(dwTmp, (Ipp16u)(-e_tmp)), Mul2_Low_32s(wm[i]*gain_pan));
         xnq[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,1));
      }
   }

   ownWindowing(window, xnq, ovlp_size);
   ownWindowing(&window[ovlp_size], &xnq[frameSize], lext);

   Apply_tcx_overlap(xnq,wwovlp,lext,frameSize);

   ippsCopy_16s(xnq, wsynth, frameSize);

   return APIAMRWB_StsNoErr;
}

