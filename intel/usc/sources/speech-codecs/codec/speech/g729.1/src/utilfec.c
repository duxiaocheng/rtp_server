/*****************************************************************************
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2006-2012 Intel Corporation. All Rights Reserved.
//
//  Purpose: G7291 speech codec: FEC functions
//
***************************************************************************/

#include "owng7291.h"

static void FEC_synchro_exc(short * exc, short true_puls_pos, short desire_puls_pos, short Old_pitch);
static void FEC_syn_bfi(short last_good, short sBFICount, const short sStabFactor, short * sDampedPitchGain,
                               short * sDampedCodeGain, short * pExcMem, short * exc2, const short T3,
                               const short sTiltCode, const short Ts, short * pitch_buf, const short Tcnt,
                               short * Aq, short * pSynth, short * mem_syn2, int * iEnergy, short * seed,
                               short * exc, short puls_pos, short new_pit);

/*   Pitch prediction for frame erasure */
static void FEC_pitch_pred(
short bfi,     /* i: Bad frame ?                     */
short *T,      /* i/o: Pitch                         */
short *T_fr,   /* i/o: fractional pitch             */
short *pMemPitch,/* i/o: Pitch memories                */
short *sBFI)/* i/o: Memory of bad frame indicator */
{
  short pit, a, b, sum0, sum1;
  int   tmp32;
  short tmp16;
  short i;

  if (bfi != 0)
  {
    /* Correct pitch */
    if (*sBFI == 0)
    {
      for (i = 3; i >= 0; i--)
      {
        if (Abs_16s(pMemPitch[i] - pMemPitch[i + 1]) > 128)
        {
          pMemPitch[i] = pMemPitch[i + 1];
        }
      }
    }

    /* Linear prediction (estimation) of pitch */
    sum0 = 0;
    tmp32 = 0;
    for (i = 0; i < 5; i++)
    {
      sum0 = Add_16s(sum0, pMemPitch[i]);
      tmp32 += (i * pMemPitch[i]) << 1;
    }
    sum1 = (short)(tmp32 >> 2);
    a = (19661 * sum0 - 13107 * sum1)>>15;
    b = sum1 - sum0;
    pit = Add_16s(a, b);

    if (pit > G7291_FEC_PIT_MAX32)
    {
      pit = G7291_FEC_PIT_MAX32;
    }
    if (pit < G7291_FEC_PIT_MIN32)
    {
      pit = G7291_FEC_PIT_MIN32;
    }
    *T = (pit + 16) >> 5;
    tmp16 = *T << 5;
    if (pit >= tmp16)
    {
      *T_fr = ((pit - tmp16) * 3072 + 0x4000) >> 15;
    }
    else
    {
      *T_fr = Negate_16s(((tmp16 - pit) * 3072 + 0x4000) >> 15);
    }
  }
  else
  {
    pit = (*T << 5) + (((*T_fr << 4) * 21845 + 0x4000) >> 15);
  }

  /* Update memory */
  for(i = 0; i < 4; i++)
  {
    pMemPitch[i] = pMemPitch[i + 1];
  }
  pMemPitch[4] = pit;
  *sBFI = bfi;
  return;
}
/*  Compute pitch-synchronous energy at the frame end */
static short FEC_frame_energy( /* o:   Pitch synchronus energy                        Q8 */
const short *pitch,   /* i:   pitch values for each subframe                 Q5 */
const short *speech,  /* i:   pointer to speech signal for E computation        */
const short sLPSpeechEnergu,/* i:   long term active speech energy average         Q8 */
short *frame_ener,    /* o:   pitch-synchronous energy at frame end          Q8 */
short Q_syn)          /* i:   Scaling of syntesis                               */
{
  int    acc32;
  short  *pt1, tmp16, exp1, frac1, tmp1, tmp2;
  short  len, enern;

  len = (((pitch[2] + pitch[3]) >> 1) + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;

  if (len < G7291_LSUBFR)
  {
    len = len << 1;
  }
  pt1 = (short *) speech + G7291_LFRAME2 - len;

  tmp2 = len;
  tmp1 = Norm_16s_I(&tmp2);
  tmp1 = 15 - tmp1;

  ippsDotProd_16s32s_Sfs(pt1, pt1, len, &acc32, -1);
  acc32 += 1;
  exp1 = 30 - Norm_32s_I(&acc32);
  tmp16 = (short)(acc32 >> 16);

  if(tmp16 > tmp2)
  {
    tmp16 = tmp16 >> 1;
    exp1 = exp1 + 1;
  }
  tmp16 = Div_16s(tmp16, tmp2);
  exp1 = exp1 - tmp1;

  Log2(tmp16 << 16, &tmp16, &frac1);

  exp1 = exp1 + 29 - tmp16 - (Q_syn << 1);

  frac1 = (frac1 * tmp2 + 0x4000) >> 15;

  acc32 = Mpy_32_16(exp1, frac1, G7291_FEC_LG10);

  *frame_ener = (short)(acc32 >> 6);
  enern = *frame_ener - sLPSpeechEnergu;
  return enern;
}
/* Correlation function.  Signal pVector1 is compared to target signal pVector2.
   Information about the similarity between vectors is returned in *gain */
static void Corre(
short *pVector1,    /* i:   vector 1                     Q12 */
short *pVector2,    /* i:   vector 2                     Q12 */
short len,   /* i:   length of vectors                */
short *gain) /* o:   normalized correlation gain  Q15 */
{
  int   tmp32;
  short cor, cor_exp;
  short den, den_exp;
  short den2, den2_exp;
  short tmp_exp;

  ippsDotProd_16s32s_Sfs(pVector1, pVector2, len, &tmp32, -1);
  tmp32 += 1;
  cor_exp = 30 - Norm_32s_I(&tmp32);
  cor = (short)(tmp32 >> 16);

  ippsDotProd_16s32s_Sfs(pVector2, pVector2, len, &tmp32, -1);
  tmp32 += 1;
  den_exp = 30 - Norm_32s_I(&tmp32);
  den = (short)(tmp32 >> 16);

  ippsDotProd_16s32s_Sfs(pVector1, pVector1, len, &tmp32, -1);
  tmp32 += 1;
  den2_exp = 30 - Norm_32s_I(&tmp32);
  den2 = (short)(tmp32 >> 16);

  tmp32 = (den * den2) << 1;
  tmp_exp = Norm_32s_I(&tmp32);
  tmp_exp = den_exp + den2_exp - tmp_exp;

  InvSqrt_32s16s_I(&tmp32, &tmp_exp);
  if ((cor_exp + tmp_exp-15) >= 0)
    gain[0] = Cnvrt_32s16s((cor * (tmp32>>16) + 0x4000) << (cor_exp + tmp_exp-15));
  else
    gain[0] = Cnvrt_32s16s((cor * (tmp32>>16) + 0x4000) >> (-(cor_exp + tmp_exp-15)));
  return;
}
/*  Find last glottal pulse in the last pitch lag */
static short FEC_findpulse(         /* o:   pulse position        */
const short *pResidual,  /* i:   residual signal       */
short T0,          /* i:   integer pitch         */
const short enc)   /* i:  enc = 1 -> encoder side; enc = 0 -> decoder side */
{
  const short *ptr;
  short val, maxval;
  short i, maxi, tmp16;
  int   acc32;
  short sign, sign_in;
  IPP_ALIGNED_ARRAY(16, short, resf, G7291_FEC_L_FRAME_FER); /* Low pass filtered residual */

  sign = (short) 0;
  sign_in = (short) 0;

  /* 1. Very simple LP filter */
  if(enc == 1)
  {
    acc32 = pResidual[0] << 1;
    resf[0] = (short)((acc32 + pResidual[1] + 2)>>2);
    for(i = 1; i < G7291_FEC_L_FRAME_FER - 1; i++)
    {
      acc32 = pResidual[i - 1];
      acc32 += pResidual[i] << 1;
      resf[i] = (short)((acc32 + pResidual[i + 1] + 2)>>2);
    }

    resf[G7291_FEC_L_FRAME_FER - 1] =
        (pResidual[G7291_FEC_L_FRAME_FER - 2] + pResidual[G7291_FEC_L_FRAME_FER - 1] * 2 + 2)>>2;
        /* 2. Find "biggest" pitch pulse */
    /* Find the biggest last pulse from the end of the vector */
    ptr = resf + G7291_FEC_L_FRAME_FER - 1;
    maxval = *ptr--;
    maxi = (short) 0;
    for(i = 1; i < T0; i++)
    {
      val = *ptr--;
      tmp16 = Abs_16s(val);
      if(tmp16 > maxval)
      {
        maxval = tmp16;
        maxi = i;
        if (val < 0)
        {
          sign = (short) 1;
        }
        if (val >= 0)
        {
          sign = (short) 0;
        }
      }
    }
    if (sign)
    {
      maxi = maxi + 256;
    }
  }
  else
  {
    if (T0 < 0)
    {
      T0 = Negate_16s(T0);
      sign_in = (short) 1;
    }
        /* 2. Find "biggest" pitch pulse */
    ptr = pResidual;
    /*if the decoded maximum pulse position is positive */
    if(sign_in == 0)
    {
      maxval = 0;
      maxi = (short) 0;
      for(i = 1; i <= T0; i++)
      {
        val = *ptr++;
        if (val >= maxval)
        {
          maxval = val;
          maxi = i;
        }
      }
    }
    else /*if the decoded maximum pulse position is negative */
    {
      maxval = 0;
      maxi = 0;
      for(i = 1; i <= T0; i++)
      {
        val = *ptr++;
        if (val <= maxval)
        {
          maxval = val;
          maxi = i;
        }
      }
    }
  }
  return (maxi);
}
/* Estimation of pitch-synchronous (VOICED sounds) or half-frame energy */
static short FEC_energy(
const short clas,     /* i:   frame classification                            */
const short *pSynth,  /* i:   synthesized speech                              */
const short pitch,    /* i:   pitch period                             Q0     */
int *enr_q,           /* o:   pitch-synchronous or half_frame energy   Q0     */
const short offset)   /* i:   speech pointer offset (0 or L_FRAME)            */
{
  short len, exp_enrq, pos;
  const short *pt_synth;
  short i;
  int   tmp32;
  IPP_ALIGNED_ARRAY(16, short, speech_temp, G7291_FEC_L_FRAME2_FER);

  if(clas >= G7291_FEC_VOICED)
  {
    len = pitch;
    if (len < G7291_LSUBFR)
    {
      len = len << 1;
    }
    if(offset == 0)
    {
      pt_synth = pSynth;
    }
    else
    {
      pt_synth = pSynth + G7291_FEC_L_FRAME_FER - len;
    }

    *enr_q = (int) 0;

    for(i = 0; i < len; i++)
    {
        tmp32 = pt_synth[i] * pt_synth[i];
        if (tmp32 > *enr_q)
        {
            *enr_q = tmp32;
        }
    }
    *enr_q = Mul4_32s(*enr_q);

    exp_enrq = (short) 0;
  }
  else
  {
    pos = (short) 0;
    if (offset)
    {
      pos = G7291_FEC_L_FRAME2_FER;
    }
    ippsCopy_16s((short *) pSynth + pos, speech_temp, G7291_FEC_L_FRAME2_FER);
    for (i = 0; i < G7291_FEC_L_FRAME2_FER; i++)
    {
        tmp32 = speech_temp[i] << 16;
        tmp32 = tmp32 >> 2;
        speech_temp[i] = Cnvrt_NR_32s16s(tmp32);
    }
    ippsDotProd_16s32s_Sfs(speech_temp, speech_temp, G7291_FEC_L_FRAME2_FER, enr_q, -1);
    *enr_q += 1;
    exp_enrq = -6;
  }
  return exp_enrq;
}

/* Frame erasure concealment.
   Find LPC coefficient, pitch and reconstruct lost synthesis */
void ownDecode_FEC(
short *parm,           /* (i)   : vector of synthesis parameters       */
short *pSynth,         /* (i/o) : synthesis speech                     */
short *Az_dec,         /* (o)   : decoded LP filter in 2 subframes     */
int bfi,               /* (i)   : Bad Frame Indicator                  */
short *last_good,      /* (i)   : last good frame classification       */
short *pitch_buf,      /* (i/o) : Pitch memory                         */
short *pExcVector,        /* (i/o) : excitiation memory                   */
G729_DecoderState *st, /* (i)   : Decoder structure                    */
short puls_pos)        /* (i)   : Position of the last pulse           */
{
  IPP_ALIGNED_ARRAY(16, short, A, G7291_NB_SUBFR * G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, lsp_new, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, isp, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, exc2, G7291_LFRAME2 + G7291_FEC_L_FIR_FER - 1);
  IPP_ALIGNED_ARRAY(16, short, exc_buf, G7291_LFRAME2 + G7291_PITCH_LAG_MAX + G7291_L_INTERPOL + G7291_LSUBFR);
  short qIndex[4];
  short *exc, T3;
  short *p_a;
  int   i, k;
  short T0, T0_frac;
  short t0;
  short index;

  qIndex[0] = (short)((parm[0] >> G7291_NC0_B) & (short) 1);
  qIndex[1] = (short)(parm[0] & (short) (G7291_NC0 - 1));
  qIndex[2] = (short)((parm[1] >> G7291_NC1_B) & (short) (G7291_NC1 - 1));
  qIndex[3] = (short)(parm[1] & (short) (G7291_NC1 - 1));

  T0 = (short) 0;
  T0_frac = (short) 0;
  exc = exc_buf + G7291_PITCH_LAG_MAX + G7291_L_INTERPOL;
  ippsCopy_16s(pExcVector, exc_buf, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);

  /* Decode the LSPs */
  if (!bfi) {
    st->sMAPredCoef = qIndex[0];
    ippsLSFDecode_G729_16s(qIndex, (short*)st->pPrevLSFVector, st->pLSFVector);
    ippsLSFToLSP_G729_16s(st->pLSFVector, lsp_new);
  } else if (bfi > 0) {
    ippsLSFDecodeErased_G729_16s(st->sMAPredCoef, (short*)st->pPrevLSFVector, st->pLSFVector);
    ippsLSFToLSP_G729_16s(st->pLSFVector, lsp_new);
  } else {
    st->sMAPredCoef = qIndex[0];
    ippsLSFDecode_G7291_16s(qIndex, (short*)st->pPrevLSFVector, st->pLSFVector);
    ippsLSFToLSP_G729_16s(st->pLSFVector, lsp_new);
  }
  parm += 2;

  /* Interpolation of LPC for the 4 subframes */
  p_a = A;
  for(k = 0; k < G7291_NB_SUBFR; k++)
  {
      for(i = 0; i < G7291_LP_ORDER; i++)
      {
          isp[i] = (st->pLSPVector[i] * 16383 + lsp_new[i]*16384 + 0x4000)>>15;
      }
      ippsLSPToLPC_G729_16s(isp, p_a);
      p_a += (G7291_G729_MP1);
  }
  ippsCopy_16s(A, Az_dec, G7291_NB_SUBFR * G7291_G729_MP1);
  for(i = 0; i < G7291_LP_ORDER; i++) st->pLSPVector[i]=lsp_new[i];
  FEC_pitch_pred(1, &T0, &T0_frac, st->pMemPitch, &(st->sBFI));
  if (bfi == G7291_FEC_SINGLE_FRAME)
  {
    index = *parm;            /* pitch index */
    /* decode future pitch */
    if (index < 197)
    {
      t0 = (index+2)/3 + 19;
    }
    else
    {
      t0 = index - 112;
    }
  }
  else
  {
    t0 = T0;                    /* t0 = predicted pitch */
  }
  T3 = st->pMemPitch[3];
  /* Find the excitation and synthesize speech */
  FEC_syn_bfi(*last_good, st->sBFICount, st->sStabFactor, &st->sDampedPitchGain, &st->sDampedCodeGain, st->pExcVector, exc2,
                     T3, st->sTiltCode, st->sFracPitch, pitch_buf, st->sUpdateCounter,
                     A, pSynth, st->pSynthMem, &(st->iEnergy), &(st->sFERSeed), exc, puls_pos, t0);
  ippsCopy_16s(&exc[G7291_LFRAME2 - G7291_PITCH_LAG_MAX - G7291_L_INTERPOL], st->pExcVector,
                   G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  st->sUpdateCounter = st->sUpdateCounter + 1;  /* counter of the age of the last update */
  if (st->sUpdateCounter > G7291_FEC_MAX_UPD_CNT)
  {
    st->sUpdateCounter = G7291_FEC_MAX_UPD_CNT;
  }
  return;
}
/* Reconstruction of erased frame
   - Find pitch
   - Estimate damping factor
   - Construct and synchronize periodic part of excitation
   - Construct random part of excitation
   - Apply gain to excitation
   - Do synthesis filtering to recover lost synthesis */
/******************************************************************************/
static void FEC_syn_bfi(
short last_good,      /* i:   last good frame clas                    */
short sBFICount,        /* i:   counter of consecutive bfi frames       */
const short sStabFactor, /* i:   ISF stability measure                   */
short *sDampedPitchGain,      /* i/o: damped pitch gain                       */
short *sDampedCodeGain,      /* i/o: damped code gain                        */
short *pExcMem,       /* o:   excitation memory (st->pExcVector)         */
short *exc2,          /* o:   excitation buffer (for synthesis)       */
const short T3,       /* i:   floating pitch                          */
const short sTiltCode,/* i:   spectral tilt                           */
const short Ts,       /* i:   previous pitch for hr G7291_FEC_VOICED */
short *pitch_buf,     /* o:   floating pitch for every subfr          */
const short Tcnt,     /* i:   counter of the last bfi pitch updt.     */
short *Aq,            /* i    LP filter coefs                         */
short *pSynth,        /* o    synthesized speech at Fs = 16kHz        */
short *mem_syn2,      /* i/o  initial synthesis filter states         */
int *iEnergy,        /* o    energy update at the end for next frame */
short *seed,          /* i/o: random generator seed                   */
short *exc,           /* i/o: pointer to present exc                  */
short puls_pos,       /* i  : Pulse position                          */
short new_pit)        /* i  : Pitch of the first future frame         */
{
  short i, indexSubfr;
  short nb_pusles, pulse_chosen, Terr;
  short P0 = 0, pit_search, diff_pit, sign;
  int   tmp32, L_tmp2, L_step;
  short hp_filt[5], tmp2, *p_Az, exp_alpha, exp_gain;
  short delta, fT0, pos, total_pit_lag = 0;
  short *pt_exc, *pt1_exc, alpha, step, gain_inov, gain, tmp16, tmp_Tc, Tc;
  short dist_Plast, Plast, Tlist[10];

    /* Find secure pitch estimation for FERs
       The new_pit is always known
       in single FER, new_pit is the futur pitch else new_pit is predict from past pitch */
  sign = 0;

  if(((Ts < ((29491 * T3)>>14)) && (Ts > ((19661 * T3)>>15))) ||
     (Tcnt >= G7291_FEC_MAX_UPD_CNT))
  {
    tmp_Tc = T3;                /*pitch period of the 4th subframe of the last good frame */
  }
  else
  {
    tmp_Tc = Ts;                /* pitch period of the 4th subframe of the last good STABLE VOICED */
  }

  if(new_pit > 0)
  {
    fT0 = tmp_Tc;
    delta = ((new_pit << G7291_FEC_SQPIT) - fT0) >> 2;
    for(i = 0; i < 4; i++)
    {
      fT0 = fT0 + delta;
      pitch_buf[i] = ((fT0 + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT) << G7291_FEC_SQPIT;
    }
  }
  else                          /* Should never happen in normal processing condition */
  {
    for(i = 0; i < 4; i++)
    {
      pitch_buf[i] = tmp_Tc;
    }
  }

   /* hp filter random part and add to excitation */
  tmp2 = 32767 - sTiltCode;
  for(i = 0; i < G7291_FEC_L_FIR_FER; i++)
  {
    hp_filt[i] = (tmp2 * G7291_FEC_h_highTbl[i] + 0x4000) >> 15;
  }

    /* Estimate damping factor */
  if(last_good == G7291_FEC_UNVOICED)
  {
    if(sBFICount > 3)
    {
      alpha = G7291_FEC_ALPHA_VT;
    }
    else if(sBFICount > 1)
    {
      alpha = Add_16s(((sStabFactor * (32767 - G7291_FEC_ALPHA_U) + 0x4000) >> 15), G7291_FEC_ALPHA_U);
    }
    else
    {
      alpha = G7291_FEC_ALPHA_U2;
    }
  }
  else if   (last_good == G7291_FEC_UV_TRANSITION)
  {
    alpha = G7291_FEC_ALPHA_UT;
  }
  else if   (last_good == G7291_FEC_ONSET)
  {
    alpha = 26214;              /* mild convergence to 0 for the first 3 erased frames */
  }
  else if   (((last_good == G7291_FEC_VOICED) || (last_good == G7291_FEC_ONSET)))
  {
    alpha = G7291_FEC_ALPHA_V; /* constant for the first 3 erased frames */
  }
  else if   (last_good == G7291_FEC_SIN_ONSET)
  {
    alpha = G7291_FEC_ALPHA_S;
  }
  else
  {
    if((sBFICount <= 2))
    {
      alpha = 2 * G7291_FEC_ALPHA_VT;  /* medium convergence to 0  */
    }
    else
    {
      alpha = (short) 6654;    /* very fast convergence to 0  */
    }
  }

  if(last_good >= G7291_FEC_VOICED)
  {
    if(sBFICount == 1)    /* if first erased frame in a block, reset harmonic gain */
    {
      if(*sDampedPitchGain == 0) *sDampedPitchGain = 1;
      tmp2 = *sDampedPitchGain;
      exp_alpha = Norm_16s_I(&tmp2);
      tmp2 = Div_16s(16384, tmp2);
      tmp32 = tmp2 << 16;
      InvSqrt_32s16s_I(&tmp32, &exp_alpha);

      gain = (short)(ShiftL_32s(tmp32, exp_alpha)>>16);
      if(gain > 32113)
      {
        gain = (short) 32113;
      }
      else if   (gain < 27853)
      {
        gain = (short) 27853;
      }
      alpha = (alpha * gain + 0x4000) >> 15;
    }
    else if(sBFICount <= 3)
    {
      alpha = Cnvrt_32s16s(*sDampedPitchGain << 1);
    }
    else
    {
      alpha = G7291_FEC_ALPHA_VT;
    }
  }
  if(last_good >= G7291_FEC_UV_TRANSITION)
  {
    Tc = (tmp_Tc + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
    /* Construct the periodic part of excitation */
    pt_exc = exc;
    pt1_exc = exc - Tc;
    pos = (short) 0;
    if(sBFICount == 1)    /* if 1st erased frame after a good frame */
    {
      for(i = 0; i < Tc; i++)  /* pitch cycle is first low-pass filtered */
      {
        tmp32 = 5898 * pt1_exc[-1];
        tmp32 += 20972 * pt1_exc[0];
        tmp32 += 5898 * pt1_exc[1];
        *pt_exc = (short)((tmp32 + 0x4000) >> 15);
        pt_exc++;
        pt1_exc++;
      }
      pos = Tc;
    }
    /* last pitch cycle of the previous frame is repeatedly copied up to an extra subframe */
    for (i=0; i<G7291_LFRAME2 + G7291_LSUBFR - pos; i++) pt_exc[i]=pt1_exc[i];
    if(new_pit > 0)
    {
      if(puls_pos == -1000) /* Information of pulse position is not present */
      {
        /* Only possible when more than 1 frame erasure or with bitrate < 14 kbps */
        i = 1;
        tmp2 = pitch_buf[0];
              /* Total delay of all pitch cycles in the concealed frame is computed
                 for both Tc used in concealment and future (or estimated) */
        while(tmp2 < (G7291_LFRAME2 - G7291_PITCH_LAG_MIN) * G7291_FEC_QPIT)
        {
          tmp16 = ((tmp2 >> G7291_FEC_SQPIT) * 819)>>15;
          tmp2 = pitch_buf[tmp16] + tmp2;
          i = i + 1;
        }
              /* Find estimated pitch lags per subframe
                 estimation of the difference between the last concealed
                 maximum glottal pulse in the frame and the estimated pulse */
        total_pit_lag = i * Tc - ((tmp2 + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT);

        /* have to play safe when we extrapolate a pulse position */
        if (total_pit_lag > 35)
        {
          total_pit_lag = (short) 0;
        }
        if (total_pit_lag < -35)
        {
          total_pit_lag = (short) 0;
        }
      }
      else
      {
              /* Decode phase information transmitted in the bitstream
                 (The position of the absolute maximum glottal pulse from
                 the end of the frame and its sign) */
        P0 = puls_pos;
        if (P0 < 0)
        {
          sign = (short) 1;
          P0 = Negate_16s(P0);
        }
        if(Tc >= 128)
        {
          P0 = P0 << 2;
          P0 = P0 + 2;
        }
        else if   (Tc >= 64)
        {
          P0 = P0 << 1;
          P0 = P0 + 1;
        }
        if (P0 > 160)
        {
          P0 = (short) 160;
        }
      }
      pit_search = Tc;

      if (sign)
      {
        pit_search = Negate_16s(pit_search);
      }

      /* Find the position of the first the maximum(minimum) lp_filtered pulse */
      Tlist[0] = FEC_findpulse(exc, pit_search, 0);
      tmp2 = Tc << 7;
      tmp2 = Div_16s(1280, tmp2);
      nb_pusles = (tmp2 >> 11) + 1;
      /* Extrapolate the position of all the other pulses */
      for(i = 1; i < nb_pusles; i++)
      {
        Tlist[i] = Tlist[i - 1] + Tc;
      }
      if(puls_pos == -1000)
      {
        if(G7291_LFRAME2 >= Tlist[nb_pusles - 1])  /* We try to find the right pulse pos */
        {
          P0 = G7291_LFRAME2 - Tlist[nb_pusles - 1] + total_pit_lag;
        }
        else
        {
          P0 = G7291_LFRAME2 - Tlist[nb_pusles - 2] + total_pit_lag;
        }
      }
      /* Choose on witch pulse to start the correction in function of pulse position sent by the encoder */
      pulse_chosen = (short) 0;
      Terr = (short) 200;
      Plast = G7291_LFRAME2 - P0;
      for(i = 0; i < nb_pusles; i++)
      {
        tmp16 = Abs_16s(Plast - Tlist[i]);
        if (tmp16 < Terr)
        {
          Terr = tmp16;
          pulse_chosen = i;
        }
      }
      diff_pit = Abs_16s(new_pit - Tc);
      tmp16 = Tc << 7;
      tmp16 = (Div_16s(1280, tmp16) + 1024) >> 11;
      dist_Plast = G7291_LFRAME2 - Tlist[pulse_chosen];

      if((Terr <= diff_pit * tmp16) && (Terr != 0)
         && (Terr < G7291_LSUBFR))
      {
        /* perform excitation resynchronization here */
        FEC_synchro_exc(exc, P0, dist_Plast, Tc);
      }
    }
    if(last_good == G7291_FEC_UV_TRANSITION)
    {
      gain = (short) 24576;
      *sDampedPitchGain = alpha >> 2;
    }
    else
    {
      gain = (short) 32767;
      *sDampedPitchGain = alpha >> 1;
    }
    step = (((gain >> 1) - *sDampedPitchGain) *  410 + 0x4000) >> 15;

    for(i = 0; i < G7291_LFRAME2; i++)
    {
      /* Linearly attenuate the gain through the frame */
      exc[i] = (exc[i] * gain + 0x4000) >> 15;
      gain = gain - step;
    }
    ippsCopy_16s(&exc[G7291_LFRAME2 - G7291_FEC_L_EXC_MEM], pExcMem, G7291_FEC_L_EXC_MEM);
  }
    /* Construct the random part of excitation */
  for(i = 0; i < G7291_LFRAME2 + G7291_FEC_L_FIR_FER - 1; i++)
  {
    exc2[i] = Rand_16s(seed) >> 3;
  }
  if ((last_good > G7291_FEC_V_TRANSITION) ||
      ((last_good > G7291_FEC_UV_TRANSITION) && sBFICount > 1))
  {
    *sDampedCodeGain = *sDampedCodeGain >> 1;
  }
  gain = *sDampedCodeGain;
  *sDampedCodeGain = (alpha * *sDampedCodeGain + 0x4000) >> 15;

  if (last_good == G7291_FEC_UV_TRANSITION)
  {
    *sDampedCodeGain = (short) 0;
  }

  pt_exc = exc2 + G7291_FEC_L_FIR_FER / 2;
  /***************************************************************
  tmp32 = (int) 0;
  for(i = 0; i < 4; i++)
  {
    ippsDotProd_16s32s_Sfs(pt_exc, pt_exc, G7291_LSUBFR, &L_tmp2, -1);
    pt_exc = pt_exc + G7291_LSUBFR;
    tmp32 += L_tmp2 >> 2; // Q-7
  }
  **************************************************************/
  /*** Can be replaced by (without the loss of accuracy) [AM] ***/
  ippsDotProd_16s32s_Sfs(pt_exc, pt_exc, G7291_LSUBFR*4, &L_tmp2, 0);
  tmp32 = L_tmp2 >> 1; // Q-7
  /***************************************************************/

  tmp32 = Mul_32s16s(tmp32, 26214);
  exp_gain = Norm_32s_I(&tmp32);
  exp_gain = 31 - exp_gain;
  InvSqrt_32s16s_I(&tmp32, &exp_gain);
  gain_inov = Cnvrt_NR_32s16s(tmp32);
  if (last_good == G7291_FEC_UNVOICED) /* Attenuate somewhat on unvoiced   */
  {
    gain_inov = (gain_inov * 13107 + 0x4000) >> 15;
  }

  pt_exc = exc2;                /* non-causal ringing of the FIR filter         */
  step = gain - *sDampedCodeGain;

  L_step = Mul_32s16s((gain_inov * step) << 1, 205);
  L_tmp2 = (gain_inov * gain) << 1;
  tmp2 = Cnvrt_NR_32s16s(L_tmp2);
  exp_gain = exp_gain + 16;

  for(i = 0; i < G7291_FEC_L_FIR_FER / 2; i++)
  {
    tmp32 = tmp2 * *pt_exc;
    *pt_exc++ = Cnvrt_NR_32s16s(ShiftL_32s(tmp32, exp_gain+1));
  }

  for(i = 0; i < G7291_LFRAME2; i++)  /* Actual filtered random part of excitation */
  {
    tmp32 = tmp2 * *pt_exc;
    *pt_exc++ = Cnvrt_NR_32s16s(ShiftL_32s(tmp32, exp_gain+1));
    L_tmp2 = L_tmp2 - L_step;
    tmp2 = Cnvrt_NR_32s16s(L_tmp2);
  }

  for(i = 0; i < G7291_FEC_L_FIR_FER / 2; i++)  /* causal ringing of the FIR filter   */
  {
    tmp32 = tmp2 * *pt_exc;
    *pt_exc++ = Cnvrt_NR_32s16s(ShiftL_32s(tmp32, exp_gain+1));
  }
    /* Construct the total excitation */
  if(last_good >= G7291_FEC_UV_TRANSITION)
  {
        /* innovation excitation is filtered through
           a linear phase FIR high-pass filter */
    pt_exc = exc2;
    for(i = 0; i < G7291_LFRAME2; i++)
    {
      tmp32 = hp_filt[0] * pt_exc[0];
      tmp32 += hp_filt[1] * pt_exc[1];
      tmp32 += hp_filt[2] * pt_exc[2];
      tmp32 += hp_filt[3] * pt_exc[3];
      tmp32 += hp_filt[4] * pt_exc[4];
      pt_exc++;
      exc[i] = Cnvrt_32s16s(exc[i] + ((tmp32 + 0x4000) >> 15));
    }
  }
  else
  {
    ippsCopy_16s(exc2 + G7291_FEC_L_FIR_FER / 2, exc, G7291_LFRAME2);
    ippsCopy_16s(&exc[G7291_LFRAME2 - G7291_FEC_L_EXC_MEM], pExcMem, G7291_FEC_L_EXC_MEM);
  }

  ippsCopy_16s(exc, exc2, G7291_LFRAME2);

    /* Speech synthesis */
  p_Az = Aq;
  for(indexSubfr = 0; indexSubfr < G7291_LFRAME2; indexSubfr += G7291_LSUBFR)
  {
    p_Az[0] >>= 1;
    ippsSynthesisFilter_NR_16s_Sfs(p_Az, &exc2[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, mem_syn2);
    for (i=0; i<G7291_LP_ORDER; i++) mem_syn2[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
    p_Az[0] <<= 1;
    p_Az += (G7291_LP_ORDER + 1);
  }
    /* estimate the pitch-synchronous speech energy per sample
       to be used when normal operation recovers */
  tmp16 = FEC_energy(last_good, pSynth, (tmp_Tc + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT, iEnergy, 1);
  if(last_good < G7291_FEC_VOICED)
  {
    *iEnergy = Mul_32s16s(*iEnergy, 26214);
    tmp16 = tmp16 + 7;
    *iEnergy = (*iEnergy + (1<<(tmp16-1))) >> tmp16;
  }
  return;
}

/* Smooth energy transition between lost frame and good frame
   - Find energy at frame end
   - Find energy at frame beginning
   - sSmooth excitation energy over all the frame
   - redo synthesis (if needed) */
void ownScaleSynthesis_FEC(
int extrate,          /* i: indicate decoded bit rate                 */
short clas,             /* i/o: frame classification                    */
const short last_good,  /* i:   last good frame classification          */
short *pSynth,          /* i/o: synthesized speech                      */
const short *pitch,     /* i:   pitch values for each subframe          */
int Lenr_q,             /* i:   transmitted energy for current frame    */
const int iPrevBFI,     /* i:   previous frame BFI                      */
short *Aq,              /* i/o: LP filter coefs (can be modified if BR) */
short *old_enr_LP,      /* i/o: LP filter E of last good G7291_FEC_VOICED frame   Q4 */
short *mem_tmp,         /* i/o: temporary synthesis memory              */
short *exc_tmp,         /* i/o: temporary excitation                    */
int iEnergy,           /* i  : energy at the end of the last frame     */
short *pExcVector,         /* i/o: excitation buffer                       */
short *pSynthMem,       /* i/o: synthesis memory                        */
short sBFICount)          /* i  : Number of bad frame                     */
{
  short i, scale_flag;
  short gain1, gain2, tmp16, exp_enr, exp2, tmp2;
  int   Lenr1, Lenr2, tmp32;
  short enr_LP;
  short indexSubfr, *p_Az;

  scale_flag = (short) 0;
  enr_LP = (short) 0;
  gain2 = (short) 0;
  gain1 = (short) 0;

  if (iEnergy == 0)
  {
    iEnergy = (short) 1;
  }
    /* Find the synthesis filter impulse response on G7291_FEC_VOICED */

  if(clas >= G7291_FEC_V_TRANSITION)
  {
    IPP_ALIGNED_ARRAY(16, short, h1, 2 * G7291_LSUBFR);
    IPP_ALIGNED_ARRAY(16, short, mem, G7291_LP_ORDER);
    int acc32;

    /* Find the impulse response */
    ippsZero_16s(h1, G7291_LSUBFR);
    ippsZero_16s(mem, G7291_LP_ORDER);
    h1[0] = 1024;                 /* h1_in Q10, h1_out Q10 */
    ippsSynthesisFilterLow_NR_16s_ISfs(&Aq[G7291_G729_MP1*3],h1,G7291_LSUBFR,12,mem);

    /* Find the impulse response energy */
    ippsDotProd_16s32s_Sfs(h1, h1, G7291_LSUBFR, &acc32, -1);
    enr_LP = Cnvrt_NR_32s16s(acc32);
  }
  if(iPrevBFI != G7291_FEC_GOOD_FRAME  /* previous frame erased */
     && (last_good < G7291_FEC_SIN_ONSET || extrate > 40))
  {
        /* Find the energy at the end of the frame */
    tmp16 = FEC_energy(clas, pSynth, (pitch[3] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT, &Lenr2, 1);
    if(clas < G7291_FEC_VOICED)
    {
      Lenr2 = Mul_32s16s(Lenr2, 26214);
      tmp16 = tmp16 + 7;
      Lenr2 = (Lenr2 + (1<<(tmp16-1))) >> tmp16;
    }

    if(extrate <= 40)   /* < 16 kbps  use the energy estimation from the decoder */
    {
      Lenr_q = Lenr2;
      if(last_good >= G7291_FEC_V_TRANSITION && clas >= G7291_FEC_V_TRANSITION) /* G7291_FEC_VOICED-G7291_FEC_VOICED recovery */
      {
        if(enr_LP > *old_enr_LP)
        {
          exp_enr = Norm_32s_I(&Lenr_q);
          tmp16 = (short)(Lenr_q>>16);
          tmp2 = enr_LP;
          exp2 = Norm_16s_I(&tmp2);
          exp_enr = exp2 - exp_enr;

          if(tmp16 > tmp2)
          {
            tmp16 = tmp16 >> 1;
            exp_enr = exp_enr + 1;
          }

          tmp16 = Div_16s(tmp16, tmp2);

          if ((exp_enr+1) > 0)
            Lenr_q = ShiftL_32s(tmp16 * *old_enr_LP, exp_enr+1);
          else
            Lenr_q = (tmp16 * *old_enr_LP) >> (-exp_enr-1);

          if (sBFICount <= 1)
          {
            Lenr_q = Mul4_32s(Lenr_q);
          }
        }

        if(enr_LP < *old_enr_LP)
        {
          if(Lenr_q > Mul4_32s(iEnergy)) /* Prevent energy to increase on G7291_FEC_VOICED */
          {
            Lenr_q = Mul4_32s(iEnergy);
          }
        }
        else if   (Lenr_q > iEnergy)
        {
          Lenr_q = iEnergy;
        }
      }
    }

    if (Lenr_q == 0)
    {
      Lenr_q = (int) 1;
    }
    exp_enr = Exp_32s(Lenr_q);
    tmp16 = (short)((Lenr_q << exp_enr)>>16);

    exp2 = Norm_32s_I(&Lenr2);
    tmp2 = (short)(Lenr2>>16);

    exp2 = exp_enr - exp2;  /* Denormalize and substract */

    if(tmp2 > tmp16)
    {
      tmp2 = tmp2 >> 1;
      exp2 = exp2 + 1;
    }

    tmp16 = Div_16s(tmp2, tmp16);
    tmp32 = tmp16 << 16;
    InvSqrt_32s16s_I(&tmp32, &exp2);
    if (exp2 > 0)
        gain2 = Cnvrt_NR_32s16s(ShiftL_32s(tmp32, exp2-1));
    else
        gain2 = Cnvrt_NR_32s16s(tmp32 >> (-exp2+1));

    /* Clipping of the smoothing gain at the frame end */
    if(Lenr_q <= 1) /* Do not allow E increase if enr_q index==0 (lower end Q clipping) */
    {
      if (gain2 > 16384)
      {
        gain2 = (short) 16384; /* Gain modification clipping */
      }
    }
    else
    {
      if (gain2 > 19661)
      {
        gain2 = (short) 19661; /* Gain modification clipping */
      }
    }
        /* Find the energy at the beginning of the frame to ensure sSmooth
           transition after erasure */

    if(clas == G7291_FEC_SIN_ONSET)  /* slow increase */
    {
      gain1 = (gain2 * 16384 + 0x4000) >> 15;
      scale_flag = (short) 1;
    }
    else if   ((last_good >= G7291_FEC_V_TRANSITION) && (clas == G7291_FEC_UNVOICED))
    {
      gain1 = gain2;
      scale_flag = (short) 0;
    }
    else
    {
      /* Find the energy at the end of the frame */
      tmp16 = FEC_energy(clas, pSynth, (pitch[0] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT, &Lenr1, 0);
      if(clas < G7291_FEC_VOICED)
      {
        Lenr1 = Mul_32s16s(Lenr1, 26214);
        tmp16 = tmp16 + 7;
        Lenr1 = (Lenr1 + (1<<(tmp16-1))) >> tmp16;
      }

      exp_enr = Norm_32s_I(&iEnergy);
      tmp16 = (short)(iEnergy>>16);
      exp2 = Norm_32s_I(&Lenr1);
      tmp2 = (short)(Lenr1>>16);

      exp2 = exp_enr - exp2;  /* Denormalize and substract */
      if(tmp2 > tmp16)
      {
        tmp2 = tmp2 >> 1;
        exp2 = exp2 + 1;
      }

      tmp16 = Div_16s(tmp2, tmp16);

      tmp32 = tmp16 << 16;
      InvSqrt_32s16s_I(&tmp32, &exp2);
      if (exp2 > 0)
          gain1 = Cnvrt_NR_32s16s(ShiftL_32s(tmp32, exp2-1));
      else
          gain1 = Cnvrt_NR_32s16s(tmp32 >> (-exp2+1));
      if (gain1 > 19661)
      {
        gain1 = (short) 19661; /* Gain modification clipping   */
      }
      if((clas == G7291_FEC_ONSET) && (gain1 > gain2)) /* Prevent a catastrophy in case of offset followed by G7291_FEC_ONSET */
      {
        gain1 = gain2;
      }
      scale_flag = (short) 1;
    }
  }

    /* Smooth the energy evolution by exponentially evolving from
       gain1 to gain2 */
  if(scale_flag != 0)
  {
    gain2 = (gain2 * (32768 - G7291_FEC_AGC) + 0x4000) >> 15;
    for(i = 0; i < G7291_LFRAME2; i++)
    {
      gain1 = ((gain2 << 15) + gain1 * G7291_FEC_AGC + 0x4000)>>15;
      exc_tmp[i] = Cnvrt_NR_32s16s(Mul4_32s(exc_tmp[i] * gain1));
    }

    /* Update excitation memory with new excitation */
    ippsCopy_16s(&exc_tmp[G7291_LFRAME2 - (G7291_PITCH_LAG_MAX + G7291_L_INTERPOL)], pExcVector,
                     G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  }
    /* Update the LP filter energy for G7291_FEC_VOICED frames */
  if (clas >= G7291_FEC_V_TRANSITION)
  {
    *old_enr_LP = enr_LP;
  }
    /* smoothing is done in excitation domain, so redo synthesis */
  if(iPrevBFI != G7291_FEC_GOOD_FRAME)
  {
    /* Speech synthesis at 8 kHz sampling rate */
    for (i=0; i<G7291_LP_ORDER; i++) pSynthMem[i]=mem_tmp[i];
    p_Az = Aq;
    for(indexSubfr = 0; indexSubfr < G7291_LFRAME2; indexSubfr += G7291_LSUBFR)
    {
      p_Az[0] >>= 1;
      ippsSynthesisFilter_NR_16s_Sfs(p_Az, &exc_tmp[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pSynthMem);
      for (i=0; i<G7291_LP_ORDER; i++) pSynthMem[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
      p_Az[0] <<= 1;
      p_Az += (G7291_LP_ORDER + 1);
    }
  }
  return;
}

/* To resynchronize concealed
   - Find number of minimum energy region
   - Find location of minimum energy region
   - Find number of sample to add/remove by minimum energy region
   - Synchronisation (sample deletion/addition) */
static void FEC_synchro_exc(
short *exc,            /* i/o : exc vector to modify */
short desire_puls_pos, /* Pulse position send by the encoder */
short true_puls_pos,   /* Present pulse location */
short Old_pitch)       /* Pitch use to create temporary adaptive codebook */
{
  short *pt_exc, *pt_exc1, tmp16, fact;
  int   tmp32, L_tmp1;
  short remaining_len;
  short i, j, point_to_remove, point_to_add, nb_min, points_by_pos[10], total_point;
  short *pt_pos, pos, start_search, tmp_len;
  short min_pos[10];
  IPP_ALIGNED_ARRAY(16, short, exc_tmp, G7291_LFRAME2 + G7291_LSUBFR);

  point_to_add = -1;
  for(i = 0; i < 10; i++)
  {
    min_pos[i] = (short) - 10000;
    points_by_pos[i] = (short) 0;
  }

  /* Find number of point to remove and number of minimum */
  point_to_remove = true_puls_pos - desire_puls_pos;

  /* Find number of minimum energy region */
  tmp16 = Old_pitch << 7;
  tmp_len = G7291_LFRAME2 - true_puls_pos;
  tmp16 = Div_16s(tmp_len << 3, tmp16);
  nb_min = tmp16 >> 11;

  /* if Old pitch < 80, must have at least 2 min */
  if (Old_pitch <= 80 && nb_min < 2)
  {
    nb_min = (short) 2;
  }
  /* Must have at least 1 min */
  if (nb_min == 0)
  {
    nb_min = (short) 1;
  }

  pos = tmp_len;
  pt_exc = exc + pos;

  /* Find starting point for minimum energy search */
  start_search = (-3 * Old_pitch + 2) >> 2;
  if((start_search + pos) < 0)
  {
    start_search = Negate_16s(pos);
    if (Abs_16s(start_search) < (Old_pitch >> 3))
    {
      return;
    }
  }

  /* Find min energy in the first pitch section
     The minimum energy regions are determined by the computing the energy
     using a sliding 5-sample window. The minimum energy position is set
     at the middle of the window at which the energy is at minimum */
  tmp32 = IPP_MAX_32S/2;
  L_tmp1 = pt_exc[start_search] * pt_exc[start_search];
  L_tmp1 += pt_exc[start_search + 1] * pt_exc[start_search + 1];
  L_tmp1 += pt_exc[start_search + 2] * pt_exc[start_search + 2];
  L_tmp1 += pt_exc[start_search + 3] * pt_exc[start_search + 3];
  L_tmp1 += pt_exc[start_search + 4] * pt_exc[start_search + 4];

  if(L_tmp1 < tmp32)
  {
    tmp32 = L_tmp1;
    min_pos[0] = pos + start_search + 2;
  }
  for(i = start_search; i < -5; i++)
  {
    L_tmp1 -= pt_exc[i] * pt_exc[i];
    L_tmp1 += pt_exc[i + 5] * pt_exc[i + 5];
    if(L_tmp1 < tmp32)
    {
      tmp32 = L_tmp1;
      min_pos[0] = pos + i + 2;
    }
  }

  for(j = 1; j < nb_min; j++)
  {
    min_pos[j] = min_pos[j - 1] - Old_pitch;
    /* If the first minimum is in the past, forget this minimum */
    if (min_pos[j] < 0)
    {
      min_pos[j] = -10000;
      nb_min = nb_min - 1;
    }
  }
   /* Determine the number of samples to be added or removed at each pitch
      cycle whereby less samples are added/removed at the beginning and
      more towards the end of the frame */
  if(nb_min == 1 || Abs_16s(point_to_remove) == 1)
  {
    nb_min = 1;
    points_by_pos[0] = Abs_16s(point_to_remove);
  }
  else
  {
    /* First position */
    fact = ((Abs_16s(point_to_remove) << 7) * invSqiTbl[nb_min - 2] + 0x4000) >> 15;
    points_by_pos[0] = (fact + 64) >> 7;
    total_point = points_by_pos[0];
    for(i = 2; i <= nb_min; i++)
    {
      points_by_pos[i - 1] = ((fact * sqiTbl[i - 2] + 64) >> 7) - total_point;
      total_point = total_point + points_by_pos[i - 1];
      /* ensure a constant increase */
      if(points_by_pos[i - 1] < points_by_pos[i - 2])
      {
        tmp16 = points_by_pos[i - 2];
        points_by_pos[i - 2] = points_by_pos[i - 1];
        points_by_pos[i - 1] = tmp16;
      }
    }
  }
   /* Sample deletion or insertion is performed in minimum energy regions.
      At the end of this section the last maximum pulse in the concealed
      excitation is forced to align to the actual maximum pulse position
      at the end of the frame which is transmitted in the future frame. */
  if (point_to_remove > 0)
  {
    point_to_add = point_to_remove;
  }

  pt_exc = exc_tmp;
  pt_exc1 = exc;
  pt_pos = min_pos + nb_min - 1;
  if(point_to_add > 0)          /* Samples insertion */
  {
    remaining_len = G7291_LFRAME2;
    for(i = 0; i < nb_min; i++)
    {
      if (i == 0)
      {
        /*Compute len to copy */
        tmp_len = *pt_pos;
      }
      if(i > 0)
      {
        /*Compute len to copy */
        tmp_len = *pt_pos - *(pt_pos + 1) - points_by_pos[i - 1];
      }
      /*Copy section */
      ippsCopy_16s(pt_exc1, pt_exc, tmp_len);
      remaining_len = remaining_len - tmp_len;
      pt_exc1 += tmp_len;
      pt_exc += tmp_len;

      /*Find point to add and Add points */
      tmp16 = (*pt_exc1 * (-1638) + 0x4000) >> 15;
      for(j = 0; j < points_by_pos[i]; j++)
      {
        *pt_exc++ = tmp16;      /* repeat last point  */
        tmp16 = Negate_16s(tmp16);
      }

      remaining_len = remaining_len - points_by_pos[i];
      pt_pos--;
    }
    /* Copy remaining data */
    ippsCopy_16s(pt_exc1, pt_exc, remaining_len);
    /* Update excitation vector */
    ippsCopy_16s(exc_tmp, exc, G7291_LFRAME2);
  }
  else                          /* Samples deletion */
  {
    remaining_len = G7291_LFRAME2;
    for(i = 0; i < nb_min; i++)
    {
      if (i == 0)
      {
        /*Compute len to copy */
        tmp_len = *pt_pos;
      }
      if(i > 0)
      {
        /*Compute len to copy */
        tmp_len = *pt_pos - *(pt_pos + 1) - points_by_pos[i - 1];
      }
      ippsCopy_16s(pt_exc1, pt_exc, tmp_len);
      remaining_len = remaining_len - tmp_len;
      pt_exc1 += tmp_len;
      pt_exc += tmp_len;
      /* Remove points */
      for(j = 0; j < points_by_pos[i]; j++)
      {
        pt_exc1++;
      }
      pt_pos--;
    }
    /* Copy remaining data */
    ippsCopy_16s(pt_exc1, pt_exc, remaining_len);
    /* Update excitation vector */
    ippsCopy_16s(exc_tmp, exc, G7291_LFRAME2);
  }
  return;
}

 /* Signal classification at the decoder for FER concealment
    Function used only at 8kbps. When bit rate >= 12kbps, the classification
    is sent by the encoder. */
void ownSignalClassification_FEC(
short *clas,           /* i/o: frame classification                      */
const short *pitch,    /* i:   pitch values for each subframe          Q5 */
const short last_good, /* i:   type of the last correctly received fr.   */
short *frame_synth,    /* i/o: synthesis buffer                          */
short *st_old_synth,   /* i/o: synthesis memory                          */
short *sLPSpeechEnergu)      /* i/o: long term active speech energy average Q1 */
{
  short i, j, pos;
  short *pt1, *pt2, zc_frame, frame_ener, tmp_scale;
  short tiltn, corn, zcn, pcn, fmerit1, enern, ener, tilt;
  short voicing, cor_max[3], *pSynth, tmp16, exp1, exp2;
  int   acc32, Ltmp1, s1, s2;
  short tmpS, zc[4], T0, pc, tmp_e, max, tmp_x, tmp_y;
  short Q_syn;
  IPP_ALIGNED_ARRAY(16, short, old_synth, G7291_PITCH_LAG_MAX + G7291_LFRAME2);

  cor_max[0] = (short) 0;
  cor_max[1] = (short) 0;
  cor_max[2] = (short) 0;
  Q_syn = (short) - 1;

  pSynth = old_synth + G7291_PITCH_LAG_MAX;  /* Pointer to the current frame     */

  /* Scaling pSynth to remove overflow results */
  ippsMaxAbs_16s(st_old_synth, G7291_PITCH_LAG_MAX, &max);
  ippsMaxAbs_16s(frame_synth, G7291_LFRAME2, &tmp_e);
  if (tmp_e > max)
  {
    max = tmp_e;
  }
  tmp_scale = Exp_16s(max) - 3;

  if (tmp_scale > 6)
  {
    tmp_scale = (short) 6;
  }
  if (tmp_scale > 0) {
    ippsLShiftC_16s(st_old_synth, tmp_scale, old_synth, G7291_PITCH_LAG_MAX);
    ippsLShiftC_16s(frame_synth, tmp_scale, &old_synth[G7291_PITCH_LAG_MAX], G7291_LFRAME2);
  }  else {
    ippsRShiftC_16s(st_old_synth, -tmp_scale, old_synth, G7291_PITCH_LAG_MAX);
    ippsRShiftC_16s(frame_synth, -tmp_scale, &old_synth[G7291_PITCH_LAG_MAX], G7291_LFRAME2);
  }
  Q_syn = Q_syn + tmp_scale;
  /* Compute the zero crossing rate for all subframes */
  pt1 = (short *) pSynth;
  pt2 = (short *) pSynth - 1;

  for(i = 0; i < 4; i++)
  {
    tmpS = 0;
    for(j = 0; j < G7291_LSUBFR; j++)
    {
      if((*pt1 <= 0) && (*pt2 > 0))
      {
        tmpS = tmpS + 1;
      }
      pt1++;
      pt2++;
    }
    zc[i] = tmpS;
  }

  zc_frame = zc[0] + zc[1] + zc[2] + zc[3];

  /* Compute the normalized correlation pitch-synch. at the end
       of the frame */
  T0 = pitch[3] >> G7291_FEC_SQPIT;

  if(T0 > 3 * (G7291_LSUBFR / 2))
  {
    T0 = (((pitch[2] + pitch[3]) >> 1) + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
  }

  pos = G7291_LFRAME2;
  j = (short) 0;
  pt1 = pSynth;

  while(pos > 3 * G7291_LSUBFR)
  {
    pos = pos - T0;
    Corre(&pt1[pos], &pt1[pos - T0], T0, &cor_max[j]);

    if((pos - T0) < 3 * G7291_LSUBFR)
    {
      T0 = (((pitch[2] + pitch[3]) >> 1) + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
    }
    j++;
  }
  acc32 = (int)cor_max[0];
  for(i = 1; i < j; i++)
  {
    acc32 = Add_32s(acc32, cor_max[i]);
  }

  if(j == 3)
  {
    voicing = (short)Mul_32s16s(acc32, 10923);
  }
  else if   (j == 2)
  {
    voicing = (short)(acc32 >> 1);
  }
  else
  {
    voicing = (short)acc32;
  }
  pc = Abs_16s(pitch[3] + pitch[2] - pitch[1] - pitch[0]) >> G7291_FEC_SQPIT;

  /* Compute spectral tilt */
  acc32 = (int) 0;
  pt1 = (short *) pSynth + G7291_LSUBFR;

  /****************************************************************
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &acc32, 1);
  ippsDotProd_16s32s_Sfs(pt2, pt2, G7291_LSUBFR, &Ltmp1, 1);
  pt1 += G7291_LSUBFR;
  pt2 += G7291_LSUBFR;
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &s1, 0);
  ippsDotProd_16s32s_Sfs(pt2, pt2, G7291_LSUBFR, &s2, 0);
  acc32 = Add_32s(acc32, s1);
  Ltmp1 = Add_32s(Ltmp1, s2);
  pt1 += G7291_LSUBFR;
  pt2 += G7291_LSUBFR;
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &s1, -1);
  ippsDotProd_16s32s_Sfs(pt2, pt2, G7291_LSUBFR, &s2, -1);
  acc32 = Add_32s(acc32, s1);
  Ltmp1 = Add_32s(Ltmp1, s2);
  ****************************************************************/
  /**** Can be replaced by (without the loss of accuracy) [AM] ***/
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &acc32, 0);
  Ltmp1 = acc32 - pt1[G7291_LSUBFR-2]*pt1[G7291_LSUBFR-2] + pt1[-1]*pt1[-1];
  acc32 = acc32 >> 1;
  Ltmp1 = Ltmp1 >> 1;
  pt1 += G7291_LSUBFR;
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &s1, 0);
  s2 = s1 - pt1[G7291_LSUBFR-2]*pt1[G7291_LSUBFR-2] + pt1[-1]*pt1[-1];
  acc32 = Add_32s(acc32, s1);
  Ltmp1 = Add_32s(Ltmp1, s2);
  pt1 += G7291_LSUBFR;
  ippsDotProd_16s32s_Sfs(pt1, pt1, G7291_LSUBFR, &s1, 0);
  s2 = s1 - pt1[G7291_LSUBFR-2]*pt1[G7291_LSUBFR-2] + pt1[-1]*pt1[-1];
  if(s1 > IPP_MAX_32S/2) s1 = IPP_MAX_32S;
  else s1 = s1 << 1;
  if(s2 > IPP_MAX_32S/2) s2 = IPP_MAX_32S;
  else s2 = s2 << 1;
  acc32 = Add_32s(acc32, s1);
  Ltmp1 = Add_32s(Ltmp1, s2);
  /****************************************************************/

  tmp16 = (short) - 1;

  if(Ltmp1 < 0)
  {
    tmp16 = Negate_16s(tmp16);
    Ltmp1 = Abs_32s(Ltmp1);
  }
  if (acc32 == 0)
  {
    acc32 = (int) 1;
  }
  exp1 = Norm_32s_I(&Ltmp1);
  tmp_y = (short)(Ltmp1>>16);
  exp1 = 33 - exp1;
  exp2 = Norm_32s_I(&acc32);
  tmp_x = (short)(acc32>>16);
  exp2 = 33 - exp2;

  if(tmp_y > tmp_x)
  {
    tmp_y = tmp_y >> 1;
    exp1 = exp1 + 1;
  }
  tilt = Div_16s(tmp_y, tmp_x);
  if (exp1 > exp2)
    tilt = Cnvrt_32s16s(tilt << (exp1 - exp2));
  else
    tilt = tilt >> (exp2 - exp1);

  if (tmp16 > 0)
  {
    tilt = Negate_16s(tilt);
  }
  /* Compute pitch-synchronous energy at the frame end */
  ener = FEC_frame_energy(pitch, pSynth, *sLPSpeechEnergu, &frame_ener, Q_syn);

    /* transform parameters between 0 & 1
       find unique merit function */
  enern = (short)((G7291_FEC_C_ENR + G7291_FEC_K_ENR * ener * 2 + 0x8000)>>16);
  tiltn = (short)((G7291_FEC_C_TILT + ((G7291_FEC_K_TILT * tilt)<<1))>>22);
  corn = (short)((G7291_FEC_C_COR + ((G7291_FEC_K_COR * voicing)<<1))>>21);
  zcn = (short)((G7291_FEC_C_ZC_DEC + ((G7291_FEC_K_ZC_DEC * zc_frame)<<1))>>8);
  pcn = (short)((G7291_FEC_C_PC_DEC + ((G7291_FEC_K_PC_DEC * pc)<<1))>>8);

  if(pcn > 256)
  {
    pcn = (short) 256;
  }
  else if (pcn < 0)
  {
    pcn = (short) 0;
  }

  acc32 = tiltn * G7291_FEC_UNS6;
  if(corn > 0)
  {
    acc32 += corn * 4 * G7291_FEC_UNS6;
  }
  else
  {
    acc32 += corn * 2 * G7291_FEC_UNS6;
  }
  acc32 += zcn * G7291_FEC_UNS6;
  acc32 += pcn * G7291_FEC_UNS6;
  acc32 += enern * G7291_FEC_UNS6;
  fmerit1 = Cnvrt_NR_32s16s(ShiftL_32s(acc32, 8));
  /* frame classification */
  switch(last_good)
  {
case G7291_FEC_VOICED:
case G7291_FEC_ONSET:
case G7291_FEC_SIN_ONSET:
case G7291_FEC_V_TRANSITION:
    if(fmerit1 < 12780)
    {
      *clas = G7291_FEC_UNVOICED;
    }
    else if   (fmerit1 < 20644)
    {
      *clas = G7291_FEC_V_TRANSITION;
    }
    else
    {
      *clas = G7291_FEC_VOICED;
    }
    break;
case G7291_FEC_UNVOICED:
case G7291_FEC_UV_TRANSITION:
    if(fmerit1 > 18350)
    {
      *clas = G7291_FEC_ONSET;
    }
    else if   (fmerit1 > 14746)
    {
      *clas = G7291_FEC_UV_TRANSITION;
    }
    else
    {
      *clas = G7291_FEC_UNVOICED;
    }
    break;

default:
    *clas = G7291_FEC_UNVOICED;
    break;
  }

    /* Measure average energy on good, active G7291_FEC_VOICED frames for FER
       concealment classification */
  if(*clas == G7291_FEC_VOICED)
  {
    acc32 = 656 * frame_ener;
    *sLPSpeechEnergu = (short)((acc32 + 32441 * *sLPSpeechEnergu * 2 +0x8000)>>16);
  }

  /* Update synthesized speech memory */
  if (tmp_scale > 0)
    ippsRShiftC_16s(&old_synth[G7291_LFRAME2], tmp_scale, st_old_synth, G7291_PITCH_LAG_MAX);
  else
    ippsLShiftC_16s(&old_synth[G7291_LFRAME2], -tmp_scale, st_old_synth, G7291_PITCH_LAG_MAX);
  return;
}

static short FEC_sig_clas(const short old_clas, const short voicing[6], const short * speech,
                          const short pit[3], const short * ee, const short * sync_sp, const short * pSynth,
                          short * sLPSpeechEnergu, short * rel_hist, short * rel_var);

/* Decode FER information
   - Find signal classification
   - Find signal energy
   - Find last glottal pulse position */
void ownEncode_FEC(
const short old_clas,     /* i:   signal classification result from previous frames  */
const short *voicing,     /* i:   normalized correlation for 3 half-frames           Q15 */
const short *speech,      /* i:   pointer to denoised speech signal                  Q_new */
const short *pSynth,      /* i:   pointer to synthesized speech for E computation    Q_new */
const short *T_op,        /* i:   close loop pitch values for 3 half-frames           */
const short *ee,          /* i:   lf/hf E ration for 2 half-frames                   Q14 */
const short *sync_sp,     /* i:   time syncronised input speech                      Q_new */
short *clas,              /* o:   signal clas for current frame                      */
short *coder_parameters,  /* o:   coder_parameters                                   */
const short *delay,           /* i:   close loop integer and fractional part of the pitch */
const short *res,         /* i:   LP residual signal frame                           Q_new */
short *sLPSpeechEnergu,         /* i/o: LP speech energu                                 Q8 */
short *rel_hist,          /* i/o: relative energy memory                            */
short *rel_var,           /* i/o: relative energy variation                         */
short *old_pos)           /* i/o: Position of the last pulse                        */
{
  short tmpS;
  short maxi, FER_pitch;
  short index[3];
  short sign, exp_enrq, enr_lg_ent, enr_lg_frac;
  int   enr_q, acc32;
  short *coder_param_ptr;

  coder_param_ptr = coder_parameters;
  sign = 0;
  enr_q = 0;
  FER_pitch = T_op[3];
    /*  Signal classification */
  *clas = FEC_sig_clas(old_clas, voicing, speech, T_op, ee, sync_sp, pSynth, sLPSpeechEnergu, rel_hist, rel_var);

    /*  Speech energy information
       - Maximum of energy per pitch period for G7291_FEC_VOICED frames
       - mean energy per sample over 2nd half of the frame for other
         frames */
  exp_enrq =
      FEC_energy(*clas, pSynth, Add_16s(delay[0], ((delay[1] << G7291_FEC_SQPIT) * 10922 +0x4000)>>20),
                 &enr_q, G7291_FEC_L_FRAME_FER);

  Log2(enr_q, &enr_lg_ent, &enr_lg_frac);
  enr_lg_ent = enr_lg_ent - exp_enrq;
  if(*clas < G7291_FEC_VOICED) /* Voiced current frame */
  {
    acc32 = Mpy_32_16(enr_lg_ent, enr_lg_frac, G7291_FEC_LG10_s1_55) - G7291_FEC_LG10_LFRAME_155;
  }
  else
  {
    acc32 = Mpy_32_16(enr_lg_ent, enr_lg_frac, G7291_FEC_LG10_s1_55);
  }

  tmpS = (short)(acc32>>16);
  if (tmpS > 31)
  {
    tmpS = 31;
  }
  if (tmpS < 0)
  {
    tmpS = (short) 0;
  }
  index[1] = tmpS;

    /*  Signal classification encoding */
  if (*clas == G7291_FEC_UNVOICED)
  {
    index[0] = (short) 0;
  }
  else if   ((*clas == G7291_FEC_V_TRANSITION) || (*clas == G7291_FEC_UV_TRANSITION))
  {
    index[0] = (short) 1;
  }
  else if   (*clas == G7291_FEC_VOICED)
  {
    index[0] = (short) 2;
  }
  else                          /* G7291_FEC_ONSET        */
  {
    index[0] = (short) 3;
  }

    /*  First pitch pulse position encoding
       - Find first pitch pulse of current frame
       - Encode first pitch pulse position */
  maxi = (short) 0;

  if(*clas > G7291_FEC_UNVOICED)
  {
    maxi = FEC_findpulse(res, FER_pitch, 1);
  }
  if (maxi > 160)
  {
    maxi = maxi - 256;
    sign = (short) 1;
  }
  if (FER_pitch >= 64)
  {
    maxi = maxi >> 1;
  }
  if (FER_pitch >= 128)
  {
    maxi = maxi >> 1;
  }
  if (maxi > 64)
  {
    maxi = (short) 63;         /* This should never happen... */
  }
  if (sign)
  {
    maxi = maxi + 64;
  }

  /* delay puls pos parameter */
  tmpS = maxi;
  maxi = *old_pos;
  *old_pos = tmpS;

  index[2] = maxi;

  *coder_param_ptr++ = index[2];
  *coder_param_ptr++ = index[0];
  *coder_param_ptr = index[1];
  return;
}

/* Do signal classification
 - Find relative signal energy
 - Find signal to noise ratio
 - Find average voicing
 - Compute the zero crossing rate
 - Compute pitch stability
 - Compute function of merit
 - Find signal classification */
static short FEC_sig_clas(  /* o:   classification for current frames             */
const short old_clas,  /* i:   signal classification result from previous frames    */
const short *voicing,  /* i:   normalized correlation for 3 half-frames             */
const short *speech,   /* i:   pointer to speech signal for E computation           */
const short *pit,      /* i:   open loop pitch values for 3 half-frames             */
const short *ee,       /* i:   lf/hf E ration for 2 half-frames                     */
const short *wsp,      /* i:   weighted speech                                      */
const short *wsyn,     /* i:   weighted synthesized signal                          */
short *sLPSpeechEnergu,      /* i/o: LP speech energu                                 Q8  */
short *last_relE,      /* i/o: relative energy memory                               */
short *sRelEnergyVar)       /* i/o: relative erergy variation                            */
{
  short i, pc, zc;
  short clas, *pt_windfx, exp_et, frac_et, relE, Etot, tmp_mean, fmerit_fac;
  int   acc32, Ltmp1, Letot;
  int   Lsegen, Lnoiseen;
  short tmp16, snr_val, exp_snr;
  short mean_voi2, een, corn, zcn, relEn, pcn, snrn, fmerit1;
  short exp_wsp, exp_noise, mant_noise, mant_wsp;
  short var1, var2;
  IPP_ALIGNED_ARRAY(16, short, wsyn_scl, G7291_FEC_L_FRAME_FER);
  IPP_ALIGNED_ARRAY(16, short, wsp_scl, G7291_FEC_L_FRAME_FER);

  /* Etot is the total energy over all the bandwith in dB */
  Letot = 1;
  Ltmp1 = (int) 0;
  pt_windfx = (short *) Sqrt_han_wind8kTbl; /*Q13 */
  for(i = 0; i < G7291_FEC_L_FRAME_FER / 4; i++)
  {
    tmp16 = (speech[i] * *pt_windfx++ + 0x4000)>>15;
    Ltmp1 += tmp16 * tmp16;
  }
  Letot += Ltmp1 >> 1;
  Ltmp1 = (int) 0;
  for(; i < G7291_FEC_L_FRAME_FER / 2; i++)
  {
    tmp16 = (speech[i] * *pt_windfx++ + 0x4000)>>15;
    Ltmp1 += tmp16 * tmp16;
  }
  Letot += Ltmp1 >> 1;
  Ltmp1 = (int) 0;
  for(; i < 3 * G7291_FEC_L_FRAME_FER / 4; i++)
  {
    tmp16 = (speech[i] * *pt_windfx-- + 0x4000)>>15;
    Ltmp1 += tmp16 * tmp16;
  }
  Letot += Ltmp1 >> 1;
  Ltmp1 = (int) 0;
  for(; i < G7291_FEC_L_FRAME_FER; i++)
  {
    tmp16 = (speech[i] * *pt_windfx-- + 0x4000)>>15;
    Ltmp1 += tmp16 * tmp16;
  }
  Ltmp1 <<= 1;

  Letot = Add_32s(Ltmp1 >> 2, Letot);
  Log2(Letot, &exp_et, &frac_et);
  exp_et = exp_et + 7;
  acc32 = Mpy_32_16(exp_et, frac_et, G7291_FEC_LG10) - G7291_FEC_LG10_LFRAME;
  Etot = Cnvrt_NR_32s16s(ShiftL_32s(acc32, 10));
  relE = Etot - *sLPSpeechEnergu;

  /* Signal classification preprocessing */
  /* Compute the SNR */
  ippsDotProd_16s32s_Sfs(wsp, wsp, G7291_FEC_L_FRAME_FER, &acc32, 0);
  if(acc32 > IPP_MAX_32S/2)
  {
    ippsRShiftC_16s(wsyn, 2, wsyn_scl, G7291_FEC_L_FRAME_FER);
    ippsRShiftC_16s(wsp, 2, wsp_scl, G7291_FEC_L_FRAME_FER);
  }
  else
  {
    ippsCopy_16s(wsyn, wsyn_scl, G7291_FEC_L_FRAME_FER);
    ippsCopy_16s(wsp, wsp_scl, G7291_FEC_L_FRAME_FER);
  }
  Lnoiseen = (int) 0;
  ippsDotProd_16s32s_Sfs(wsp_scl, wsp_scl, G7291_FEC_L_FRAME_FER, &Lsegen, -1);
  for(i = 0; i < G7291_FEC_L_FRAME_FER; i++)
  {
    tmp16 = wsp_scl[i] - wsyn_scl[i]; /* ajust scaling between signals */
    Lnoiseen += tmp16 * tmp16;
  }
  Lnoiseen <<= 1;
  if(Lnoiseen != 0 && Lsegen != 0)
  {
    /* Normalize segen */
    exp_wsp = Norm_32s_I(&Lsegen);
    mant_wsp = (short)(Lsegen>>16);
    exp_wsp = 30 - exp_wsp;

    /* Normalize noiseen */
    exp_noise = Norm_32s_I(&Lnoiseen);
    mant_noise = (short)(Lnoiseen>>16);
    exp_noise = 30 - exp_noise;

    /* ensure mant_wsp < mant_noise */
    mant_wsp = mant_wsp >> 1;
    exp_wsp = exp_wsp + 1;

    /*snr_val = segen / noiseen; *//* Total SNR    */
    snr_val = Div_16s(mant_wsp, mant_noise);
    exp_snr = exp_wsp - exp_noise;

    if ((-1 - exp_snr) > 0)
        acc32 = snr_val >> (-1 - exp_snr);
    else
        acc32 = snr_val << (1 + exp_snr);
    Log2(acc32, &exp_et, &frac_et);
    exp_et = exp_et - 16;
    acc32 = Mpy_32_16(exp_et, frac_et, G7291_FEC_LG10);
    snr_val = Cnvrt_NR_32s16s(ShiftL_32s(acc32, 16 - 5));
  }
  else
  {
    snr_val = (short) 0;
  }

  /* average voicing on second half-frame and look-ahead  */
  mean_voi2 = (voicing[0] + voicing[3] + 1)>>1;

  /* Average the spectral tilt in dB        */
  acc32 = (ee[0] * ee[1]) << 1;

  een = (short)(acc32>>18);
  een = (short)((G7291_FEC_C_EE/2 + een * G7291_FEC_K_EE + 0x4000)>>15);
  if (een < 0 || ee[0] < 0 || ee[1] < 0)
  {
    een = (short) 0;
  }
  if (een > 512)
  {
    een = (short) 512;
  }

  /* Compute the zero crossing rate    */
  zc = (short) 0;
  for(i = 1; i < G7291_FEC_L_FRAME_FER; i++)
  {
    if ((speech[i] <= 0) && (speech[i - 1] > 0))
    {
      zc = zc + 1;
    }
  }

  /* Compute the pitch stability      */
  pc = Add_16s(Abs_16s(pit[2] - pit[1]), Abs_16s(pit[3] - pit[2]));
  /* This section is needed ot improve signal classification in noisy condition */
  tmp_mean = *last_relE;
  *last_relE = relE;
  fmerit_fac = (short) 16384;

  if(Abs_16s(relE - tmp_mean) < Add_16s(Abs_16s(*sRelEnergyVar), 6 * 256) && old_clas <= G7291_FEC_UNVOICED)  /* absolute variation */
  {
    fmerit_fac = (short) 13107;
  }
  else if   ((relE - tmp_mean) > Add_16s(*sRelEnergyVar, 3 * 256) && old_clas <= G7291_FEC_V_TRANSITION)
  {
    fmerit_fac = (short) 18022;
  }
  else if   ((relE - tmp_mean) < Add_16s(*sRelEnergyVar, -5 * 256) && old_clas >= G7291_FEC_V_TRANSITION)
  {
    fmerit_fac = (short) 9830;
  }
  *sRelEnergyVar = (1638 * (relE - tmp_mean) + 31130 * *sRelEnergyVar + 0x4000)>>15;

    /* transform parameters between 0 & 1
       find unique merit function */
  corn = (short)((G7291_FEC_C_COR/2 + mean_voi2 * G7291_FEC_K_COR + 0x40000) >> 19);

  if (corn > 512)
  {
    corn = (short) 512;
  }
  if (corn < 0)
  {
    corn = (short) 0;
  }
  acc32 = G7291_FEC_C_ZC_ENC << 2;
  acc32 += zc * G7291_FEC_K_ZC_ENC;
  zcn = (short)((acc32 + 32) >> 6);

  if (zcn > 512)
  {
    zcn = (short) 512;
  }
  if (zcn < 0)
  {
    zcn = (short) 0;
  }
  acc32 = G7291_FEC_C_RELE << 8;
  acc32 += relE * G7291_FEC_K_RELE;
  relEn = (short)((acc32 + 0x2000) >> 14);  /*relE in Q8 but relEn in Q9 */
  if (relEn > 512)
  {
    relEn = (short) 512;
  }
  if (relEn < 256)
  {
    relEn = (short) 256;
  }
  acc32 = G7291_FEC_C_PC_ENC >> 1;
  acc32 += pc * G7291_FEC_K_PC_ENC;
  pcn = (short)((acc32 + 32) >> 6);

  if (pcn > 512)
  {
    pcn = (short) 512;
  }
  if (pcn < 0)
  {
    pcn = (short) 0;
  }

  acc32 = G7291_FEC_C_SNR * 1024;
  snrn = (short)((acc32 + snr_val * G7291_FEC_K_SNR * 2 + 0x8000)>>16);

  if (snrn > 512)
  {
    snrn = (short) 512;
  }
  if (snrn < 0)
  {
    snrn = (short) 0;
  }
  if(relEn <= 256)
  {
    var1 = (short) 9830;
    var2 = (short) 19661;
  }
  else if   (relEn > 384)
  {
    var1 = (short) 11703;
    var2 = (short) 23406;
  }
  else
  {
    var1 = (short) 9362;
    var2 = (short) 18725;
  }

  acc32 = een * var1;
  acc32 += corn * var2;
  acc32 += zcn * var1;
  acc32 += relEn * var1;
  acc32 += pcn * var1;
  acc32 += snrn * var2;
  fmerit1 = Cnvrt_NR_32s16s(ShiftL_32s(acc32, 6));
  /* Correction when noisy condition */
  fmerit1 = Cnvrt_32s16s((fmerit_fac * fmerit1 & 0xffff8000L)>>14);
  if(relE < -2560)
  {
    clas = G7291_FEC_UNVOICED;
  }
  else
  {
    switch(old_clas)
    {
case G7291_FEC_VOICED:
case G7291_FEC_ONSET:
case G7291_FEC_V_TRANSITION:

      if(fmerit1 < 18350)
      {
        clas = G7291_FEC_UNVOICED;
      }
      else if   (fmerit1 < 22282)
      {
        clas = G7291_FEC_V_TRANSITION;
      }
      else
      {
        clas = G7291_FEC_VOICED;
      }
      break;

case G7291_FEC_UNVOICED:
case G7291_FEC_UV_TRANSITION:

      if(fmerit1 > 20972)
      {
        clas = G7291_FEC_ONSET;
      }
      else if   (fmerit1 > 19005)
      {
        clas = G7291_FEC_UV_TRANSITION;
      }
      else
      {
        clas = G7291_FEC_UNVOICED;
      }
      break;

default:
      clas = G7291_FEC_UNVOICED;
      break;
    }
  }

  if(clas > G7291_FEC_V_TRANSITION)
  {
    *sLPSpeechEnergu = (32440 * *sLPSpeechEnergu + 328 * Etot + 0x4000) >> 15;
  }
  return clas;
}
/* ownUpdatePitch_FEC :  Update last reliable pitch                                */
void ownUpdatePitch_FEC(
short clas,       /* i:   frame classification              */
short *pitch_buf, /* i:   fractional pitch buffer    Q5    */
short last_good,  /* i:   Last good frame classification    */
short *sUpdateCounter,   /* i/o: update counter                    */
short *sFracPitch,/* i/o: old fractional pitch buffer Q5   */
short *pFracPitch)    /* i/o: fractional pitch            Q5   */
{
  short    tmp1, tmp_old;

  if ((clas == G7291_FEC_VOICED) && (last_good >= G7291_FEC_V_TRANSITION))
  {
    tmp1 = (22938 * pitch_buf[1])>>15;
    tmp_old = (22938 * *sFracPitch)>>15;
    if (((pitch_buf[3] >> 1) < tmp1) && (pitch_buf[3] > tmp1) && /* Check for pitch coherence */
        ((pitch_buf[1] >> 1) < tmp_old) && (pitch_buf[3] > tmp_old))
    {
      *sUpdateCounter = 0;
    }
  }

  if (clas == G7291_FEC_UNVOICED)
  {
    *sFracPitch = (pFracPitch[0] + pFracPitch[1]) >> 1;
  }
  else
  {
    *sFracPitch = pitch_buf[3];
  }
  *sUpdateCounter = *sUpdateCounter + 1;  /* counter of the age of the last update */

  if (*sUpdateCounter > G7291_FEC_MAX_UPD_CNT)
  {
    *sUpdateCounter = G7291_FEC_MAX_UPD_CNT;
  }
  return;
}
/* called for  an erased frame
   1 tap LTP filtering of 1 as input value for one g729 frame
   last G7291_MEM_LEN_TAM samples are saved  (no fractional pitch for taming -> no interpolation) */
void ownUpdateMemTam(G729_DecoderState *pDecStat, short CoefLtp, short Pitch)
{
  short    i, j;
  IPP_ALIGNED_ARRAY(16, short, tamBuf, G7291_LFRAME4);

  /*filtering */
  if(Pitch < G7291_LFRAME4)
  {
    j = 0;
    for(i = (G7291_MEM_LEN_TAM - Pitch); i < G7291_MEM_LEN_TAM; i++) /*memory from the end of pMemTam */
    {
      tamBuf[j] = (short)(((0x408000 + CoefLtp * pDecStat->pMemTam[i] * 2)>>16) << 1); /*CoefLtp * pDecStat->pMemTam[i] + 1;, 0x400000:1 @ Q22 */
      j++;
    }
    for(j = Pitch; j < G7291_LFRAME4; j++) /*memory from tamBuf */
    {
      tamBuf[j] = (short)(((0x408000 + CoefLtp * tamBuf[j - Pitch] * 2)>>16) << 1); /*CoefLtp * tamBuf[j - Pitch] + 1;, 0x400000:1 @ Q22 */
    }
  }
  else
  {
    i = G7291_MEM_LEN_TAM - Pitch;
    for(j = 0; j < G7291_LFRAME4; j++) /*memory from the end of pMemTam */
    {
      tamBuf[j] = (short)(((0x408000 + CoefLtp * pDecStat->pMemTam[i] * 2)>>16) << 1); /*CoefLtp * pDecStat->pMemTam[i] + 1;, 0x400000:1 @ Q22 */
      i++;
    }
  }
  /*memory update */
  ippsMove_16s(&pDecStat->pMemTam[G7291_LFRAME4], pDecStat->pMemTam, G7291_LEN_MEMTAM_UPDATE_UPDATE);
  ippsCopy_16s(tamBuf, &pDecStat->pMemTam[G7291_LEN_MEMTAM_UPDATE_UPDATE], G7291_LFRAME4);
  return;
}
/* Artificial reconstruction of the adaptive
            codebook memory after the lost of an ONSET
   - Decode location and sign of the last pulse
   - Reset adaptive codebook memory
   - Find LP filter impulse response energy
   - Scale pulse "prototype" energy
   - Place pulse "prototype" at the right position in the adaptive codebooke memory
   - Update adaptive codebook memory
   - Find signal classification */
void ownUpdateAdaptCdbk_FEC(
short *exc,       /* i/o : exc vector to modify                                               */
short puls_pos,   /* i   : Last pulse position desired                                        */
short *pitch_buf, /* i   : contains pitch modified in function of bfi_pitch and new pitch   Q6 */
int enr_q,        /* i   : energy provide by the encoder                                    Q0 */
short *Aq)        /* i   : Lsp coefficient                                                     */
{
  short T0, P0, onset_len, sign, i, len;
  short *pt_end, *pt_exc, enr_LP, gain, H_low_s[5], exp_gain, exp2, tmp16;
  int   tmp32;
  short Q_exc;
  IPP_ALIGNED_ARRAY(16, short, h1, G7291_LSUBFR);
  IPP_ALIGNED_ARRAY(16, short, mem, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, exc_tmp, G729_FEC_L_MEM);

  sign = 0;
  Q_exc = 0;
  T0 = pitch_buf[3] >> G7291_FEC_SQPIT;
  if(T0 < 2 * G7291_LSUBFR)
  {
    onset_len = 2 * G7291_LSUBFR;
  }
  else
  {
    onset_len = T0;
  }

  P0 = puls_pos;

  if(P0 < 0)
  {
    sign = (short) 1;
    P0 = Negate_16s(P0);
  }

  if(T0 >= 128)
  {
    P0 = P0 << 2;
    P0 = P0 + 2;
  }
  else if   (T0 >= 64)
  {
    P0 = P0 << 1;
    P0 = P0 + 1;
  }

  if (P0 > 160)
  {
    P0 = 160;
  }

  ippsZero_16s(exc_tmp, G729_FEC_L_MEM);  /* Reset excitation vector            */

  /* Find LP filter impulse response energy */
  ippsZero_16s(h1, G7291_LSUBFR);  /* Find the impulse response          */
  ippsZero_16s(mem, G7291_LP_ORDER);
  h1[0] = 1024;
  ippsSynthesisFilterLow_NR_16s_ISfs(Aq,h1,G7291_LSUBFR,12,mem);
  ippsDotProd_16s32s_Sfs(h1, h1, G7291_LSUBFR, &tmp32, -1);
  tmp32 += 1;
  exp_gain = 30 - Norm_32s_I(&tmp32);
  enr_LP = (short)(tmp32 >> 16);

  exp_gain = exp_gain - 20;

  /* divide by LP filter E, scaled by transmitted E   */
  if (enr_q == 0)
  {
    enr_q = 1L;
  }

  exp2 = Norm_32s_I(&enr_q);
  tmp16 = (short)(enr_q>>16);
  tmp16 = (tmp16 * 24576)>>15;

  if(tmp16 < 16384)
  {
    exp2 = exp2 + 1;
    tmp16 = tmp16 << 1;
  }
  exp2 = exp2 - 1;
  exp2 = 31 - exp2;


  if(enr_LP > tmp16)
  {
    enr_LP = enr_LP >> 1;
    exp_gain = exp_gain + 1;
  }

  tmp16 = Div_16s(enr_LP, tmp16);
  exp2 = exp_gain - exp2;

  tmp32 = tmp16 << 16;
  InvSqrt_32s16s_I(&tmp32, &exp2);
  gain = Cnvrt_NR_32s16s(tmp32);

  gain = (gain * 31457 + 0x4000)>>15;
  exp2 = exp2 - 15 + Q_exc;

  /* Find if rescaling needed */
  tmp16 = (short)((G7291_FEC_h_lowTbl[2] * gain)>>15);
  exp_gain = Exp_16s(tmp16);
  tmp16 = exp_gain - exp2;

  if(tmp16 < 0)                   /* Need to correct scaling */
  {
    exp2 = exp2 + tmp16;
  }
  /* Scale pulse "prototype" energy
     Generate the scaled pulse */
  for(i = 0; i < G7291_FEC_L_FIR_FER; i++)
  {
    tmp32 = gain * G7291_FEC_h_lowTbl[i];
    H_low_s[i] = Cnvrt_NR_32s16s(tmp32 << (exp2+1));
  }

  /* Place pulse "prototype" at the right position in the adaptive codebooke memory */
  pt_exc = exc_tmp + G7291_LFRAME2 - (1 + G7291_FEC_L_FIR_FER / 2 + P0);  /* beginning of the 1st pulse         */
  pt_end = exc_tmp + onset_len;
  len = (short)(pt_exc - pt_end);

  if (len > G7291_FEC_L_FIR_FER)
  {
    len = G7291_FEC_L_FIR_FER;
  }
  if(!sign)
  {
    ippsAdd_16s(H_low_s, pt_exc, pt_exc, len);
  }
  else
  {
    ippsSub_16s(pt_exc, H_low_s, pt_exc, len);
  }
  ippsCopy_16s(&exc_tmp[G7291_LFRAME2 - G7291_PITCH_LAG_MAX - G7291_L_INTERPOL], exc,
                   G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  return;
}
/* Decode FER information
   - Decode class information (if available)
   - Decode pulse position and sign information (if available)
   - Decode enegy information (if available) */
short ownDecodeInfo_FEC(/* o:   frame class information               */
short *last_good,   /* i:   last good received frame class        */
short sPrevLastGood,   /* i:  previous last good received frame class */
const int iPrevBFI, /* i:   BFI of the previous frame             */
short *indice,      /* i/o: Q indices (parameter vector)          */
int *enr_q,         /* o:   E information for FER protection      */
int Rate)           /* i:   Bitrate decoded                       */
{
  short clas, ind, tmp16, frac;
  int   tmp32;

  if(Rate >= 14000)     /* 14 kbps find pulse position information */
  {
    if(indice[2] >= 64) /* take sign */
    {
      indice[2] = Negate_16s((short) (indice[2] & 0x3F));
    }
  }

  ind = (short) indice[0];

  if(ind == 0)
  {
    clas = G7291_FEC_UNVOICED;
  }
  else if   (ind == 1)
  {
    if(*last_good >= G7291_FEC_V_TRANSITION)
    {
      clas = G7291_FEC_V_TRANSITION;
    }
    else
    {
      clas = G7291_FEC_UV_TRANSITION;
    }
  }
  else if   (ind == 2)
  {
    clas = G7291_FEC_VOICED;
  }
  else
  {
    clas = G7291_FEC_ONSET;
  }
    /* Decide whether to model ONSET by sinusoidal model (FER)
       or regular ACELP */
  if(iPrevBFI != G7291_FEC_GOOD_FRAME) /* first good after bfi                  */
  {
    if((clas >= G7291_FEC_VOICED) &&
       ((*last_good < G7291_FEC_UV_TRANSITION)))
    {
      clas = G7291_FEC_SIN_ONSET;
      *last_good = G7291_FEC_SIN_ONSET;
    }
    if((clas == G7291_FEC_VOICED) &&
       ((*last_good > G7291_FEC_VOICED))
        )
    {
      *last_good = G7291_FEC_VOICED;
    }
    else if   ((clas == G7291_FEC_VOICED) &&
               (sPrevLastGood >= G7291_FEC_VOICED))
    {
      *last_good = G7291_FEC_VOICED;
    }
    else if   ((clas == G7291_FEC_UNVOICED) &&
               (*last_good == G7291_FEC_VOICED))
    {
      *last_good = G7291_FEC_UV_TRANSITION;
    }
  }
  /* Decode the energy for FER concealment */
  if (indice[1] != -1000)  /* if information is present */
  {
    tmp16 = ((indice[1] << 9) * 25395 + 0x4000)>>15;
    tmp32 = tmp16 * 10885;
    tmp32 = tmp32 >> 6;
    L_Extract(tmp32, &tmp16, &frac);
    *enr_q = Pow2(tmp16, frac);
  }
  return clas;
}

