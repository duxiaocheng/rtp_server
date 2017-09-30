/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
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
// Purpose: G.729.1 speech codec: main own header file.
//
*/

#ifndef __OWNCODEC_H__
#define __OWNCODEC_H__

#include <ipps.h>
#include <ippsc.h>
#include "g7291api.h"
#include "aux_fnxs.h"
#include "scratchmem.h"

#define ENC_KEY 0xecd729
#define DEC_KEY 0xdec729

/*************************
 * Constants for framing
 *************************/

#define G7291_LFRAME         320
#define G7291_LFRAME2        160
#define G7291_LFRAME4         80
#define G7291_NB_SUBFR  4

/* Constants for bitstream packing */
#define G7291_MAX_BITRATE  32000
#define G7291_MAX_BITS_BST 640
#define G7291_COUNT_RCV_MAX 50

/* constants for TDAC module */
/* constants for MDCT and inverse MDCT */
#define G7291_TDAC_L_WIN   320
#define G7291_TDAC_L_WIN2  160
#define G7291_TDAC_L_WIN4   80

/* constants for spectral envelope coding */
#define G7291_TDAC_NB_SB          18
#define G7291_TDAC_NB_SB_NB       10
#define G7291_TDAC_NB_SB_WB        8

/* Constants for spectrum envelope quantization */
#define G7291_TDAC_MAX_LONG_HUFF   16  /* longueur max code d'Huffman */
#define G7291_TDAC_MIN_RMS      -11
#define G7291_TDAC_MAX_RMS      20
#define G7291_TDAC_RMS_OFFSET    6
#define G7291_TDAC_Q3db_INV     16384  /* 1 / 3dB in Q14 */
#define G7291_TDAC_Q3db        16384   /* 3dB in Q15 */
#define G7291_TDAC_N_NBC      16       /* Maximum number of coeff. (dim. of vectors)    */
#define G7291_TDAC_NORM_MDCT_TRESH  4

/* Constants for bit allocation */
#define G7291_TDAC_SORTED_IP    -2000  /* Q1 */

/* Constants for filtering */
/* number of taps for median filter (used to adapt postfilter's gammas) */
#define G7291_NMAX 5

/* perceptual weighting filtering of lower-band difference signal */
#define  G7291_GAMMA1     (short) 30802  /* Q15 */
#define  G7291_GAMMA2     (short) 19661  /* Q15 */

/* Constants for pre/post-echo reduction */
#define G7291_PRE_SUBFRAME 40 /* block for energy comparison in 8khz-samples: 5msec */

/* constants for CELP2S module */
#define G7291_CELP2S_SP          (short)   -11141
#define G7291_CELP2S_HP_FACT     (short)    -4915

/* constants for TDBWE module */
/* TD-BWE frame size constants */
#define G7291_TDBWE_L_FRAME    160
#define G7291_TDBWE_L_FRAME2    80
#define G7291_TDBWE_L_FRAME4    40

/* TDBWE buffering and pointer address constants */
#define G7291_TDBWE_L_TOTAL                             232
#define G7291_TDBWE_L_NEXT                              40
#define G7291_TDBWE_MEM_SPEECH                          72
#define G7291_TDBWE_DELTA_TIME_ENV                      16
#define G7291_TDBWE_DELTA_FREQ_ENV                      -32
#define G7291_TDBWE_L_TOTAL_EXC                         208
#define G7291_TDBWE_MEM_EXC                             48

/* TDBWE time envelope computation, shaping & postprocessing */

#define G7291_TDBWE_NB_SUBFRAMES         16
#define G7291_TDBWE_NB_SUBFRAMES_2       8
#define G7291_TDBWE_NB_SUBFRAMES_2_M_1   7
#define G7291_TDBWE_NB_SUBFRAMES_2_M_2   6
#define G7291_TDBWE_SUBFRAME_SIZE        10
#define G7291_TDBWE_SUBFRAME_SIZE_2      5

/* TDBWE frequency envelope computation, shaping & postprocessing */

#define G7291_TDBWE_FFT_NUM_STAGE     5
#define G7291_TDBWE_FFT_SIZE2         (2 << (G7291_TDBWE_FFT_NUM_STAGE+1))   /* 128 */
#define G7291_TDBWE_FFT_SIZE          (2 << G7291_TDBWE_FFT_NUM_STAGE)       /* 64  */
#define G7291_TDBWE_FFT_SIZE_M_2      ((2 << G7291_TDBWE_FFT_NUM_STAGE) - 2) /* 62  */
#define G7291_TDBWE_FFT_SIZE_BY_TWO   (2 << (G7291_TDBWE_FFT_NUM_STAGE-1))   /* 32  */

