/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2012 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: G.729/A/B/D/E/EV speech codec: Miscellaneous.
//
*/

#include <ipps.h>
#include <stdio.h>
#include <stdlib.h>
#include "owng7291.h"
#include "ippdefs.h"

static void Qua_Sidgain(short *ener,  /* (i)   array of energies                   */
                 short *sh_ener,      /* (i)   corresponding scaling factors       */
                 int *enerq,          /* (o)   decoded energies in dB              */
                 int *idx             /* (o)   SID gain quantization index         */
    )
{
  int L_acc, L_x;
  short hi, lo;
  short exp, frac;
  short e_tmp;

  /* Quantize energy saved for frame erasure case                */
  L_acc = (int)(*ener);
  if (*sh_ener < 0)
    L_acc >>= (-*sh_ener);
  else
    L_acc <<= *sh_ener;
  L_Extract(L_acc, &hi, &lo);
  L_x = Mpy_32_16(hi, lo, 410);

  Log2(L_x, &exp, &frac);
  e_tmp = exp << 10;
  e_tmp = e_tmp + (short)(((frac * 1024 + 0x4000) & 0xffff8000) >> 15);
  if (e_tmp <= -2721)     /* -2721 -> -8dB */
  {
    *enerq = -12;
    *idx = 0;
  }
  if (e_tmp > 22111)     /* 22111 -> 65 dB */
  {
    *enerq = 66;
    *idx = 31;
  }
  if (e_tmp <= 4762)      /* 4762 -> 14 dB */
  {
    e_tmp = e_tmp + 3401;
    *idx = (e_tmp * 24 & 0xffff8000) >> 15;
    if (*idx < 1) *idx = 1;
    *enerq = (*idx << 2) - 8;
  }
  e_tmp = e_tmp - 340;
  *idx = ((e_tmp * 193 & 0xffff8000) >> 17) - 1;
  if (*idx < 6)
  {
    *idx = 6;
  }
  *enerq = (*idx << 1) + 4;
  return;
}

/* Square root function : returns sqrt(Num/2) */
/**********************************************/
static short Sqrt(int Num)
{
  int i, acc32;
  short Rez = 0;
  short Exp = 0x4000;

  for (i = 0; i < 14; i++)
  {
    acc32 = (Rez + Exp) * (Rez + Exp) * 2;
    if (Num >= acc32) Rez = Rez + Exp;
    Exp = Exp >> (short)1;
  }
  return Rez;
}

