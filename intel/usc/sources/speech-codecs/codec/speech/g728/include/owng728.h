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
// Purpose: G.728 speech codec: internal definitions.
//
*/

#ifndef __OWNG728_H__
#define __OWNG728_H__

#if defined( _WIN32_WCE)
#pragma warning( disable : 4505 )
#endif

#include <ipps.h>
#include "g728api.h"

#include "scratchmem.h"

#define ENC_KEY 0xecd728
#define DEC_KEY 0xdec728

#define   G728_CODECFUN(type,name,arg)                extern type name arg

#define G728_TRUE           1
#define G728_FALSE          0

#define G728_J_FRAMESIZE_SAMPLES 80
/* G728 Constants */

#define AGCFAC     16220 /* AGC adaptation speed controlling factor */
#define AGCFAC1    20972 /*  The value of (1 - AGCFAC)*/
#define IDIM           5 /*  Vector dimension (excitation block size)*/
#define GOFF       16384 /*  Log-gain offset value*/
#define KPDELTA        6 /*  Allowed deviation from previous pitch period*/
#define KPMIN         20 /*  Minimum pitch period (samples)*/
#define KPMAX        140 /*  Maximum pitch period (samples)*/
#define LPC           50 /*  Synthesis filter order*/
#define LPCLG         10 /*  Log-gain predictor order*/
#define LPCW          10 /*  Perceptual weighting filter order*/
#define NCWD         128 /*  Shape codebook size (number of codevectors)*/
#define NCWD3_4      96
#define NCWD4        32
#define NFRSZ         20 /*  Frame size (adaptation cycle size in samples)*/
#define NG             8 /*  Gain codebook size (number of gain levels)*/
#define NONR          35 /*  Number of non-recursive window samples for synthesis filter*/
#define NONRLG        20 /*  Number of non-recursive window samples for log-gain predictor*/
#define NONRW         30 /*  Number of non-recursive window samples for weighting filter*/
#define NPWSZ        100 /*  Pitch analysis window size (samples)*/
#define NUPDATE        4 /*  Predictor update period (in terms of vectors)*/
#define PPFTH       9830 /*  Tap threshold for turning off pitch postfilter*/
#define PPFZCF      9830 /*  Pitch postfilter zero controlling factor*/
#define TAPTH      26214 /*  Tap threshold for fundamental pitch replacement*/
#define TILTF       4915 /*  Spectral tilt compensation controlling factor*/

#define REXPWMEMSIZE (8+LPCW+1+1+60)
#define REXPLGMEMSIZE (8+LPCLG+1+1+34)
#define REXPMEMSIZE (8+LPC+1+1+105+21)
#define COMBMEMSIZE (LPC+11 + 2*(LPCW+1))
#define IIRMEMSIZE (2*LPCW)
#define STPMEMSIZE (2*LPCW)
#define SYNTHMEMSIZE (LPC+11)

/* Rate 12.8 specific*/

#define NG_128         4 /* Gain codebook size (number of gain levels)*/
#define NG_96          4 /* Gain codebook size (number of gain levels)*/

#define ST_BUFF_SIZ

/* G728I coder specific*/
#define FESIZE         4 /* Number of 2.5 msec adaptation cycles in a frame erasure (FE)
                            e.g.: 1 for 2.5 msec FEs, 4 for 10 msec FEs.
                            FEs may be any multiple of 2.5 msec. SET BY USER*/

#define AFTERFEMAX    16 /* Maximum 2.5 ms frames for gain clipping */

// G728J specific
#define LOG_POL_ORDER        5 // Order of logarithmic approximation polynomial
#define BITS_PER_LEVEL       4 // Number of bits, that identify quantization levels
#define LEVEL_MASK        0x0F // Mask for level bits
#define PREDICTOR_ORDER     10 // Order of data-mode predictor
#define CODEBOOK_Q          11 // The Q presentation of the codebook
#define LOG_2            24660 // 10*log10(2) in Q13
#define RMS_BUF_LEN          8 // Length of RMS calculation
#define BLOCK_LEN            5 // Trellis block length
#define MAX_NUMBER  0x7fffffff // Maximum positive number
#define N_BRANCHES           2 // Number of branches emanating from or incoming to each trellis state
#define N_STATES             4 // Number of Trellis states
#define Q_CELLS             16 // Number of Quantization levels in each subset
#define TCQ_Kd               5 // Trellis delay length
#define TCQ_Kr               5 // Trellis release role
#define TCQ_DEPTH   (BLOCK_LEN - 1) // Trellis block length - 1
#define G_CONST              5 // Used for the calculation of G_ave in the Gain Compensation module
#define ATMP_CONST           3 // A threshold for detection of narrow bandwidth signal in the Signal Classifier
#define G_TRS             1800 // Gain Compensation Gdiff threshold
#define G_LEN               80 // The period of time in which  the Gain was steady prior to activation of Gain Compensation
#define GC_NLS_LIMIT_INIT    7 // Gain Compensation limiter
#define GC_COMPENSATION_INIT 3 // The value subtracted from the gain NLS when a Gain Compensation occurs
#define GC_CNT_INIT         11 // The period of time in which the Gain Compensation is active


