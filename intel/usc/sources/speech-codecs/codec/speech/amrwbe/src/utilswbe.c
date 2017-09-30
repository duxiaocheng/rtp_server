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
// Purpose: AMRWBE speech codec: common utilities.
//
*/

#include "ownamrwbe.h"

#define MASK      0x0001


Ipp16s Bin2int(Ipp32s no_of_bits, Ipp16s *bitstream)
{
   Ipp16s value=0;
   Ipp32s i;

   for (i=0; i<no_of_bits; i++) {
      value  = ShiftL_16s(value, 1);
      value  = Add_16s(value, (Ipp16s)(*bitstream & MASK));
      bitstream ++;
   }
   return(value);
}

void Int2bin(
  Ipp16s value,
  Ipp32s no_of_bits,
  Ipp16s *bitstream
)
{
  Ipp16s *pt_bitstream;
  Ipp32s i;

  pt_bitstream = bitstream + no_of_bits;

  for (i = 0; i < no_of_bits; i++)
  {
    --pt_bitstream;
    *pt_bitstream = (Ipp16s)(value & MASK);
    value = (Ipp16s)(value>>1);
  }

}

void ownScaleHPFilterState(Ipp16s* pMemHPFilter, Ipp32s scale, Ipp32s order) {

   Ipp32s tmp32;
   Ipp16s tmp16;

   if (order == 2) {
      Ipp16s y2H,y2L,y1H,y1L,x0,x1;
      y2H=pMemHPFilter[0]; y2L=pMemHPFilter[1];
      y1H=pMemHPFilter[2]; y1L=pMemHPFilter[3];
      x0=pMemHPFilter[4];
      x1=pMemHPFilter[5];
      if (scale > 0) {
         tmp32 = x0 << 16;
         tmp32 = ShiftL_32s(tmp32, (Ipp16u)(scale));
         Unpack_32s(tmp32, &x0, &tmp16);

         tmp32 = (y1H << 16) + (y1L << 1);
         tmp32 = ShiftL_32s(tmp32, (Ipp16u)(scale));
         Unpack_32s(tmp32, &y1H, &y1L);

         y1L = (Ipp16s)(y1L - (tmp16 >> 1));

         tmp32 = x1 << 16;
         tmp32 = ShiftL_32s(tmp32, (Ipp16u)(scale));
         Unpack_32s(tmp32, &x1, &tmp16);

         tmp32 = (y2H << 16) + (y2L << 1);
         tmp32 = ShiftL_32s(tmp32, (Ipp16u)(scale));
         Unpack_32s(tmp32, &y2H, &y2L);

         y2L = (Ipp16s)(y2L - (tmp16 >> 1));
      } else {
         scale = -scale;
         tmp32 = x0 << 16;
         tmp32 >>=  scale;
         Unpack_32s(tmp32, &x0, &tmp16);

         tmp32 = (y1H << 16) + (y1L << 1);
         tmp32 >>= scale;
         Unpack_32s(tmp32, &y1H, &y1L);

         y1L = (Ipp16s)(y1L - (tmp16 >> 1));

         tmp32 = x1 << 16;
         tmp32 >>= scale;
         Unpack_32s(tmp32, &x1, &tmp16);

         tmp32 = (y2H << 16) + (y2L << 1);
         tmp32 >>= scale;
         Unpack_32s(tmp32, &y2H, &y2L);

         y2L = (Ipp16s)(y2L - (tmp16 >> 1));
      }
      pMemHPFilter[0]=y2H; pMemHPFilter[1]=y2L;
      pMemHPFilter[2]=y1H; pMemHPFilter[3]=y1L;
      pMemHPFilter[4]=x0;
      pMemHPFilter[5]=x1;
   }
}

