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
// Purpose: G.729/A/B/D/E/EV speech codec: encoder API functions.
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <ippsc.h>
#include <ipps.h>
#include "owng7291.h"

static void CELP2S_InitEncoder(G729_EncoderState * pCodStat, int rate);
static void CELP2S_encoder(G729_EncoderState * pCodStat, short * DataIn, short * coder_parameters,
                    short *pSynth, int SubFrNum, short * celp_Az, short * sync_speech,
                    short * resBsFreqk, short * pit, short *delay,
                    short * voicing, short * ee, short * diff);
static void TDBWE_encoder(short *pMemTDBWESpeech, short *input_hi, short *coder_parameters,
                   short *input_hi_delay);
static void TDAC_encoder(short *y_lo, short *y_hi, short *pPrms, short *nbit_env, unsigned int *pVQPrms,
                  short *vqnbit_env, int nbit, int nbit_max, int norm_lo, int norm_hi, int norm_MDCT);
static void ownPrms2Bits(const short* pPrms, const short* nbit_env,
                         const unsigned int* pvqPrms, const short* vqnbit_env,
                         unsigned char *pBitstream, int rate, int nb_bits_used);

/* 50 Hz high-pass filter coefficients (preprocessing of lower band at 8kHz) Q14*/
static __ALIGN32 CONST short hp50_b[3] = { (short) 15655, (short) - 31310, (short) 15655 };
static __ALIGN32 CONST short hp50_a[3] = { (short) 16384, (short) 32219, (short) - 15846 };

int apiG7291Encoder_Alloc(G729Codec_Type codecType, int *pCodecSize)
{
    int fltSize;
    if(codecType != G7291_CODEC) {
        return 1;
    }
    *pCodecSize = (sizeof(G7291Encoder_Obj)+7)&(~7);
    ippsQMFGetStateSize_G7291_16s(&fltSize);
    *pCodecSize += fltSize;
    ippsFilterHighpassGetStateSize_G7291_16s(&fltSize);
    *pCodecSize += fltSize;
//    ippsLowPassFilterStateSize_G7291_16s(&fltSize);
//    *pCodecSize += fltSize;
    return 0;
}

 /*  Encoder Initializations */
int apiG7291Encoder_Init(G7291Encoder_Obj *codstat, int rate, int freq)
{
  Ipp16s abHP[6];
  int fltSize, i;
  if(codstat == NULL) return 1;

  ippsZero_8u((Ipp8u*)codstat, (sizeof(*codstat)+7)&(~7));

  /* initialize QMF filterbank */
  codstat->pMemQMFana = (IppsQMFState_G7291_16s*)((char*)codstat + ((sizeof(G7291Encoder_Obj)+7)&(~7)));
  ippsQMFInit_G7291_16s(codstat->pMemQMFana);
  ippsQMFGetStateSize_G7291_16s(&fltSize);

  /* initialize preprocessing of lower and higher bands */
  codstat->pMemHP50 = (IppsFilterHighpassState_G7291_16s*)((char*)codstat->pMemQMFana + fltSize);
  abHP[0] = hp50_a[0]; abHP[1] = hp50_a[1]; abHP[2] = hp50_a[2];
  abHP[3] = hp50_b[0]; abHP[4] = hp50_b[1]; abHP[5] = hp50_b[2];
  ippsFilterHighpassInit_G7291_16s(codstat->pMemHP50, abHP);
  ippsFilterHighpassGetStateSize_G7291_16s(&fltSize);

//  codstat->pMemLP3k = (IppsLowPassFilterState_G7291_16s*)((char*)codstat->pMemHP50 + fltSize);
//  ippsLowPassFilterInit_G7291_16s(abLP3k, codstat->pMemLP3k);
//  ippsLowPassFilterStateSize_G7291_16s(&fltSize);
  ippsZero_8u((Ipp8u*)codstat->pMemLP3k, 24);

  /* set the flag for 8kHz sampled input */
  codstat->freq = freq;

  /* initialize memories for MDCT computation */
  ippsZero_16s(codstat->pMDCTCoeffsLow, G7291_LFRAME2);
  ippsZero_16s(codstat->pMDCTCoeffsHigh, G7291_LFRAME2);

  /* initialize G729 encoder */
  CELP2S_InitEncoder(&codstat->pG729State, rate);

  /* initialize FEC variables */
  codstat->sFERClassOld = G7291_FEC_UNVOICED;
  codstat->sLPSpeechEnergu = (256 * 55);

  codstat->sRelEnergyMem = 0;
  codstat->sRelEnergyVar = 0;
  codstat->sPosPulseOld = 0;

  /* initialize TDBWE encoder */
  ippsZero_16s(codstat->pMemTDBWESpeech, G7291_TDBWE_MEM_SPEECH);

  /* perceptual weighting filter of lower band difference signal */
  for (i=0; i<G7291_LP_ORDER; i++) {
      codstat->pWeightFltInput[i]=0;
      codstat->pWeightFltOutput[i]=0;
  }
  return 0;
}

