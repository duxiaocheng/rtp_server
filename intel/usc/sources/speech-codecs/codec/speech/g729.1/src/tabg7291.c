/*/////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2006-2011 Intel Corporation. All Rights Reserved.
//
//     Intel(R) Integrated Performance Primitives
//     USC - Unified Speech Codec interface library
//
// By downloading and installing USC codec, you hereby agree that the
// accompanying Materials are being provided to you under the terms and
// conditions of the End User License Agreement for the Intel(R) Integrated
// Performance Primitives product previously accepted by you. Please refer
// to the file ipplic.htm located in the root directory of your Intel(R) IPP
// product installation for more information.
//
// A speech coding standards promoted by ITU, ETSI, 3GPP and other
// organizations. Implementations of these standards, or the standard enabled
// platforms may require licenses from various entities, including
// Intel Corporation.
//
//
// Purpose: G7291 speech codec: tables.
//
*/

#include "owng7291.h"

__ALIGN32 CONST int abLP3k[10] =
{
0x08000000, 0xf19c8562, 0xf2672d0a, 0xfa58af70, 0xfea4c47e,
0x02ccdb5c, 0x0a6fbf9a, 0x0f4d5a48, 0x0a6fbf9a, 0x02ccdb5c,
};

/* table for bitrate switching high-band attenuation */
__ALIGN32 CONST short G7291_switching_gainTbl[G7291_COUNT_RCV_MAX + 1] = {
  0, 0, 0, 0, 0,
  1, 1, 2, 2, 3,
  3, 4, 5, 6, 8,
  10, 13, 16, 20, 25,
  32, 40, 51, 64, 81,
  102, 128, 161, 203, 256,
  323, 406, 512, 645, 813,
  1024, 1290, 1625, 2048, 2580,
  3251, 4096, 5161, 6502, 8192,
  10321, 13004, 16384, 20643, 26008,
  32767
};

/* codebook for fixed-codebook gain correction in cascade CELP (3 bits) Q14*/
__ALIGN32 CONST short G7291_GainEnhaTbl[8] = {
   5428,  7184,  8656, 10110,
  11710, 13716, 16840, 23448
};

/* codebook for fixed-codebook gain correction in cascade CELP (2 bits) Q14*/
__ALIGN32 CONST short G7291_GainEnha2bitsTbl[4] = {8211, 11599, 16384, 23143};

__ALIGN32 CONST short Sqrt_han_wind8kTbl[G7291_LFRAME4 + 1] = {
  0, 161, 322, 482, 643, 803, 963, 1122,
  1282, 1440, 1598, 1756, 1912, 2068, 2224, 2378,
  2531, 2684, 2835, 2986, 3135, 3283, 3430, 3575,
  3719, 3862, 4003, 4142, 4280, 4417, 4551, 4684,
  4815, 4944, 5072, 5197, 5320, 5442, 5561, 5678,
  5793, 5905, 6016, 6124, 6229, 6333, 6433, 6532,
  6627, 6721, 6811, 6899, 6985, 7068, 7147, 7225,
  7299, 7371, 7440, 7505, 7568, 7629, 7686, 7740,
  7791, 7839, 7884, 7927, 7966, 8002, 8035, 8064,
  8091, 8115, 8135, 8153, 8167, 8178, 8186, 8190,
  8192
};
__ALIGN32 CONST short G7291_FEC_h_lowTbl[5] = { -410, 3572, 25602, 3572, -410 };
__ALIGN32 CONST short G7291_FEC_h_highTbl[5] = { -410, -3572, 25602, -3572, -410 };
__ALIGN32 CONST short sqiTbl[7] = {4, 9, 16, 25, 36, 49, 64 };
__ALIGN32 CONST short invSqiTbl[7] = {8192, 3641, 2048, 1311, 910, 669, 512};

 /* Tables for MDCT subbands */
__ALIGN32 CONST short G7291_TDAC_sb_boundTbl[G7291_TDAC_NB_SB + 1] =
    { 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224,
  240, 256, 272, 280
};
__ALIGN32 CONST short G7291_TDAC_nb_coefTbl[G7291_TDAC_NB_SB] =
    { 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 8
};

