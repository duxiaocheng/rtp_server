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
// Purpose: G.729/A/B/D/E/EV speech codec: Miscellaneous.
//
*/

#include <ipps.h>
#include "owng7291.h"
#include <ippdefs.h>

static __ALIGN32 CONST short tabpow[33] = {
 16384, 16743, 17109, 17484, 17867,
 18258, 18658, 19066, 19484, 19911,
 20347, 20792, 21247, 21713, 22188,
 22674, 23170, 23678, 24196, 24726,
 25268, 25821, 26386, 26964, 27554, 28158,
 28774, 29405, 30048, 30706, 31379, 32066, 32767 };

int Pow2(short exponent, short fraction)
{
  short exp, i, a, tmp;
  int L_x,L32,temp;

  i = fraction >> 10;
  a = (fraction << 5) & 0x7fff;

  L_x = tabpow[i] << 16;
  tmp = tabpow[i] - tabpow[i+1];
  L_x = L_x - 2*tmp*a;

  exp = 30 - exponent;

  if (exp >= (short)31)
    L32 = (L_x < 0L) ? -1L : 0L;
  else
    L32 = L_x >> exp;

  temp = 1<<(exp-1);
  if(L_x&temp) L32++;

  return(L32);
}
/* Table for routine ownLog2() */
static __ALIGN32 CONST short tablog[33] = {
     0,  1455,  2866,  4236,  5568,
  6863,  8124,  9352, 10549, 11716,
 12855, 13967, 15054, 16117, 17156,
 18172, 19167, 20142, 21097, 22033,
 22951, 23852, 24735, 25603, 26455, 27291,
 28113, 28922, 29716, 30497, 31266, 32023, 32767 };

void Log2(int L_x, short *logExponent, short *logFraction)
{
  short exp, i, a, tmp;
  int L_y;

  if( L_x <= 0 ){
    *logExponent = 0;
    *logFraction = 0;
    return;
  }
  exp = Norm_32s_I(&L_x);/* L_x is normalized */

  *logExponent = 30 - exp;

  i = (short)((L_x >> 25) - 32);           /* Extract b25-b31 */
  a = (short)((L_x >> 10) & 0x7fff);       /* Extract b10-b24 of fraction */
  L_y = tablog[i] << 15;
  tmp = tablog[i] - tablog[i+1];
  L_y -= tmp * a;
  *logFraction =  (short)(L_y >> 15);
  return;
}

