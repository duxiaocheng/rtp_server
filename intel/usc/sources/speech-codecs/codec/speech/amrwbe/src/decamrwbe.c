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
// Purpose: AMRWBE speech codec decoder API functions.
//
*/

#include "ownamrwbe.h"


extern __ALIGN32 CONST Ipp16s tblGainRampHF[64];

extern Ipp16s ownVoiceFactor(Ipp16s *pPitchExc, Ipp16s valExcFmt, Ipp16s valPitchGain,
   Ipp16s *pFixCdbkExc, Ipp16s valCodeGain, Ipp32s len); /*in WB modul */

void Init_dec_hi_stereo(DecoderObj_AMRWBE* st);

static void Dec_prm(Ipp16s mod[], Ipp32s bad_frame[], Ipp16s serial[], Ipp32s nbits_pack,
   Ipp32s codec_mode, Ipp16s param[], Ipp32s nbits_AVQ[]);
static void Dec_prm_stereo_x(Ipp32s bad_frame[], Ipp16s serial[], Ipp32s nbits_pack,
   Ipp16s nbits_bwe, Ipp16s param[], Ipp32s  brMode, DecoderObj_AMRWBE* st);
static void Dec_prm_hf(Ipp16s mod[], Ipp32s bad_frame[], Ipp16s serial[], Ipp32s nbits_pack,
   Ipp16s param[]);
static APIAMRWB_Status Decoder_amrwb_plus_1(Ipp16s* chan_right, Ipp16s* chan_left, Ipp16s* mod,
   Ipp16s* param, Ipp16s* prm_hf_right, Ipp16s* prm_hf_left, Ipp32s* nbits_AVQ,
   Ipp32s  codec_mode, Ipp32s* bad_frame, Ipp32s* bad_frame_hf, Ipp16s* AqLF, Ipp16s* synth,
   DecoderObj_AMRWBE* st, Ipp32s n_channel, Ipp32s L_frame, Ipp16s fscale, Ipp32s mono_dec_stereo);
static APIAMRWB_Status Decoder_stereo_x(Ipp16s param[], Ipp32s bad_frame[], Ipp16s sig_left_[],
   Ipp16s synth_[], Ipp16s Az[], Ipp32s StbrMode, Ipp16s fscale, DecoderObj_AMRWBE* st);
static APIAMRWB_Status Decoder_lf(Ipp16s mod[], Ipp16s param[], Ipp32s nbits_AVQ[], Ipp32s codec_mode,
   Ipp32s bad_frame[], Ipp16s AzLF[], Ipp16s exc_[], Ipp16s syn[], Ipp16s pitch[], Ipp16s pit_gain[],
   Ipp16s pit_adj, DecoderObj_AMRWBE *st);
static void Decoder_hf(Ipp16s mod[], Ipp16s param[], Ipp16s param_other[], Ipp32s mono_dec_stereo,
   Ipp32s bad_frame[], Ipp16s AqLF[], Ipp16s exc[], Ipp16s synth_hf[], Ipp16s mem_lpc_hf[],
   Ipp16s *mem_gain_hf, Ipp16s *ramp_state, DecoderOneChanelState_AMRWBE *st, Ipp16s QexcHF, Ipp16s fscale);
static APIAMRWB_Status Dec_tcx_stereo(Ipp16s* synth_2k, Ipp16s* left_2k, Ipp16s* right_2k,
   Ipp16s* param, Ipp32s* bad_frame, DecoderObj_AMRWBE *st);
static APIAMRWB_Status Dtcx_stereo(Ipp16s* synth, Ipp16s* mono, Ipp16s* wovlp, Ipp32s ovlp_size,
   Ipp32s L_frame, Ipp16s* prm, Ipp32s pre_echo, Ipp32s* bad_frame, DecoderObj_AMRWBE* st);
static void Dec_hi_stereo(Ipp16s synth_hi_t0[], Ipp16s right_hi[], Ipp16s left_hi[], Ipp16s AqLF[],
   Ipp16s  param[], Ipp32s bad_frame[], Ipp16s fscale, DecoderObj_AMRWBE* st);
static void Decoder_acelp(Ipp16s prm[], Ipp16s lg, Ipp32s codec_mode, Ipp32s bfi, Ipp16s *pT,
   Ipp16s *pgainT, Ipp16s Az[], Ipp16s exc[], Ipp16s synth[], Ipp16s pit_adj,Ipp32s len,
   Ipp16s stab_fac, DecoderObj_AMRWBE* st);
static APIAMRWB_Status Decoder_tcx(Ipp16s* prm, Ipp32s* nbits_AVQ, Ipp16s* A, Ipp32s L_frame,
   Ipp32s* bad_frame, Ipp16s* exc, Ipp16s* synth, DecoderObj_AMRWBE* st);

AMRWB_CODECFUN(APIAMRWB_Status, apiDecoderGetObjSize_AMRWBE, (Ipp32u *pCodecSize))
{
   Ipp32s sizeObj;
   if(NULL == pCodecSize)
      return APIAMRWB_StsBadArgErr;

   *pCodecSize=sizeof(DecoderObj_AMRWBE);
   ippsHighPassFilterGetSize_AMRWB_16s(2, &sizeObj);
   *pCodecSize += (3*sizeObj); /* left/right/common memSigOut */

   return APIAMRWB_StsNoErr;
}

AMRWB_CODECFUN( APIAMRWB_Status, apiDecoder_AMRWBE,
   (DecoderObj_AMRWBE *st,
   Ipp16s serial[],
   Ipp32s bad_frame[],
   Ipp32s L_frame,
   Ipp32s n_channel,
   Ipp16s channel_right[],
   Ipp16s channel_left[],
   Ipp32s codec_mode,
   Ipp32s StbrMode,
   Ipp16s fscale,
   Ipp16s upsamp_fscale,
   Ipp32s *pSamplesCount,
   Ipp32s *pFrameScale
))
{

   IPP_ALIGNED_ARRAY(16, Ipp16s, AqLF        , (NB_SUBFR_WBP+1)*(M+1));     /* LPC coefficients for band 0..6400Hz */
   IPP_ALIGNED_ARRAY(16, Ipp16s, sig_left    , L_FRAME_PLUS+2*L_FDEL+20);
   IPP_ALIGNED_ARRAY(16, Ipp16s, synth_buf   , L_BSP+L_TCX);
   IPP_ALIGNED_ARRAY(16, Ipp32s, nbits_AVQ   , 4);
   IPP_ALIGNED_ARRAY(16, Ipp16s, param       , DEC_NPRM_DIV*NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_stereo  , MAX_NPRM_STEREO_DIV*NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_hf_left , NPRM_BWE_DIV*NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_hf_right, NPRM_BWE_DIV*NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, mod_buf     , 1+NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp32s, bad_frame_hf, 4);


   Ipp16s *mod;

   Ipp32s i, k;
   Ipp32s nbits_pack;
   Ipp32s n_loss, any_tcx80;
   Ipp32s nb_samp;
   Ipp16s *synth;

   Ipp32s mono_dec_stereo;

   synth = synth_buf + L_FILT_2k;

   nbits_pack = (tblNBitsMonoCore_16s[codec_mode] + NBITS_BWE) >> 2;

   mono_dec_stereo = 0;
   if (StbrMode >= 0) {
      if (n_channel == 1) {
         mono_dec_stereo = 1;
      }
      nbits_pack = nbits_pack + ((tblNBitsStereoCore_16s[StbrMode] + NBITS_BWE) >> 2);
   }

   mod = mod_buf+1;
   mod[-1] = st->last_mode;

   n_loss = 0;
   for (k=0; k<NB_DIV; k++) {
      if (bad_frame[k] == 0) {
         mod[k] = Bin2int(2, &serial[k*nbits_pack]);
      } else {
         mod[k] = -1;
      }
      n_loss = n_loss + bad_frame[k];
   }

   for(i=0; i<4; i++){
      bad_frame_hf[i] = bad_frame[i];
   }

   any_tcx80 = (mod[0] == 3);

   for (i=1; i<4; i++) {
      any_tcx80 |= (mod[i]==3);
   }

   if ((n_loss>2) && (any_tcx80)) {
      for (i=0; i<4; i++) {
         mod[i] = -1;
         bad_frame[i] = 1;
      }
   }

   if ( (mod[0]==3) || (mod[1]==3) || (mod[2]==3) || (mod[3]==3)) {
      for (k=0; k<NB_DIV; k++) {
         mod[k] = 3;
      }
   } else {
      if ((mod[0]==2) || (mod[1]==2)) {
         mod[0] = mod[1] = 2;
      }
      if ((mod[2]==2) || (mod[3]==2)) {
         mod[2] = mod[3] = 2;
      }

      for (k=0; k<NB_DIV; k++) {
         if (mod[k] < 0) {
            if (mod[k-1] == 0) {
               mod[k] = 0;
            } else {
               mod[k] = 1;
            }
         }
      }
   }


   Dec_prm(mod, bad_frame, serial, nbits_pack, codec_mode, param, nbits_AVQ);

   if ((n_channel==2) && (StbrMode >= 0)) {
      Dec_prm_stereo_x(bad_frame_hf, serial, nbits_pack, NBITS_BWE, prm_stereo, StbrMode, st);
   }

   Dec_prm_hf(mod, bad_frame, serial, nbits_pack, prm_hf_right);

   if ((n_channel==2) || (mono_dec_stereo==1)) {
      if (StbrMode < 0) {
         ippsCopy_16s(prm_hf_right, prm_hf_left, NPRM_BWE_DIV*NB_DIV);
      } else {
         Dec_prm_hf(mod, bad_frame, serial-NBITS_BWE/4, nbits_pack, prm_hf_left);
      }
   }

   if (Decoder_amrwb_plus_1(channel_right, channel_left, mod, param, prm_hf_right,
       prm_hf_left, nbits_AVQ, codec_mode, bad_frame, bad_frame_hf, AqLF,synth,
       st, n_channel, L_frame, fscale, mono_dec_stereo))
       return APIAMRWB_StsErr;

   if (n_channel == 2) {

      if (Decoder_stereo_x(prm_stereo, bad_frame_hf, sig_left, synth, AqLF,StbrMode,fscale, st))
         return APIAMRWB_StsErr;

      if (fscale == 0) {
         ippsHighPassFilter_AMRWB_16s_ISfs(sig_left, L_FRAME_PLUS, st->right.wmemSigOut, 14);
         ippsHighPassFilter_AMRWB_16s_ISfs(synth, L_FRAME_PLUS, st->left.wmemSigOut, 14);

         ippsUpsample_AMRWBE_16s(sig_left, channel_left, L_frame, st->left.wmem_oversamp, 0,1);
         ippsUpsample_AMRWBE_16s(synth, channel_right, L_frame, st->right.wmem_oversamp, 0,1);
         nb_samp = (Ipp32s)L_frame;
      } else {
         ippsBandJoinUpsample_AMRWBE_16s(sig_left, channel_left, L_FRAME_PLUS, channel_left, L_frame,
            st->left.wmem_oversamp, &(st->left.wover_frac), &nb_samp, upsamp_fscale);
         ippsBandJoinUpsample_AMRWBE_16s(synth, channel_right, L_FRAME_PLUS, channel_right,L_frame,
            st->right.wmem_oversamp, &(st->right.wover_frac), &nb_samp, upsamp_fscale);
      }
   } else { /* mono */
      if (fscale == 0) {
         ippsUpsample_AMRWBE_16s(synth, channel_right, L_frame, st->right.wmem_oversamp, 0,1);
         nb_samp = (Ipp32s)L_frame;
      } else {
         ippsBandJoinUpsample_AMRWBE_16s(synth, channel_right, L_FRAME_PLUS, channel_right,L_frame,
            st->right.wmem_oversamp, &(st->right.wover_frac), &nb_samp, upsamp_fscale);
      }
   }

   st->last_mode = mod[NB_DIV-1];

   *pSamplesCount = (Ipp32s)nb_samp;
   *pFrameScale = st->Old_Q_syn;
   return APIAMRWB_StsNoErr;
}