void ownIntLPC(
   const Ipp16s*  pIspOld,
   const Ipp16s*  pIspNew,
   const Ipp16s*  pIntWnd,
   Ipp16s*        pAz,
   Ipp32s         nSubfr,
   Ipp32s         m
) {
   IPP_ALIGNED_ARRAY(16, Ipp16s, pIspTmp, M);
   Ipp32s k;
   Ipp16s fac_old, fac_new;

   for(k=0; k<nSubfr; k++) {
      fac_new = pIntWnd[k];  /* modify tables and remove "Add_16s" */
      fac_old = Add_16s((Ipp16s)(32767-fac_new) ,1);
      ippsInterpolateC_NR_G729_16s_Sfs(pIspOld, fac_old, pIspNew, fac_new, pIspTmp, m, 15);
      ippsISPToLPC_AMRWB_16s(pIspTmp, pAz, m);
      pAz += (m+1);
  }

   ippsISPToLPC_AMRWB_16s(pIspNew, pAz, m);
   return;
}

void ownFindWSP(const Ipp16s* Az, const Ipp16s* speech_ns,
                Ipp16s* wsp, Ipp16s* mem_wsp, Ipp32s lg)
{
   IPP_ALIGNED_ARRAY(16, Ipp16s, Azp, (M+1));
   const Ipp16s* p_Az;
   Ipp32s i_subfr;

   p_Az = Az;
   for (i_subfr=0; i_subfr<lg; i_subfr+=L_SUBFR) {
      ippsMulPowerC_NR_16s_Sfs(p_Az, GAMMA1_FX, Azp, (M+1), 15);
      ippsResidualFilter_Low_16s_Sfs(Azp, M, &speech_ns[i_subfr], &wsp[i_subfr], L_SUBFR, 12);
      p_Az += (M+1);
   }
   ippsDeemphasize_AMRWBE_NR_16s_I(TILT_FAC_FX, 15, wsp, lg, mem_wsp);

   return;
}


Ipp16s Match_gain_6k4(Ipp16s *AqLF, Ipp16s *AqHF)
{
  Ipp32s i, Ltmp;
  Ipp16s buf[M+L_SUBFR], tmp16, max16s, scale;
  Ipp16s code[L_SUBFR], exp_g, frac;
  Ipp16s gain16;

   ippsZero_16s(buf, M);
   tmp16 = 128;
   for (i=0; i<L_SUBFR; i++) {
      buf[i+M] = tmp16;
      tmp16 = MulHR_16s(tmp16, -29490);
   }
   /*^^^^ make table ??!! ^^^^*/

   ippsResidualFilter_Low_16s_Sfs(AqLF, M, buf+M, code, L_SUBFR, 11);

   tmp16 = AqHF[0];
   AqHF[0] >>= 1;
   ippsSynthesisFilter_G729E_16s_I(AqHF, MHF, code, L_SUBFR, buf);
   AqHF[0] = tmp16;

   ippsMaxAbs_16s(code, L_SUBFR, &max16s);

   scale = (Ipp16s)(Exp_16s(max16s) - 3);
   ownScaleSignal_AMRWB_16s_ISfs(code, L_SUBFR, scale);

   ippsDotProd_16s32s_Sfs(code, code, L_SUBFR, &Ltmp, -1);
   Ltmp = Add_32s(Ltmp, 1);
   exp_g = (Ipp16s)(30 - Norm_32s_I(&Ltmp));

   _Log2_norm(Ltmp,exp_g, &tmp16, &frac);
   exp_g = Sub_16s(29, Add_16s(tmp16, ShiftL_16s((Ipp16s)(scale+7),1)));

   Ltmp    = Mpy_32_16(exp_g, frac, LG10);
   gain16 = (Ipp16s)(Ltmp>>(14-8));

   return Negate_16s(gain16);
}

static __ALIGN(16) CONST Ipp16s tabLog_16s[33] = {
   0x0000, 0x05AF, 0x0B32, 0x108C, 0x15C0, 0x1ACF, 0x1FBC, 0x2488,
   0x2935, 0x2DC4, 0x3237, 0x368F, 0x3ACE, 0x3EF5, 0x4304, 0x46FC,
   0x4ADF, 0x4EAE, 0x5269, 0x5611, 0x59A7, 0x5D2C, 0x609F, 0x6403,
   0x6757, 0x6A9B, 0x6DD1, 0x70FA, 0x7414, 0x7721, 0x7A22, 0x7D17,
   0x7FFF};