/* MDCT TABLES */
__ALIGN32 CONST short G7291_TDAC_hTbl[G7291_TDAC_L_WIN] = {
  (short) 114, (short) 341, (short) 569, (short) 796, (short) 1023,
  (short) 1251, (short) 1478, (short) 1705, (short) 1931, (short) 2158,
  (short) 2384, (short) 2610, (short) 2836, (short) 3062, (short) 3287,
  (short) 3512, (short) 3737, (short) 3961, (short) 4185, (short) 4409,
  (short) 4632, (short) 4854, (short) 5077, (short) 5298, (short) 5520,
  (short) 5740, (short) 5960, (short) 6180, (short) 6399, (short) 6617,
  (short) 6835, (short) 7052, (short) 7268, (short) 7484, (short) 7699,
  (short) 7913, (short) 8126, (short) 8339, (short) 8551, (short) 8762,
  (short) 8972, (short) 9181, (short) 9390, (short) 9597, (short) 9804,
  (short) 10009, (short) 10214, (short) 10418, (short) 10620, (short) 10822,
  (short) 11023, (short) 11222, (short) 11421, (short) 11618, (short) 11814,
  (short) 12009, (short) 12203, (short) 12396, (short) 12588, (short) 12778,
  (short) 12967, (short) 13155, (short) 13342, (short) 13527, (short) 13711,
  (short) 13894, (short) 14075, (short) 14255, (short) 14434, (short) 14611,
  (short) 14787, (short) 14961, (short) 15134, (short) 15306, (short) 15476,
  (short) 15644, (short) 15811, (short) 15977, (short) 16141, (short) 16303,
  (short) 16464, (short) 16623, (short) 16781, (short) 16937, (short) 17092,
  (short) 17244, (short) 17395, (short) 17545, (short) 17693, (short) 17839,
  (short) 17983, (short) 18126, (short) 18266, (short) 18405, (short) 18543,
  (short) 18678, (short) 18812, (short) 18944, (short) 19074, (short) 19202,
  (short) 19329, (short) 19453, (short) 19576, (short) 19696, (short) 19815,
  (short) 19932, (short) 20047, (short) 20160, (short) 20271, (short) 20381,
  (short) 20488, (short) 20593, (short) 20696, (short) 20798, (short) 20897,
  (short) 20994, (short) 21089, (short) 21183, (short) 21274, (short) 21363,
  (short) 21450, (short) 21535, (short) 21618, (short) 21699, (short) 21777,
  (short) 21854, (short) 21929, (short) 22001, (short) 22071, (short) 22139,
  (short) 22206, (short) 22269, (short) 22331, (short) 22391, (short) 22448,
  (short) 22503, (short) 22557, (short) 22607, (short) 22656, (short) 22703,
  (short) 22747, (short) 22789, (short) 22829, (short) 22867, (short) 22903,
  (short) 22936, (short) 22967, (short) 22996, (short) 23023, (short) 23047,
  (short) 23070, (short) 23090, (short) 23108, (short) 23123, (short) 23137,
  (short) 23148, (short) 23157, (short) 23163, (short) 23168, (short) 23170,
  (short) 23170, (short) 23168, (short) 23163, (short) 23157, (short) 23148,
  (short) 23137, (short) 23123, (short) 23108, (short) 23090, (short) 23070,
  (short) 23047, (short) 23023, (short) 22996, (short) 22967, (short) 22936,
  (short) 22903, (short) 22867, (short) 22829, (short) 22789, (short) 22747,
  (short) 22703, (short) 22656, (short) 22607, (short) 22557, (short) 22503,
  (short) 22448, (short) 22391, (short) 22331, (short) 22269, (short) 22206,
  (short) 22139, (short) 22071, (short) 22001, (short) 21929, (short) 21854,
  (short) 21777, (short) 21699, (short) 21618, (short) 21535, (short) 21450,
  (short) 21363, (short) 21274, (short) 21183, (short) 21089, (short) 20994,
  (short) 20897, (short) 20798, (short) 20696, (short) 20593, (short) 20488,
  (short) 20381, (short) 20271, (short) 20160, (short) 20047, (short) 19932,
  (short) 19815, (short) 19696, (short) 19576, (short) 19453, (short) 19329,
  (short) 19202, (short) 19074, (short) 18944, (short) 18812, (short) 18678,
  (short) 18543, (short) 18405, (short) 18266, (short) 18126, (short) 17983,
  (short) 17839, (short) 17693, (short) 17545, (short) 17395, (short) 17244,
  (short) 17092, (short) 16937, (short) 16781, (short) 16623, (short) 16464,
  (short) 16303, (short) 16141, (short) 15977, (short) 15811, (short) 15644,
  (short) 15476, (short) 15306, (short) 15134, (short) 14961, (short) 14787,
  (short) 14611, (short) 14434, (short) 14255, (short) 14075, (short) 13894,
  (short) 13711, (short) 13527, (short) 13342, (short) 13155, (short) 12967,
  (short) 12778, (short) 12588, (short) 12396, (short) 12203, (short) 12009,
  (short) 11814, (short) 11618, (short) 11421, (short) 11222, (short) 11023,
  (short) 10822, (short) 10620, (short) 10418, (short) 10214, (short) 10009,
  (short) 9804, (short) 9597, (short) 9390, (short) 9181, (short) 8972,
  (short) 8762, (short) 8551, (short) 8339, (short) 8126, (short) 7913,
  (short) 7699, (short) 7484, (short) 7268, (short) 7052, (short) 6835,
  (short) 6617, (short) 6399, (short) 6180, (short) 5960, (short) 5740,
  (short) 5520, (short) 5298, (short) 5077, (short) 4854, (short) 4632,
  (short) 4409, (short) 4185, (short) 3961, (short) 3737, (short) 3512,
  (short) 3287, (short) 3062, (short) 2836, (short) 2610, (short) 2384,
  (short) 2158, (short) 1931, (short) 1705, (short) 1478, (short) 1251,
  (short) 1023, (short) 796, (short) 569, (short) 341, (short) 114
};
__ALIGN32 CONST short G7291_TDAC_nb_coef_divTbl[G7291_TDAC_NB_SB] = { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 3 };
__ALIGN32 CONST short G7291_NbDicTbl[G7291_TDAC_N_NBC + 1] = { 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 20 };
 /* Tables for VQ */
