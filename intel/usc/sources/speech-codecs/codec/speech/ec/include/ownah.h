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
//  to the file ipplic.htm located in the root directory of your Intel(R) IPP
//  product installation for more information.
//
//
//  Purpose: anti - howling header file.
//
*/

#ifndef __OWNAH_H__
#define __OWNAH_H__

#ifdef __cplusplus
extern "C" {
#endif
#ifndef __OWNAH_INT_H__

#define FFT_ORDER6_SRC_LEN       (1<<FFT_ORDER6)
#define FFT_ORDER10_SRC_LEN       (1<<FFT_ORDER10)
#define FFT_ORDER11_SRC_LEN       (1<<FFT_ORDER11)
#define MAIN_FREQS_NUM_ORDER 6
#define MAIN_FREQS_NUM       (1<<MAIN_FREQS_NUM_ORDER)
#define NOISE_SUBBANDS_NUM   (((1<<FFT_ORDER6)+2)>>1)
#define ORDER10_SPECTR_LEN   (((1<<FFT_ORDER10)+2)>>1)
#define ORDER11_SPECTR_LEN   (((1<<FFT_ORDER11)+2)>>1)
#define TONE_SCALE 7


#define S16_SAT(r)   ((r) > 32767 ? 32767 :   ((r) < -32768 ? -32768 : (short)(r)))
#define SIGNAL_CALC_SCALE 15
#define AEC_EXP_TBL_SIZE  72
#define WNOISE_LEN_AEC    (1<<13)
#define BW_HWL 11
#define BW_DISP_LEN ((((1<<FFT_ORDER6)+2)>>1)/BW_HWL)
#define BW_AMPL_PRECISION 2
#define MAX_HOWL_IDX0 2
#define MAX_HOWL_IDX2 29
#define dFScale           5
#define LOW_BAND          100    //FFT band pass filter margin
#define HIGH_BAND         3300
#define LOW_WIDE_BAND     90
#define HIGH_WIDE_BAND    15600
#define MIN_SIGNAL_LEVEL  1251

#define FFT_ORDER6_SRC_LEN       (1<<FFT_ORDER6)
#define FFT_ORDER10_SRC_LEN       (1<<FFT_ORDER10)
#define FFT_ORDER11_SRC_LEN       (1<<FFT_ORDER11)
#define MAIN_FREQS_NUM_ORDER 6
#define MAIN_FREQS_NUM       (1<<MAIN_FREQS_NUM_ORDER)
#define NOISE_SUBBANDS_NUM   (((1<<FFT_ORDER6)+2)>>1)
#define ORDER10_SPECTR_LEN   (((1<<FFT_ORDER10)+2)>>1)
#define ORDER11_SPECTR_LEN   (((1<<FFT_ORDER11)+2)>>1)
#define PSD_SF 10
#define TONE_SCALE 7
#define COEF_SF 14
#define COEF_ONE (1 << COEF_SF)
#define COEF_0_5 (COEF_ONE / 2)
#define COEF_0_2 (COEF_ONE / 5)
#define COEF_0_77 (COEF_ONE * 10 / 13)
#define COEF_0_35 (COEF_ONE * 7 / 20)
#define COEF_0_48 (COEF_ONE * 12 / 25)
#define COEF_0_66 (COEF_ONE * 2 / 3)
#define COEF_0_8 (COEF_ONE * 4 / 5)
#define COEF_0_85 (COEF_ONE * 17 / 20)
#define COEF_Q15_ONE ((1 << 15) - 1)
#define AEC_NLP_OBJ_NUM 5

#define CMUL(x, v) SCALE((x) * (v), COEF_SF)

#define WEIGHTED_INT(val, add, coef) SCALE(((val) * (coef) + (COEF_ONE - coef) * (add)), COEF_SF)

#endif /* __OWNAH_INT_H__ */


#define AH_FIR_TAPS 64
typedef struct __shiftupCosState_int{
    int     frameSize;
    int     phase;
    Ipp16s *upDownBuf16;        /* work buffer */
    Ipp32s *upDownBuf32;        /* work buffer */
    IppsFIRState32s_16s *fir64State;
    Ipp16s  fir64Dly[AH_FIR_TAPS];
    Ipp32s  ibq2Cfs[2*6];
    Ipp32s  ibq2Dly[2*6];
} _shiftupCosState_int;

typedef struct __ahState{
    ScratchMem_Obj Mem;
    _td        td;
    nlpState  nlpObj[AEC_NLP_OBJ_NUM];
    IppsIIRState_32f  *bq2State;
    IppsIIRState_32f  *bq2StateTest;
    IppsFFTSpec_R_32f *FFTspec10;
    IppsFFTSpec_R_32f *FFTspec6;
    IppsFFTSpec_R_32f *FFTspec7;
    IppsFFTSpec_R_32f *FFTspec11;
    IppsRandUniState_32f *pRandUniState_32;
    Ipp64f   noiseCurMagn[AEC_FFT_ORDER7_LEN];
    Ipp32f  *histRin;
    Ipp32f  *histSin;
    Ipp32f  *histSout;
    Ipp32f  *upDownBuf;         /* work buffer sd*/
    IppsFilterNoiseState_EC_32f    *nr;
    IppsFilterNoiseState_EC_32f    *nrSin;
    Ipp32f   pFDdly2[6*2];
    Ipp32f   pDly2Test[6*2];
    Ipp32f   pDly1Test[6*1];
    Ipp32f   iirDly[6];
    Ipp32f   iirDly2[6];
    Ipp32f   pDly4Test[6*4];
    Ipp32f   wbNoisePwr;
    Ipp32f   rinNoisePwr,dcOffset;
    //Ipp32f   nAttn;
    Ipp32s   dtFlag;
    //Ipp32s   coefc;
    Ipp32s   histLen;
    Ipp32s   histSinCurLen,histRinCurLen;
    Ipp32s   histSoutLen;
    Ipp32s   histSoutCurLen;
    Ipp32s   prevHistExist;
    Ipp32s   dF;
    Ipp32s   howlLowBand;
    Ipp32s   howlHighBand;
    Ipp32s   howlPeriod;
    Ipp32s   howlPeriodInit;
    Ipp32s   shift;
    Ipp32s   hd_mode;
    Ipp32s   nCatches;
    Ipp32s   nNLP;
    unsigned  int seed;
    AHOpcode  mode;
    IppsNRLevel nrMode;
    Ipp64f    so_pwr;
    Ipp64f    sol_pwr;
    Ipp64f    sl_pwr;
    Ipp64f    nsi_pwr,sinNoisePwr;
    Ipp64f    sqrtNoisePwr;
    int       frameNum;
    Ipp16s    samplingRate;
    Ipp16s    noiseFlag;
    Ipp16s    noiseMax;
    Ipp16s    fftWorkBufSize;
    Ipp16s    initHowl;
} _ahState;
typedef struct __ahState_int{
    ScratchMem_Obj Mem;
    _td_int        td;
    nlpState  nlpObj;
    IppsFFTSpec_R_16s32s *FFTspec10;
    IppsFFTSpec_R_16s32s *FFTspec6;
    IppsFFTSpec_R_16s32s *FFTspec7;
    IppsFFTSpec_R_16s32s *FFTspec11;
    IppsRandUniState_16s *pRandUniState_16;
    Ipp64s   noiseCurMagn[AEC_FFT_ORDER7_LEN];
    Ipp16s  *histRin;
    Ipp16s  *histSin;
    Ipp16s  *histSout;
    Ipp16s  *histSoutS;
    Ipp16s  *upDownBuf;         /* work buffer sd*/
    IppsFilterNoiseState_EC_32f    *nr;
    IppsFilterNoiseState_EC_32f    *nrSin;
    Ipp32s   pFDdly2[6*2];
    Ipp32s   pDly2Test[6*2];
    Ipp32s   pDly1Test[6*1];
    Ipp32s   iirDly[6];
    Ipp32s   iirDly2[6];
    Ipp16s   pDly4Test[6*4];
    Ipp32s   wbNoisePwr;
    Ipp32s   rinNoisePwr;
    Ipp32s   avgAttn;
    Ipp32s   sinPwr;
    Ipp32s   rinPwr;
    Ipp32s   dtFlag;
    Ipp32s   coefc;
    Ipp32s   histLen;
    Ipp32s   histSinCurLen,histRinCurLen;
    Ipp32s   histSoutLen;
    Ipp32s   histSoutSLen;
    Ipp32s   histSoutCurLen;
    Ipp32s   prevHistExist;
    Ipp32s   dF;
    Ipp32s   howlLowBand;
    Ipp32s   howlHighBand;
    Ipp32s   howlPeriod;
    Ipp32s   howlPeriodInit;
    Ipp32s   shift;
    Ipp32s   hd_mode;
    Ipp32s   fdShift;
    unsigned  int seed;
    AHOpcode  mode;
    Ipp32s    so_pwr;
    Ipp32s    sol_pwr;
    Ipp32s    sl_pwr;
    Ipp32s    nsi_pwr,sinNoisePwr;
    int       frameNum;
    Ipp16u    sqrtNoisePwr;
    Ipp16u    sqrtNoisePwrOrder;
    Ipp16s    samplingRate;
    Ipp16s    noiseFlag;
    Ipp16s    noiseMax;
    Ipp16s    initHowl;
    Ipp16s    fftWorkBufSize;
} _ahState_int;
#define AH_FIR_TAPS 64
typedef struct __shiftupCosState{
    int     frameSize;
    int     phase;
    Ipp32f *upDownBuf16;        /* work buffer */
    Ipp32f *upDownBuf32;        /* work buffer */
    IppsFIRState_32f *fir64State;
    Ipp32f fir64Dly[AH_FIR_TAPS];
    Ipp32f  ibq2Cfs[2*6];
    Ipp32f  ibq2Dly[2*6];
} _shiftupCosState;
#define IPP_AEC_CONST_NOISE_LEVEL 10

#ifdef __cplusplus
}
#endif

#endif /* __OWNAH_H__ */

