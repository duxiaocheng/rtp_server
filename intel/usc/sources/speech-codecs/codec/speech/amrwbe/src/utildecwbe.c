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
// Purpose: AMRWBE speech codec: decoder utilities.
//
*/
#include "ownamrwbe.h"

static void Deemph1k6(Ipp16s* xri, Ipp16s e_max, Ipp16s m_max, Ipp32s len);
static void Div_phasor(Ipp16s ph1[], Ipp16s ph2[], Ipp16s res[]);
static void Mult_phasor(Ipp16s ph1[], Ipp16s ph2[], Ipp16s res[]);
static void Get_phasor(Ipp16s xri[], Ipp16s ph[]);
static Ipp16s Atan2(Ipp16s ph_y, Ipp16s ph_x);
static void Find_x_y(Ipp16s angle, Ipp16s *ph);
static void Rnd_ph16(Ipp16s *seed, Ipp16s *xri, Ipp32s len, Ipp16s Qifft);

static Ipp32s Div_32s_hl (Ipp32s L_num, Ipp16s denom_hi, Ipp16s denom_lo)
{
   Ipp16s approx, hi, lo, n_hi, n_lo;
   Ipp32s L_32;

   approx = Div_16s ((Ipp16s) 0x3fff, denom_hi);
   L_32 = Mpy_32_16 (denom_hi, denom_lo, approx);
   L_32 = Sub_32s((Ipp32s) 0x7fffffffL, L_32);
   Unpack_32s(L_32, &hi, &lo);
   L_32 = Mpy_32_16 (hi, lo, approx);
   Unpack_32s(L_32, &hi, &lo);
   Unpack_32s(L_num, &n_hi, &n_lo);
   L_32 = Mpy_16s32s(n_hi, n_lo, hi, lo);
   L_32 = ShiftL_32s(L_32, 2);
   return (L_32);
}



Ipp32s Pack4bits(Ipp32s nbits, Ipp16s *ptr, Ipp16s *prm)
{
   Ipp32s i=0;

   while (nbits > 4) {
      prm[i] = Bin2int(4, ptr);
      ptr += 4;
      nbits = nbits - 4;
      i++;
   }
   prm[i] = Bin2int(nbits, ptr);
   i++;
   return(i);
}


void Scale_mem2(
  Ipp16s Signal[],
  Ipp16s* Old_q,
  DecoderObj_AMRWBE *st,
  Ipp16s Syn
)
{
   Ipp16s i, j, tmp, exp_e, exp_s;
   Ipp16s *pt_scale;


   tmp = 16;
   for(i=1; i<6; i++) {
      if(Old_q[i] < tmp)
         tmp = Old_q[i];
   }

   pt_scale = &Old_q[2];

   for (j=0; j<L_FRAME_PLUS; j+=L_DIV) {
      ownScaleSignal_AMRWB_16s_ISfs(&Signal[j], L_DIV, (Ipp16s)(tmp - (*pt_scale)) );
      pt_scale++;
   }
   Old_q[2] = tmp;

   if (Syn > 0) {
      exp_s = (Ipp16s)(tmp - st->Old_Q_syn);
      st->Old_Q_syn = tmp;
      st->left.Q_synHF = (Ipp16s)(st->Old_Q_syn + 1);
      st->right.Q_synHF = (Ipp16s)(st->Old_Q_syn + 1);

      if(exp_s != 0) {
         Ipp16s memHPFilter[6];
         ownScaleSignal_AMRWB_16s_ISfs(&(st->wmem_deemph), 1, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->wold_synth_pf, (PIT_MAX_MAX+2*L_SUBFR), exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->wold_noise_pf, 2*L_FILT, exp_s);

         ippsHighPassFilterGetDlyLine_AMRWB_16s(st->wmemSigOut, memHPFilter, 2);
         ownScaleHPFilterState(memHPFilter, exp_s, 2);
         ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, st->wmemSigOut, 2);

         ippsHighPassFilterGetDlyLine_AMRWB_16s(st->left.wmemSigOut, memHPFilter, 2);
         ownScaleHPFilterState(memHPFilter, exp_s, 2);
         ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, st->left.wmemSigOut, 2);

         ippsHighPassFilterGetDlyLine_AMRWB_16s(st->right.wmemSigOut, memHPFilter, 2);
         ownScaleHPFilterState(memHPFilter, exp_s, 2);
         ippsHighPassFilterSetDlyLine_AMRWB_16s(memHPFilter, st->right.wmemSigOut, 2);

         ownScaleSignal_AMRWB_16s_ISfs(st->left.wmem_oversamp, L_MEM_JOIN_OVER, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->right.wmem_oversamp, L_MEM_JOIN_OVER, exp_s);

         ownScaleSignal_AMRWB_16s_ISfs(st->left.wmem_syn_hf, MHF, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.wold_synth_hf, D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.wmem_oversamp_hf, 2*L_FILT, exp_s);

         ownScaleSignal_AMRWB_16s_ISfs(st->right.wmem_syn_hf, MHF, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->right.wold_synth_hf, D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->right.wmem_oversamp_hf, 2*L_FILT, exp_s);

         if (exp_s > 0) {
            st->left.Lthreshold = ShiftL_32s(st->left.Lthreshold, (Ipp16u)(exp_s<<1));
            st->right.Lthreshold = ShiftL_32s(st->right.Lthreshold, (Ipp16u)(exp_s<<1));
         } else {
            st->left.Lthreshold = st->left.Lthreshold >> (-(exp_s<<1));
            st->right.Lthreshold = st->right.Lthreshold  >> (-(exp_s<<1));
         }

         ownScaleSignal_AMRWB_16s_ISfs(st->my_old_synth_fx, 2*L_FDEL+20, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->my_old_synth_2k_fx, L_FDEL_2k + D_STEREO_TCX + 2*(D_NC*5)/32, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->my_old_synth_hi_fx, 2*L_FDEL, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->mem_left_2k_fx, 2*L_FDEL_2k, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->mem_right_2k_fx, 2*L_FDEL_2k, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->mem_left_hi_fx, L_FDEL, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->mem_right_hi_fx, L_FDEL, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->mem_stereo_ovlp_fx, L_OVLP_2k, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.mem_d_tcx_fx,D_NC + (D_STEREO_TCX*32/5), exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->right.mem_d_tcx_fx,D_NC + (D_STEREO_TCX*32/5), exp_s);

         ownScaleSignal_AMRWB_16s_ISfs(st->old_exc_mono_fx, HI_FILT_ORDER, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.mem_synth_hi_fx, M, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->right.mem_synth_hi_fx, M, exp_s);

         ownScaleSignal_AMRWB_16s_ISfs(st->right.wmem_d_nonc, D_NC, exp_s);
         ownScaleSignal_AMRWB_16s_ISfs(st->left.wmem_d_nonc, D_NC, exp_s);
      }
      return;
   } else {
      exp_e = (Ipp16s)(tmp - st->Old_Q_exc);
      st->Old_Q_exc = tmp;
      if(exp_e > 0) {
         st->left.Lp_amp = ShiftL_32s(st->left.Lp_amp, exp_e);
         st->right.Lp_amp = ShiftL_32s(st->right.Lp_amp, exp_e);
      } else {
         st->left.Lp_amp = st->left.Lp_amp >> (-exp_e);
         st->right.Lp_amp = st->right.Lp_amp >> (-exp_e);
      }
      return;
   }
}

#define DELAY_MAX (D_BPF +L_SUBFR+ L_BSP + 2*D_NC + L_FDEL + 32*D_STEREO_TCX/5)
void Delay(Ipp16s *signal, Ipp32s len, Ipp32s lenDelay, Ipp16s *mem)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, buf, (DELAY_MAX));

   ippsCopy_16s(mem, buf, lenDelay);
   ippsCopy_16s(&signal[len-lenDelay], mem, lenDelay);
   ippsMove_16s(signal, &signal[lenDelay], (len-lenDelay));
   ippsCopy_16s(buf, signal, lenDelay);
}