#define G7291_TDBWE_NB_SUBBANDS                         12
#define G7291_TDBWE_FREQUENCY_ENVELOPE_WINDOW_SIZE      (G7291_TDBWE_FFT_SIZE<<1) /* 128 */
#define G7291_TDBWE_FREQUENCY_ENVELOPE_WINDOW_SIZE_M_FS ((G7291_TDBWE_FFT_SIZE<<1)-G7291_TDBWE_L_FRAME2)
#define G7291_TDBWE_BINS_PER_SUBBAND                    3  /* 375 Hz */
#define G7291_TDBWE_BINS_BTW_CENTER_BINS                1

#define G7291_TDBWE_BUFLEN_FIR (80+32)
#define G7291_TDBWE_FREQ_ENV_SHAPING_STORAGE_LENGTH      17
#define G7291_TDBWE_FREQ_ENV_SHAPING_GAIN_HIGHPASS       26
#define G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH       33
#define G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH_M_1   32

/* TD-BWE parameter array offsets */
#define G7291_TDBWE_PAROFS_FREQ_ENVELOPE G7291_TDBWE_NB_SUBFRAMES
#define G7291_TDBWE_NB_PARAMETERS (G7291_TDBWE_PAROFS_FREQ_ENVELOPE+G7291_TDBWE_NB_SUBBANDS)

/* TD-BWE excitation generation */

#define G7291_TDBWE_LENGTH_PULSE_SHAPE   57
#define G7291_TDBWE_NUMBER_PULSE_SHAPES   6

/* TD-BWE quantization table constants */

#define G7291_TDBWE_MEAN_TIME_ENV_BITS                5
#define G7291_TDBWE_TIME_ENV_BITS                     7
#define G7291_TDBWE_TIME_ENV_SPLIT_VQ_NUMBER_SPLITS   2
#define G7291_TDBWE_FREQ_ENV_SPLIT_VQ_NUMBER_SPLITS   3

/* constants for FEC module */
#define G7291_FEC_SINGLE_FRAME    (-1)

/* attenuation strategy in case of FER */
#define G7291_FEC_ALPHA_S         19661
#define G7291_FEC_ALPHA_V         32767
#define G7291_FEC_ALPHA_VT        13107
#define G7291_FEC_ALPHA_UT        28836
#define G7291_FEC_ALPHA_U         13107
#define G7291_FEC_ALPHA_U2        31130
#define G7291_FEC_L_FIR_FER       5  /* impulse response length for low- & high-pass filters   */

#define G7291_FEC_MAX_UPD_CNT     30
#define G7291_FEC_L_EXC_MEM       (G7291_PITCH_LAG_MAX+G7291_L_INTERPOL)
#define G7291_FEC_QPIT            32
#define G7291_FEC_QPIT2           16
#define G7291_FEC_SQPIT           5

/* signal classification for FER protection */
#define G7291_FEC_UNVOICED          0  /* G7291_FEC_UNVOICED, silence, noise, G7291_FEC_VOICED offset   */
#define G7291_FEC_UV_TRANSITION     1  /* Transition from G7291_FEC_UNVOICED to G7291_FEC_VOICED components - possible onset, but too small */
#define G7291_FEC_V_TRANSITION      2  /* Transition from G7291_FEC_VOICED - still G7291_FEC_VOICED, but with very weak G7291_FEC_VOICED characteristics     */
#define G7291_FEC_VOICED            3  /* G7291_FEC_VOICED frame, previous frame was also G7291_FEC_VOICED or ONSET                   */
#define G7291_FEC_ONSET             4  /* G7291_FEC_VOICED onset sufficiently well built to follow with a G7291_FEC_VOICED concealments    */
#define G7291_FEC_SIN_ONSET         5  /* artificial harmonic+noise onset (used only in decoder)                       */

#define G7291_FEC_GOOD_FRAME      0  /* Correct frame                   */
#define G7291_FEC_BAD_FRAME       1  /* Bad frame                       */

#define G7291_FEC_LG10             24660   /* 10*log10(2)          in Q13 */
#define G7291_FEC_LG10_LFRAME      361123  /* 10*log10(160)        in Q14 */
#define G7291_FEC_LG10_LFRAME_155  465965  /* 5.0*log10(160)/1.55  in Q16 */
#define G7291_FEC_LG10_s1_55       31820   /* 5*log10(2)/1.55      in Q15 */