static void Dec_prm(
  Ipp16s mod[],
  Ipp32s bad_frame[],
  Ipp16s serial[],
  Ipp32s nbits_pack,
  Ipp32s codec_mode,
  Ipp16s param[],
  Ipp32s nbits_AVQ[]
)
{
  Ipp16s index0, index1, parity;
  Ipp16s *ptr, *prm;
  Ipp32s k, i, j, n_loss, nbits;
  Ipp32s nSubfarme, n, mode;

   nbits = (tblNBitsMonoCore_16s[codec_mode]>>2) - 2;

   k = 0;

   while (k < NB_DIV) {

      mode = mod[k];
      prm = param + (k*DEC_NPRM_DIV);

      if ((mode == 0) || (mode == 1)) {

         ptr = serial + (2 + (k*nbits_pack));

         prm[0] = Bin2int(8, ptr);      ptr += 8;
         prm[1] = Bin2int(8, ptr);      ptr += 8;
         prm[2] = Bin2int(6, ptr);      ptr += 6;
         prm[3] = Bin2int(7, ptr);      ptr += 7;
         prm[4] = Bin2int(7, ptr);      ptr += 7;
         prm[5] = Bin2int(5, ptr);      ptr += 5;
         prm[6] = Bin2int(5, ptr);      ptr += 5;

         if (mode == 0) { /* ACELP */
            j = 7;
            prm[j] = Bin2int(2, ptr);   ptr += 2;   j++;

            for (nSubfarme=0; nSubfarme<4; nSubfarme++) {
               if ((nSubfarme==0) || (nSubfarme==2)) {
                  n = 9;
               } else {
                  n = 6;
               }

               prm[j] = Bin2int(n, ptr);       ptr += n;    j++;
               prm[j] = Bin2int(1, ptr);       ptr += 1;    j++;

               if (codec_mode == MODE_9k6) {
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
               } else if (codec_mode == MODE_11k2) {
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
                  prm[j] = Bin2int(5, ptr);       ptr += 5;     j++;
               } else if (codec_mode == MODE_12k8) {
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
               } else if (codec_mode == MODE_14k4) {
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
                  prm[j] = Bin2int(9, ptr);       ptr += 9;     j++;
               } else if (codec_mode == MODE_16k) {
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
                  prm[j] = Bin2int(13, ptr);      ptr += 13;    j++;
               } else if (codec_mode == MODE_18k4) {
                  prm[j] = Bin2int(2, ptr);       ptr += 2;     j++;
                  prm[j] = Bin2int(2, ptr);       ptr += 2;     j++;
                  prm[j] = Bin2int(2, ptr);       ptr += 2;     j++;
                  prm[j] = Bin2int(2, ptr);       ptr += 2;     j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
               } else if (codec_mode == MODE_20k) {
                  prm[j] = Bin2int(10, ptr);      ptr += 10;    j++;
                  prm[j] = Bin2int(10, ptr);      ptr += 10;    j++;
                  prm[j] = Bin2int(2,  ptr);      ptr += 2;     j++;
                  prm[j] = Bin2int(2,  ptr);      ptr += 2;     j++;
                  prm[j] = Bin2int(10, ptr);      ptr += 10;    j++;
                  prm[j] = Bin2int(10, ptr);      ptr += 10;    j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
                  prm[j] = Bin2int(14, ptr);      ptr += 14;    j++;
               } else if (codec_mode == MODE_23k2) {
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
                  prm[j] = Bin2int(11, ptr);      ptr += 11;    j++;
               }
               prm[j] = Bin2int(7, ptr);       ptr += 7;    j++;
            }
         } else { /* TCX-20 */
            prm[7] = Bin2int(3, ptr);     ptr += 3;
            prm[8] = Bin2int(7, ptr);     ptr += 7;

            nbits_AVQ[k] = nbits - 56;
            Pack4bits(nbits_AVQ[k], ptr, prm+9);
         }
         k++;
      } else if (mode == 2) {/* TCX-40 */
         /* decode first 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[0] = Bin2int(8, ptr);    ptr += 8;
         prm[1] = Bin2int(8, ptr);    ptr += 8;
         prm[7] = Bin2int(3, ptr);    ptr += 3;
         prm[8] = Bin2int(7, ptr);    ptr += 7;
         nbits_AVQ[k] = nbits - 26;
         j = 9 + Pack4bits(nbits_AVQ[k], ptr, prm+9);
         k++;

         /* decode second 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[2] = Bin2int(6, ptr);    ptr += 6;
         prm[3] = Bin2int(7, ptr);    ptr += 7;
         prm[4] = Bin2int(7, ptr);    ptr += 7;
         prm[5] = Bin2int(5, ptr);    ptr += 5;
         prm[6] = Bin2int(5, ptr);    ptr += 5;

         if (bad_frame[k-1] == 1) {
            prm[8] = ShiftL_16s(Bin2int(6, ptr),1);
         }
         ptr += 6;

         nbits_AVQ[k] = nbits - (30+6);
         Pack4bits(nbits_AVQ[k], ptr, prm+j);
         k++;
      } else if (mode == 3) { /* TCX-80 */
         /* decode first 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[0] = Bin2int(8, ptr);    ptr += 8;
         prm[1] = Bin2int(8, ptr);    ptr += 8;
         prm[8] = Bin2int(7, ptr);    ptr += 7;
         nbits_AVQ[k] = nbits - 23;
         j = 9 + Pack4bits(nbits_AVQ[k], ptr, prm+9);
         k++;

         /* decode second 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[2] = Bin2int(6, ptr);    ptr += 6;
         prm[7] = Bin2int(3, ptr);    ptr += 3;

         parity = Bin2int(3, ptr);    ptr += 3;
         nbits_AVQ[k] = nbits - (9+3);
         j = j + Pack4bits(nbits_AVQ[k], ptr, prm+j);
         k++;

         /* decode third 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[3] = Bin2int(7, ptr);    ptr += 7;
         prm[5] = Bin2int(5, ptr);    ptr += 5;

         index0 = Bin2int(3, ptr);  ptr += 3;
         nbits_AVQ[k] = nbits - (12+3);
         j = j + Pack4bits(nbits_AVQ[k], ptr, prm+j);
         k++;

         /* decode fourth 20ms */
         ptr = serial + (2 + (k*nbits_pack));
         prm[4] = Bin2int(7, ptr);    ptr += 7;
         prm[6] = Bin2int(5, ptr);    ptr += 5;
         index1 = Bin2int(3, ptr);    ptr += 3;

         n_loss = 0;
         for (i=1; i<4; i++) {
            n_loss = n_loss + bad_frame[i];
         }

         if ((bad_frame[0]==1) && (n_loss<=1)) {
            if (bad_frame[2] == 1) {
               index0 = (Ipp16s)(parity^index1);
            }
            if (bad_frame[3]==1) {
               index1 = (Ipp16s)(parity^index0);
            }
            prm[8] = (Ipp16s)(ShiftL_16s(index0,4) + ShiftL_16s(index1,1));
         }

         nbits_AVQ[k] = nbits - (12+3);
         Pack4bits(nbits_AVQ[k], ptr, prm+j);
         k++;
      }
   }
}

static void Dec_prm_stereo_x(
  Ipp32s bad_frame[],
  Ipp16s serial[],
  Ipp32s nbits_pack,
  Ipp16s nbits_bwe,
  Ipp16s param[],
  Ipp32s  brMode,
  DecoderObj_AMRWBE* st)
{
   Ipp16s *prm, *mod, *ptr;
   IPP_ALIGNED_ARRAY(16, Ipp16s, mod_buf  , 1+NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp32s, nbits_AVQ, NB_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, prm_AVQ  , NBITS_MAX+N_PACK_MAX);
   Ipp32s j, k;
   Ipp32s nbits, hf_bits=6, hiband_mode;

   nbits = (tblNBitsStereoCore_16s[brMode] + (nbits_bwe <<1)) >> 2;

   hiband_mode = 0;
   if(tblNBitsStereoCore_16s[brMode] > (300+4)) {
      hiband_mode = 1;
   }

   mod = &mod_buf[1];
   mod[-1] = st->last_stereo_mode;

   for(k=0; k<NB_DIV; k++) {
      prm = param + k*NPRM_STEREO_HI_X;
      ptr = serial + (k+1)*nbits_pack - nbits;

      if(hiband_mode == 0) {
         prm[0] = Bin2int(4, ptr); ptr += 4;
         prm[1] = Bin2int(2, ptr);
      } else {
         prm[0] = Bin2int(4, ptr); ptr += 4;
         prm[1] = Bin2int(3, ptr); ptr += 3;
         prm[2] = Bin2int(5, ptr);
      }
   }

   if(hiband_mode == 0) {
      hf_bits = 4+2;
   } else {
      hf_bits = 7+5;
   }

   for(k=0; k<NB_DIV; k++) {
      ptr = serial + (k+1)*nbits_pack - nbits + hf_bits +1;
      if(bad_frame[k] == 0) {
         mod[k] = Bin2int(2, ptr);
      } else {
         mod[k] = -1;
      }
   }


   if ((mod[0]==3) || (mod[1]==3) || (mod[2]==3) || (mod[3]==3)) {
      for (k=0; k<NB_DIV; k++) {
         mod[k] = 3;
      }
   } else {
      if ((mod[0]==2) || (mod[1]==2)) {
         for (k=0; k<2; k++) {
            mod[k] = 2;
         }
      }
      if ((mod[2]==2) || (mod[3]==2)) {
         for (k=2; k<4; k++) {
            mod[k] = 2;
         }
      }
      for (k=0; k<NB_DIV; k++) {
         if (mod[k] < 0){
            if (mod[k-1] == 0) {
               mod[k] = 0;
            }else {
               mod[k] = 1;
            }
         }
      }
   }
   for (k=0; k<NB_DIV; k++) {
      param[NPRM_STEREO_HI_X*NB_DIV+k]=mod[k];
   }

   k = 0;
   while (k < NB_DIV) {

      prm = param + ((4+NPRM_STEREO_HI_X*NB_DIV) + (k*NPRM_DIV_TCX_STEREO));

      if ((mod[k]==1) || (mod[k]==0)) {
         nbits_AVQ[0] = (((tblNBitsStereoCore_16s[brMode]-4)>>2) - (7+7+2 + hf_bits));

         ptr = serial + ((((k+1)*nbits_pack - nbits) + hf_bits) + 2+1);

         prm[0] = Bin2int(7, ptr); ptr += 7;
         prm[1] = Bin2int(7, ptr); ptr += 7;

         Pack4bits(nbits_AVQ[0], ptr, prm_AVQ);

         ippsDecodeDemux_AMRWBE_16s(prm_AVQ, nbits_AVQ, &bad_frame[k], 1, &prm[2], TOT_PRM_20/8);

         k++;
      } else if (mod[k] == 2) {
         nbits_AVQ[0] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - ((2+7) + hf_bits);
         nbits_AVQ[1] = ((tblNBitsStereoCore_16s[brMode]-4)>>2) - ((2+7) + hf_bits);

         ptr = serial + (( (((k+1)*nbits_pack) - nbits) + hf_bits) + 2+1);

         prm[0] = Bin2int(7, ptr);    ptr += 7;

         j = Pack4bits(nbits_AVQ[0], ptr, prm_AVQ);

         ptr = serial + ((( ((k+2)*nbits_pack) - nbits ) + hf_bits) + 2+1);

         prm[1] = Bin2int(7, ptr);    ptr += 7;
         Pack4bits(nbits_AVQ[1], ptr, &prm_AVQ[j]);

         ippsDecodeDemux_AMRWBE_16s(prm_AVQ, nbits_AVQ, &bad_frame[k], 2, &prm[2], TOT_PRM_40/8);

         k+=2;
      } else if (mod[k] == 3) {
         nbits_AVQ[0] = ((tblNBitsStereoCore_16s[brMode]-4) >> 2) - (7+2+hf_bits);
         nbits_AVQ[1] = ((tblNBitsStereoCore_16s[brMode]-4) >> 2) - (2+hf_bits);
         nbits_AVQ[2] = ((tblNBitsStereoCore_16s[brMode]-4) >> 2) - (7+2+hf_bits);
         nbits_AVQ[3] = ((tblNBitsStereoCore_16s[brMode]-4) >> 2) - (2+hf_bits);

         ptr = serial + ((( ((k+1)*nbits_pack) - nbits) + hf_bits) + 2+1);
         prm[0] = Bin2int(7, ptr);    ptr += 7;
         j = Pack4bits(nbits_AVQ[0], ptr, prm_AVQ);

         ptr = serial + ((( ( (k+2)*nbits_pack ) - nbits) + hf_bits) + 2+1);
         j = j + Pack4bits(nbits_AVQ[1], ptr, &prm_AVQ[j]);

         ptr = serial + ((( ((k+3)*nbits_pack) - nbits) + hf_bits) + 2+1);

         prm[1] = Bin2int(7, ptr);
         ptr += 7;
         j = j + Pack4bits(nbits_AVQ[2], ptr, &prm_AVQ[j]);

         ptr = serial + ((( ((k+4)*nbits_pack) - nbits) + hf_bits) + 2+1);
         Pack4bits(nbits_AVQ[3], ptr, &prm_AVQ[j]);

         ippsDecodeDemux_AMRWBE_16s(prm_AVQ, nbits_AVQ, bad_frame, 4, &prm[2], TOT_PRM_80/8);
         k+=4;
      }
   }
}