void ownLog2_AMRWBE(Ipp32s L_x, Ipp16s *logExponent, Ipp16s *logFraction)
{
  Ipp16s exp, i, a, tmp;
  Ipp32s L_y;

  if( L_x <= 0 ){
    *logExponent = 0;
    *logFraction = 0;
    return;
  }

  exp = Norm_32s_I(&L_x);

  *logExponent = (Ipp16s)(30 - exp);

  i = (Ipp16s)((L_x >> 25) - 32);
  a = (Ipp16s)((L_x >> 10) & 0x7fff);
  L_y = tabLog_16s[i] << 15;
  tmp = (Ipp16s)(tabLog_16s[i] - tabLog_16s[i+1]);
  L_y -= tmp * a;
  *logFraction =  (Ipp16s)(L_y >> 15);

  return;
}

void _Log2_norm (Ipp32s L_x, Ipp16s exp, Ipp16s *exponent, Ipp16s *fraction)
{
    Ipp16s i, a, tmp;
    Ipp32s L_y;

    if (L_x <= (Ipp32s) 0)
    {
        *exponent = 0;
        *fraction = 0;
        return;
    }

    *exponent = Sub_16s (30, exp);

    L_x = L_x >> 9;
    i = ExtractHigh(L_x);
    L_x = L_x >> 1;
    a = (Ipp16s)(L_x);
    a = (Ipp16s)(a & (Ipp16s)0x7fff);

    i = Sub_16s (i, 32);

    L_y = (Ipp32s)tabLog_16s[i] << 16;
    tmp = Sub_16s (tabLog_16s[i], tabLog_16s[i + 1]);
    L_y = Sub_32s(L_y, Mul2_Low_32s(tmp*a));

    *fraction = ExtractHigh(L_y);

    return;
}


void Int_gain(Ipp16s old_gain, Ipp16s new_gain, const Ipp16s *Int_wind,
      Ipp16s *gain, Ipp32s nb_subfr)
{
  Ipp16s fold;
  Ipp32s k;
  const Ipp16s *fnew;
  Ipp32s Ltmp;

  fnew = Int_wind;

  for (k=0; k<nb_subfr; k++) {
    fold = Sub_16s(32767, *fnew);
    Ltmp = Mul2_Low_32s(old_gain*fold);
    Ltmp = Add_32s(Ltmp, Mul2_Low_32s(new_gain*(*fnew)));
    *gain = Cnvrt_NR_32s16s(Ltmp);
    fnew ++;
    gain++;
  }
  return;
}