#define G7291_FEC_K_COR            23405       /* Q13      2.8547f                       */
#define G7291_FEC_C_COR            -690415993  /* Q13+16   -1.286f                       */
#define G7291_FEC_K_TILT           13653       /*Q14       0.8333f     <-0.35, 0.85>     */
#define G7291_FEC_C_TILT           313210491   /*Q14+16    0.2917f                       */
#define G7291_FEC_K_ZC_DEC         -2195       /*Q15       -0.067f     <63, 38>          */
#define G7291_FEC_C_ZC_DEC         171246      /*Q0+16     2.613       <63, 38>          */
#define G7291_FEC_K_ENR            18725       /*Q15       0.57143     <-14, 11>         */
#define G7291_FEC_C_ENR            14384953    /*Q8+16     0.85741f                      */
#define G7291_FEC_K_PC_DEC         -1927       /*Q15       -0.0588f    <45, 17>          */
#define G7291_FEC_C_PC_DEC         107925      /*Q0+16     1.6071f                       */
#define G7291_FEC_UNS6             10923/2

#define G7291_FEC_AGC              31470

#define G7291_FEC_L_FRAME_FER      G7291_LFRAME2
#define G7291_FEC_L_FRAME2_FER     (G7291_FEC_L_FRAME_FER/2)
#define G729_FEC_L_MEM            (G7291_LFRAME2+G7291_FEC_L_FIR_FER)
#define G7291_FEC_PIT_MAX32        (short)(G7291_PITCH_LAG_MAX*32)
#define G7291_FEC_PIT_MIN32        (short)(G7291_PITCH_LAG_MIN*32)

#define G7291_FEC_K_EE             20480 /*Q13 */
#define G7291_FEC_C_EE             -41943040
#define G7291_FEC_K_RELE           1638  /*Q15 */
#define G7291_FEC_C_RELE           14746 /*Q15 */
#define G7291_FEC_K_SNR            3151  /*Q15 */
#define G7291_FEC_C_SNR            -8192 /*Q15 */
#define G7291_FEC_K_PC_ENC         -3854 /*Q15 */
#define G7291_FEC_C_PC_ENC         131072/*Q16 */
#define G7291_FEC_K_ZC_ENC         -3277 /*Q15 */
#define G7291_FEC_C_ZC_ENC         16384 /*Q13 */

/*--------------------------------------------------------------------------*
 *       Codec constant parameters (coder, decoder, and postfilter)         *
 *--------------------------------------------------------------------------*/
#define G7291_L_TOTAL     240 /* Total size of speech buffer.              */
#define G7291_L_WINDOW    240 /* Window size in LP analysis.               */
#define G7291_L_NEXT      40  /* Lookahead in LP analysis.                 */
#define G7291_LSUBFR            40  /* Subframe size.                            */
#define G7291_LP_ORDER           10  /* Order of LP filter.                       */
#define G7291_G729_MP1         (G7291_LP_ORDER+1) /* Order of LP filter + 1        */
#define G7291_PITCH_LAG_MIN     20  /* Minimum pitch lag.                        */
#define G7291_PITCH_LAG_MAX     143 /* Maximum pitch lag.                        */
#define G7291_L_INTERPOL  (10+1)  /* Length of filter for interpolation.       */

#define G7291_PRM_SIZE    11  /* Size of vector of analysis parameters.    */

#define G7291_SHARPMAX    13017 /* Maximum value of pitch sharpening 0.8 Q14 */
#define G7291_SHARPMIN    3277  /* Minimum value of pitch sharpening 0.2 Q14 */

#define G7291_L_SUBFRP1  (G7291_LSUBFR + 1)
#define G7291_GPCLIP      15564 /* Maximum pitch gain if taming is needed Q14 */

#define G7291_MEM_SPEECH  (G7291_L_TOTAL - G7291_LFRAME4)
#define G7291_M_LPC       G7291_LP_ORDER
#define G7291_M_LPC1      (G7291_M_LPC+1)

/* LSP quantizer constant parameters */
#define G7291_NC          5 /*  NC = M/2                                 */
#define G7291_MA_NP             4 /* MA prediction order for LSP               */
#define G7291_MODE        2 /* number of modes for MA prediction         */
#define G7291_NC0_B       7 /* number of first stage bits                */
#define G7291_NC1_B       5 /* number of second stage bits               */
#define G7291_NC0               (1<<G7291_NC0_B)  /* number of entries in first stage          */
#define G7291_NC1               (1<<G7291_NC1_B)  /* number of entries in second stage         */

#define G7291_L_LIMIT     40  /* minimum lsf value Q13:0.005               */
#define G7291_M_LIMIT     25681 /* maximum lsf value Q13:3.135               */