void Updt_mem_q(Ipp16s* old_sub_q, Ipp16s n, Ipp16s new_Q)
{
   Ipp16s i, j;

   for(j = 0; j <n; j++) {
      for(i = 0;i < 5; i++) {
         old_sub_q[i] = old_sub_q[i+1];
      }
      old_sub_q[i] = new_Q;
   }
}

Ipp16s D_gain_chan(
  Ipp16s * gain_hf,
  Ipp16s *mem_gain,
  Ipp16s *prm,
  Ipp32s bfi,
  Ipp32s mode,
  Ipp32s *bad_frame,
  Ipp32s *Lgain,
  Ipp32s mono_dec_stereo
)
{
   Ipp32s i;
   Ipp16s index, gain4[4], prm_adv, *pt_prm, tmp16;

   pt_prm = prm;
   D_gain_hf(pt_prm[2], gain4, mem_gain, bfi);
   pt_prm += 3;

   if (mode == 3) {
      prm_adv = 3+16;
      for (i=0; i<16; i++) {
         index = *pt_prm++;
         tmp16 = Sub_16s(ShiftL_16s((Ipp16s)((index<<1)+index),8), 2688);
         if (bfi == 1) {
            tmp16 = 0;
         }
         if ((bad_frame[1]==1) && (i<8)) {
            tmp16 = 0;
         }
         if ((bad_frame[2]==1) && (i>=8)) {
            tmp16 = 0;
         }
         if (mono_dec_stereo == 1) {
            gain_hf[i] = Add_16s(tmp16, gain4[i>>2]);
            *Lgain = Add_32s(gain_hf[i], *Lgain);
         } else {
            gain_hf[i] = Add_16s(Add_16s(tmp16,gain_hf[i]), gain4[i>>2]);
         }
      }
   } else if (mode == 2) {
      prm_adv = 3+8;
      for (i=0; i<8; i++) {
         index = *pt_prm++;
         tmp16 = Sub_16s(ShiftL_16s((Ipp16s)((index<<1)+index),8), 1152);

         if ((bfi==1) || (bad_frame[1]==1)) {
            tmp16 = 0;
         }
         if(mono_dec_stereo == 1) {
            gain_hf[i] = Add_16s(tmp16, gain4[i>>1]);
            *Lgain = Add_32s(gain_hf[i], *Lgain);
         } else {
            gain_hf[i] = Add_16s(Add_16s(tmp16,gain_hf[i]), gain4[i>>1]);
         }
      }
   } else {
      prm_adv = 3;

      if(mono_dec_stereo == 1) {
         for (i=0; i<4; i++) {
            gain_hf[i] = gain4[i];
            *Lgain = Add_32s(gain_hf[i], *Lgain);
         }
      } else {
         for (i=0; i<4; i++) {
            gain_hf[i] = Add_16s(gain_hf[i], gain4[i]);
         }
      }
   }
   return prm_adv;
}

void Comp_gain_shap(Ipp16s* wm, Ipp16s* gain_shap, Ipp32s lg, Ipp16s Qwm)
{
   Ipp16s inv_lg16;
   Ipp16s tmp, expt, expt2, frac, log_sum;
   Ipp32s i, j, k, dwTmp;
   log_sum = 0;

   inv_lg16 = 5461;

   j = 0;
   for (i=0; i<lg; i+=16) {
      dwTmp = 0;
      for (k=0; k<16; k++) {
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(wm[i+k]*wm[i+k]));
      }
      if (dwTmp == 0) {
         dwTmp = 1;
      }

      ownLog2_AMRWBE(dwTmp,&expt,&frac);
      expt = (Ipp16s)(expt - ((Qwm<<1) + 1));
      gain_shap[j] = Add_16s((Ipp16s)(expt<<8), (Ipp16s)(frac>>7));
      log_sum = Add_16s(log_sum, gain_shap[j]);
      gain_shap[j] = MulHR_16s(gain_shap[j], 16384);
      j++;
   }
   log_sum = MulHR_16s(log_sum,inv_lg16);

   expt = (Ipp16s)(log_sum >> 8);
   frac = ShiftL_16s(Sub_16s(log_sum, ShiftL_16s(expt,8)),7);
   ownPow2_AMRWB(expt,frac);
   log_sum = (Ipp16s)(ownPow2_AMRWB(30,frac)>>16);
   log_sum = Div_16s(16384,log_sum);
   j = 0;
   for(i=0; i<lg; i+=16) {
      tmp = gain_shap[j];
      expt2 = (Ipp16s)(tmp >> 8);
      frac = ShiftL_16s(Sub_16s(tmp, ShiftL_16s(expt2,8)),7);
      tmp = (Ipp16s)(ownPow2_AMRWB(30,frac)>>16);
      gain_shap[j] = (Ipp16s)((tmp*log_sum)>>15);

      if (expt2 > expt) {
         gain_shap[j] = ShiftL_16s(gain_shap[j], (Ipp16u)(expt2-expt));
      } else {
         gain_shap[j] = (Ipp16s)(gain_shap[j] >> (expt-expt2));
      }
      j++;
   }
}

static Ipp16s D_Balance(
  Ipp16s index
)
{
   Ipp16s balance_fx, tmp;
   tmp = ShiftL_16s(index,8);
   balance_fx = ShiftL_16s(Sub_16s(tmp,16384),1);
   return balance_fx;
}

Ipp16s Balance(Ipp32s bad_frame, Ipp16s* mem_balance_fx, Ipp16s prm)
{
   Ipp16s result_fx;

   if (bad_frame > 0) {
      result_fx = MulHR_16s(*mem_balance_fx , 29491);
      *mem_balance_fx = result_fx;
   } else {
      result_fx = D_Balance(prm);
      *mem_balance_fx  = result_fx;
   }
   return result_fx;
}

void Apply_xnq_gain2(Ipp32s len, Ipp16s wbalance, Ipp32s Lgain, Ipp16s* xnq_fx, Ipp16s *wm_fx, Ipp16s Q_syn)
{
   Ipp16s tmp16,tmpgain, tmp_scale;
   Ipp32s i, L_tmp1,L_tmp2;

   L_tmp1 = Lgain;
   tmp16 = Norm_32s_I(&L_tmp1);
   tmpgain = (Ipp16s)(L_tmp1>>16);

   tmp16 = (Ipp16s)(tmp16 - 16);
   tmp_scale = (Ipp16s)(Q_syn - tmp16 - 1);
   if (tmp_scale > 0) {
      for(i=0; i<len; i++) {
         L_tmp1 = Mul2_Low_32s(wbalance*wm_fx[i]);
         L_tmp2 = Mul2_Low_32s(tmpgain*xnq_fx[i]);
         xnq_fx[i]= Cnvrt_NR_32s16s(Add_32s(ShiftL_32s(L_tmp2, tmp_scale), ShiftL_32s(L_tmp1,1)));
      }
   } else {
      for(i=0; i<len; i++) {
         L_tmp1 = Mul2_Low_32s(wbalance*wm_fx[i]);
         L_tmp2 = Mul2_Low_32s(tmpgain*xnq_fx[i]);
         xnq_fx[i]= Cnvrt_NR_32s16s(Add_32s((L_tmp2>>(-tmp_scale)), ShiftL_32s(L_tmp1,1)));
      }
   }
   return;
}

void Apply_wien_filt(Ipp32s len, Ipp16s* xnq_fx, Ipp16s* h_fx, Ipp16s* wm_fx)
{
   Ipp32s i, k, dwTmp;
   for(i=0; i<len; i++) {
      dwTmp=0;
      for (k=0; k<ECU_WIEN_ORD+1; k++) {
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(h_fx[k]*wm_fx[i-k]));
      }
      xnq_fx[i] = Cnvrt_NR_32s16s(dwTmp);
   }
   return;
}


