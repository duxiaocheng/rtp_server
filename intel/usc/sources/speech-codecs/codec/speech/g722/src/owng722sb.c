/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2011-2012 Intel Corporation. All Rights Reserved.
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
// Purpose: G.722 speech codec: encode API.
//
*/

#include "owng722sb.h"

__ALIGN32 CONST short    G722PLC_lpc_win_80[HAMWINDLEN] = {
  2621,   2637,   2684,   2762,   2871,
  3010,   3180,   3380,   3610,   3869,
  4157,   4473,   4816,   5185,   5581,
  6002,   6447,   6915,   7406,   7918,
  8451,   9002,   9571,  10158,  10760,
 11376,  12005,  12647,  13298,  13959,
 14628,  15302,  15982,  16666,  17351,
 18037,  18723,  19406,  20086,  20761,
 21429,  22090,  22742,  23383,  24012,
 24629,  25231,  25817,  26386,  26938,
 27470,  27982,  28473,  28941,  29386,
 29807,  30203,  30573,  30916,  31231,
 31519,  31778,  32008,  32208,  32378,
 32518,  32627,  32705,  32751,  32767,
 32029,  29888,  26554,  22352,  17694,
 13036,   8835,   5500,   3359,   2621
};

__ALIGN32 CONST short G722PLC_b_hp[2] = {16384, -16384};
__ALIGN32 CONST short G722PLC_a_hp[2] = {16384,  15744};


__ALIGN32 CONST short tblWeightFade[CROSSFADELEN-20] = {
546,   1092, 1638, 2184, 2730, 3276, 3822, 4368, 4914, 5460,
6006,  6552, 7098, 7644, 8190, 8736, 9282, 9828,10374,10920,
11466,12012,12558,13104,13650,14196,14742,15288,15834,16380,
16926,17472,18018,18564,19110,19656,20202,20748,21294,21840,
22386,22932,23478,24024,24570,25116,25662,26208,26754,27300,
27846,28392,28938,29484,30030,30576,31122,31668,32214,32760
};
__ALIGN32 CONST short tblWeightInvFade[CROSSFADELEN] = {
32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,
32767,32767,32767,32767,32767,32767,32767,32767,32767,32767,
32221,31675,31129,30583,30037,29491,28945,28399,27853,27307,
26761,26215,25669,25123,24577,24031,23485,22939,22393,21847,
21301,20755,20209,19663,19117,18571,18025,17479,16933,16387,
15841,15295,14749,14203,13657,13111,12565,12019,11473,10927,
10381, 9835, 9289, 8743, 8197, 7651, 7105, 6559, 6013, 5467,
 4921, 4375, 3829, 3283, 2737, 2191, 1645, 1099,  553,    7
};


__ALIGN32 CONST short tblWeight_VR[END_3RD_PART] = {
32358,31949,31540,31131,30722,30313,29904,29495,
29086,28677,28268,27859,27450,27041,26632,26223,
25814,25405,24996,24587,24178,23769,23360,22951,
22542,22133,21724,21315,20906,20497,20088,19679,
19270,18861,18452,18043,17634,17225,16816,16407,
15998,15589,15180,14771,14362,13953,13544,13135,
12726,12317,11908,11499,11090,10681,10272,9863,
9454,9045,8636,8227,7818,7409,7000,6591,
6182,5773,5364,4955,4546,4137,3728,3319,
2910,2501,2092,1683,1274, 865, 456,  47,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0
};

__ALIGN32 CONST short tblWeight_UV[END_3RD_PART] = {
32757,32747,32737,32727,32717,32707,32697,32687,
32677,32667,32657,32647,32637,32627,32617,32607,
32597,32587,32577,32567,32557,32547,32537,32527,
32517,32507,32497,32487,32477,32467,32457,32447,
32437,32427,32417,32407,32397,32387,32377,32367,
32357,32347,32337,32327,32317,32307,32297,32287,
32277,32267,32257,32247,32237,32227,32217,32207,
32197,32187,32177,32167,32157,32147,32137,32127,
32117,32107,32097,32087,32077,32067,32057,32047,
32037,32027,32017,32007,31997,31987,31977,31967,
31568,31169,30770,30371,29972,29573,29174,28775,
28376,27977,27578,27179,26780,26381,25982,25583,
25184,24785,24386,23987,23588,23189,22790,22391,
21992,21593,21194,20795,20396,19997,19598,19199,
18800,18401,18002,17603,17204,16805,16406,16007,
15608,15209,14810,14411,14012,13613,13214,12815,
12416,12017,11618,11219,10820,10421,10022,9623,
9224,8825,8426,8027,7628,7229,6830,6431,
6032,5633,5234,4835,4436,4037,3638,3239,
2840,2441,2042,1643,1244, 845, 446,  47,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0
};