int ownExtractBits( const unsigned char **pBits, int *nBit, int Count )
{
    int  i;
    int  ResSum = 0;

    for ( i = 0 ; i < Count ; i ++ ){
        int ExtVal, idx;
        idx = i + *nBit;
        ExtVal = ((*pBits)[idx>>3] >> (~idx & 0x7)) & 1;
        ResSum += ExtVal << (Count-i-1) ;
    }

    *pBits += (Count + *nBit)>>3;
    *nBit = (Count + *nBit) & 0x7;

    return ResSum ;
}
void _ippsRCToLAR_G7291_16s (const Ipp16s* pSrc, Ipp16s* pDst, int len) {
    short refC, Ax;
    int   i, tmp32, L_Bx;
    for(i=0; i<len; i++) {
        refC = Abs_16s(pSrc[i]);
        refC >>= 4;
        if(refC <= 1299) {
            pDst[i] = refC;
        } else {
            if(refC <= 1815) {
                Ax = 4567;
                L_Bx = 3271557L;
            } else if(refC <= 1944) {
                Ax = 11776;
                L_Bx = 16357786L;
            } else {
                Ax = 27443;
                L_Bx = 46808433L;
            }
            refC >>= 1;
            tmp32 = (refC * Ax)<<1;
            tmp32=Sub_32s(tmp32,L_Bx);
            pDst[i] = (short)(tmp32 >> 11);
        }
        if(pSrc[i] < 0) {
            pDst[i]=Cnvrt_32s16s(-pDst[i]);
        }
    }
}
void _ippsPWGammaFactor_G7291_16s ( const Ipp16s *pLAR, const Ipp16s *pLSF,
                                   Ipp16s *flat, Ipp16s *pGamma1, Ipp16s *pGamma2, Ipp16s *pMem ) {

    int   i, tmp32, L_min;
    short lar0, lar1, bwf1,bwf2;

    lar0 = pLAR[0];
    lar1 = pLAR[1];

    if(*flat != 0) {
        if((lar0 < -3562 /* -1.74 */)&&(lar1 > 1336 /* 0.65 */)) {
            *flat = 0;
        }
    } else {
        if((lar0 > -3116 /* -1.52 */) || (lar1 < 890 /* 0.43 */) ) {
            *flat = 1;
        }
    }

    if(*flat == 0) {
        for(i=0; i<G7291_LP_ORDER; i++) pMem[i]=pLSF[i]<<1;
        bwf1 = 32113 /* 0.98 */;
        L_min = pMem[1] - pMem[0];
        for(i=1; i<G7291_LP_ORDER-1; i++) {
            tmp32 = pMem[i+1] - pMem[i];
            if(tmp32 < L_min) {
                L_min = tmp32;
            }
        }

        tmp32 = 19302 * L_min;
        tmp32 >>= 15;
        tmp32 = 1024 /* 1 */ - tmp32;
        tmp32 = tmp32 << 5;
        if(tmp32 > 22938 /* 0.70 */) {
            bwf2 = 22938;
        } else if(tmp32 < 13107 /* 0.40 */) {
            bwf2 = 13107;
        } else {
            bwf2 = (short)tmp32;
        }
    } else {
        bwf1 = 30802 /* 0.94 */;
        bwf2 = 19661 /* 0.60 */;
    }
    *pGamma1 = bwf1;
    *pGamma2 = bwf2;
    return;
}
__ALIGN32 CONST short LUT1Tbl[8]={
        5,
        1,
        4,
        7,
        3,
        0,
        6,
        2
};
__ALIGN32 CONST short LUT2Tbl[16]={
        4,
        6,
        0,
        2,
        12,
        14,
        8,
        10,
        15,
        11,
        9,
        13,
        7,
        3,
        1,
        5
};
__ALIGN32 CONST short areasTbl[156] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,0,0,0
};
short calcErr_G7291(int val, int *pSrc) {
    short i, area1;
    area1 = areasTbl[IPP_MAX(0, val - G7291_LSUBFR - 10)];
    for(i=areasTbl[val + 10 - 2]; i>=area1; i--) {
        if(pSrc[i] > 983040000)
            return 1; /* Error threshold improving BWF_HARMONIC. * 60000.   */
    }
    return 0;
}
void updateExcErr_G7291( short val, int indx, int *pSrcDst) {
    int   i, area1, area2, tmp32, L_tmp1=0, tmp = 0;
    short high, low;
    tmp32 = -1;
    if(indx < G7291_LSUBFR) {
        high   = (short)(pSrcDst[0] >> 16);
        low    = (short)((pSrcDst[0]>>1) & IPP_MAX_16S);
        tmp    = high*val + ((low*val)>>15);
        tmp    = (tmp << 2) + 0x00004000L;
        tmp32  = IPP_MAX(tmp32,tmp);
        high   = (short)(tmp >> 16);
        low    = (short)((tmp>>1) & IPP_MAX_16S);
        L_tmp1 = high*val + ((low*val)>>15);
        L_tmp1 = ShiftL_32s(L_tmp1, 2);
        L_tmp1 = Add_32s(L_tmp1, 0x00004000L);
        tmp32  = IPP_MAX(tmp32,L_tmp1);
    } else {
        area1 = areasTbl[indx-G7291_LSUBFR];
        area2 = areasTbl[indx-1];
        for(i = area1; i <= area2; i++) {
            high   = (short)(pSrcDst[i] >> 16);
            low    = (short)((pSrcDst[i]>>1) & IPP_MAX_16S);
            L_tmp1 = high*val + ((low*val)>>15);
            L_tmp1 = (L_tmp1 << 2) + 0x00004000L;
            tmp32  = IPP_MAX(tmp32,L_tmp1);
        }
    }
    for(i=3; i>=1; i--) {
        pSrcDst[i] = pSrcDst[i-1];
    }
    pSrcDst[0] = tmp32;
    return;
}
void ownDecodeAdaptCodebookDelays(
short index,      /* input : received pitch index           */
short indexSubfr, /* input : subframe flag                  */
short *delayVal)  /* output: integer and fractional parts of pitch lag      */
{
  short i;
  short T0_min, T0_max;

  if(indexSubfr == 0)              /* if 1st subframe */
  {
    if(index < 197)
    {
      delayVal[0] = (index+2)/3 + 19;
      delayVal[1] = index - delayVal[0] * 3 + 58;
    }
    else
    {
      delayVal[0] = index - 112;
      delayVal[1] = 0;
    }
  }
  else                          /* second subframe */
  {
    /* find T0_min and T0_max for 2nd subframe */
    T0_min = delayVal[0] - 5;
    if (T0_min < G7291_PITCH_LAG_MIN)
    {
      T0_min = G7291_PITCH_LAG_MIN;
    }

    T0_max = T0_min + 9;
    if (T0_max > G7291_PITCH_LAG_MAX)
    {
      T0_max = G7291_PITCH_LAG_MAX;
      T0_min = T0_max - 9;
    }

    i = (index+2)/3 - 1;
    delayVal[0] = i + T0_min;
    delayVal[1] = index - 2 - i*3;
  }
  return;
}