void VscaleFive_16s(const Ipp16s* pSrc, Ipp16s* pDst, Ipp16s rangeScale, Ipp16s* pScale);
void VscaleOne_Range30_32s(const Ipp32s* pSrc, Ipp32s* pDst, Ipp16s* pScale);

void Divide_16s(Ipp16s num, Ipp16s numnls, Ipp16s den, Ipp16s dennls, Ipp16s *quo, Ipp16s *quonls);

void LevinsonDurbin(const Ipp16s *rtmp, Ipp32s ind1, Ipp32s ind2, Ipp16s *pTmpSynFltrCoeffs, Ipp16s *rc1,
                    Ipp16s *alphatmp, Ipp16s *scaleSynFltrCoeffs, Ipp16s *illcondp, Ipp16s *illcond);

void GetShapeGainIdxs(Ipp16s codebookIdx, Ipp32s *pShapeIdx, Ipp32s *pGainIdx, Ipp16s rate);

/* VQ functions*/
void VQ_target_vector_calc(Ipp16s *sw, Ipp16s *zir, Ipp16s *target);
void Impulse_response_vec_calc(Ipp16s *a, Ipp16s *pWghtFltrCoeffs, Ipp16s *h);
void Time_reversed_conv(Ipp16s *h, Ipp16s *target, Ipp16s nlstarget, Ipp16s *pn);
void VQ_target_vec_norm(Ipp16s linearGain, Ipp16s scaleGain, Ipp16s* target, Ipp16s* nlstarget);
void Excitation_VQ_and_gain_scaling(Ipp16s linearGain, const Ipp16s* tbl, Ipp16s* et);

/* Adapter functions*/
void WeightingFilterCoeffsCalc(Ipp16s *pTmpWghtFltrCoeffs, Ipp16s coeffsScale, Ipp16s *pVecWghtFltrCoeffs);
void LoggainLinearPrediction(Ipp16s *pVecLGPredictorCoeffs, Ipp16s *pLGPredictorState, Ipp32s *pGainLog);
void InverseLogarithmCalc(Ipp32s gainLog, Ipp16s *pLinearGain, Ipp16s *pScaleGain);
Ipp16s LoggainAdderLimiter(Ipp32s pGainLog, Ipp16s gainIdx, Ipp16s shapeIdx,
                           const Ipp16s *pCodebookGain);
void BandwidthExpansionModul(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, Ipp16s coeffsScale,
                             Ipp16s* pCoeffs, Ipp32s len);
void BandwidthExpansionModulFE(const Ipp16s* pConstBroadenVector, Ipp16s* pTmpCoeffs, Ipp16s coeffsScale,
                               Ipp16s* pCoeffs, Ipp32s len, Ipp16s countFE, Ipp16s illCond);
/* Postfilter functions*/
void AbsSum_G728_16s32s(Ipp16s* pSrc, Ipp32s* pSum);
void ScaleFactorCalc(Ipp32s sumunfil, Ipp32s sumfil, Ipp16s *scale, Ipp16s *nlsscale);
void FirstOrderLowpassFilter_OuputGainScaling(Ipp16s scale, Ipp16s nlsscale, Ipp16s *scalefil,
                                       Ipp16s *temp, Ipp16s *spf);

void STPCoeffsCalc(Ipp16s *pLPCFltrCoeffs, Ipp16s scaleLPCFltrCoeffs,
                   Ipp16s *pstA, Ipp16s rc1, Ipp16s *tiltz);

/*Pitch search functions*/
void LTPCoeffsCalc(const Ipp16s *sst, Ipp32s kp, Ipp16s *gl, Ipp16s *glb, Ipp16s *pTab);

void Set_Flags_and_Scalin_Factor_for_Frame_Erasure(Ipp16s fecount, Ipp16s ptab, Ipp16s kp, Ipp16s* fedelay, Ipp16s* fescale,
                                                   Ipp16s* nlsfescale, Ipp16s* voiced, Ipp16s* etpast,
                                                   Ipp16s *avmag, Ipp16s *nlsavmag);