#define G7291_GAP1        10  /* bandwidth expansion factor Q13            */
#define G7291_GAP2        5 /* bandwidth expansion factor Q13            */
#define G7291_GAP3        321 /* bandwidth expansion factor Q13            */

/* gain VQ constants */
#define G7291_NCODE2_B  4 /* number of Codebook-bit                */
#define G7291_NCODE2    (1<<G7291_NCODE2_B) /* Codebook 2 size           */

/* Postfilter constants */
/* short term pst parameters :                                              */
#define G7291_GAMMA1_PST  22938 /* denominator weighting factor   (Q15)      */
#define G7291_GAMMA2_PST  18022 /* numerator  weighting factor (Q15)         */
#define G7291_GAMMA1_PST12K   24576 /* denominator weighting factor 14K (Q15) */
#define G7291_GAMMA2_PST12K   22938 /* numerator  weighting factor  14K (Q15) */
#define G7291_LONG_H_ST   20  /* impulse response length                   */
#define G7291_GAMMA3_PLUS 6554  /* tilt weighting factor when k1>0 (Q15)     */
#define G7291_GAMMA3_MINUS 29491  /* tilt weighting factor when k1<0 (Q15)     */

/* long term pst parameters :                                               */
#define G7291_L2_LH2_L    4 /* log2(LH2_L)                               */
#define G7291_LH2_L       (1 << G7291_L2_LH2_L)
#define G7291_LH_UP_L     (G7291_LH2_L/2)

/* Array sizes */
#define G7291_MEM_RES2    (G7291_PITCH_LAG_MAX + 1 + G7291_LH_UP_L)
#define G7291_SIZ_RES2    (G7291_MEM_RES2 + G7291_LSUBFR)

/***************************************************************************************** */
#define G7291_PRM_SIZE_MAX    17  /* number of parameters per 10 ms frame      */
#define G7291_MEM_LEN_TAM         150 /* Maximum pitch is not enough, 2nd subframe can be biger then 143 */
#define G7291_LEN_MEMTAM_UPDATE_UPDATE (G7291_MEM_LEN_TAM-G7291_LFRAME4)
#define G7291_LEN_MEMTAM_UPDATE_TAM (G7291_MEM_LEN_TAM-G7291_LSUBFR)
#define G7291_MAXTAMLIM 15360 /* 3*40*128 */
#define G7291_MINTAMLIM 10240 /* 2*40*128 */
#define G7291_DIFF_MAXMINTAMLIM   5120  /* 40*128 */
#define INIT_SEED       11111

/* DTX constants */
#define A_GAIN0         28672
#define A_GAIN1         4096    /* 32768L - A_GAIN0 */

/* CNG excitation generation constant */
                                /* alpha = 0.5 */
#define FRAC1           19043   /* (sqrt(40)xalpha/2 - 1) * 32768 */
#define K0              24576   /* (1 - alpha ** 2) in Q15        */
#define G_MAX           5000

typedef struct
{
  int   L_exc_err[4];                          /* To tame the coder (4)*/
  short pSpeechVector[G7291_MEM_SPEECH];       /* Speech vector (160)*/
  short pPrevLSFVector[G7291_MA_NP][G7291_LP_ORDER];  /* previous LSP vector  (4*10)*/
  short pExcVector[G7291_PITCH_LAG_MAX + G7291_L_INTERPOL];  /* Excitation vector (154)*/
  short pLARMem[2];                            /* LAR memory        (2)*/
  short pWgtSpchVector[G7291_PITCH_LAG_MAX];   /* Weighted speech vector (143)*/
  short pLPCCoeffs[G7291_G729_MP1];            /* Last LPC coefs  (11)*/
  short pReflectCoeffs[2];                     /* Reflect coeffs   (2)*/
  short pLSPVector[G7291_LP_ORDER];            /* Line Spectral Pairs (10)*/
  short pLSPQntVector[G7291_LP_ORDER];         /* Quantified LSP (10)*/
  short pSynthMem[G7291_M_LPC];                /* Filter's memory (10)*/
  short pSynthMem0[G7291_M_LPC];               /* Filter's memory (10)*/
  short pSynthMem1[G7291_M_LPC];               /* Filter's memory (10)*/
  short pSynthMem2[G7291_M_LPC];               /* Filter's memory (10)*/
  short pSynthMem3[G7291_M_LPC];               /* Filter's memory (10)*/
  short pSynthMemErr1[G7291_M_LPC];            /* Filter's memory (10)*/
  short pSynthMemErr2[G7291_M_LPC];            /* Filter's memory (10)*/
  short pPastQntEnergy[4];                     /* Past quantized energies (4)*/
  int   rate;                                  /* Rate */
  short sPitchSharp;
  short sSmooth;
  short sPitch;
} G729_EncoderState; /* 1240 bytes */