int apiG7291Encode(G7291Encoder_Obj *codstat, const short *samples_in,
                   unsigned char *itu_192_bitstream, int rate)
{
  IPP_ALIGNED_ARRAY(16, short, input_lo, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, input_hi, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, input_hi_delay, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, celp_output, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, diff, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, y_lo, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, y_hi, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, celp_Az, G7291_NB_SUBFR * G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, sync_speech, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, resBsFreqk, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, coder_parameters, G7291_PRM_SIZE_MAX * 2 + 10 + G7291_TDAC_NB_SB+2);
  IPP_ALIGNED_ARRAY(16, short, nbit_env, G7291_TDAC_NB_SB+2);
  IPP_ALIGNED_ARRAY(16, short, vqnbit_env, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, unsigned int, vqcoder_parameters, G7291_TDAC_NB_SB);
  short voicing[6], pit[4], ee[2], delay[2];
  short *pPrms;
  unsigned int *pVQPrms;
  int i, j;
  int nb_bits_max;        /* maximal bit allocation (-> 32 kbps) */
  int nb_bits_left;       /* number of bits left */
  int nb_bits_used;       /* number of bits used */
  int norm_MDCT, norm_hi, norm_lo;

  if(NULL==codstat || NULL==itu_192_bitstream || NULL ==samples_in)
        return APIG729_StsBadArgErr;

  /* set number of bits for 20-ms frame */
  nb_bits_max = G7291_MAX_BITRATE / 50;  /* maximal bit allocation (-> 32 kbit/s) */
  nb_bits_left = (rate / 50);            /* actual bit allocation  (-> actual encoding bit rate) */

  /* initialize number of bits */
  nb_bits_used = 0;
  pPrms = coder_parameters;
  pVQPrms = vqcoder_parameters;
  ippsZero_16s(coder_parameters, G7291_PRM_SIZE_MAX * 2 + 10 + G7291_TDAC_NB_SB+2);
  for (i=0; i<G7291_TDAC_NB_SB; i++) vqcoder_parameters[i]=0;

  /* buffering */
  if(codstat->freq == 16000)
  {
    ippsQMFEncode_G7291_16s(samples_in, G7291_LFRAME, input_lo, input_hi, codstat->pMemQMFana);
  } else
  {
    ippsRShiftC_16s(samples_in, 1, input_lo, G7291_LFRAME2);
    ippsZero_16s(input_hi, G7291_LFRAME2);
  }

  /* preprocessing of lower band (50 Hz high-pass) */
  ippsFilterHighpass_G7291_16s_ISfs(input_lo, G7291_LFRAME2, codstat->pMemHP50, 0);

  for(i=0, j=0; i<G7291_LFRAME2; i+=G7291_LFRAME4, j++)
  {
    /* cascade CELP (10 ms frame) */
    CELP2S_encoder(&codstat->pG729State, &input_lo[i], pPrms,
        &celp_output[i], j, celp_Az, &sync_speech[i], &resBsFreqk[i],
        &(pit[2*j]), delay, &voicing[j*3], &ee[j], &diff[i]);

    if (rate >= 12000) nb_bits_used += 120;
    else nb_bits_used += 80;
    pPrms += G7291_PRM_SIZE_MAX;
  }
  nb_bits_left -= nb_bits_used;

  /* perform time-domain BWE */
  if (rate >= 14000)
  {
    if(codstat->freq == 16000)
    {
      /* preprocessing of higher band (3 kHz low-pass filter) */
#ifdef _FAST_G7291
      ippsFilterLowpass_G7291_16s_I(input_hi, G7291_LFRAME2, codstat->pMemLP3k, ippAlgHintFast);
#else
      ippsFilterLowpass_G7291_16s_I(input_hi, G7291_LFRAME2, codstat->pMemLP3k, ippAlgHintAccurate);
#endif
      /* TDBWE encoder */
      TDBWE_encoder(codstat->pMemTDBWESpeech, input_hi, pPrms, input_hi_delay);
    }
    else
    {
      pPrms[0] = 28;
      pPrms[1] = 71;
      pPrms[2] = 71;
      pPrms[3] = 30;
      pPrms[4] = 26;
      pPrms[5] = 1;
    }
    nb_bits_used += 40;
    nb_bits_left -= 40;
  }
  pPrms += 6;         /* 6 parameters in TDBWE */

  /* FEC Encoder */
  if(rate >= 12000)
  {
    /* Should compute FER parameters of next frame here; before lag */
    ownEncode_FEC(codstat->sFERClassOld, voicing, input_lo, celp_output, pit, ee,
        sync_speech, &codstat->sFERClassOld, pPrms, delay, resBsFreqk,
        &(codstat->sLPSpeechEnergu), &codstat->sRelEnergyMem, &codstat->sRelEnergyVar,
        &(codstat->sPosPulseOld));
    pPrms += 3;         /* 3 parameters in FEC */
  }

  /* Encoding of full-band spectrum: 16-32k enhancement layers */
  if(rate >= 16000)
  {
    /* apply perceptual weighting filter */
    ownWeightingFilterFwd_G7291_16s(celp_Az, codstat->pWeightFltInput, codstat->pWeightFltOutput, diff);
    ippsLShiftC_16s(diff, 1, diff, G7291_LFRAME2);

    /* perform MDCT of difference signal in lower band */
    {
#ifdef _FAST_G7291
       IppHintAlgorithm hint = ippAlgHintFast;
#else
       IppHintAlgorithm hint = ippAlgHintAccurate;
#endif
       ippsMDCTFwd_G7291_16s(diff, codstat->pMDCTCoeffsLow,
          &norm_lo, y_lo, hint);
    }
    ippsCopy_16s(diff,codstat->pMDCTCoeffsLow,G7291_LFRAME2);

    /* compute MDCT of higher band */
    if(codstat->freq == 16000)
    {

#ifdef _FAST_G7291
      IppHintAlgorithm hint = ippAlgHintFast;
#else
      IppHintAlgorithm hint = ippAlgHintAccurate;
#endif
      ippsMDCTFwd_G7291_16s(input_hi_delay, codstat->pMDCTCoeffsHigh,
         &norm_hi, y_hi, hint);
      ippsCopy_16s(input_hi_delay,codstat->pMDCTCoeffsHigh,G7291_LFRAME2);
    }
    else
    {
      norm_hi = norm_lo;
      ippsZero_16s(y_hi, G7291_LFRAME2);
    }

    if(norm_hi > norm_lo)
        norm_MDCT = norm_lo;
    else
        norm_MDCT = norm_hi;

    nb_bits_max -= (nb_bits_used + 9);
    nb_bits_left -= 9;
    if (nb_bits_left < 0) nb_bits_left = 0;
    *pPrms++ = (short)norm_MDCT;

    /* apply TDAC encoder */
    TDAC_encoder(y_lo, y_hi, pPrms, nbit_env, pVQPrms, vqnbit_env, nb_bits_left, nb_bits_max, norm_lo,
                        norm_hi, norm_MDCT);
    nb_bits_used += (nb_bits_left + 9);
  }

  /* Create itu_192_bitstream */
  ownPrms2Bits(coder_parameters, nbit_env, vqcoder_parameters, vqnbit_env, itu_192_bitstream, rate, nb_bits_used);
  return 0;
}

 /*   Variables initialization for the G.729 coder  */