static void Dec_prm_hf(
  Ipp16s *mod,
  Ipp32s *bad_frame,
  Ipp16s *serial,
  Ipp32s nbits_pack,
  Ipp16s *param
)
{
   Ipp16s *prm, *ptr;
   Ipp32s k, i, nbits, mode;

   nbits = NBITS_BWE/4;
   k = 0;
   while (k < NB_DIV) {
      mode = mod[k];
      prm = param + (k*NPRM_BWE_DIV);

      ptr = serial + ( ((k+1)*nbits_pack) - nbits );
      prm[0] = Bin2int(2, ptr);      ptr += 2;
      prm[1] = Bin2int(7, ptr);      ptr += 7;
      prm[2] = Bin2int(7, ptr);
      k++;

      if (mode == 2) {
         ptr = serial + ( ((k+1)*nbits_pack) - nbits);
         for (i=3; i<=10; i++) {
            prm[i] = Bin2int(2, ptr);
            ptr += 2;
         }
         k++;
      } else if (mode == 3) {
         ptr = serial + ( ((k+1)*nbits_pack) - nbits );
         for (i=3; i<=10; i++) {
            prm[i] = ShiftL_16s(Bin2int(2, ptr),1);
            ptr += 2;
         }
         k++;
         ptr = serial + ( ((k+1)*nbits_pack) - nbits);
         for (i=11; i<=18; i++) {
            prm[i] = ShiftL_16s(Bin2int(2, ptr),1);
            ptr += 2;
         }
         k++;
         ptr = serial + (((k+1)*nbits_pack) - nbits);
         if (bad_frame[k] == 0) {
            for (i=3; i<=18; i++) {
               prm[i] = Add_16s(prm[i],  Bin2int(1, ptr));
               ptr += 1;
            }
         }
         k++;
      }
   }
}

static APIAMRWB_Status Decoder_amrwb_plus_1(
    Ipp16s* chan_right,
    Ipp16s* chan_left,
    Ipp16s* mod,
    Ipp16s* param,
    Ipp16s* prm_hf_right,
    Ipp16s* prm_hf_left,
    Ipp32s* nbits_AVQ,
    Ipp32s  codec_mode,
    Ipp32s* bad_frame,
    Ipp32s* bad_frame_hf,
    Ipp16s* AqLF,
    Ipp16s* synth,
    DecoderObj_AMRWBE* st,
    Ipp32s n_channel,
    Ipp32s L_frame,
    Ipp16s fscale,
    Ipp32s mono_dec_stereo
)
{
   Ipp32s i;
   Ipp16s  *exc, *pPitchLag, *pPitchGain;
   IPP_ALIGNED_ARRAY(16, Ipp16s, exc_buf, PIT_MAX_MAX+L_INTERPOL+L_TCX);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pitchLag, NB_SUBFR_WBP+2);
   IPP_ALIGNED_ARRAY(16, Ipp16s, pitchGain, NB_SUBFR_WBP+2);

   /* initialize bass postfilter */
   pitchLag[0] = st->wold_T_pf[0];
   pitchGain[0] = st->wold_gain_pf[0];
   if (fscale) {
      pitchLag[1] = st->wold_T_pf[1];
      pitchGain[1] = st->wold_gain_pf[1];
      pPitchLag = &pitchLag[2];
      pPitchGain = &pitchGain[2];
   } else {
      pPitchLag = &pitchLag[1];
      pPitchGain = &pitchGain[1];
   }
   for (i=0; i<NB_SUBFR_WBP; i++) {
      pPitchLag[i] = L_SUBFR;
      pPitchGain[i] = 0;
   }

   exc = exc_buf + PIT_MAX_MAX + L_INTERPOL;
   ippsCopy_16s(st->wold_exc, exc_buf, PIT_MAX_MAX + L_INTERPOL);

   if (Decoder_lf(mod, param, nbits_AVQ, codec_mode, bad_frame, AqLF, exc, synth, pPitchLag, pPitchGain, fscale, st))
      return APIAMRWB_StsErr;

   Scale_mem2(synth, st->old_syn_q, st, 1);
   ippsCopy_16s(&synth[L_FRAME_PLUS-M], st->cp_old_synth, M);
   ippsCopy_16s(exc_buf+L_FRAME_PLUS, st->wold_exc, PIT_MAX_MAX+L_INTERPOL);

   exc = exc_buf;

   Scale_mem2(exc, st->old_subfr_q, st, 0);
   ippsDeemphasize_AMRWBE_NR_16s_I(PREEMPH_FAC_FX, 15, synth, L_FRAME_PLUS, &(st->wmem_deemph));
   ippsPostFilterLowBand_AMRWBE_16s(pitchLag, pitchGain, synth,
      st->wold_synth_pf, st->wold_noise_pf, &st->Old_bpf_scale, fscale);

   st->wold_T_pf[0] = pitchLag[NB_SUBFR_WBP];
   st->wold_gain_pf[0] = pitchGain[NB_SUBFR_WBP];
   if (fscale) {
      st->wold_T_pf[1] = pitchLag[NB_SUBFR_WBP+1];
      st->wold_gain_pf[1] = pitchGain[NB_SUBFR_WBP+1];
   }

   if ((fscale==0) && (n_channel==1)) {
      ippsHighPassFilter_AMRWB_16s_ISfs(synth, L_FRAME_PLUS, st->wmemSigOut, 14);
   }

   if (L_frame > L_FRAME8k) {
      Decoder_hf(mod, prm_hf_right, prm_hf_left, mono_dec_stereo, bad_frame_hf, AqLF, exc, chan_right,
         st->wmem_lpc_hf, &(st->wmem_gain_hf), &(st->wramp_state), &(st->right),  st->Old_Q_exc, fscale);

      if (n_channel == 1) {
         if(fscale == 0) {
            Delay(chan_right, L_FRAME_PLUS, DELAY_PF, st->right.wold_synth_hf);
         } else {
            Delay(chan_right, L_FRAME_PLUS, DELAY_PF+L_SUBFR, st->right.wold_synth_hf);
         }
      } else {
         if(fscale == 0) {
            Delay(chan_right, L_FRAME_PLUS, (D_BPF+L_BSP+2*D_NC+L_FDEL+32*D_STEREO_TCX/5) , st->right.wold_synth_hf);
         } else {
            Delay(chan_right, L_FRAME_PLUS, (D_BPF+L_SUBFR+L_BSP+2*D_NC+L_FDEL+32*D_STEREO_TCX/5) , st->right.wold_synth_hf);
         }
      }

      if (n_channel == 2) {
         Decoder_hf(mod, prm_hf_left, prm_hf_right, mono_dec_stereo, bad_frame_hf, AqLF, exc, chan_left,
            st->wmem_lpc_hf, &(st->wmem_gain_hf), &(st->wramp_state), &(st->left),  st->Old_Q_exc, fscale);

         if(fscale == 0) {
            Delay(chan_left, L_FRAME_PLUS,(D_BPF + L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5), st->left.wold_synth_hf);
         } else {
            Delay(chan_left, L_FRAME_PLUS,(D_BPF + L_SUBFR + L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5), st->left.wold_synth_hf);
         }
      }

      if (fscale == 0) {
         ippsUpsample_AMRWBE_16s(chan_right, chan_right, L_frame, st->right.wmem_oversamp_hf, 1, 0);

         if (n_channel == 2) {
            ippsUpsample_AMRWBE_16s(chan_left, chan_left, L_frame, st->left.wmem_oversamp_hf, 1,0);
         }
      }
   } else {
      ippsZero_16s( chan_right, L_frame );
      ippsZero_16s( chan_left, L_frame );
   }

   return APIAMRWB_StsNoErr;
}

static APIAMRWB_Status Decoder_stereo_x(Ipp16s *param, Ipp32s *bad_frame, Ipp16s *sig_left_,
      Ipp16s *synth_, Ipp16s *Az, Ipp32s StbrMode, Ipp16s fscale, DecoderObj_AMRWBE* st)
{

   IPP_ALIGNED_ARRAY(16, Ipp16s, my_old_synth_2k_, L_FRAME_2k + D_STEREO_TCX + L_FDEL_2k + 2*(D_NC*5)/32);
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_right_2k_   , L_FRAME_2k+2*L_FDEL_2k);
   IPP_ALIGNED_ARRAY(16, Ipp16s, old_left_2k_    , L_FRAME_2k+2*L_FDEL_2k);

   Ipp16s *my_synth_buf_   = sig_left_;
   Ipp16s *my_new_synth_   = my_synth_buf_+2*L_FDEL;
   Ipp16s *my_new_synth_2k_= my_old_synth_2k_ + D_STEREO_TCX + L_FDEL_2k + 2*(D_NC*5)/32;
   Ipp16s *my_old_synth_hi_= synth_;
   Ipp16s *my_new_synth_hi_= my_old_synth_hi_+2*D_NC;
   Ipp16s *my_synth_hi_t0_ = my_old_synth_hi_+2*D_NC+L_BSP+D_BPF;
   Ipp16s *new_right_2k_   = old_right_2k_ + 2*L_FDEL_2k;
   Ipp16s *new_left_2k_  = old_left_2k_ + 2*L_FDEL_2k;



   ippsCopy_16s(st->my_old_synth_fx, my_synth_buf_, 2*L_FDEL+20);
   ippsCopy_16s(synth_, my_new_synth_+20, L_FRAME_PLUS);
   ippsCopy_16s(st->my_old_synth_2k_fx, my_old_synth_2k_, D_STEREO_TCX + L_FDEL_2k + 2*(D_NC*5)/32);
   ippsCopy_16s(st->my_old_synth_hi_fx, my_old_synth_hi_, 2*D_NC);
   ippsCopy_16s(st->mem_left_2k_fx, old_left_2k_, 2*L_FDEL_2k);
   ippsCopy_16s(st->mem_right_2k_fx, old_right_2k_, 2*L_FDEL_2k);

   ippsBandSplit_AMRWBE_16s(my_new_synth_-(2*L_FDEL), my_new_synth_2k_-(2*L_FDEL_2k), my_new_synth_hi_, L_FRAME_PLUS);

   if (StbrMode < 0) {

      ippsAdd_16s(st->mem_stereo_ovlp_fx, my_old_synth_2k_, new_left_2k_, L_OVLP_2k);
      ippsSub_16s(st->mem_stereo_ovlp_fx, my_old_synth_2k_, new_right_2k_, L_OVLP_2k);
      ippsCopy_16s(&my_old_synth_2k_[L_OVLP_2k], &new_right_2k_[L_OVLP_2k], (L_FRAME_2k-L_OVLP_2k));
      ippsCopy_16s(&new_right_2k_[L_OVLP_2k], &new_left_2k_[L_OVLP_2k], (L_FRAME_2k-L_OVLP_2k));

      st->mem_stereo_ovlp_size_fx = 0;
      ippsZero_16s(st->mem_stereo_ovlp_fx,L_OVLP_2k);
      st->mem_balance_fx = 0;

      st->mem_stereo_ovlp_size_fx = L_OVLP_2k;
      st->last_stereo_mode = 0;
      st->side_rms_fx = 0;
   } else {
      if (Dec_tcx_stereo(my_old_synth_2k_,new_left_2k_,new_right_2k_,param+NPRM_STEREO_HI_X*NB_DIV,bad_frame,st)) {
         return APIAMRWB_StsErr;
      }
   }
   /* High band */
   {
      Ipp16s old_right_hi_[L_FRAME_PLUS+L_FDEL+HI_FILT_ORDER];
      Ipp16s *new_right_hi_ = old_right_hi_ + L_FDEL;
      Ipp16s old_left_hi_[L_FRAME_PLUS+L_FDEL];
      Ipp16s *new_left_hi_ = old_left_hi_ + L_FDEL;

      if (tblNBitsStereoCore_16s[StbrMode] > (300+4)) {
         st->Filt_hi_pmsvq_fx = &mspqFiltHighBand;
         st->Gain_hi_pmsvq_fx = &mspqGainHighBand;
      } else {
         st->Filt_hi_pmsvq_fx = &mspqFiltLowBand;
         st->Gain_hi_pmsvq_fx = &mspqGainLowBand;
      }

      ippsCopy_16s(st->mem_left_hi_fx,old_left_hi_,L_FDEL);
      ippsCopy_16s(st->mem_right_hi_fx,old_right_hi_,L_FDEL);

      if (StbrMode < 0) {
         ippsCopy_16s(&my_synth_hi_t0_[-L_BSP-D_BPF], new_right_hi_, L_FRAME_PLUS);
         ippsCopy_16s(&my_synth_hi_t0_[-L_BSP-D_BPF], new_left_hi_, L_FRAME_PLUS);
         Init_dec_hi_stereo(st);
      } else {
         Dec_hi_stereo(my_synth_hi_t0_,new_right_hi_,new_left_hi_,Az,param,bad_frame,fscale,st);
      }
      Delay(new_right_hi_,L_FRAME_PLUS,D_NC,st->right.wmem_d_nonc);
      Delay(new_left_hi_,L_FRAME_PLUS,D_NC,st->left.wmem_d_nonc);

      Delay(new_right_hi_,L_FRAME_PLUS,D_NC+(D_STEREO_TCX*32/5),st->right.mem_d_tcx_fx);
      Delay(new_left_hi_,L_FRAME_PLUS,D_NC+(D_STEREO_TCX*32/5),st->left.mem_d_tcx_fx);

      ippsCopy_16s(&my_synth_buf_[L_FRAME_PLUS],st->my_old_synth_fx,2*L_FDEL+20);
      ippsBandJoin_AMRWBE_16s(new_left_2k_-(2*L_FDEL_2k), new_left_hi_-L_FDEL, sig_left_, L_FRAME_PLUS);

      ippsCopy_16s(&my_old_synth_hi_[L_FRAME_PLUS],st->my_old_synth_hi_fx,2*D_NC);
      ippsBandJoin_AMRWBE_16s(new_right_2k_-(2*L_FDEL_2k), new_right_hi_-L_FDEL, synth_, L_FRAME_PLUS);

      ippsCopy_16s(&old_left_hi_[L_FRAME_PLUS],st->mem_left_hi_fx,L_FDEL);
      ippsCopy_16s(&old_right_hi_[L_FRAME_PLUS],st->mem_right_hi_fx,L_FDEL);
   }

   ippsCopy_16s(&my_old_synth_2k_[L_FRAME_2k],st->my_old_synth_2k_fx,L_FDEL_2k + D_STEREO_TCX+2*(D_NC*5)/32);
   ippsCopy_16s(&old_left_2k_[L_FRAME_2k],st->mem_left_2k_fx,2*L_FDEL_2k);
   ippsCopy_16s(&old_right_2k_[L_FRAME_2k],st->mem_right_2k_fx,2*L_FDEL_2k);

   return APIAMRWB_StsNoErr;
}

