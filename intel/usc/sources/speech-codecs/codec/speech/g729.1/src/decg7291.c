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
// Purpose: G.729/A/B/D/E/EV speech codec: decoder API functions.
//
*/

#include <stdio.h>
#include <stdlib.h>
#include <ippsc.h>
#include <ipps.h>
#include "owng7291.h"

static void CELP2SDecoderInit(G729_DecoderState *pDecStat, int g729b);
static void CELP2S_decoder(G729_DecoderState *pDecStat, short *coder_parameters, int rate,
                    int SubFrNum, G729ParamsStruct *nb_celp_param, short *BfAq,
                    short *coreExc, short *extExc, short *pitch_buf, short clas, int bfi);
static void TDBWEDecoderInit(TDBWE_DecoderState * state);
static void TDBWE_decoder(TDBWE_DecoderState * state, G729ParamsStruct * nb_celp_param, short * signal_highband,
                   short * parameters);
static void TDAC_decoder(const short *bitstream, int *valBitCnt, int nbit, int nbit_max,
                  short *yt_hi, short *yq_lo, short *yq_hi, int norm_MDCT);
static void PostFilterInit(PostFilterState *pfstat);
static void Post_G7291(PostFilterState * pfstat, short t0, short * signal_ptr, short * coeff,
                 short * sig_out, short gamma1, short gamma2, short PostNB, short Vad);
static void ownBits2Prms(const unsigned char **pBitstream, short* pPrms, short* ind_FER,
                         int* valBitCnt, int rate);
static void ownBits2Prms_G729B(const unsigned char **pBitstream, short* pPrms, short *ftyp);

/* filter coefficients (fc = 100 Hz) */
static __ALIGN32 CONST short b100[3] = { 15398, -30795, 15398 };
static __ALIGN32 CONST short a100[3] = { 16384, 31671, -15334 };

int apiG7291Decoder_Alloc(G729Codec_Type codecType, int *pCodecSize)
{
    int fltSize;
    if(codecType != G7291_CODEC) return 1;
    *pCodecSize = (sizeof(G7291Decoder_Obj)+7)&(~7);
    ippsQMFGetStateSize_G7291_16s(&fltSize);
    *pCodecSize += fltSize;
    ippsFilterHighpassGetStateSize_G7291_16s(&fltSize);
    *pCodecSize += fltSize;
    ippsGenerateExcitationGetStateSize_G7291_16s(&fltSize);
    *pCodecSize += fltSize;
//    ippsLowPassFilterStateSize_G7291_16s(&fltSize);
//    *pCodecSize += fltSize;
//    *pCodecSize += fltSize;
    return 0;
}

 /*  Decoder Initializations */
int apiG7291Decoder_Init(G7291Decoder_Obj *decstat, int freq, int outMode, int lowDelay)
{
  Ipp16s abHP[6];
  int i, fltSize;

  if(decstat == NULL) return 1;

  ippsZero_8u((Ipp8u*)decstat, (sizeof(*decstat)+7)&(~7));

  /* set previous bad frame indicator */
  decstat->iPrevBFI = 0;

  /* set the flag for 8kHz sampled output */
  decstat->freq = freq;

  /* set the flag for g729 bitstream mode */
  decstat->g729b = outMode;
  /* set the flag for low delay mode */
  decstat->iLowDelayFlg = lowDelay;

  /* initialize QMF filterbank */
  decstat->pMemQMFsyn = (IppsQMFState_G7291_16s*)((char*)decstat + ((sizeof(G7291Decoder_Obj)+7)&(~7)));
  ippsQMFInit_G7291_16s(decstat->pMemQMFsyn);
  ippsQMFGetStateSize_G7291_16s(&fltSize);

  /* initialize CELP decoder */
  CELP2SDecoderInit(&decstat->pG729State, decstat->g729b);

  /* initialize memories for MDCT computation */
  ippsZero_16s(decstat->pMDCTCoeffsHigh, G7291_LFRAME2);
  ippsZero_16s(decstat->pInvMDCTCoeffsHigh, G7291_LFRAME2);
  ippsZero_16s(decstat->pInvMDCTCoeffsLow, G7291_LFRAME2);

  ippsZero_16s(decstat->pTdbweHighOld, G7291_LFRAME2);

  decstat->sSmoothHigh = 0x7fff;
  decstat->sSmoothLow = 0x7fff;

  decstat->pMemHPflt = (IppsFilterHighpassState_G7291_16s*)((char*)decstat->pMemQMFsyn + fltSize);
  ippsFilterHighpassGetStateSize_G7291_16s(&fltSize);

  /* initialize TDBWE decoder */
  decstat->pTDWBEState.pGenExcState = (IppsGenerateExcitationState_G7291_16s*)((char*)decstat->pMemHPflt + fltSize);
  ippsGenerateExcitationGetStateSize_G7291_16s(&fltSize);
//  decstat->pTDWBEState.lowpass_3_kHz = (IppsLowPassFilterState_G7291_16s*)((char*)decstat->pTDWBEState.pGenExcState + fltSize);
  TDBWEDecoderInit(&decstat->pTDWBEState);

  if(decstat->iLowDelayFlg > 0) {
//    ippsLowPassFilterStateSize_G7291_16s(&fltSize);
//    decstat->pMemLP3k = (IppsLowPassFilterState_G7291_16s*)((char*)decstat->pMemHPflt + (fltSize<<1));
//    ippsLowPassFilterInit_G7291_16s(abLP3k, decstat->pMemLP3k);
    ippsZero_8u((Ipp8u*)decstat->pMemLP3k, 24);
  }

  /* initialize memory of previous CELP-decoded frame (for difference computation in lower band) */
  ippsZero_16s(decstat->pG729OutputOld, G7291_LFRAME2);

  /* postfiltering */
  for (i=0; i<G7291_M_LPC; i++) decstat->pMemPstFltInput[i]=0;
  PostFilterInit(&decstat->pPstFltState);

  ippsZero_16s(decstat->pMemLPC, 4 * G7291_G729_MP1);
  for(i = 0; i < (4 * G7291_G729_MP1); i += G7291_G729_MP1) decstat->pMemLPC[i] = 4096;
  for(i = 0; i < 4; i++) decstat->pMemPitchDelay[i] = 60;

  /* post processing */
  if(decstat->iLowDelayFlg == 0) decstat->sPostProcFlg = 0;
  else decstat->sPostProcFlg = 1;

  abHP[0] = a100[0]; abHP[1] = a100[1]; abHP[2] = a100[2];
  abHP[3] = b100[0]; abHP[4] = b100[1]; abHP[5] = b100[2];
  ippsFilterHighpassInit_G7291_16s(decstat->pMemHPflt, abHP);

  /* perceptual weighting filter of lower band difference signal */
  for (i=0; i<G7291_LP_ORDER; i++) {
      decstat->pInvWeightFltInput[i]=0;
      decstat->pInvWeightFltOutput[i]=0;
  }

  /* bitrate switching initialization */
  decstat->sRcvCount = G7291_COUNT_RCV_MAX;
  decstat->sFirstFrameFlg = 1;
  decstat->sNumRcvHigh = 0;
  decstat->prevRate = 0;

  /* init FEC variables */
  decstat->sLPSpeechEnergyFER = (256 * 60);
  decstat->sLastGood = G7291_FEC_UNVOICED;
  decstat->sPrevLastGood = G7291_FEC_UNVOICED;
  decstat->sLPEnergyOld = 0;
  ippsZero_16s(decstat->pSynthBuffer, G7291_PITCH_LAG_MAX);

  /* initialize memory for median filter to sSmooth the gamma parameters of the postfilter */
  for(i = 0; i < G7291_NMAX; i++)
  {
    decstat->pMemAdaptGamma[i] = 0;
  }
  return 0;
}