void Crosscorr_2(Ipp16s* vec1_fx, Ipp16s* vec2_fx, Ipp16s* result_fx_h, Ipp16s* result_fx_l,
      Ipp32s length, Ipp32s minlag, Ipp32s maxlag, Ipp16s *shift_fx, Ipp32s crosscorr_type)
{
   Ipp32s i,j,k, Li;
   Ipp32s t_L, dwTmp, t_L_max=0, res_tmp[ECU_WIEN_ORD+1];
   Ipp16s vec1_fx_max=0;
   Ipp16s vec2_fx_max=0;
   Ipp16s tmp_16, shf;
   Ipp16s norm;

   for(i=0; i<length; i++) {
      tmp_16 = Abs_16s(vec1_fx[i]);

      if(tmp_16 > vec1_fx_max) {
         vec1_fx_max = tmp_16;
      }
      tmp_16 = Abs_16s(vec2_fx[i]);
      if(tmp_16 > vec2_fx_max) {
         vec2_fx_max = tmp_16;
      }
   }
   dwTmp = Mul2_Low_32s(vec1_fx_max*vec2_fx_max);
   norm = Exp_32s(dwTmp);
   *shift_fx = (Ipp16s)(6 - norm);
   shf = ShiftR_NR_16s(*shift_fx, 1);

   k = 0;
   if (shf >= 0) {
      for (i=minlag; i < 0; i++) {
         t_L = 0;
         Li = length + i;
         for (j = 0; j < Li; j++) {
            t_L = Add_32s(t_L, Mul2_Low_32s((vec1_fx[j]>>shf)*(vec2_fx[j-i]>>shf)));
         }
         dwTmp = Abs_32s(t_L);
         if(dwTmp > t_L_max) {
            t_L_max = dwTmp;
         }
         res_tmp[k] = t_L;
         k++;
      }
      for (i=0; i < maxlag; i++) {
         t_L = 0;
         Li = length - i;
         for (j = 0; j < Li; j++) {
            t_L = Add_32s(t_L, Mul2_Low_32s((vec1_fx[j+i]>>shf)*(vec2_fx[j]>>shf)));
         }
         dwTmp = Abs_32s(t_L);
         if(dwTmp > t_L_max) {
            t_L_max = dwTmp;
         }
         res_tmp[k] = t_L;
         k++;
      }
   } else {
      for (i=minlag; i < 0; i++) {
         t_L = 0;
         Li = length + i;
         for (j = 0; j < Li; j++) {
            t_L = Add_32s(t_L, Mul2_Low_32s(ShiftL_16s(vec1_fx[j],(Ipp16u)(-shf))*
               ShiftL_16s(vec2_fx[j-i],(Ipp16u)(-shf))));
         }
         dwTmp = Abs_32s(t_L);
         if(dwTmp > t_L_max) {
            t_L_max = dwTmp;
         }
         res_tmp[k] = t_L;
         k++;
      }
      for (i=0; i < maxlag; i++) {
         t_L = 0;
         Li = length - i;
         for (j = 0; j < Li; j++) {
            t_L = Add_32s(t_L, Mul2_Low_32s(ShiftL_16s(vec1_fx[j+i],(Ipp16u)(-shf))*
               ShiftL_16s(vec2_fx[j],(Ipp16u)(-shf))));
         }
         dwTmp = Abs_32s(t_L);
         if(dwTmp > t_L_max) {
            t_L_max = dwTmp;
         }
         res_tmp[k] = t_L;
         k++;
      }
   }

   norm = Exp_32s(t_L_max);
   norm = (Ipp16s)(norm - crosscorr_type);
   for(i=0; i<(maxlag-minlag); i++) {
      t_L = ShiftL_32s(res_tmp[i], norm);
      Unpack_32s(t_L,&result_fx_h[i],&result_fx_l[i]);
   }
   *shift_fx = (Ipp16s)((*shift_fx) - norm);
}

Ipp16s Glev_s(Ipp16s *b_fx, Ipp16s *Rh, Ipp16s *Rl, Ipp16s *Zh, Ipp16s *Zl,
      Ipp32s m, Ipp16s shift)
{
   Ipp32s i, j, i1;
   Ipp32s t0,t1,t2;
   Ipp16s Ah[ECU_WIEN_ORD+1], Al[ECU_WIEN_ORD+1];
   Ipp16s Bh[ECU_WIEN_ORD+1], Bl[ECU_WIEN_ORD+1];
   Ipp16s Anh[ECU_WIEN_ORD+1], Anl[ECU_WIEN_ORD+1];
   Ipp16s Kh, Kl;
   Ipp16s hi,lo;
   Ipp16s alp_h, alp_l, alp_exp;
   //Ipp16s rc_fx[ECU_WIEN_ORD+1];

   t1 = Pack_16s32s(Rh[1], Rl[1]);
   t2 = Abs_32s(t1);

   if((Rh[0]==0) && (Rl[0]==0)) {
      Rh[0] = 32767;
   }
   t0 = Div_32s_hl(t2, Rh[0], Rl[0]);

   if (t1 > 0) {
      t0 = Negate_32s(t0);
   }
   Unpack_32s(t0, &Kh, &Kl);
   //rc_fx[0] = Kh;
   t0 = t0>>4;
   Unpack_32s(t0, &Ah[1], &Al[1]);

   t0 = Mpy_16s32s(Kh, Kl, Kh, Kl);
   t0 = Abs_32s(t0);
   t0 = Sub_32s((Ipp32s) 0x7fffffffL, t0);
   Unpack_32s(t0, &hi, &lo);
   t0 = Mpy_16s32s(Rh[0], Rl[0], hi, lo);

   alp_exp = Norm_32s_I(&t0);
   Unpack_32s(t0, &alp_h, &alp_l);

   t1 = Pack_16s32s(Zh[0], Zl[0]);
   t2 = Abs_32s(t1);
   t0 = Div_32s_hl(t2, Rh[0], Rl[0]);

   if (t1 < 0) {
      t0 = Negate_32s(t0);
   }
   t0 = t0>>4;
   Unpack_32s(t0, &Bh[0], &Bl[0]);

   for (i=2; i<=m; i++) {
      t0 = 0;
      i1 = i - 1;
      for (j=0; j<i1; j++) {
         t0 = Add_32s(t0, Mpy_16s32s(Rh[i1-j], Rl[i1-j], Bh[j], Bl[j]));
      }
      t0 = ShiftL_32s(t0, 4);
      t1 = Pack_16s32s(Zh[i1], Zl[i1]);
      t1 = Sub_32s(t1,t0);
      t2 = Abs_32s(t1);
      t0 = Div_32s_hl(t2, alp_h, alp_l);

      if (t1 < 0) {
         t0 = Negate_32s(t0);
      }
      if (alp_exp > 4) {
         t0 = ShiftL_32s(t0, (Ipp16u)(alp_exp-4));
      } else {
         t0 = t0 >> (4 - alp_exp);
      }
      Unpack_32s(t0, &Bh[i-1], &Bl[i-1]);

      for (j=0; j<i1; j++) {
         t0 = Pack_16s32s(Bh[j],Bl[j]);
         t1 = Mpy_16s32s(Bh[i1],Bl[i1],Ah[i1-j],Al[i1-j]);
         t1 = ShiftL_32s(t1,4);
         t0 = Add_32s(t0,t1);
         Unpack_32s(t0,&Bh[j],&Bl[j]);
      }

      if(i == m) {
         break;
      }

      t0 = 0;
      for (j=1; j<i; j++) {
         t0 = Add_32s(t0, Mpy_16s32s(Rh[j], Rl[j], Ah[i - j], Al[i - j]));
      }
      t0 = ShiftL_32s(t0, 4);
      t1 = Pack_16s32s(Rh[i], Rl[i]);
      t0 = Add_32s(t0, t1);
      t1 = Abs_32s(t0);
      t2 = Div_32s_hl(t1, alp_h, alp_l);
      if (t0 > 0) {
         t2 = Negate_32s(t2);
      }
      t2 = ShiftL_32s(t2, alp_exp);
      Unpack_32s(t2, &Kh, &Kl);
      //rc_fx[i - 1] = Kh;

      for (j=1; j<i; j++) {
        t0 = Mpy_16s32s(Kh, Kl, Ah[i - j], Al[i - j]);
        t0 = Add_32s(t0, Pack_16s32s(Ah[j], Al[j]));
        Unpack_32s(t0, &Anh[j], &Anl[j]);
      }
      t2 = t2>>4;
      Unpack_32s(t2, &Anh[i], &Anl[i]);

      t0 = Mpy_16s32s(Kh, Kl, Kh, Kl);
      t0 = Abs_32s(t0);
      t0 = Sub_32s((Ipp32s) 0x7fffffffL, t0);
      Unpack_32s(t0, &hi, &lo);
      t0 = Mpy_16s32s(alp_h, alp_l, hi, lo);
      if(t0 <= 0) {
         t0 = 21474836;
      }

      alp_exp = (Ipp16s)(alp_exp + Norm_32s_I(&t0));
      Unpack_32s(t0, &alp_h, &alp_l);

      for (j=1; j<=i; j++) {
         Ah[j] = Anh[j];
         Al[j] = Anl[j];
      }
   }

   if (4 > shift) {
      for (j=0; j<(ECU_WIEN_ORD+1); j++) {
         t0 = Pack_16s32s(Bh[j],Bl[j]);
         b_fx[j] = Cnvrt_NR_32s16s(ShiftL_32s(t0, (Ipp16u)(4-shift)));
      }
   } else {
      for (j=0; j<(ECU_WIEN_ORD+1); j++) {
         t0 = Pack_16s32s(Bh[j],Bl[j]);
         b_fx[j] = Cnvrt_NR_32s16s(t0 >> (shift-4));
      }
   }
   return 0;
}

