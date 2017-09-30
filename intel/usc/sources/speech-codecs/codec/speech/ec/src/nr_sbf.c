/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the tems of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the tems of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives EC Sample
//
//  By downloading and installing this sample, you hereby agree that the
//  accompanying Materials are being provided to you under the terms and
//  conditions of the End User License Agreement for the Intel(R) Integrated
//  Performance Primitives product previously accepted by you. Please refer
//  to the file ippEULA.rtf or ippEULA.txt located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//
//  Purpose: sbf NR
//
*/

#include <stdlib.h>
#include <ipps.h>
#include <ippsc.h>
#include "scratchmem.h"
#include "nlp_api.h"
#include "ec_api.h"
#include "ec_td.h"
#include "ownah.h"
#include "ah_api.h"
#include "ownec.h"

#define ALIGN(s) (((s) + 15) & ~15)
#define COEF_SF 14
#define COEF_ONE (1 << COEF_SF)

static Ipp8u t_fp_norm_table[256] = {
    0, 1, 2, 2,
    3, 3, 3, 3,
    4, 4, 4, 4,
    4, 4, 4, 4,
    5, 5, 5, 5,
    5, 5, 5, 5,
    5, 5, 5, 5,
    5, 5, 5, 5,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    6, 6, 6, 6,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    7, 7, 7, 7,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8,
    8, 8, 8, 8
};
#define t_fp_norm(a)                        \
{                                           \
    int t, shift;                             \
    int m = a.m;                              \
    \
    if (m == 0) {                             \
    a.e = 0;                                \
    } else {                                  \
    if (m > 0xffff) {                       \
    t = m >> 16;                          \
    if (t > 0xff){                          \
    shift = 9 + t_fp_norm_table[t >> 8];  \
    } else {                                \
    shift = 1 + t_fp_norm_table[t];       \
    }                                       \
    a.m = m >> shift;                     \
    a.e += shift;                         \
    } else {                                \
    if (m > 0xff){                        \
    shift = 8 - t_fp_norm_table[m >> 8];\
    } else {                              \
    shift = 16 - t_fp_norm_table[m];    \
    }                                     \
    m <<= shift;                          \
    a.e -= (shift - 1);                   \
    a.m = m >> 1;                         \
    }                                       \
    }                                         \
}

__INLINE t_fp t_fp_ini_imm(int a, int b) {
    t_fp c;
    c.m = a;
    c.e = b;
    return c;
}

#define t_fp_ini32s(a, c) \
{                         \
    c.m = a;                \
    c.e = 15;               \
    t_fp_norm(c);           \
}

#define t_fp_align(a, b)                         \
{                                                \
    int shift;                                     \
    if (a.e < b.e) {                               \
    shift = b.e - a.e;                           \
    if (shift) {                                 \
    if (shift < 32) {                          \
    a.m >>= shift;                           \
    } else {                                   \
    a.m = 0;                                 \
    }                                          \
    a.e += shift;                              \
    }                                            \
    } else {                                       \
    shift = a.e - b.e;                           \
    if (shift) {                                 \
    if (shift < 32) {                          \
    b.m >>= shift;                           \
    } else {                                   \
    b.m = 0;                                 \
    }                                          \
    b.e += shift;                              \
    }                                            \
    }                                              \
}


#define t_fp_add(a, b, c) \
{                         \
    t_fp a0 = a;            \
    t_fp b0 = b;            \
    \
    t_fp_align(a0, b0);     \
    \
    c.m = a0.m + b0.m;      \
    c.e = a0.e;             \
    \
    if (c.m >= 0x8000) {    \
    c.m >>= 1;            \
    c.e += 1;             \
    }                       \
}

#define t_fp_sub(a, b, c) \
{                         \
    t_fp a0 = a;            \
    t_fp b0 = b;            \
    \
    t_fp_align(a0, b0);     \
    c.m = a0.m - b0.m;      \
    c.e = a0.e;             \
    \
    if (c.m < 0x4000) {     \
    c.m <<= 1;            \
    c.e -= 1;             \
    }                       \
}

#define t_fp_eq(a, b, ret)       \
{                                \
    t_fp a0 = a;                   \
    t_fp b0 = b;                   \
    \
    t_fp_align(a0, b0);            \
    \
    if (a0.m < b0.m) ret = -1;     \
  else if (a0.m > b0.m) ret = 1; \
  else ret = 0;                  \
}

#define t_fp_mul(a, b, c) {    \
    c.m = a.m * b.m;             \
    if (c.m >= 0x20000000) {     \
    c.m >>= 15;                \
    c.e = a.e + b.e;           \
    } else {                     \
    c.m >>= 14;                \
    c.e = a.e + b.e - 1;       \
    }                            \
}