CONST short G7291_Rate8Tbl[8] = {0, 7, 10, 12, 13, 14, 15, 16};
CONST short G7291_Rate8Tbl_ind[17] = {
   0, 0, 0, 0, 7, 7, 7, 7,
   7,10,10,10,12,13,14,15,16
};
CONST short G7291_Rate16Tbl[20] = {
   0, 9, 14, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
};
CONST short G7291_Rate16Tbl_ind[33] = {
    0, 0, 0, 0, 0, 9, 9, 9,
    9, 9, 9, 9,14,14,14,14,
   16,17,18,19,20,21,22,23,
   24,25,26,27,28,29,30,31,32
};

__ALIGN32 CONST short *G7291_adRateTbl[G7291_TDAC_N_NBC + 1] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  (short *) G7291_Rate8Tbl, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  (short *) G7291_Rate16Tbl,
};
CONST short *G7291_adRateTbl_ind[G7291_TDAC_N_NBC + 1] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  (short *) G7291_Rate8Tbl_ind, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  (short *) G7291_Rate16Tbl_ind,
};


/* TDBWE table for mean time envelope quantization (Q10) */
__ALIGN32 CONST short G7291_TDBWE_MEAN_TIME_ENV_cbTbl[32] = {
  (short) - 3072, (short) - 2048, (short) - 1024, (short) - 512,
  (short) 0, (short) 512, (short) 1024, (short) 1536,
  (short) 2048, (short) 2560, (short) 3072, (short) 3584,
  (short) 4096, (short) 4608, (short) 5120, (short) 5632,
  (short) 6144, (short) 6656, (short) 7168, (short) 7680,
  (short) 8192, (short) 8704, (short) 9216, (short) 10240,
  (short) 11264, (short) 11776, (short) 12288, (short) 12800,
  (short) - 1536, (short) 9728, (short) 10752, (short) - 2560
};