void Dec_filt(
  Ipp16s *prm,
  Ipp16s wh[],
  Ipp16s old_wh[],
  Ipp32s bfi,
  const sMSPQTab* filt_hi_pmsvq)
{
   const Ipp16s **cb = filt_hi_pmsvq->cbs;
   const Ipp16s *cbm = filt_hi_pmsvq->mean;
   const Ipp16s vdim =filt_hi_pmsvq->vdim;
   const Ipp16s stages = filt_hi_pmsvq->nstages;
   Ipp16s eq[HI_FILT_ORDER];
  Ipp16s i,l;

  Ipp32s dwTmp;

   if (!bfi) {
      for (i=0; i<vdim; i++) {
         eq[i] = cb[0][prm[0]*vdim + i];
         for (l=1;l<stages;l++) {
            eq[i] = Add_16s(eq[i], cb[l][prm[l]*vdim + i]);
         }
      }
   } else {
      for (i=0; i<vdim; i++) {
         eq[i] = 0;
      }
   }

   for(i=0; i<vdim; i++) {
      dwTmp = Add_32s(Mul2_Low_32s(eq[i] * 16384), Mul2_Low_32s(old_wh[i]*MSVQ_PERDICTCOEFF));
      old_wh[i] = Cnvrt_NR_32s16s(dwTmp);
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(cbm[i]*16384));
      wh[i] = Cnvrt_NR_32s16s(dwTmp);
   }
}


void Dec_gain(
  Ipp16s *prm,
  Ipp16s *gain_left,
  Ipp16s *gain_right,
  Ipp16s *old_gain,
  Ipp32s bfi,
  const sMSPQTab* Gain_hi_pmsvq_fx)
{
  Ipp16s tmp[2];
  const Ipp16s **cb = Gain_hi_pmsvq_fx->cbs;
  const Ipp16s *cbm = Gain_hi_pmsvq_fx->mean;
  const Ipp32s vdim =Gain_hi_pmsvq_fx->vdim;
  Ipp16s eq[HI_FILT_ORDER];
  Ipp32s i;

  if (!bfi)
  {
    for (i=0;i<vdim;i++)
    {
      eq[i] = cb[0][prm[0] * vdim + i];
    }
  }
  else
  {
    for (i=0;i<vdim;i++)
    {
      eq[i] = 0;
    }
  }

  for(i=0;i<vdim;i++)
  {
    old_gain[i] = Cnvrt_NR_32s16s(Add_32s(Mul2_Low_32s(eq[i] * 32767), Mul2_Low_32s(old_gain[i]*MSVQ_PERDICTCOEFF)));
    tmp[i] = Add_16s(old_gain[i],cbm[i]);

  }

  *gain_left  = tmp[0];
  *gain_right = tmp[1];
}


Ipp16s Comp_hi_gain(Ipp16s gcode0)
{
  Ipp32s dwTmp;
  Ipp16s exp_gcode0,frac;

   dwTmp = Mul2_Low_32s(gcode0 * 5443);
   dwTmp = dwTmp >> 10;
   Unpack_32s(dwTmp, &exp_gcode0, &frac);

   gcode0 = (Ipp16s)ownPow2_AMRWB(12, frac);
   if ((exp_gcode0+16) > 0) {
      gcode0 = Cnvrt_NR_32s16s(ShiftL_32s(gcode0, (Ipp16u)(exp_gcode0+16)));
   } else {
      gcode0 = Cnvrt_NR_32s16s(gcode0 >> (-(exp_gcode0+16)));
   }

  return gcode0;
}


void Get_exc_win(Ipp16s *exc_ch, Ipp16s *buf, Ipp16s *exc_mono, Ipp16s *side_buf, Ipp16s *win,
  Ipp16s* gain, Ipp32s len, Ipp32s doSum)
{
  Ipp32s i, dwTmp;
  Ipp16s tmp1;

  if (doSum>0) {
     for (i=0; i<len; i++) {
         tmp1 = (Ipp16s)((gain[0]*win[i])>>15);
         dwTmp = Mul2_Low_32s(tmp1 * exc_mono[i]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(tmp1*buf[i]));
         tmp1 = Sub_16s(32767, win[i]);
         tmp1 = (Ipp16s)((tmp1*gain[1])>>15);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(tmp1*exc_mono[i]));
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(tmp1*side_buf[i]));
         exc_ch[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,3));
      }
   } else {
      for (i=0; i<len; i++) {
         tmp1 = (Ipp16s)((win[i]*gain[0])>>15);
         dwTmp = Mul2_Low_32s(tmp1 * exc_mono[i]);
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(tmp1*buf[i]));
         tmp1 = Sub_16s(32767, win[i]);
         tmp1 = (Ipp16s)((tmp1*gain[1])>>15);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(tmp1*exc_mono[i]));
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(tmp1*side_buf[i]));
         exc_ch[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,3));
      }
   }
}



void Get_exc(Ipp16s *exc_ch, Ipp16s *exc_mono, Ipp16s *side_buf, Ipp16s gain,
      Ipp32s len, Ipp32s doSum)
{
   Ipp32s i, dwTmp;

   if (doSum>0) {
      for(i=0; i<len; i++) {
         dwTmp = Mul2_Low_32s(gain * exc_mono[i]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(side_buf[i]*gain));
         exc_ch[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,3));
      }
   } else {
      for(i=0; i<len; i++) {
         dwTmp = Mul2_Low_32s(gain *exc_mono[i]);
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(side_buf[i]*gain));
         exc_ch[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,3));
      }
   }
}