__ALIGN32 CONST short tblWeight_V[END_3RD_PART] = {
32757,32747,32737,32727,32717,32707,32697,32687,
32677,32667,32657,32647,32637,32627,32617,32607,
32597,32587,32577,32567,32557,32547,32537,32527,
32517,32507,32497,32487,32477,32467,32457,32447,
32437,32427,32417,32407,32397,32387,32377,32367,
32357,32347,32337,32327,32317,32307,32297,32287,
32277,32267,32257,32247,32237,32227,32217,32207,
32197,32187,32177,32167,32157,32147,32137,32127,
32117,32107,32097,32087,32077,32067,32057,32047,
32037,32027,32017,32007,31997,31987,31977,31967,
31947,31927,31907,31887,31867,31847,31827,31807,
31787,31767,31747,31727,31707,31687,31667,31647,
31627,31607,31587,31567,31547,31527,31507,31487,
31467,31447,31427,31407,31387,31367,31347,31327,
31307,31287,31267,31247,31227,31207,31187,31167,
31147,31127,31107,31087,31067,31047,31027,31007,
30987,30967,30947,30927,30907,30887,30867,30847,
30827,30807,30787,30767,30747,30727,30707,30687,
30667,30647,30627,30607,30587,30567,30547,30527,
30507,30487,30467,30447,30427,30407,30387,30367,
30177,29987,29797,29607,29417,29227,29037,28847,
28657,28467,28277,28087,27897,27707,27517,27327,
27137,26947,26757,26567,26377,26187,25997,25807,
25617,25427,25237,25047,24857,24667,24477,24287,
24097,23907,23717,23527,23337,23147,22957,22767,
22577,22387,22197,22007,21817,21627,21437,21247,
21057,20867,20677,20487,20297,20107,19917,19727,
19537,19347,19157,18967,18777,18587,18397,18207,
18017,17827,17637,17447,17257,17067,16877,16687,
16497,16307,16117,15927,15737,15547,15357,15167,
14977,14787,14597,14407,14217,14027,13837,13647,
13457,13267,13077,12887,12697,12507,12317,12127,
11937,11747,11557,11367,11177,10987,10797,10607,
10417,10227,10037,9847,9657,9467,9277,9087,
8897,8707,8517,8327,8137,7947,7757,7567,
7377,7187,6997,6807,6617,6427,6237,6047,
5857,5667,5477,5287,5097,4907,4717,4527,
4337,4147,3957,3767,3577,3387,3197,3007,
2817,2627,2437,2247,2057,1867,1677,1487,
1297,1107, 917, 727, 537, 347, 157, -33
};

static void UnpackCodeword_g722(const Ipp8s *pSrc, Ipp32s len, Ipp16s *pDst)
{
   Ipp32s i, j;
   for(i = 0, j = 0; i < len; i++) {
     pDst[j++] = (Ipp16s)((Ipp8s)pSrc[i] & 0x3F); /* 6 LSB */
     pDst[j++] = (Ipp16s)(((Ipp8s)pSrc[i] >> 6) & 0x03);/* 2 MSB */
   }
}

static void PackCodeword_g722(const Ipp16s *pSrc, Ipp32s len, Ipp8s *pDst)
{
    Ipp32s i, j;
    for(i=0,j=0; i<len; i+=2) pDst[j++] = (Ipp8s)(((pSrc[i+1] << 6) + pSrc[i]) & 0xFF);
}

/****************************** own-functions **************************************/
void ownCrossCorrInv_G722_16s32s(const Ipp16s *pSrc1, const Ipp16s *pSrc2, int len, Ipp32s *pDst, int lenDst)
{
   Ipp32s i, j;
   Ipp32s sum;

   for(i = 0; i < lenDst; i++) {
       sum = 0;
       for(j = 0; j < len; j++) {
          sum = Add_32s(sum, (pSrc1[-j]*pSrc2[-j-i]));
       }
       pDst[i] = sum;
    }

   return;
}

void ownDCRemoveFilter_G722_16s(const Ipp16s *pSrc, int len, Ipp16s *pDst)
{
    Ipp32s i;
    Ipp16s x1 = 0;
    Ipp32s s0, y1 = 0;
    Ipp64s z;

    for(i = 0; i < len; i++) {
       s0 = pSrc[i] * G722PLC_b_hp[0];
       s0 += x1 * G722PLC_b_hp[1];
       z = (Ipp64s)y1 * G722PLC_a_hp[1];
       s0 += (Ipp32s)(z >> 14);
       x1 = pSrc[i];
       y1 = s0;
       pDst[i] = (Ipp16s)((s0 + 0x00002000) >> 14);
    }

    return;
}