/* TDBWE tables for time envelope quantization (Q10)     */
__ALIGN32 CONST short G7291_TDBWE_TIME_ENV_cbTbl[1024] = {
  (short) - 4082, (short) - 4082, (short) - 3061, (short) - 2041,
  (short) 1020, (short) 2041, (short) 1020, (short) 1020,
  (short) - 4082, (short) - 3061, (short) 0, (short) 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) - 3061, (short) - 3061, (short) - 3061, (short) - 3061,
  (short) - 3061, (short) - 2041, (short) - 1020, (short) 2041,
  (short) - 3061, (short) - 3061, (short) - 3061, (short) - 3061,
  (short) - 2041, (short) 1020, (short) 2041, (short) 2041,
  (short) - 2041, (short) - 2041, (short) - 2041, (short) - 2041,
  (short) - 2041, (short) - 2041, (short) - 2041, (short) - 2041,
  (short) - 2041, (short) - 2041, (short) - 2041, (short) - 1020,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) - 2041, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 2041, (short) - 1020, (short) 2041, (short) 4082,
  (short) 3061, (short) 3061, (short) 3061, (short) 3061,
  (short) - 1020, (short) - 2041, (short) - 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 0, (short) 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) 0, (short) 3061, (short) 4082,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) 2041, (short) 2041, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) 3061, (short) 4082, (short) 4082,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) 0,
  (short) 2041, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) 0,
  (short) 2041, (short) 2041, (short) 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) 1020,
  (short) 4082, (short) 4082, (short) 3061, (short) 3061,
  (short) - 1020, (short) - 1020, (short) 0, (short) 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) - 1020, (short) - 1020, (short) 0, (short) 2041,
  (short) 1020, (short) 0, (short) - 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) 1020,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 2041, (short) 2041,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) - 1020,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) - 1020, (short) 0, (short) 1020, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 2041, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 0, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) 1020,
  (short) - 1020, (short) 1020, (short) 1020, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 1020,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) 1020,
  (short) - 1020, (short) 1020, (short) 2041, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 2041, (short) 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) - 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 2041, (short) - 2041, (short) - 2041, (short) - 2041,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 2041,
  (short) 2041, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 2041, (short) 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) - 1020, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 2041,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 3061,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 2041, (short) 2041, (short) 2041,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) 1020, (short) 1020, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 1020,
  (short) 0, (short) 1020, (short) 1020, (short) 0,
  (short) 0, (short) 1020, (short) 1020, (short) 0,
  (short) 0, (short) 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 2041, (short) 2041, (short) 2041,
  (short) 0, (short) 2041, (short) 1020, (short) 0,
  (short) - 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) 2041, (short) 2041, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) 1020, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 1020,
  (short) 1020, (short) 0, (short) 1020, (short) 0,
  (short) - 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 1020, (short) - 1020, (short) - 1020,
  (short) 1020, (short) 0, (short) - 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) - 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) - 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) - 2041, (short) - 2041,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 1020, (short) 2041, (short) 2041, (short) 2041,
  (short) 2041, (short) 2041, (short) 2041, (short) 2041,
  (short) 2041, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 2041, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 2041, (short) 2041, (short) 2041, (short) 2041,
  (short) 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 3061, (short) 2041, (short) 2041, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) - 1020,
  (short) 0, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 0,
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) - 1020, (short) - 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) - 1020, (short) 0, (short) 0
};

/* TDBWE tables for frequency envelope quantization (Q10)*/
__ALIGN32 CONST short G7291_TDBWE_FREQ_ENV_NUMBER_ENTRIESTbl[3] = { 32, 32, 16 };
static __ALIGN32 CONST short G7291_TDBWE_FREQ_ENV_cb1Tbl[128] = {
  (short) - 12246, (short) - 12246, (short) - 10205, (short) - 6123,
  (short) - 7143, (short) 2041, (short) 2041, (short) - 7143,
  (short) - 2041, (short) - 2041, (short) - 2041, (short) - 2041,
  (short) - 2041, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 1020, (short) - 2041, (short) 1020, (short) 2041,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) - 1020, (short) 1020, (short) 1020, (short) - 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) 0, (short) 0, (short) 1020, (short) 2041,
  (short) 0, (short) 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 0, (short) 0, (short) 1020,
  (short) 1020, (short) 1020, (short) - 2041, (short) - 2041,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 1020, (short) 0, (short) 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 0,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) 1020, (short) 1020, (short) 2041, (short) 2041,
  (short) 1020, (short) 2041, (short) 2041, (short) 1020,
  (short) 2041, (short) 0, (short) - 1020, (short) - 1020,
  (short) 2041, (short) 1020, (short) - 1020, (short) - 1020,
  (short) 2041, (short) 1020, (short) 0, (short) 0,
  (short) 2041, (short) 1020, (short) 1020, (short) 1020,
  (short) 2041, (short) 2041, (short) - 5102, (short) - 5102,
  (short) 2041, (short) 2041, (short) 1020, (short) - 1020,
  (short) 2041, (short) 2041, (short) 1020, (short) 0,
  (short) 2041, (short) 2041, (short) 2041, (short) 2041,
  (short) 3061, (short) - 6123, (short) - 11225, (short) - 11225,
  (short) 3061, (short) 3061, (short) 3061, (short) 3061,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
};

