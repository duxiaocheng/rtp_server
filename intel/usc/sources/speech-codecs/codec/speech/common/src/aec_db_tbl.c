/* /////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c)  2005-2011 Intel Corporation. All Rights Reserved.
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
//  Purpose: common ec-int/float dB table
//
*/

//[0]=32767, [-6]=16383, [-12]=8191, [-18]=4095, [-24]=2047, [-30]=1023, [-36]=511, [-42]=255,
//[-48]=127, [-54]=63, [-60]=31, [-66]=15, [-72]=7, [-78]=3, [-84]=1,
#include <ipps.h>
Ipp16s IPP_AEC_dBtbl[80]={//starts from -10dBm0
10361, 9234, 8191, 7335, 6537, 5826, 5193, 4628, 4095, 3676, 3276, 2920, 2602, 2319,
2047, 1842, 1642, 1463, 1304, 1162, 1023, 923, 823, 733, 653, 582, 511, 462, 412, 367,
327, 292, 255, 231, 206, 184, 164, 146, 127, 116, 103, 92, 82, 73, 63, 58, 51, 46, 41,
36, 31, 29, 26, 23, 20, 18, 15, 14, 13, 11, 10, 9, 8, 7, 6, 5, 5, 4, 3, 3, 3, 2, 2, 2,
1, 1, 1, 1, 1, 1
};