Ipp16s D_gain2_plus(
  Ipp16s index,
  Ipp16s code[],
  Ipp16s lcode,
  Ipp16s *gain_pit,
  Ipp32s  *gain_code,
  Ipp32s bfi,
  Ipp16s mean_ener,
  Ipp16s *past_gpit,
  Ipp32s  *past_gcode
)
{
  Ipp32s  dwTmp;
  Ipp16s tmp16, exp, frac_enc, hi, lo, ind2, ind2p1;
  Ipp16s gcode0, exp_enc, exp_inov;
  Ipp16s exp_gcode, mant_code, mant_inov;
  Ipp32s  ener_code;
  Ipp16s gcode_inov;


   ippsDotProd_16s32s_Sfs(code, code, lcode, &dwTmp, -1);
   dwTmp = Add_32s(dwTmp, 1);
   exp = (Ipp16s)(30 - Norm_32s_I(&dwTmp));

   ener_code = dwTmp;
   exp_enc = exp;

   exp = (Ipp16s)(exp - 24);
   InvSqrt_32s16s_I(&dwTmp, &exp);
   if (exp > 3) {
      gcode_inov = (Ipp16s)(ShiftL_32s(dwTmp, (Ipp16u)(exp-3))>>16);
   } else {
      gcode_inov = (Ipp16s)((dwTmp >> (3-exp))>>16);
   }

   if (bfi != 0) {
      if ((*past_gpit) > 15565) {
         *past_gpit = 15565;
      } else if ((*past_gpit) < 8192) {
         *past_gpit = 8192;
      }
      *gain_pit = *past_gpit;
      *past_gpit  = MulHR_16s(*past_gpit, 31130);

      tmp16 = Sub_16s(22938, *past_gpit);
      Unpack_32s(*past_gcode, &hi, &lo);
      *past_gcode = ShiftL_32s(Mpy_32_16(hi, lo, tmp16), 1);

      Unpack_32s(*past_gcode, &hi, &lo);
      *gain_code= ShiftL_32s(Mpy_32_16(hi, lo, gcode_inov), 3);
      return 0;
   }

   ownLog2_AMRWBE(ener_code, &exp, &frac_enc);
   hi = (Ipp16s)(exp + exp_enc - 55);
   ener_code = Mpy_32_16(hi, frac_enc, LG10);

   gcode0 = (Ipp16s)(Sub_32s(mean_ener, (ener_code>>6)));

   dwTmp = Mul2_Low_32s(gcode0* 5443);
   dwTmp = (dwTmp >> 8);
   Unpack_32s(dwTmp, &exp_enc, &frac_enc);

   gcode0 = (Ipp16s)ownPow2_AMRWB(14, frac_enc);
   exp_enc = (Ipp16s)(exp_enc - 14);

   ind2 = ShiftL_16s(index,1);
   ind2p1 = Add_16s(ind2,1);
   *gain_pit = tblGainQuantWBE_16s[ind2];
   dwTmp  = Mul2_Low_32s(tblGainQuantWBE_16s[ind2p1]*gcode0);

   if (exp_enc > -4) {
     *gain_code = ShiftL_32s(dwTmp, (Ipp16u)(exp_enc+4));
   } else {
      *gain_code = dwTmp >> (-(exp_enc+4));
   }

   *past_gpit = *gain_pit;

   dwTmp = *gain_code;
   exp_gcode = Norm_32s_I(&dwTmp);
   mant_code = (Ipp16s)(dwTmp >> 16);

   dwTmp = gcode_inov;
   exp_inov = Norm_32s_I(&dwTmp);
   mant_inov = (Ipp16s)(dwTmp >> 16);


   if (mant_code > mant_inov) {
      exp_gcode = (Ipp16s)(exp_gcode - 1);
      mant_code = (Ipp16s)(mant_code >> 1);
   }
   exp_gcode = (Ipp16s)(exp_gcode - exp_inov);
   mant_code = Div_16s(mant_code, mant_inov);
   if ((3+exp_gcode) >= 0) {
      *past_gcode = mant_code >> (3+exp_gcode);
   } else {
      *past_gcode = ShiftL_32s(mant_code, (Ipp16u)(-3-exp_gcode));
   }

   return tblGainQuantWBE_16s[ind2p1];
}

Ipp16s Scale_exc(
  Ipp16s* exc,
  Ipp32s lenSubframe,
  Ipp32s L_gain_code,
  Ipp16s *Q_exc,
  Ipp16s *Q_exsub,
  Ipp32s len
)
{
   Ipp32s i;
   Ipp16s tmp16, max16s, new_Q;

   max16s = 1;
   ippsMaxAbs_16s(exc, lenSubframe, &max16s);
   max16s = (Ipp16s)(IPP_MAX(1, max16s));

   tmp16 = (Ipp16s)(Exp_16s(max16s) + (*Q_exc) - 3);

   if (tmp16 > 12) {
      tmp16 = 12;
   }

   new_Q = -1;
   while ((L_gain_code<0x08000000L) && (new_Q<tmp16)) {
      new_Q = (Ipp16s)(new_Q + 1);
      L_gain_code = ShiftL_32s(L_gain_code, 1);
   }
   tmp16 = new_Q;
   for (i=0; i<7; i++) {
      if (Q_exsub[i] < new_Q) {
         new_Q = Q_exsub[i];
      }
   }
   Q_exsub[6] = Q_exsub[5];
   Q_exsub[5] = Q_exsub[4];
   Q_exsub[4] = Q_exsub[3];
   Q_exsub[3] = Q_exsub[2];
   Q_exsub[2] = Q_exsub[1];
   Q_exsub[1] = Q_exsub[0];
   Q_exsub[0] = tmp16;

   tmp16 = (Ipp16s)(new_Q - (*Q_exc));

   if (tmp16 != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(&exc[-len], (len+lenSubframe), tmp16);
   }

   *Q_exc = new_Q;

   return tmp16;
}

void Rescale_mem(Ipp16s *mem_syn2, DecoderObj_AMRWBE* st)
{
   Ipp16s expn, new_Q, tmp;

   new_Q = (Ipp16s)(st->Q_exc - 6);
   if (new_Q < -1) {
      new_Q = -1;
   }
   tmp = new_Q;
   if(st->prev_Q_syn < tmp) {
      tmp = st->prev_Q_syn;
   }
   st->prev_Q_syn = new_Q;

   expn = (Ipp16s)(tmp - st->Q_syn);
   st->Q_syn = tmp;

   if(expn != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(mem_syn2, M, expn);
   }
}

void Adapt_low_freq_deemph_ecu(Ipp16s* xri, Ipp32s len, Ipp16s Q_ifft, DecoderObj_AMRWBE* st)
{
   Ipp16s buf[L_TCX];
   Ipp32s pred_max, max;
   Ipp16s m_max, e_max;
   Ipp16s e_pred, m_pred;
   Ipp32s curr_mode;

   e_pred = 0;
   m_pred = 0;

   if (len == 1152) {
      curr_mode =3;
   } else if (len == 576) {
      curr_mode =2;
   } else {
      curr_mode =1;
   }

   if ((st->last_mode!=curr_mode) || (curr_mode<=2)) {
      pred_max = 0;
   } else {
      if(Q_ifft != st->Old_Qxri) {
         ownScaleSignal_AMRWB_16s_ISfs(st->wold_xri, L_TCX, (Ipp16s)(Q_ifft-st->Old_Qxri));
      }
      ippsCopy_16s(st->wold_xri, buf, len>>2);

      pred_max = SpPeak1k6(buf, &e_pred, len>>2);
      m_pred = (Ipp16s)(pred_max>>16);
      if (e_pred > 0) {
         pred_max = ShiftL_32s(pred_max, e_pred);
      } else {
         pred_max >>= (-e_pred);
      }
   }

   max = SpPeak1k6(xri, &e_max, len>>2);
   m_max = (Ipp16s)(max>>16);
   if (e_max > 0) {
      max = ShiftL_32s(max, e_max);
   } else {
      max >>= (-e_max);
   }

   if((max>pred_max) && (pred_max!=0)) {
      m_max = m_pred;
      e_max = e_pred;
   }

   Deemph1k6(xri, e_max, m_max, len>>2);

   return;
}