static __ALIGN(16) CONST Ipp16s tblGainQuantHF_16s[] = {
   0xD640, 0xD291, 0xD455, 0xDCF9, 0xD60A, 0xD917, 0xDFEC, 0xEEF0,
   0xD776, 0xE1B7, 0xEE95, 0xF2AB, 0xFB1A, 0xD1D3, 0xD37E, 0xD544,
   0xD712, 0xE38E, 0xFFF3, 0x0093, 0xF1FB, 0xF21C, 0xF2D9, 0xF2E1,
   0xF661, 0xF51F, 0xF3FC, 0xF446, 0xF3F7, 0xF4F0, 0xF60A, 0xF92E,
   0xF5FD, 0xF5C5, 0xF9AA, 0xF525, 0xF729, 0xF842, 0xF6B4, 0xF7B6,
   0xFA1E, 0xF95C, 0xF5A4, 0xF400, 0xFB52, 0xF610, 0xF6FF, 0xF80D,
   0xF83E, 0xF79C, 0xF8FE, 0xFAA7, 0xF9ED, 0xF9BC, 0xFAB1, 0xF739,
   0xF748, 0xFCC8, 0xF75C, 0xFA1F, 0xFBAB, 0xFA9F, 0xF86E, 0xF9A9,
   0xF948, 0xFAC9, 0xFB15, 0xFB50, 0xF549, 0xF8D9, 0xFBB2, 0xFB2E,
   0xFBDF, 0xF81B, 0xFB46, 0xFBCE, 0xFB59, 0xF9D0, 0xF729, 0xFE05,
   0xFCB4, 0xFBAC, 0xFBB2, 0xFA1C, 0xFC25, 0xFC02, 0xFAB2, 0xFCF9,
   0xF9E1, 0xF9B0, 0xFC10, 0xFF01, 0xFA1B, 0xFBE4, 0xFE22, 0xFCED,
   0xFD5B, 0xFB2A, 0xFD27, 0xFD61, 0xFBA3, 0xF928, 0xFF85, 0xF928,
   0xFC9C, 0xFE5F, 0xFCD8, 0xFC6A, 0xF9EA, 0xFE9B, 0xFC7D, 0xF92E,
   0xF938, 0xFE0E, 0xFAFA, 0xFEDB, 0xFC04, 0xFCC3, 0xFD7F, 0x000A,
   0xFD10, 0xFD3D, 0xFFD7, 0xFBFB, 0xFF6E, 0xFD1A, 0xFBCE, 0xFC0D,
   0xFE51, 0xFDCF, 0xFDC4, 0xFE5E, 0xFF3F, 0xFDD4, 0xFD44, 0xF840,
   0xFDB6, 0xFF4B, 0xF94B, 0xFB01, 0xFFAB, 0xFF28, 0xFE33, 0xFC19,
   0xFE00, 0xFFD0, 0xFB94, 0xFF62, 0xFF8A, 0xFC04, 0xFA98, 0xFF9E,
   0x0070, 0xF9AB, 0xF9CA, 0xFA36, 0xFE49, 0xFC34, 0xF824, 0xF5FF,
   0x032A, 0xFEEA, 0xFA17, 0xF9C0, 0x0174, 0xFF21, 0xFD24, 0xFE59,
   0x0103, 0xFB6F, 0xFEFC, 0xFCC6, 0xFFBC, 0xFE88, 0xFFB1, 0xFEA3,
   0xFFDB, 0xFCF7, 0xFE27, 0x0189, 0xFFA1, 0xF620, 0xFDBA, 0xFFD4,
   0xFD6E, 0xFB80, 0x0047, 0xFF8D, 0xFDBD, 0xFEC7, 0xFF82, 0x0077,
   0xFB10, 0xFF13, 0xFF00, 0xFEB0, 0xFE57, 0x00D6, 0xFEF0, 0xFE65,
   0x0030, 0x003A, 0xFE9A, 0x0079, 0xFCF8, 0xFF85, 0x0200, 0xFE75,
   0xFFF7, 0x0083, 0x0117, 0xFFFE, 0xFFB3, 0xFDF0, 0x01D5, 0x00ED,
   0xFD4F, 0x00D6, 0x01CC, 0x01BA, 0xFD40, 0x007C, 0xFDC3, 0x02EF,
   0xFFF7, 0xFFD8, 0x009D, 0x02CA, 0xF957, 0xFF9E, 0x0118, 0x021C,
   0xFCCB, 0xFD31, 0x0158, 0x03F8, 0xF8C6, 0xFA00, 0x00C7, 0x0101,
   0xFF96, 0x030F, 0x00B1, 0x01BC, 0xFB5B, 0xFB5B, 0xFBA0, 0x0423,
   0xF4DD, 0xFDE6, 0xFF20, 0xFE51, 0xFB73, 0x0414, 0xFE1A, 0xFE48,
   0xFE56, 0x011D, 0x00E5, 0xFA2D, 0x0168, 0x0129, 0x0005, 0xFDFB,
   0x02A2, 0xFEE5, 0x0044, 0x004B, 0x00E6, 0xFE47, 0x029A, 0xFCCD,
   0x0138, 0x029E, 0xFCFC, 0xFB7C, 0x0404, 0xFFF7, 0xFF7A, 0xFBB2,
   0x00E5, 0x0412, 0xFDD5, 0xFFCE, 0x023D, 0x01DF, 0x00CA, 0x00DE,
   0x04BC, 0x017D, 0xFE82, 0xFF86, 0x0240, 0x01A3, 0xF7F4, 0xFF95,
   0x029E, 0x00B2, 0xFD9A, 0x0376, 0x041D, 0x03ED, 0x013B, 0xFE9C,
   0x05AC, 0xFBC4, 0xFD81, 0xFE86, 0x03A8, 0x00AF, 0x039E, 0xFFA2,
   0xFFD6, 0x0333, 0x02D3, 0xFE72, 0x020A, 0x015A, 0x021B, 0x0383,
   0x0060, 0x0165, 0x03A7, 0x0188, 0x028F, 0x03F9, 0x02F8, 0x020C,
   0x051C, 0x0272, 0x0195, 0x02A7, 0x033F, 0xFBE0, 0x025A, 0x0329,
   0xFFD0, 0xFFA9, 0x0440, 0x047F, 0xFE54, 0x01E4, 0x01FE, 0x0543,
   0x011A, 0x0375, 0x04A5, 0x04F2, 0x035F, 0x0188, 0x0291, 0x06D6,
   0x04B2, 0x014C, 0x057F, 0x035E, 0x0195, 0x0503, 0x0060, 0x052A,
   0x0470, 0x04C4, 0x0452, 0x04CC, 0xFCB5, 0x0435, 0x04A1, 0x02E8,
   0x022A, 0x03F1, 0x0747, 0x019A, 0x0570, 0x0524, 0x045B, 0x012A,
   0x0175, 0x0825, 0x02F1, 0x0123, 0x03B8, 0x0402, 0x0443, 0xFAD0,
   0x05F3, 0x060F, 0xFFB1, 0x0213, 0x086A, 0x052F, 0x0455, 0x03ED,
   0x090F, 0x01EF, 0x019F, 0x006E, 0x0890, 0x0754, 0x0357, 0xFE49,
   0x06FE, 0x04E4, 0xFBBB, 0xFC1B, 0x0251, 0x01AD, 0x00BB, 0xF556,
   0x05FC, 0x085A, 0x050A, 0x0491, 0x06AB, 0x0663, 0x02D0, 0x07CB,
   0x0781, 0x06DA, 0x08C0, 0x02A3, 0x05F4, 0x05E1, 0x0752, 0x0695,
   0x079A, 0x0240, 0x061A, 0x0785, 0x01DB, 0x0788, 0x077A, 0x05F1,
   0x02C7, 0x02CC, 0x07D5, 0x074E, 0x0180, 0x04E4, 0x04CB, 0x094F,
   0x059A, 0x07D2, 0x072D, 0x0A29, 0x09B2, 0x07F4, 0x073C, 0x0729,
   0xFCEF, 0x00A2, 0x061F, 0x085F, 0xFF5F, 0xFE98, 0xFF53, 0x084A,
   0xFD86, 0xFDF8, 0x06B3, 0x0020, 0x01CA, 0x0621, 0x0B04, 0x0C39,
   0x0828, 0x0759, 0x0A82, 0x0A17, 0x0738, 0x0AD9, 0x099A, 0x0760,
   0x0B84, 0x0A24, 0x05B4, 0x0307, 0x0B0B, 0x0A8C, 0x095A, 0x0A92,
   0x0D2E, 0x0B95, 0x0A9B, 0x0698, 0x08F6, 0x0B89, 0x0CF5, 0x0D5F,
   0x0DFC, 0x0DB2, 0x0D32, 0x0C80, 0x109F, 0x110F, 0x1151, 0x10B3,
   0xF656, 0xF628, 0xFA09, 0x0012, 0x0073, 0x05DE, 0xFEE4, 0xD1C9,
   0x0979, 0x0472, 0xEBEE, 0xD769, 0x0531, 0xF7F1, 0xD27F, 0xD538};