static APIAMRWB_Status Decoder_lf(
  Ipp16s mod[],
  Ipp16s param[],
  Ipp32s nbits_AVQ[],
  Ipp32s codec_mode,
  Ipp32s bad_frame[],
  Ipp16s AzLF[],
  Ipp16s exc_[],
  Ipp16s syn[],
  Ipp16s pitch[],
  Ipp16s pit_gain[],
  Ipp16s pit_adj,
  DecoderObj_AMRWBE *st
)
{
   Ipp32s bfi, bfi_2nd_st;
   Ipp16s *prm, *synth_;
   Ipp32s len;
   Ipp16s tmp, tmp16;
   Ipp16s stab_fac;
   Ipp32s Ltmp;
   Ipp32s i, k, mode;

   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew_, M);
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew_, M);
   IPP_ALIGNED_ARRAY(16, Ipp16s, Az, (NB_SUBFR_WBP+1)*(M+1));


   if(pit_adj == 0) {
      len = L_DIV;
      st->i_offset = 0;
   } else {
      Ltmp = Add_32s(FSCALE_DENOM, Mul2_Low_32s(pit_adj*PIT_MIN_12k8));
      tmp = Div_16s((Ipp16s)(ShiftL_32s(Ltmp, 8)>>16), 24576);
      tmp16 = (Ipp16s)(tmp + 1);
      Ltmp = Sub_32s(Ltmp, Mul2_Low_32s(tmp16*FSCALE_DENOM));
      tmp = (Ipp16s)(tmp - PIT_MIN_12k8);
      if (Ltmp >= 0) {
         tmp = (Ipp16s)(tmp16 - PIT_MIN_12k8);
      }
      st->i_offset = tmp;
      len = (6*tmp) + (PIT_MAX_12k8+L_INTERPOL);
      if (len < L_DIV)
         len = L_DIV;
   }

   synth_ = syn;

   ippsCopy_16s(st->wold_synth, syn-M, M);

   k = 0;
   while (k < NB_DIV) {

      mode = mod[k];
      bfi = bad_frame[k];
      bfi_2nd_st = 0;

      if ((mode == 2) && (bad_frame[k+1] != 0))
         bfi_2nd_st = (1+2+4+8+16);

      if (mode == 3) {
         if (bad_frame[k+1] != 0)
            bfi_2nd_st = 1;

         if (bad_frame[k+2] != 0)
            bfi_2nd_st = bfi_2nd_st + (2+8);

         if (bad_frame[k+3] != 0)
            bfi_2nd_st = bfi_2nd_st + (4+16);
      }

      prm = param + (k*DEC_NPRM_DIV);

      ippsISFQuantDecode_AMRWBE_16s(prm, isfnew_, st->wpast_isfq, st->wisfold, st->wisf_buf, bfi, bfi_2nd_st);

      if(bfi == 0 && bfi_2nd_st == 0)
         st->seed_tcx = (Ipp16s)(ShiftL_32s(Add_32s(Mul2_Low_32s(prm[0]*prm[1]), Mul2_Low_32s(prm[4]*prm[5])),7));
      prm += 7;

      ippsISFToISP_AMRWB_16s(isfnew_, ispnew_, M);

      Ltmp = 0;
      for (i=0; i<(M-1); i++) {
         tmp = Sub_16s(isfnew_[i], st->wisfold[i]);
         Ltmp = Add_32s(Ltmp, Mul2_Low_32s(tmp*tmp));
      }
      tmp = (Ipp16s)(ShiftL_32s(Ltmp, 8)>>16);
      tmp = (Ipp16s)((tmp*26214)>>15);

      tmp = Sub_16s(20480, tmp);
      stab_fac = ShiftL_16s(tmp, 1);

      if (stab_fac < 0)
         stab_fac = 0;

      if ((st->prev_lpc_lost != 0) && (bfi == 0)) {
         for (i=0; i<M-1; i++) {
            if (ispnew_[i] < st->wispold[i]) {
               st->wispold[i] = ispnew_[i];
            }
         }
      }

      switch (mode) {
      case 0:
      case 1:
         ownIntLPC(st->wispold, ispnew_, tblInterpolTCX20_16s, Az, 4, M);
         ippsCopy_16s(Az, &AzLF[(k*4)*(M+1)], 5*(M+1));
         if (mode == 0) {
            Decoder_acelp(prm, L_DIV, codec_mode, bad_frame[k],&pitch[k*4],
               &pit_gain[k*4], Az, &exc_[k*L_DIV], &synth_[k*L_DIV],pit_adj, len, stab_fac,st);
            st->last_mode = 0;
            ippsMove_16s(&exc_[k*L_DIV],&exc_[k*L_DIV-(PIT_MAX_MAX + L_INTERPOL)], L_DIV);
         } else {
            tmp16 = st->Q_exc;
            if (Decoder_tcx(prm, &nbits_AVQ[k], Az, L_DIV, bad_frame+k, &exc_[k*L_DIV], &synth_[k*L_DIV], st)) {
               return APIAMRWB_StsErr;
            }
            st->last_mode =1;
            ippsMove_16s(&exc_[k*L_DIV],&exc_[k*L_DIV-(PIT_MAX_MAX + L_INTERPOL)], L_DIV);
            ownScaleSignal_AMRWB_16s_ISfs(&exc_[k*L_DIV+L_DIV-(PIT_MAX_MAX + L_INTERPOL)],
               (PIT_MAX_MAX + L_INTERPOL)-L_DIV, (Ipp16s)(st->Q_exc-tmp16));
         }
         Updt_mem_q(st->old_syn_q, 1, st->Q_syn);
         Updt_mem_q(st->old_subfr_q, 1, st->Q_exc);
         k++;
         break;

      case 2:
         ownIntLPC(st->wispold, ispnew_, tblInterpolTCX40_16s, Az, 8, M);
         ippsCopy_16s(Az, &AzLF[(k*4)*(M+1)], 9*(M+1));

         if (Decoder_tcx(prm, &nbits_AVQ[k], Az, 2*L_DIV, bad_frame+k, &exc_[k*L_DIV], &synth_[k*L_DIV], st)) {
            return APIAMRWB_StsErr;
         }
         st->last_mode = 2;
         Updt_mem_q(st->old_subfr_q, 2, st->Q_exc);
         Updt_mem_q(st->old_syn_q, 2, st->Q_syn);
         ippsMove_16s(&exc_[k*L_DIV],&exc_[k*L_DIV-(PIT_MAX_MAX + L_INTERPOL)], 2*L_DIV);
         k+=2;
         break;

      case 3:
         ownIntLPC(st->wispold, ispnew_, tblInterpolTCX80_16s, Az, 16, M);
         ippsCopy_16s(Az, &AzLF[(k*4)*(M+1)], 17*(M+1));

         if (Decoder_tcx(prm, &nbits_AVQ[k], Az, 4*L_DIV, bad_frame+k, &exc_[k*L_DIV], &synth_[k*L_DIV], st)) {
            return APIAMRWB_StsErr;
         }
         st->last_mode = 3;
         Updt_mem_q(st->old_subfr_q, 4, st->Q_exc);
         Updt_mem_q(st->old_syn_q, 4, st->Q_syn);
         ippsMove_16s(&exc_[k*L_DIV],&exc_[k*L_DIV-(PIT_MAX_MAX + L_INTERPOL)], 4*L_DIV);
         k+=4;
         break;
      default:
         return APIAMRWB_StsErr;
      }

      st->prev_lpc_lost = (Ipp16s)bfi;

      ippsCopy_16s(ispnew_, st->wispold, M);
      ippsCopy_16s(isfnew_, st->wisfold, M);
   }

   ippsCopy_16s(&syn[L_FRAME_PLUS-M], st->wold_synth, M);
   ippsCopy_16s(synth_, syn, L_FRAME_PLUS);

   return APIAMRWB_StsNoErr;
}