void Reconst_spect(Ipp16s* xri, Ipp16s* old_xri, Ipp32s n_pack, Ipp32s* bfi,
      Ipp32s len, Ipp32s last_mode, Ipp16s Q_xri)
{
   Ipp32s Lbuf[L_TCX], *sp = Lbuf, *old_sp=Lbuf+L_TCX/2;
   Ipp16s lost[L_TCX/2];
   Ipp16s e_oenr, m_oenr, e_enr, m_enr;
   Ipp16s hi ,lo, e_sp, tmp16;
   Ipp16s gain_sq, gain16;
   Ipp32s energy ,old_energy;
   Ipp32s dwTmp, sp_tmp;
   Ipp16s angle, ph_c[2], ph_tmp[2], ph_d[2];
   Ipp32s bin, bin_start, bin_end, start, end;
   Ipp32s curr_mode;
   Ipp32s i, l, p, lg2, lg8;

   if(len == 1152) {
      curr_mode = 3;
   } else if (len == 576) {
      curr_mode = 2;
   } else {
      curr_mode = 1;
   }

   if((last_mode!=curr_mode) || (curr_mode<=2)) {
      return;
   }
   lg2 = len >> 1;
   lg8 = len >> 3;
   for (i=0; i<lg2; i++) {
      lost[i] = 0;
   }
   for (p=0; p<n_pack; p++) {
      if (bfi[p] > 0) {
         for (l=p; l<lg8; l=l+n_pack) {
            for (i=0; i<4; i++) {
               lost[4*l+i] = 1;
            }
         }
      }
   }

   old_sp[0] = Mul2_Low_32s(old_xri[0]*old_xri[0]);
   for (i=1; i<lg2; i++) {
      dwTmp = Mul2_Low_32s(old_xri[2*i]*old_xri[2*i]);
      old_sp[i] = Add_32s(dwTmp, Mul2_Low_32s(old_xri[2*i+1]*old_xri[2*i+1]));
   }

   sp[0] = Mul2_Low_32s(xri[0]*xri[0]);
   for (i=1; i<lg2; i++) {
      dwTmp = Mul2_Low_32s(xri[2*i]*xri[2*i]);
      sp[i] = Add_32s(dwTmp, Mul2_Low_32s(xri[2*i+1]*xri[2*i+1]));
   }
   energy = 0;
   old_energy = 0;
   for (i=0; i<lg2; i++) {
      if (sp[i] > 0) {
         energy = Add_32s(energy, sp[i]);
         old_energy = Add_32s(old_energy, old_sp[i]);
      }
   }

   if (energy == 0) {
      gain16 = 0;
   } else {
      if (old_energy > 0) {
         dwTmp = old_energy;
         e_oenr = (Ipp16s)(30 - Norm_32s_I(&dwTmp));
         m_oenr = (Ipp16s)(dwTmp >> 16);

         dwTmp = energy;
         e_enr = (Ipp16s)(30 - Norm_32s_I(&dwTmp));
         m_enr = (Ipp16s)(dwTmp >> 16);

         if(m_oenr > m_enr) {
            m_oenr = (Ipp16s)(m_oenr >> 1);
            e_oenr = (Ipp16s)(e_oenr + 1);
         }
         m_enr = Div_16s(m_oenr, m_enr);
         e_enr = (Ipp16s)(e_oenr - e_enr);

         energy = ((Ipp32s)m_enr)<<16;

         InvSqrt_32s16s_I(&energy, &e_enr);
         if (e_enr > 1) {
            gain16 = (Ipp16s)(ShiftL_32s(energy, (Ipp16u)(e_enr-1))>>16);
         } else {
            gain16 = (Ipp16s)((energy >> (1-e_enr))>>16);
         }

         if(gain16 > 23170)
            gain16 = 23170;
      } else {
         gain16 = 23170;
      }
   }
   dwTmp = Mul2_Low_32s(gain16*gain16);
   gain_sq = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,1));

   for (i=0; i<len; i++) {
      if (lost[i>>1] > 0) {
         dwTmp = Mul2_Low_32s(gain16*old_xri[i]);
         xri[i] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,1));
      }
   }
   for (i=0; i<lg2; i++) {
      if (lost[i] > 0) {
         Unpack_32s(old_sp[i], &hi, &lo);
         dwTmp = Mpy_32_16(hi,lo, gain_sq);
         sp[i]= ShiftL_32s(dwTmp,1);
      }
   }
   bin = 0;
   do {
      if (lost[bin] > 0) {
         bin_start = bin;
         while((lost[bin]>0) && (bin<lg2)) {
            bin = bin + 4;
         }
         bin_end = bin;
         if (bin_start == 0) {
            bin_start = 1;
         }

         if (bin_end != lg2) {
            start = (bin_start-1) << 1;
            end = bin_end << 1;
            Get_phasor(&xri[start],ph_tmp);
            Get_phasor(&xri[end],ph_c);
            Div_phasor(ph_c,ph_tmp,ph_c);

            Get_phasor(&old_xri[start],ph_tmp);
            Mult_phasor(ph_c,ph_tmp,ph_c);
            Get_phasor(&old_xri[end],ph_tmp);
            Div_phasor(ph_c,ph_tmp,ph_c);

            angle = Atan2(ph_c[1],ph_c[0]);
            angle = MulHR_16s(angle, tblLen_16s[(bin_end-bin_start) + (1-4)]);
            Find_x_y(angle, ph_c);
         } else {
            ph_c[0] = 32767;
            ph_c[1] = 0;
         }

         for (i=bin_start; i<bin_end; i++) {
            Get_phasor(&old_xri[2*(i-1)],ph_tmp);
            Get_phasor(&old_xri[2*i],ph_d);
            Div_phasor(ph_d,ph_tmp,ph_d);
            Get_phasor(&xri[2*(i-1)],ph_tmp);
            Mult_phasor(ph_tmp,ph_d,ph_tmp);
            Mult_phasor(ph_tmp,ph_c,ph_tmp);

            if (sp[i] == 0) {
               sp_tmp = 1;
            } else {
               sp_tmp = sp[i];
            }

            dwTmp = sp_tmp;
            e_sp = (Ipp16s)(30 - Norm_32s_I(&dwTmp));

            e_sp = (Ipp16s)(e_sp - (Q_xri<<1));
            InvSqrt_32s16s_I(&dwTmp, &e_sp);
            tmp16 = Cnvrt_NR_32s16s(dwTmp);
            tmp16 = Div_16s(16384,tmp16);

            e_sp = (Ipp16s)(2 - e_sp);
            if ((Q_xri+e_sp) > 0) {
               e_enr = MulHR_16s(tmp16,ph_tmp[0]);
               xri[2*i] = Cnvrt_NR_32s16s(ShiftL_32s(e_enr, (Ipp16u)(Q_xri+e_sp)));
               e_enr= MulHR_16s(tmp16,ph_tmp[1]);
               xri[2*i+1] = Cnvrt_NR_32s16s(ShiftL_32s(e_enr, (Ipp16u)(Q_xri+e_sp)));
            } else {
               e_enr = MulHR_16s(tmp16,ph_tmp[0]);
               xri[2*i] = Cnvrt_NR_32s16s(e_enr >>(-(Q_xri+e_sp)));
               e_enr= MulHR_16s(tmp16,ph_tmp[1]);
               xri[2*i+1] = Cnvrt_NR_32s16s(e_enr>>(-(Q_xri+e_sp)));
            }
         }
      }
      bin  = bin + 4;
   } while (bin < lg2);
}


void NoiseFill(Ipp16s *xri, Ipp16s *buf, Ipp16s *seed_tcx, Ipp16s fac_ns, Ipp16s Q_ifft, Ipp32s len)
{
   Ipp32s lg6, i, k;
   Ipp32s dwTmp;

   lg6 = MulHR_16s((Ipp16s)len, 5461);

   ippsZero_16s(buf, len);
   Rnd_ph16(seed_tcx, &buf[lg6], (len-lg6), Q_ifft);

   for (k=lg6; k<len; k+=8) {
      dwTmp = 0;
      for(i=k; i<k+8; i++) {
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(xri[i]*xri[i]));
      }
      if (dwTmp == 0) {
         for(i=k; i<k+8; i++) {
            xri[i] = MulHR_16s(fac_ns, buf[i]);
         }
      }
   }
}