/* Structure of the decoder status variables */
typedef struct
{
  short pExcVector[G7291_PITCH_LAG_MAX + G7291_L_INTERPOL]; /* Excitation vector (154)*/
  short pMemTam[G7291_MEM_LEN_TAM];                         /* memory Tam        (150)*/
  short pPastQntEnergy[4];                                  /* Past quantized energies (4)*/
  short pPrevLSFVector[G7291_MA_NP][G7291_LP_ORDER];        /* previous LSP vector (4*10)*/
  short pLSPVector[G7291_LP_ORDER];                         /* Line Spectral Pairs (10)*/
  short pSynthMem[G7291_M_LPC];                             /* Filter's memory (10)*/
  short pLSFVector[G7291_LP_ORDER];                         /* previous LSP vector (10)*/
  short pFracPitch[2];
  short pMemPitch[5];
  int   rate;                      /* Rate */
  int   iEnergy;
  short sFERSeed;
  short sPitchSharp;               /* Pitch sharpening of previous frame */
  short sGainCode;                 /* Code gain */
  short sGainPitch;                /* pitch gain   */
  short sMAPredCoef;               /* Previous MA prediction coef */
  short sDampedPitchGain;
  short sDampedCodeGain;
  short sSubFrameCount;
  short sTiltCode;
  short sStabFactor;
  short sUpdateCounter;
  short sFracPitch;
  short sBFICount;
  short sBFI;
  /* G.729B mode */
  short g729b_bst;
  short ftyp[G7291_MODE];
  short sid_sav;
  short sh_sid_sav;
  short sid_gain;
  short cur_gain;
  int   L_exc_err[G7291_MA_NP];       /* To tame the coder */
  short past_ftyp;
  short seed;
  short noise_fg[G7291_MODE][G7291_MA_NP][G7291_M_LPC];
  short lspSid[G7291_M_LPC];

} G729_DecoderState;

typedef struct
{
  IppsGenerateExcitationState_G7291_16s* pGenExcState;
//  IppsLowPassFilterState_G7291_16s* lowpass_3_kHz;
  Ipp16s lowpass_3_kHz[12];
  short pMemFreqEnvShapingFilter[G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH_M_1]; /* Filter's memory (32)*/
  short pMemExcOld[G7291_TDBWE_MEM_EXC];      /* Excitation's memory (48)*/
  short pMemFreqEnv[G7291_TDBWE_NB_SUBBANDS]; /* FreqEnv's memory    (48)*/
  short pMemFltCoeffs[G7291_TDBWE_FREQ_ENV_SHAPING_FILTER_LENGTH]; /* Filter's memory (33)*/
  short pMemTimeEnv[2];                       /* TimeEnv's memory     (2)*/
  short sPrevGainTimeEnvelopeShaping;
  short sPrevNormTimeEnvelopeShaping;
} TDBWE_DecoderState;

typedef struct
{
  int   pFixPower[G7291_NB_SUBFR]; /* fix power ratio (4)*/
  int   pLtpPower[G7291_NB_SUBFR]; /* ltp power ratio (4)*/
  short pLPC[G7291_NB_SUBFR * G7291_G729_MP1]; /* LPC buffer (4*11)*/
  short pPitch[G7291_NB_SUBFR];                /* int pitch  (4)*/
  short pPitchFrac[G7291_NB_SUBFR];            /* frac pitch (4)*/
} G729ParamsStruct;

typedef struct
{                               /* memory state structure */
  short pMemResidual[G7291_MEM_RES2];      /* Residual's memory (152)*/
  short pNumeratorCoeffs[G7291_LONG_H_ST]; /* Numerator coeffs   (20)*/
  short pMemSynthesis[G7291_M_LPC];        /* Synthesis's memory (10)*/
  short pNullMemory[G7291_M_LPC];          /* Filter's memory    (10)*/
  int   iSmoothLevel;
  short sPrevGain;
} PostFilterState;