#define t_fp_div(a, b, c)    \
{                            \
    if (b.m == 0) {            \
    c.m = c.e = 0;           \
    } else {                   \
    c.m = (a.m << 15) / b.m; \
    if (c.m >= 0x8000) {     \
    c.m >>= 1;             \
    c.e = a.e - b.e + 1;   \
    } else {                 \
    c.e = a.e - b.e;       \
    }                        \
    }                          \
}

static Ipp16u isbf_sqrt(Ipp32u x)
{
    Ipp16u r=0;
    Ipp16u i;
    for(i=IPP_MAX_16U; i>0; i>>=1)
    {
        r = r + i;
        if( (Ipp32u)(r*r) > x)
            r = r - i;
    }
    return (Ipp16u)r;
}
__INLINE t_fp t_fp_sqrt(t_fp a) {
    Ipp16u tt;
    t_fp sqrt2 = t_fp_ini_imm(23170, 1);

    a.e--;
    a.m <<= 1;
    tt = (Ipp16u)a.m;
    //ippsSqrt_16u_ISfs(&tt, 1, -7);
    tt = isbf_sqrt(tt<<14);
    a.m = tt;
    if (a.e & 1) {
        a.e = (a.e >> 1) + 1;
    } else {
        a.e = (a.e >> 1);
        t_fp_mul(a, sqrt2, a);
    }

    t_fp_norm(a);
    return a;
}