static __ALIGN32 CONST short G7291_TDBWE_FREQ_ENV_cb2Tbl[128] = {
  (short) 0, (short) 0, (short) - 1020, (short) - 1020,
  (short) 2041, (short) 2041, (short) 2041, (short) 2041,
  (short) 1020, (short) 0, (short) - 1020, (short) - 2041,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 2041, (short) 2041, (short) 1020, (short) 0,
  (short) - 1020, (short) 0, (short) 1020, (short) 1020,
  (short) 0, (short) 1020, (short) 2041, (short) 2041,
  (short) 0, (short) - 1020, (short) - 1020, (short) 0,
  (short) 0, (short) 1020, (short) 1020, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) - 2041,
  (short) - 1020, (short) 0, (short) 0, (short) 0,
  (short) 1020, (short) 1020, (short) 1020, (short) 1020,
  (short) - 1020, (short) - 1020, (short) - 2041, (short) - 2041,
  (short) 1020, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) 0, (short) 0, (short) 1020,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) 1020, (short) - 1020, (short) 0, (short) - 1020,
  (short) - 7143, (short) - 6123, (short) - 6123, (short) - 7143,
  (short) 0, (short) 1020, (short) 0, (short) 0,
  (short) 0, (short) - 1020, (short) - 2041, (short) - 1020,
  (short) 2041, (short) - 7143, (short) - 11225, (short) - 11225,
  (short) - 11225, (short) - 12246, (short) - 12246, (short) - 12246,
  (short) 0, (short) - 2041, (short) - 3061, (short) - 4082,
  (short) 0, (short) 0, (short) - 2041, (short) - 3061,
  (short) - 2041, (short) - 2041, (short) - 1020, (short) 0,
  (short) 0, (short) 0, (short) 1020, (short) 1020,
  (short) 3061, (short) 3061, (short) 3061, (short) 3061,
  (short) - 2041, (short) - 1020, (short) 0, (short) 1020,
  (short) - 2041, (short) - 3061, (short) - 3061, (short) - 2041,
  (short) - 4082, (short) - 4082, (short) - 4082, (short) - 5102,
  (short) - 11225, (short) - 11225, (short) - 7143, (short) 2041,
  (short) 1020, (short) 0, (short) - 1020, (short) 0,
};

static __ALIGN32 CONST short G7291_TDBWE_FREQ_ENV_cb3Tbl[64] = {
  (short) - 3061, (short) - 3061, (short) - 2041, (short) - 1020,
  (short) 2041, (short) 2041, (short) 2041, (short) 2041,
  (short) 0, (short) 0, (short) - 2041, (short) - 2041,
  (short) - 1020, (short) - 1020, (short) - 1020, (short) - 2041,
  (short) 1020, (short) 1020, (short) 0, (short) - 1020,
  (short) 1020, (short) 1020, (short) 1020, (short) 0,
  (short) - 2041, (short) - 2041, (short) - 3061, (short) - 3061,
  (short) 0, (short) 1020, (short) 0, (short) - 1020,
  (short) - 13266, (short) - 13266, (short) - 13266, (short) - 13266,
  (short) 0, (short) 1020, (short) 1020, (short) 1020,
  (short) 0, (short) - 1020, (short) - 1020, (short) - 1020,
  (short) - 2041, (short) - 1020, (short) 0, (short) 0,
  (short) - 1020, (short) 0, (short) 0, (short) 1020,
  (short) - 6123, (short) - 9184, (short) - 9184, (short) - 5102,
  (short) 1020, (short) 0, (short) 0, (short) 0,
  (short) - 4082, (short) - 4082, (short) - 5102, (short) - 5102,
};

__ALIGN32 CONST short *G7291_TDBWE_FREQ_ENV_cbTbl[3] = {
  G7291_TDBWE_FREQ_ENV_cb1Tbl,
  G7291_TDBWE_FREQ_ENV_cb2Tbl,
  G7291_TDBWE_FREQ_ENV_cb3Tbl,
};