typedef struct _G7291Encoder_Obj
{
  /* filter states */
  G729_EncoderState pG729State;           /* G.729 state variables (1240 bytes)*/
  IppsQMFState_G7291_16s* pMemQMFana;     /* QMF analysis filterbank */
  IppsFilterHighpassState_G7291_16s* pMemHP50;/* lower band (50 Hz high-pass) state variables */
//  IppsLowPassFilterState_G7291_16s* pMemLP3k;/* higher band (3 kHz low-pass filter) state variables */
  Ipp16s pMemLP3k[12];                      /* higher band (3 kHz low-pass filter) state variables */
  short pMDCTCoeffsLow[G7291_LFRAME2];    /* memory of MDCT (160)*/
  short pMDCTCoeffsHigh[G7291_LFRAME2];   /* memory of MDCT (160)*/
  short pMemTDBWESpeech[G7291_TDBWE_MEM_SPEECH]; /* memory of TDBWE (72)*/
  short pWeightFltInput[G7291_LP_ORDER];  /* states for perceptual weighting filtering (10)*/
  short pWeightFltOutput[G7291_LP_ORDER]; /* of lower-band difference signal (10)*/
  int   freq;                             /* encoder flag (8kHz sampled input) */
  short sFERClassOld;                     /* states variables of FER encoder */
  short sLPSpeechEnergu;                  /* states variables of FER encoder */
  short sRelEnergyMem;                    /* states variables of FER encoder */
  short sRelEnergyVar;                    /* states variables of FER encoder */
  short sPosPulseOld;                     /* states variables of FER encoder */
} G7291Encoder_Obj;

/* Types : */
typedef struct _G7291Decoder_Obj
{
  IppsQMFState_G7291_16s*      pMemQMFsyn;/* QMF synthesis filterbank */
  IppsFilterHighpassState_G7291_16s* pMemHPflt;/* high-pass filtered state variables */
//  IppsLowPassFilterState_G7291_16s* pMemLP3k;/* higher band (3 kHz low-pass filter) state variables */
  Ipp16s pMemLP3k[12];                      /* higher band (3 kHz low-pass filter) state variables */
  int   freq;                             /* 8kHz sampled input flag */
  G729_DecoderState pG729State;           /* state variables of G.729 decoder (808 bytes)*/
  TDBWE_DecoderState pTDWBEState;         /* state variables of TDBWE decoders (268 bytes)*/
  int   prevRate;                         /* rate of previous frame*/
  G729ParamsStruct pG729ParamPrev;        /* previous G.729 parameters (for FEC) (136 bytes)*/
  PostFilterState pPstFltState;           /* lower-band postfiltering (392 bytes)*/
  int   iLowDelayFlg;                     /* low-delay mode flag */
  int   iPrevBFI;                         /* bfi of previous frame */
  int   iPrevMaxEnergyLow;                /* MaxEnergyLow of previous frame */
  int   iPrevMaxEnergyHigh;               /* MaxEnergyHigh of previous frame */
  short pMDCTCoeffsHigh[G7291_LFRAME2];    /* memory of MDCT (160)*/
  short pInvMDCTCoeffsLow[G7291_LFRAME2];  /* memory of MDCT (160)*/
  short pInvMDCTCoeffsHigh[G7291_LFRAME2]; /* memory of MDCT (160)*/
  short pG729OutputOld[G7291_LFRAME2];     /* g729 output (160)*/
  short pTdbweHighOld[G7291_LFRAME2];      /* tdbwe high  (160)*/
  short pMemLPC[G7291_NB_SUBFR*G7291_G729_MP1*G7291_G729_MP1]; /* LPC coefficients (4*11*11)*/
  short pSynthBuffer[G7291_PITCH_LAG_MAX]; /* synth buffer (143)*/
  short pMemAdaptGamma[G7291_NMAX]; /* gamma adaptation memory for lower-band postfilter (5)*/
  short pTDBWEPrmsOld[G7291_TDBWE_NB_PARAMETERS]; /* tdbwe parameters (28)*/
  short pMemPitchDelay[G7291_NB_SUBFR];   /* pitch delay given by coder (4)*/
  short pMemPstFltInput[G7291_M_LPC];     /* Post Filter memory (10)*/
  short pInvWeightFltInput[G7291_LP_ORDER]; /* states for inverse perceptual weighting filtering (10)*/
  short pInvWeightFltOutput[G7291_LP_ORDER];/* of lower-band difference signal (10)*/
  /* state variables for FEC */
  int   g729b;                              /* g729b mode flag */
  short sSmoothHigh;
  short sSmoothLow;
  short sFirstFrameFlg;
  short sNumRcvHigh;
  short sLPSpeechEnergyFER;
  short sLastGood;
  short sLPEnergyOld;
  short sPrevLastGood;
  short sRcvCount;                 /* variable for bit-rate switching */
  short sPostProcFlg;              /* post processing flag*/
} G7291Decoder_Obj;