/*   Computes comfort noise excitation for SID and not-transmitted frames */
static void Calc_exc_rand(G729_DecoderState *pDecStat,
                          short cur_gain, /* (i)   :   target sample gain                 */
                          short *exc,     /* (i/o) :   excitation array                   */
                          short *seed     /* (i)   :   current Vad decision               */
    )
{
  short i, j, i_subfr;
  short temp1, temp2, tmp16;
  short pos[4];
  short sign[4];
  short delayVal[2];
  short *cur_exc;
  short g, Gp, Gp2;
  short excg[G7291_LSUBFR], excs[G7291_LSUBFR];
  int   L_acc, L_ener, L_k;
  short max, hi, lo, inter_exc;
  short sh;
  short x1, x2;

  if (cur_gain == 0)
  {
    ippsZero_16s(exc, G7291_LFRAME4);
    return;
  }

  /* Loop on subframes */
  cur_exc = exc;

  for (i_subfr = 0; i_subfr < G7291_LFRAME4; i_subfr += G7291_LSUBFR)
  {
    /* generate random adaptive codebook & fixed codebook parameters */
    /*****************************************************************/
    temp1 = Rand_16s(seed);
    delayVal[1] = (temp1 & (short)0x0003) - 1;
    if (delayVal[1] == 2)
    {
      delayVal[1] = 0;
    }
    temp1 = (temp1 >> 2);
    delayVal[0] = (temp1 & (short) 0x003F) + 40;
    temp1 = (temp1 >> 6);
    temp2 = (temp1 & (short) 0x0007);
    pos[0] = temp2 * 5;

    temp1 = (temp1 >> 3);
    sign[0] = (temp1 & (short) 0x0001);

    temp1 = (temp1 >> 1);
    temp2 = (temp1 & (short) 0x0007);
    pos[1] = temp2 * 5 + 1;

    temp1 = (temp1 >> 3);
    sign[1] = (temp1 & (short) 0x0001);

    temp1 = Rand_16s(seed);
    temp2 = (temp1 & (short) 0x0007);
    pos[2] = temp2 * 5 + 2;

    temp1 = (temp1 >> 3);
    sign[2] = (temp1 & (short) 0x0001);

    temp1 = (temp1 >> 1);
    temp2 = (temp1 & (short) 0x000F);
    pos[3] = (temp2 & (short) 1) + 3;

    temp2 = ((temp2 >> 1) & (short) 7);
    pos[3] = (pos[3] + temp2 * 5);

    temp1 = (temp1 >> 4);
    sign[3] = (temp1 & (short) 0x0001);

    Gp = (Rand_16s(seed) & (short) 0x1FFF);
    Gp2 = (Gp << 1);


    /* Generate gaussian excitation */
    ippsRandomNoiseExcitation_G729B_16s(seed, excg, G7291_LSUBFR);
    ippsDotProd_16s32s_Sfs(excg, excg, G7291_LSUBFR, &L_acc, 0);
    ippsInvSqrt_32s_I(&L_acc, 1);
    L_Extract(L_acc, &hi, &lo);

    temp1 = (short)(((cur_gain * FRAC1 +0x4000) & 0xffff8000) >> 15);
    temp1 = cur_gain + temp1;

    L_acc = Mpy_32_16(hi, lo, temp1);
    sh = Norm_32s_I(&L_acc) - 14;
    temp1 = (short)(L_acc >> 16);

    ippsMulC_NR_16s_Sfs(excg,temp1,excg,G7291_LSUBFR,15);
    if(sh<0) {
        ippsLShiftC_16s(excg,-sh,excg,G7291_LSUBFR);
    } else if(sh>0) {
        Ipp16s const1=(Ipp16s)(1<<(sh-1));
        ippsAddC_16s_Sfs(excg,const1,excg,G7291_LSUBFR,0);
        ippsRShiftC_16s(excg,sh,excg,G7291_LSUBFR);
    }

    /* generate random  adaptive excitation */
    /* ippsAdaptiveCodebookDecode_G7291_16s_I(delayVal, cur_exc); */
    /* Can be replaced by (without the loss of accuracy) [AM] */
    ippsDecodeAdaptiveVector_G729_16s_I(delayVal, &cur_exc[-G7291_FEC_L_EXC_MEM]);

    ippsRShiftC_16s(cur_exc, 1, cur_exc, G7291_LSUBFR);

    /* compute adaptive + gaussian exc -> cur_exc */
    ippsMulC_NR_16s_ISfs(Gp2,cur_exc,G7291_LSUBFR,15);
    ippsAdd_16s_Sfs(excg,cur_exc,cur_exc,G7291_LSUBFR,0);
    ippsMax_16s(cur_exc,G7291_LSUBFR,&max);
    ippsMin_16s(cur_exc,G7291_LSUBFR,&temp1);
    if(temp1==IPP_MIN_16S)
        temp1++;
    if(temp1<0)
        temp1 = (Ipp16s)(-temp1);
    max = (Ipp16s)(IPP_MAX(temp1,max));
    if (max == 0) sh = 0;
    else
    {
      sh = 3 - Exp_16s_Pos(max);
      if (sh < 0) sh = 0;
    }
    ippsRShiftC_16s(cur_exc, sh, excs, G7291_LSUBFR);

    /* Compute fixed code gain */
    ippsDotProd_16s32s_Sfs(excs,excs,G7291_LSUBFR,&L_ener,-1);

    inter_exc = 0;
    for (i = 0; i < 4; i++)
    {
      j = pos[i];
      if (sign[i] == 0)
      {
        inter_exc = inter_exc - excs[j];
      }
      else
      {
        inter_exc = inter_exc + excs[j];
      }
    }

    /* Compute k = cur_gainR x cur_gainR x L_SUBFR */
    L_acc = cur_gain * G7291_LSUBFR;
    L_acc >>= 5;
    L_k = cur_gain * L_acc;
    L_acc = L_k >> (sh << 1);

    /* Compute delta = b^2 - 4 c */
    L_acc -= L_ener;
    inter_exc = inter_exc >> 1;
    L_acc += (inter_exc * inter_exc) << 1;
    sh = sh + 1;

    if (L_acc < 0)
    {
      /* adaptive excitation = 0 */
      ippsCopy_16s(excg, cur_exc, G7291_LSUBFR);
      temp1 = (Abs_16s(excg[(int) pos[0]]) | Abs_16s(excg[(int) pos[1]]));
      temp2 = (Abs_16s(excg[(int) pos[2]]) | Abs_16s(excg[(int) pos[3]]));
      temp1 = (temp1 | temp2);
      if ((temp1 & (short) 0x4000) == 0) sh = (short) 1;
      else sh = (short) 2;
      inter_exc = 0;
      for (i = 0; i < 4; i++)
      {
        temp1 = excg[pos[i]] >> sh;
        if (sign[i] == 0)
        {
          inter_exc = inter_exc - temp1;
        }
        else
        {
          inter_exc = inter_exc + temp1;
        }
      }
      L_Extract(L_k, &hi, &lo);
      L_acc = Mpy_32_16(hi, lo, K0);
      L_acc >>= (sh << 1) - 1;
      L_acc += inter_exc * inter_exc * 2;
      Gp = 0;
    }

    temp2 = Sqrt(L_acc);
    x1 = temp2 - inter_exc;
    x2 = (short)(-(inter_exc + temp2));
    if (Abs_16s(x2) < Abs_16s(x1))
    {
      x1 = x2;
    }
    temp1 = 2 - sh;
    if(temp1>=0)
        g = ShiftR_NR_16s(x1, temp1);
    else
        g = ShiftL_16s((Ipp16s)x1, (Ipp16u)(-temp1));

    if (g >= 0)
    {
      if (g > G_MAX) g = G_MAX;
    }
    else
    {
      if (g < (-G_MAX)) g = (-G_MAX);
    }

    /* Update cur_exc with ACELP excitation */
    for (i = 0; i < 4; i++)
    {
      j = pos[i];
      if (sign[i] != 0)
      {
        cur_exc[j] = cur_exc[j] + g;
      }
      else
      {
        cur_exc[j] = cur_exc[j] - g;
      }
    }

    ippsAdd_16s(cur_exc, cur_exc, cur_exc, G7291_LSUBFR);

    tmp16 = (pDecStat->sSubFrameCount+1)*3277;
    pDecStat->sDampedPitchGain = pDecStat->sDampedPitchGain + ((tmp16 * Gp + 0x4000) >> 15);
    pDecStat->sDampedCodeGain = pDecStat->sDampedCodeGain + ((tmp16 * Cnvrt_32s16s(g<<3) + 0x4000) >> 15);
    pDecStat->sTiltCode = 819;
    pDecStat->sSubFrameCount++;

    cur_exc += G7291_LSUBFR;
  } /* end of loop on subframes */

  return;
}