__ALIGN32 CONST short NormTG729_1[256] = {
   7,6,5,5,4,4,4,4,
   3,3,3,3,3,3,3,3,
   2,2,2,2,2,2,2,2,
   2,2,2,2,2,2,2,2,
   1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1,
   1,1,1,1,1,1,1,1,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1,
   -1,-1,-1,-1,-1,-1,-1,-1
};
__ALIGN32 CONST short NormTG729_12[256] = {
  15,14,13,13,12,12,12,12,
  11,11,11,11,11,11,11,11,
  10,10,10,10,10,10,10,10,
  10,10,10,10,10,10,10,10,
   9, 9, 9, 9, 9, 9, 9, 9,
   9, 9, 9, 9, 9, 9, 9, 9,
   9, 9, 9, 9, 9, 9, 9, 9,
   9, 9, 9, 9, 9, 9, 9, 9,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   8, 8, 8, 8, 8, 8, 8, 8,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7
};
/* Hamming_cos window for LPC analysis.           */
__ALIGN32 CONST short HammingWindowTbl[G7291_L_WINDOW] = {
  2621, 2623, 2629, 2638, 2651, 2668, 2689, 2713, 2741, 2772,
  2808, 2847, 2890, 2936, 2986, 3040, 3097, 3158, 3223, 3291,
  3363, 3438, 3517, 3599, 3685, 3774, 3867, 3963, 4063, 4166,
  4272, 4382, 4495, 4611, 4731, 4853, 4979, 5108, 5240, 5376,
  5514, 5655, 5800, 5947, 6097, 6250, 6406, 6565, 6726, 6890,
  7057, 7227, 7399, 7573, 7750, 7930, 8112, 8296, 8483, 8672,
  8863, 9057, 9252, 9450, 9650, 9852, 10055, 10261, 10468, 10677,
  10888, 11101, 11315, 11531, 11748, 11967, 12187, 12409, 12632, 12856,
  13082, 13308, 13536, 13764, 13994, 14225, 14456, 14688, 14921, 15155,
  15389, 15624, 15859, 16095, 16331, 16568, 16805, 17042, 17279, 17516,
  17754, 17991, 18228, 18465, 18702, 18939, 19175, 19411, 19647, 19882,
  20117, 20350, 20584, 20816, 21048, 21279, 21509, 21738, 21967, 22194,
  22420, 22644, 22868, 23090, 23311, 23531, 23749, 23965, 24181, 24394,
  24606, 24816, 25024, 25231, 25435, 25638, 25839, 26037, 26234, 26428,
  26621, 26811, 26999, 27184, 27368, 27548, 27727, 27903, 28076, 28247,
  28415, 28581, 28743, 28903, 29061, 29215, 29367, 29515, 29661, 29804,
  29944, 30081, 30214, 30345, 30472, 30597, 30718, 30836, 30950, 31062,
  31170, 31274, 31376, 31474, 31568, 31659, 31747, 31831, 31911, 31988,
  32062, 32132, 32198, 32261, 32320, 32376, 32428, 32476, 32521, 32561,
  32599, 32632, 32662, 32688, 32711, 32729, 32744, 32755, 32763, 32767,
  32767, 32741, 32665, 32537, 32359, 32129, 31850, 31521, 31143, 30716,
  30242, 29720, 29151, 28538, 27879, 27177, 26433, 25647, 24821, 23957,
  23055, 22117, 21145, 20139, 19102, 18036, 16941, 15820, 14674, 13505,
  12315, 11106, 9879, 8637, 7381, 6114, 4838, 3554, 2264, 971
};

/* Table for taming procedure test_err */
__ALIGN32 CONST short FreqPrevResetTbl[G7291_LP_ORDER] = {
  2339, 4679, 7018, 9358, 11698, 14037, 16377, 18717, 21056, 23396
};

/* Quantization of SID gain */
__ALIGN32 CONST short tab_Sidgain[32] = {
  2, 5, 8, 13, 20, 32, 50, 64,
  80, 101, 127, 160, 201, 253, 318, 401,
  505, 635, 800, 1007, 1268, 1596, 2010, 2530,
  3185, 4009, 5048, 6355, 8000, 10071, 12679, 15962
};

__ALIGN32 CONST short fg[2][G7291_MA_NP][G7291_LP_ORDER] = {
  {
   {8421, 9109, 9175, 8965, 9034, 9057, 8765, 8775, 9106, 8673}
   ,
   {7018, 7189, 7638, 7307, 7444, 7379, 7038, 6956, 6930, 6868}
   ,
   {5472, 4990, 5134, 5177, 5246, 5141, 5206, 5095, 4830, 5147}
   ,
   {4056, 3031, 2614, 3024, 2916, 2713, 3309, 3237, 2857, 3473}
   }
  ,
  {
   {8145, 8617, 8779, 8648, 8718, 8829, 8713, 8705, 8806, 8231}
   ,
   {5894, 5525, 5603, 5773, 6016, 5968, 5896, 5835, 5721, 5707}
   ,
   {4568, 3765, 3605, 3963, 4144, 4038, 4225, 4139, 3914, 4255}
   ,
   {3643, 2455, 1944, 2466, 2438, 2259, 2798, 2775, 2479, 3124}
   }
};