void Q_gain_hf(Ipp16s *gain, Ipp16s *gain_q, Ipp16s *indice)
{
   Ipp16s gain2[Q_GN_ORDER], tmp16;
   Ipp32s i, k, iQ, min_err, distance, idx=0;

   distance = IPP_MAX_32S;
   for (k=0; k<SIZE_BK_HF; k++) {
      for (i=0; i<Q_GN_ORDER; i++) {
         gain2[i] = Sub_16s(Sub_16s(gain[i], tblGainQuantHF_16s[(k*Q_GN_ORDER)+i]), MEAN_GAIN_HF_FX);
      }
      min_err = 0;
      for (i=0; i<Q_GN_ORDER; i++) {
         tmp16 = (Ipp16s)((gain2[i]*8192)>>15);
         min_err = Add_32s(min_err, Mul2_Low_32s(tmp16*tmp16));
      }

      if (min_err < distance) {
         distance = min_err;
         idx = k;
      }
   }
   indice[0] = (Ipp16s) idx;
   iQ = idx << 2;
   for (i=0; i<Q_GN_ORDER; i++) {
      gain_q[i] = Add_16s(tblGainQuantHF_16s[iQ+i], MEAN_GAIN_HF_FX);
   }
   return;
}



#define ALPHA_FX    29491

