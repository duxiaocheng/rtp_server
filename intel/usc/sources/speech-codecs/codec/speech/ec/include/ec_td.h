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
//  Purpose: anti - howling header file.
//
*/

#ifndef __OWNTD_H__
#define __OWNTD_H__
#ifdef __cplusplus
extern "C" {
#endif
typedef struct __td{
    IppsToneDetectState_EC_32f *tdr;
    IppsToneDetectState_EC_32f *tds;
    Ipp64f    td_thres;
    Ipp32f    td_ress;
    Ipp32f    td_resr;
    Ipp32f    td_sil_thresh;
    Ipp32f    td_sil_time;
} _td;
typedef struct __td_int{
    IppsToneDetectState_EC_16s *tdr;
    IppsToneDetectState_EC_16s *tds;
    Ipp64s    td_thres;
    Ipp32s    td_ress;
    Ipp32s    td_resr;
    Ipp16s    td_sil_thresh;
    Ipp16s    td_sil_time;
} _td_int;
int toneDisabler_32f(Ipp32f *r_in, Ipp32f *s_in,Ipp16s samplingRate,int frameSize,
                 _td *td);
#ifdef __cplusplus
}
#endif

#endif /* __OWNTD_INT_H__ */
