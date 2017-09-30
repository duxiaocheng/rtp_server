/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the tems of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the tems of that agreement.
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
//  Purpose: sbf NR
//
*/

#if !defined(SBF_20) && !defined(SBF_40)
#define SBF_SUBBANDS 33
#define SBF_FRAME_SIZE 16
#define SBF_FFT_LEN 64
#define SBF_FFT_ORDER 6
#define SBF_WIN_LEN 64
#endif
#define ALIGN(s) (((s) + 15) & ~15)

typedef struct __t_fp {
    int e, m;
} t_fp;
typedef struct _FilterNoiseState_EC_FB_32f{
    t_fp ns_lambda_d[SBF_SUBBANDS];
    t_fp ns_x_pwr[SBF_SUBBANDS];
    t_fp ns_one, ns_beta, ns_beta1m, ns_alpha, ns_alpha1m, ns_gamma, ns_gamma1m;
    t_fp ns_al, ns_al1m;

} IppsFilterNoiseState_EC_FB_32f;
