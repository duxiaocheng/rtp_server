/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2005-2011 Intel Corporation. All Rights Reserved.
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
//  Purpose: Echo Canceller header file.
//
*/

#ifndef __OWNEC_H__
#define __OWNEC_H__

#if defined(__ICC) || defined( __ICL ) || defined ( __ECL )
#define __INLINE static __inline
#elif defined( __GNUC__ )
#define __INLINE static __inline__
#else
#define __INLINE static
#endif
#include "nr_sbf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* common section */

#define COEF_SF 14
#define COEF_ONE (1 << COEF_SF)

#define F_SF 20

#define ADDWEIGHTED(val, add, coef) ((val) = (val) * (coef) + (1 - (coef)) * (add))
#define ALIGN(s) (((s) + 15) & ~15)
#define ADDWEIGHTED_INT(val, add, coef) ((val) = SCALE(((val) * (coef) + (COEF_ONE - coef) * (add)), COEF_SF))

#define SAT_32S_16S(v)((v) > 32767 ? 32767 : ((v) < -32768 ? -32768 : (v)))
#define SCALE(v, f) (((v) + (1<<((f)-1))) >> (f))

#define ADAPTATION_ENABLED (state->mode & 1)
#define NLP_ENABLED (state->mode & 2)
#define TD_ENABLED (state->mode & 4)
#define AH_ENABLED (state->mode & 16)
#define AH_ENABLED3 (state->mode & 64)
//#define AEC_SB_8ms_interface
/* fullband section */

/* maximum size of frame */
#define FB_FRAME_SIZE_MAX 16
/* size of frame */
#define FB_FRAME_SIZE 8

typedef struct __fbECState{
    ScratchMem_Obj Mem;
    IppsFullbandControllerState_EC_32f *controller; /* controller */
    Ipp32f  *firf_coef;        /* fixed filter coeffs */
    Ipp32f  *fira_coef;        /* adaptive filter coeffs */
    Ipp32f  *rin_hist;         /* history buffer */
    Ipp32f  *pErrToWeights;
    ahState *ah;
    _shiftupCosState_int *shiftupCos;
    Ipp32f  err;               /* error of NLMS */
    Ipp32s  tap;                  /* number of filter coeffs */
    Ipp32s  tap_m;                /* size of history buffer */
    Ipp32s  mode;                 /* current operating mode */
    Ipp32s  hist_buf;             /* history buffer */
    Ipp32s  pos;                  /* current position in history buffer */
    Ipp32s  frameSize;            /* size of frame */
    int     freq;
    Ipp32s  scratch_mem_size;
    Ipp32s  nlp;
    Ipp16s  cng;
} _fbECState;

/* subband section */

#define SB_FRAME_SIZE (1<<6)
#define SB_FFT_ORDER  7
#define SB_FFT_LEN    (1<<SB_FFT_ORDER)
#define SB_WIN_LEN    SB_FFT_LEN
#define SB_SUBBANDS   ((1<<(SB_FFT_ORDER-1))+1)
#define INIT_SEED     11111

/* affine projection subband section */
typedef struct __sbECState {
    ScratchMem_Obj Mem;
    IppsSubbandControllerState_EC_32f *controller; /* subband controller */
    IppsFFTSpec_R_32f* spec_fft;   /* FFT structure */
    IppsFFTSpec_C_32fc* spec_fftC; /* FFT structure */
    Ipp8u    *pBuf;                   /* FFT work buffer */
    Ipp32fc **ppRinSubbandHistory; /* receive-in subband history */
    Ipp32fc **ppAdaptiveCoefs;     /* adaptive coeffs */
    Ipp32fc **ppFixedCoefs;        /* fixed coeffs */
    Ipp32fc **pPrevError;
    Ipp64f  **pMu;
    ahState *ah;
    _shiftupCosState_int *shiftupCos;
    Ipp32f    pRinHistory[SB_WIN_LEN];   /* receive-in signal history */
    Ipp32f    ff,ffa;
    Ipp32s    FFTSize, frameSize, windowLen;
    Ipp32s    numSegments;               /* number of filter segments */
    Ipp32s    numSubbands;               /* number of subbands */
    Ipp32s    mode;                      /* current operating mode */
#ifndef AEC_SB_8ms_interface
    int    instackLenSout,instackLenRin,instackLenSin;
    int    instackCurLenSout,instackCurLenRin,instackCurLenSin;
    Ipp32f instackRin[FRAME_SIZE_10ms+FRAME_SIZE_6ms];
    Ipp32f instackSin[FRAME_SIZE_10ms+FRAME_SIZE_6ms];
    Ipp32f instackSout[FRAME_SIZE_10ms+FRAME_SIZE_6ms];
#endif
    int     freq;
    Ipp32s  scratch_mem_size;
    Ipp32s  nlp;
    Ipp32s  ap_order;
    Ipp16s  cng;
    Ipp16s  seed;
} _sbECState;