void ownSynthesisLowbandPLC_G722_16s(G722SB_Decoder_Obj *state, Ipp16s *syn, short n)
{
    Ipp16s   *ptr;
    Ipp16s   *cur_exc;    /* pointer to current sample of excition */
    Ipp16s   *exc;        /* pointer to beginning of excitation in current frame */
    Ipp16s   i, j, jitter, temp, lag;
    IPP_ALIGNED_ARRAY(16, Ipp16s, buffer_exc, (LEN_HB_MEM*3));

    cur_exc = &buffer_exc[state->t0+T0_PLUS];
    exc = cur_exc;

    ippsCopy_16s(&state->mem_exc_lb[MAXPIT2P1 - (state->t0+T0_PLUS)], buffer_exc, (state->t0+T0_PLUS));

    if(state->frameClass == IPP_G722_VOICED) jitter = 0;
    else jitter = 1;
    state->t0 = state->t0 | jitter; /* change even delay as jitter is more efficient for odd delays */

    for(i = 0; i < n; i++) {
      j = jitter - state->t0;
      jitter = -jitter;
      cur_exc[i] = cur_exc[i+j];
    }
    ippsSynthesisFilter_G729E_16s(state->lpc, ORD_LPC, cur_exc, syn, n, state->mem_syn);
    for(i = 0; i < ORD_LPC; i++) state->mem_syn[i] = syn[n-ORD_LPC+i];

    lag = state->t0 + T0_PLUS; /*update temp samples*/
    temp = lag - n;
    ptr = &state->mem_exc_lb[MAXPIT2P1-lag];
    if(temp > 0) {
      ippsMove_16s(&ptr[n], ptr, temp);
      ippsCopy_16s(exc, &ptr[temp], n);
    } else ippsCopy_16s(&exc[n-lag], ptr, lag);

    return;
}

Ipp16s*  ownSynthesisHighbandPLC_G722_16s(G722SB_Decoder_Obj *state)
{
  int i;
  Ipp16s   *ptr, *ptr2;
  Ipp16s    mem_t0, tmp, tmp2;


  /* save pitch delay */
  mem_t0 = state->t0;

  /* if signal is not voiced, cut harmonic structure by forcing a 10 ms pitch */
  if(state->frameClass != IPP_G722_VOICED) state->t0 = 80;

  /* reconstruct higher band signal by pitch prediction */
  tmp = state->frameSize - state->t0;
  ptr = state->mem_speech_hb + (LEN_HB_MEM - state->t0); /*beginning of copy zone*/
  tmp2 = LEN_HB_MEM - state->frameSize;
  ptr2 = state->mem_speech_hb + tmp2; /*beginning of last frame in mem_speech_hb*/

  if(tmp <= 0) {/* l_frame <= t0; only possible in 10 ms mode*/
    /* temporary save of new frame in state->mem_speech_lb[0 ...state->frameSize-1] of low_band!! that will be shifted after*/
    ippsCopy_16s(ptr, state->mem_speech_lb, state->frameSize);
    ippsMove_16s(ptr2, state->mem_speech_hb, state->frameSize); /*shift 1 frame*/
    ippsCopy_16s(state->mem_speech_lb, ptr2, state->frameSize);
  } else { /*t0 < state->frameSize*/
    if(tmp2 > 0) ippsCopy_16s(ptr2, state->mem_speech_hb, tmp2); /*shift memory*/
    ippsMove_16s(ptr, ptr2, state->t0);             /*copy last period*/
    for(i=0;i<tmp;i++) ptr2[state->t0+i] = ptr2[i]; /*repeat last period*/
  }

  /* restore pitch delay */
  state->t0 = mem_t0;

  return(ptr2);
}