void D_gain_hf(Ipp16s indice, Ipp16s *gain_q, Ipp16s *past_q, Ipp32s bfi)
{
   Ipp32s i, iQ, Ltmp;

   iQ = indice << 2;
   if(bfi == 0) {
      for (i=0; i<Q_GN_ORDER; i++) {
         gain_q[i] = Add_16s(tblGainQuantHF_16s[iQ+i], MEAN_GAIN_HF_FX);
      }
   } else {
      *past_q = Sub_16s(MulHR_16s(ALPHA_FX, Add_16s(*past_q ,5120)), 5120);

      for (i=0; i<Q_GN_ORDER; i++) {
         gain_q[i] = Add_16s( *past_q ,  MEAN_GAIN_HF_FX);
      }
   }

   Ltmp = 0;
   for (i=0; i<Q_GN_ORDER; i++) {
      Ltmp = Add_32s(Ltmp, gain_q[i]);
   }
   *past_q = (Ipp16s)((Sub_32s(Ltmp, 2875)>>2));
   return;
}

void Cos_window(Ipp16s *fh, Ipp32s n1, Ipp32s n2, Ipp32s inc2)
{
   Ipp32s i, j;
   Ipp32s inc, offset,offset2;
   Ipp32s Ltmp;
   const Ipp16s *pt_cosup, *pt_cosdwn;

   offset2 = 1;

   if(n1 == L_OVLP) {
      inc = 1;
      offset2 = 0;
   } else if (n1 == (L_OVLP/2)) {
      inc = 2;
   } else if (n1 == (L_OVLP/4)) {
      inc = 4;
   } else if (n1 == (L_OVLP/8)) {
      inc = 8;
   } else {
      inc = 16;
   }

   offset = (inc>>1) - 1;

   if(offset < 0) {
      offset = 0;
   }

   pt_cosup = tblCosHF_16s;
   j = offset;
   for(i=0; i<n1; i++,j+=inc) {
      Ltmp = Mul2_Low_32s(pt_cosup[j]*16384);
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(pt_cosup[j+offset2]*16384));
      fh[i] = Cnvrt_NR_32s16s(Ltmp);
   }

   offset = (inc2>>1) - 1;

   if(offset < 0) {
      offset = 0;
   }

   if(inc2 == 1) {
      offset2 = 0;
   } else {
      offset2 = 1;
   }

   pt_cosdwn = tblCosHF_16s;
   j = L_OVLP - 1 - offset;
   for(i=0; i<n2; i++,j-=inc2) {
      Ltmp = Mul2_Low_32s(pt_cosdwn[j]*16384);
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(pt_cosdwn[j-offset2]*16384));
      fh[i+n1] = Cnvrt_NR_32s16s(Ltmp);
   }
   return;
}


Ipp32s SpPeak1k6(Ipp16s* xri, Ipp16s *exp_m, Ipp32s len)
{
   Ipp32s Lmax, Ltmp;
   Ipp32s i, j, i8;

   Lmax = 1;
   for(i=0; i<len; i+=8) {
      Ltmp = 1;
      i8 = i + 8;
      for(j=i; j<i8; j++) {
         Ltmp = Add_32s(Ltmp, Mul2_Low_32s(xri[j]*xri[j]));
      }
      if (Ltmp > Lmax) {
         Lmax = Ltmp;
      }
   }

   *exp_m = Norm_32s_I(&Lmax);
   *exp_m = (Ipp16s)(30 - (*exp_m));
   InvSqrt_32s16s_I(&Lmax, exp_m);
   return Lmax;
}