static void Decoder_hf(Ipp16s* mod, Ipp16s* param, Ipp16s* param_other, Ipp32s mono_dec_stereo,
      Ipp32s* bad_frame, Ipp16s* AqLF, Ipp16s* exc, Ipp16s* synth_hf, Ipp16s* mem_lpc_hf,
      Ipp16s* mem_gain_hf, Ipp16s* ramp_state, DecoderOneChanelState_AMRWBE *st,
      Ipp16s QexcHF, Ipp16s fscale)
{

   IPP_ALIGNED_ARRAY(16, Ipp16s, ispnew, MHF);
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew_other, MHF);
   IPP_ALIGNED_ARRAY(16, Ipp16s, isfnew, MHF);
   IPP_ALIGNED_ARRAY(16, Ipp16s, Aq, (NB_SUBFR_WBP+1)*(MHF+1));
   IPP_ALIGNED_ARRAY(16, Ipp16s, gain_hf, NB_SUBFR_WBP);
   IPP_ALIGNED_ARRAY(16, Ipp16s, gain_hf_tmp, NB_SUBFR_WBP);
   IPP_ALIGNED_ARRAY(16, Ipp16s, gain_hf_other, NB_SUBFR_WBP);
   IPP_ALIGNED_ARRAY(16, Ipp16s, HF, L_SUBFR);

   Ipp16s gain16, *prm_in_other;
   Ipp16s *p_Aq, tmp16, exp_g, frac_g;
   Ipp32s bfi;
   Ipp32s i, k, mode, i_subfr, sf, nsf;
   Ipp16s *prm, *prm_other;
   Ipp16s thr_lo, thr_hi;
   const Ipp16s *interpol_wind;
   Ipp32s Ltmp,Lgain_sum,Lgain_sum_other, gdiff;

   prm_other = param_other;
   prm_in_other = prm_other;

   for (k=0; k<NB_DIV; k++) {
      bfi = bad_frame[k];
      mode = mod[k];

      if (mode == 0 || (mode == 1)) {
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

      if(mono_dec_stereo == 1) {
         prm_other = param_other + (k*NPRM_BWE_DIV);
         prm_in_other = prm_other;
         Lgain_sum = 0;
         Lgain_sum_other = 0;
      }

      ippsISFQuantDecodeHighBand_AMRWBE_16s(prm, st->wpast_q_isf_hf, isfnew, bfi, fscale);

      if(mono_dec_stereo == 0) {
         ippsISFToISP_AMRWB_16s(isfnew, ispnew, MHF);
         ownIntLPC(st->wispold_hf, ispnew, interpol_wind, Aq, nsf, MHF);
         ippsCopy_16s(ispnew, st->wispold_hf, MHF);

         gain16 = Match_gain_6k4(&AqLF[((k*4)+nsf)*(M+1)], &Aq[nsf*(MHF+1)]);
         Int_gain(st->wold_gain, gain16, interpol_wind, gain_hf, nsf);
         st->wold_gain = gain16;
      }

      D_gain_chan(gain_hf, &(st->wpast_q_gain_hf),prm, bfi, mode, &bad_frame[k], &Lgain_sum, mono_dec_stereo );

      if(mono_dec_stereo == 1) {
         prm_other += D_gain_chan(gain_hf_other, &(st->wpast_q_gain_hf_other),
            prm_other, bfi, mode, &bad_frame[k], &Lgain_sum_other, mono_dec_stereo);

         thr_lo = (Ipp16s)(Mul2_Low_32s((-10) * nsf));
         thr_hi = Negate_16s(thr_lo);

         gdiff = Sub_32s(Lgain_sum , Lgain_sum_other);

         ippsISFQuantDecodeHighBand_AMRWBE_16s(prm_in_other, st->wpast_q_isf_hf_other, isfnew_other, bfi, fscale);

         if (gdiff < ShiftL_32s(thr_lo,8)) {
            for (i=0; i<MHF; i++) {
               isfnew[i] = isfnew_other[i];
            }
         } else if (gdiff < ShiftL_32s(thr_hi,8)) {
            for (i=0; i<MHF; i++) {
               Ltmp = Mul2_Low_32s(isfnew[i] * 16384);
               Ltmp = Add_32s(Ltmp, Mul2_Low_32s(isfnew_other[i]*16384));
               isfnew[i] = Cnvrt_NR_32s16s(Ltmp);
            }
         }
         ippsISFToISP_AMRWB_16s(isfnew, ispnew, MHF);
         ownIntLPC(st->wispold_hf, ispnew, interpol_wind, Aq, nsf, MHF);
         ippsCopy_16s(ispnew, st->wispold_hf, MHF);

         gain16 = Match_gain_6k4(&AqLF[((k*4)+nsf)*(M+1)], &Aq[nsf*(MHF+1)]);
         Int_gain(st->wold_gain, gain16, interpol_wind, gain_hf_tmp, nsf);
         for(i=0;i<nsf;i++) {
            gain_hf[i] = Add_16s(gain_hf[i], gain_hf_tmp[i]);
            gain_hf_other[i] = Add_16s(gain_hf_other[i], gain_hf_tmp[i]);
         }
         st->wold_gain = gain16;
      }

      ippsCopy_16s(&Aq[nsf*(MHF+1)],mem_lpc_hf, MHF+1);

      p_Aq = Aq;
      for (sf=0; sf<nsf; sf++) {
         i_subfr = ((k<<2) + sf) << 6;
         Ltmp = Mul2_Low_32s(gain_hf[sf] * 21771);
         Ltmp = (Ltmp >> 10);
         Unpack_32s(Ltmp, &exp_g, &frac_g);
         tmp16 = (Ipp16s)ownPow2_AMRWB(11, frac_g);

         if (exp_g > 0) {
            gain16 = ShiftL_16s(tmp16, exp_g);
         } else {
            gain16 = (Ipp16s)(tmp16 >> (-exp_g));
         }

         if(mono_dec_stereo == 1) {
            Ltmp = Mul2_Low_32s(gain_hf_other[sf] * 21771);
            Ltmp = (Ltmp >> 10);
            Unpack_32s(Ltmp, &exp_g, &frac_g);
            tmp16 = (Ipp16s)ownPow2_AMRWB(11, frac_g);
            if (exp_g > 0) {
               tmp16 = ShiftL_16s(tmp16, exp_g);
            } else {
               tmp16 = (Ipp16s)(tmp16 >> (-exp_g));
            }
            Ltmp = Mul2_Low_32s(tmp16 * 16384);
            Ltmp = Add_32s(Ltmp, Mul2_Low_32s(gain16*16384));
            gain16 = Cnvrt_NR_32s16s(Ltmp);
         }

         if (*ramp_state < 64) {
            gain16 = MulHR_16s(gain16, tblGainRampHF[*ramp_state]);
            *ramp_state = Add_16s(*ramp_state, 1);
         }

         *mem_gain_hf = gain16;
         for (i=0; i<L_SUBFR; i++) {
            Ltmp = Mul2_Low_32s(gain16 * exc[i+i_subfr]);
            HF[i] = Cnvrt_NR_32s16s(ShiftL_32s(Ltmp, 4));
         }

         ownSoftExcitationHF(HF, &(st->Lp_amp));

         {
            Ipp16s wTmp, scale;
            wTmp = p_Aq[0];
            scale = (Ipp16s)(QexcHF - st->Q_synHF);
            if (scale >= 0)
               p_Aq[0] >>= scale;
            else
               p_Aq[0] <<= (-scale);
            ippsSynthesisFilter_G729E_16s_I(p_Aq, MHF, HF, L_SUBFR, st->wmem_syn_hf);
            ippsCopy_16s(&HF[L_SUBFR-MHF], st->wmem_syn_hf, MHF);
            p_Aq[0] = wTmp;
         }
         ownSmoothEnergyHF(HF, &(st->Lthreshold));
         for (i=0; i<L_SUBFR; i++) {
            synth_hf[i+i_subfr] = HF[i];
         }
         p_Aq += (MHF+1);
      }

      if (mode == 2) {
         k = k + 1;
      } else if (mode == 3) {
         k = k + 3;
      }
   }
}

static APIAMRWB_Status Dec_tcx_stereo(Ipp16s* synth_2k, Ipp16s* left_2k, Ipp16s* right_2k,
      Ipp16s* param, Ipp32s* bad_frame, DecoderObj_AMRWBE *st)
{
  Ipp32s k, start, end;
  Ipp16s *prm;

  IPP_ALIGNED_ARRAY(16, Ipp16s, synth_side, L_FRAME_2k);
  IPP_ALIGNED_ARRAY(16, Ipp32s, mod, 4);

  for(k=0;k<4;k++)
  {
    mod[k] = param[k];
  }

   k = 0;
   while (k < NB_DIV) {
      prm = param+ 4 + (k*NPRM_DIV_TCX_STEREO);

      if ((mod[k]==1) || (mod[k] == 0)) {
         if (Dtcx_stereo(&synth_side[k*L_DIV_2k], &synth_2k[k*L_DIV_2k], st->mem_stereo_ovlp_fx,
            st->mem_stereo_ovlp_size_fx, L_FRAME_2k/4, prm, !mod[k], &bad_frame[k], st)) {
               return APIAMRWB_StsErr;
         }
         st->mem_stereo_ovlp_size_fx = L_OVLP_2k/4;
         k++;
      } else if (mod[k] == 2) {
         if (Dtcx_stereo(&synth_side[k*L_DIV_2k], &synth_2k[k*L_DIV_2k],st->mem_stereo_ovlp_fx,
            st->mem_stereo_ovlp_size_fx, L_FRAME_2k/2, prm, 0, bad_frame+k, st)) {
               return APIAMRWB_StsErr;
         }
         st->mem_stereo_ovlp_size_fx = L_OVLP_2k/2;
         k = k + 2;
      } else if(mod[k] == 3) {
         if (Dtcx_stereo(&synth_side[k*L_DIV_2k], &synth_2k[k*L_DIV_2k],st->mem_stereo_ovlp_fx,
            st->mem_stereo_ovlp_size_fx, L_FRAME_2k, prm, 0, bad_frame+k, st)) {
               return APIAMRWB_StsErr;
         }
         st->mem_stereo_ovlp_size_fx = L_OVLP_2k;
         k = k + 4;
      } else {
         return APIAMRWB_StsErr;
      }
   }

   k=0;
   while (k < NB_DIV) {
      if ((mod[k]==0) || (mod[k]==1)) {
         start = k * (L_FRAME_2k/4);
         end = (L_FRAME_2k/4) + (k*L_FRAME_2k/4) ;
         k ++;
      } else if(mod[k] == 2) {
         start = k*(L_FRAME_2k/4);
         end = (L_FRAME_2k/2) + (k*L_FRAME_2k/4);
         k = k + 2;
      } else {
         start = 0;
         end = L_FRAME_2k;
         k += 4;
      }
      ippsAdd_16s(&synth_side[start], &synth_2k[start], &left_2k[start], (end-start));
      ippsSub_16s(&synth_side[start], &synth_2k[start], &right_2k[start], (end-start));
   }
   return APIAMRWB_StsNoErr;
}

static APIAMRWB_Status Dtcx_stereo(
  Ipp16s* synth,
  Ipp16s* mono,
  Ipp16s* wovlp,
  Ipp32s ovlp_size,
  Ipp32s L_frame,
  Ipp16s* prm,
  Ipp32s pre_echo,
  Ipp32s* bad_frame,
  DecoderObj_AMRWBE* st)
{
  Ipp32s i, lg;
  Ipp32s any_bad_frame;
  IPP_ALIGNED_ARRAY(16, Ipp16s, xri, L_TCX_LB);
  Ipp16s *xnq=xri;
  IPP_ALIGNED_ARRAY(16, Ipp16s, wm_arr, L_TCX_LB+ECU_WIEN_ORD);
  IPP_ALIGNED_ARRAY(16, Ipp16s, window, 32+32);

  Ipp16s *wm = &wm_arr[ECU_WIEN_ORD];
  Ipp16s wbalance;
  IPP_ALIGNED_ARRAY(16, Ipp16s, gain_shap, 8);
  IPP_ALIGNED_ARRAY(16, Ipp16s, rmm_h, ECU_WIEN_ORD+1);
  IPP_ALIGNED_ARRAY(16, Ipp16s, rmm_l, ECU_WIEN_ORD+1);
  IPP_ALIGNED_ARRAY(16, Ipp16s, rms_h, ECU_WIEN_ORD+1);
  IPP_ALIGNED_ARRAY(16, Ipp16s, rms_l, ECU_WIEN_ORD+1);

  Ipp16s *h;
  Ipp16s ifft3_scl;
  Ipp32s Lgain;
  Ipp32s lext=32;
  Ipp32s n_pack = 4;
  Ipp32s inc2;

  Ipp16s rms_shift,rmm_shift;

   ifft3_scl = 5;

   for (i=0; i<ECU_WIEN_ORD; i++) {
      wm_arr[i] = 0;
   }

   h = st->h_fx;

   switch (L_frame) {
   case 40:
      lext = 8;  n_pack=1;  inc2 = 16;  break;
   case 80:
      lext = 16;  n_pack =2;  inc2 = 8;  break;
   case 160:
      lext = 32;  n_pack= 4;  inc2 = 4;  break;
   default:
      return APIAMRWB_StsErr;
   };

   lg = L_frame + lext;

   any_bad_frame = 0;
   for (i=0; i<n_pack; i++) {
      any_bad_frame |= bad_frame[i];
   }

   Cos_window(window, ovlp_size, lext, inc2);

   ippsCopy_16s(&prm[2], xri, lg);
   Scale_tcx_ifft(xri, lg, &ifft3_scl);

   ippsCopy_16s(mono, wm, lg);

   ownWindowing(window, wm, ovlp_size);
   ownWindowing(&window[ovlp_size], &wm[L_frame], lext);

   if(pre_echo > 0) {
      Comp_gain_shap(wm, gain_shap, lg, st->Old_Q_syn);
   }

   Adap_low_freq_deemph(xri, (lg<<2));
   xri[1]=0;

   ippsFFTInv_PermToR_AMRWBE_16s(xri, xnq, lg);

   wbalance = Balance(bad_frame[0], &(st->mem_balance_fx), prm[0]);
   if (pre_echo > 0) {
      Apply_gain_shap(lg, xnq, gain_shap);
   }

   ippsGainDecodeTCX_AMRWBE_16s(xnq, lg, prm[1], any_bad_frame, &(st->side_rms_fx), &Lgain);

   if ((L_frame!=40) || (bad_frame[0]==0) ) {
      Apply_xnq_gain2(lg, wbalance, Lgain, xnq, wm, st->Old_Q_syn);
   } else {
      Apply_wien_filt(lg, xnq, h, wm);
   }

   if (any_bad_frame == 0 ) {
      Crosscorr_2(wm, wm, rmm_h, rmm_l, lg, 0, ECU_WIEN_ORD+1, &rmm_shift, 0);
      Crosscorr_2(xnq, wm, rms_h, rms_l, lg, 0, ECU_WIEN_ORD+1, &rms_shift , 1);
      Glev_s(h, rmm_h, rmm_l, rms_h, rms_l, ECU_WIEN_ORD+1, (Ipp16s)(rmm_shift-rms_shift));
   }

   ownWindowing(window, xnq, ovlp_size);
   ownWindowing(&window[ovlp_size], &xnq[L_frame], lext);

   Apply_tcx_overlap(xnq, wovlp, lext, L_frame);

   ippsCopy_16s(xnq, synth, L_frame);
   return APIAMRWB_StsNoErr;
}