Ipp16s ownOpenLoopPitchSearchPLC_G722_16s(short *signal, short *maxco)
{
  Ipp16s i, j, il, ind, ind2;
  Ipp16s start_ind, end_ind, beg_last_per, end_last_per;
  Ipp16s e1, e2, co, em, norm_e, ne1, ne2;
  Ipp16s *nooffsigptr, valid = 0; /*not valid for the first lobe */

  Ipp16s energy, overfl_shft, maxco_s8;
  Ipp16s ai[3], rc[3];
  Ipp32s L_temp, ener1_f, ener2_f;
  Ipp32s ener1n, ener2n, en, n;
  Ipp32s cor_32[3];
  IPP_ALIGNED_ARRAY(16, Ipp16s, nooffsig, (MAXPIT2+FEC_L_FIR_FILTER_LTP_M1));
  IPP_ALIGNED_ARRAY(16, Ipp16s, w_ds_sig, MAXPIT2_DS);
  IPP_ALIGNED_ARRAY(16, Ipp16s, ds_sig, MAXPIT2_DS);
  IPP_ALIGNED_ARRAY(16, Ipp16s, y, MAXPIT2_DS);
  IPP_ALIGNED_ARRAY(16, Ipp32s, pCorr, MAXPIT2);
  IPP_ALIGNED_ARRAY(16, Ipp32s, energy1, MAXPIT_DS);
  IPP_ALIGNED_ARRAY(16, Ipp32s, energy2, MAXPIT_DS);
  IPP_ALIGNED_ARRAY(16, Ipp32s, energy_ds, MAXPIT_DS*2);

  /* intialize memory of DC-remove filter */
  for(i = 0; i < FEC_L_FIR_FILTER_LTP_M1; i++) nooffsig[i] = 0;
  nooffsigptr = nooffsig + FEC_L_FIR_FILTER_LTP_M1;

  /* DC-remove filter */
  ownDCRemoveFilter_G722_16s((const Ipp16s *)signal, MAXPIT2, nooffsigptr);

  /* downsample (filter and decimate) signal */
  ippsDownsampleFilter_G722_16s((const Ipp16s *)nooffsigptr, MAXPIT2, ds_sig);

  ippsMul_NR_16s_Sfs(ds_sig, &G722PLC_lpc_win_80[HAMWINDLEN-MAXPIT2_DS], y, MAXPIT2_DS, 15);
  while(ippsAutoCorr_NormE_16s32s(y, MAXPIT2_DS, cor_32, 3, &n) != ippStsNoErr) {
     ippsRShiftC_16s(y, 2, y, MAXPIT2_DS);
  }
  ippsLagWindow_G729_32s_I(cor_32+1,2); /* Lag windowing        */
  ippsLevinsonDurbin_G729_32s16s(cor_32, 2, ai, rc, &energy);
  ai[1] = (Ipp16s)(((ai[1] * GAMMA) + 0x00002000) >> 14);
  ai[2] = (Ipp16s)(((ai[2] * GAMMA2) + 0x00002000) >> 14);

    /* filter */
  w_ds_sig[0] = ds_sig[0];
  L_temp = ds_sig[1] << 12;
  L_temp += ai[1] * ds_sig[0];
  w_ds_sig[1] = (Ipp16s)((L_temp + 0x0800) >> 12);
  ippsResidualFilter_G729E_16s(ai, 2, &ds_sig[2], &w_ds_sig[2], (MAXPIT2_DS-2));

  ind = MAXPIT_S2_DS; /*default value*/
  ind2 = 0;

  /*Test overflow on w_ds_sig*/
  /* compute maximal correlation (maxco) and pitch lag (ind) */
  *maxco = 0;
  overfl_shft = 0;

  /* compute energy of signal in range [len/fac-1,(len-MAX_PIT)/fac-1] */
  ippsCrossCorr_NR_16s32s(&w_ds_sig[MAXPIT_DSP1], &w_ds_sig[MAXPIT_DSP1-MAXPIT_DS+1],
                          (MAXPIT2_DSM1-MAXPIT_DSP1+1), pCorr, MAXPIT_DS);
  ippsMul_16s32s_Sfs(w_ds_sig, w_ds_sig, energy_ds, MAXPIT2_DSM1, 0);
  ener1_f = pCorr[MAXPIT_DS-1];

  if(ener1_f < IPP_MAX_32S) ener1_f++;
  en = ener1_f;
  ne1 = Norm_32s_I(&en);
  ener2_f = ener1_f - w_ds_sig[MAXPIT2_DSM1]*w_ds_sig[MAXPIT2_DSM1];  /*update, part 1*/

  for(i = 1; i < MAXPIT_DS; i++) {
     ind2++;

     ener2_f += energy_ds[MAXPIT_DSP1-i]; /*update, part 2*/
     en = ener2_f;
     ne2 = Norm_32s_I(&en);
     norm_e = IPP_MIN(ne1, ne2);
     ener1n = ener1_f << norm_e;
     ener2n = ener2_f << norm_e;
     pCorr[MAXPIT_DS-1-i] = pCorr[MAXPIT_DS-1-i] << norm_e;
     e1 = Cnvrt_NR_32s16s(ener1n);
     e2 = Cnvrt_NR_32s16s(ener2n);
     co = Cnvrt_NR_32s16s(pCorr[MAXPIT_DS-1-i]);
     em = IPP_MAX(e1, e2);

     if(co > em) em = co;
     if(co > 0) {
         co = Div_16s(co, em);
         //co = (em>0)? (co<<15)/em :  IPP_MAX_16S;
     }
     if(co < 0) valid = 1;

     if(valid == 1) {
       if((ind2 == ind) || (ind2 == (ind <<1))) {
         if(*maxco > 27850) *maxco = 32767; /* 0.85 : high correlation, small chance that double pitch is OK*/
         maxco_s8 = *maxco >> 3;
         if(*maxco < 29126)  *maxco = *maxco + maxco_s8;/*to avoid overflow*/
       }
       if((co > *maxco) && (i >= MINPIT_DS)) { *maxco = co; ind = i; ind2 = 1; }
     }
     ener2_f -= energy_ds[MAXPIT2_DSM1-i]; /*update, part 1*/
  }

  /* convert pitch to non decimated domain */
  il = ind << FACTLOG2;
  ind = il;
  /* shift DC-removed signal to avoid overflow in correlation */
  if(ener1_f > 0x01000000) overfl_shft = overfl_shft + 1;
  if(overfl_shft > 0) ippsRShiftC_16s(&nooffsigptr[1], overfl_shft, &nooffsigptr[1], (MAXPIT2-1));

  /* refine pitch in non-decimated (8 kHz) domain by step of 1
  -> maximize correlation around estimated pitch lag (ind) */
  start_ind = il - 2;
  start_ind = IPP_MAX(start_ind, MINPIT);
  end_ind = il + 2;
  beg_last_per = MAXPIT2 - il;
  end_last_per = MAXPIT2 - 1;
  j = end_last_per - start_ind;

  ener1_f = ener2_f = 1; /*to avoid division by 0*/
  ener1_f += nooffsigptr[end_last_per] * nooffsigptr[end_last_per];
  ener2_f += nooffsigptr[j] * nooffsigptr[j];
  ippsDotProd_16s32s_Sfs(&nooffsigptr[beg_last_per], &nooffsigptr[beg_last_per],
                         (end_last_per-beg_last_per), &L_temp, 0);
  ener1_f = Add_32s(ener1_f, L_temp);
  ippsDotProd_16s32s_Sfs(&nooffsigptr[beg_last_per-start_ind], &nooffsigptr[beg_last_per-start_ind],
                         (end_last_per-beg_last_per), &L_temp, 0);
  ener2_f = Add_32s(ener2_f, L_temp);

  j = beg_last_per - start_ind;
  ener2_f -= nooffsigptr[j] * nooffsigptr[j]; /*to compansate first update part 2*/
  /* compute exponent */
  en = ener1_f;
  ne1 = Norm_32s_I(&en);
  /* compute maximal correlation (maxco) and pitch lag (ind) */
  *maxco = 0;

  ownCrossCorrInv_G722_16s32s(&nooffsigptr[end_last_per], &nooffsigptr[end_last_per-start_ind],
                              (end_last_per-beg_last_per+1), pCorr, (end_ind-start_ind+1));
  for(i = 0; i < (end_ind-start_ind+1); i++) energy1[i] = nooffsigptr[beg_last_per-end_ind+i]*nooffsigptr[beg_last_per-end_ind+i];
  for(i = 0; i < (end_ind-start_ind+1); i++) energy2[i] = nooffsigptr[end_last_per-end_ind+i]*nooffsigptr[end_last_per-end_ind+i];

  for(i = start_ind; i <= end_ind; i++) {
     ener2_f = Add_32s(ener2_f, energy1[end_ind-i]); /*update, part 2*/
     en = ener2_f;
     ne2 = Norm_32s_I(&en);
     norm_e = IPP_MIN(ne1, ne2);
     ener1n = ener1_f << norm_e;
     ener2n = ener2_f << norm_e;
     pCorr[i-start_ind] = pCorr[i-start_ind] << norm_e;
     e1 = Cnvrt_NR_32s16s(ener1n);
     e2 = Cnvrt_NR_32s16s(ener2n);
     co = Cnvrt_NR_32s16s(pCorr[i-start_ind]);
     em = IPP_MAX(e1, e2);

     if(co > em) em = co;
     if(co > 0) {
         co = Div_16s(co, em);
         //co = (em>0)? (co<<15)/em :  IPP_MAX_16S;
     }
     if(co > *maxco) ind = i;
     *maxco = IPP_MAX(co, *maxco);
     ener2_f = Sub_32s(ener2_f, energy2[end_ind-i]); /*update, part 1*/
  }

  /*2 times pitch for very small pitch, at least 2 times MINPIT */
  if((*maxco < 8192) && (ind < 32)) ind <<= 1;

  return ind;
}