void CNG_Decoder_G729B(G729_DecoderState *pDecStat, short *parm, short *exc, short *A_t, short ftyp)
{
  int i, ind, temp;

  if (ftyp != 0) /* SID Frame */
  {
    IPP_ALIGNED_ARRAY(32, Ipp16s, lsfq, G7291_M_LPC);
    pDecStat->sid_gain = tab_Sidgain[parm[3]];
    ippsLSFDecode_G729B_16s(parm, (Ipp16s*)pDecStat->pPrevLSFVector, lsfq);
    ippsLSFToLSP_G729_16s(lsfq, pDecStat->lspSid);
  }
  else /* non SID Frame */
  {
    if (pDecStat->past_ftyp == 1)
    {
      Qua_Sidgain(&pDecStat->sid_sav, &pDecStat->sh_sid_sav, &temp, &ind);
      pDecStat->sid_gain = tab_Sidgain[ind];
    }
  }

  if(pDecStat->past_ftyp == 1)
  {
    pDecStat->cur_gain = pDecStat->sid_gain;
  }
  else
  {
    pDecStat->cur_gain = (short)(((pDecStat->cur_gain * A_GAIN0 + 0x4000) & 0xffff8000) >> 15);
    pDecStat->cur_gain = (short)(pDecStat->cur_gain + (((pDecStat->sid_gain * A_GAIN1 + 0x4000) & 0xffff8000) >> 15));
  }

  Calc_exc_rand(pDecStat, pDecStat->cur_gain, exc, &pDecStat->seed);

  /* Interpolate the Lsp vectors */
  ippsInterpolate_G729_16s(pDecStat->lspSid, pDecStat->pLSPVector, pDecStat->pLSPVector, G7291_M_LPC);
  ippsLSPToLPC_G729_16s(pDecStat->pLSPVector, A_t);
  ippsLSPToLPC_G729_16s(pDecStat->lspSid, &A_t[G7291_G729_MP1]);
  for (i=0; i<G7291_M_LPC; i++) pDecStat->pLSPVector[i] = pDecStat->lspSid[i];

  return;
}