/* Functions : */
int apiG7291Encoder_Alloc(G729Codec_Type codecType, int *pCodecSize);
int apiG7291Encoder_Init(G7291Encoder_Obj *codstat, int rate, int freq);
int apiG7291Encode(G7291Encoder_Obj * codstat, const short *samplesIn, unsigned char *itu_192_bitstream, int rate);

int apiG7291Decoder_Alloc(G729Codec_Type codecType, int *pCodecSize);
int apiG7291Decoder_Init(G7291Decoder_Obj * pDecStatH, int freq, int outMode, int lowDelay);
int apiG7291Decode(G7291Decoder_Obj *pDecStatH, const short *itu_192_bitstream,
                             short *samplesOut, int rate, int bfi);
int ownExtractBits( const unsigned char **pBits, int *nBit, int Count );
void _ippsRCToLAR_G7291_16s (const Ipp16s* pSrc, Ipp16s* pDst, int len);
void _ippsPWGammaFactor_G7291_16s ( const Ipp16s *pLAR, const Ipp16s *pLSF,
                                   Ipp16s *flat, Ipp16s *pGamma1, Ipp16s *pGamma2, Ipp16s *pMem );
short calcErr_G7291(int val, int *pSrc);
void updateExcErr_G7291( short val, int indx, int *pSrcDst);
void ownSynthesys_G7291(short *pExcCore, short *pExcExt, short *pExcMem, short *BfAq,
              short *pSynth, short *pSynthMem, int extrate);
short ownFindVoiceFactor(const short *pSrcPitchExc, const short *pSrcGains, const short *code);
void Log2(int L_x, short *logExponent, short *logFraction);
int Pow2(short exponent, short fraction);
void ownQuantization_TDBWE_16s(short *pSrcDst, short *pCdbkIndices);
void ownDeQuantization_TDBWE_16s(short *pCdbkIndices, short *pSrcDst);
void ownCalcSpecEnv_TDAC(short *y, short *log_rms, int numSubBands, int norm_lo, int norm_hi,
                      int norm_MDCT);
void ownEncodeSpectralEnvelope_TDAC(short *log_rms, short *rms_index, short *coder_param_ptr,
                                 short *nbit_env, int *bit_cnt, int nbit_to_encode,
                                 int norm_MDCT);
void ownSort_TDAC(short *ip, short *pos);
void ownBitAllocation_TDAC(short *bit_alloc, int nbit_max, short *ip, short *ord_b);
void ownDecodeSpectralEnvelope_TDAC(short **bitstream, int *valBitCnt, short *rms_index, short *rmsq,
                      short *bit_cnt, int nbit_to_decode, short *ndec_nb, short *ndec_wb,
                      int norm_MDCT);
void ownSetAdaptGamma(short *output_lo, short *pMemAdaptGamma, int rate, short *ga1_post, short *ga2_post);
void ownDiscriminationEcho(short *rec_sig, short *imdct_mem, int *max_prev, short *begin_ind, short *end_ind);
void ownEchoReduction(short *sOut_low, short *sBasic_low, short *sTdacAdd_low, short *sSmoothLow, short *sOut_hi,
                   short *sBasic_hi, short *sTdacInc_hi, short *sSmoothHigh, short work, short begin_index_low,
                   short end_index_low, short begin_index_high, short end_index_high);
void ownUpdatePitch_FEC(short clas, short *pitch_buf, short last_good, short *sUpdateCounter, short *sFracPitch, short *pFracPitch);
void ownUpdateMemTam(G729_DecoderState *pDecStat, short CoefLtp, short Pitch);
void ownUpdateAdaptCdbk_FEC(short *exc, short puls_pos, short *pitch_buf, int enr_q, short *Aq);
short ownDecodeInfo_FEC(short *last_good, short sPrevLastGood, const int iPrevBFI, short *indice, int *enr_q, int Rate);
void ownSignalClassification_FEC(short *clas, const short *pitch, const short last_good, short *frame_synth, short *st_old_synth, short *sLPSpeechEnergu);
void ownDecode_FEC(short *parm, short *pSynth, short *Az_dec, int bfi, short *last_good, short *pitch_buf,
                short *pExcVector, G729_DecoderState *st, short puls_pos);
void ownEncode_FEC(const short old_clas, const short *voicing, const short *speech, const short *pSynth, const short *T_op,
               const short *ee, const short *sync_sp, short *clas, short *coder_parameters, const short *delay,
               const short *res, short *sLPSpeechEnergu, short *rel_hist, short *rel_var, short *old_pos);