void Log_gain_Limiter_after_erasure(Ipp32s *pGainLog, Ipp32s ogaindb, Ipp16s afterfe);
void ExcitationSignalExtrapolation(Ipp16s voiced, Ipp16s *fedelay, Ipp16s fescale, Ipp16s nlsfescale, Ipp16s* etpast,
                                     Ipp16s *et, Ipp16s *nlset, Ipp16s *seed);
void UpdateExcitationSignal(Ipp16s *etpast, Ipp16s *et, Ipp16s nlset);
void UpdateLGPredictorState(Ipp16s *et, Ipp16s *pLGPredictorState);

typedef struct {
   Ipp32u        objSize;
   Ipp32s        key;
   G728_Rate     rate ;      /* rate*/
   G728_Type     type;  /* Type of decoder;*/
}own_G728_Obj_t;


typedef struct {
  Ipp16s after_limiter_98;  // Q9:        Input of adder block #G.99
  Ipp16s ap_q11;            // Q11:       Quantizer locking factor
  Ipp32s dml;               // Q11:       Quantized residuals' long term energy
  Ipp32s dms;               // Q11:       Quantized residuals' short term energy
  Ipp16s GAIN_state;        // Q9         Weighting filter memory
  Ipp16s speech_to_vbd_transition; // Q0      Speech-to-VBD transition flag
  Ipp32s GDIFF;                  // 32bit Q9   The difference between the current gain and its average
  Ipp32s G_AVE;                  // 32bit Q9   Gain average
  Ipp32s GC_ATMP_SUM;            //            Sum of absolute LPC Parameters of block #G.50
  Ipp16s GC_ATMP1;               //            Second LPC Parameter of block #G.50
  Ipp16s DLQ_NLSGAIN;
  Ipp16s G_CNT;
  Ipp16s GC_CNT;
  Ipp16s GC_FLAG;
  Ipp32s_EC_Sfs excGain;    // SFL:       Linear Excitation gain and NLS for linear gain (+ Q format correction offset),
                                          // equivalent to GAIN in Annex G
  Ipp32s_EC_Sfs invGain;     // SBFL:      Inverted GAIN for VBD and NLS for the inverted VBD GAIN

  Ipp16s ET[RMS_BUF_LEN];   // [0..(RMS_BUF_LEN - 1)]      Memory of quantized residuals
  Ipp16s rtmp[LPC+1];
  Ipp16s TCQ_STTMP_q2[NFRSZ+IDIM+1]; // [0..NFRSZ]        Q2         Buffer for synthesis filter hybrid window

  Ipp16s TCQ_decoder_state;

  // pointers to variables of base codec
  Ipp16s *vecSyntFltrCoeffs;    //base
  Ipp16s *tmpSyntFltrCoeffs;
  Ipp16s *vecLGPredictorCoeffs;
  Ipp16s *tmpLGPredictorCoeffs;
  Ipp16s *vecLGPredictorState;
  Ipp16s *p_illcond;
  Ipp16s *p_illcondg;
  Ipp16s *p_illcondp;
  Ipp16s *p_scaleSyntFltrCoeffs;
  Ipp16s *p_scaleLGPredictorCoeffs;
  Ipp16s *nlssttmp;
  Ipp16s *sttmp;
  IppsWinHybridState_G728_16s         *rexpMem;
  IppsWinHybridState_G728_16s         *rexplgMem;

  Ipp32s distortion_metric;
} G728J_Obj;

struct _G728Encoder_Obj {
   Ipp16s   vecSyntFltrCoeffs[LPC      +6]; /*a*/
   Ipp16s   tmpSyntFltrCoeffs[LPC      +6]; /*atmp*/
   Ipp16s   vecWghtFltrCoeffs[2*LPCW   +4]; /* awp + awz */
   Ipp16s   tmpWghtFltrCoeffs[LPCW     +6]; /* awztmp */
   Ipp16s   vecLGPredictorCoeffs[LPCLG +6]; /*gp*/
   Ipp16s   tmpLGPredictorCoeffs[LPCLG +6]; /*gptmp*/
   Ipp16s   vecLGPredictorState[LPCLG   +6];/*gstate*/
   Ipp16s   y2[NCWD        +0];
   Ipp16s   h[IDIM         +3];
   Ipp16s   st[IDIM        +3];
   Ipp16s   sttmp[4*IDIM   +4];
   Ipp16s   nlssttmp[4     +4];
   Ipp16s   stmp[4*IDIM    +4];
   Ipp16s   r[11           +5];
   Ipp16s   rtmp[LPC+1     +5];