Ipp16s Find_mpitch(Ipp16s* xri, Ipp32s len)
{
  Ipp32s dwTmp, Lmax;
  Ipp16s pitch, mpitch, exp_tmp, tmp16;
  Ipp32s i, n, lg4;


   Lmax = 0;
   n = 2;
   lg4 = len >> 4;
   for (i=2; i<lg4; i+=2) {
      dwTmp = Mul2_Low_32s(xri[i]*xri[i]);
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(xri[i+1]*xri[i+1]));
      if (dwTmp > Lmax) {
         Lmax = dwTmp;
         n = i;
      }
   }
   exp_tmp  = Exp_16s((Ipp16s)n);
   tmp16 = ShiftL_16s((Ipp16s)n, exp_tmp);
   exp_tmp = (Ipp16s)(15 - exp_tmp);
   tmp16 = Div_16s(16384, tmp16);
   exp_tmp = (Ipp16s)(2 - exp_tmp);
   if (exp_tmp > 0) {
      pitch = Cnvrt_NR_32s16s(Mul2_Low_32s(len * ShiftL_16s(tmp16, exp_tmp)));
   } else {
      pitch = Cnvrt_NR_32s16s(Mul2_Low_32s(len * (tmp16 >> (-exp_tmp))));
   }

   if (pitch >= 256) {
      n = 256;
   } else {
      mpitch = pitch;
      while (mpitch < 256) {
         mpitch = Add_16s(pitch, mpitch);
      }
      n = Sub_16s(mpitch, pitch);
   }
   return (Ipp16s)(n);
}

void Scale_mem_tcx(Ipp16s* xnq, Ipp32s len, Ipp32s Lgain, Ipp16s* mem_syn, DecoderObj_AMRWBE *st)
{
   Ipp16s max16s, tmp, expn, tmpgain, tmp16, new_Q, tmpQ;
   Ipp32s i, dwTmp;

   dwTmp = Lgain;
   tmp16 = Norm_32s_I(&dwTmp);
   tmpgain = (Ipp16s)(dwTmp >> 16);

   tmp16 = (Ipp16s)(tmp16 - 15);

   for (i=0; i<len; i++) {
      xnq[i] = (Ipp16s)((xnq[i]*tmpgain)>>15);
   }

   max16s = 0;
   ippsMaxAbs_16s(xnq, len, &max16s);
   //max16s = (Ipp16s)(IPP_MAX(0, max16s));

   tmp = (Ipp16s)(Exp_16s(max16s) - 2);
   if ((tmp+tmp16) > Q_MAX) {
      tmp = (Ipp16s)(Q_MAX - tmp16);
   }
   tmpQ = (Ipp16s)(tmp + tmp16);
   new_Q = tmpQ;
   for (i=0; i<7; i++) {
      if (st->mem_subfr_q[i] < new_Q) {
         new_Q = st->mem_subfr_q[i];
      }
   }
   tmp = (Ipp16s)(new_Q - tmp16);

   if (tmp != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(xnq, len, tmp);
   }

   st->Q_exc = new_Q;

   if(len == 288) {
      st->mem_subfr_q[6] = st->mem_subfr_q[5];
      st->mem_subfr_q[5] = st->mem_subfr_q[4];
      st->mem_subfr_q[4] = st->mem_subfr_q[3];
      st->mem_subfr_q[3] = tmpQ;
      st->mem_subfr_q[2] = tmpQ;
      st->mem_subfr_q[1] = tmpQ;
      st->mem_subfr_q[0] = tmpQ;
   } else {
      for (i=0; i<7; i++) {
         st->mem_subfr_q[i] = tmpQ;
      }
   }

   new_Q = (Ipp16s)(new_Q - 4);
   if (new_Q < -1) {
      if (st->Q_exc < 0) {
         new_Q = st->Q_exc;
      } else {
         new_Q = -1;
      }
   }
   tmp = new_Q;
   if(st->prev_Q_syn < tmp) {
      tmp = st->prev_Q_syn;
   }
   st->prev_Q_syn = new_Q;

   expn = (Ipp16s)(tmp - st->Q_syn);
   st->Q_syn = tmp;

   if (expn != 0) {
      ownScaleSignal_AMRWB_16s_ISfs(mem_syn, M, expn);
   }

   if(st->Q_exc != st->Old_Qxnq) {
      ownScaleSignal_AMRWB_16s_ISfs(st->wwovlp, L_OVLP, (Ipp16s)(st->Q_exc - st->Old_Qxnq));
      ownScaleSignal_AMRWB_16s_ISfs(&(st->wmem_wsyn), 1, (Ipp16s)(st->Q_exc - st->Old_Qxnq));
   }
   st->Old_Qxnq  = st->Q_exc;
   return;
}


static void Deemph1k6(Ipp16s* xri, Ipp16s e_max, Ipp16s m_max, Ipp32s len)
{
  Ipp32s dwTmp;
  Ipp32s i, j, i8;
  Ipp16s exp_tmp, tmp16,fac;

   fac = 3277;
   for (i=0; i<len; i+=8) {
      dwTmp = 0;
      i8 = i + 8;
      for(j=i; j<i8; j++) {
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(xri[j]*xri[j]));
      }
      exp_tmp = Norm_32s_I(&dwTmp);
      exp_tmp = (Ipp16s)(30 - exp_tmp);
      InvSqrt_32s16s_I(&dwTmp, &exp_tmp);

      tmp16 = (Ipp16s)(dwTmp>>16);

      if (m_max > tmp16) {
         m_max = (Ipp16s)(m_max>>1);
         e_max = (Ipp16s)(e_max + 1);
      }
      tmp16 = Div_16s(m_max, tmp16);
      exp_tmp = (Ipp16s)(e_max - exp_tmp);
      if (exp_tmp > 0) {
         tmp16 = ShiftL_16s(tmp16, exp_tmp);
      } else {
         tmp16 = (Ipp16s)(tmp16 >> (-exp_tmp));
      }

      if (tmp16 > fac) {
         fac = tmp16;
      }

      for (j=i; j<i8; j++) {
         xri[j] = (Ipp16s)((xri[j]*fac)>>15);
      }
   }
   return;
}

static void Get_phasor(
  Ipp16s xri[],
  Ipp16s ph[]
)
{
  Ipp16s e_tmp, gain;
  Ipp32s dwTmp;

  dwTmp  = Mul2_Low_32s(xri[0]*xri[0]);
  dwTmp = Add_32s(dwTmp, Mul2_Low_32s(xri[1]*xri[1]));

   if(dwTmp == 0) {
      ph[0] = 32767;
      ph[1] = 0;
   } else  {
      e_tmp = (Ipp16s)(30 - Norm_32s_I(&dwTmp));
      InvSqrt_32s16s_I(&dwTmp, &e_tmp);
      gain = Cnvrt_NR_32s16s(dwTmp);
      ph[0] = MulHR_16s(xri[0], gain);
      ph[1] = MulHR_16s(xri[1], gain);
      ownScaleSignal_AMRWB_16s_ISfs(ph, 2, (Ipp16s)(15+e_tmp));
   }
}

static void Mult_phasor(Ipp16s *ph1, Ipp16s *ph2, Ipp16s *res)
{
   Ipp16s re, im;
   Ipp32s dwTmp;

   dwTmp = Mul2_Low_32s(ph1[0]*ph2[0]);
   dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(ph1[1]*ph2[1]));
   re = Cnvrt_NR_32s16s(dwTmp);

   dwTmp = Mul2_Low_32s(ph1[1]*ph2[0]);
   dwTmp = Add_32s(dwTmp, Mul2_Low_32s(ph1[0]*ph2[1]));
   im = Cnvrt_NR_32s16s(dwTmp);

   res[0] = re;
   res[1] = im;
}

static void Div_phasor(Ipp16s* ph1, Ipp16s* ph2, Ipp16s* res)
{
   Ipp16s re, im;
   Ipp32s dwTmp;

   dwTmp = Mul2_Low_32s(ph1[0]*ph2[0]);
   dwTmp = Add_32s(dwTmp, Mul2_Low_32s(ph1[1]*ph2[1]));
   re = Cnvrt_NR_32s16s(dwTmp);

   dwTmp = Mul2_Low_32s(ph1[1]*ph2[0]);
   dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(ph1[0]*ph2[1]));
   im = Cnvrt_NR_32s16s(dwTmp);

   res[0] = re;
   res[1] = im;
}