void Adap_low_freq_emph(Ipp16s* xri, Ipp32s lg)
{
   Ipp32s i, j,lg4, i8;
   Ipp16s m_max, fac, tmp16, e_max, e_tmp, e_maxtmp, max ;
   Ipp32s Ltmp;

   lg4  = lg >>2;

   m_max = (Ipp16s)(SpPeak1k6(xri, &e_max, lg4)>>16);

   fac = 20480;
   for(i=0; i<lg4; i+=8) {
      max = m_max;
      e_maxtmp = e_max;

      Ltmp = 1;
      i8 = i + 8;
      for(j=i; j<i8; j++) {
         Ltmp = Add_32s(Ltmp, Mul2_Low_32s(xri[j]*xri[j]));
      }

      e_tmp = (Ipp16s)(30 - Norm_32s_I(&Ltmp));
      InvSqrt_32s16s_I(&Ltmp, &e_tmp);
      tmp16 = (Ipp16s)(Ltmp>>16);
      if(max > tmp16) {
         max = (Ipp16s)(max >> 1);
         e_maxtmp = (Ipp16s)(e_maxtmp + 1);
      }

      tmp16 = Div_16s(max, tmp16);
      e_tmp = (Ipp16s)(e_maxtmp - e_tmp);

      Ltmp = ((Ipp32s)tmp16)<<16;
      InvSqrt_32s16s_I(&Ltmp,&e_tmp);

      if (e_tmp > 4) {
         tmp16 = (Ipp16s)(ShiftL_32s(Ltmp, (Ipp16u)(e_tmp-4))>>16);
      } else {
         tmp16 = (Ipp16s)( (Ltmp>>(4-e_tmp)) >> 16);
      }

      if (tmp16 < fac) {
         fac = tmp16;
      }
      i8 = i + 8;
      for(j=i; j<i8; j++) {
         Ltmp = Mul2_Low_32s(xri[j]*fac);
         xri[j] = Cnvrt_NR_32s16s(ShiftL_32s(Ltmp, 4));
      }
   }

  return;
}

static void Deemph1k6(Ipp16s *xri, Ipp16s e_max, Ipp16s m_max, Ipp32s len)
{
  Ipp32s Ltmp;
  Ipp32s i, j, i8;
  Ipp16s exp_tmp, tmp16,fac;

   fac = 3277;
   for(i=0; i<len; i+=8) {
      Ltmp = 0;
      i8 = i + 8;
      for(j=i; j<i8; j++) {
         Ltmp = Add_32s(Ltmp, Mul2_Low_32s(xri[j]*xri[j]));
      }

      exp_tmp = (Ipp16s)(30 - Norm_32s_I(&Ltmp));
      InvSqrt_32s16s_I(&Ltmp, &exp_tmp);

      tmp16 = (Ipp16s)(Ltmp>>16);

      if(m_max > tmp16) {
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

      for(j=i; j<i8; j++) {
         xri[j] = (Ipp16s)((xri[j]*fac)>>15);
      }
   }
   return;
}

void Adap_low_freq_deemph(Ipp16s* xri, Ipp32s len)
{
   Ipp16s exp_m, max;
   max = (Ipp16s)(SpPeak1k6(xri, &exp_m, (len>>2))>>16);
   Deemph1k6(xri, exp_m, max, (len>>2));
   return;
}


void Scale_tcx_ifft(Ipp16s* exc, Ipp32s lg, Ipp16s *Q_exc)
{
   Ipp16s tmp, rem_bit;
   Ipp32s i, Ltmp;

   Ltmp = 0;
   for(i=0; i<lg; i++) {
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(exc[i]*exc[i]));
   }
   tmp = 0;

   rem_bit = 4;
   if(lg == 1152) {
      rem_bit = 5;
   }

   if (Ltmp != 0) {
      tmp = Exp_32s(Ltmp);
      tmp = (Ipp16s)((tmp - rem_bit) >> 1);
      if (tmp < 0) {
         tmp = 0;
      }
   }

   if(tmp > 10) {
      tmp = 10;
   }

   *Q_exc = tmp;

   if (tmp!=0) {
      ownScaleSignal_AMRWB_16s_ISfs(exc, lg, tmp);
   }
   return;
}