/***********************************************************************************/
void g722Encode(G722SB_Encoder_Obj *state, short *code, char *outcode, int len)
{
  //Ipp32s          i, j;
  /* Encoder variables */
  IPP_ALIGNED_ARRAY(16, Ipp16s, QMFVec, LEN_BLOCK);
  IPP_ALIGNED_ARRAY(16, Ipp16s, CodeVec, LEN_BLOCK);

  /* QMF synthesis */
  ippsQMFEncode_G722_16s(code, QMFVec, len, state->qmf_tx_delayx);

  /* ADPCM encode */
  ippsSBADPCMEncode_G722_16s(QMFVec, CodeVec, len, (IppsEncoderState_G722_16s*)state->stateEnc);

  /* Pack codeword bits:
   - [0-5 bits] - lower-band;
   - [6-7 bits] - upper-band.
  */
  PackCodeword_g722(CodeVec, len, outcode);

  return;
}

void g722NQMFEncode(G722SB_Encoder_Obj *state, short *code, char *outcode, int len)
{
  Ipp32s          i, j;
  /* Encoder variables */
  IPP_ALIGNED_ARRAY(16, Ipp16s, QMFVec, LEN_BLOCK);
  IPP_ALIGNED_ARRAY(16, Ipp16s, CodeVec, LEN_BLOCK);

  for(i=0,j=0;i<len;i++,j+=2) QMFVec[j] = QMFVec[j+1] = (Ipp16s)(code[i] >> 1);

  /* ADPCM encode */
  ippsSBADPCMEncode_G722_16s(QMFVec, CodeVec, 2*len, (IppsEncoderState_G722_16s*)state->stateEnc);

  PackCodeword_g722(CodeVec, 2*len, outcode);

  return;
}