static Ipp16s Atan2(Ipp16s ph_y, Ipp16s ph_x)
{
   Ipp16s ent_frac, alpha, alpham1;
   Ipp16s Num, angle, Den, e_num, m_num, e_den, m_den,  sign_x = 1, sign_y = 1;
   Ipp32s frac, index, dwTmp;

   Den = ph_x;
   Num = ph_y;

   if (Num < 0) {
      Num = Negate_16s(Num);
      sign_y = -1;
   }

   if (Den < 0) {
      Den = Negate_16s(Den);
      sign_x = -1;
   }

   if (Den == 0) {
      angle = 6434;
   } else if (Num == 0) {
      angle = 0;
   } else {
      e_num = Exp_16s(Num);
      m_num = ShiftL_16s(Num, e_num);
      e_num = (Ipp16s)(15 - e_num);

      e_den = Exp_16s(Den);
      m_den = ShiftL_16s(Den, e_den);
      e_den = (Ipp16s)(15 - e_den);

      if (m_num > m_den) {
         m_num = (Ipp16s)(m_num >> 1);
         e_num = (Ipp16s)(e_num + 1);
      }

      m_num = Div_16s(m_num,m_den);
      e_num = (Ipp16s)(e_num - e_den);
      if (e_num > 0) {
         frac = ShiftL_32s(m_num, e_num);
      } else {
         frac = (m_num >> (-e_num));
      }

      ent_frac = (Ipp16s)((ShiftL_32s(frac,1))>>16);

      if (ent_frac >= 88) {
         alpha = ShiftL_16s(Sub_16s(ent_frac, 88), 9);
         alpham1 = Sub_16s(32767, alpha);
         dwTmp = Mul2_Low_32s(alpham1*tblTXV_16s[30]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*25736));
         angle = Cnvrt_NR_32s16s(dwTmp>>2);
      } else if (ent_frac >= 24) {
         alpha = ShiftL_16s(Sub_16s(ent_frac, 24), 9);
         alpham1 = Sub_16s(32767, alpha);
         dwTmp = Mul2_Low_32s(alpham1*tblTXV_16s[29]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblTXV_16s[30]));
         angle = Cnvrt_NR_32s16s(dwTmp>>2);
      } else if (ent_frac > 12) {
         index = (frac>>16) + 18;
         alpha = (Ipp16s)((frac>>1)&0x7fff);
         alpham1 = Sub_16s(32767, alpha);
         dwTmp = Mul2_Low_32s(alpham1*tblTXV_16s[index]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblTXV_16s[index+1]));
         angle = Cnvrt_NR_32s16s(dwTmp>>2);
      } else if (ent_frac >= 4) {
         index = ent_frac + 12;
         alpha = (Ipp16s)(frac&0x7fff);
         alpham1 = Sub_16s(32767, alpha);
         dwTmp = Mul2_Low_32s(alpham1*tblTXV_16s[index]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblTXV_16s[index+1]));
         angle = Cnvrt_NR_32s16s(dwTmp>>2);
      } else {
         index = (ShiftL_32s(frac,3)) >> 16;
         alpha = (Ipp16s)(ShiftL_32s(frac,2)&0x7fff);
         alpham1 = Sub_16s(32767, alpha);
         dwTmp = Mul2_Low_32s(alpham1*tblTXV_16s[ index]);
         dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblTXV_16s[index+1]));
         angle = Cnvrt_NR_32s16s(dwTmp>>2);
      }
   }

   if (((sign_x*sign_y)>>15)>=0) {
      if (sign_x > 0) {
         return angle;
      } else {
         return Sub_16s(angle, 12868);
      }
   } else {
      if (sign_x < 0) {
         if (angle == 0) {
            return 12868;
         } else {
            return Sub_16s(12868, angle);
         }
      } else {
         return Negate_16s(angle);
      }
   }
}

static void Find_x_y(Ipp16s angle, Ipp16s *ph)
{
   Ipp16s index, fr_ind, hi, lo, alpha, alpham1;
   Ipp32s dwTmp;

   dwTmp = ShiftL_32s(angle, 9);
   Unpack_32s(dwTmp, &hi, &lo);
   dwTmp = Mpy_32_16(hi, lo, 23468);
   index = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,31-13-15));
   fr_ind = Abs_16s((Ipp16s)(Sub_32s(dwTmp, Mul2_Low_32s(index*4096))));

   alpha = ShiftL_16s(fr_ind,2);
   alpham1 = Sub_16s(32767, alpha);

   dwTmp = Mul2_Low_32s(alpham1*tblSinCos_16s[index + COSOFFSET]);
   dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblSinCos_16s[index+1+COSOFFSET]));
   ph[0] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,2));

   if(angle>0) {
      dwTmp = Mul2_Low_32s(alpham1*tblSinCos_16s[index]);
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblSinCos_16s[index+1]));
      ph[1] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,2));
   } else {
      dwTmp = Mul2_Low_32s(alpham1*tblSinCos_16s[index + 4*COSOFFSET]);
      dwTmp = Add_32s(dwTmp, Mul2_Low_32s(alpha*tblSinCos_16s[index+1+4*COSOFFSET]));
      ph[1] = Cnvrt_NR_32s16s(ShiftL_32s(dwTmp,2));
   }
}


static void Rnd_ph16(Ipp16s *seed, Ipp16s *xri, Ipp32s len, Ipp16s Qifft)
{
   Ipp16s scale;
   Ipp32s i, phase;

   scale  = (Ipp16s)(15 - Qifft);
   if (scale >= 0) {
      for (i=0; i<len; i+=2) {
         phase = (Ipp16s)((Rand_16s(seed)>>12) & 0x000f);
         xri[i] = (Ipp16s)(tblRandPh16_16s[phase+4] >> scale);
         xri[i+1] = (Ipp16s)(tblRandPh16_16s[phase] >> scale);
      }
   } else {
      for (i=0; i<len; i+=2) {
         phase = (Ipp16s)((Rand_16s(seed)>>12) & 0x000f);
         xri[i] = ShiftL_16s(tblRandPh16_16s[phase+4], (Ipp16u)(-scale));
         xri[i+1] = ShiftL_16s(tblRandPh16_16s[phase], (Ipp16u)(-scale));
      }
   }
}


void Simple_frame_limiter(Ipp16s* x, Ipp16s Qx, Ipp16s* mem, Ipp32s n)
{
   Ipp16s fac, prev, tmp, frame_fac, max16s;
   Ipp32s i, dwTmp;

   max16s = 0;
   ippsMaxAbs_16s(x, n, &max16s);
   //max16s = (Ipp16s)(IPP_MAX(0, max16s));

   if (Qx > 0) {
      tmp = ShiftL_16s(30000, Qx);
   } else {
      tmp = (Ipp16s)(30000 >> (-Qx));
   }
   frame_fac = 0;

   if (max16s > tmp) {
      frame_fac = Sub_16s(16384, Div_16s((Ipp16s)(tmp>>1), max16s));
   }

   fac = mem[0];
   prev = mem[1];

   if (Qx >= 0) {
      for (i=0; i<n; i++) {
         fac = Cnvrt_NR_32s16s(Add_32s(Mul2_Low_32s(32440*fac), Mul2_Low_32s(328*frame_fac)));
         dwTmp = ((Ipp32s)x[i])<<16;
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(fac*x[i]));
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(fac*prev));
         dwTmp = dwTmp >> Qx;
         prev = x[i];
         x[i] = Cnvrt_NR_32s16s(dwTmp);
      }
   } else {
      for (i=0; i<n; i++) {
         fac = Cnvrt_NR_32s16s(Add_32s(Mul2_Low_32s(32440*fac), Mul2_Low_32s(328*frame_fac)));
         dwTmp = ((Ipp32s)x[i])<<16;
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(fac*x[i]));
         dwTmp = Sub_32s(dwTmp, Mul2_Low_32s(fac*prev));
         dwTmp = ShiftL_32s(dwTmp, (Ipp16u)(-Qx));
         prev = x[i];
         x[i] = Cnvrt_NR_32s16s(dwTmp);
      }
   }

   mem[0] = fac;
   mem[1] = prev;
}