static Ipp16s table_isqrt[49] =
{
    32767, 31790, 30894, 30070, 29309, 28602, 27945, 27330, 26755, 26214,
    25705, 25225, 24770, 24339, 23930, 23541, 23170, 22817, 22479, 22155,
    21845, 21548, 21263, 20988, 20724, 20470, 20225, 19988, 19760, 19539,
    19326, 19119, 18919, 18725, 18536, 18354, 18176, 18004, 17837, 17674,
    17515, 17361, 17211, 17064, 16921, 16782, 16646, 16514, 16384
};
void _Isqrt_n(
     Ipp32s* frac,
     Ipp16s* exp
)
{
    Ipp16s i, a, tmp;

    if (*frac <= (Ipp32s) 0)
    {
        *exp = 0;
        *frac = 0x7fffffffL;
        return;
    }

   if ((*exp & 1) == 1) {
      *frac = (*frac) >> 1;
   }

   *exp = Negate_16s((Ipp16s)((*exp - 1) >> 1));

   *frac = (*frac) >> 9;
   i = ExtractHigh(*frac);
   *frac = (*frac) >> 1;
   a = (Ipp16s)(*frac);
   a = (Ipp16s) (a & (Ipp16s) 0x7fff);

   i = (Ipp16s)(i - 16);
   *frac = (Ipp32s)table_isqrt[i] << 16;
   tmp = Sub_16s(table_isqrt[i], table_isqrt[i + 1]);
   *frac = Sub_32s(*frac, Mul2_Low_32s(tmp*a));

   return;
}
Ipp32s _Isqrt(Ipp32s L_x)
{
   Ipp16s exp;
   Ipp32s L_y;

   exp = (Ipp16s)(31 - Norm_32s_I(&L_x));
   _Isqrt_n(&L_x, &exp);

   if (exp > 0) {
      L_y = ShiftL_32s(L_x, exp);
   } else {
      L_y = L_x >> (-exp);
   }

    return (L_y);
}

/*Ipp16s Div_16s(Ipp16s var1, Ipp16s var2)
{
   if ((var1<var2) && (var1>0) && (var2>0)) {
      return (Ipp16s)( (((Ipp32s)var1)<<15) / var2);
   } else if ((var2!=0) && (var1==var2)) {
      return IPP_MAX_16S;
   }
   //else any other ((var1>var2), (var1<=0), (var2<=0))
   return 0;
}*/


void ownWindowing(const Ipp16s *pWindow, Ipp16s *pSrcDstVec, Ipp32s length)
{
   ippsMul_NR_16s_ISfs(pWindow, pSrcDstVec, length, 15);
}

void Fir_filt(Ipp16s* a, Ipp32s m, Ipp16s* x, Ipp16s* y, Ipp32s len)
{
   Ipp32s i, j, L_s;

   for (i=0; i<len; i++) {
      L_s = Mul2_Low_32s(x[i]*a[0]);
      for (j=1; j<m; j++) {
         L_s = Add_32s(L_s, Mul2_Low_32s(a[j]*x[i-j]));
      }
      y[i] = Cnvrt_NR_32s16s(ShiftL_32s(L_s,2));
   }
}

void Apply_tcx_overlap(Ipp16s* xnq_fx, Ipp16s* wovlp_fx, Ipp32s lext, Ipp32s L_frame)
{
   Ipp32s i, Ltmp;
   for (i=0; i<L_OVLP_2k; i++) {
      Ltmp = Mul2_Low_32s(xnq_fx[i]*32767);
      Ltmp = Add_32s(Ltmp, Mul2_Low_32s(wovlp_fx[i]*32767));
      xnq_fx[i] = Cnvrt_NR_32s16s(Ltmp);
   }
   for (i=0; i<lext; i++) {
      wovlp_fx[i] = xnq_fx[i+L_frame];
   }
   for (i=lext; i<L_OVLP_2k; i++) {
      wovlp_fx[i] = 0;
   }
   return;
}