void g722NQMFDecode(G722SB_Decoder_Obj *state, char *code, short *outcode, int len, short mode)
{
   Ipp32s     i, plc_mode;
   IPP_ALIGNED_ARRAY(16, Ipp16s, CodeVec, LEN_BLOCK);
   IPP_ALIGNED_ARRAY(16, Ipp16s, OutVec, LEN_BLOCK);

   if(code == NULL) plc_mode = 1;
   else plc_mode = 0;

   if(plc_mode) ippsZero_16s(CodeVec, LEN_BLOCK);
   else {
     /* Unpack codeword bits: [0,5] - lower-band, [6,7] - upper-band */
     UnpackCodeword_g722(code, len, CodeVec);
   }
   /* ADPCM decode */
   ippsSBADPCMDecode_G722_16s(CodeVec, OutVec, 2*len, mode, (IppsDecoderState_G722_16s*)state->stateDec);

   for(i=0; i < 2*len; i++) outcode[i] = (Ipp16s)((OutVec[i] << 1) & 0xFFFE);

   return;
}

void g722PLCDecode(G722SB_Decoder_Obj *state, char *code, short *outcode, int len, short mode)
{

  /* Auxiliary variables */
  short   i, j, k;
  short   ind_l, ind_h;
  /* Decoder arrays */
  Ipp16s *ptr_l, *ptr_h;
  IPP_ALIGNED_ARRAY(16, Ipp16s, band_code_i, LEN_BLOCK);
  IPP_ALIGNED_ARRAY(16, Ipp16s, band_code_r, LEN_BLOCK);
/* Complexity optimization : note that the qmf_rx function called by G722PLC_decode
 *                 can be replaced by the complexity optimized qmf_rx_buf function
 *                 in the same way as it is done in G722PLC_qmf_updstat.
 */

  /* shift speech buffers */
  ind_l = LEN_MEM_SPEECH - state->frameSize;
  ind_h = 160 - state->frameSize;
  ippsMove_16s(&state->mem_speech_lb[state->frameSize],state->mem_speech_lb, ind_l); /*shift 10 ms*/
  if(ind_h > 0) ippsCopy_16s(&state->mem_speech_hb[state->frameSize],state->mem_speech_hb, ind_h); /*shift 10 ms*/
  ptr_l = &(state->mem_speech_lb[ind_l]);
  ptr_h = &(state->mem_speech_hb[ind_h]);

  /* increment counter */
  if(state->count_hpf > 32000) state->count_hpf = 32000;
  state->count_hpf += len; /* upper count after loop, 32000 = 4sec*/

  /* Separate the input G722 codeword: bits 0 to 5 are the lower-band
   * portion of the encoding, and bits 6 and 7 are the upper-band
   * portion of the encoding */
  UnpackCodeword_g722((const Ipp8s*)code, len, band_code_i);

  /* Call the upper and lower band ADPCM decoders */
  ippsSBADPCMDecode_G722_16s((const Ipp16s*)band_code_i, band_code_r, (len*2), mode, state->stateDec);

  if(state->good1stFrame == 0) {      /* first good frame, crossfade is needed */
    IPP_ALIGNED_ARRAY(16, Ipp16s, lbs_code, CROSSFADELEN);
    IPP_ALIGNED_ARRAY(16, Ipp32s, lb_code1, CROSSFADELEN);
    IPP_ALIGNED_ARRAY(16, Ipp32s, lb_code2, CROSSFADELEN);
    /* cross-fade samples with PLC synthesis (in lower band only) */
    for(i = 0, j = 40; i < (CROSSFADELEN-20); i++, j+=2) lbs_code[i] = band_code_r[j];
    ippsMul_16s32s_Sfs(state->crossfade_buf, tblWeightInvFade, lb_code1, CROSSFADELEN, 0);
    ippsRShiftC_32s(lb_code1, 15, lb_code1, CROSSFADELEN);
    ippsMul_16s32s_Sfs(lbs_code, tblWeightFade, lb_code2, (CROSSFADELEN-20), 0);
    ippsRShiftC_32s(lb_code2, 15, lb_code2, (CROSSFADELEN-20));
    ippsAdd_32s_Sfs(lb_code2, (lb_code1+20), (lb_code1+20), (CROSSFADELEN-20), 0);
    for(i = 0, j = 0; i < CROSSFADELEN; i++, j+=2) band_code_r[j] = (short)lb_code1[i];
    state->good1stFrame = 1;
  }

  /* remove-DC filter */
  if(state->count_hpf <= 32000) {
     ippsFilterHighband_G722_16s_I(band_code_r, (len*2), state->mem_hpf);
     /* 4 s good frame HP filtering (offset remove) */
  }
  /* copy lower and higher band sample */
  for(k=0,j=0; k < len; k++,j+=2) {
    *ptr_l++ = band_code_r[j];
    *ptr_h++ = band_code_r[j+1];
  }
  /* Calculation of output samples from QMF filter */
  ippsQMFDecode_G722_16s(band_code_r, outcode, (len*2), state->qmf_rx_delayx);

  /* Return number of samples read */
  return;
}

