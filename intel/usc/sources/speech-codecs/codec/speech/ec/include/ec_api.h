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

#ifndef __EC_API_H__
#define __EC_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __EC_API_INT_H__
    typedef enum op_code {
        EC_COEFFS_ZERO = 0,
        EC_ADAPTATION_ENABLE,
        EC_ADAPTATION_ENABLE_LITE,
        EC_ADAPTATION_DISABLE,
        EC_ADAPTATION_DISABLE_LITE,
        EC_NLP_ENABLE,
        EC_NLP_DISABLE,
        EC_TD_ENABLE,
        EC_TD_DISABLE,
        EC_AH_ENABLE,  /* Enable AH, HD mode 1 - FFT-based shift up*/
        EC_AH_ENABLE3,  /* Enable AH, HD mode 3 - cos shift up*/
        EC_AH_DISABLE,
        EC_ALG1
    } ECOpcode;
    typedef enum hd_op_code_enum {
        HD_1=0,
        HD_2
    } HDOpcode;
    typedef enum ah_op_code_enum {
        AH_ENABLE= 0,
        AH_DISABLE,
        AH_ENABLE1,
        AH_ENABLE2,
        AH_ENABLE3
    } AHOpcode;
#endif /* __EC_API_H__ */

    typedef int (*ec_GetFrameSize_ptr)(IppPCMFrequency freq, int taptime_ms, int *s);
    typedef int (*ec_GetSize_ptr)(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *s2, int ap);
    typedef int (*ec_GetSendPathDelay_ptr)(int *delay);
    typedef int (*ec_Init_ptr)(void *state,void *hObj, IppPCMFrequency freq, int taptime_ms,int ap);
    typedef int (*ec_ModeOp_ptr)(void *state, ECOpcode op);
    typedef int (*ec_ProcessFrame_ptr)(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout);//, Ipp32f mu
    typedef int (*ec_GetInfo_ptr)(void *state,ECOpcode *isNLP,ECOpcode *isTD,
        ECOpcode *isAdapt,ECOpcode *isAdaptLite, AHOpcode *isAH);
    typedef int (*ec_GetSubbandNum_ptr)(void *state);
    typedef int (*ec_setNR_ptr)(void *state, int num);
    typedef int (*ec_setNRreal_ptr)(void *state, int num, int num2);
    typedef int (*ec_setCNG_ptr)(void *state, int num);
    typedef int (*ec_setTDThreshold_ptr)(void *state, int num);
    typedef int (*ec_setTDSilenceTime_ptr)(void *state, int num);

    int ec_fb_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *s2, int ap);
    int ec_fb_Init(void *state,void *hObj, IppPCMFrequency freq, int taptime_ms, int ap);
    int ec_fb_InitBuff(void *state, char *buff);
    int ec_fb_ModeOp(void *state, ECOpcode op);
    int ec_fb_ProcessFrame(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout);
    int ec_fb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s);
    int ec_fb_GetSendPathDelay(int *delay);
    Ipp32f ec_fb_GetNLPGain(void *state);
    int ec_fb_GetInfo(void *state,ECOpcode *isNLP,ECOpcode *isTD,
        ECOpcode *isAdapt,ECOpcode *isAdaptLite, AHOpcode *isAH);
    int ec_fb_GetSubbandNum(void *state);

    int ec_sb_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *s2, int ap);
    int ec_sb_Init(void *state,void *hObj, IppPCMFrequency freq, int taptime_ms, int ap);
    int ec_sb_InitBuff(void *state, char *buff);
    int ec_sb_ModeOp(void *state, ECOpcode op);
    int ec_sb_ModeCheck(void *state, ECOpcode op);
    int ec_sb_GetSegNum(void *state);
    int ec_sb_GetSubbandNum(void *state);
    Ipp32f ec_sb_GetNLPGain(void *state);

    int ec_sb_ProcessFrame(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout);//,Ipp32f mu
    int ec_sb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s);
    int ec_sb_GetSendPathDelay(int *delay);
    int ec_sb_GetInfo(void *stat,ECOpcode *isNLP,ECOpcode *isTD,
        ECOpcode *isAdapt,ECOpcode *isAdaptLite, ECOpcode *isAH);
    int ec_sb_GetSubbandNum(void *state);
    int ec_sb_setTDThreshold(void *stat,int thresh);// 1=1db
    int ec_sb_setCNG(void *stat, int n);
    int ec_sb_setNR(void *stat, int n);
    int ec_sb_setTDSilenceTime(void *stat,int thresh);// 1=10msec

    //int ec_asb_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *s2);
    //int ec_asb_Init(void *state,void *hObj, IppPCMFrequency freq, int taptime_ms);
    //int ec_asb_InitBuff(void *state, char *buff);
    //int ec_asb_ModeOp(void *state, ECOpcode op);
    //int ec_asb_ModeCheck(void *state, ECOpcode op);
    //int ec_asb_GetSegNum(void *state);
    //int ec_asb_GetSubbandNum(void *state);
    //Ipp32f ec_asb_GetNLPGain(void *state);

    //int ec_asb_ProcessFrame(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout, Ipp32f mu);
    //int ec_asb_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s);
    //int ec_asb_GetSendPathDelay(int *delay);
    //int ec_asb_GetInfo(void *stat,ECOpcode *isNLP,ECOpcode *isTD,
    //    ECOpcode *isAdapt,ECOpcode *isAdaptLite, ECOpcode *isAH);
    //int ec_asb_GetSubbandNum(void *state);
    //int ec_asb_setTDThreshold(void *stat,int thresh);// 1=1db
    //int ec_asb_setCNG(void *stat, int n);
    //int ec_asb_setNR(void *stat, int n);
    //int ec_asb_setTDSilenceTime(void *stat,int thresh);// 1=10msec


    int ec_sbf_GetSize(IppPCMFrequency freq, int taptime_ms, int *s, int *s1, int *s2, int ap);
    int ec_sbf_Init(void *state,void *hObj, IppPCMFrequency freq, int taptime_ms, int ap);
    int ec_sbf_InitBuff(void *state, char *buff);
    int ec_sbf_ModeOp(void *state, ECOpcode op);
    int ec_sbf_ProcessFrame(void *stat, Ipp32f *frin, Ipp32f *fsin, Ipp32f *fsout);
    int ec_sbf_GetFrameSize(IppPCMFrequency freq, int taptime_ms, int *s);
    int ec_sbf_GetSendPathDelay(int *delay);
    Ipp32f ec_sbf_GetNLPGain(void *state);

    int ec_sbf_GetInfo(void *state,ECOpcode *isNLP,ECOpcode *isTD,
        ECOpcode *isAdapt,ECOpcode *isAdaptLite, AHOpcode *isAH);
    int ec_sbf_GetSubbandNum(void *state);

    int ec_sb_setAHmodeHD(void *stat, HDOpcode op);

    int ec_sbf_setNR(void *stat, int n);
    int ec_fb_setNR(void *stat, int n);
    int ec_fb_setNRreal(void *stat, int n, int n2);

    int ec_sbf_setCNG(void *stat, int n);
    int ec_fb_setCNG(void *stat, int n);
    int ec_sbf_setNRreal(void *stat, int n, int n2);
    int ec_sb_setNRreal(void *stat, int n, int n2);

    int ec_sbf_setTDThreshold(void *stat,int thresh);
    int ec_fb_setTDThreshold(void *stat,int thresh);

    int ec_sbf_setTDSilenceTime(void *stat,int thresh);
    int ec_fb_setTDSilenceTime(void *stat,int thresh);

