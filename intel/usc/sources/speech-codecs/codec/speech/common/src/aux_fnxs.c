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
// Purpose: USC miscellaneous functions
//
*/
#include "aux_fnxs.h"

static __ALIGN32 CONST Ipp32s sqrTbl[49] =
{
// 32767, 31790, 30894, 30070, 29309, 28602, 27945, 27330, 26755, 26214,
// 25705, 25225, 24770, 24339, 23930, 23541, 23170, 22817, 22479, 22155,
// 21845, 21548, 21263, 20988, 20724, 20470, 20225, 19988, 19760, 19539,
// 19326, 19119, 18919, 18725, 18536, 18354, 18176, 18004, 17837, 17674,
// 17515, 17361, 17211, 17064, 16921, 16782, 16646, 16514, 16384
0x7fff0000,0x7c2e0000, 0x78ae0000, 0x75760000, 0x727d0000, 0x6fba0000, 0x6d290000, 0x6ac20000,
0x68830000,0x66660000, 0x64690000, 0x62890000, 0x60c20000, 0x5f130000, 0x5d7a0000, 0x5bf50000,
0x5a820000,0x59210000, 0x57cf0000, 0x568b0000, 0x55550000, 0x542c0000, 0x530f0000, 0x51fc0000,
0x50f40000,0x4ff60000, 0x4f010000, 0x4e140000, 0x4d300000, 0x4c530000, 0x4b7e0000, 0x4aaf0000,
0x49e70000,0x49250000, 0x48680000, 0x47b20000, 0x47000000, 0x46540000, 0x45ad0000, 0x450a0000,
0x446b0000,0x43d10000, 0x433b0000, 0x42a80000, 0x42190000, 0x418e0000, 0x41060000, 0x40820000,
0x40000000
};

static __ALIGN32 CONST Ipp16s sqrDiffTbl[48] =
{ // sqrDiffTbl[i]=sqrTbl[i]-sqrTbl[i+1]
  1954,  1792,  1648,  1522,  1414,  1314,  1230,  1150,  1082,  1018,
   960,   910,   862,   818,   778,   742,   706,   676,   648,   620,
   594,   570,   550,   528,   508,   490,   474,   456,   442,   426,
   414,   400,   388,   378,   364,   356,   344,   334,   326,   318,
   308,   300,   294,   286,   278,   272,   264,   260
};

/* 
    sqrt(valFrac,valExp)
       valFrac - normalized mantissa in Q31: 0x40000000 <= valFraq <= 0x7fffffff 
       vaExp   - exponent  
*/
void InvSqrt_32s16s_I(Ipp32s *valFrac, Ipp16s *valExp)
{
    Ipp16s i, a;

    if (*valFrac <= 0)
    {
        *valExp = 0;
        *valFrac = IPP_MAX_32S;
        return;
    }
    if( (*valExp & 1) == 1 )
          *valFrac = *valFrac >> 1;

    *valExp = (Ipp16s)(-((*valExp - 1) >> 1));

    i = (Ipp16s)(*valFrac >> 25);  // 16 <= i < 64 
    a = (Ipp16s)((*valFrac >> 10) & 0x7fff);
    *valFrac = (sqrTbl[i-16] - (sqrDiffTbl[i-16] * a));
}
