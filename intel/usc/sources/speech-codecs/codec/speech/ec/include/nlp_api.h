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
//  Purpose: NLP header file.
//
*/

#ifndef __NLP_API_H__
#define __NLP_API_H__

#ifdef __cplusplus
extern "C" {
#endif
#define NLP_HIST_LEN_ORDER  8
#define NLP_HIST_LEN        (1<<NLP_HIST_LEN_ORDER)
#define NLP_FRAME_ORDER 4
#define NLP_FRAME_SIZE  (1<<NLP_FRAME_ORDER)
typedef struct __nlpState{
    Ipp32s    si_pwr,sil_pwr,srin_pwr,sin_pwr2,suppStop,cnvgd,c1,c2,c3,c4,c5,coef,sstart;
    Ipp32f    nAttn;
} nlpState;
void soutProc(nlpState *state,Ipp16s *h,Ipp16s *inout,Ipp16s *rin,Ipp16s *sin,Ipp32s noisePwr);
void initNLP(nlpState *state,int c1,int c2,int c3,int c4,int c5,int coef);
#ifdef __cplusplus
}
#endif
#endif /* __AH_API_H__ */