static void Dec_hi_stereo(
  Ipp16s synth_hi_t0[],
  Ipp16s right_hi[],
  Ipp16s left_hi[],
  Ipp16s AqLF[],
  Ipp16s  param[],
  Ipp32s bad_frame[],
  Ipp16s fscale,
  DecoderObj_AMRWBE*st
  )
{
  Ipp16s *exc_buf = right_hi;
  Ipp16s *exc_mono=exc_buf+HI_FILT_ORDER;
  Ipp16s *synth_hi_d;

  Ipp16s *p_Aq,*p_h;
  Ipp32s i,k;
  Ipp16s i_subfr;
  Ipp16s start;
  Ipp16s *left_exc, *right_exc;
  Ipp16s *prm;
  Ipp16s wTmp;

  IPP_ALIGNED_ARRAY(16, Ipp16s, gain_left     , NB_DIV+2);
  IPP_ALIGNED_ARRAY(16, Ipp16s, gain_right    , NB_DIV+2);
  IPP_ALIGNED_ARRAY(16, Ipp16s, new_gain_left , NB_DIV);
  IPP_ALIGNED_ARRAY(16, Ipp16s, new_gain_right, NB_DIV);
  IPP_ALIGNED_ARRAY(16, Ipp16s, side_buf      , L_DIV+L_SUBFR);
  IPP_ALIGNED_ARRAY(16, Ipp16s, buf           , L_SUBFR);
  IPP_ALIGNED_ARRAY(16, Ipp16s, wh            , NB_DIV*HI_FILT_ORDER);

  left_exc = left_hi;
  right_exc = right_hi;

   for(i=0;i<NB_DIV;i++) {
      prm = param + i*NPRM_STEREO_HI_X;
      Dec_filt(prm,&wh[i*HI_FILT_ORDER],st->old_wh_q_fx, bad_frame[i],st->Filt_hi_pmsvq_fx);
      prm += st->Filt_hi_pmsvq_fx->nstages;
      Dec_gain(prm,&new_gain_left[i],&new_gain_right[i],st->old_gm_gain_fx, bad_frame[i],st->Gain_hi_pmsvq_fx);
      new_gain_left[i] = Comp_hi_gain(new_gain_left[i]);
      new_gain_right[i] = Comp_hi_gain(new_gain_right[i]);
   }

   ippsCopy_16s(st->old_gain_left_fx,gain_left,2);
   ippsCopy_16s(st->old_gain_right_fx,gain_right,2);

   ippsCopy_16s(new_gain_left,gain_left+2,4);
   ippsCopy_16s(new_gain_right,gain_right+2,4);

   ippsCopy_16s(new_gain_left+2,st->old_gain_left_fx,2);
   ippsCopy_16s(new_gain_right+2,st->old_gain_right_fx,2);

   synth_hi_d = &synth_hi_t0[-L_BSP-D_BPF];
   p_Aq = st->old_AqLF_fx;

   ippsResidualFilter_Low_16s_Sfs(p_Aq, M, synth_hi_d, exc_mono, L_SUBFR/2, 11);

   p_Aq += (M+1);

   for(i_subfr=L_SUBFR/2; i_subfr<(3*L_SUBFR+L_SUBFR/2); i_subfr+=L_SUBFR) {
      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &synth_hi_d[i_subfr], &exc_mono[i_subfr], L_SUBFR, 11);
      p_Aq += (M+1);
   }

   if (fscale != 0) {
      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &synth_hi_d[i_subfr], &exc_mono[i_subfr], L_SUBFR, 11);
   }

   p_Aq = AqLF;

   if(fscale==0) {
      start = 3*L_SUBFR + L_SUBFR/2;
   } else {
      start = 4*L_SUBFR + L_SUBFR/2;
   }

   for (i_subfr=start; i_subfr<(L_FRAME_PLUS-L_SUBFR/2); i_subfr+=L_SUBFR) {
      ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &synth_hi_d[i_subfr], &exc_mono[i_subfr], L_SUBFR, 11);
      p_Aq += (M+1);
   }

   ippsResidualFilter_Low_16s_Sfs(p_Aq, M, &synth_hi_d[L_FRAME_PLUS-L_SUBFR/2],
      &exc_mono[L_FRAME_PLUS-L_SUBFR/2], L_SUBFR/2, 11);

   ippsCopy_16s(st->old_exc_mono_fx,exc_buf,HI_FILT_ORDER);
   ippsCopy_16s(exc_buf+L_FRAME_PLUS,st->old_exc_mono_fx,HI_FILT_ORDER);

  if(fscale ==0 )
  {
    Fir_filt(st->old_wh_fx,HI_FILT_ORDER,exc_mono,side_buf,L_DIV-L_SUBFR/2);
    Fir_filt(st->old_wh2_fx,HI_FILT_ORDER,exc_mono,buf,L_SUBFR/2);

    Get_exc_win(left_exc, buf,exc_mono,side_buf,&st->W_window[L_SUBFR/2],gain_left, L_SUBFR/2,1);
    Get_exc_win(right_exc,buf,exc_mono,side_buf,&st->W_window[L_SUBFR/2],gain_right,L_SUBFR/2,0);

    Get_exc(&left_exc[L_SUBFR/2],&exc_mono[L_SUBFR/2],&side_buf[L_SUBFR/2],gain_left[1],L_DIV-L_SUBFR,1);
    Get_exc(&right_exc[L_SUBFR/2],&exc_mono[L_SUBFR/2],&side_buf[L_SUBFR/2],gain_right[1],L_DIV-L_SUBFR,0);

    Fir_filt(st->old_wh_fx,HI_FILT_ORDER,&exc_mono[L_DIV-L_SUBFR/2],buf,L_SUBFR);

    p_h = wh;
    k = 1;
    for(i_subfr=L_DIV-L_SUBFR/2;i_subfr<L_FRAME_PLUS-L_SUBFR/2;i_subfr+=L_DIV)
    {
      Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[i_subfr],side_buf,L_DIV);

      Get_exc_win(&left_exc[i_subfr], buf,&exc_mono[i_subfr],side_buf,st->W_window,&gain_left[k],L_SUBFR,1);
      Get_exc_win(&right_exc[i_subfr],buf,&exc_mono[i_subfr],side_buf,st->W_window,&gain_right[k],L_SUBFR,0);

      k++;
      Get_exc(&left_exc[i_subfr+L_SUBFR],&exc_mono[i_subfr+L_SUBFR],&side_buf[L_SUBFR],gain_left[k],L_DIV-L_SUBFR,1);
      Get_exc(&right_exc[i_subfr+L_SUBFR],&exc_mono[i_subfr+L_SUBFR],&side_buf[L_SUBFR],gain_right[k],L_DIV-L_SUBFR,0);

         if(Add_16s(i_subfr, L_DIV + L_SUBFR) < (L_FRAME_PLUS-L_SUBFR/2) ) {
            Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[i_subfr+L_DIV],buf,L_SUBFR);
         } else {
            Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[i_subfr+L_DIV],buf,L_SUBFR/2);
         }
         p_h += HI_FILT_ORDER;
      }
      Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[L_FRAME_PLUS-L_SUBFR/2],side_buf,L_SUBFR/2);
      Get_exc_win(&left_exc[L_FRAME_PLUS-L_SUBFR/2], buf,&exc_mono[L_FRAME_PLUS-L_SUBFR/2],side_buf,st->W_window,&gain_left[k],L_SUBFR/2,1);
      Get_exc_win(&right_exc[L_FRAME_PLUS-L_SUBFR/2], buf,&exc_mono[L_FRAME_PLUS-L_SUBFR/2],side_buf,st->W_window,&gain_right[k],L_SUBFR/2,0);
   } else {
      Fir_filt(st->old_wh2_fx,HI_FILT_ORDER,exc_mono,buf,L_SUBFR/2);

      Get_exc(&left_exc[0],&exc_mono[0],&buf[0],gain_left[0],L_SUBFR/2,1);
      Get_exc(&right_exc[0],&exc_mono[0],&buf[0],gain_right[0],L_SUBFR/2,0);

      Fir_filt(st->old_wh2_fx,HI_FILT_ORDER,exc_mono +L_SUBFR/2,buf,L_SUBFR);
      Fir_filt(st->old_wh_fx,HI_FILT_ORDER, exc_mono + L_SUBFR/2,side_buf,L_DIV);

      Get_exc_win(&left_exc[L_SUBFR/2], buf,&exc_mono[L_SUBFR/2],side_buf,st->W_window,&gain_left[0],L_SUBFR,1);
      Get_exc_win(&right_exc[L_SUBFR/2], buf,&exc_mono[L_SUBFR/2],side_buf,st->W_window,&gain_right[0],L_SUBFR,0);

      Get_exc(&left_exc[L_SUBFR+L_SUBFR/2],&exc_mono[L_SUBFR+L_SUBFR/2],&side_buf[L_SUBFR],gain_left[1],L_DIV-L_SUBFR,1);
      Get_exc(&right_exc[L_SUBFR+L_SUBFR/2],&exc_mono[L_SUBFR+L_SUBFR/2],&side_buf[L_SUBFR],gain_right[1],L_DIV-L_SUBFR,0);

      Fir_filt(st->old_wh_fx,HI_FILT_ORDER,&exc_mono[L_DIV+L_SUBFR/2],buf,L_SUBFR);
      p_h = wh;
      k = 1;

      Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[L_DIV+L_SUBFR/2],side_buf,L_DIV);

      Get_exc_win(&left_exc[L_DIV+L_SUBFR/2], buf,&exc_mono[L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_left[k],L_SUBFR,1);
      Get_exc_win(&right_exc[L_DIV+L_SUBFR/2], buf,&exc_mono[L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_right[k],L_SUBFR,0);

      Get_exc(&left_exc[L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_left[k+1],L_DIV-L_SUBFR,1);
      Get_exc(&right_exc[L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_right[k+1],L_DIV-L_SUBFR,0);

      Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[(2*L_DIV)+(L_SUBFR/2)],buf,L_SUBFR);
      p_h += HI_FILT_ORDER;
      k++;

    Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[(2*L_DIV)+L_SUBFR/2],side_buf,L_DIV);

    Get_exc_win(&left_exc[2*L_DIV+L_SUBFR/2], buf,&exc_mono[2*L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_left[k],L_SUBFR,1);
    Get_exc_win(&right_exc[2*L_DIV+L_SUBFR/2], buf,&exc_mono[2*L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_right[k],L_SUBFR,0);

    Get_exc(&left_exc[2*L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[2*L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_left[k+1],L_DIV-L_SUBFR,1);
    Get_exc(&right_exc[2*L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[2*L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_right[k+1],L_DIV-L_SUBFR,0);

    Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[(3*L_DIV)+(L_SUBFR/2)],buf,L_SUBFR);
    p_h += HI_FILT_ORDER;
    k++;
    Fir_filt(p_h,HI_FILT_ORDER,&exc_mono[(3*L_DIV)+L_SUBFR/2],side_buf,L_DIV-L_SUBFR/2);

    Get_exc_win(&left_exc[3*L_DIV+L_SUBFR/2], buf,&exc_mono[3*L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_left[k],L_SUBFR,1);
    Get_exc_win(&right_exc[3*L_DIV+L_SUBFR/2], buf,&exc_mono[3*L_DIV+L_SUBFR/2],side_buf,st->W_window,&gain_right[k],L_SUBFR,0);

    Get_exc(&left_exc[3*L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[3*L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_left[k+1],L_DIV-L_SUBFR-L_SUBFR/2,1);
    Get_exc(&right_exc[3*L_DIV+L_SUBFR/2+L_SUBFR],&exc_mono[3*L_DIV+L_SUBFR/2+L_SUBFR],&side_buf[L_SUBFR],gain_right[k+1],L_DIV-L_SUBFR-L_SUBFR/2,0);
    p_h += HI_FILT_ORDER;
  }

   ippsCopy_16s(p_h,st->old_wh_fx,HI_FILT_ORDER);
   ippsCopy_16s(p_h-HI_FILT_ORDER,st->old_wh2_fx,HI_FILT_ORDER);

   p_Aq = st->old_AqLF_fx;

   wTmp = p_Aq[0];
   p_Aq[0] >>= 1;
   ippsSynthesisFilter_G729E_16s(p_Aq, M, left_exc, left_hi, L_SUBFR/2, st->left.mem_synth_hi_fx);
   ippsCopy_16s(&left_hi[L_SUBFR/2-M], st->left.mem_synth_hi_fx, M);
   ippsSynthesisFilter_G729E_16s(p_Aq, M, right_exc, right_hi, L_SUBFR/2, st->right.mem_synth_hi_fx);
   ippsCopy_16s(&right_hi[L_SUBFR/2-M], st->right.mem_synth_hi_fx, M);
   p_Aq[0] = wTmp;

   p_Aq += (M+1);

   for(k=0; k<3; k++) {
      wTmp = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &left_exc[k*L_SUBFR+L_SUBFR/2], &left_hi[k*L_SUBFR+L_SUBFR/2],
         L_SUBFR, st->left.mem_synth_hi_fx);
      ippsCopy_16s(&left_hi[k*L_SUBFR+L_SUBFR/2+(L_SUBFR-M)], st->left.mem_synth_hi_fx, M);
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &right_exc[k*L_SUBFR+L_SUBFR/2], &right_hi[k*L_SUBFR+L_SUBFR/2],
         L_SUBFR, st->right.mem_synth_hi_fx);
      ippsCopy_16s(&right_hi[k*L_SUBFR+L_SUBFR/2+(L_SUBFR-M)], st->right.mem_synth_hi_fx, M);
      p_Aq[0] = wTmp;

      p_Aq += (M+1);
   }

   if (fscale != 0) {
      wTmp = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &left_exc[k*L_SUBFR+L_SUBFR/2], &left_hi[k*L_SUBFR+L_SUBFR/2],
         L_SUBFR, st->left.mem_synth_hi_fx);
      ippsCopy_16s(&left_hi[k*L_SUBFR+L_SUBFR/2+(L_SUBFR-M)], st->left.mem_synth_hi_fx, M);
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &right_exc[k*L_SUBFR+L_SUBFR/2], &right_hi[k*L_SUBFR+L_SUBFR/2],
         L_SUBFR, st->right.mem_synth_hi_fx);
      ippsCopy_16s(&right_hi[k*L_SUBFR+L_SUBFR/2+(L_SUBFR-M)], st->right.mem_synth_hi_fx, M);
      p_Aq[0] = wTmp;
  }

   if(fscale==0)
      start = 3*L_SUBFR + L_SUBFR/2;
   else
      start = 4*L_SUBFR + L_SUBFR/2;

   p_Aq = AqLF;
   for (i_subfr=start; i_subfr<L_FRAME_PLUS-L_SUBFR/2; i_subfr+=L_SUBFR) {
      wTmp = p_Aq[0];
      p_Aq[0] >>= 1;
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &left_exc[i_subfr], &left_hi[i_subfr], L_SUBFR, st->left.mem_synth_hi_fx);
      ippsCopy_16s(&left_hi[i_subfr+(L_SUBFR-M)], st->left.mem_synth_hi_fx, M);
      ippsSynthesisFilter_G729E_16s(p_Aq, M, &right_exc[i_subfr], &right_hi[i_subfr], L_SUBFR, st->right.mem_synth_hi_fx);
      ippsCopy_16s(&right_hi[i_subfr+(L_SUBFR-M)], st->right.mem_synth_hi_fx, M);
      p_Aq[0] = wTmp;
      p_Aq += (M+1);
   }

   wTmp = p_Aq[0];
   p_Aq[0] >>= 1;
   ippsSynthesisFilter_G729E_16s(p_Aq, M, &left_exc[L_FRAME_PLUS-L_SUBFR/2], &left_hi[L_FRAME_PLUS-L_SUBFR/2],
      L_SUBFR/2, st->left.mem_synth_hi_fx);
   ippsCopy_16s(&left_hi[L_FRAME_PLUS-L_SUBFR/2+(L_SUBFR/2-M)], st->left.mem_synth_hi_fx, M);
   ippsSynthesisFilter_G729E_16s(p_Aq, M, &right_exc[L_FRAME_PLUS-L_SUBFR/2], &right_hi[L_FRAME_PLUS-L_SUBFR/2],
      L_SUBFR/2, st->right.mem_synth_hi_fx);
   ippsCopy_16s(&right_hi[L_FRAME_PLUS-L_SUBFR/2+(L_SUBFR/2-M)], st->right.mem_synth_hi_fx, M);
   p_Aq[0] = wTmp;

   if(fscale == 0)
      ippsCopy_16s(p_Aq,st->old_AqLF_fx,4*(M+1));
   else
      ippsCopy_16s(p_Aq,st->old_AqLF_fx,5*(M+1));
}

static void Decoder_acelp(
  Ipp16s prm[],
  Ipp16s lg,
  Ipp32s codec_mode,
  Ipp32s bfi,
  Ipp16s *pT,
  Ipp16s *pgainT,
  Ipp16s Az[],
  Ipp16s exc[],
  Ipp16s synth[],
  Ipp16s pit_adj,
  Ipp32s len,
  Ipp16s stab_fac,
  DecoderObj_AMRWBE* st
)
{
   Ipp32s i, i_subfr;
   Ipp16s select, tmp16_2;
   Ipp16s T0=0, T0_frac=0, index;
   Ipp16s scale, tmp16  =0;
   Ipp16s mean_ener_code;
   Ipp16s ener_e, ener_m, lg_m, lg_e;
   Ipp16s gain_pit16, *p_Az;
   Ipp32s Lgain_code, Lenerwsyn, L_tmp;
   Ipp16s gain_code16;
   Ipp16s Scaling_update;

   Ipp16s gain_code, gain_code_lo, fac;

   Ipp16s T0_min_max[2]={0,0}; //T0_min, T0_max
   Ipp32s nSubFrame;
   IppSpchBitRate ippRate = tblMonoMode2MonoRate_WBE[codec_mode];

   Ipp16s hpCoeffs[2];
   Ipp16s voice_fac;

   IPP_ALIGNED_ARRAY(16, Ipp16s, exc2, L_DIV);
   IPP_ALIGNED_ARRAY(16, Ipp16s, code2, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, code, L_SUBFR);
   IPP_ALIGNED_ARRAY(16, Ipp16s, Azp, 1+M);
   IPP_ALIGNED_ARRAY(16, Ipp16s, buf, M+L_OVLP);
   IPP_ALIGNED_ARRAY(16, Ipp16s, tmp_buf , M);

   mean_ener_code = 0;
   lg_e = 31-22;
   lg_m = 16384;

   index = *prm++;

   if (!bfi) {
      L_tmp = Add_32s(36, Mul2_Low_32s(index*12));
      mean_ener_code = (Ipp16s)(ShiftL_32s(L_tmp, 8-1));
   }

   T0_min_max[0] = (Ipp16s)(st->siOldPitchLag - 8);

   Lenerwsyn = 1;

   p_Az = Az;
   for (i_subfr=0,nSubFrame=0; i_subfr<lg; i_subfr+=L_SUBFR,nSubFrame++) {
      T0 = st->siOldPitchLag;
      T0_frac = st->siOldPitchLagFrac;
      ippsAdaptiveCodebookDecode_AMRWBE_16s(*prm, &exc[i_subfr], &T0, &T0_frac, T0_min_max,
         nSubFrame, bfi, st->i_offset);
      prm++;

      select = *prm++;
      if (bfi) {
         select = 1;
         st->siOldPitchLag = T0;
         st->siOldPitchLagFrac = T0_frac;
      }

      if (select == 0) {
          hpCoeffs[0] = 20972;
          hpCoeffs[1] = -5898;
         ippsHighPassFilter_Direct_AMRWB_16s(hpCoeffs, &exc[i_subfr], code, L_SUBFR, 1);
         ippsCopy_16s(code, &exc[i_subfr], L_SUBFR);
      }

      if (!bfi)
         ippsAlgebraicCodebookDecode_AMRWB_16s(prm, code, ippRate);

      if (codec_mode <= MODE_16k)
         prm += 4;
      else
         prm += 8;

      if (bfi) {
         for (i=0; i<L_SUBFR; i++)
            code[i] = (Ipp16s)(Rand_16s(&(st->seed_ace)) >> 3);
      }

      tmp16 = 0;
      ippsPreemphasize_AMRWB_16s_ISfs(TILT_CODE_FX, code, L_SUBFR, 14, &tmp16);

      i = T0;
      if (T0_frac > 2)
         i = i + 1;
      if (i < L_SUBFR)
         ippsHarmonicFilter_NR_16s(PIT_SHARP_FX, i, &code[i], &code[i], L_SUBFR-i);

      index = *prm++;
      st->wmem_gain_code[i_subfr>>6] = D_gain2_plus((Ipp16s)index, code, L_SUBFR, &gain_pit16, &Lgain_code, bfi,
         mean_ener_code, &(st->wpast_gpit), &(st->Lpast_gcode));

      Scaling_update = Scale_exc(&exc[i_subfr], L_SUBFR, Lgain_code, &(st->Q_exc),st->mem_subfr_q, len);

      if (st->Q_exc > 0) {
         gain_code16 = Cnvrt_NR_32s16s(ShiftL_32s(Lgain_code, st->Q_exc));
      } else {
         gain_code16 = Cnvrt_NR_32s16s(Lgain_code >> (-st->Q_exc));
      }

      ippsCopy_16s(&exc[i_subfr], &exc2[i_subfr], L_SUBFR);
      ownScaleSignal_AMRWB_16s_ISfs(&exc2[i_subfr], L_SUBFR, -3);
      voice_fac = ownVoiceFactor(&exc2[i_subfr], -3, gain_pit16, code, gain_code16, L_SUBFR);

      ippsCopy_16s(&exc[i_subfr], &exc2[i_subfr], L_SUBFR);

      ippsInterpolateC_NR_16s(code, gain_code16, 7, &exc[i_subfr], gain_pit16, 2, &exc[i_subfr], L_SUBFR);

      i = T0;
      if (T0_frac > 2) {
         i = i + 1;
      }
      if (i > (PIT_MAX_12k8 + (6*st->i_offset))) {
         i = PIT_MAX_12k8 + (6*st->i_offset);
      }
      *pT = (Ipp16s)i;
      *pgainT = gain_pit16;

      pT++;
      pgainT++;

      if(pit_adj > 0) {
         Unpack_32s(Lgain_code, &gain_code, &gain_code_lo);

         tmp16 = Sub_16s(16384, (Ipp16s)(voice_fac>>1));
         fac = (Ipp16s)((stab_fac*tmp16)>>15);

         L_tmp = Lgain_code;

         if (L_tmp < st->L_gc_thres) {
            L_tmp = Add_32s(L_tmp, Mpy_32_16(gain_code, gain_code_lo, 6226));
            if (L_tmp > st->L_gc_thres) {
               L_tmp = st->L_gc_thres;
            }
         } else {
            L_tmp = Mpy_32_16(gain_code, gain_code_lo, 27536);
            if (L_tmp < st->L_gc_thres) {
               L_tmp = st->L_gc_thres;
            }
         }
         st->L_gc_thres = L_tmp;

         Lgain_code = Mpy_32_16(gain_code, gain_code_lo, Sub_16s(32767, fac));
         Unpack_32s(L_tmp, &gain_code, &gain_code_lo);
         Lgain_code = Add_32s(Lgain_code, Mpy_32_16(gain_code, gain_code_lo, fac));

         tmp16 = Add_16s((Ipp16s)(voice_fac>>3), 4096);

         hpCoeffs[1] = tmp16;
         ippsHighPassFilter_Direct_AMRWB_16s(hpCoeffs, code, code2, L_SUBFR, 0);

         if (st->Q_exc > 0) {
            gain_code = Cnvrt_NR_32s16s(ShiftL_32s(Lgain_code, st->Q_exc));
         } else {
            gain_code = Cnvrt_NR_32s16s(Lgain_code >> (-st->Q_exc));
         }

         ippsInterpolateC_NR_16s(code2, gain_code, 7, &exc2[i_subfr], gain_pit16, 2, &exc2[i_subfr], L_SUBFR);
         ownScaleSignal_AMRWB_16s_ISfs(exc2, i_subfr, Scaling_update);
      }
      p_Az += (M+1);
   }

   ippsCopy_16s(synth - M, tmp_buf, M);
   Rescale_mem(tmp_buf, st);
   ownScaleSignal_AMRWB_16s_ISfs(&(st->wmem_wsyn), 1, (Ipp16s)(st->Q_syn - st->Old_Qxnq));

   p_Az = Az;
   for(i_subfr=0; i_subfr<L_DIV; i_subfr+=L_SUBFR) {
      if (pit_adj == 0) {
         tmp16 = p_Az[0];
         scale = (Ipp16s)(st->Q_exc - st->Q_syn);
         if (scale >= 0)
            p_Az[0] >>= scale;
         else
            p_Az[0] <<= (-scale);
         ippsSynthesisFilter_G729E_16s(p_Az, M, &exc[i_subfr], &synth[i_subfr], L_SUBFR, tmp_buf);
         p_Az[0] = tmp16;
         ippsCopy_16s(synth + i_subfr + L_SUBFR - M, tmp_buf, M);
      } else {
         tmp16 = p_Az[0];
         scale = (Ipp16s)(st->Q_exc - st->Q_syn);
         if (scale >= 0) {
            p_Az[0] >>= scale;
         } else {
            p_Az[0] <<= (-scale);
         }
         ippsSynthesisFilter_G729E_16s(p_Az, M, &exc2[i_subfr], &synth[i_subfr], L_SUBFR, tmp_buf);
         p_Az[0] = tmp16;
         ippsCopy_16s(synth + i_subfr + L_SUBFR  - M, tmp_buf, M);
      }

      ippsMulPowerC_NR_16s_Sfs(p_Az, GAMMA1_FX, Azp, (M+1), 15);
      ippsResidualFilter_Low_16s_Sfs(Azp, M, &synth[i_subfr], code, L_SUBFR, 11);

      ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 14, code, L_SUBFR, &(st->wmem_wsyn));

      L_tmp = 0;
      if (12 > st->Q_syn) {
         tmp16 = ShiftL_16s(1, (Ipp16u)(12-st->Q_syn));
      } else {
         tmp16 = (Ipp16s)(1 >> (st->Q_syn-12));
      }
      for (i=0; i<L_SUBFR; i++) {
         tmp16_2 = (Ipp16s)((code[i]*tmp16)>>15);
         L_tmp  = Add_32s(L_tmp, Mul2_Low_32s(tmp16_2*tmp16_2));
      }
      Lenerwsyn = Add_32s(Lenerwsyn,L_tmp);

      p_Az += (M+1);
   }

   L_tmp = Lenerwsyn;
   ener_e = Norm_32s_I(&L_tmp);
   ener_m = (Ipp16s)(L_tmp >> 16);

   ener_e = (Ipp16s)(36 - ener_e);

   if(lg_m > ener_m) {
      lg_m = (Ipp16s)(lg_m >> 1);
      lg_e = (Ipp16s)(lg_e - 1);
   }
   ener_m = Div_16s(lg_m, ener_m);
   ener_e = (Ipp16s)(lg_e - ener_e);
   Lenerwsyn = ((Ipp32s)ener_m)<<16;
   InvSqrt_32s16s_I(&Lenerwsyn, &ener_e);
   if (15 >= ener_e) {
      st->wwsyn_rms = (Ipp16s)((Lenerwsyn >> (15-ener_e))>>16);
   } else {
      st->wwsyn_rms = (Ipp16s)(ShiftL_32s(Lenerwsyn, (Ipp16u)(ener_e-15))>>16);
   }
   st->Q_old = st->Q_exc;
   st->Old_Qxnq = st->Q_syn;

   ippsCopy_16s(&synth[lg-M], buf, M);
   ippsZero_16s(buf+M, L_OVLP);

   tmp16 = p_Az[0];
   p_Az[0] >>= 1;
   ippsSynthesisFilter_G729E_16s_I(p_Az, M, buf+M, L_OVLP, buf);
   p_Az[0] = tmp16;

   ippsMulPowerC_NR_16s_Sfs(p_Az, GAMMA1_FX, Azp, (M+1), 15);
   ippsResidualFilter_Low_16s_Sfs(Azp, M, buf+M, st->wwovlp, L_OVLP, 11);

   tmp16 = st->wmem_wsyn;
   ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 14, st->wwovlp, L_OVLP, &tmp16);

   for (i=1; i<(L_OVLP/2); i++) {
      st->wwovlp[L_OVLP-i] = MulHR_16s(st->wwovlp[L_OVLP-i], tblOverlap5ms_16s[i-1]);
   }
   st->ovlp_size = 0;

   st->siOldPitchLag = T0;
   st->siOldPitchLagFrac = T0_frac;
}


#if ((defined _WIN32) || (defined _WIN64))
#pragma optimize( "", off )
#endif

static APIAMRWB_Status Decoder_tcx(Ipp16s* prm, Ipp32s* nbits_AVQ, Ipp16s* A, Ipp32s L_frame,
      Ipp32s* bad_frame, Ipp16s* exc, Ipp16s* synth, DecoderObj_AMRWBE* st)
{
  Ipp32s i, k, i_subfr, lg, lext, n_pack;
  Ipp16s index, scale, tmp16, fac_ns;
  Ipp16s *p_A;
  Ipp16s *xri;
  Ipp16s *xnq;
  Ipp32s any_loss, bfi;
  Ipp32s gain32;
  Ipp16s inc2, Q_ifft;

  IPP_ALIGNED_ARRAY(16, Ipp16s, Ap     , M+1);
  IPP_ALIGNED_ARRAY(16, Ipp16s, window , 2*L_OVLP);
  IPP_ALIGNED_ARRAY(16, Ipp16s, buf    , L_TCX);
  IPP_ALIGNED_ARRAY(16, Ipp16s, tmp_buf, M);

  xri = exc;
  xnq = synth;

  lext = L_OVLP;

   if (L_frame == (L_FRAME_PLUS/4)) {        //256
      n_pack = 1;
      lext = L_OVLP/4;                       //32
      inc2 = 4;
   } else if (L_frame == (L_FRAME_PLUS/2)) { //512
      n_pack = 2;
      lext = L_OVLP/2;                       //64
      inc2 = 2;
   } else if (L_frame == (L_FRAME_PLUS)) {   //1024
      n_pack = 4;
      inc2 = 1;
   } else {
      return APIAMRWB_StsErr;
   }

   lg = L_frame + lext;

   any_loss = 0;
   for (i=0; i<n_pack; i++) {
      any_loss |= bad_frame[i];
   }

   if ((n_pack==1) && (bad_frame[0] != 0)) {
      k = st->pitch_tcx;

      for (i=0; i<L_frame; i++) {
         exc[i] = MulHR_16s(exc[i-k], 22938);
      }
      ippsMove_16s(synth-M, synth, M);

      p_A = A;
      for (i_subfr=0; i_subfr<L_frame; i_subfr+=L_SUBFR) {
         tmp16 = p_A[0];
         scale = (Ipp16s)(st->Q_exc - st->Q_syn);
         if (scale >= 0) {
            p_A[0] = (Ipp16s)(p_A[0] >> scale);
         } else {
            p_A[0] = (Ipp16s)(p_A[0] << (-scale));
         }
         ippsSynthesisFilter_G729E_16s(p_A, M, &exc[i_subfr], &synth[i_subfr+M], L_SUBFR, &synth[i_subfr]);
         p_A[0] = tmp16;

         ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);
         ippsResidualFilter_Low_16s_Sfs(Ap, M, &synth[i_subfr+M], &xnq[i_subfr], L_SUBFR, 11);
         p_A += (M+1);
      }

      ownScaleSignal_AMRWB_16s_ISfs(&(st->wmem_wsyn), 1, (Ipp16s)(st->Q_syn - st->Old_Qxnq + 1));
      tmp16 = st->wmem_wsyn;
      ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 15, xnq, L_frame, &tmp16);

      st->wwsyn_rms = MulHR_16s(st->wwsyn_rms, 22938);
      tmp16 = st->wwsyn_rms;
      ownScaleSignal_AMRWB_16s_ISfs(&tmp16, 1, (Ipp16s)(st->Q_syn + 1));
      for (i=0; i<L_frame; i++) {
         if (xnq[i] > tmp16) {
            xnq[i] = tmp16;
         } else if (xnq[i] < Negate_16s(tmp16)) {
            xnq[i] = Negate_16s(tmp16);
         }
      }

      ippsPreemphasize_AMRWB_16s_ISfs(TILT_FAC_FX, xnq, L_frame, 14, &(st->wmem_wsyn));

      p_A = A;
      for (i_subfr=0; i_subfr<L_frame; i_subfr+=L_SUBFR) {
         ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);

         tmp16 = Ap[0];
         Ap[0] >>= 1;
         ippsSynthesisFilter_G729E_16s(Ap, M, &xnq[i_subfr], &synth[i_subfr], L_SUBFR, &synth[i_subfr-M]);
         Ap[0] = tmp16;

         p_A += (M+1);
      }

      ippsZero_16s(st->wwovlp, lext);

      ownScaleSignal_AMRWB_16s_ISfs(st->wwovlp + lext, L_OVLP - lext, (Ipp16s)(st->Q_exc - st->Old_Qxnq));
      st->Old_Qxnq = (Ipp16s)(st->Q_syn + 1);
   } else {
      index = *prm++;

      if (n_pack == 4) {
         bfi = bad_frame[1];
      } else {
         bfi = bad_frame[0];
      }

      if (bfi != 0) {
         index = 0;
      }

      fac_ns = (Ipp16s)(Mul2_Low_32s(3277*(8-index)) >> 1);
      index = *prm++;
      ippsDecodeDemux_AMRWBE_16s(prm, nbits_AVQ, bad_frame, n_pack, xri, lg/8);

      Q_ifft = 0;
      Scale_tcx_ifft(xri, lg, &Q_ifft);

      if(any_loss) {
         Adapt_low_freq_deemph_ecu(xri, lg, Q_ifft, st);
         Reconst_spect(xri, st->wold_xri, n_pack, bad_frame, lg, st->last_mode, Q_ifft);
         NoiseFill(xri, buf, &(st->seed_tcx), fac_ns, Q_ifft, lg);
      } else {
         NoiseFill(xri, buf, &(st->seed_tcx), fac_ns, Q_ifft, lg);
         Adap_low_freq_deemph(xri, lg);
      }
      st->Old_Qxri = Q_ifft;
      ippsCopy_16s(xri, st->wold_xri, lg);
      st->pitch_tcx = Find_mpitch(xri, lg);

      xri[1] = 0;
      ippsFFTInv_PermToR_AMRWBE_16s(xri, xnq, lg);

      Cos_window(window, st->ovlp_size, lext, inc2);
      ippsGainDecodeTCX_AMRWBE_16s(xnq, lg, index, 0, &(st->wwsyn_rms), &gain32);

      ownWindowing(window, xnq, st->ovlp_size);
      ownWindowing(&window[st->ovlp_size], &xnq[L_frame], lext);

      ippsCopy_16s(synth-M, tmp_buf, M);
      Scale_mem_tcx(xnq, lg, gain32, synth-M, st);

      ippsAdd_16s(st->wwovlp, xnq, xnq, L_OVLP);

      for (i=0; i<lext; i++) {
         st->wwovlp[i] = xnq[i+L_frame];
      }
      for (i=lext; i<L_OVLP; i++) {
         st->wwovlp[i] = 0;
      }

      st->ovlp_size = (Ipp16s)lext;
      ippsPreemphasize_AMRWB_16s_ISfs(TILT_FAC_FX, xnq, L_frame, 14, &(st->wmem_wsyn));

      p_A = A;
      for (i_subfr=0; i_subfr<L_frame; i_subfr+=L_SUBFR) {
         ippsMulPowerC_NR_16s_Sfs(p_A, GAMMA1_FX, Ap, (M+1), 15);

         tmp16 = Ap[0];
         scale = (Ipp16s)(st->Q_exc - st->Q_syn);
         if (scale >= 0) {
            Ap[0] >>= scale;
         } else {
            Ap[0] <<= (-scale);
         }
         ippsSynthesisFilter_G729E_16s(Ap, M, &xnq[i_subfr], &synth[i_subfr], L_SUBFR, &synth[i_subfr-M]);
         Ap[0] = tmp16;

         ippsResidualFilter_Low_16s_Sfs(p_A, M, &synth[i_subfr], &exc[i_subfr], L_SUBFR, 12);
         p_A += (M+1);
      }
      ippsCopy_16s(tmp_buf,synth-M, M);
      st->Q_exc = st->Q_syn;
   }
   return APIAMRWB_StsNoErr;
}

#if ((defined _WIN32) || (defined _WIN64))
#pragma optimize( "", on )
#endif