void g722PLCConceal(G722SB_Decoder_Obj *state, Ipp16s* outcode)
{
  short i, j;
  short Temp, maxCorr, energy, len, idx;
  int   resetFlag, n, numZero;
  Ipp32f fnum0;
  Ipp16s *xl = NULL;
  Ipp16s *xh = NULL;
  IppG722SBClass clas;
  Ipp16s rc[ORD_LPC + 1];
  Ipp32s cor_32[ORD_LPC + 1];
  IPP_ALIGNED_ARRAY(16, Ipp16s, y, HAMWINDLEN);
  IPP_ALIGNED_ARRAY(16, Ipp16s, band_code_r, LEN_BLOCK);

  state->good1stFrame = 0;     /* reset bfi sign for cross-fading */

  len = LEN_MEM_SPEECH - state->frameSize;
  xl = &state->mem_speech_lb[len];
  if(state->prev_bfi == 0) {
      state->count_lb = 0;    /* reset counter for attenuation in lower band */
      state->count_hb = 0;    /* reset counter for attenuation in higher band */

      /**********************************
      * analyze buffer of past samples *
      * - LPC analysis
      * - pitch estimation
      * - signal classification
      **********************************/
      state->t0 = ownOpenLoopPitchSearchPLC_G722_16s((state->mem_speech_lb+LEN_MEM_SPEECH-MAXPIT2), &maxCorr);
      /* perform LPC analysis and compute residual signal */
      ippsMul_NR_16s_Sfs(&state->mem_speech_lb[LEN_MEM_SPEECH-HAMWINDLEN], G722PLC_lpc_win_80, y, HAMWINDLEN, 15);
      while(ippsAutoCorr_NormE_16s32s(y, HAMWINDLEN, cor_32, ORD_LPC+1, &n) != ippStsNoErr) {
         ippsRShiftC_16s(y, 2, y, HAMWINDLEN);
      }
      ippsLagWindow_G729_32s_I(cor_32+1,ORD_LPC); /* Lag windowing      */
      ippsLevinsonDurbin_G729_32s16s(cor_32, ORD_LPC, state->lpc, rc, &energy);
      /*if(ret_ipp = ippStsOverflow)  {
        printf("WARNING: LPC filter unstable !!!\n");
        exit(0);
      }*/
      ippsResidualFilter_G729E_16s(state->lpc, ORD_LPC, &state->mem_speech_lb[LEN_MEM_SPEECH-MAXPIT2P1],
                                   state->mem_exc_lb, MAXPIT2P1);
      for(idx=0;idx<ORD_LPC;idx++) state->mem_syn[idx] = state->mem_speech_lb[LEN_MEM_SPEECH-ORD_LPC+idx];

      /* compute zero-crossing rate in last 10 ms */
      /****************************************/
      ippsZeroCrossing_16s32f(&state->mem_speech_lb[LEN_MEM_SPEECH-81], 81, &fnum0, ippZCR);
      numZero=(int)fnum0;
      numZero = numZero >> 1;
      /************* orig code ***************
      pt1 = &mem_speech[l_mem_speech - 80 - 1];
      zcr = 0;
      for(i = 1; i< 81; i++)  {
        sum = 0;
        if(pt1[i] <= 0) sum = 1;
        if(pt1[i-1] > 0) sum++;
        zcr += sum >> 1;
      }
      ****************************************/

      if(numZero >= 20) {
        state->frameClass = IPP_G722_UNVOICED;
        if(state->t0 < 32) state->t0 = state->t0 << 1;
      } else {
          if(maxCorr > MAX_CORR_VOICED) clas = IPP_G722_VOICED;
          else clas = IPP_G722_WEAKLY_VOICED;
          ippsClassifyFrame_G722_16s_I(&state->mem_exc_lb[MAXPIT2P1-2*state->t0],
                                        state->t0, &clas, state->stateDec);
          state->frameClass = clas;
      }

      /******************************
      * synthesize missing samples *
      ******************************/
      /* set increment for attenuation */
      if(state->frameClass == IPP_G722_TRANSIENT) {
         /* attenuation in 10 ms */
         state->inc_att = 2;
         state->classWeight = (short*)tblWeight_VR;
      } else if(state->frameClass == IPP_G722_VUV_TRANSITION) {
         /* attenuation in 20 ms */
         state->inc_att = 1;
         state->classWeight = (short*)tblWeight_UV;
      } else {
         /* attenuation in 40 ms */
         state->inc_att = 0;
         state->classWeight = (short*)tblWeight_V;
      }

      /* synthesize lost frame, high band */
      xh = ownSynthesisHighbandPLC_G722_16s(state);

      ippsMove_16s(&state->mem_speech_lb[state->frameSize], state->mem_speech_lb, len); /*shift low band*/

      /* synthesize lost frame, low band directly to state->mem_speech_lb*/
      ownSynthesisLowbandPLC_G722_16s(state, xl, state->frameSize);

      /* synthesize cross-fade buffer (part of future frame)*/
      ownSynthesisLowbandPLC_G722_16s(state, state->crossfade_buf, CROSSFADELEN);

      /* attenuate outputs */
      /* mode 10 ms, first lost frame : linear weighting*/
      if(state->frameSize == 80) {
        ippsMul_NR_16s_ISfs(state->classWeight, xl, state->frameSize, 15);
        if(state->frameClass == IPP_G722_TRANSIENT) ippsZero_16s(state->crossfade_buf, CROSSFADELEN);
        else ippsMul_NR_16s_ISfs(&state->classWeight[state->frameSize], state->crossfade_buf, state->frameSize, 15);
        ippsMul_NR_16s_ISfs(state->classWeight, xh, state->frameSize, 15);

        state->count_lb +=  (state->frameSize+CROSSFADELEN) << state->inc_att;
        state->count_hb +=   CROSSFADELEN << state->inc_att;
      } else {
        ippsMul_NR_16s_ISfs(state->classWeight, xl, state->frameSize, 15);
        ippsMul_NR_16s_ISfs(&state->classWeight[state->frameSize], state->crossfade_buf, CROSSFADELEN, 15);
        ippsMul_NR_16s_ISfs(state->classWeight, xh, state->frameSize, 15);

        state->count_lb += (CROSSFADELEN+state->frameSize) << state->inc_att;
        state->count_hb +=  state->frameSize << state->inc_att;
        if(state->count_lb > END_3RD_PART) state->count_lb = END_3RD_PART;
        if(state->count_hb > END_3RD_PART) state->count_hb = END_3RD_PART;
      }
    } else {
      ippsMove_16s(&state->mem_speech_lb[state->frameSize], state->mem_speech_lb, len);
      /* copy samples from cross-fading buffer (already generated in previous bad frame decoding)  */
      ippsCopy_16s(state->crossfade_buf, xl, CROSSFADELEN);

      /* synthesize rest of lost frame */
      Temp = state->frameSize - CROSSFADELEN;
      if(Temp > 0) {
        ownSynthesisLowbandPLC_G722_16s(state, &xl[CROSSFADELEN], Temp);/* remaining synthese for lost frame*/
        if(state->count_lb >= END_3RD_PART) ippsZero_16s(&xl[CROSSFADELEN], Temp);
        else {
          idx = state->count_lb >> state->inc_att;
          ippsMul_NR_16s_ISfs(&state->classWeight[idx], &xl[CROSSFADELEN], Temp, 15);

          state->count_lb += Temp << state->inc_att;
          if(state->count_lb > END_3RD_PART) state->count_lb = END_3RD_PART;
        }
      }

      /* synthesize cross-fade buffer (part of future frame) and attenuate output */
      ownSynthesisLowbandPLC_G722_16s(state, state->crossfade_buf, CROSSFADELEN);
      if(state->count_lb >= END_3RD_PART) ippsZero_16s(state->crossfade_buf, CROSSFADELEN);
      else {
        idx = state->count_lb >> state->inc_att;
        ippsMul_NR_16s_ISfs(&state->classWeight[idx], state->crossfade_buf, CROSSFADELEN, 15);

        state->count_lb +=  CROSSFADELEN << state->inc_att;
        if(state->count_lb > END_3RD_PART) state->count_lb = END_3RD_PART;
      }

      xh = ownSynthesisHighbandPLC_G722_16s(state);
      if(state->count_hb >= END_3RD_PART) ippsZero_16s(xh, state->frameSize);
      else {
        idx = state->count_hb >> state->inc_att;
        ippsMul_NR_16s_ISfs(&state->classWeight[idx], xh, state->frameSize, 15);

        state->count_hb +=  state->frameSize << state->inc_att;
        if(state->count_hb > END_3RD_PART) state->count_hb = END_3RD_PART;
      }
  }

  /************************
   * generate higher band *
   ************************/
  /* reset DC-remove filter states for good frame decoding */
  if(state->count_hpf >= 32000) {/* 4 s good frame HP filtering (offset remove) */
    state->mem_hpf[0] = 0;
    state->mem_hpf[1] = 0;
  }
  state->count_hpf = 0;                 /* reset counter used to activate DC-remove filter in good frame mode  */

  /*****************************************
   * QMF synthesis filter and state update *
   *****************************************/
  for(i = 0, j = 0; i < state->frameSize; i++,j+=2) {
    band_code_r[j] = xl[i];
    band_code_r[j+1] = xh[i];
  }
  /* filter higher-band */
  ippsFilterHighband_G722_16s_I(band_code_r, (state->frameSize*2), state->mem_hpf);
  /* calculate output samples of QMF filter */
  ippsQMFDecode_G722_16s(band_code_r, outcode, (state->frameSize*2), state->qmf_rx_delayx);

  if(state->count_hb > 160) resetFlag = 1;
  else resetFlag = 0;
  ippsSBADPCMDecodeStateUpdate_G722_16s((const Ipp16s*)&state->mem_speech_lb[LEN_MEM_SPEECH-2],
                                 state->crossfade_buf[0], resetFlag, state->stateDec);

  return;
}