#define FFT_ORDER11         8
#define FFT_ORDER10         10
#define FFT_ORDER6          6
#define AEC_FFT_ORDER11_LEN (((1<<FFT_ORDER11)+2)>>1)
#define AEC_FFT_ORDER10_LEN (((1<<FFT_ORDER10)+2)>>1)
#define AEC_FFT_ORDER6_LEN  (((1<<FFT_ORDER6)+2)>>1)
#define AEC_FFT_ORDER6_LEN2 ((1<<FFT_ORDER6)+2)
#define FFT_ORDER7          7
#define AEC_FFT_ORDER7_LEN  (((1<<FFT_ORDER7)+2)>>1)
#define FFT_ORDER7_SRC_LEN  (1<<FFT_ORDER7)
#define FFT_ORDER10_SRC_LEN (1<<FFT_ORDER10)
#define FFT_ORDER11_SRC_LEN (1<<FFT_ORDER11)
#define FRAME_SIZE_10ms     80
#define FRAME_SIZE_6ms      48
#define SBF_OFF     48
#ifdef AEC_SB_8ms_interface
#define SB_OFF      0
#else
#define SB_OFF      48
#endif
#define FB_OFF      0

    //#define _AEC_DBG_PRT
#ifdef _AEC_DBG_PRT
#define mprintf printf
#else
#define mprintf //a
#endif

#define IPP_AEC_VER 86
extern Ipp16s IPP_AEC_dBtbl[];

#ifdef __cplusplus
}
#endif

#endif /* __EC_API_INT_H__ */
