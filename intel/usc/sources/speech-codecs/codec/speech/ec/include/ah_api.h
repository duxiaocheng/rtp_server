/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
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
//  Purpose: AH header file.
//
*/

#ifndef __AH_API_H__
#define __AH_API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __ahState ahState;
/* set mode */
Ipp32s ah_SetMode(ahState *state, AHOpcode op);

/* Returns size of buffer for state structure */
Ipp32s ah_GetSize(Ipp32s frameSize, Ipp32s *size,int freq,int *scratchMemSize);

/* Initializes state structure */
Ipp32s ah_Init(ahState *state,Ipp32s samplingRate,Ipp32s frameSize,Ipp32s coef);

/* Process one frame */
AHOpcode ah_ProcessFrame_32f(ahState *state, Ipp32f *sin, Ipp32f *sout,int frameSize);
void formFFTin_32f(ahState *state, Ipp32f *sout, int len);
void shiftupFft_32f(ahState *state, Ipp32f *out,int frameSize);
int saveHistRS_32f(ahState *state, Ipp32f *rin, Ipp32f *sin);
int cnr_32f(ahState *state,Ipp32f *rin,Ipp32f *sin,Ipp32f *sout,int nlp,int cng,int off);
int shiftupCos_GetSize(int frameSize, int *size);
int shiftupCos_Init(_shiftupCosState_int *state, int frameSize);
int shiftupCos(_shiftupCosState_int *state, Ipp16s *in1, Ipp16s *out1);
int aecSetNlpThresholds(_ahState *state,Ipp32s n,Ipp32s n2,Ipp32s coef);
#ifdef __cplusplus
}
#endif
#endif /* __AH_API_H__ */