void CELP2S_InitEncoder(G729_EncoderState *pCodStat, int rate)
{
  int i, j;

  /* Static vectors to zero */
  ippsZero_16s(pCodStat->pSpeechVector, G7291_MEM_SPEECH);
  ippsZero_16s(pCodStat->pExcVector, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  ippsZero_16s(pCodStat->pWgtSpchVector, G7291_PITCH_LAG_MAX);

  for (i=0; i<G7291_LP_ORDER; i++) {
      pCodStat->pSynthMem[i]=0;
      pCodStat->pSynthMem0[i]=0;
      pCodStat->pSynthMem1[i]=0;
      pCodStat->pSynthMem2[i]=0;
      pCodStat->pSynthMem3[i]=0;
      pCodStat->pSynthMemErr1[i]=0;
      pCodStat->pSynthMemErr2[i]=0;
  }
  pCodStat->rate = rate;
  pCodStat->sPitchSharp = G7291_SHARPMIN;

  /* Initialize pPastQntEnergy */
  pCodStat->pPastQntEnergy[0] = (-14336);
  pCodStat->pPastQntEnergy[1] = (-14336);
  pCodStat->pPastQntEnergy[2] = (-14336);
  pCodStat->pPastQntEnergy[3] = (-14336);

  /* Initialize pLSPVector[] */
  pCodStat->pLSPVector[0] = 30000;
  pCodStat->pLSPVector[1] = 26000;
  pCodStat->pLSPVector[2] = 21000;
  pCodStat->pLSPVector[3] = 15000;
  pCodStat->pLSPVector[4] = 8000;
  pCodStat->pLSPVector[5] = 0;
  pCodStat->pLSPVector[6] = (-8000);
  pCodStat->pLSPVector[7] = (-15000);
  pCodStat->pLSPVector[8] = (-21000);
  pCodStat->pLSPVector[9] = (-26000);

  /* Initialize pLSPQntVector[] */
  for (i=0; i<G7291_LP_ORDER; i++) pCodStat->pLSPQntVector[i]=pCodStat->pLSPVector[i];
  for (i=0; i<G7291_MA_NP; i++)
    for (j=0; j<G7291_LP_ORDER; j++) {
      pCodStat->pPrevLSFVector[i][j]=FreqPrevResetTbl[j];
    }
  for(i=0; i<4; i++) pCodStat->L_exc_err[i] = 0x00004000;
  pCodStat->sSmooth = 1;
  pCodStat->pLARMem[0] = (short) 0;
  pCodStat->pLARMem[1] = (short) 0;
  pCodStat->pLPCCoeffs[0] = 4096;
  for (i=1; i<G7291_G729_MP1; i++) pCodStat->pLPCCoeffs[i]=0;
  pCodStat->pReflectCoeffs[0] = 0;
  pCodStat->pReflectCoeffs[1] = 0;
  pCodStat->sPitch = 40;

  return;
}

void CELP2S_encoder(
G729_EncoderState *pCodStat, /* (i)  core coder states vaiables  */
short *DataIn,               /* (i)  data to encode              */
short *coder_parameters,     /* (o)  coder parameters            */
short *pSynth,               /* (o)  Synthesis signal            */
int   SubFrNum,              /* (i)  Subframe number             */
short *celp_Az,              /* (i)  Az buffer                   */
short *sync_speech,          /* (o)  weighted synthesis          */
short *resBsFreqk,           /* (o)  residual signal frame       */
short *pit,                  /* (o)  open loop pitch             */
short *delay,                /* (o)  close loop integer and fract. pitch    */
short *voicing,              /* (o)  frame voicing               */
short *ee,                   /* (o)  spectral tilt info.         */
short *diff)                 /* (o)  difference signal between
                                     input and local synthesis at
                                     12 kbit/s */
{
  IPP_ALIGNED_ARRAY(16, int, autoR, G7291_G729_MP1);
  IPP_ALIGNED_ARRAY(16, short, rc, G7291_LP_ORDER); /* Reflection coefficients. */
  IPP_ALIGNED_ARRAY(16, short, A_t, (G7291_G729_MP1)*2); /* A(z) unquantized for the 2 subframes */
  IPP_ALIGNED_ARRAY(16, short, Aq_t,(G7291_G729_MP1)*2); /* A(z) quantized for the 2 subframes */
  IPP_ALIGNED_ARRAY(16, short, Ap1, (G7291_G729_MP1)*2); /* A(z) with spectral expansion */
  IPP_ALIGNED_ARRAY(16, short, lsp_new, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, lsp_new_q, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, lsf_int, G7291_LP_ORDER);  /* Interpolated LSF 1st subframe */
  IPP_ALIGNED_ARRAY(16, short, lsf_new, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, pSynthBuffer, G7291_LFRAME4); /* Synthesis signal */
  IPP_ALIGNED_ARRAY(16, short, pWgtSpchVector, G7291_LFRAME4 + G7291_PITCH_LAG_MAX);
  IPP_ALIGNED_ARRAY(16, short, pExcVector, G7291_LFRAME4 + G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  IPP_ALIGNED_ARRAY(16, short, ai_zero, G7291_LSUBFR + G7291_M_LPC1);
  IPP_ALIGNED_ARRAY(16, short, h1, G7291_LSUBFR);  /* Impulse response h1[]               */
  IPP_ALIGNED_ARRAY(16, short, xn, G7291_LSUBFR);  /* Target vector for pitch search      */
  IPP_ALIGNED_ARRAY(16, short, xn2, G7291_LSUBFR); /* Target vector for codebook search   */
  IPP_ALIGNED_ARRAY(16, short, code, G7291_LSUBFR);/* Fixed codebook excitation           */
  IPP_ALIGNED_ARRAY(16, short, y1, G7291_LSUBFR);  /* Filtered adaptive excitation        */
  IPP_ALIGNED_ARRAY(16, short, y2, G7291_LSUBFR);  /* Filtered fixed codebook excitation  */
  IPP_ALIGNED_ARRAY(16, short, pSpeechVector, G7291_L_TOTAL);

  IPP_ALIGNED_ARRAY(16, short, exc2, G7291_LFRAME4);
  IPP_ALIGNED_ARRAY(16, short, pSynthMemErr1, G7291_M_LPC + G7291_LSUBFR); /* Filter memory */
  IPP_ALIGNED_ARRAY(16, short, pSynthMemErr2, G7291_M_LPC + G7291_LSUBFR);
  IPP_ALIGNED_ARRAY(16, short, res1, G7291_LSUBFR);

  IPP_ALIGNED_ARRAY(16, short, yVal, G7291_L_WINDOW);
  IPP_ALIGNED_ARRAY(16, short, tmpvec, G7291_LP_ORDER);
  IPP_ALIGNED_ARRAY(16, short, PWGammaFactorMem, G7291_LP_ORDER);

  short *A, *Aq, *Ap2;       /* Pointers on A_t and Aq_t         */
  short *ana, *ana2;         /* Pointers on analysis parameters */
  short *synth_ptr;          /* Pointer on synthesis buffer */
  short *new_speech;         /* Pointer on new speech */
  short *speech;             /* Pointer on speech */
  short *p_window;           /* Pointer on p_window; */
  short *wsp;                /* Pointer on pWgtSpchVector */
  short *exc;                /* Pointer on pExcVector, exitation vector */
  short *error;              /* Pointers on pSynthMemErr1 filter's memory */

  short gamma1[2], gamma2[2];/* Weighting factor for the 2 subframes  */
  short indexFC[2];
  short gains[2];
  short gInd[2];
  short LAR[4];

  int i, j, L_temp;
  int indexSubfr, i0;
  short T0_min, T0_max;
  short sGainPitch, sGainCode, index;
  short temp;
  short voice_fac = 0;

  /* Pointeurs initialization */
  new_speech = pSpeechVector + G7291_MEM_SPEECH;             /* New speech     */
  p_window = pSpeechVector + G7291_L_TOTAL - G7291_L_WINDOW; /* For LPC window */
  speech = new_speech - G7291_L_NEXT;                        /* Present frame  */
  synth_ptr = pSynthBuffer;
  wsp = &pWgtSpchVector[G7291_PITCH_LAG_MAX];
  exc = &pExcVector[G7291_PITCH_LAG_MAX+G7291_L_INTERPOL];
  error = &pSynthMemErr1[G7291_LP_ORDER];
  ana = coder_parameters;
  ana2 = &coder_parameters[G7291_PRM_SIZE];
  Ap2 = &Ap1[G7291_G729_MP1];

  ippsCopy_16s(pCodStat->pSpeechVector, pSpeechVector, G7291_MEM_SPEECH);
  ippsCopy_16s(DataIn, new_speech, G7291_LFRAME4);
  ippsCopy_16s(pCodStat->pWgtSpchVector, pWgtSpchVector, G7291_PITCH_LAG_MAX);
  ippsCopy_16s(pCodStat->pExcVector, pExcVector, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  ippsZero_16s(ai_zero, (G7291_LSUBFR + G7291_M_LPC1));
  for (i=0; i<G7291_M_LPC; i++) {
      pSynthMemErr1[i]=pCodStat->pSynthMemErr1[i];
      pSynthMemErr2[i]=pCodStat->pSynthMemErr2[i];
  }

  /* LP analysis */
  ippsMul_NR_16s_Sfs(p_window, HammingWindowTbl, yVal, G7291_L_WINDOW, 15);
  while(ippsAutoCorr_NormE_16s32s(yVal, G7291_L_WINDOW, autoR, G7291_G729_MP1,
      &i)!=ippStsNoErr) {
    ippsRShiftC_16s(yVal, 2, yVal, G7291_L_WINDOW);
  }
  ippsLagWindow_G729_32s_I(autoR+1, G7291_LP_ORDER);
  if(ippsLevinsonDurbin_G729_32s16s(autoR, G7291_LP_ORDER, &A_t[G7291_G729_MP1], rc, &temp ) == ippStsOverflow) {
      for(i=0; i <= G7291_LP_ORDER; i++) {
        A_t[G7291_G729_MP1+i] = pCodStat->pLPCCoeffs[i];
      }
      rc[0] = pCodStat->pReflectCoeffs[0];
      rc[1] = pCodStat->pReflectCoeffs[1];
  } else {
      for(i=0; i <= G7291_LP_ORDER; i++) {
        pCodStat->pLPCCoeffs[i] = A_t[G7291_G729_MP1+i];
      }
      pCodStat->pReflectCoeffs[0] = rc[0];
      pCodStat->pReflectCoeffs[1] = rc[1];
  }
  *ee = (-rc[0])>>1;

  ippsLPCToLSP_G729_16s(&A_t[G7291_G729_MP1], pCodStat->pLSPVector, lsp_new);
  /* LSP quantization */
  ippsLSPQuant_G729_16s(lsp_new,(Ipp16s*)pCodStat->pPrevLSFVector, lsp_new_q, ana);
  ana += 2;

  /* Find interpolated LPC parameters in all subframes (both quantized and unquantized) */
  ippsInterpolate_G729_16s(lsp_new, pCodStat->pLSPVector, tmpvec, G7291_LP_ORDER);
  ippsLSPToLPC_G729_16s(tmpvec, A_t);
  ippsLSPToLSF_Norm_G729_16s(tmpvec, lsf_int);
  ippsLSPToLSF_Norm_G729_16s(lsp_new, lsf_new);

  ippsInterpolate_G729_16s(lsp_new_q, pCodStat->pLSPQntVector, tmpvec, G7291_LP_ORDER);
  ippsLSPToLPC_G729_16s(tmpvec, Aq_t);
  ippsLSPToLPC_G729_16s(lsp_new_q, &Aq_t[G7291_G729_MP1]);

  ippsCopy_16s(Aq_t, &celp_Az[2 * SubFrNum * G7291_G729_MP1], 2 * G7291_G729_MP1);

  /* update the LSPs for the next frame */
  for (i=0; i<G7291_LP_ORDER; i++) {
      pCodStat->pLSPVector[i]=lsp_new[i];
      pCodStat->pLSPQntVector[i]=lsp_new_q[i];
  }
  /* - Find the weighting factors */
  _ippsRCToLAR_G7291_16s(rc, &LAR[2], 2);
  temp = (short)(pCodStat->pLARMem[0] + LAR[2]);
  LAR[0] = (short)((temp<0)? ~(( ~temp) >> 1 ) : temp>>1);
  temp = (short)(pCodStat->pLARMem[1] + LAR[3]);
  LAR[1] = (short)((temp<0)? ~(( ~temp) >> 1 ) : temp>>1);
  pCodStat->pLARMem[0] = LAR[2];
  pCodStat->pLARMem[1] = LAR[3];
  _ippsPWGammaFactor_G7291_16s(LAR, lsf_int, &pCodStat->sSmooth, gamma1, gamma2, PWGammaFactorMem);
  _ippsPWGammaFactor_G7291_16s(LAR+2, lsf_new, &pCodStat->sSmooth, gamma1+1, gamma2+1, PWGammaFactorMem);

  /* - Find the weighted input speech w_sp[] for the whole speech frame
     - Find the open-loop pitch delay */
  ippsMulPowerC_NR_16s_Sfs(&A_t[0],gamma1[0], Ap1, G7291_G729_MP1, 15);
  ippsMulPowerC_NR_16s_Sfs(&A_t[0],gamma2[0], Ap2, G7291_G729_MP1, 15);
  ippsIIR16sLow_G729_16s(Ap1, &speech[-G7291_LP_ORDER], wsp, pCodStat->pSynthMem1);

  ippsMulPowerC_NR_16s_Sfs(&A_t[G7291_G729_MP1], gamma1[1], Ap1, G7291_G729_MP1, 15);
  ippsMulPowerC_NR_16s_Sfs(&A_t[G7291_G729_MP1], gamma2[1], Ap2, G7291_G729_MP1, 15);
  ippsIIR16sLow_G729_16s(Ap1, &speech[G7291_LSUBFR-G7291_LP_ORDER], &wsp[G7291_LSUBFR], pCodStat->pSynthMem1);

  /* Find open loop pitch lag */
  ownOpenLoopPitchSearch_G7291_16s(wsp, voicing, &(pCodStat->sPitch));

  /* Range for closed loop pitch search in 1st subframe */
  T0_min = pCodStat->sPitch - 3;
  if (T0_min < G7291_PITCH_LAG_MIN) T0_min = G7291_PITCH_LAG_MIN;

  T0_max = T0_min + 6;
  if(T0_max > G7291_PITCH_LAG_MAX)
  {
    T0_max = G7291_PITCH_LAG_MAX;
    T0_min = T0_max - 6;
  }

  A = A_t;                      /* pointer to interpolated LPC parameters           */
  Aq = Aq_t;                    /* pointer to interpolated quantized LPC parameters */

  for(i0 = 0; i0 < G7291_LFRAME4 / G7291_LSUBFR; i0++)
  {
    indexSubfr = i0 * G7291_LSUBFR;

    /* Find the weighted LPC coefficients for the weighting filter */
    ippsMulPowerC_NR_16s_Sfs(A, gamma1[i0], Ap1, G7291_G729_MP1, 15);
    ippsMulPowerC_NR_16s_Sfs(A, gamma2[i0], Ap2, G7291_G729_MP1, 15);

    /* Compute impulse response, h1[], of weighted synthesis filter  */
    for (i=0; i<G7291_G729_MP1; i++) ai_zero[i]=Ap1[i];
    ippsSynthesisFilter_NR_16s_Sfs(Aq, ai_zero, h1, G7291_LSUBFR, 12, NULL);
    ippsSynthesisFilterLow_NR_16s_ISfs(Ap2,h1,G7291_LSUBFR,12,NULL);
    /* ILBC is some faster, can be used without accuracy losses [AM] */
    ippsResidualFilter_AMRWB_16s_Sfs(Aq, G7291_LP_ORDER, &speech[indexSubfr], &exc[indexSubfr], G7291_LSUBFR, 10); 
    /*ippsResidualFilter_ILBC_16s_Sfs(&speech[indexSubfr], Aq, &exc[indexSubfr], G7291_LSUBFR, 11, &speech[indexSubfr-G7291_LP_ORDER]);*/

    if(pCodStat->rate >= 12000)
    {
      /* Residual will be needed to compute the error signal in the residual domain */
      ippsCopy_16s(&exc[indexSubfr], &exc2[indexSubfr], G7291_LSUBFR);
    }
    Aq[0] >>= 1;
    Ap2[0] >>= 1;
    ippsSynthesisFilter_NR_16s_Sfs(Aq, &exc[indexSubfr], error, G7291_LSUBFR, 12, pSynthMemErr1);
    /* ILBC is some faster, can be used without accuracy losses [AM] */
    ippsResidualFilter_AMRWB_16s_Sfs(Ap1, G7291_LP_ORDER, error, xn, G7291_LSUBFR, 10); 
    /*ippsResidualFilter_ILBC_16s_Sfs(error, Ap1, xn, G7291_LSUBFR, 11, &error[-G7291_LP_ORDER]);*/


    /* xn is in Q1 prior to the synthesis filter and in Q0 after... */
    ippsSynthesisFilterLow_NR_16s_ISfs(Ap2,xn,G7291_LSUBFR,12,pCodStat->pSynthMem0);

    /* Save residual signal (will be needed to compute second second stage target */
    ippsCopy_16s(&exc[indexSubfr], res1, G7291_LSUBFR);

    /* Closed-loop fractional pitch search */
    ippsAdaptiveCodebookSearch_G7291_16s(xn, h1, &exc[indexSubfr], delay, T0_min, T0_max, indexSubfr);
    if (indexSubfr == 0)
    {
      if(delay[0] <= 85)
        index = (delay[0] * 3 - 58) + delay[1];
      else
        index = delay[0] + 112;

      T0_min = delay[0] - 5;
      if (T0_min < G7291_PITCH_LAG_MIN)
        T0_min = G7291_PITCH_LAG_MIN;

      T0_max = T0_min + 9;
      if (T0_max > G7291_PITCH_LAG_MAX)
      {
        T0_max = G7291_PITCH_LAG_MAX;
        T0_min = T0_max - 9;
      }
    }
    else
      index = (delay[0] - T0_min) * 3 + 2 + delay[1];

    *ana++ = index;
    if(indexSubfr == 0) {
       *ana++ = (Ipp16s)equality(index);
    }
    *pit++ = delay[0];

    /*   - find unity gain pitch excitation (adaptive codebook entry)
          with fractional interpolation.
        - find filtered pitch exc. y1[]=exc[] convolve with h1[])
        - compute pitch gain and limit between 0 and 1.2
        - update target vector for codebook search
        - find LTP residual. */
    /*** ippsAdaptiveCodebookDecode_G7291_16s_I(delay, &exc[indexSubfr]); ***/
    /* Can be replaced by (without the loss of accuracy) [AM] */
    ippsDecodeAdaptiveVector_G729_16s_I(delay, &exc[indexSubfr-G7291_FEC_L_EXC_MEM]);

    ippsAdaptiveCodebookGain_G7291_16s(xn, h1, &exc[indexSubfr], y1, &sGainPitch);
    if(delay[1] > 0) {
       i = delay[0] + 1;
    } else {
       i = delay[0];
    }
    temp = calcErr_G7291(i, pCodStat->L_exc_err);
    if((temp == 1) && (sGainPitch > G7291_GPCLIP)) sGainPitch = G7291_GPCLIP;

    ippsAdaptiveCodebookContribution_G729_16s(sGainPitch, y1, xn, xn2);

    /* - Innovative codebook search. */
    {
      IPP_ALIGNED_ARRAY(16, short, h, G7291_LSUBFR);
      ippsCopy_16s(h1, h, G7291_LSUBFR);

      /* Compute residual after long term prediction */
      ippsInterpolateC_NR_G729_16s_Sfs(res1, 16384, &exc[indexSubfr], -sGainPitch, res1, G7291_LSUBFR, 14);

      ippsAlgebraicCodebookSearchL1_G7291_16s(xn2, res1, y1, h, delay[0], pCodStat->sPitchSharp, code, y2, indexFC);
      *ana++ = indexFC[1];           /* Positions index */
      *ana++ = indexFC[0];           /* Signs index     */
    }

    /* - Quantization of gains. */
    ippsGainQuant_G729_16s(xn, y1, code, y2, pCodStat->pPastQntEnergy, gains, gInd, temp);
    sGainPitch  = gains[0];
    sGainCode = gains[1];
    *ana++ = (short)((LUT1Tbl[gInd[0]]<<4) | LUT2Tbl[gInd[1]]);

    if(pCodStat->rate >= 12000)
    {
        voice_fac = ownFindVoiceFactor(&exc[indexSubfr], gains, code);
        voice_fac = (short)((voice_fac + 32768) >> 1);
    }

   /* - Find the total excitation
      - find synthesis speech corresponding to exc[]
      - update filters memories for finding the target
        vector in the next subframe
        (update error[-m..-1] and mem_w_err[])
        update error function for taming process */

    if(sGainCode <= IPP_MAX_16S/2)
    {
        ippsInterpolateC_NR_G729_16s_Sfs(&exc[indexSubfr],sGainPitch,code,sGainCode<<1,&exc[indexSubfr],G7291_LSUBFR,14);
    }
    else
    {
        for (i = 0; i < G7291_LSUBFR; i++)
        {
            L_temp = code[i]*(sGainCode<<1) + exc[i+indexSubfr]*sGainPitch;
            exc[indexSubfr+i] = Cnvrt_32s16s((L_temp + 0x2000) >> 14);
        }
    }
    //ippsInterpolateC_NR_G729_16s_Sfs(&exc[indexSubfr],sGainPitch,code,sGainCode<<1,&exc[indexSubfr],G7291_LSUBFR,14);

    ippsSynthesisFilter_NR_16s_Sfs(Aq, &exc[indexSubfr], &synth_ptr[indexSubfr], G7291_LSUBFR, 12, pCodStat->pSynthMem);
    for (i=0; i<G7291_LP_ORDER; i++) {
        pCodStat->pSynthMem[i]=synth_ptr[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
    }

    /* Addition of a second fixed codebook (12Kbit/s mode)   */
    if(pCodStat->rate >= 12000)
    {
      IPP_ALIGNED_ARRAY(16, short, res2, G7291_LSUBFR);
      IPP_ALIGNED_ARRAY(16, short, y2_enha, G7291_LSUBFR);   /* Filtered fixed codebook excitation */
      IPP_ALIGNED_ARRAY(16, short, code_enha, G7291_LSUBFR); /* Fixed codebook excitation          */
      IPP_ALIGNED_ARRAY(16, short, exc_enha, G7291_LFRAME4);
      IPP_ALIGNED_ARRAY(16, short, xn_enha, G7291_LSUBFR);   /* Target vector for codebook search  */
      short *error2 = &pSynthMemErr2[G7291_LP_ORDER];        /* Pointers on pSynthMemErr2 filter's memory */
      int i;
      short gain_code_enha = 0;

      /* Compute the error signal (input to the second stage) in the residual domain */
      for(i = 0; i < G7291_LSUBFR; i++) {
        res2[i] = Sub_16s(exc2[i+indexSubfr], exc[i+indexSubfr]);
      }
      //ippsSub_16s(&exc2[indexSubfr], &exc[indexSubfr], res2, G7291_LSUBFR);

      /*  Find the target vector for the second fixed codebook   */
      ippsSynthesisFilter_NR_16s_Sfs(Aq, res2, error2, G7291_LSUBFR, 12, pSynthMemErr2);
    /* ILBC is some faster, can be used without accuracy losses [AM] */
      ippsResidualFilter_AMRWB_16s_Sfs(Ap1, G7291_LP_ORDER, error2, xn_enha, G7291_LSUBFR, 10); 
      /*ippsResidualFilter_ILBC_16s_Sfs(error2, Ap1, xn_enha, G7291_LSUBFR, 11, &error2[-G7291_LP_ORDER]);*/

      ippsSynthesisFilterLow_NR_16s_ISfs(Ap2,xn_enha,G7291_LSUBFR,12,pCodStat->pSynthMem2);

      /* - Innovative codebook search. */

      /* search in fixed-codebook:  */
      ippsAlgebraicCodebookSearchL2_G7291_16s(xn_enha, res2, h1, voice_fac, code_enha, y2_enha, indexFC);
      *ana2++ = indexFC[1];           /* Positions index */
      *ana2++ = indexFC[0];           /* Signs index     */

      i = i0 + (SubFrNum<<1);
      /* quantize second fixed codebook gain and Get Quantized gain value*/
      ippsGainQuant_G7291_16s(xn_enha, y2_enha, sGainCode, &index, &gain_code_enha, i);
      *ana2++ = index; /* Save Gain index */

      /*   Find final output excitation  */
      if(gain_code_enha <= IPP_MAX_16S/2)
      {
          ippsInterpolateC_NR_G729_16s_Sfs(&exc[indexSubfr],16384,code_enha,gain_code_enha<<1,&exc_enha[indexSubfr],G7291_LSUBFR,14);
      }
      else
      {
        for (i = 0; i < G7291_LSUBFR; i++)
        {
            L_temp = code_enha[i]*(gain_code_enha<<1) + exc[indexSubfr+i]*16384;
            exc_enha[indexSubfr+i] = Cnvrt_32s16s((L_temp + 0x2000) >> 14);
        }
      }
      //ippsInterpolateC_NR_G729_16s_Sfs(&exc[indexSubfr],16384,code_enha,gain_code_enha<<1,&exc_enha[indexSubfr],G7291_LSUBFR,14);

      /* Compute synthesis signal. exc_enha[] / A(z)    */
      ippsSynthesisFilter_NR_16s_Sfs(Aq, &exc_enha[indexSubfr], &pSynth[indexSubfr], G7291_LSUBFR, 12, pCodStat->pSynthMem3);
      for (i = 0; i < G7291_LP_ORDER; i++) {
          pCodStat->pSynthMem3[i]=pSynth[indexSubfr+G7291_LSUBFR-G7291_LP_ORDER+i];
      }
      /* compute difference signal between input and local synthesis */
      for(i = 0; i < G7291_LSUBFR; i++) {
        diff[indexSubfr+i] = speech[indexSubfr+i] - pSynth[indexSubfr+i];
      }
      for(i=G7291_LSUBFR-G7291_LP_ORDER, j=0; i<G7291_LSUBFR; i++, j++)
      {
        pSynthMemErr2[j] = speech[indexSubfr + i] - pSynth[indexSubfr + i];
        pCodStat->pSynthMem2[j] = xn_enha[i] - ((y2_enha[i] * gain_code_enha) >> 13);
      }
    }

    /* - Update pitch sharpening "sPitchSharp" with quantized sGainPitch  */
    pCodStat->sPitchSharp = sGainPitch;
    if (pCodStat->sPitchSharp > G7291_SHARPMAX) pCodStat->sPitchSharp = G7291_SHARPMAX;
    if (pCodStat->sPitchSharp < G7291_SHARPMIN) pCodStat->sPitchSharp = G7291_SHARPMIN;

    updateExcErr_G7291(sGainPitch, delay[0], pCodStat->L_exc_err);

    for(i=G7291_LSUBFR-G7291_LP_ORDER, j=0; i < G7291_LSUBFR; i++, j++)
    {
      pSynthMemErr1[j] = speech[indexSubfr+i] - synth_ptr[indexSubfr+i];
      temp = ((y1[i] * sGainPitch) >> 14) + ((y2[i] * sGainCode) >> 13);
      pCodStat->pSynthMem0[j] = xn[i] - temp;
    }

    A += G7291_G729_MP1;       /* interpolated LPC parameters for next subframe */
    Aq += G7291_G729_MP1;
  }

  if(pCodStat->rate == 8000) ippsCopy_16s(synth_ptr, pSynth, G7291_LFRAME4);

  /* Update signal for next frame */
  for (i = 0; i < G7291_M_LPC; i++) {
      pCodStat->pSynthMemErr1[i]=pSynthMemErr1[i];
      pCodStat->pSynthMemErr2[i]=pSynthMemErr2[i];
  }
  ippsCopy_16s(&pWgtSpchVector[G7291_LFRAME4], &pCodStat->pWgtSpchVector[0], G7291_PITCH_LAG_MAX);
  ippsCopy_16s(&pExcVector[G7291_LFRAME4], &pCodStat->pExcVector[0], G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  ippsCopy_16s(&pSpeechVector[G7291_LFRAME4], pCodStat->pSpeechVector, G7291_MEM_SPEECH);
  ippsCopy_16s(exc2, resBsFreqk, G7291_LFRAME4);
  ippsCopy_16s(speech, sync_speech, G7291_LFRAME4);

  return;
}

/*  TDBWE encoder (parameter extraction), computes and quantize time/frequency envelopes */
void TDBWE_encoder(short *pMemTDBWESpeech, short *input_hi, short *coder_parameters, short *input_hi_delay)
{
  IPP_ALIGNED_ARRAY(16, short, parameters, G7291_TDBWE_NB_PARAMETERS); /* Q10 */
  IPP_ALIGNED_ARRAY(16, short, pSpeechVector, G7291_TDBWE_L_TOTAL);
  short *new_speech, *speech;

  new_speech = pSpeechVector + G7291_TDBWE_MEM_SPEECH;  /* New speech     */
  speech = new_speech - G7291_TDBWE_L_NEXT;  /* Present frame  */

  /* update buffer */
  ippsCopy_16s(pMemTDBWESpeech, pSpeechVector, G7291_TDBWE_MEM_SPEECH);
  ippsCopy_16s(input_hi, new_speech, G7291_TDBWE_L_FRAME);

  /* time envelope extraction */
  ippsEnvelopTime_G7291_16s(speech + G7291_TDBWE_DELTA_TIME_ENV, 0, /* nls */
                                     parameters, G7291_TDBWE_NB_SUBFRAMES);

  /* extraction of the frequency envelope for the 2nd 10ms frame */
  ippsEnvelopFrequency_G7291_16s(speech + (G7291_TDBWE_L_FRAME2 + G7291_TDBWE_DELTA_FREQ_ENV),
                                          &parameters[G7291_TDBWE_PAROFS_FREQ_ENVELOPE]);

  /* quantize the parameter set and write bitstream */
  ippsQuantParam_G7291_16s(parameters, coder_parameters);

  /* update memory */
  ippsCopy_16s(&pSpeechVector[G7291_TDBWE_L_FRAME], pMemTDBWESpeech, G7291_TDBWE_MEM_SPEECH);
  ippsCopy_16s(speech, input_hi_delay, G7291_TDBWE_L_FRAME);
  return;
}

/* TDAC_encoder */
void TDAC_encoder(short *y_lo, short *y_hi, short *pPrms, short *nbit_env, unsigned int *pVQPrms,
                  short *vqnbit_env, int nbit, int nbit_max, int norm_lo,
                  int norm_hi, int norm_MDCT)
{
  IPP_ALIGNED_ARRAY(16, unsigned int, prm_alloc, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, rms_index, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, log_rms, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, ip, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, ord_b, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, bit_alloc, G7291_TDAC_NB_SB);
  IPP_ALIGNED_ARRAY(16, short, y, G7291_LFRAME);
  int i, j;
  int tmp, bit_cnt;

  if (nbit > 0)
  {
    /* Form full-band spectrum */
    ippsCopy_16s(y_lo, y, G7291_LFRAME2);
    ippsCopy_16s(y_hi, &y[G7291_LFRAME2], G7291_LFRAME2);

    /* compute spectral envelope */
    ownCalcSpecEnv_TDAC(y, log_rms, G7291_TDAC_NB_SB, norm_lo, norm_hi, norm_MDCT);

    /* encode spectral envelope */
    bit_cnt = 0;
    ownEncodeSpectralEnvelope_TDAC(log_rms, rms_index, pPrms, nbit_env, &bit_cnt, nbit, norm_MDCT);

    /* compute the number of available bits */
    nbit_max -= bit_cnt;
    nbit -= bit_cnt;  /* number of bits left (at this stage) */

    /* compute perceptual importance:
       for 3dB step in spectral envelope coding
       1/2log2(enerq) = 1/2 ( rms_index + log2 nb_coef) */
    for (i=0; i<G7291_TDAC_NB_SB; i++) ip[i] = rms_index[i];
    ip[G7291_TDAC_NB_SB-1]++;

    /* order subbands by decreasing perceptual importance  */
    ownSort_TDAC(ip, ord_b);

    /* allocate bits to subbands */
    ownBitAllocation_TDAC(bit_alloc, nbit_max, ip, ord_b);

    /* force zero bit to match bit budget (loop by decreasing perceptual importance) */
    bit_cnt = 0;
    for (j = 0; j < G7291_TDAC_NB_SB; j++)
    {
      prm_alloc[j] = 0;
      tmp = bit_cnt + bit_alloc[ord_b[j]];
      if (tmp <= nbit)
      {
        bit_cnt = tmp;
      }
      else
      {
        /* bit budget is zero, set allocation to 0 bits for the other bands  */
        for (i = j; i < G7291_TDAC_NB_SB; i++) bit_alloc[ord_b[i]] = 0;
      }
    }

    i=0;
    for (j = 0; j < G7291_TDAC_NB_SB; j++){
       i += bit_alloc[j];
    }

    /* - quantize MDCT coefficients */
    /* apply split spherical vector quantization to MDCT coefficients */
    if(i)
       ippsMDCTQuantFwd_G7291_16s32u(y, bit_alloc, prm_alloc);
    for (i=0; i<G7291_TDAC_NB_SB; i++) {
        pVQPrms[i] = prm_alloc[ord_b[i]];
        vqnbit_env[i] = bit_alloc[ord_b[i]];
    }
  }
  return;
}

/* parameter sizes (# of bits), one table per rate */
static __ALIGN32 CONST short NumberBitsLayer1Tbl[11] = {
    /* Layer 1 - core layer (narrowband embedded CELP */
                /* 10 ms frame 1 */
/* subframe1 */
   8, 10,                                   /* Line spectrum pairs: L0, L1, L2, L3 */
   8,                                       /* Adaptive-codebook delay: P1 */
   1,                                       /* Pitch-delay parity: P0 */
   13,                                      /* Fixed-codebook index: C1 */
   4,                                       /* Fixed-codebook sign: S1 */
   7,                                       /* Codebook gains (stage 1, 2): GA1, GB1 */
/* subframe2 */
   5,                                       /* Adaptive-codebook delay: P2 */
   13,                                      /* Fixed-codebook index: C2 */
   4,                                       /* Fixed-codebook sign: S2 */
   7                                        /* Codebook gains (stage 1, 2): GA2, GB2 */
};

static __ALIGN32 CONST short NumberBitsLayer2Tbl[7] = {
    /* Layer 2 - narrowband enhancement layer (embedded CELP */
                /* 10 ms frame 1 */
/* subframe1 */
   13,                                      /* 2-nd Fixed-codebook index: C1 */
   4,                                       /* 2-nd Fixed-codebook sign: S1 */
   3,                                       /* 2-nd Fixed-codebook gain: G1 */
/* subframe2 */
   13,                                      /* 2-nd Fixed-codebook index: C2 */
   4,                                       /* 2-nd Fixed-codebook sign: S2 */
   1,                                       /* FEC bit (class information): CL1 */
   2                                        /* 2-nd Fixed-codebook gain: G2 */
};
static __ALIGN32 CONST short NumberBitsLayer3Tbl[7] = {
   /* Layer 3 - wideband enhancement layer (TDBWE) */
   5,                                       /* Time envelope mean: MU */
   7, 7,                                    /* Time envelope split VQ: T1, T2 */
   5, 5, 4,                                 /* Frequency envelope split VQ: F1, F2, F3 */
   7                                        /* FEC bits phase information): PH */
};
static __ALIGN32 CONST short NumberBitsLayer4Tbl[2] = {
   /* Layer 4-12 - wideband enhancement layers (TDAC) */
   5,                                       /* FEC bits (energy information): E */
   4                                        /* MDCT norm: N */
};

static char* ownPar2Ser( int valPrm, char *pBitStreamvec, int valNumBits )
{
    char *TempPnt = pBitStreamvec + valNumBits -1;
    int i;
    short tmp;

    for ( i = 0 ; i < valNumBits; i ++ ) {
        tmp = (short) (valPrm & 0x1);
        valPrm >>= 1;
        *TempPnt-- = (char)tmp;
    }

    return (pBitStreamvec + valNumBits);
}

static char* ownVQPar2Ser( unsigned int valPrm, char *pBitStreamvec, int valNumBits )
{
    char *TempPnt = pBitStreamvec + valNumBits -1;
    int i;
    short tmp;

    for ( i = 0 ; i < valNumBits; i ++ ) {
        tmp = (short) (valPrm & 0x1);
        valPrm >>= 1;
        *TempPnt-- = (char)tmp;
    }

    return (pBitStreamvec + valNumBits);
}

static void ownPrms2Bits(const short* pPrms, const short* nbit_env,
                         const unsigned int* pvqPrms, const short* vqnbit_env,
                         unsigned char *pBitstream, int rate, int nb_bits_used)
{
    IPP_ALIGNED_ARRAY(16, char, pBitStreamvec, G7291_MAX_BITS_BST);
    unsigned int utmp;
    unsigned char *pBits = (unsigned char*)pBitstream;
    char *pBsp = pBitStreamvec;
    int i, tmp, valSht, valBitCnt = 0;

    if (rate >= 8000) {
        /* Generate 8k bistream */
        /* --> first subframe (80 bits) */
        for(i = 0; i < 11; i++) {
            valBitCnt += NumberBitsLayer1Tbl[i];
            tmp = pPrms[i];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer1Tbl[i]);
        }
        /* --> second subframe (80 bits) */
        for(i = 0; i < 11; i++) {
            valBitCnt += NumberBitsLayer1Tbl[i];
            tmp = pPrms[i+17];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer1Tbl[i]);
        }
    }
    if (rate >= 12000) {
        /* Generate 12k bitstream */
        /* --> first subframe (40 bits) */
        for(i = 0; i < 5; i++) {
            valBitCnt += NumberBitsLayer2Tbl[i];
            tmp = pPrms[i+11];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[i]);
        }
        valBitCnt += NumberBitsLayer2Tbl[5];
        tmp = pPrms[41] >> 1;
        pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[5]);
        valBitCnt += NumberBitsLayer2Tbl[6];
        tmp = pPrms[i+11];
        pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[6]);

        for(i = 0; i < 5; i++) {
            valBitCnt += NumberBitsLayer2Tbl[i];
            tmp = pPrms[i+28];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[i]);
        }
        valBitCnt += NumberBitsLayer2Tbl[5];
        tmp = pPrms[41];
        pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[5]);
        valBitCnt += NumberBitsLayer2Tbl[6];
        tmp = pPrms[i+28];
        pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer2Tbl[6]);
    }
    if (rate >= 14000) {
        /* Generate 14k bitstream (33 bits + 7 bits - FEC -) */
        for(i = 0; i < 6; i++) {
            valBitCnt += NumberBitsLayer3Tbl[i];
            tmp = pPrms[i+34];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer3Tbl[i]);
        }
        valBitCnt += NumberBitsLayer3Tbl[6];
        tmp = pPrms[40];
        pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer3Tbl[6]);
    }
    if (rate >= 16000) {
    /* Generate 16k+ bitstream (FEC : 5 bits ; norm_shift : 4 bits) */
        for(i = 0; i < 2; i++) {
            valBitCnt += NumberBitsLayer4Tbl[i];
            tmp = pPrms[i+42];
            pBsp = ownPar2Ser(tmp, pBsp, NumberBitsLayer4Tbl[i]);
        }
        for(i = 0; i < G7291_TDAC_NB_SB_WB+1; i++) {
            if (nb_bits_used >= valBitCnt+nbit_env[G7291_TDAC_NB_SB_NB+1+i]) {
                valBitCnt += nbit_env[G7291_TDAC_NB_SB_NB+1+i];
                tmp = pPrms[i+44+G7291_TDAC_NB_SB_NB+1];
                pBsp = ownPar2Ser(tmp, pBsp, nbit_env[G7291_TDAC_NB_SB_NB+1+i]);
            } else {
                int nbit = nb_bits_used - valBitCnt;
                valBitCnt += nbit;
                tmp = pPrms[i+44+G7291_TDAC_NB_SB_NB+1] >> (nbit_env[G7291_TDAC_NB_SB_NB+1+i]-nbit);
                pBsp = ownPar2Ser(tmp, pBsp, nbit);
                break;
            }
        }
        for(i = 0; i < G7291_TDAC_NB_SB_NB+1; i++) {
            if (nb_bits_used >= valBitCnt+nbit_env[i]) {
                valBitCnt += nbit_env[i];
                tmp = pPrms[i+44];
                pBsp = ownPar2Ser(tmp, pBsp, nbit_env[i]);
            } else {
                int nbit = nb_bits_used - valBitCnt;
                valBitCnt += nbit;
                tmp = pPrms[i+44] >> (nbit_env[i]-nbit);
                pBsp = ownPar2Ser(tmp, pBsp, nbit);
                break;
            }
        }
        for(i = 0; i < G7291_TDAC_NB_SB; i++) {
            if (nb_bits_used >= valBitCnt+vqnbit_env[i]) {
                valBitCnt += vqnbit_env[i];
                utmp = pvqPrms[i];
                pBsp = ownVQPar2Ser(utmp, pBsp, vqnbit_env[i]);
            } else {
                int nbit = nb_bits_used - valBitCnt;
                valBitCnt += nbit;
                utmp = pvqPrms[i] >> (vqnbit_env[i]-nbit);
                pBsp = ownVQPar2Ser(utmp, pBsp, nbit);
                break;
            }
        }
    }
    for(i=valBitCnt; i<nb_bits_used; i++) {
        valBitCnt += 1;
        pBsp = ownPar2Ser(0, pBsp, 1);
    }


    /* Clear the output vector */
    ippsZero_8u((Ipp8u*)pBitstream, (valBitCnt+7)/8);
    valSht  = (valBitCnt + 7) / 8 * 8 - valBitCnt;

    for ( i = 0 ; i < valBitCnt; i ++ ) {
        pBits[i/8] <<= 1;
        if (pBitStreamvec[i]) pBits[i/8]++;
    }
    pBits[(valBitCnt-1)/8] <<= valSht;

    return;
}