/*  Main decoder routine  */
int apiG7291Decode(G7291Decoder_Obj *decstat, const short *itu_192_bitstream, short *tabout,
                   int rate, int bfi)
{
  IPP_ALIGNED_ARRAY(16, short, celp_output, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, output_lo, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, output_hi, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, output_hiTdbwe, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, diff_q, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, yq_lo, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, yq_hi, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, yt_hi, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, pst_in, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, parameters_tdbwe, G7291_TDBWE_NB_PARAMETERS);
  IPP_ALIGNED_ARRAY(16, short, pf_in_buffer, G7291_M_LPC + G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, coreExc, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, extExc, G7291_LFRAME2);
  IPP_ALIGNED_ARRAY(16, short, BfAq, 4 * G7291_G729_MP1);
  short mem_syntmp[G7291_LP_ORDER];
  short coder_parameters[G7291_PRM_SIZE_MAX * 2 + 10];
  short pitch_buf[G7291_NB_SUBFR];
  short ind_fer[3];
  short parm_FEC[13];
  G729ParamsStruct nb_celp_param;
  PostFilterState *pfstat;
  short *ptr_Az;
  short *coder_param_ptr;

  int Ltmp, L_enr_q;
  int valBitCnt = 0;
  int i, j, ind;
  int nb_bits_to_decode;
  int nb_bits_left;
  int nb_bits_used;
  int nb_bits_max;
  int norm_MDCT, norm_local, diff_norm;
  short nRcv_hi;
  short ga1_post, ga2_post;
  short tmp16;
  short clas;
  short FerPitch;
  short begin_index_low, end_index_low, begin_index_high, end_index_high; /*beggining & end of the false alarm zone */

  if(NULL==decstat || NULL==itu_192_bitstream || NULL ==tabout)
        return APIG729_StsBadArgErr;

  decstat->pG729State.rate = rate;
  /* set pointer to parameters */
  coder_param_ptr = coder_parameters;

  /* compute number of bits to decode */
  nb_bits_to_decode = rate / 50;

  /* set FEC variables */
  L_enr_q = (-1);
  clas = decstat->sLastGood;
  pitch_buf[0] = decstat->pG729State.pMemPitch[0];
  pitch_buf[1] = decstat->pG729State.pMemPitch[1];
  pitch_buf[2] = decstat->pG729State.pMemPitch[2];
  pitch_buf[3] = decstat->pG729State.pMemPitch[3];
  ind_fer[0] = (-1000);
  ind_fer[1] = (-1000);
  ind_fer[2] = (-1000);

  /* set frame type (ftyp) for G.729B decoding */
  if (decstat->g729b)
  {
    if (bfi == -1)
    {
        bfi = 1;
        if (decstat->pG729State.past_ftyp == 1)
        {
            decstat->pG729State.ftyp[0] = 1;
            decstat->pG729State.ftyp[1] = 1;
        } else
        {
            decstat->pG729State.ftyp[0] = 0;
            decstat->pG729State.ftyp[1] = 0;
        }
    } else
    {
        decstat->pG729State.ftyp[0] = (short)(bfi & 3);
        decstat->pG729State.ftyp[1] = (short)(bfi >> 2);
        bfi = 0;
    }

  }

  if((!bfi) || ((decstat->g729b) && (decstat->pG729State.ftyp[0] == 0)))
  {
    /* decode received frame (bfi=0) */
    nb_bits_max = G7291_MAX_BITRATE / 50;  /* maximal bit allocation (-> 32 kbit/s) */
    nb_bits_left = nb_bits_to_decode;      /* number of bits available for decoder */
    nb_bits_used = 0;                      /* number of bits used */

    /* Read bitstream and demultiplex parameters */
    if (decstat->g729b) ownBits2Prms_G729B((const unsigned char **)&itu_192_bitstream, coder_parameters, decstat->pG729State.ftyp);
    else
        ownBits2Prms((const unsigned char **)&itu_192_bitstream, coder_parameters, ind_fer, &valBitCnt, rate);

    /* decode FEC information (clas, puslse offset and energy) */
    if(rate >= 12000)
    {
      clas = ownDecodeInfo_FEC(&(decstat->sLastGood), decstat->sPrevLastGood, decstat->iPrevBFI, ind_fer, &L_enr_q, rate);
    }

    /* Update decoder CELP parameters if previous frame was erased (recovery) */
    if((decstat->iPrevBFI == 1) && (decstat->iLowDelayFlg == 0))
    {
      short pitch_index = coder_parameters[2];
      short parity = coder_parameters[3];
      short temp, sum, bit;

      temp = pitch_index >> 1;
      sum = 1;
      for (i = 0; i <= 5; i++)
      {
        temp = temp >> 1;
        bit = temp & 1;
        sum = sum + bit;
      }
      sum = sum + parity;
      sum = sum & 1;
      coder_parameters[3] = sum;

      /* The new clas is already known with the 14 kbit/s bitstream */
      ownDecode_FEC(coder_parameters, decstat->pG729OutputOld, BfAq, G7291_FEC_SINGLE_FRAME,
                        &(decstat->sLastGood), pitch_buf,
                        decstat->pG729State.pExcVector, &(decstat->pG729State), ind_fer[2]);

      /* Update to be done before decoding a good frame */
      FerPitch = (pitch_buf[0] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
      if (FerPitch < G7291_PITCH_LAG_MIN) FerPitch = G7291_PITCH_LAG_MIN;
      if (FerPitch > G7291_PITCH_LAG_MAX) FerPitch = G7291_PITCH_LAG_MAX;

      ownUpdateMemTam(&decstat->pG729State, decstat->pG729State.sDampedPitchGain, FerPitch);
      ownUpdateMemTam(&decstat->pG729State, decstat->pG729State.sDampedPitchGain, FerPitch);

      /* update memory of A(z) */
      ippsCopy_16s(BfAq, decstat->pMemLPC, 4 * G7291_G729_MP1);
      for(i = 0; i < 4; i++)
      {
        decstat->pMemPitchDelay[i] = (pitch_buf[i] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
      }
    }

    if((clas == G7291_FEC_SIN_ONSET) && (rate >= 14000) && (ind_fer[2] > 0))
    {
      if (rate < 16000)
      {
        /* Use reconstruct energy information when not present in bitstream */
        L_enr_q = decstat->pG729State.iEnergy;
      }

      if(decstat->pG729State.sBFICount == 1)
      {
        ownUpdateAdaptCdbk_FEC(decstat->pG729State.pExcVector, ind_fer[2], pitch_buf, L_enr_q,
                                      BfAq + 3 * G7291_G729_MP1);
      }
      else
      {
        ownUpdateAdaptCdbk_FEC(decstat->pG729State.pExcVector, ind_fer[2], pitch_buf, L_enr_q,
                                      decstat->pMemLPC + 3 * G7291_G729_MP1);
      }
    }

    for (i=0; i<G7291_LP_ORDER; i++)
        mem_syntmp[i]=decstat->pG729State.pSynthMem[i];
    decstat->pG729State.sDampedCodeGain = 0;
    decstat->pG729State.sDampedPitchGain = 0;
    decstat->pG729State.sSubFrameCount = 0;

    /* Decoding of lower band : 8k core and 12k enhancement layer */
    for(i=0, j=0; i<G7291_LFRAME2; i+=G7291_LFRAME4, j++)
    {
      /* get synthesis of G729/12k decoder (without postfilter) */
      CELP2S_decoder(&decstat->pG729State, coder_param_ptr, rate, j, &nb_celp_param,
                            &BfAq[j * 2 * G7291_G729_MP1], &coreExc[i], &extExc[i],
                            pitch_buf, clas, bfi);
      if(rate >= 12000) nb_bits_used += 120;
      else              nb_bits_used += 80;
      coder_param_ptr += G7291_PRM_SIZE_MAX;
    }

    /* number of bits lefts for decoding after CELP decoding */
    nb_bits_left -= nb_bits_used;

    ownSynthesys_G7291(coreExc, extExc, decstat->pG729State.pExcVector, BfAq, celp_output, decstat->pG729State.pSynthMem,
                    decstat->pG729State.rate);

    if(rate < 12000)
    {
      ownSignalClassification_FEC(&clas, pitch_buf, decstat->sLastGood, celp_output, decstat->pSynthBuffer,
                          &(decstat->sLPSpeechEnergyFER));
    }

    ownUpdatePitch_FEC(clas, pitch_buf, decstat->sLastGood, &(decstat->pG729State.sUpdateCounter),
                        &(decstat->pG729State.sFracPitch), decstat->pG729State.pFracPitch);

    ownScaleSynthesis_FEC(nb_bits_left, clas, decstat->sLastGood, celp_output, pitch_buf, L_enr_q,
                         decstat->iPrevBFI, BfAq, &(decstat->sLPEnergyOld), mem_syntmp, coreExc,
                         decstat->pG729State.iEnergy, decstat->pG729State.pExcVector, decstat->pG729State.pSynthMem,
                         decstat->pG729State.sBFICount);

    /* Decoding of higher band : 14k enhancement layer */
    if(rate >= 14000)
    {
      /* decode TDBWE parameters */
      ownDeQuantization_TDBWE_16s(coder_param_ptr, parameters_tdbwe);
      TDBWE_decoder(&decstat->pTDWBEState, &nb_celp_param, output_hiTdbwe, parameters_tdbwe);

      /* copy TDBWE parameters for FEC */
      ippsCopy_16s(parameters_tdbwe, decstat->pTDBWEPrmsOld, G7291_TDBWE_NB_PARAMETERS);
      for(i = 0; i < G7291_NB_SUBFR; i++)
      {
        decstat->pG729ParamPrev.pPitch[i] = nb_celp_param.pPitch[i];
        decstat->pG729ParamPrev.pPitchFrac[i] = nb_celp_param.pPitchFrac[i];
        decstat->pG729ParamPrev.pFixPower[i] = nb_celp_param.pFixPower[i];
        decstat->pG729ParamPrev.pLtpPower[i] = nb_celp_param.pLtpPower[i];
      }

      nb_bits_used += 40;
      nb_bits_left -= 40;
      coder_param_ptr += 6;

      /* postprocessing of higher band (3 kHz low-pass filter) */
      if(decstat->iLowDelayFlg > 0)
      {
#ifdef _FAST_G7291
        ippsFilterLowpass_G7291_16s_I(output_hiTdbwe, G7291_LFRAME2, decstat->pMemLP3k, ippAlgHintFast);
#else
        ippsFilterLowpass_G7291_16s_I(output_hiTdbwe, G7291_LFRAME2, decstat->pMemLP3k, ippAlgHintAccurate);
#endif
        ippsZero_16s(output_hiTdbwe, G7291_LFRAME2);
      }
    }
    else
    {
      /* if rate < 14000 TDBWE output is forced to 0 */
      ippsZero_16s(output_hiTdbwe, G7291_LFRAME2);
    }

    /* Decoding of full-band spectrum: 16-32k enhancement layers */
    if(decstat->iLowDelayFlg == 0)
    {
      nb_bits_max -= nb_bits_used;

      /* decode TDAC module - get norm_MDCT value */
      norm_MDCT = *coder_param_ptr;
      /* compute MDCT of the TDBWE synthesis */
      {
#ifdef _FAST_G7291
         IppHintAlgorithm hint = ippAlgHintFast;
#else
         IppHintAlgorithm hint = ippAlgHintAccurate;
#endif
         ippsMDCTFwd_G7291_16s(output_hiTdbwe, decstat->pMDCTCoeffsHigh,
                                &norm_local, yt_hi, hint);
      }
      ippsCopy_16s(output_hiTdbwe,decstat->pMDCTCoeffsHigh,G7291_LFRAME2);

      /* Update nb_bits (read FEC (5 bits) + norm_shift (4 bits)) */
      nb_bits_max -= 9;
      if(rate >= 16000)
      {
        nb_bits_left -= 9;
      }
      else
      {
        nb_bits_left = 0;
        norm_MDCT = norm_local;
      }

      /* rescale yt_hi to be norm_shift compliant */
      diff_norm = norm_MDCT - norm_local;
      if (diff_norm > 0)
        ippsLShiftC_16s(yt_hi, diff_norm, yt_hi, G7291_LFRAME2);
      else
        ippsRShiftC_16s(yt_hi, -diff_norm, yt_hi, G7291_LFRAME2);

      /* TDAC decoder */
      TDAC_decoder(itu_192_bitstream, &valBitCnt, nb_bits_left, nb_bits_max,
                          yt_hi, yq_lo, yq_hi, norm_MDCT);

      /* post-process reconstructed higher-band spectrum */
      ippsMDCTPostProcess_G7291_16s(yq_hi, (short)nb_bits_to_decode);

      /* inverse MDCT to obtain
         - reconstructed difference signal in lower band
         - reconstructed signal in higher band */
      ippsMDCTInv_G7291_16s(yq_lo, decstat->pInvMDCTCoeffsLow, diff_q, norm_MDCT);
      ippsMDCTInv_G7291_16s(yq_hi, decstat->pInvMDCTCoeffsHigh, output_hi, norm_MDCT);

      /* compute false alarm zones for pre/post-echo reduction */
      ownDiscriminationEcho(diff_q, decstat->pInvMDCTCoeffsLow, &decstat->iPrevMaxEnergyLow,
                                    &begin_index_low, &end_index_low);
      ownDiscriminationEcho(output_hi, decstat->pInvMDCTCoeffsHigh, &decstat->iPrevMaxEnergyHigh,
                                    &begin_index_high, &end_index_high);

      /* apply inverse weighting filter to the reconstructed lower-band difference signal */
      ownWeightingFilterInv_G7291_16s(decstat->pMemLPC, decstat->pInvWeightFltInput, decstat->pInvWeightFltOutput, diff_q);

      /* signal upsampling */
      ippsMulC_16s_Sfs(decstat->pG729OutputOld, 2, decstat->pG729OutputOld, G7291_LFRAME2, 0);
//      for(i = 0; i < G7291_LFRAME2; i++)
//      {
//        decstat->pG729OutputOld[i] = Cnvrt_32s16s(decstat->pG729OutputOld[i] << 1);
//      }

      /* preecho reduction & add 12k output to reconstructed difference signal */
      ownEchoReduction(output_lo, decstat->pG729OutputOld, diff_q, &decstat->sSmoothLow,
                           output_hi, decstat->pTdbweHighOld, output_hi, &decstat->sSmoothHigh, (short) (rate > 14000),
                           begin_index_low, end_index_low, begin_index_high, end_index_high);
      ippsRShiftC_16s(output_lo, 1, output_lo, G7291_LFRAME2);

      /* update output for next frame */
      ippsCopy_16s(output_hiTdbwe, decstat->pTdbweHighOld, G7291_LFRAME2);
      ippsCopy_16s(celp_output, decstat->pG729OutputOld, G7291_LFRAME2);
    }
    else
    {
      ippsCopy_16s(celp_output, output_lo, G7291_LFRAME2);
      ippsCopy_16s(output_hiTdbwe, output_hi, G7291_LFRAME2);
      ippsCopy_16s(celp_output, decstat->pG729OutputOld, G7291_LFRAME2);
    }

    /* Adapt bitrate switching gain */

    /* count number of frames with same bandwidth */
    if(rate <= 12000) nRcv_hi = 1;
    else              nRcv_hi = 0;

    if((nRcv_hi == 0) && (decstat->sNumRcvHigh == 0))
    {
      decstat->sRcvCount++;
      if (decstat->sRcvCount > G7291_COUNT_RCV_MAX) decstat->sRcvCount = G7291_COUNT_RCV_MAX;
    }
    else
    {
      if(nRcv_hi != 0)
      {
        if ((decstat->sFirstFrameFlg != 0) ||
            (decstat->sNumRcvHigh != 0) ||
            ((decstat->sNumRcvHigh == 0) && (decstat->sRcvCount < G7291_COUNT_RCV_MAX)))
        {
          decstat->sRcvCount = 0;
        }
      }
    }
    decstat->sNumRcvHigh = nRcv_hi;
    decstat->sFirstFrameFlg = 0;

    /* Postfiltering in lower-band */

    /* set adaptive gamma parameters of postfilter */
    ownSetAdaptGamma(output_lo, decstat->pMemAdaptGamma, rate, &ga1_post, &ga2_post);

    /* set post-filter input */
    for (i=0; i<G7291_M_LPC; i++) {
        pf_in_buffer[i]=decstat->pMemPstFltInput[i];
        decstat->pMemPstFltInput[i]=output_lo[G7291_LFRAME2-G7291_M_LPC+i];
    }
    ippsCopy_16s(output_lo, &pf_in_buffer[G7291_M_LPC], G7291_LFRAME2);

    /* postfilter */
    pfstat = &decstat->pPstFltState;
    if(decstat->iLowDelayFlg == 0) ptr_Az = decstat->pMemLPC;
    else ptr_Az = nb_celp_param.pLPC;

    for(i=0, j=0; i<G7291_LFRAME2; i+=G7291_LSUBFR, j++)
    {
      short t0;
      if(decstat->iLowDelayFlg == 0) t0 = decstat->pMemPitchDelay[(j / 2) * 2];
      else t0 = nb_celp_param.pPitch[(j / 2) * 2];
      Post_G7291(pfstat, t0, &pf_in_buffer[G7291_M_LPC+i], ptr_Az, &output_lo[i],
          ga1_post, ga2_post, G7291_switching_gainTbl[decstat->sRcvCount],
          decstat->pG729State.ftyp[(j/2)]);
      ptr_Az += G7291_G729_MP1;
    }

    if(decstat->iLowDelayFlg == 0)
    {
      /* update memory of A(z) */
      ippsCopy_16s(nb_celp_param.pLPC, decstat->pMemLPC, 4 * G7291_G729_MP1);
      for (i=0; i<4; i++) {
          decstat->pMemPitchDelay[i]=nb_celp_param.pPitch[i];
      }
    }

    /* Postprocess lower band */

    /* G729 postprocessing */
    ippsCopy_16s(output_lo, pst_in, G7291_LFRAME2);
    ippsFilterHighpass_G7291_16s_ISfs(output_lo, G7291_LFRAME2, decstat->pMemHPflt, 0);

    /* cross-fade output_lo and pst_out if bit rate switching */
    if(rate <= 12000)
    {
      /* cross-fade two narrowband outputs (without or with postprocessing) if previous decoded frame was not postprocessed */
      if(decstat->sPostProcFlg == 0)
      {
        for(i=0, tmp16=102; i<G7291_LFRAME2; i++, tmp16+=102)
        {
          Ltmp = tmp16*output_lo[i] + pst_in[i]*(16384-tmp16);
          output_lo[i] = (short)((Ltmp + 0x1000) >> 13);
        }
      }
      else
      {
        ippsAdd_16s(output_lo, output_lo, output_lo, G7291_LFRAME2);
      }
      decstat->sPostProcFlg = 1;
    }
    else
    {
      /* wideband output (modes >= 14k) */
      /* cross-fade two narrowband outputs (without or with postprocessing) if previous decoded frame was postprocessed
         otherwise copy non postprocessed output */
      if(decstat->sPostProcFlg == 1)
      {
        for(i=0, tmp16 = 102; i < G7291_LFRAME2; i++, tmp16+=102)
        {
          Ltmp = tmp16*pst_in[i] + output_lo[i]*(16384-tmp16);
          output_lo[i] = (short)((Ltmp + 0x1000) >> 13);
        }
      }
      else
      {
        ippsAdd_16s(pst_in, pst_in, output_lo, G7291_LFRAME2);
      }
      decstat->sPostProcFlg = 0;
    }

    /* Fold higher band and combine bands by QMF synthesis filterbank */
    if(decstat->freq == 16000)
    {
      /* synthesis filterbank */
      ippsQMFDecode_G7291_16s(output_lo, output_hi,
          G7291_switching_gainTbl[decstat->sRcvCount], G7291_LFRAME2, tabout, decstat->pMemQMFsyn);
    }
    else
    {
      ippsCopy_16s(output_lo, tabout, G7291_LFRAME2);
    }

    /* Update memories */
    decstat->sPrevLastGood = decstat->sLastGood;
    decstat->sLastGood = clas;
    decstat->pG729State.sBFICount = 0;
    decstat->prevRate = rate;
  }
  else
  {
    /* FEC (bfi=1) */

    /* Decode 8-12k layers */
    if((decstat->pG729State.sBFICount > 0) || (decstat->iLowDelayFlg > 0))
    {
      /* FEC decoder */
      ownDecode_FEC(parm_FEC, decstat->pG729OutputOld, BfAq, bfi,
                        &(decstat->sLastGood), pitch_buf,
                        decstat->pG729State.pExcVector, &(decstat->pG729State), ind_fer[2]);

      /* Update to be done before decoding a good frame */
      FerPitch = (pitch_buf[0] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
      if (FerPitch < G7291_PITCH_LAG_MIN) FerPitch = G7291_PITCH_LAG_MIN;
      if (FerPitch > G7291_PITCH_LAG_MAX) FerPitch = G7291_PITCH_LAG_MAX;

      /* taming */
      ownUpdateMemTam(&decstat->pG729State, decstat->pG729State.sDampedPitchGain, FerPitch);
      ownUpdateMemTam(&decstat->pG729State, decstat->pG729State.sDampedPitchGain, FerPitch);

      /* update memory of A(z) */
      ippsCopy_16s(BfAq, decstat->pMemLPC, 4 * G7291_G729_MP1);

      for(i = 0; i < 4; i++)
      {
        decstat->pMemPitchDelay[i] = (pitch_buf[i] + G7291_FEC_QPIT2) >> G7291_FEC_SQPIT;
      }
    }

    if(decstat->iLowDelayFlg == 0)
    {
      for(i=0, ind=G7291_TDAC_L_WIN2-1; i < G7291_TDAC_L_WIN2; i++, ind--)
      {
        output_lo[i] = (decstat->pG729OutputOld[i] * 32767 +
            decstat->pInvMDCTCoeffsLow[i] * G7291_TDAC_hTbl[ind] + 0x4000) >> 15;
      }
      ippsZero_16s(decstat->pInvMDCTCoeffsLow, G7291_TDAC_L_WIN2);
    }
    else
    {
      ippsCopy_16s(decstat->pG729OutputOld, output_lo, G7291_LFRAME2);
    }

    /* Decode 14-32k enhancement layers */
    /* TDBWE decoder in FEC */
    if(decstat->prevRate >= 14000)
    {
      if(decstat->pG729State.sBFICount > 2)
      {
        for(i = 0; i < G7291_TDBWE_NB_SUBFRAMES; i++) {
          tmp16 = decstat->pTDBWEPrmsOld[i] - 1024; /* -3 dB */
          if(tmp16 < -1700) tmp16 = -1700;
          decstat->pTDBWEPrmsOld[i] = tmp16;
        }
        for(i = 0; i < G7291_TDBWE_NB_SUBBANDS; i++) {
          tmp16 = decstat->pTDBWEPrmsOld[i+G7291_TDBWE_PAROFS_FREQ_ENVELOPE] - 1024; /* -3 dB */
          if(tmp16 < -11264) tmp16 = -11264;
          decstat->pTDBWEPrmsOld[i+G7291_TDBWE_PAROFS_FREQ_ENVELOPE] = tmp16;
        }
      }

      TDBWE_decoder(&decstat->pTDWBEState, &decstat->pG729ParamPrev,
                           output_hiTdbwe, decstat->pTDBWEPrmsOld);

      /* postprocessing of higher band (3 kHz low-pass filter) */
      if(decstat->iLowDelayFlg > 0)
      {
#ifdef _FAST_G7291
        ippsFilterLowpass_G7291_16s_I(output_hiTdbwe, G7291_LFRAME2, decstat->pMemLP3k, ippAlgHintFast);
#else
        ippsFilterLowpass_G7291_16s_I(output_hiTdbwe, G7291_LFRAME2, decstat->pMemLP3k, ippAlgHintAccurate);
#endif
        ippsZero_16s(output_hiTdbwe, G7291_LFRAME2);
      }
    }
    else
    {
      ippsZero_16s(output_hiTdbwe, G7291_LFRAME2);
    }

    /* TDAC decoder in FEC */
    if(decstat->iLowDelayFlg == 0)
    {
#ifdef _FAST_G7291
      IppHintAlgorithm hint = ippAlgHintFast;
#else
      IppHintAlgorithm hint = ippAlgHintAccurate;
#endif
      /* set 3000-4000 Hz band of TDBWE synthesis to zero */
      ippsMDCTFwd_G7291_16s(output_hiTdbwe, decstat->pMDCTCoeffsHigh,
         &norm_local, yq_hi, hint);
      ippsCopy_16s(output_hiTdbwe,decstat->pMDCTCoeffsHigh, G7291_LFRAME2);
      ippsZero_16s(&yq_hi[G7291_LFRAME2 - G7291_LSUBFR], G7291_LSUBFR);

      /* inverse MDCT to obtain reconstructed signal in higher band */
      ippsMDCTInv_G7291_16s(yq_hi, decstat->pInvMDCTCoeffsHigh, output_hi, norm_local);
    }
    else
    {
      ippsCopy_16s(output_hiTdbwe, output_hi, G7291_LFRAME2);
    }

    /* Postfilter lower band */

    /* set adaptive gamma paramaters of postfilter */
    ownSetAdaptGamma(output_lo, decstat->pMemAdaptGamma, decstat->prevRate, &ga1_post, &ga2_post);

    /* set post-filter input */
    for (i=0; i<G7291_M_LPC; i++) {
        pf_in_buffer[i]=decstat->pMemPstFltInput[i];
        decstat->pMemPstFltInput[i]=output_lo[G7291_LFRAME2-G7291_M_LPC+i];
    }
    ippsCopy_16s(output_lo, &pf_in_buffer[G7291_M_LPC], G7291_LFRAME2);

    /* postfilter */
    pfstat = &decstat->pPstFltState;
    ptr_Az = decstat->pMemLPC;

    for(i=0, j=0; i<G7291_LFRAME2; i+=G7291_LSUBFR, j++)
    {
      short t0;
      t0 = decstat->pMemPitchDelay[(j / 2) * 2];
      Post_G7291(pfstat, t0, &pf_in_buffer[G7291_M_LPC+i], ptr_Az, &output_lo[i],
          ga1_post, ga2_post, G7291_switching_gainTbl[decstat->sRcvCount],
          decstat->pG729State.ftyp[(j/2)]);
      ptr_Az += G7291_G729_MP1;
    }

    /* Postprocess lower band */
    /* G729 postprocessing */
    ippsCopy_16s(output_lo, pst_in, G7291_LFRAME2);
    ippsFilterHighpass_G7291_16s_ISfs(output_lo, G7291_LFRAME2, decstat->pMemHPflt, 1);
    if(decstat->sPostProcFlg == 0)
    {
      /* wideband bit rate -> leave lower-band unprocessed */
      ippsLShiftC_16s(pst_in, 1, output_lo, G7291_LFRAME2);
    }

    /* Fold higher band and combine bands by QMF synthesis filterbank */

    if(decstat->freq == 16000)
    {                           /* test if 8kHz sampled output */
      /* synthesis filterbank */
      ippsQMFDecode_G7291_16s(output_lo, output_hi,
          G7291_switching_gainTbl[decstat->sRcvCount], G7291_LFRAME2, tabout, decstat->pMemQMFsyn);
    }
    else
    {
      ippsCopy_16s(output_lo, tabout, G7291_LFRAME2);
    }

    /* Update memories */
    /* Update to be done before decoding a good frame */
    decstat->pG729State.sBFICount++;

    /* set buffer for W(z) */
    for (i=0; i<G7291_LP_ORDER; i++) {
        decstat->pInvWeightFltInput[i]=0;
        decstat->pInvWeightFltOutput[i]=0;
    }
  }
  decstat->iPrevBFI = bfi;
  return 0;
}


/*  Variables initialization for the CELP2S */
void CELP2SDecoderInit(G729_DecoderState *pDecStat, int g729b)
{
  short i, j;
  /* initialize pLSPVector */
  pDecStat->pLSPVector[0] = 30000;
  pDecStat->pLSPVector[1] = 26000;
  pDecStat->pLSPVector[2] = 21000;
  pDecStat->pLSPVector[3] = 15000;
  pDecStat->pLSPVector[4] = 8000;
  pDecStat->pLSPVector[5] = 0;
  pDecStat->pLSPVector[6] = -8000;
  pDecStat->pLSPVector[7] = -15000;
  pDecStat->pLSPVector[8] = -21000;
  pDecStat->pLSPVector[9] = -26000;

  /* vectors to zero */
  ippsZero_16s(pDecStat->pExcVector, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);

  pDecStat->sFERSeed = 21845;
  pDecStat->sPitchSharp = G7291_SHARPMIN;
  pDecStat->sGainCode = 0;
  pDecStat->sGainPitch = 0;

  pDecStat->pPastQntEnergy[0] = -14336;
  pDecStat->pPastQntEnergy[1] = -14336;
  pDecStat->pPastQntEnergy[2] = -14336;
  pDecStat->pPastQntEnergy[3] = -14336;

  for (i = 0; i < G7291_MA_NP; i++)
  {
      for (j=0; j<G7291_LP_ORDER; j++){
          pDecStat->pPrevLSFVector[i][j]=FreqPrevResetTbl[j];
      }
  }
  pDecStat->sMAPredCoef = 0;
  for (i=0; i<G7291_LP_ORDER; i++){
      pDecStat->pSynthMem[i]=0;
      pDecStat->pLSFVector[i]=FreqPrevResetTbl[i];
  }

  ippsZero_16s(pDecStat->pMemTam, G7291_MEM_LEN_TAM);

  pDecStat->sDampedPitchGain = 0;
  pDecStat->sDampedCodeGain = 0;
  pDecStat->sSubFrameCount = 0;
  pDecStat->sStabFactor = 0;
  pDecStat->sTiltCode = 0;
  pDecStat->sUpdateCounter = G7291_FEC_MAX_UPD_CNT;
  pDecStat->iEnergy = 0;
  pDecStat->pFracPitch[0] = (short) (G7291_FEC_QPIT * G7291_PITCH_LAG_MIN);
  pDecStat->pFracPitch[1] = (short) (G7291_FEC_QPIT * G7291_PITCH_LAG_MIN);
  pDecStat->sFracPitch = (short) (G7291_FEC_QPIT * 40);
  pDecStat->pMemPitch[0] = (short) (G7291_FEC_QPIT * 20);
  pDecStat->pMemPitch[1] = (short) (G7291_FEC_QPIT * 20);
  pDecStat->pMemPitch[2] = (short) (G7291_FEC_QPIT * 20);
  pDecStat->pMemPitch[3] = (short) (G7291_FEC_QPIT * 20);
  pDecStat->pMemPitch[4] = (short) (G7291_FEC_QPIT * 20);
  pDecStat->sBFI = 0;
  /* G.729B mode */
  pDecStat->g729b_bst = (short)g729b;
  pDecStat->ftyp[0] = 1;
  pDecStat->ftyp[1] = 1;
  pDecStat->sid_sav = 0;
  pDecStat->sh_sid_sav = 1;
  pDecStat->sid_gain = tab_Sidgain[0];
  pDecStat->past_ftyp = 1;
  pDecStat->seed = INIT_SEED;
  for(i = 0; i < 4; i++)
  {
    pDecStat->L_exc_err[i] = (int) 0x00004000L;  /* Q14 */
    for (j=0; j<G7291_LP_ORDER; j++) {/* initialize the noise_fg */
        pDecStat->noise_fg[0][i][j]=fg[0][i][j];
        pDecStat->noise_fg[1][i][j]=fg[1][i][j];
    }
  }
  pDecStat->lspSid[0] = 31441;
  pDecStat->lspSid[1] = 27566;
  pDecStat->lspSid[2] = 21458;
  pDecStat->lspSid[3] = 13612;
  pDecStat->lspSid[4] = 4663;
  pDecStat->lspSid[5] = -4663;
  pDecStat->lspSid[6] = -13612;
  pDecStat->lspSid[7] = -21458;
  pDecStat->lspSid[8] = -27566;
  pDecStat->lspSid[9] = -31441;

  return;
}

/*  Core Codec decoder  */
void CELP2S_decoder(
G729_DecoderState *pDecStat,        /* (i/o)    Core decoder parameters     */
short *coder_parameters,            /* (i)      decoder parameters          */
int rate,                           /* (i)      decoder rate                */
int SubFrNum,                       /* (i)      subframe number             */
G729ParamsStruct *nb_celp_param,    /* (i/o)    celp parameters             */
short *BfAq,                        /* (i)      A_t_fwd parameters          */
short *coreExc,                     /* (o)      8k excitation               */
short *extExc,                      /* (o)      12k excitation              */
short *pitch_buf,                   /* (o)      pitch buffer                */
short clas,                         /* (i)      FEC information             */
int bfi)                            /* (i)      Bad frame information       */
{
  /* Array of the coded parameters */
  IPP_ALIGNED_ARRAY(16, short, pExcVector, G7291_LFRAME4 + G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  IPP_ALIGNED_ARRAY(16, short, A_t_fwd, 2 * G7291_G729_MP1); /* LPC Forward filter  */
  IPP_ALIGNED_ARRAY(16, short, pSynthBuffer, G7291_LFRAME4 + G7291_M_LPC); /* synthesis signal buffer  */
  IPP_ALIGNED_ARRAY(16, short, lsp_new, G7291_LP_ORDER); /* LSPs              */
  IPP_ALIGNED_ARRAY(16, short, code, G7291_LSUBFR);  /* ACELP codevector  */
  IPP_ALIGNED_ARRAY(16, short, code_enha, G7291_LSUBFR);
  IPP_ALIGNED_ARRAY(16, short, exc2, G7291_LFRAME4);
  IPP_ALIGNED_ARRAY(16, short, tmpbuf, G7291_LSUBFR);
  short *prm, *prm2;         /* Pointer on analysis parameters */
  short *exc;                /* Pointer on exitation vector */
  short *pA_t;               /* Pointer on A_t */
  int L_temp, L_temp2;
  int i, j, i0_2sub;
  short temp;
  short indexSubfr;
  short index;
  short g_p, g_c;           /* fixed and adaptive codebook gain  */
  short voice_fac;
  /* for G.729B */
  short ftyp;

  /*Test if Decoder Initialized */
  if(pDecStat == NULL) return;

  pDecStat->rate = rate;
  if(rate >= 12000)
  {
    short pitch_index = coder_parameters[2];
    short parity = coder_parameters[3];
    short sum, bit;

    temp = pitch_index >> 1;
    sum = 1;
    for (i = 0; i <= 5; i++)
    {
        temp = temp >> 1;
        bit = temp & 1;
        sum = sum + bit;
    }
    sum = sum + parity;
    sum = sum & 1;
    coder_parameters[3] = sum;
  }

  /* initialize pointers and restore memory */
  ippsCopy_16s(pDecStat->pExcVector, pExcVector, G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  exc = pExcVector + G7291_PITCH_LAG_MAX + G7291_L_INTERPOL;
  prm = coder_parameters;
  prm2 = &coder_parameters[G7291_PRM_SIZE];
  for (i=0; i<G7291_M_LPC; i++) pSynthBuffer[i]=pDecStat->pSynthMem[i];
  ftyp = pDecStat->ftyp[SubFrNum];

  if (ftyp != 1) /* Processing non active frames (SID & not transmitted) */
  {
    int i;
    CNG_Decoder_G729B(pDecStat, prm, exc, A_t_fwd, ftyp);
    for (i = 0; i < 2 * G7291_G729_MP1; i++)
        nb_celp_param->pLPC[2*SubFrNum*G7291_G729_MP1+i] = A_t_fwd[i];
    pDecStat->sPitchSharp = G7291_SHARPMIN;
  }
  else
  {
    short qIndex[4];
    short delayVal[2];
    short gIngx[2];
    short gains[2];
    pDecStat->seed = INIT_SEED;
    qIndex[0] = (short)((prm[0] >> G7291_NC0_B) & (short) 1);
    qIndex[1] = (short)(prm[0] & (short) (G7291_NC0 - 1));
    qIndex[2] = (short)((prm[1] >> G7291_NC1_B) & (short) (G7291_NC1 - 1));
    qIndex[3] = (short)(prm[1] & (short) (G7291_NC1 - 1));
    prm += 2;

    /* LPC decoding  */
    /* Decode the LSPs */
    pDecStat->sMAPredCoef = qIndex[0];
    ippsLSFDecode_G729_16s(qIndex, (Ipp16s*)pDecStat->pPrevLSFVector, pDecStat->pLSFVector);
    ippsLSFToLSP_G729_16s(pDecStat->pLSFVector, lsp_new);

    /* Interpolation of LPC for the 2 subframes */
    ippsInterpolate_G729_16s(lsp_new, pDecStat->pLSPVector, pDecStat->pLSPVector, G7291_LP_ORDER);
    ippsLSPToLPC_G729_16s(pDecStat->pLSPVector, A_t_fwd);          /* 1-st subframe */
    ippsLSPToLPC_G729_16s(lsp_new, &A_t_fwd[G7291_G729_MP1]);    /* 2-nd one */

    ippsCopy_16s(A_t_fwd, &nb_celp_param->pLPC[2 * SubFrNum * G7291_G729_MP1], 2 * G7291_G729_MP1);

    /* update the LSFs for the next frame */
    for (i=0; i<G7291_LP_ORDER; i++) pDecStat->pLSPVector[i]=lsp_new[i];

    pA_t = A_t_fwd;

/*          Loop for every subframe in the analysis frame
   The subframe size is L_SUBFR and the loop is repeated L_FRAME/L_SUBFR times
       - decode the pitch delay
       - decode algebraic code
       - decode pitch and codebook gains
       - find the excitation and compute synthesis speech */

    for(indexSubfr=0, i0_2sub=SubFrNum<<1; indexSubfr<G7291_LFRAME4; indexSubfr+=G7291_LSUBFR, i0_2sub++)
    {
      index = *prm++;           /* pitch index */
      ownDecodeAdaptCodebookDelays(index, indexSubfr, delayVal);

      if(indexSubfr == 0) prm++; /* get parity check result */

      /* Update memory */
      for (i = 0; i < 4; i++) pDecStat->pMemPitch[i] = pDecStat->pMemPitch[i+1];
      pDecStat->pMemPitch[4] = (delayVal[0]<<5) + (((delayVal[1]<<4)*21845+0x4000)>>15);
      pDecStat->sBFI = 0;

      pitch_buf[pDecStat->sSubFrameCount] = delayVal[0] << G7291_FEC_SQPIT;
      if(clas >= G7291_FEC_UV_TRANSITION)
      {
        pDecStat->pFracPitch[0] = pDecStat->pFracPitch[1];
        pDecStat->pFracPitch[1] = (delayVal[0]<<G7291_FEC_SQPIT) + (((delayVal[1]<<G7291_FEC_SQPIT)*10922+0x4000)>>15);
      }
      /* - Find the adaptive codebook vector. */
      /*** ippsAdaptiveCodebookDecode_G7291_16s_I(delayVal, &exc[indexSubfr]); ***/
      /* Can be replaced by (without the loss of accuracy) [AM] */
      ippsDecodeAdaptiveVector_G729_16s_I(delayVal, &exc[indexSubfr-G7291_FEC_L_EXC_MEM]);


      /* - Decode innovative codebook. */
      {
        short pos[4];
        short sign = prm[1];
        short index = prm[0];

        /* Decode the positions */
        i = index & 7;
        pos[0] = (short)(i * 5);

        index = index >> 3;
        i = index & 7;
        pos[1] = (short)(i * 5 + 1);

        index = index >> 3;
        i = index & 7;
        pos[2] = (short)(i * 5 + 2);

        index = index >> 3;
        j = index & 1;
        index = index >> 1;
        i = index & 7;
        pos[3] = (short)(i * 5 + 3 + j);

        /* decode the signs  and build the codeword */
        ippsZero_16s(code, G7291_LSUBFR);

        for(j = 0; j < 4; j++)
        {
            i = sign & 1;
            sign = sign >> 1;
            if(i != 0) code[pos[j]] = 8191;
            else       code[pos[j]] = -8192;
        }
      }
      prm += 2;

      j = pDecStat->sPitchSharp << 1;
      if(delayVal[0] < G7291_LSUBFR)
      {
        ippsHarmonicFilter_16s_I((short)j, delayVal[0], &code[delayVal[0]], G7291_LSUBFR-delayVal[0]);
      }

      /* - Decode pitch and codebook gains. */
      index = *prm++;           /* index of energy VQ */

      ippsDotProd_16s32s_Sfs(code, code, G7291_LSUBFR, &i, 0);
      gIngx[0] = (short)(index >> G7291_NCODE2_B) ;
      gIngx[1] = (short)(index & (G7291_NCODE2-1));
      ippsDecodeGain_G729_16s(i, pDecStat->pPastQntEnergy, gIngx, gains);
      pDecStat->sGainPitch = gains[0];
      pDecStat->sGainCode = gains[1];

      voice_fac = ownFindVoiceFactor((const short*)&exc[indexSubfr], (const short*)gains, (const short*)code);

      /* - Decode Second innovative codebook. */
      if(pDecStat->rate >= 12000)
      {
        short pos[4];
        short si[4];
        short sign = prm2[1];
        short index = prm2[0];
        short sp = (short)((voice_fac + 32767) >> 1);
        sp = (sp * G7291_CELP2S_SP + 0x4000) >> 15;

        /* Decode the positions */
        i = index & 7;
        pos[0] = (short)(i * 5);
        index = index >> 3;

        i = index & 7;
        pos[1] = (short)(i * 5 + 1);
        index = index >> 3;

        i = index & 7;
        pos[2] = (short)(i * 5 + 2);
        index = index >> 3;

        j = index & 1;
        index = index >> 1;
        i = index & 7;
        pos[3] = (short)(i * 5 + 3 + j);

        /* Decode the signs */
        for(j = 0; j < 4; j++)
        {
            i = sign & 1;
            si[j] = (short)i;
            sign = sign >> 1;
        }

        /* Put the tri-pulse patterns */
        ownArrangePatterns(pos, si, sp, code_enha);

        prm2 += 2;
      }

      /* - Update pitch sharpening "sPitchSharp" with quantized sGainPitch */
      pDecStat->sPitchSharp = pDecStat->sGainPitch;

      if (pDecStat->sPitchSharp > G7291_SHARPMAX) pDecStat->sPitchSharp = G7291_SHARPMAX;
      if (pDecStat->sPitchSharp < G7291_SHARPMIN) pDecStat->sPitchSharp = G7291_SHARPMIN;

      /* - Find the total excitation.
         - Find synthesis speech corresponding to exc[]. */
      g_p = ownTamFer(pDecStat->pMemTam, pDecStat->sGainPitch, delayVal[0]);
      g_c = pDecStat->sGainCode;
      nb_celp_param->pPitch[i0_2sub] = delayVal[0];
      nb_celp_param->pPitchFrac[i0_2sub] = delayVal[1];

      L_temp2 = 0;
      for(i = 0; i < G7291_LSUBFR; i++)
      {
        temp = (short)((g_p*exc[indexSubfr+i])>>15);
        L_temp2 = Add_32s(L_temp2, temp*temp);
      }
      nb_celp_param->pLtpPower[i0_2sub] = L_temp2;
      if (g_c <= IPP_MAX_16S/2)
        ippsInterpolateC_NR_G729_16s_Sfs(code, g_c<<1, &exc[indexSubfr], g_p, &exc[indexSubfr], G7291_LSUBFR, 14);
      else
        for(i = 0; i < G7291_LSUBFR; i++)
        {
            L_temp = code[i]*(g_c<<1) + exc[indexSubfr+i]*g_p;
            exc[indexSubfr+i] = Cnvrt_32s16s((L_temp+0x2000)>>14);
        }

      if(pDecStat->rate >= 12000)
      {
        short g_c_enha;

        index = *prm2++;
        /* decode gain (optimized version: table lookup) */
        if ((i0_2sub == 1) || (i0_2sub == 3))
            L_temp = g_c * G7291_GainEnha2bitsTbl[index & 0x03];
        else
            L_temp = g_c * G7291_GainEnhaTbl[index];
        g_c_enha = (short)((L_temp + 0x2000) >> 14);

        /* Compute excitation for with two inovative codebook  */
        ippsInterpolateC_G729_16s_Sfs(code, g_c, code_enha,g_c_enha, tmpbuf, G7291_LSUBFR, 15);
        ippsDotProd_16s32s_Sfs(tmpbuf,tmpbuf,G7291_LSUBFR,&L_temp2,0);
//        L_temp2 = 0;
//        for(i = 0; i < G7291_LSUBFR; i++)
//        {
//          temp = (short)(((g_c*code[i])>>15) + ((g_c_enha*code_enha[i])>>15));
//          L_temp2 += temp*temp;
//        }
        nb_celp_param->pFixPower[i0_2sub] = L_temp2;
        if (g_c_enha <= IPP_MAX_16S/2)
          ippsInterpolateC_NR_G729_16s_Sfs(code_enha, g_c_enha<<1, &exc[indexSubfr], 16384, &exc2[indexSubfr], G7291_LSUBFR, 14);
        else
          for(i = 0; i < G7291_LSUBFR; i++)
          {
            L_temp = code_enha[i]*(g_c_enha<<1) + exc[i+indexSubfr]*16384;
            exc2[indexSubfr+i] = Cnvrt_32s16s((L_temp+0x2000)>>14);
          }
      }

      temp = (pDecStat->sSubFrameCount+1)*3277;
      pDecStat->sDampedPitchGain = pDecStat->sDampedPitchGain + ((temp * g_p + 0x4000) >> 15);
      pDecStat->sDampedCodeGain = pDecStat->sDampedCodeGain + ((temp * Cnvrt_32s16s(g_c<<2) + 0x4000) >> 15);
      pDecStat->sTiltCode = (short)((voice_fac + 32767) >> 2);
      pDecStat->sSubFrameCount++;

      pA_t += G7291_G729_MP1;  /* interpolated LPC parameters for next subframe */
    }
  }

  /*------------*
   *  For G729b
   *-----------*/
  if(bfi == 0)
  {
    ippsDotProd_16s32s_Sfs(exc,exc,G7291_LFRAME4,&L_temp,-1);
    pDecStat->sh_sid_sav = Norm_32s_Pos_I(&L_temp);
    pDecStat->sid_sav = Cnvrt_NR_32s16s(L_temp);
    pDecStat->sh_sid_sav = 16 - pDecStat->sh_sid_sav;
  }

  /* Update signal for next frame. */
  ippsCopy_16s(&pExcVector[G7291_LFRAME4], &pDecStat->pExcVector[0], G7291_PITCH_LAG_MAX + G7291_L_INTERPOL);
  ippsCopy_16s(exc, coreExc, G7291_LFRAME4);
  ippsCopy_16s(exc2, extExc, G7291_LFRAME4);
  ippsCopy_16s(A_t_fwd, BfAq, 2 * G7291_G729_MP1);
  /* for G729b */
  pDecStat->past_ftyp = ftyp;

  return;
}

/*  Initialization of TDBWE decoder   */
void TDBWEDecoderInit(TDBWE_DecoderState *state)
{
  int i;
  state->sPrevGainTimeEnvelopeShaping = 0;
  state->sPrevNormTimeEnvelopeShaping = 0;

  ippsGenerateExcitationInit_G7291_16s(state->pGenExcState);
//  ippsLowPassFilterInit_G7291_16s(abLP3k, state->lowpass_3_kHz);
  ippsZero_8u((Ipp8u*)state->lowpass_3_kHz, 24);
  ippsZero_16s(state->pMemFltCoeffs, G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH);
  ippsZero_16s(state->pMemFreqEnvShapingFilter, G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH_M_1);
  ippsZero_16s(state->pMemExcOld, G7291_TDBWE_MEM_EXC);
  for (i=0; i<G7291_TDBWE_NB_SUBBANDS; i++) state->pMemFreqEnv[i]=0;

  state->pMemTimeEnv[0] = 0;
  state->pMemTimeEnv[1] = 0;
  return;
}

/*  TDBWE decoder (highband synthesis),
    generates the exication signal, shape time & frequency envelope and
    invokes postprocessing  */
void TDBWE_decoder(TDBWE_DecoderState *state, G729ParamsStruct *nb_celp_param, short *signal_highband,
                   short *parameters)
{
  IPP_ALIGNED_ARRAY(16, short, excitation, G7291_TDBWE_L_FRAME2);
  IPP_ALIGNED_ARRAY(16, short, interp, G7291_TDBWE_NB_SUBBANDS);
  IPP_ALIGNED_ARRAY(16, short, pMemExcOld, G7291_TDBWE_L_TOTAL_EXC);
  short *excitation_1, *tenv, *out, *fenv;
  int i;

  /* rearrange information on G729 NB excitation */
  excitation_1 = pMemExcOld + G7291_TDBWE_MEM_EXC;
  ippsCopy_16s(state->pMemExcOld, pMemExcOld, G7291_TDBWE_MEM_EXC);

  fenv = &parameters[G7291_TDBWE_PAROFS_FREQ_ENVELOPE];
  tenv = parameters;

  /* Interpolate frequency envelope for the first 10ms frame */
  for(i = 0; i < G7291_TDBWE_NB_SUBBANDS; i++) interp[i] = (state->pMemFreqEnv[i]+fenv[i])>>1;

  /* generate and shape synthetic excitation */
  out = signal_highband;
  for(i = 0; i < 2; i++)
  {
    /* excitation Q.12 */
    ippsGenerateExcitation_G7291_16s(&nb_celp_param->pPitch[i*2], &nb_celp_param->pPitchFrac[i*2],
        &nb_celp_param->pLtpPower[i*2], &nb_celp_param->pFixPower[i*2], excitation,
        state->pGenExcState);
    /* apply 3 kHz lowpass */
#ifdef _FAST_G7291
    ippsFilterLowpass_G7291_16s_I(excitation, G7291_TDBWE_L_FRAME2, state->lowpass_3_kHz, ippAlgHintFast);
#else
    ippsFilterLowpass_G7291_16s_I(excitation, G7291_TDBWE_L_FRAME2, state->lowpass_3_kHz, ippAlgHintAccurate);
#endif

    /* time envelope Q.10 */
    ippsShapeEnvelopTime_G7291_16s(excitation, tenv, &state->sPrevGainTimeEnvelopeShaping,
        &state->sPrevNormTimeEnvelopeShaping, excitation_1);
    /* excitation_1 Q.0 */
    if (i == 0)
        ippsShapeEnvelopFrequency_G7291_16s(excitation_1, interp, out, state->pMemFltCoeffs, state->pMemFreqEnvShapingFilter);
    else
        ippsShapeEnvelopFrequency_G7291_16s(excitation_1, fenv, out, state->pMemFltCoeffs, state->pMemFreqEnvShapingFilter);
    /* output Q.0 */

    /* invoke post-processing */
    ippsCompressEnvelopTime_G7291_16s(tenv, out, state->pMemTimeEnv);

    out += G7291_TDBWE_L_FRAME2;
    tenv += G7291_TDBWE_NB_SUBFRAMES_2;
    excitation_1 += G7291_TDBWE_L_FRAME2;
  }

  /* memory update */
  for (i=0; i<G7291_TDBWE_NB_SUBBANDS; i++) state->pMemFreqEnv[i]=fenv[i];
  ippsCopy_16s(&pMemExcOld[G7291_TDBWE_L_FRAME], state->pMemExcOld, G7291_TDBWE_MEM_EXC);
  return;
}

/*  TDAC decoder  */
void TDAC_decoder(
const short *bitstream,  /* (i)     TDAC bitstream */
int   *valBitCnt,
int nbit,                /* (i)     effective number of bits for TDAC */
int nbit_max,            /* (i)     maximal bit allocation for TDAC */
short *yt_hi,            /* (i)     MDCT spectrum of TDBWE synthesis */
short *yq_lo,            /* (o)     reconstructed MDCT spectrum of lower band */
short *yq_hi,            /* (o)     reconstructed MDCT spectrum of higher band */
int norm_MDCT)           /* (i)     MDCT normalization factor */
{
  IPP_ALIGNED_ARRAY(16, short, y, G7291_LFRAME);
  short rmsq[G7291_TDAC_NB_SB];  /* quantized spectrum envelope */
  short rms_index[G7291_TDAC_NB_SB];
  short ip[G7291_TDAC_NB_SB];
  short ord_b[G7291_TDAC_NB_SB];
  short bit_alloc[G7291_TDAC_NB_SB];
  short startbit[G7291_TDAC_NB_SB];
  unsigned int vqPrms[G7291_TDAC_NB_SB];

  const short *pBit;
  int fact, tmp32;
  int i, j, k;
  int tmp;
  short bit_cnt;
  short ndec_env_nb, ndec_env_wb;
  short Hi, Lo;
  short soi;
  short delta;
  short end_index;

  if (nbit > 0)
  {
    pBit = bitstream;
    bit_cnt = 0;

    /* - decode spectral envelope (subband rms)   */
    ownDecodeSpectralEnvelope_TDAC((short **)&pBit, valBitCnt, rms_index, rmsq, &bit_cnt,
        nbit, &ndec_env_nb, &ndec_env_wb, norm_MDCT);

    if((ndec_env_nb == G7291_TDAC_NB_SB_NB) && (ndec_env_wb == G7291_TDAC_NB_SB_WB))
    {
      /* compute the number of available bits */
      nbit_max -= bit_cnt;
      nbit -= bit_cnt;  /* number of bits left (at this stage) */

      /* compute perceptual importance:
         for 3dB step in spectral envelope coding
         1/2log2(enerq) = 1/2 ( rms_index + log2 nb_coef) */
      for (i=0; i<G7291_TDAC_NB_SB; i++) ip[i]=rms_index[i];
      tmp = G7291_TDAC_NB_SB-1;
      ip[tmp] = ip[tmp] + 1;

      /* order subbands by decreasing perceptual importance  */
      ownSort_TDAC(ip, ord_b);

      /* allocate bits to subbands */
      ownBitAllocation_TDAC(bit_alloc, nbit_max, ip, ord_b);

      /* force zero bit to match bit budget (loop by decreasing perceptuel importance) */
      bit_cnt = 0;
      for(j = 0; j < G7291_TDAC_NB_SB; j++)
      {
        tmp = bit_cnt + bit_alloc[ord_b[j]];
        if(tmp <= nbit)
        {
          bit_cnt = bit_cnt + bit_alloc[ord_b[j]];
        }
        else
        {
          /* bit budget is zero, set allocation to 0 bits for the other bands */
          for(i = j; i < G7291_TDAC_NB_SB; i++)
          {
            bit_alloc[ord_b[i]] = 0;
          }
        }
      }
      /* - decode MDCT coefficients */

      /* read bits for VQ decoding */
      soi = 0;
      for(j = 0; j < G7291_TDAC_NB_SB; j++)
      {
        startbit[j] = soi;
        soi = soi + bit_alloc[j];
      }

      for(j = 0; j < G7291_TDAC_NB_SB; j++)
      {
        vqPrms[ord_b[j]] = ownExtractBits((const unsigned char **)&pBit, valBitCnt, bit_alloc[ord_b[j]]);
      }

      ippsMDCTQuantInv_G7291_32u16s(vqPrms, bit_alloc, rmsq, y);

      /* - reconstruct lower- and higher-band spectra */

      /* split full-band spectrum */
      ippsCopy_16s(y, yq_lo, G7291_LFRAME2);
      ippsCopy_16s(&y[G7291_LFRAME2], yq_hi, G7291_LFRAME2);

      /* extrapolation (in wideband part) of unquantized subbands */
      delta = G7291_LFRAME2;
      for(j = G7291_TDAC_NB_SB_NB; j < G7291_TDAC_NB_SB; j++)
      {
        if (bit_alloc[j] == 0)
        {
          for (k = G7291_TDAC_sb_boundTbl[j]; k < G7291_TDAC_sb_boundTbl[j + 1]; k++)
          {
            tmp = k - delta;
            yq_hi[tmp] = yt_hi[tmp];
          }
          ippsDotProd_16s32s_Sfs(&yq_hi[G7291_TDAC_sb_boundTbl[j]-delta], &yq_hi[G7291_TDAC_sb_boundTbl[j]-delta],
              G7291_TDAC_sb_boundTbl[j+1]-G7291_TDAC_sb_boundTbl[j], &fact, 0);
          fact = Add_32s(fact, 16);
          fact >>= G7291_TDAC_nb_coef_divTbl[j];
          ippsInvSqrt_32s_I(&fact, 1);
          tmp32 = fact;
          L_Extract(tmp32, &Hi, &Lo);
          fact = Mpy_32_16(Hi, Lo, rmsq[j]);
          for (k = G7291_TDAC_sb_boundTbl[j]; k < G7291_TDAC_sb_boundTbl[j + 1]; k++)
          {
            tmp = k - delta;
            L_Extract(fact, &Hi, &Lo);
            tmp32 = Mpy_32_16(Hi, Lo, yq_hi[tmp]);
            yq_hi[tmp] = (short)tmp32;
          }
        }
      }

      /* zero coefficients in 7-8 kHz band */
      ippsZero_16s(&yq_hi[120], 40);
      return;
    }
    else
    {
      if (ndec_env_wb > 0)
      {
        /* level adjustment of unquantized subbands (in wideband part) */
        delta = G7291_LFRAME2;
        end_index = G7291_TDAC_NB_SB_NB + ndec_env_wb;
        for (j = G7291_TDAC_NB_SB_NB; j < end_index; j++)
        {
          ippsDotProd_16s32s_Sfs(&yt_hi[G7291_TDAC_sb_boundTbl[j]-delta], &yt_hi[G7291_TDAC_sb_boundTbl[j]-delta],
              G7291_TDAC_sb_boundTbl[j+1]-G7291_TDAC_sb_boundTbl[j], &fact, 0);
          fact = Add_32s(fact, 16);
          fact >>= G7291_TDAC_nb_coef_divTbl[j];
          ippsInvSqrt_32s_I(&fact, 1);
          tmp32 = fact;
          L_Extract(tmp32, &Hi, &Lo);
          fact = Mpy_32_16(Hi, Lo, rmsq[j]);
          for (k = G7291_TDAC_sb_boundTbl[j]; k < G7291_TDAC_sb_boundTbl[j + 1]; k++)
          {
            tmp = k - delta;
            L_Extract(fact, &Hi, &Lo);
            tmp32 = Mpy_32_16(Hi, Lo, yt_hi[tmp]);
            yt_hi[tmp] = (short)tmp32;
          }
        }
      }
    }
  }
  /* handle case: nbit = 0 */

  /* zero coefficients in 7-8 kHz band */
  ippsZero_16s(&yt_hi[120], 40);

  /* split full-band spectrum */
  ippsZero_16s(yq_lo, G7291_LFRAME2);
  ippsCopy_16s(yt_hi, yq_hi, G7291_LFRAME2);
  return;
}

/*  CELP2S post filter initialization  */
void PostFilterInit(PostFilterState *pfstat)
{
  int i;
  /* Initialize arrays and pointers */
  ippsZero_16s(pfstat->pMemResidual, G7291_MEM_RES2);

  /* 1/A(gamma1) memory */
  /* null memory to compute i.r. of A(gamma2)/A(gamma1) */
  for (i=0; i<G7291_M_LPC; i++) {
      pfstat->pMemSynthesis[i]=0;
      pfstat->pNullMemory[i]=0;
  }

  /* null memory to compute i.r. of A(gamma2)/A(gamma1) */
  for (i=0; i<G7291_LONG_H_ST; i++) {
      pfstat->pNumeratorCoeffs[i]=0;
  }

  /* for gain adjustment */
  pfstat->sPrevGain = 16384;
  pfstat->iSmoothLevel = 1024;
  return;
}
/****   Short term postfilter :
        Hst(z) = Hst0(z) Hst1(z)
        Hst0(z) = 1/g0 A(gamma2)(z) / A(gamma1)(z)
        if {hi} = i.r. filter A(gamma2)/A(gamma1) (truncated)
        g0 = SUM(|hi|) if > 1
        g0 = 1. else
        Hst1(z) = 1/(1 - |mu|) (1 + mu z-1)
        with mu = k1 * gamma3
        k1 = 1st parcor calculated on {hi}
        gamma3 = gamma3_minus if k1<0, gamma3_plus if k1>0
 ****   Long term postfilter :
        harmonic postfilter :   H0(z) = gl * (1 + b * z-p)
        b = gamma_g * gain_ltp
        gl = 1 / 1 + b
        computation of delay p on A(gamma2)(z) s(z)
        sub optimal search
        1. search around 1st subframe delay (3 integer values)
        2. search around best integer with fract. delays (1/8)  */

/* Post - adaptive postfilter main function */
void Post_G7291(
PostFilterState *pfstat,    /* (i/o)  stqtes strucure */
short t0,                   /* (i)    pitch delay given by coder */
short *signal_ptr,          /* (i)    input signal (pointer to current subframe */
short *coeff,               /* (i)    LPC coefficients for current subframe */
short *sig_out,             /* (o)    postfiltered output */
short gamma1,               /* (i)    short term postfilt. den. weighting factor */
short gamma2,               /* (i)    short term postfilt. num. weighting factor */
short PostNB,               /* (i)    Narrow Band flag    */
short Vad)                  /* (i)    VAD flag            */
{
  /* Local variables and arrays */
  IPP_ALIGNED_ARRAY(16, short, apond1, G7291_G729_MP1);  /* s.t. denominator coeff.      */
  IPP_ALIGNED_ARRAY(16, short, sig_ltp, G7291_L_SUBFRP1); /* H0 output signal             */
  IPP_ALIGNED_ARRAY(16, short, res2, G7291_SIZ_RES2);
  IPP_ALIGNED_ARRAY(16, short, h, G7291_LONG_H_ST);
  short *sig_ltp_ptr;
  short *res2_ptr, *ptr_mem_stp;
  int i, L_g0, status = 0, normVal = 0, irACF[2];
  short parcor0, g0, temp;
  short ACFval0, ACFval1;
  short voiceFlag = 0;

  irACF[0] = irACF[1] = 0;

  /* Init pointers and restore memories */
  res2_ptr = res2 + G7291_MEM_RES2;
  ptr_mem_stp = pfstat->pMemSynthesis + G7291_M_LPC - 1;

  ippsCopy_16s(pfstat->pMemResidual, res2, G7291_MEM_RES2);

  /* Compute weighted LPC coefficients */
  ippsMulPowerC_NR_16s_Sfs(coeff, gamma1, apond1, G7291_G729_MP1, 15);
  ippsMulPowerC_NR_16s_Sfs(coeff, gamma2, pfstat->pNumeratorCoeffs, G7291_G729_MP1, 15);

  /* Compute A(gamma2) residual */
  ippsResidualFilter_G729_16s(signal_ptr, pfstat->pNumeratorCoeffs, res2_ptr);

  /* Harmonic filtering */
  sig_ltp_ptr = sig_ltp + 1;
  if (Vad == 1)
    ippsLongTermPostFilter_G729_16s(16384, t0, res2_ptr, sig_ltp_ptr, &voiceFlag);
  else
    ippsCopy_16s(res2_ptr, sig_ltp_ptr, G7291_LSUBFR);
  /* Save last output of 1/A(gamma1)
     (from preceding subframe)  */
  sig_ltp[0] = *ptr_mem_stp;

  /* Controls short term pst filter gain and compute parcor0
     compute i.r. of composed filter pfstat->pNumeratorCoeffs / apond1 */
  ippsSynthesisFilter_NR_16s_Sfs(apond1, pfstat->pNumeratorCoeffs, h, G7291_LONG_H_ST, 12, pfstat->pNullMemory);

  /* compute 1st parcor */
  status = ippsAutoCorr_NormE_16s32s(h, G7291_LONG_H_ST, irACF, 2, &normVal);
  ACFval0   = (short)(irACF[0]>>16);
  ACFval1  = (short)(irACF[1]>>16);
  if( ACFval0 < Abs_16s(ACFval1) || status) {
      parcor0 = 0;
  } else {
      parcor0 = (short)((Abs_16s(ACFval1)<<15)/ACFval0);
      if(ACFval1 > 0) {
          parcor0 = (short)(-parcor0);
      }
  }
  /* compute g0 */
  ippsAbs_16s(h, h, G7291_LONG_H_ST);
  ippsSum_16s32s_Sfs(h, G7291_LONG_H_ST, &L_g0, 0);
  g0 = (short)(ShiftL_32s(L_g0, 14)>>16);

  /* Scale signal input of 1/A(gamma1) */
  if(g0 > 1024) {
      temp = (short)((1024<<15)/g0);
      ippsMulC_NR_16s_ISfs(temp, sig_ltp_ptr, G7291_LSUBFR, 15);
  }

  /* 1/A(gamma1) filtering, pMemSynthesis is updated */
  ippsSynthesisFilterLow_NR_16s_ISfs(apond1,sig_ltp_ptr,G7291_LSUBFR,12,pfstat->pMemSynthesis);
  for (i=0; i<G7291_LP_ORDER; i++)
    pfstat->pMemSynthesis[i]=sig_ltp_ptr[G7291_LSUBFR-G7291_LP_ORDER+i];

  /* Tilt filtering */
  ippsTiltCompensation_G7291_16s(sig_ltp, sig_out, parcor0);

  /* Gain control */
  ippsGainControl_G7291_16s_I(signal_ptr, sig_out, &pfstat->sPrevGain, parcor0, PostNB, &pfstat->iSmoothLevel);

    /**** Update for next subframe */
  ippsCopy_16s(&res2[G7291_LSUBFR], pfstat->pMemResidual, G7291_MEM_RES2);

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

static __ALIGN32 CONST short NumberBitsLayer2Tbl[6] = {
    /* Layer 2 - narrowband enhancement layer (embedded CELP */
                /* 10 ms frame 1 */
/* subframe1 */
   13,                                      /* 2-nd Fixed-codebook index: C1 */
   4,                                       /* 2-nd Fixed-codebook sign: S1 */
   3,                                       /* 2-nd Fixed-codebook gain: G1 */
/* subframe2 */
   13,                                      /* 2-nd Fixed-codebook index: C2 */
   4,                                       /* 2-nd Fixed-codebook sign: S2 */
   3                                        /* 2-nd Fixed-codebook gain: G2
                                               FEC bit (class information): CL1 */
//   2,                                       /* 2-nd Fixed-codebook gain: G2 */
//   1,                                       /* FEC bit (class information): CL1 */
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
static __ALIGN32 CONST short NumberBitsSIDTbl[4] = {
/* SID frame (15 bits) */
   1, 5, 4, 5
};

static void ownBits2Prms_G729B(const unsigned char **pBitstream, short* pPrms, short *ftyp)
{
    const unsigned char **pBits = pBitstream;
    int i, valBitCnt = 0;

    /* Get 8k bistream */
    if (ftyp[0] == 1) {/* --> first subframe (80 bits) */
        for( i = 0; i < 11; i++) {
            pPrms[i] = (short)ownExtractBits(pBits, &valBitCnt, NumberBitsLayer1Tbl[i]);
        }
    } else if (ftyp[0] == 2) {/* SID frame (15 bits) */
        for( i = 0; i < 4; i++) {
            pPrms[i] = (short)ownExtractBits(pBits, &valBitCnt, NumberBitsSIDTbl[i]);
        }
        ownExtractBits(pBits, &valBitCnt, 1);
    }

    if (ftyp[1] == 1) {/* --> second subframe (80 bits) */
        for( i = 0; i < 11; i++) {
            pPrms[i+17] = (short)ownExtractBits(pBits, &valBitCnt, NumberBitsLayer1Tbl[i]);
        }
    } else if (ftyp[1] == 2) {/* SID frame (15 bits) */
        for( i = 0; i < 4; i++) {
            pPrms[i+17] = (short)ownExtractBits(pBits, &valBitCnt, NumberBitsSIDTbl[i]);
        }
        ownExtractBits(pBits, &valBitCnt, 1);
    }
    return;
}

static void ownBits2Prms(const unsigned char **pBitstream, short* pPrms, short* ind_FER,
                         int* valBitCnt, int rate)
{
    const unsigned char **pBits = pBitstream;
    int i, tmp;

    if (rate >= 8000) {
       /* Get 8k bistream */
       /* --> first subframe (80 bits) */
       for( i = 0; i < 11; i++) {
          pPrms[i] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer1Tbl[i]);
       }
       /* --> second subframe (80 bits) */
       for( i = 0; i < 11; i++) {
          pPrms[i+17] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer1Tbl[i]);
       }
    }
    if (rate >= 12000) {
       /* Get 12k bitstream */
       /* --> first subframe (40 bits) */
       for( i = 0; i < 6; i++) {
          pPrms[i+11] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer2Tbl[i]);
       }
       /* --> second subframe (40 bits) */
       tmp = (short)(pPrms[16] >> 2);
       for( i = 0; i < 6; i++) {
          pPrms[i+28] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer2Tbl[i]);
       }
       /* --> get FEC information */
       ind_FER[0] = (short)(pPrms[33] >> 2);
       ind_FER[0] = (short)(ind_FER[0] | (tmp << 1));
    }
    if (rate >= 14000) {
       /* Get 14k bitstream (33 bits) */
       for( i = 0; i < 6; i++) {
          pPrms[i+34] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer3Tbl[i]);
       }
       ind_FER[2] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer3Tbl[6]);
    }
    if (rate >= 16000) {
    /* Get 16k+ bitstream (FEC : 5 bits ; norm_shift : 4 bits) */
       ind_FER[1] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer4Tbl[0]);
       pPrms[40] = (short)ownExtractBits(pBits, valBitCnt, NumberBitsLayer4Tbl[1]);
    }
    return;
}