void ownScaleSynthesis_FEC(int extrate, short clas, const short last_good, short *pSynth, const short *pitch,
                        int Lenr_q, const int iPrevBFI, short *Aq, short *old_enr_LP, short *mem_tmp,
                        short *exc_tmp, int iEnergy, short *pExcVector, short *pSynthMem, short sBFICount);
short ownTamFer(short *pMemTam, short CoefLtp, short Pitch);
void ownArrangePatterns(const short *pPatternPos, const short *pPatternSign, short AmplSidePulse, short *pFCBvector);
void ownDecodeAdaptCodebookDelays(short index, short indexSubfr, short *delayVal);
void ownWeightingFilterFwd_G7291_16s(const short *pLPC, short *pSrcDst1, short *pSrcDst2, short *pDiff);
void ownWeightingFilterInv_G7291_16s(const short *pLPC, short *pSrcDst1, short *pSrcDst2, short *pDiff);
void ownOpenLoopPitchSearch_G7291_16s(const Ipp16s *pSrc, Ipp16s *pVoicing, Ipp16s *pBestLag);
void CNG_Decoder_G729B(G729_DecoderState *pDecStat, short *parm, short *exc, short *A_t, short ftyp);

extern CONST short G7291_switching_gainTbl[G7291_COUNT_RCV_MAX + 1];
extern CONST short LUT1Tbl[8];
extern CONST short LUT2Tbl[16];
extern CONST short NormTG729_1[256];
extern CONST short NormTG729_12[256];
extern CONST short G7291_GainEnha2bitsTbl[4];
extern CONST short G7291_GainEnhaTbl[8];
extern CONST short FreqPrevResetTbl[G7291_LP_ORDER];
extern CONST short tab_Sidgain[32];
extern CONST short fg[2][G7291_MA_NP][G7291_LP_ORDER];


/* TDAC Tables : */
extern CONST short G7291_Rate8Tbl[8];
extern CONST short G7291_Rate16Tbl[20];
extern CONST short G7291_Rate16Tbl_ind[33];
extern CONST short G7291_Rate8Tbl_ind[17];
extern CONST short G7291_TDAC_hTbl[G7291_TDAC_L_WIN];
extern CONST short G7291_TDAC_sb_boundTbl[G7291_TDAC_NB_SB + 1];
extern CONST short G7291_TDAC_nb_coefTbl[G7291_TDAC_NB_SB];
extern CONST short G7291_TDAC_nb_coef_divTbl[G7291_TDAC_NB_SB];
extern CONST short *G7291_adRateTbl[G7291_TDAC_N_NBC + 1];
extern CONST short *G7291_adRateTbl_ind[G7291_TDAC_N_NBC + 1];
extern CONST short G7291_NbDicTbl[G7291_TDAC_N_NBC + 1];

/* Hamming_cos window for LPC analysis */
extern CONST short HammingWindowTbl[G7291_L_WINDOW];

/* FEC tables: */
extern CONST short G7291_FEC_h_lowTbl[5];
extern CONST short G7291_FEC_h_highTbl[5];
extern CONST short sqiTbl[7];
extern CONST short invSqiTbl[7];
extern CONST short Sqrt_han_wind8kTbl[G7291_LFRAME4 + 1];

/* TDBWE tables: */
extern CONST short G7291_TDBWE_MEAN_TIME_ENV_cbTbl[32];
extern CONST short G7291_TDBWE_TIME_ENV_cbTbl[1024];
extern CONST short G7291_TDBWE_FREQ_ENV_NUMBER_ENTRIESTbl[3];
extern CONST short *G7291_TDBWE_FREQ_ENV_cbTbl[3];
extern CONST int abLP3k[10];

__INLINE Ipp32s equality( Ipp32s val) {
    Ipp32s temp, i, bit;
    Ipp32s sum;
    sum = 1;
    temp = val >> 1;
    for(i = 0; i <6; i++) {
        temp >>= 1;
        bit = temp & 1;
        sum += bit;
    }
    sum &= 1;
    return sum;
}

__INLINE void L_Extract(int L_32, short *hi, short *lo)
{
    *hi = (short)(L_32 >> 16);
    *lo = (short)((L_32>>1)&0x7fff);
    return;
}

__INLINE int Mpy_32(short hi1, short lo1, short hi2, short lo2)
{
  return ((hi1*hi2)<<1) + ((((hi1*lo2)>>15) + ((lo1*hi2)>>15) )<<1);
}

#endif /*__OWNCODEC_H__*/