   own_G728_Obj_t    objPrm;
   const Ipp16s *pGq;
   const Ipp16s *pNgq;
   const Ipp16s *pCodebookGain;
   IppsIIRState_G728_16s            *wgtMem;
   IppsWinHybridState_G728_16s      *rexpMem;
   IppsWinHybridState_G728_16s      *rexpwMem;
   IppsWinHybridState_G728_16s      *rexplgMem;
   IppsCombinedFilterState_G728_16s *combMem;

   Ipp16s        illcond;
   Ipp16s        illcondw;
   Ipp16s        illcondg;
   Ipp16s        illcondp;
   Ipp16s        scaleWghtFltrCoeffs; /* nlsawztmp */
   Ipp16s        scaleSyntFltrCoeffs; /*nlsatmp*/
   Ipp16s        nlsst;
   Ipp16s        scaleLGPredictorCoeffs;/*nlsgptmp*/
   Ipp16s        icount;
   G728J_Obj     objJ;
};

struct _G728Decoder_Obj {
   Ipp16s   vecSyntFltrCoeffs[LPC                   +6]; /*a*/
   Ipp16s   tmpSyntFltrCoeffs[LPC                +6]; /*atmp*/
   Ipp16s   vecLGPredictorCoeffs[LPCLG                +6];/*gp*/
   Ipp16s   tmpLGPredictorCoeffs[LPCLG             +6];/*gptmp*/
   Ipp16s   vecLGPredictorState[LPCLG            +6];/*gstate*/
   Ipp16s   vecLPCFltrCoeffs[10                  +6];/*apf*/
   Ipp16s   pstA[20                 +4];
   Ipp16s   d_buff[239+2            +7];
   Ipp16s   sst_buff[239+IDIM+1+2   +1];
   Ipp16s   sttmp[4*IDIM            +4];
   Ipp16s   nlssttmp[4              +4];
   Ipp16s   etpast_buff[140+IDIM    +7];/* G728I coder parameter*/

   own_G728_Obj_t    objPrm;
   const Ipp16s *pGq;
   const Ipp16s *pNgq;
   const Ipp16s *pCodebookGain;
   IppsSynthesisFilterState_G728_16s   *syntState;
   IppsPostFilterState_G728_16s        *stpMem;
   IppsWinHybridState_G728_16s         *rexpMem;
   IppsWinHybridState_G728_16s         *rexplgMem;
   IppsPostFilterAdapterState_G728     *postFltAdapterMem;
   Ipp16s        pst;
   Ipp16s        illcond;
   Ipp16s        illcondg;
   Ipp16s        illcondp;
   Ipp16s        scaleSyntFltrCoeffs;/*nlsatmp*/
   Ipp16s        scaleLGPredictorCoeffs;/*nlsgptmp*/
   Ipp16s        scaleLPCFltrCoeffs;/*nlsapf*/
   Ipp16s        rc1;
   Ipp16s        ip;
   Ipp32s        kp1;
   Ipp16s        gl;
   Ipp16s        glb;
   Ipp16s        scalefil;
   /* G728I coder parameters*/
   Ipp16s        ferror;                /* Frame erasure indicator*/
   Ipp16s        adcount;               /* G728 adaptation cycle counter (2.5 ms)*/
   Ipp16s        afterfe;               /* Counter for gain clipping after FE (2.5 ms)*/
   Ipp16s        fecount;               /* Lenght of current frame erasure (2.5 ms)*/
   Ipp16s        fedelay;               /* Pitch (KP) stored at beginning of FE*/
   Ipp16s        fescale;               /* etpast scaling factor for frame erasures*/
   Ipp16s        nlsfescale;            /* NLS for fescale*/
   Ipp32s        ogaindb;               /* Old prediction gain in dB*/
   Ipp16s        voiced;                /* Voiced/Unvoiced flag*/
   Ipp16s        avmag;                 /* Average magnitude of past 8 excitation vector*/
   Ipp16s        nlsavmag;              /* NLS for avmag*/
   Ipp16s        seed;
   Ipp16s        pTab;
   G728J_Obj     objJ;
};

void Init_G728JEncoder(G728Encoder_Obj *obj);
void Init_G728JDecoder(G728Decoder_Obj *obj);
APIG728_Status apiG728JEncode(G728Encoder_Obj* encoderObj, Ipp16s *src, Ipp8u *dst);
APIG728_Status apiG728JDecode(G728Decoder_Obj* decoderObj, Ipp8u *src, Ipp16s *dst);

#include "aux_fnxs.h"

#endif /* __OWNG728_H__ */