IppStatus ippsFilterNoiseGetStateSize_EC_FB_32f (IppPCMFrequency pcmFreq,int *size)
{
    //IPP_BAD_PTR1_RET(size);
    //IPP_BADARG_RET(pcmFreq != IPP_PCM_FREQ_8000 && pcmFreq != IPP_PCM_FREQ_16000 && pcmFreq != IPP_PCM_FREQ_22050 && pcmFreq != IPP_PCM_FREQ_32000, ippStsRangeErr);
    *size = ALIGN(sizeof(IppsFilterNoiseState_EC_FB_32f));
    return ippStsNoErr;
}
IppStatus ippsFilterNoiseInit_EC_FB_32f (IppPCMFrequency pcmFreq, IppsFilterNoiseState_EC_FB_32f *state)
{
    IppStatus status=ippStsNoErr;
    //Ipp8u *pInit = (Ipp8u *)state;
    //IPP_BAD_PTR1_RET(state);
    //IPP_BADARG_RET(pcmFreq != IPP_PCM_FREQ_8000 && pcmFreq != IPP_PCM_FREQ_16000 && pcmFreq != IPP_PCM_FREQ_22050 && pcmFreq != IPP_PCM_FREQ_32000, ippStsRangeErr);
    //pInit += ALIGN(sizeof(IppsFilterNoiseState_EC_FB_32f));
    ippsZero_32s((Ipp32s*)state->ns_lambda_d, SBF_SUBBANDS * 2);
    state->ns_one = t_fp_ini_imm(0x4000, 1);
    //state->ns_beta = t_fp_ini_imm(0x7fbe, 0);
    state->ns_beta = t_fp_ini_imm(0x7f7c, 0);
    //state->ns_beta1m = t_fp_ini_imm(0x4188, -8);
    state->ns_beta1m = t_fp_ini_imm(0x4188, -7);
    state->ns_alpha = t_fp_ini_imm(0x7d70, 0);
    state->ns_alpha1m = t_fp_ini_imm(0x51eb, -5);
    state->ns_gamma = t_fp_ini_imm(0x6666, 0);
    state->ns_gamma1m = t_fp_ini_imm(0x6666, -2);
    //    state->ns_al = state->ns_alpha;
    //    state->ns_al1m = state->ns_alpha1m;
    //    state->ns_al = t_fp_ini_imm(0x7fce, 0); // 0.9985
    //    state->ns_al1m = t_fp_ini_imm(0x6240, -9); // 0.0015
    state->ns_al = t_fp_ini_imm(0x7fbe, 0); //0.998
    state->ns_al1m = t_fp_ini_imm(0x4188, -8); //0.002
    //      ns_al = t_fp_ini_imm(0x7f9d, 0); // 0.997
    //      ns_al1m = t_fp_ini_imm(0x6248, -8); // 0.003
    //state->ns_al = t_fp_ini_imm(0x7eb8, 0); // 0.99
    //state->ns_al1m = t_fp_ini_imm(0x51ea, -6); // 0.01
    //        state->ns_al = t_ini(0.980);
    //              printf("%x %d %f\n",state->ns_al.m, state->ns_al.e, t_get(state->ns_al));
    //              printf("%x %d %f\n",state->ns_al1m.m, state->ns_al1m.e, t_get(state->ns_al1m));exit(1);
    ippsZero_32s((Ipp32s*)state->ns_x_pwr, SBF_SUBBANDS * 2);
    return status;
}
int noise_supp(IppsFilterNoiseState_EC_FB_32f *state, Ipp32sc *sout,int dt,int es_off) {
    int i;
    int res;
    for (i = 0; i < SBF_SUBBANDS; i++) {
        t_fp y_pwr_t_fp, gamma_t_fp, ksi_t_fp, fau_t_fp, h_f;
        t_fp tmp;
        Ipp32s tt;

        tt = sout[i].re;
        if (tt < 0) tt = -tt;
        t_fp_ini32s(tt, tmp);
        t_fp_mul(tmp, tmp, y_pwr_t_fp);
        tt = sout[i].im;
        if (tt < 0) tt = -tt;
        t_fp_ini32s(tt, tmp);
        t_fp_mul(tmp, tmp, tmp);
        t_fp_add(y_pwr_t_fp, tmp, y_pwr_t_fp);

        if (!dt) {
            t_fp_mul(state->ns_beta, state->ns_lambda_d[i], state->ns_lambda_d[i]);
            t_fp_mul(state->ns_beta1m, y_pwr_t_fp, tmp);
            t_fp_add(state->ns_lambda_d[i], tmp, state->ns_lambda_d[i]);
        }

        ksi_t_fp.m = ksi_t_fp.e = 0;
        h_f = state->ns_one;

        if (state->ns_lambda_d[i].m > 0) {
            t_fp_mul(state->ns_alpha, state->ns_x_pwr[i], ksi_t_fp);

            t_fp_eq(y_pwr_t_fp, state->ns_lambda_d[i], res);
            if (res < 0){
                gamma_t_fp = state->ns_one;
            } else {
                t_fp_sub(y_pwr_t_fp, state->ns_lambda_d[i], tmp);
                t_fp_mul(state->ns_alpha1m, tmp, tmp);
                t_fp_add(ksi_t_fp, tmp, ksi_t_fp);
                t_fp_div(state->ns_lambda_d[i], y_pwr_t_fp, gamma_t_fp);
            }

            t_fp_add(ksi_t_fp, state->ns_lambda_d[i], tmp);
            t_fp_div(ksi_t_fp, tmp, fau_t_fp);

            t_fp_add(fau_t_fp, gamma_t_fp, tmp);
            t_fp_mul(fau_t_fp, tmp, h_f);

            t_fp_eq(state->ns_one, h_f, res);
            if (res < 0)
                h_f = state->ns_one;

            t_fp_mul(state->ns_gamma, state->ns_x_pwr[i], state->ns_x_pwr[i]);
            t_fp_mul(y_pwr_t_fp, h_f, tmp);
            t_fp_mul(state->ns_gamma1m, tmp, tmp);
            t_fp_add(state->ns_x_pwr[i], tmp, state->ns_x_pwr[i]);

            h_f = t_fp_sqrt(h_f);
        }


        while(h_f.e > 1) {h_f.e--; h_f.m <<= 1;}
        while(h_f.e < 1) {h_f.e++; h_f.m >>= 1;}

        if (h_f.m < (1 << (COEF_SF - 3)))
            h_f.m = (1 << (COEF_SF - 3));

        if (es_off == 1) {
            sout[i].re = (Ipp32s)(sout[i].re * (Ipp64s)h_f.m >> COEF_SF);
            sout[i].im = (Ipp32s)(sout[i].im * (Ipp64s)h_f.m >> COEF_SF);
        }
    }
    return ippStsNoErr;
}
IppStatus ippsFilterNoise_EC_FB_32f_I (Ipp32f pSrcDst[16],IppsFilterNoiseState_EC_FB_32f *state)
{
    //IPP_BAD_PTR2_RET(pSrcDst,state);
    return ippStsOk;
}
IppStatus ippsFilterNoise_EC_FB_32f (const Ipp32f pSrc[16],Ipp32f pDst[16],IppsFilterNoiseState_EC_FB_32f *state)
{
    //IPP_BAD_PTR3_RET(pSrc,pDst,state);
    return ippStsOk;
}
IppStatus ippsFilterNoiseLevel_EC_FB_32f (IppsNRLevel  level, IppsFilterNoiseState_EC_FB_32f* state)
{
    //IPP_BAD_PTR1_RET(state);
    return ippStsNoErr;
}