/* subbandfast section */

#define SBF_FRAME_ORDER 4
//#define SBF_20

#if 0

#define SBF_SUBBANDS 17
#define SBF_FRAME_SIZE 24
#define SBF_FFT_LEN 32
#define SBF_FFT_ORDER 5
#define SBF_WIN_LEN 128

#else
/*
#define SBF_SUBBANDS 33
#define SBF_FRAME_SIZE 44
#define SBF_FFT_LEN 64
#define SBF_FFT_ORDER 6
#define SBF_WIN_LEN 256
*/
#if !defined(SBF_20) && !defined(SBF_40)
#define SBF_SUBBANDS 33
#define SBF_FRAME_SIZE 16
#define SBF_FFT_LEN 64
#define SBF_FFT_ORDER 6
#define SBF_WIN_LEN 64
#endif

#ifdef SBF_20
#define SBF_SUBBANDS 33
#define SBF_FRAME_SIZE 20
#define SBF_FFT_LEN 64
#define SBF_FFT_ORDER 6
#define SBF_WIN_LEN 64
#endif

#ifdef SBF_40
#define SBF_SUBBANDS 65
#define SBF_FRAME_SIZE 40
#define SBF_FFT_LEN 128
#define SBF_FFT_ORDER 7
#define SBF_WIN_LEN 128
#endif
#endif

#define NOISE_GEN_LEVEL 10
#define SAFETY_BUFFER_SIZE 64 /* must be pow of 2 */

//#define ADAPT_NOISE


typedef struct __isbfECState {
    ScratchMem_Obj Mem;
    Ipp64s r_in_pwr;               /* receive-in signal power */
    Ipp64s s_in_pwr;               /* send-in signal power */
    Ipp64s rinPow;               /* receive-in signal power */
    Ipp64s sinPow;               /* send-in signal power */
    Ipp64s soutPow;
    Ipp32s coupling;
    Ipp32s es_off;
    Ipp32s conv;
    Ipp32s start_time;
    Ipp16s rinBuf[SBF_WIN_LEN];
    Ipp16s sinBuf[SBF_WIN_LEN];
    /* receive-in subband analysis structure */
    IppsSubbandProcessState_16s *state_ana_rin;
    /* send-in subband analysis structure */
    IppsSubbandProcessState_16s *state_ana_sin;
    /* subband synthesis structure */
    IppsSubbandProcessState_16s *state_syn;
    Ipp32sc **ppRinSubbandHistory; /* receive-in subband history */
    Ipp32sc **ppAdaptiveCoefs;     /* adaptive coeffs */
    Ipp32sc **ppFixedCoefs;        /* fixed coeffs */
    IppsSubbandControllerDTState_EC_16s *controller; /* subband controller */
    //        struct _SubbandControllerDTState_EC_16s *controller2; /* subband controller 2*/
    IppsToneDetectState_EC_16s *tdr, *tds; /* ToneDetect state structures */
    ahState *ah;
    _shiftupCosState_int *shiftupCos;
    IppsFilterNoiseState_EC_FB_32f fstate;
    Ipp8u *pBuf;                   /* FFT work buffer */
    //Ipp32s td_coeff;/* ToneDisabler coeffs */
    //Ipp64s td_thres;/* ToneDisabler coeffs */
    int FFTSize, frameSize, windowLen;
    int numSegments;               /* number of filter segments */
    int numSubbands;               /* number of subbands */
    int mode;                      /* current operating mode */
    int td_mode;                   /* mode before activate ToneDisabler */
    //int td_resr, td_ress;          /* used in ToneDisabler */
    int dt, dt1;                        /* Double talk */
    int frame;
    int upCounter;
    Ipp16s sgain;                  /* NLP gain */
    Ipp16s nlp;
    Ipp16s cng;
    Ipp32s smoothSin, smoothSout;
    Ipp32s shortTermSin, shortTermSout, longTermSout;
    Ipp32s nlp_mult;
    Ipp32s noise_seed;
    Ipp32s noise_lev;
    Ipp32s save_noise_lev[5];
    Ipp32s iirDly[6];
    Ipp16s saved_sin[48];
    Ipp32s saved_sin_idx;
    Ipp32s scratch_mem_size;
    //Ipp16s  samplingRate,td_sil_time,td_sil_thresh;
    // new members
    Ipp64s acpwr;
    Ipp64s cdlt;
    int fixCfgInit;
    Ipp64s rinSubbandPow[SBF_SUBBANDS];

} _isbfECState;
#define AEC_MASK1S 2

#ifdef __cplusplus
}
#endif

#endif /* __OWNEC_H__ */

